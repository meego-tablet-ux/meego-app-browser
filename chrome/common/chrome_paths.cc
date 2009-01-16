// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#endif

#include "chrome/common/chrome_paths.h"

#include "base/command_line.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/sys_info.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_switches.h"

namespace chrome {

// Gets the default user data directory, regardless of whether
// DIR_USER_DATA has been overridden by a command-line option.
bool GetDefaultUserDataDirectory(std::wstring* result) {
#if defined(OS_WIN)
  if (!PathService::Get(base::DIR_LOCAL_APP_DATA, result))
    return false;
#if defined(GOOGLE_CHROME_BUILD)
  file_util::AppendToPath(result, L"Google");
#endif
  file_util::AppendToPath(result, chrome::kBrowserAppName);
  file_util::AppendToPath(result, chrome::kUserDataDirname);
  return true;
#elif defined(OS_MACOSX)
  if (!PathService::Get(base::DIR_LOCAL_APP_DATA, result))
    return false;
  return true;
#elif defined(OS_LINUX)
  // TODO(port): Decide what to do on linux.
  NOTIMPLEMENTED();
  return false;
#endif  // defined(OS_WIN)
}

bool GetGearsPluginPathFromCommandLine(std::wstring *path) {
#ifndef NDEBUG
  // for debugging, support a cmd line based override
  CommandLine command_line;
  *path = command_line.GetSwitchValue(switches::kGearsPluginPathOverride);
  return !path->empty();
#else
  return false;
#endif
}

bool PathProvider(int key, FilePath* result) {
  // Some keys are just aliases...
  switch (key) {
    case chrome::DIR_APP:
      return PathService::Get(base::DIR_MODULE, result);
    case chrome::DIR_LOGS:
#ifndef NDEBUG
      return PathService::Get(chrome::DIR_USER_DATA, result);
#else
      return PathService::Get(base::DIR_EXE, result);
#endif
    case chrome::FILE_RESOURCE_MODULE:
      return PathService::Get(base::FILE_MODULE, result);
  }

  // Assume that we will not need to create the directory if it does not exist.
  // This flag can be set to true for the cases where we want to create it.
  bool create_dir = false;

  std::wstring cur;
  switch (key) {
    case chrome::DIR_USER_DATA:
      if (!GetDefaultUserDataDirectory(&cur))
        return false;
      create_dir = true;
      break;
    case chrome::DIR_USER_DOCUMENTS:
#if defined(OS_WIN)
      {
        wchar_t path_buf[MAX_PATH];
        if (FAILED(SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL,
                                   SHGFP_TYPE_CURRENT, path_buf)))
          return false;
        cur.assign(path_buf);
      }
#else
      // TODO(port): Get the path (possibly using xdg-user-dirs)
      // or decide we don't need it on other platforms.
      NOTIMPLEMENTED();
      return false;
#endif
      create_dir = true;
      break;
    case chrome::DIR_DEFAULT_DOWNLOADS:
      // On Vista, we can get the download path using a Win API
      // (http://msdn.microsoft.com/en-us/library/bb762584(VS.85).aspx),
      // but it can be set to Desktop, which is dangerous. Instead,
      // we just use 'Downloads' under DIR_USER_DOCUMENTS. Localizing
      // 'downloads' is not a good idea because Chrome's UI language
      // can be changed. 
      if (!PathService::Get(chrome::DIR_USER_DOCUMENTS, &cur))
        return false;
      file_util::AppendToPath(&cur, L"Downloads");
      // TODO(port): This will fail on other platforms unless we 
      // implement DIR_USER_DOCUMENTS or use xdg-user-dirs to 
      // get the download directory independently of DIR_USER_DOCUMENTS.
      break;
    case chrome::DIR_CRASH_DUMPS:
      // The crash reports are always stored relative to the default user data
      // directory.  This avoids the problem of having to re-initialize the
      // exception handler after parsing command line options, which may
      // override the location of the app's profile directory.
      if (!GetDefaultUserDataDirectory(&cur))
        return false;
      file_util::AppendToPath(&cur, L"Crash Reports");
      create_dir = true;
      break;
    case chrome::DIR_USER_DESKTOP:
#if defined(OS_WIN)
      {
        // We need to go compute the value. It would be nice to support paths
        // with names longer than MAX_PATH, but the system functions don't seem
        // to be designed for it either, with the exception of GetTempPath
        // (but other things will surely break if the temp path is too long,
        // so we don't bother handling it.
        wchar_t system_buffer[MAX_PATH];
        system_buffer[0] = 0;
        if (FAILED(SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL,
                                   SHGFP_TYPE_CURRENT, system_buffer)))
          return false;
        cur.assign(system_buffer);
      }
#else
      // TODO(port): Get the path (possibly using xdg-user-dirs)
      // or decide we don't need it on other platforms.
      NOTIMPLEMENTED();
      return false;
#endif
      break;
    case chrome::DIR_RESOURCES:
      if (!PathService::Get(chrome::DIR_APP, &cur))
        return false;
      file_util::AppendToPath(&cur, L"resources");
      create_dir = true;
      break;
    case chrome::DIR_INSPECTOR:
      if (!PathService::Get(chrome::DIR_APP, &cur))
        return false;
      file_util::AppendToPath(&cur, L"Resources");
      file_util::AppendToPath(&cur, L"Inspector");
      break;
    case chrome::DIR_THEMES:
      if (!PathService::Get(chrome::DIR_APP, &cur))
        return false;
      file_util::AppendToPath(&cur, L"themes");
      create_dir = true;
      break;
    case chrome::DIR_LOCALES:
      if (!PathService::Get(chrome::DIR_APP, &cur))
        return false;
      file_util::AppendToPath(&cur, L"locales");
      create_dir = true;
      break;
    case chrome::DIR_APP_DICTIONARIES:
      if (!PathService::Get(base::DIR_EXE, &cur))
        return false;
      file_util::AppendToPath(&cur, L"Dictionaries");
      create_dir = true;
      break;
    case chrome::FILE_LOCAL_STATE:
      if (!PathService::Get(chrome::DIR_USER_DATA, &cur))
        return false;
      file_util::AppendToPath(&cur, chrome::kLocalStateFilename);
      break;
    case chrome::FILE_RECORDED_SCRIPT:
      if (!PathService::Get(chrome::DIR_USER_DATA, &cur))
        return false;
      file_util::AppendToPath(&cur, L"script.log");
      break;
    case chrome::FILE_GEARS_PLUGIN:
      if (!GetGearsPluginPathFromCommandLine(&cur)) {
        // Search for gears.dll alongside chrome.dll first.  This new model
        // allows us to package gears.dll with the Chrome installer and update
        // it while Chrome is running.
        if (!PathService::Get(base::DIR_MODULE, &cur))
          return false;
        file_util::AppendToPath(&cur, L"gears.dll");

        if (!file_util::PathExists(cur)) {
          if (!PathService::Get(base::DIR_EXE, &cur))
            return false;
          file_util::AppendToPath(&cur, L"plugins");
          file_util::AppendToPath(&cur, L"gears");
          file_util::AppendToPath(&cur, L"gears.dll");
        }
      }
      break;
    // The following are only valid in the development environment, and
    // will fail if executed from an installed executable (because the
    // generated path won't exist).
    case chrome::DIR_TEST_DATA:
      if (!PathService::Get(base::DIR_SOURCE_ROOT, &cur))
        return false;
      file_util::AppendToPath(&cur, L"chrome");
      file_util::AppendToPath(&cur, L"test");
      file_util::AppendToPath(&cur, L"data");
      if (!file_util::PathExists(cur))  // we don't want to create this
        return false;
      break;
    case chrome::DIR_TEST_TOOLS:
      if (!PathService::Get(base::DIR_SOURCE_ROOT, &cur))
        return false;
      file_util::AppendToPath(&cur, L"chrome");
      file_util::AppendToPath(&cur, L"tools");
      file_util::AppendToPath(&cur, L"test");
      if (!file_util::PathExists(cur))  // we don't want to create this
        return false;
      break;
    case chrome::FILE_PYTHON_RUNTIME:
      if (!PathService::Get(base::DIR_SOURCE_ROOT, &cur))
        return false;
      file_util::AppendToPath(&cur, L"third_party");
      file_util::AppendToPath(&cur, L"python_24");
      file_util::AppendToPath(&cur, L"python.exe");
      if (!file_util::PathExists(cur))  // we don't want to create this
        return false;
      break;
    case chrome::FILE_TEST_SERVER:
      if (!PathService::Get(base::DIR_SOURCE_ROOT, &cur))
        return false;
      file_util::AppendToPath(&cur, L"net");
      file_util::AppendToPath(&cur, L"tools");
      file_util::AppendToPath(&cur, L"test");
      file_util::AppendToPath(&cur, L"testserver");
      file_util::AppendToPath(&cur, L"testserver.py");
      if (!file_util::PathExists(cur))  // we don't want to create this
        return false;
      break;
    default:
      return false;
  }

  if (create_dir && !file_util::PathExists(cur) && !file_util::CreateDirectory(cur))
    return false;

  *result = FilePath::FromWStringHack(cur);
  return true;
}

// This cannot be done as a static initializer sadly since Visual Studio will
// eliminate this object file if there is no direct entry point into it.
void RegisterPathProvider() {
  PathService::RegisterProvider(PathProvider, PATH_START, PATH_END);
}

}  // namespace chrome

