// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/platform_file.h"

#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include "base/logging.h"
#include "base/string_util.h"

namespace base {

// TODO(erikkay): does it make sense to support PLATFORM_FILE_EXCLUSIVE_* here?
PlatformFile CreatePlatformFile(const std::wstring& name,
                                int flags,
                                bool* created) {
  int open_flags = 0;
  if (flags & PLATFORM_FILE_CREATE)
    open_flags = O_CREAT | O_EXCL;

  if (flags & PLATFORM_FILE_CREATE_ALWAYS) {
    DCHECK(!open_flags);
    open_flags = O_CREAT | O_TRUNC;
  }

  if (!open_flags && !(flags & PLATFORM_FILE_OPEN) &&
      !(flags & PLATFORM_FILE_OPEN_ALWAYS)) {
    NOTREACHED();
    errno = EOPNOTSUPP;
    return kInvalidPlatformFileValue;
  }

  if (flags & PLATFORM_FILE_WRITE && flags & PLATFORM_FILE_READ) {
    open_flags |= O_RDWR;
  } else if (flags & PLATFORM_FILE_WRITE) {
    open_flags |= O_WRONLY;
  } else if (!(flags & PLATFORM_FILE_READ)) {
    NOTREACHED();
  }

  DCHECK(O_RDONLY == 0);

  int descriptor = open(WideToUTF8(name).c_str(), open_flags,
                        S_IRUSR | S_IWUSR);

  if (flags & PLATFORM_FILE_OPEN_ALWAYS) {
    if (descriptor > 0) {
      if (created)
        *created = false;
    } else {
      open_flags |= O_CREAT;
      if (flags & PLATFORM_FILE_EXCLUSIVE_READ ||
          flags & PLATFORM_FILE_EXCLUSIVE_WRITE) {
        open_flags |= O_EXCL;   // together with O_CREAT implies O_NOFOLLOW
      }
      descriptor = open(WideToUTF8(name).c_str(), open_flags,
                        S_IRUSR | S_IWUSR);
      if (created && descriptor > 0)
        *created = true;
    }
  }

  if ((descriptor > 0) && (flags & PLATFORM_FILE_DELETE_ON_CLOSE)) {
    unlink(WideToUTF8(name).c_str());
  }

  return descriptor;
}

bool ClosePlatformFile(PlatformFile file) {
  return close(file);
}

}  // namespace base
