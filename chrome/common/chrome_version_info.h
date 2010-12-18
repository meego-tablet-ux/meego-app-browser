// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_CHROME_VERSION_INFO_H_
#define CHROME_COMMON_CHROME_VERSION_INFO_H_
#pragma once

#include <string>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"

class FileVersionInfo;

namespace chrome {

// An instance of chrome::VersionInfo has information about the
// current running build of Chrome.
class VersionInfo {
 public:
  VersionInfo();
  ~VersionInfo();

  // E.g. "Chromium" or "Google Chrome".
  std::string Name() const;

  // Version number, e.g. "6.0.490.1".
  std::string Version() const;

  // The SVN revision of this release.  E.g. "55800".
  std::string LastChange() const;

  // Whether this is an "official" release of the current Version():
  // whether knowing Version() is enough to completely determine what
  // LastChange() is.
  bool IsOfficialBuild() const;

 private:
#if defined(OS_WIN) || defined(OS_MACOSX)
  scoped_ptr<FileVersionInfo> version_info_;
#endif

  DISALLOW_COPY_AND_ASSIGN(VersionInfo);
};

}  // namespace chrome

#endif  // CHROME_COMMON_CHROME_VERSION_INFO_H_
