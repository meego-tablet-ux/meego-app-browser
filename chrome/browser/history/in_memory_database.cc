// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/history/in_memory_database.h"

#include "base/file_path.h"
#include "base/histogram.h"
#include "base/logging.h"
#include "base/time.h"
#include "base/utf_string_conversions.h"
#include "build/build_config.h"

namespace history {

InMemoryDatabase::InMemoryDatabase() : URLDatabase() {
}

InMemoryDatabase::~InMemoryDatabase() {
}

bool InMemoryDatabase::InitDB() {
  // Set the database page size to 4K for better performance.
  db_.set_page_size(4096);

  if (!db_.OpenInMemory()) {
    NOTREACHED() << "Cannot open databse " << GetDB().GetErrorMessage();
    return false;
  }

  // No reason to leave data behind in memory when rows are removed.
  db_.Execute("PRAGMA auto_vacuum=1");

  // Ensure this is really an in-memory-only cache.
  db_.Execute("PRAGMA temp_store=MEMORY");

  // Create the URL table, but leave it empty for now.
  if (!CreateURLTable(false)) {
    NOTREACHED() << "Unable to create table";
    db_.Close();
    return false;
  }

  return true;
}

bool InMemoryDatabase::InitFromScratch() {
  if (!InitDB())
    return false;

  // InitDB doesn't create the index so in the disk-loading case, it can be
  // added afterwards.
  CreateMainURLIndex();
  return true;
}

bool InMemoryDatabase::InitFromDisk(const FilePath& history_name) {
  if (!InitDB())
    return false;

  // Attach to the history database on disk.  (We can't ATTACH in the middle of
  // a transaction.)
  sql::Statement attach(GetDB().GetUniqueStatement("ATTACH ? AS history"));
  if (!attach) {
    NOTREACHED() << "Unable to attach to history database.";
    return false;
  }
#if defined(OS_POSIX)
  attach.BindString(0, history_name.value());
#else
  attach.BindString(0, WideToUTF8(history_name.value()));
#endif
  if (!attach.Run()) {
    NOTREACHED() << GetDB().GetErrorMessage();
    return false;
  }

  // Copy URL data to memory.
  base::TimeTicks begin_load = base::TimeTicks::Now();
  if (!db_.Execute(
      "INSERT INTO urls SELECT * FROM history.urls WHERE typed_count > 0")) {
    // Unable to get data from the history database. This is OK, the file may
    // just not exist yet.
  }
  base::TimeTicks end_load = base::TimeTicks::Now();
  UMA_HISTOGRAM_MEDIUM_TIMES("History.InMemoryDBPopulate",
                             end_load - begin_load);
  UMA_HISTOGRAM_COUNTS("History.InMemoryDBItemCount", db_.GetLastChangeCount());

  // Detach from the history database on disk.
  if (!db_.Execute("DETACH history")) {
    NOTREACHED() << "Unable to detach from history database.";
    return false;
  }

  // Index the table, this is faster than creating the index first and then
  // inserting into it.
  CreateMainURLIndex();

  return true;
}

sql::Connection& InMemoryDatabase::GetDB() {
  return db_;
}

}  // namespace history
