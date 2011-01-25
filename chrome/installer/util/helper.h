// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file contains helper functions used by setup.

#ifndef CHROME_INSTALLER_UTIL_HELPER_H_
#define CHROME_INSTALLER_UTIL_HELPER_H_
#pragma once

#include <string>

class BrowserDistribution;
class CommandLine;
class FilePath;

namespace installer {

// This function returns the install path for Chrome depending on whether its
// system wide install or user specific install.
// system_install: if true, the function returns system wide location
//                 (ProgramFiles\Google). Otherwise it returns user specific
//                 location (Document And Settings\<user>\Local Settings...)
FilePath GetChromeInstallPath(bool system_install, BrowserDistribution* dist);

// This function returns the path to the directory that holds the user data,
// this is always inside "Document And Settings\<user>\Local Settings.". Note
// that this is the default user data directory and does not take into account
// that it can be overriden with a command line parameter.
FilePath GetChromeUserDataPath(BrowserDistribution* dist);

// Returns the distribution corresponding to the current process's binaries.
// In the case of a multi-install product, this will be the CHROME_BINARIES
// distribution.
BrowserDistribution* GetBinariesDistribution(bool system_install);

// Returns the app guid under which the current process receives updates from
// Google Update.
std::wstring GetAppGuidForUpdates(bool system_install);

}  // namespace installer

#endif  // CHROME_INSTALLER_UTIL_HELPER_H_
