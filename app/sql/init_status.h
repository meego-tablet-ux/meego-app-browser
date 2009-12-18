// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APP_SQL_INIT_STATUS_H_
#define APP_SQL_INIT_STATUS_H_

namespace sql {

// Used as the return value for some databases' init functions.
enum InitStatus {
  INIT_OK,

  // Some error, usually I/O related opening the file.
  INIT_FAILURE,

  // The database is from a future version of the app and cannot be read.
  INIT_TOO_NEW,
};

}  // namespace sql

#endif  // APP_SQL_INIT_STATUS_H_
