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

#ifndef CHROME_INSTALLER_UTIL_CREATE_DIR_WORK_ITEM_H__
#define CHROME_INSTALLER_UTIL_CREATE_DIR_WORK_ITEM_H__

#include <string>
#include <windows.h>
#include "chrome/installer/util/work_item.h"

// A WorkItem subclass that creates a directory with the specified path.
// It also creates all necessary intermediate paths if they do not exist.
class CreateDirWorkItem : public WorkItem {
 public:
  virtual ~CreateDirWorkItem();

  virtual bool Do();

  // Rollback tries to remove all directories created along the path.
  // If the leaf directory or one of the intermediate directories are not
  // empty, the non-empty directory and its parent directories will not be
  // removed.
  virtual void Rollback();

 private:
  friend class WorkItem;

  CreateDirWorkItem(const std::wstring& path);

  // Get the top most directory that needs to be created in order to create
  // "path_", and set "top_path_" accordingly. if "path_" already exists,
  // "top_path_" is set to empty string.
  void GetTopDirToCreate();

  // Path of the directory to be created.
  std::wstring path_;

  // The top most directory that needs to be created.
  std::wstring top_path_;

  bool rollback_needed_;
};

#endif  // CHROME_INSTALLER_UTIL_CREATE_DIR_WORK_ITEM_H__
