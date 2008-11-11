// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>
#include <windows.h>

#include "chrome/test/test_file_util.h"

#include "base/file_util.h"
#include "chrome/common/win_util.h"

namespace file_util {

// Like CopyFileNoCache but recursively copies all files and subdirectories
// in the given input directory to the output directory.
bool CopyRecursiveDirNoCache(const std::wstring& source_dir,
                             const std::wstring& dest_dir) {
  // Try to create the directory if it doesn't already exist.
  if (!CreateDirectory(dest_dir)) {
    if (GetLastError() != ERROR_ALREADY_EXISTS)
      return false;
  }

  std::vector<std::wstring> files_copied;

  std::wstring src(source_dir);
  file_util::AppendToPath(&src, L"*");

  WIN32_FIND_DATA fd;
  HANDLE fh = FindFirstFile(src.c_str(), &fd);
  if (fh == INVALID_HANDLE_VALUE)
    return false;

  do {
    std::wstring cur_file(fd.cFileName);
    if (cur_file == L"." || cur_file == L"..")
      continue;  // Skip these special entries.

    std::wstring cur_source_path(source_dir);
    file_util::AppendToPath(&cur_source_path, cur_file);

    std::wstring cur_dest_path(dest_dir);
    file_util::AppendToPath(&cur_dest_path, cur_file);

    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      // Recursively copy a subdirectory. We stripped "." and ".." already.
      if (!CopyRecursiveDirNoCache(cur_source_path, cur_dest_path)) {
        FindClose(fh);
        return false;
      }
    } else {
      // Copy the file.
      if (!::CopyFile(cur_source_path.c_str(), cur_dest_path.c_str(), false)) {
        FindClose(fh);
        return false;
      }

      // We don't check for errors from this function, often, we are copying
      // files that are in the repository, and they will have read-only set.
      // This will prevent us from evicting from the cache, but these don't
      // matter anyway.
      file_util::EvictFileFromSystemCache(cur_dest_path.c_str());
    }
  } while (FindNextFile(fh, &fd));

  FindClose(fh);
  return true;
}

}  // namespace file_util

