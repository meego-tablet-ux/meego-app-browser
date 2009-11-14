// Copyright (c) 2009 The Chromium Authors. All rights reserved.  Use of this
// source code is governed by a BSD-style license that can be found in the
// LICENSE file.

#ifndef WEBKIT_TOOLS_TEST_SHELL_SIMPLE_DATABASE_SYSTEM_H_
#define WEBKIT_TOOLS_TEST_SHELL_SIMPLE_DATABASE_SYSTEM_H_

#include "base/platform_file.h"
#include "base/ref_counted.h"
#include "base/scoped_temp_dir.h"
#include "base/string16.h"
#include "third_party/WebKit/WebKit/chromium/public/WebDatabaseObserver.h"
#include "webkit/database/database_tracker.h"

class SimpleDatabaseSystem : public webkit_database::DatabaseTracker::Observer,
                             public WebKit::WebDatabaseObserver {
 public:
  static SimpleDatabaseSystem* GetInstance();
  SimpleDatabaseSystem();
  ~SimpleDatabaseSystem();

  // VFS functions
  base::PlatformFile OpenFile(const string16& vfs_file_name,
                              int desired_flags,
                              base::PlatformFile* dir_handle);
  int DeleteFile(const string16& vfs_file_name, bool sync_dir);
  long GetFileAttributes(const string16& vfs_file_name);
  long long GetFileSize(const string16& vfs_file_name);

  // database tracker functions
  void DatabaseOpened(const string16& origin_identifier,
                      const string16& database_name,
                      const string16& description,
                      int64 estimated_size);
  void DatabaseModified(const string16& origin_identifier,
                        const string16& database_name);
  void DatabaseClosed(const string16& origin_identifier,
                      const string16& database_name);

  // DatabaseTracker::Observer implementation
  virtual void OnDatabaseSizeChanged(const string16& origin_identifier,
                                     const string16& database_name,
                                     int64 database_size,
                                     int64 space_available);

  // WebDatabaseObserver implementation
  virtual void databaseOpened(const WebKit::WebDatabase& database);
  virtual void databaseModified(const WebKit::WebDatabase& database);
  virtual void databaseClosed(const WebKit::WebDatabase& database);

  void ClearAllDatabases();

 private:
  static SimpleDatabaseSystem* instance_;

  ScopedTempDir temp_dir_;

  scoped_refptr<webkit_database::DatabaseTracker> db_tracker_;
};

#endif  // WEBKIT_TOOLS_TEST_SHELL_SIMPLE_DATABASE_SYSTEM_H_
