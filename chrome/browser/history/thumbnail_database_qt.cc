// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/thumbnail_database_qt.h"

#include <algorithm>
#include <string>

#include "app/sql/statement.h"
#include "app/sql/transaction.h"
#include "base/command_line.h"
#include "base/file_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/time.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/diagnostics/sqlite_diagnostics.h"
#include "chrome/browser/history/history_publisher.h"
#include "chrome/browser/history/url_database.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/codec/jpeg_codec.h"


namespace history {

// Version number of the database.
static const int kCurrentVersionNumber = 4;
static const int kCompatibleVersionNumber = 4;

ThumbnailDatabaseQt::ThumbnailDatabaseQt() {
  DLOG(INFO)<<__FUNCTION__;

}

ThumbnailDatabaseQt::~ThumbnailDatabaseQt() {
  DLOG(INFO)<<__FUNCTION__;

}

bool ThumbnailDatabaseQt::Init(const FilePath& db_name) {
  DLOG(INFO)<<__FUNCTION__;
  sql::InitStatus status = OpenDatabase(&db_, db_name);
  if (status != sql::INIT_OK) {
    assert(false);
    return false;
  }

  // Scope initialization in a transaction so we can't be partially initialized.
  sql::Transaction transaction(&db_);
  transaction.Begin();

  // Create the tables.
  if (!initThumbnailTable())  {
    db_.Close();
    assert(false);
    return false;
  }

  // Initialization is complete.
  if (!transaction.Commit()) {
    db_.Close();
    assert(false);
    return false;
  }

  DLOG(INFO)<<__FUNCTION__;
  return true;
}

sql::InitStatus ThumbnailDatabaseQt::OpenDatabase(sql::Connection* db,
                                                const FilePath& db_name) {
  DLOG(INFO)<<__FUNCTION__;
  // Set the exceptional sqlite error handler.
  db->set_error_delegate(GetErrorHandlerForThumbnailDb());

  db->set_page_size(2048);
  db->set_cache_size(32);

  // Run the database in exclusive mode. Nobody else should be accessing the
  // database while we're running, and this will give somewhat improved perf.
  db->set_exclusive_locking();

  if (!db->Open(db_name))
    return sql::INIT_FAILURE;

  return sql::INIT_OK;
}

bool ThumbnailDatabaseQt::initThumbnailTable() {
  DLOG(INFO)<<__FUNCTION__;
  if (!db_.DoesTableExist("rec_thumbnails")) {
    if (!db_.Execute("CREATE TABLE rec_thumbnails ("
	"url LONGVARCHAR PRIMARY KEY,"
        "bookmarked INTEGER DEFAULT 0,"
        "valid INTEGER DEFAULT 0,"
        "data BLOB)"))
      return false;
  }

  return true;
}

bool ThumbnailDatabaseQt::RecreateThumbnailTable() {
  DLOG(INFO)<<__FUNCTION__;
  if (!db_.Execute("DROP TABLE rec_thumbnails"))
    return false;
  return initThumbnailTable();
}

void ThumbnailDatabaseQt::BeginTransaction() {
  DLOG(INFO)<<__FUNCTION__;
  db_.BeginTransaction();
}

void ThumbnailDatabaseQt::CommitTransaction() {
  DLOG(INFO)<<__FUNCTION__;
  db_.CommitTransaction();
}

void ThumbnailDatabaseQt::Vacuum() {
  DLOG(INFO)<<__FUNCTION__;
  DCHECK(db_.transaction_nesting() == 0) <<
      "Can not have a transaction when vacuuming.";
  db_.Execute("VACUUM");
}

bool ThumbnailDatabaseQt::DeleteThumbnail(const std::string url) {
  DLOG(INFO)<<__FUNCTION__;
  sql::Statement statement(db_.GetCachedStatement(SQL_FROM_HERE,
      "DELETE FROM rec_thumbnails WHERE url = ?"));
  if (!statement)
    return false;

  statement.BindString(0, url);
  return statement.Run();
}

void ThumbnailDatabaseQt::CleanUnusedThumbnails(std::vector<GURL> list_url) {
  DLOG(INFO)<<__FUNCTION__;
  sql::Statement statement(db_.GetCachedStatement(SQL_FROM_HERE,
      "SELECT url FROM rec_thumbnails WHERE bookmarked=0"));
  if (!statement)
    return;
  
  int list_count = list_url.size();
  while (statement.Step()) {
    bool found = false;
    std::string url_str = statement.ColumnString(0);
    for(int i=0; i<list_count; i++) {
	if(url_str == list_url[i].spec()) {
	   found = true;
           break;
        } 
    }
    if(!found)
      DeleteThumbnail(url_str);
  }
}

int ThumbnailDatabaseQt::ThumbnailsCountExcludeBookmarked() {
  DLOG(INFO)<<__FUNCTION__;
  sql::Statement statement(db_.GetCachedStatement(SQL_FROM_HERE,
      "SELECT COUNT(*) FROM rec_thumbnails WHERE bookmarked=0"));
  if (!statement)
    return -1;

  if (!statement.Step())
    return -1;  // don't have this url

  return statement.ColumnInt(0);
}

bool ThumbnailDatabaseQt::HasThisPage(const GURL& url) {
  DLOG(INFO)<<__FUNCTION__;
  sql::Statement statement(db_.GetCachedStatement(SQL_FROM_HERE,
      "SELECT * FROM rec_thumbnails WHERE url=?"));
  if (!statement)
    return false;

  statement.BindString(0, url.spec());

  if (!statement.Step())
    return false;  // don't have this url

  DLOG(INFO)<<__FUNCTION__<<": "<<true;
  return true;
}

bool ThumbnailDatabaseQt::IsThumbnailValid(const GURL& url) {
  DLOG(INFO)<<__FUNCTION__;
  sql::Statement statement(db_.GetCachedStatement(SQL_FROM_HERE,
      "SELECT valid FROM rec_thumbnails WHERE url=?"));
  if (!statement)
    return false;

  statement.BindString(0, url.spec());

  if (!statement.Step())
    return false;  // don't have this url

  return statement.ColumnBool(0);
}

// Return false: URL not exist or not bookmarked.
bool ThumbnailDatabaseQt::IsBookmarkedPage(const GURL& url) {
  DLOG(INFO)<<__FUNCTION__;
  sql::Statement statement(db_.GetCachedStatement(SQL_FROM_HERE,
      "SELECT bookmarked FROM rec_thumbnails WHERE url=?"));
  if (!statement)
    return false;

  statement.BindString(0, url.spec());
  if (!statement.Step())
    return false;  // don't have this url

  return statement.ColumnBool(0);
}

bool ThumbnailDatabaseQt::UpdateBookmarkedColumn(const GURL& url, bool bookmarked) {
  DLOG(INFO)<<__FUNCTION__;
  sql::Statement statement(db_.GetCachedStatement(SQL_FROM_HERE,
	 "UPDATE rec_thumbnails SET bookmarked = ? WHERE url = ?"));
  if (!statement)
    return false;

  statement.BindBool(0, bookmarked);
  statement.BindString(1, url.spec());

  return statement.Run();
}

bool ThumbnailDatabaseQt::SetPageThumbnail(const GURL& url,
					const SkBitmap& thumbnail) {
  DLOG(INFO)<<__FUNCTION__;
  if (!thumbnail.isNull()) {
      sql::Statement statement(db_.GetCachedStatement(SQL_FROM_HERE,
	 "UPDATE rec_thumbnails SET data = ?, valid = ? WHERE url = ?"));
      if (!statement)
        return false;

      // We use 90 quality (out of 100) which is pretty high, because
      // we're very sensitive to artifacts for these small sized,
      // highly detailed images.
      std::vector<unsigned char> jpeg_data;
      SkAutoLockPixels thumbnail_lock(thumbnail);
      bool encoded = gfx::JPEGCodec::Encode(
          reinterpret_cast<unsigned char*>(thumbnail.getAddr32(0, 0)),
          gfx::JPEGCodec::FORMAT_SkBitmap, thumbnail.width(),
          thumbnail.height(),
          static_cast<int>(thumbnail.rowBytes()), 90,
          &jpeg_data);

      if (encoded) {
        statement.BindBlob(0, &jpeg_data[0],
                           static_cast<int>(jpeg_data.size()));
	statement.BindBool(1, true);
	statement.BindString(2, url.spec());
        if (!statement.Run()) {
          NOTREACHED() << db_.GetErrorMessage();
          return false;
        }
	return true;
      }
      return false;
  } 
}

bool ThumbnailDatabaseQt::InsertNewRow(
    const GURL& url,
    const bool bookmarked) {

  DLOG(INFO)<<__FUNCTION__;
  
  sql::Statement statement(db_.GetCachedStatement(SQL_FROM_HERE,
      "INSERT OR REPLACE INTO rec_thumbnails "
      "(url, bookmarked, valid, data) "
      "VALUES (?,?,?,?)"));
  if (!statement)
    return false;

  //char jpeg_data;

  statement.BindString(0, url.spec());
  statement.BindBool(1, bookmarked);
  statement.BindBool(2, false);
  statement.BindNull(3);
  //statement.BindBlob(3, &jpeg_data, 0);
  
  if (!statement.Run()){
    NOTREACHED() << db_.GetErrorMessage();
    return false;
  }
  return true;
}

bool ThumbnailDatabaseQt::InsertNewRow(
    const GURL& url,
    const bool bookmarked,
    const SkBitmap& thumbnail) {

  DLOG(INFO)<<__FUNCTION__;
  
  sql::Statement statement(db_.GetCachedStatement(SQL_FROM_HERE,
      "INSERT OR REPLACE INTO rec_thumbnails "
      "(url, bookmarked, valid, data) "
      "VALUES (?,?,?,?)"));
  if (!statement)
    return false;

  //char jpeg_data;

  statement.BindString(0, url.spec());
  statement.BindBool(1, bookmarked);
  statement.BindBool(2, false);
  //statement.BindBlob(3, &jpeg_data, 0);
  statement.BindNull(3);
  
  if (!statement.Run()){
    NOTREACHED() << db_.GetErrorMessage();
    return false;
  }

  return SetPageThumbnail(url, thumbnail);
}

bool ThumbnailDatabaseQt::GetPageThumbnail(const GURL& url,
                                         std::vector<unsigned char>* data) {
  DLOG(INFO)<<__FUNCTION__;
  sql::Statement statement(db_.GetCachedStatement(SQL_FROM_HERE,
      "SELECT data FROM rec_thumbnails WHERE url=?"));
  if (!statement)
    return false;

  statement.BindString(0, url.spec());
  if (!statement.Step())
    return false;  // don't have a thumbnail for this ID

  statement.ColumnBlobAsVector(0, data);
  DLOG(INFO)<<__FUNCTION__<<"got data";
  return true;
}

bool ThumbnailDatabaseQt::RenameAndDropThumbnails(const FilePath& old_db_file,
                                                const FilePath& new_db_file) {
  DLOG(INFO)<<__FUNCTION__;
  return true;
}

}  // namespace history
