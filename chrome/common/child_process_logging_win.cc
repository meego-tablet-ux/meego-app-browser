// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/child_process_logging.h"

#include <windows.h>

#include "base/string_util.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/installer/util/google_update_settings.h"
#include "googleurl/src/gurl.h"

namespace child_process_logging {
// exported in breakpad_win.cc: void __declspec(dllexport) __cdecl SetActiveURL.
typedef void (__cdecl *MainSetActiveURL)(const wchar_t*);

// exported in breakpad_win.cc: void __declspec(dllexport) __cdecl SetClientId.
typedef void (__cdecl *MainSetClientId)(const wchar_t*);

void SetActiveURL(const GURL& url) {
  static MainSetActiveURL set_active_url = NULL;
  // note: benign race condition on set_active_url.
  if (!set_active_url) {
    HMODULE exe_module = GetModuleHandle(chrome::kBrowserProcessExecutableName);
    if (!exe_module)
      return;
    set_active_url = reinterpret_cast<MainSetActiveURL>(
        GetProcAddress(exe_module, "SetActiveURL"));
    if (!set_active_url)
      return;
  }

  (set_active_url)(UTF8ToWide(url.possibly_invalid_spec()).c_str());
}

void SetClientId(const std::string& client_id) {
  std::string str(client_id);
  // Remove all instance of '-' char from the GUID. So BCD-WXY becomes BCDWXY.
  ReplaceSubstringsAfterOffset(&str, 0, "-", "");

  if (str.empty())
    return;

  std::wstring wstr = ASCIIToWide(str);
  std::wstring old_wstr;
  if (!GoogleUpdateSettings::GetMetricsId(&old_wstr) ||
      wstr != old_wstr)
    GoogleUpdateSettings::SetMetricsId(wstr);

  static MainSetClientId set_client_id = NULL;
  if (!set_client_id) {
    HMODULE exe_module = GetModuleHandle(chrome::kBrowserProcessExecutableName);
    if (!exe_module)
      return;
    set_client_id = reinterpret_cast<MainSetClientId>(
        GetProcAddress(exe_module, "SetClientId"));
    if (!set_client_id)
      return;
  }
  (set_client_id)(wstr.c_str());
}

}  // namespace child_process_logging
