// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_HISTORY_THUMBNAIL_DATABASE_QT_H_
#define CHROME_BROWSER_HISTORY_THUMBNAIL_DATABASE_QT_H_
#pragma once

#include <vector>

#include "app/sql/connection.h"
#include "app/sql/init_status.h"
#include "app/sql/meta_table.h"
#include "base/gtest_prod_util.h"
#include "base/memory/ref_counted.h"
#include "base/scoped_ptr.h"
#include "chrome/browser/history/history_types.h"

class FilePath;
class RefCountedMemory;
class SkBitmap;
class GURL;

namespace base {
class Time;
}

namespace history {

static const int kRecThumbnailMaxNum = 50;

class ThumbnailDatabaseQt {
 public:
  ThumbnailDatabaseQt();
  ~ThumbnailDatabaseQt();

  // Must be called after creation but before any other methods are called.
  bool Init(const FilePath& db_name);

  // Open database on a given filename. If the file does not exist,
  // it is created.
  // |db| is the database to open.
  // |db_name| is a path to the database file.
  static sql::InitStatus OpenDatabase(sql::Connection* db,
                                      const FilePath& db_name);

  // Transactions on the database.
  void BeginTransaction();
  void CommitTransaction();
  int transaction_nesting() const {
    return db_.transaction_nesting();
  }

  // Vacuums the database. This will cause sqlite to defragment and collect
  // unused space in the file. It can be VERY SLOW.
  void Vacuum();

  // Thumbnails ----------------------------------------------------------------

  // Sets the given data to be the thumbnail for the given URL,
  // overwriting any previous data. If the SkBitmap contains no pixel
  // data, the thumbnail will be deleted.
  bool SetPageThumbnail(const GURL& url,
                        const SkBitmap& thumbnail);

  // Retrieves thumbnail data for the given URL, returning true on success,
  // false if there is no such thumbnail or there was some other error.
  bool GetPageThumbnail(const GURL& url, std::vector<unsigned char>* data);

  bool InsertNewRow(const GURL& url,
    		    const bool bookmarked,
    		    const SkBitmap& thumbnail);

  bool InsertNewRow(const GURL& url,
    		    const bool bookmarked);

  bool UpdateBookmarkedColumn(const GURL& url, bool bookmarked);

  bool HasThisPage(const GURL& url);

  // Get thumbnails count expcept bookmared page.
  int ThumbnailsCountExcludeBookmarked();

  void CleanUnusedThumbnails(std::vector<GURL> list_url);


  // Called by the to delete all old thumbnails and make a clean table.
  // Returns true on success.
  bool RecreateThumbnailTable();

  // Renames the database file and drops the Thumbnails table.
  bool RenameAndDropThumbnails(const FilePath& old_db_file,
                               const FilePath& new_db_file);

  bool IsThumbnailValid(const GURL& url);

  bool IsBookmarkedPage(const GURL& url);
 private:
  // Creates the thumbnail table, returning true if the table already exists
  // or was successfully created.
  bool initThumbnailTable();

  // Delete the thumbnail with url. Returns false on failure
  bool DeleteThumbnail(std::string url);

  sql::Connection db_;

};

}  // namespace history

#endif  // CHROME_BROWSER_HISTORY_THUMBNAIL_DATABASE_QT_H_
