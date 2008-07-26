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

#include "chrome/installer/util/copy_tree_work_item.h"

#include <shlwapi.h>
#include "base/file_util.h"
#include "chrome/installer/util/logging_installer.h"

CopyTreeWorkItem::~CopyTreeWorkItem() {
  if (file_util::PathExists(backup_path_)) {
    file_util::Delete(backup_path_, true);
  }
}

CopyTreeWorkItem::CopyTreeWorkItem(std::wstring source_path,
                                   std::wstring dest_path,
                                   std::wstring temp_dir,
                                   CopyOverWriteOption overwrite_option,
                                   std::wstring alternative_path)
    : source_path_(source_path),
      dest_path_(dest_path),
      temp_dir_(temp_dir),
      overwrite_option_(overwrite_option),
      alternative_path_(alternative_path),
      copied_to_dest_path_(false),
      moved_to_backup_(false),
      copied_to_alternate_path_(false) {
}

bool CopyTreeWorkItem::Do() {
  if (!file_util::PathExists(source_path_)) {
    LOG(ERROR) << source_path_ << " does not exist";
    return false;
  }

  bool dest_exist = file_util::PathExists(dest_path_);
  // handle overwrite_option_ = IF_DIFFERENT case
  if ((dest_exist) &&
      (overwrite_option_ == WorkItem::IF_DIFFERENT) && // only for single file
      (!PathIsDirectory(source_path_.c_str())) &&
      (!PathIsDirectory(dest_path_.c_str())) &&
      (file_util::ContentsEqual(source_path_, dest_path_))) {
    LOG(INFO) << "Source file " << source_path_
              << " and destination file " << dest_path_
              << " are exactly same. Returning true.";
    return true;
  }

  // handle overwrite_option_ = RENAME_IF_IN_USE case
  if ((dest_exist) &&
      (overwrite_option_ == WorkItem::RENAME_IF_IN_USE) && // only for a file
      (!PathIsDirectory(source_path_.c_str())) &&
      (!PathIsDirectory(dest_path_.c_str())) &&
      (IsFileInUse(dest_path_))) {
    if (alternative_path_.empty() ||
        file_util::PathExists(alternative_path_) ||
        !file_util::CopyFile(source_path_, alternative_path_)) {
      LOG(ERROR) << "failed to copy " << source_path_ <<
                    " to " << alternative_path_;
      return false;
    } else {
      copied_to_alternate_path_ = true;
      LOG(INFO) << "Copied source file " << source_path_
                << " to alternative path " << alternative_path_;
      return true;
    }
  }

  // All other cases where we move dest if it exists, and copy the files
  if (dest_exist) {
    if (!GetBackupPath())
      return false;

    if (file_util::Move(dest_path_, backup_path_)) {
      moved_to_backup_ = true;
      LOG(INFO) << "Moved destination " << dest_path_
                << " to backup path " << backup_path_;
    } else {
      LOG(ERROR) << "failed moving " << dest_path_ << " to " << backup_path_;
      return false;
    }
  }

  if (file_util::CopyDirectory(source_path_, dest_path_, true)) {
    copied_to_dest_path_ = true;
    LOG(INFO) << "Copied source " << source_path_
              << " to destination " << dest_path_;
  } else {
    LOG(ERROR) << "failed copy " << source_path_ << " to " << dest_path_;
    return false;
  }

  return true;
}

void CopyTreeWorkItem::Rollback() {
  // Normally the delete operations below should not fail unless some
  // programs like anti-virus are inpecting the files we just copied.
  // If this does happen sometimes, we may consider using Move instead of
  // Delete here. For now we just log the error and continue with the
  // rest of rollback operation.
  if (copied_to_dest_path_ && !file_util::Delete(dest_path_, true)) {
    LOG(ERROR) << "Can not delete " << dest_path_;
  }
  if (moved_to_backup_ && !file_util::Move(backup_path_, dest_path_)) {
    LOG(ERROR) << "failed move " << backup_path_ << " to " << dest_path_;
  }
  if (copied_to_alternate_path_ &&
      !file_util::Delete(alternative_path_, true)) {
    LOG(ERROR) << "Can not delete " << alternative_path_;
  }
}

bool CopyTreeWorkItem::IsFileInUse(std::wstring path) {
  if (!file_util::PathExists(path))
    return false;

  HANDLE handle = ::CreateFile(path.c_str(), FILE_ALL_ACCESS,
                               NULL, NULL, OPEN_EXISTING, NULL, NULL);
  if (handle  == INVALID_HANDLE_VALUE)
    return true;

  CloseHandle(handle);
  return false;
}

bool CopyTreeWorkItem::GetBackupPath() {
  std::wstring file_name = file_util::GetFilenameFromPath(dest_path_);
  backup_path_.assign(temp_dir_);
  file_util::AppendToPath(&backup_path_, file_name);

  if (file_util::PathExists(backup_path_)) {
    // Ideally we should not fail immediately. Instead we could try some
    // random paths under temp_dir_ until we reach certain limit.
    // For now our caller always provides a good temporary directory so
    // we don't bother.
    LOG(ERROR) << "backup path " << backup_path_ << " already exists";
    return false;
  }

  return true;
}
