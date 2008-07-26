// Copyright 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "chrome/browser/history/history_database.h"

#include <algorithm>
#include <set>

#include "base/string_util.h"

// Only needed for migration.
#include "base/file_util.h"
#include "chrome/browser/history/text_database_manager.h"

namespace history {

namespace {

// Current version number.
const int kCurrentVersionNumber = 15;

}  // namespace

HistoryDatabase::HistoryDatabase()
    : transaction_nesting_(0),
      db_(NULL) {
}

HistoryDatabase::~HistoryDatabase() {
}

InitStatus HistoryDatabase::Init(const std::wstring& history_name) {
  // Open the history database, using the narrow version of open indicates to
  // sqlite that we want the database to be in UTF-8 if it doesn't already
  // exist.
  DCHECK(!db_) << "Already initialized!";
  if (sqlite3_open(WideToUTF8(history_name).c_str(), &db_) != SQLITE_OK)
    return INIT_FAILURE;
  statement_cache_ = new SqliteStatementCache;
  DBCloseScoper scoper(&db_, &statement_cache_);

  // Set the database page size to something a little larger to give us
  // better performance (we're typically seek rather than bandwidth limited).
  // This only has an effect before any tables have been created, otherwise
  // this is a NOP. Must be a power of 2 and a max of 8192.
  sqlite3_exec(db_, "PRAGMA page_size=4096", NULL, NULL, NULL);

  // Increase the cache size. The page size, plus a little extra, times this
  // value, tells us how much memory the cache will use maximum.
  // 6000 * 4MB = 24MB
  // TODO(brettw) scale this value to the amount of available memory.
  sqlite3_exec(db_, "PRAGMA cache_size=6000", NULL, NULL, NULL);

  // Wrap the rest of init in a tranaction. This will prevent the database from
  // getting corrupted if we crash in the middle of initialization or migration.
  TransactionScoper transaction(this);

  // Make sure the statement cache is properly initialized.
  statement_cache_->set_db(db_);

  // Prime the cache. See the header file's documentation for this function.
  PrimeCache();

  // Create the tables and indices.
  // NOTE: If you add something here, also add it to
  //       RecreateAllButStarAndURLTables.
  if (!meta_table_.Init(std::string(), kCurrentVersionNumber, db_))
    return INIT_FAILURE;
  if (!CreateURLTable(false) || !InitVisitTable() ||
      !InitKeywordSearchTermsTable() || !InitDownloadTable() ||
      !InitSegmentTables() || !InitStarTable())
    return INIT_FAILURE;
  CreateMainURLIndex();
  CreateSupplimentaryURLIndices();

  // Version check.
  InitStatus version_status = EnsureCurrentVersion();
  if (version_status != INIT_OK)
    return version_status;

  EnsureStarredIntegrity();

  // Succeeded: keep the DB open by detaching the auto-closer.
  scoper.Detach();
  db_closer_.Attach(&db_, &statement_cache_);
  return INIT_OK;
}

void HistoryDatabase::BeginExclusiveMode() {
  sqlite3_exec(db_, "PRAGMA locking_mode=EXCLUSIVE", NULL, NULL, NULL);
}

void HistoryDatabase::PrimeCache() {
  // A statement must be open for the preload command to work. If the meta
  // table can't be read, it probably means this is a new database and there
  // is nothing to preload (so it's OK we do nothing).
  SQLStatement dummy;
  if (dummy.prepare(db_, "SELECT * from meta") != SQLITE_OK)
    return;
  if (dummy.step() != SQLITE_ROW)
    return;

  sqlite3Preload(db_);
}

// static
int HistoryDatabase::GetCurrentVersion() {
  return kCurrentVersionNumber;
}

void HistoryDatabase::BeginTransaction() {
  DCHECK(db_);
  if (transaction_nesting_ == 0) {
    int rv = sqlite3_exec(db_, "BEGIN TRANSACTION", NULL, NULL, NULL);
    DCHECK(rv == SQLITE_OK) << "Failed to begin transaction";
  }
  transaction_nesting_++;
}

void HistoryDatabase::CommitTransaction() {
  DCHECK(db_);
  DCHECK(transaction_nesting_ > 0) << "Committing too many transactions";
  transaction_nesting_--;
  if (transaction_nesting_ == 0) {
    int rv = sqlite3_exec(db_, "COMMIT", NULL, NULL, NULL);
    DCHECK(rv == SQLITE_OK) << "Failed to commit transaction";
  }
}

bool HistoryDatabase::RecreateAllButStarAndURLTables() {
  if (!DropVisitTable())
    return false;
  if (!InitVisitTable())
    return false;

  if (!DropKeywordSearchTermsTable())
    return false;
  if (!InitKeywordSearchTermsTable())
    return false;

  if (!DropSegmentTables())
    return false;
  if (!InitSegmentTables())
    return false;

  // We also add the supplimentary URL indices at this point. This index is
  // over parts of the URL table that weren't automatically created when the
  // temporary URL table was
  CreateSupplimentaryURLIndices();
  return true;
}

void HistoryDatabase::Vacuum() {
  DCHECK(transaction_nesting_ == 0) <<
      "Can not have a transaction when vacuuming.";
  sqlite3_exec(db_, "VACUUM", NULL, NULL, NULL);
}

bool HistoryDatabase::SetSegmentID(VisitID visit_id, SegmentID segment_id) {
  SQLStatement s;
  if (s.prepare(db_, "UPDATE visits SET segment_id = ? WHERE id = ?") !=
      SQLITE_OK) {
    NOTREACHED();
    return false;
  }
  s.bind_int64(0, segment_id);
  s.bind_int64(1, visit_id);
  return s.step() == SQLITE_DONE;
}

SegmentID HistoryDatabase::GetSegmentID(VisitID visit_id) {
  SQLStatement s;
  if (s.prepare(db_, "SELECT segment_id FROM visits WHERE id = ?")
      != SQLITE_OK) {
    NOTREACHED();
    return 0;
  }

  s.bind_int64(0, visit_id);
  if (s.step() == SQLITE_ROW) {
    if (s.column_type(0) == SQLITE_NULL)
      return 0;
    else
      return s.column_int64(0);
  }
  return 0;
}

sqlite3* HistoryDatabase::GetDB() {
  return db_;
}

SqliteStatementCache& HistoryDatabase::GetStatementCache() {
  return *statement_cache_;
}

// Migration -------------------------------------------------------------------

InitStatus HistoryDatabase::EnsureCurrentVersion() {
  // We can't read databases newer than we were designed for.
  if (meta_table_.GetCompatibleVersionNumber() > kCurrentVersionNumber)
    return INIT_TOO_NEW;

  // NOTICE: If you are changing structures for things shared with the archived
  // history file like URLs, visits, or downloads, that will need migration as
  // well. Instead of putting such migration code in this class, it should be
  // in the corresponding file (url_database.cc, etc.) and called from here and
  // from the archived_database.cc.

  // When the version is too old, we just try to continue anyway, there should
  // not be a released product that makes a database too old for us to handle.
  int cur_version = meta_table_.GetVersionNumber();

  // Put migration code here

  LOG_IF(WARNING, cur_version < kCurrentVersionNumber) <<
      "History database version " << cur_version << " is too old to handle.";

  return INIT_OK;
}

}  // namespace history
