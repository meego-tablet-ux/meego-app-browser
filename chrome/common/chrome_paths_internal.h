// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_CHROME_PATHS_INTERNAL_H_
#define CHROME_COMMON_CHROME_PATHS_INTERNAL_H_

#include "build/build_config.h"
#include "base/file_path.h"

namespace chrome {

// Get the path to the user's data directory, regardless of whether
// DIR_USER_DATA has been overridden by a command-line option.
bool GetDefaultUserDataDirectory(FilePath* result);

// This returns the base directory in which Chrome Frame stores user profiles.
// Note that this cannot be wrapped in a preprocessor define since
// CF and Google Chrome want to share the same binaries.
bool GetChromeFrameUserDataDirectory(FilePath* result);

#if defined(OS_LINUX)
// Get the path to the user's cache directory.
bool GetUserCacheDirectory(FilePath* result);
#endif

// Get the path to the user's documents directory.
bool GetUserDocumentsDirectory(FilePath* result);

// Get the path to the user's downloads directory.
bool GetUserDownloadsDirectory(FilePath* result);

// The path to the user's desktop.
bool GetUserDesktop(FilePath* result);

#if defined(OS_MACOSX)
// The "versioned directory" is a directory in the browser .app bundle.  It
// contains the bulk of the application, except for the things that the system
// requires be located at spepcific locations.  The versioned directory is
// in the .app at Contents/Versions/w.x.y.z.
FilePath GetVersionedDirectory();

// Most of the application is further contained within the framework.  The
// framework bundle is located within the versioned directory at a specific
// path.  The only components in the versioned directory not included in the
// framework are things that also depend on the framework, such as the helper
// app bundle.
FilePath GetFrameworkBundlePath();
#endif  // OS_MACOSX

}  // namespace chrome

#endif  // CHROME_COMMON_CHROME_PATHS_INTERNAL_H_
