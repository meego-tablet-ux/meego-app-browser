// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file declares util functions for setup project.

#include "chrome/installer/setup/setup_util.h"

#include "base/file_util.h"
#include "base/logging.h"
#include "chrome/installer/util/master_preferences.h"
#include "chrome/installer/util/util_constants.h"
#include "courgette/courgette.h"
#include "third_party/bspatch/mbspatch.h"

int setup_util::ApplyDiffPatch(const FilePath& src,
                               const FilePath& patch,
                               const FilePath& dest) {
  VLOG(1) << "Applying patch " << patch.value() << " to file " << src.value()
          << " and generating file " << dest.value();

  // Try Courgette first.  Courgette checks the patch file first and fails
  // quickly if the patch file does not have a valid Courgette header.
  courgette::Status patch_status =
      courgette::ApplyEnsemblePatch(src.value().c_str(),
                                    patch.value().c_str(),
                                    dest.value().c_str());
  if (patch_status == courgette::C_OK)
    return 0;

  VLOG(1) << "Failed to apply patch " << patch.value() << " using courgette.";
  return ApplyBinaryPatch(src.value().c_str(), patch.value().c_str(),
                          dest.value().c_str());
}

installer::Version* setup_util::GetVersionFromArchiveDir(
    const FilePath& chrome_path) {
  VLOG(1) << "Looking for Chrome version folder under " << chrome_path.value();
  FilePath root_path = chrome_path.Append(L"*");

  // TODO(tommi): The version directory really should match the version of
  // setup.exe.  To begin with, we should at least DCHECK that that's true.

  // TODO(tommi): use file_util::FileEnumerator.
  WIN32_FIND_DATA find_data = {0};
  HANDLE file_handle = FindFirstFile(root_path.value().c_str(), &find_data);
  BOOL ret = TRUE;
  installer::Version* version = NULL;
  // Here we are assuming that the installer we have is really valid so there
  // can not be two version directories. We exit as soon as we find a valid
  // version directory.
  while (ret) {
    if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY &&
        lstrcmpW(find_data.cFileName, L"..") != 0 &&
        lstrcmpW(find_data.cFileName, L".") != 0) {
      VLOG(1) << "directory found: " << find_data.cFileName;
      version = installer::Version::GetVersionFromString(find_data.cFileName);
      if (version)
        break;
    }
    ret = FindNextFile(file_handle, &find_data);
  }
  FindClose(file_handle);

  return version;
}
