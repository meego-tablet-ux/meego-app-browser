// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file defines a specific implementation of BrowserDistribution class for
// Chrome Frame. It overrides the bare minimum of methods necessary to get a
// Chrome Frame installer that does not interact with Google Chrome or
// Chromium installations.

#include "chrome/installer/util/chrome_frame_distribution.h"

#include "chrome/installer/util/l10n_string_util.h"
#include "chrome/installer/util/google_update_constants.h"

#include "installer_util_strings.h"

std::wstring ChromeFrameDistribution::GetApplicationName() {
  // TODO(robertshield): localize
  return L"Google Chrome Frame";
}

std::wstring ChromeFrameDistribution::GetAlternateApplicationName() {
  // TODO(robertshield): localize
  return L"Chromium technology in your existing browser";
}

std::wstring ChromeFrameDistribution::GetInstallSubDir() {
  // TODO(robertshield): localize
  return L"Google\\Chrome Frame";
}

std::wstring ChromeFrameDistribution::GetPublisherName() {
  const std::wstring& publisher_name =
      installer_util::GetLocalizedString(IDS_ABOUT_VERSION_COMPANY_NAME_BASE);
  return publisher_name;
}

std::wstring ChromeFrameDistribution::GetAppDescription() {
  return L"Chrome in a Frame.";
}

std::wstring ChromeFrameDistribution::GetStateKey() {
  std::wstring key(google_update::kRegPathClientState);
  key.append(L"\\");
  key.append(google_update::kChromeGuid);
  return key;
}

std::wstring ChromeFrameDistribution::GetStateMediumKey() {
  std::wstring key(google_update::kRegPathClientStateMedium);
  key.append(L"\\");
  key.append(google_update::kChromeGuid);
  return key;
}

std::wstring ChromeFrameDistribution::GetStatsServerURL() {
  return L"https://clients4.google.com/firefox/metrics/collect";
}


std::wstring ChromeFrameDistribution::GetUninstallLinkName() {
  return L"Uninstall Chrome Frame";
}

std::wstring ChromeFrameDistribution::GetUninstallRegPath() {
  return L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\"
         L"Google Chrome Frame";
}

std::wstring ChromeFrameDistribution::GetVersionKey() {
  std::wstring key(google_update::kRegPathClients);
  key.append(L"\\");
  key.append(google_update::kChromeGuid);
  return key;
}

int ChromeFrameDistribution::GetInstallReturnCode(
    installer_util::InstallStatus status) {
  switch (status) {
    case installer_util::FIRST_INSTALL_SUCCESS:
    case installer_util::INSTALL_REPAIRED:
    case installer_util::NEW_VERSION_UPDATED:
    case installer_util::HIGHER_VERSION_EXISTS:
      return 0;  // For Google Update's benefit we need to return 0 for success
    default:
      return status;
  }
}
