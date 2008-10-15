// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// See the corresponding header file for description of the functions in this
// file.

#include "install_util.h"

#include <windows.h>

#include "base/logging.h"
#include "base/registry.h"
#include "base/string_util.h"
#include "base/win_util.h"
#include "chrome/installer/util/browser_distribution.h"
#include "chrome/installer/util/google_update_constants.h"


std::wstring InstallUtil::GetChromeUninstallCmd(bool system_install) {
  HKEY root = system_install ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  RegKey key(root, dist->GetUninstallRegPath().c_str());
  std::wstring uninstall_cmd;
  key.ReadValue(installer_util::kUninstallStringField, &uninstall_cmd);
  return uninstall_cmd;
}

installer::Version* InstallUtil::GetChromeVersion(bool system_install) {
  RegKey key;
  std::wstring version_str;

  HKEY reg_root = (system_install) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  if (!key.Open(reg_root, dist->GetVersionKey().c_str(), KEY_READ) ||
      !key.ReadValue(google_update::kRegVersionField, &version_str)) {
    LOG(INFO) << "No existing Chrome install found.";
    key.Close();
    return NULL;
  }
  key.Close();
  LOG(INFO) << "Existing Chrome version found " << version_str;
  return installer::Version::GetVersionFromString(version_str);
}

bool InstallUtil::IsOSSupported() {
  int major, minor;
  win_util::WinVersion version = win_util::GetWinVersion();
  win_util::GetServicePackLevel(&major, &minor);

  // We do not support Win2K or older, or XP without service pack 1.
  LOG(INFO) << "Windows Version: " << version
            << ", Service Pack: " << major << "." << minor;
  if ((version == win_util::WINVERSION_VISTA) ||
      (version == win_util::WINVERSION_SERVER_2003) ||
      (version == win_util::WINVERSION_XP && major >= 1)) {
    return true;
  }
  return false;
}
