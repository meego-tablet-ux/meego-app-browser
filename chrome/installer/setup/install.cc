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

#include <time.h>

#include "base/file_util.h"
#include "base/scoped_ptr.h"
#include "base/string_util.h"
#include "chrome/installer/setup/setup.h"
#include "chrome/installer/setup/setup_constants.h"
#include "chrome/installer/util/copy_tree_work_item.h"
#include "chrome/installer/util/create_dir_work_item.h"
#include "chrome/installer/util/create_reg_key_work_item.h"
#include "chrome/installer/util/delete_tree_work_item.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/l10n_string_util.h"
#include "chrome/installer/util/logging_installer.h"
#include "chrome/installer/util/google_update_constants.h"
#include "chrome/installer/util/set_reg_value_work_item.h"
#include "chrome/installer/util/shell_util.h"
#include "chrome/installer/util/util_constants.h"
#include "chrome/installer/util/version.h"
#include "chrome/installer/util/work_item_list.h"

#include "setup_strings.h"

namespace {
std::wstring AppendPath(const std::wstring parent_path,
                        const std::wstring path) {
  std::wstring new_path(parent_path);
  file_util::AppendToPath(&new_path, path);
  return new_path;
}


// This method adds work items to create (or update) Chrome uninstall entry in
// Control Panel->Add/Remove Programs list.
void AddUninstallShortcutWorkItems(HKEY reg_root,
                                   const std::wstring& exe_path,
                                   const std::wstring& install_path,
                                   const std::wstring& new_version,
                                   WorkItemList* install_list) {
  std::wstring uninstall_cmd(L"\"");
  uninstall_cmd.append(installer::GetInstallerPathUnderChrome(install_path,
                                                              new_version));
  file_util::AppendToPath(&uninstall_cmd,
                          file_util::GetFilenameFromPath(exe_path));
  uninstall_cmd.append(L"\" --");
  uninstall_cmd.append(installer_util::switches::kUninstall);

  // Create DisplayName, UninstallString and InstallLocation keys
  install_list->AddCreateRegKeyWorkItem(reg_root,
                                        installer_util::kUninstallRegPath);
  const std::wstring& product_name =
      installer_util::GetLocalizedString(IDS_PRODUCT_NAME_BASE);
  install_list->AddSetRegValueWorkItem(
      reg_root, installer_util::kUninstallRegPath,
      installer_util::kUninstallDisplayNameField, product_name, true);
  install_list->AddSetRegValueWorkItem(reg_root,
                                       installer_util::kUninstallRegPath,
                                       installer_util::kUninstallStringField,
                                       uninstall_cmd, true);
  install_list->AddSetRegValueWorkItem(reg_root,
                                       installer_util::kUninstallRegPath,
                                       L"InstallLocation", install_path, true);

  // DisplayIcon, NoModify and NoRepair
  std::wstring chrome_icon = AppendPath(install_path,
                                        installer_util::kChromeExe);
  ShellUtil::GetChromeIcon(chrome_icon);
  install_list->AddSetRegValueWorkItem(reg_root,
                                       installer_util::kUninstallRegPath,
                                       L"DisplayIcon", chrome_icon, true);
  install_list->AddSetRegValueWorkItem(reg_root,
                                       installer_util::kUninstallRegPath,
                                       L"NoModify", 1, true);
  install_list->AddSetRegValueWorkItem(reg_root,
                                       installer_util::kUninstallRegPath,
                                       L"NoRepair", 1, true);

  install_list->AddSetRegValueWorkItem(reg_root,
                                       installer_util::kUninstallRegPath,
                                       L"Publisher",
                                       installer_util::kPublisherName, true);
  install_list->AddSetRegValueWorkItem(reg_root,
                                       installer_util::kUninstallRegPath,
                                       L"Version", new_version.c_str(), true);
  install_list->AddSetRegValueWorkItem(reg_root,
                                       installer_util::kUninstallRegPath,
                                       L"DisplayVersion",
                                       new_version.c_str(), true);
  time_t rawtime = time(NULL);
  struct tm timeinfo = {0};
  localtime_s(&timeinfo, &rawtime);
  wchar_t buffer[9];
  if (wcsftime(buffer, 9, L"%Y%m%d", &timeinfo) == 8) {
    install_list->AddSetRegValueWorkItem(reg_root,
                                         installer_util::kUninstallRegPath,
                                         L"InstallDate",
                                         buffer, false);
  }
}

void AddInstallerCopyTasks(const std::wstring& exe_path,
                           const std::wstring& archive_path,
                           const std::wstring& temp_path,
                           const std::wstring& install_path,
                           const std::wstring& new_version,
                           WorkItemList* install_list) {
  std::wstring installer_dir(installer::GetInstallerPathUnderChrome(
      install_path, new_version));
  install_list->AddCreateDirWorkItem(installer_dir);

  std::wstring exe_dst(installer_dir);
  std::wstring archive_dst(installer_dir);
  file_util::AppendToPath(&exe_dst,
      file_util::GetFilenameFromPath(exe_path));
  file_util::AppendToPath(&archive_dst,
      file_util::GetFilenameFromPath(archive_path));

  install_list->AddCopyTreeWorkItem(exe_path, exe_dst, temp_path,
                                    WorkItem::ALWAYS);
  install_list->AddCopyTreeWorkItem(archive_path, archive_dst, temp_path,
                                    WorkItem::ALWAYS);
}


// This method tells if we are running on 64 bit platform so that we can copy
// one extra exe. If the API call to determine 64 bit fails, we play it safe
// and return true anyway so that the executable can be copied.
bool Is64bit() {
  typedef BOOL (WINAPI *WOW_FUNC)(HANDLE, PBOOL);
  BOOL is64 = FALSE;

  HANDLE handle = GetCurrentProcess();
  HMODULE module = GetModuleHandle(L"kernel32.dll");
  WOW_FUNC p = reinterpret_cast<WOW_FUNC>(GetProcAddress(module,
                                                         "IsWow64Process"));
  if ((p != NULL) && (!(p)(handle, &is64) || (is64 != FALSE))) {
    return true;
  }

  return false;
}

}

bool installer::InstallNewVersion(const std::wstring& exe_path,
                                  const std::wstring& archive_path,
                                  const std::wstring& src_path,
                                  const std::wstring& install_path,
                                  const std::wstring& temp_dir,
                                  const HKEY reg_root,
                                  const Version& new_version) {

  if (reg_root != HKEY_LOCAL_MACHINE && reg_root != HKEY_CURRENT_USER)
    return false;

  scoped_ptr<WorkItemList> install_list(WorkItem::CreateWorkItemList());
  // A temp directory that work items need and the actual install directory.
  install_list->AddCreateDirWorkItem(temp_dir);
  install_list->AddCreateDirWorkItem(install_path);

  // Copy the version folder
  install_list->AddCopyTreeWorkItem(
      AppendPath(src_path, new_version.GetString()),
      AppendPath(install_path, new_version.GetString()),
      temp_dir, WorkItem::ALWAYS);    // Always overwrite.

  // Delete any new_chrome.exe if present (we will end up create a new one
  // if required) and then copy chrome.exe
  install_list->AddDeleteTreeWorkItem(
      AppendPath(install_path, installer::kChromeNewExe), std::wstring());
  install_list->AddCopyTreeWorkItem(
      AppendPath(src_path, installer_util::kChromeExe),
      AppendPath(install_path, installer_util::kChromeExe),
      temp_dir, WorkItem::RENAME_IF_IN_USE,
      AppendPath(install_path, installer::kChromeNewExe));

  // Extra executable for 64 bit systems.
  if (Is64bit()) {
    install_list->AddCopyTreeWorkItem(
        AppendPath(src_path, installer::kWowHelperExe),
        AppendPath(install_path, installer::kWowHelperExe),
        temp_dir, WorkItem::ALWAYS);
  }

  // Copy the default Dictionaries only if the folder doesnt exist already
  install_list->AddCopyTreeWorkItem(
      AppendPath(src_path, installer::kDictionaries),
      AppendPath(install_path, installer::kDictionaries),
      temp_dir, WorkItem::IF_NOT_PRESENT);

  // Copy installer in install directory and
  // add shortcut in Control Panel->Add/Remove Programs.
  AddInstallerCopyTasks(exe_path, archive_path, temp_dir, install_path,
      new_version.GetString(), install_list.get());
  AddUninstallShortcutWorkItems(reg_root, exe_path, install_path,
      new_version.GetString(), install_list.get());

  // Delete any old_chrome.exe if present.
  install_list->AddDeleteTreeWorkItem(
      AppendPath(install_path, installer::kChromeOldExe), std::wstring());

  // Create Google Update key (if not already present) and set the new Chrome
  // version as last step.
  std::wstring chrome_google_update_key =
      InstallUtil::GetChromeGoogleUpdateKey();
  install_list->AddCreateRegKeyWorkItem(reg_root, chrome_google_update_key);
  install_list->AddSetRegValueWorkItem(reg_root, chrome_google_update_key,
                                       google_update::kRegNameField,
                                       installer_util::kChrome,
                                       false);    // Don't overwrite.
  install_list->AddSetRegValueWorkItem(reg_root, chrome_google_update_key,
                                       google_update::kRegVersionField,
                                       new_version.GetString(),
                                       true);    // overwrite version

  // Perform install operations.
  if (!install_list->Do()) {
    LOG(ERROR) << "install failed, roll back... ";
    install_list->Rollback();
    return false;
  }

  return true;
}
