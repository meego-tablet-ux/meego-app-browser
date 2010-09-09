// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/platform_file.h"

#include "base/file_path.h"
#include "base/logging.h"

namespace base {

PlatformFile CreatePlatformFile(const FilePath& name,
                                int flags,
                                bool* created,
                                PlatformFileError* error_code) {
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

  if (flags & PLATFORM_FILE_TRUNCATE) {
    DCHECK(!disposition);
    DCHECK(flags & PLATFORM_FILE_WRITE);
    disposition = TRUNCATE_EXISTING;
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
  if (flags & PLATFORM_FILE_WRITE_ATTRIBUTES)
    create_flags |= FILE_WRITE_ATTRIBUTES;

  HANDLE file = CreateFile(name.value().c_str(), access, sharing, NULL,
                           disposition, create_flags, NULL);

  if (created && (INVALID_HANDLE_VALUE != file)) {
    if (flags & PLATFORM_FILE_OPEN_ALWAYS)
      *created = (ERROR_ALREADY_EXISTS != GetLastError());
    else if (flags & PLATFORM_FILE_CREATE_ALWAYS)
      *created = true;
  }

  if (error_code) {
    if (file != kInvalidPlatformFileValue)
      *error_code = PLATFORM_FILE_OK;
    else {
      DWORD last_error = GetLastError();
      switch (last_error) {
        case ERROR_SHARING_VIOLATION:
          *error_code = PLATFORM_FILE_ERROR_IN_USE;
          break;
        case ERROR_FILE_EXISTS:
          *error_code = PLATFORM_FILE_ERROR_EXISTS;
          break;
        case ERROR_FILE_NOT_FOUND:
          *error_code = PLATFORM_FILE_ERROR_NOT_FOUND;
          break;
        case ERROR_ACCESS_DENIED:
          *error_code = PLATFORM_FILE_ERROR_ACCESS_DENIED;
          break;
        default:
          *error_code = PLATFORM_FILE_ERROR_FAILED;
      }
    }
  }

  return file;
}

PlatformFile CreatePlatformFile(const std::wstring& name, int flags,
                                bool* created) {
  return CreatePlatformFile(FilePath::FromWStringHack(name), flags,
                            created, NULL);
}

bool ClosePlatformFile(PlatformFile file) {
  return (CloseHandle(file) != 0);
}

int ReadPlatformFile(PlatformFile file, int64 offset, char* data, int size) {
  if (file == kInvalidPlatformFileValue)
    return -1;

  LARGE_INTEGER offset_li;
  offset_li.QuadPart = offset;

  OVERLAPPED overlapped = {0};
  overlapped.Offset = offset_li.LowPart;
  overlapped.OffsetHigh = offset_li.HighPart;

  DWORD bytes_read;
  if (::ReadFile(file, data, size, &bytes_read, &overlapped) != 0)
    return bytes_read;
  else if (ERROR_HANDLE_EOF == GetLastError())
    return 0;

  return -1;
}

int WritePlatformFile(PlatformFile file, int64 offset,
                      const char* data, int size) {
  if (file == kInvalidPlatformFileValue)
    return -1;

  LARGE_INTEGER offset_li;
  offset_li.QuadPart = offset;

  OVERLAPPED overlapped = {0};
  overlapped.Offset = offset_li.LowPart;
  overlapped.OffsetHigh = offset_li.HighPart;

  DWORD bytes_written;
  if (::WriteFile(file, data, size, &bytes_written, &overlapped) != 0)
    return bytes_written;

  return -1;
}

bool TruncatePlatformFile(PlatformFile file, int64 length) {
  if (file == kInvalidPlatformFileValue)
    return false;

  // Get the current file pointer.
  LARGE_INTEGER file_pointer;
  LARGE_INTEGER zero;
  zero.QuadPart = 0;
  if (::SetFilePointerEx(file, zero, &file_pointer, FILE_CURRENT) == 0)
    return false;

  LARGE_INTEGER length_li;
  length_li.QuadPart = length;
  // If length > file size, SetFilePointerEx() should extend the file
  // with zeroes on all Windows standard file systems (NTFS, FATxx).
  if (!::SetFilePointerEx(file, length_li, NULL, FILE_BEGIN))
    return false;

  // Set the new file length and move the file pointer to its old position.
  // This is consistent with ftruncate()'s behavior, even when the file
  // pointer points to a location beyond the end of the file.
  return ((::SetEndOfFile(file) != 0) &&
          (::SetFilePointerEx(file, file_pointer, NULL, FILE_BEGIN) != 0));
}

bool FlushPlatformFile(PlatformFile file) {
  return ((file != kInvalidPlatformFileValue) && ::FlushFileBuffers(file));
}

bool TouchPlatformFile(PlatformFile file, const base::Time& last_access_time,
                       const base::Time& last_modified_time) {
  if (file == kInvalidPlatformFileValue)
    return false;

  FILETIME last_access_filetime = last_access_time.ToFileTime();
  FILETIME last_modified_filetime = last_modified_time.ToFileTime();
  return (::SetFileTime(file, NULL, &last_access_filetime,
                        &last_modified_filetime) != 0);
}

bool GetPlatformFileInfo(PlatformFile file, PlatformFileInfo* info) {
  if (!info)
    return false;

  FILE_BASIC_INFO basic_info;
  FILE_STANDARD_INFO standard_info;
  if (!::GetFileInformationByHandleEx(
          file, FileBasicInfo, &basic_info, sizeof(basic_info)) ||
      !::GetFileInformationByHandleEx(
          file, FileStandardInfo, &standard_info, sizeof(standard_info)))
    return false;

  info->size = standard_info.EndOfFile.QuadPart;
  info->is_directory = (standard_info.Directory != 0);
  info->last_modified = base::Time::FromLargeInteger(basic_info.LastWriteTime);
  info->last_accessed = base::Time::FromLargeInteger(basic_info.LastAccessTime);
  info->creation_time = base::Time::FromLargeInteger(basic_info.CreationTime);
  return true;
}

}  // namespace disk_cache
