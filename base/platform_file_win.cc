// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/platform_file.h"

#include "base/file_path.h"
#include "base/logging.h"

namespace base {

PlatformFile CreatePlatformFile(const FilePath& name,
                                int flags,
                                bool* created) {
  DWORD disposition = 0;

  if (flags & PLATFORM_FILE_OPEN)
    disposition = OPEN_EXISTING;

  if (flags & PLATFORM_FILE_CREATE) {
    DCHECK(!disposition);
    disposition = CREATE_NEW;
  }

  if (flags & PLATFORM_FILE_OPEN_ALWAYS) {
    DCHECK(!disposition);
    disposition = OPEN_ALWAYS;
  }

  if (flags & PLATFORM_FILE_CREATE_ALWAYS) {
    DCHECK(!disposition);
    disposition = CREATE_ALWAYS;
  }

  if (!disposition) {
    NOTREACHED();
    return NULL;
  }

  DWORD access = (flags & PLATFORM_FILE_READ) ? GENERIC_READ : 0;
  if (flags & PLATFORM_FILE_WRITE)
    access |= GENERIC_WRITE;

  DWORD sharing = (flags & PLATFORM_FILE_EXCLUSIVE_READ) ? 0 : FILE_SHARE_READ;
  if (!(flags & PLATFORM_FILE_EXCLUSIVE_WRITE))
    sharing |= FILE_SHARE_WRITE;

  DWORD create_flags = 0;
  if (flags & PLATFORM_FILE_ASYNC)
    create_flags |= FILE_FLAG_OVERLAPPED;
  if (flags & PLATFORM_FILE_TEMPORARY)
    create_flags |= FILE_ATTRIBUTE_TEMPORARY;
  if (flags & PLATFORM_FILE_HIDDEN)
    create_flags |= FILE_ATTRIBUTE_HIDDEN;
  if (flags & PLATFORM_FILE_DELETE_ON_CLOSE)
    create_flags |= FILE_FLAG_DELETE_ON_CLOSE;

  HANDLE file = CreateFile(name.value().c_str(), access, sharing, NULL,
                           disposition, create_flags, NULL);

  if ((flags & PLATFORM_FILE_OPEN_ALWAYS) && created &&
      INVALID_HANDLE_VALUE != file) {
    *created = (ERROR_ALREADY_EXISTS != GetLastError());
  }

  return file;
}

PlatformFile CreatePlatformFile(const std::wstring& name, int flags,
                                bool* created) {
  return CreatePlatformFile(FilePath::FromWStringHack(name), flags, created);
}

bool ClosePlatformFile(PlatformFile file) {
  return (CloseHandle(file) == 0);
}

bool GetPlatformFileSize(PlatformFile file, uint64* out_size) {
  LARGE_INTEGER size;
  if (!GetFileSizeEx(file, &size))
    return false;
  *out_size = size.QuadPart;
  return true;
}

}  // namespace disk_cache
