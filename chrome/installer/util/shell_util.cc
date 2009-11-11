// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file defines functions that integrate Chrome in Windows shell. These
// functions can be used by Chrome as well as Chrome installer. All of the
// work is done by the local functions defined in anonymous namespace in
// this class.

#include "chrome/installer/util/shell_util.h"

#include <windows.h>
#include <shlobj.h>

#include "base/command_line.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/registry.h"
#include "base/scoped_ptr.h"
#include "base/stl_util-inl.h"
#include "base/string_util.h"
#include "base/win_util.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/installer/util/browser_distribution.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/master_preferences.h"

#include "installer_util_strings.h"

namespace {

// This class represents a single registry entry. The objective is to
// encapsulate all the registry entries required for registering Chrome at one
// place. This class can not be instantiated outside the class and the objects
// of this class type can be obtained only by calling a static method of this
// class.
class RegistryEntry {
 public:
  // This method returns a list of all the registry entries that
  // are needed to register Chromium ProgIds.
  static bool GetProgIdEntries(const std::wstring& chrome_exe,
                               const std::wstring& suffix,
                               std::list<RegistryEntry*>* entries) {
    std::wstring icon_path = ShellUtil::GetChromeIcon(chrome_exe);
    std::wstring open_cmd = ShellUtil::GetChromeShellOpenCmd(chrome_exe);

    // File association ProgId
    std::wstring chrome_html_prog_id(ShellUtil::kRegClasses);
    file_util::AppendToPath(&chrome_html_prog_id, ShellUtil::kChromeHTMLProgId);
    chrome_html_prog_id.append(suffix);
    entries->push_front(new RegistryEntry(
        chrome_html_prog_id, ShellUtil::kChromeHTMLProgIdDesc));
    entries->push_front(new RegistryEntry(
        chrome_html_prog_id, ShellUtil::kRegUrlProtocol, L""));
    entries->push_front(new RegistryEntry(
        chrome_html_prog_id + ShellUtil::kRegDefaultIcon, icon_path));
    entries->push_front(new RegistryEntry(
        chrome_html_prog_id + ShellUtil::kRegShellOpen, open_cmd));

    return true;
  }

  // This method returns a list of all the system level registry entries that
  // are needed to register Chromium on the machine.
  static bool GetSystemEntries(const std::wstring& chrome_exe,
                               const std::wstring& suffix,
                               std::list<RegistryEntry*>* entries) {
    std::wstring icon_path = ShellUtil::GetChromeIcon(chrome_exe);
    std::wstring quoted_exe_path = L"\"" + chrome_exe + L"\"";

    BrowserDistribution* dist = BrowserDistribution::GetDistribution();
    std::wstring app_name = dist->GetApplicationName() + suffix;
    std::wstring start_menu_entry(ShellUtil::kRegStartMenuInternet);
    start_menu_entry.append(L"\\" + app_name);
    entries->push_front(new RegistryEntry(start_menu_entry, app_name));
    entries->push_front(new RegistryEntry(
        start_menu_entry + ShellUtil::kRegShellOpen, quoted_exe_path));
    entries->push_front(new RegistryEntry(
        start_menu_entry + ShellUtil::kRegDefaultIcon, icon_path));

    std::wstring install_info(start_menu_entry + L"\\InstallInfo");
    // TODO: use CommandLine API instead of constructing command lines.
    entries->push_front(new RegistryEntry(install_info, L"ReinstallCommand",
        quoted_exe_path + L" --" + ASCIIToWide(switches::kMakeDefaultBrowser)));
    entries->push_front(new RegistryEntry(install_info, L"HideIconsCommand",
        quoted_exe_path + L" --" + ASCIIToWide(switches::kHideIcons)));
    entries->push_front(new RegistryEntry(install_info, L"ShowIconsCommand",
        quoted_exe_path + L" --" + ASCIIToWide(switches::kShowIcons)));
    entries->push_front(new RegistryEntry(install_info, L"IconsVisible", 1));

    std::wstring capabilities(start_menu_entry + L"\\Capabilities");
    entries->push_front(new RegistryEntry(ShellUtil::kRegRegisteredApplications,
        app_name, capabilities));
    entries->push_front(new RegistryEntry(
        capabilities, L"ApplicationDescription", dist->GetApplicationName()));
    entries->push_front(new RegistryEntry(
        capabilities, L"ApplicationIcon", icon_path));
    entries->push_front(new RegistryEntry(
        capabilities, L"ApplicationName", app_name));

    entries->push_front(new RegistryEntry(capabilities + L"\\StartMenu",
        L"StartMenuInternet", app_name));

    std::wstring html_prog_id(ShellUtil::kChromeHTMLProgId);
    html_prog_id.append(suffix);
    for (int i = 0; ShellUtil::kFileAssociations[i] != NULL; i++) {
      entries->push_front(new RegistryEntry(
          capabilities + L"\\FileAssociations",
          ShellUtil::kFileAssociations[i], html_prog_id));
    }
    for (int i = 0; ShellUtil::kProtocolAssociations[i] != NULL; i++) {
      entries->push_front(new RegistryEntry(
          capabilities + L"\\URLAssociations",
          ShellUtil::kProtocolAssociations[i], html_prog_id));
    }

    FilePath chrome_path(chrome_exe);
    std::wstring app_path_key(ShellUtil::kAppPathsRegistryKey);
    file_util::AppendToPath(&app_path_key, chrome_path.BaseName().value());
    entries->push_front(new RegistryEntry(app_path_key, chrome_exe));
    entries->push_front(new RegistryEntry(app_path_key,
        ShellUtil::kAppPathsRegistryPathName, chrome_path.DirName().value()));

    // TODO: add chrome to open with list (Bug 16726).
    return true;
  }

  // This method returns a list of all the user level registry entries that
  // are needed to make Chromium default browser.
  static bool GetUserEntries(const std::wstring& chrome_exe,
                             const std::wstring& suffix,
                             std::list<RegistryEntry*>* entries) {
    // File extension associations.
    std::wstring html_prog_id(ShellUtil::kChromeHTMLProgId);
    html_prog_id.append(suffix);
    for (int i = 0; ShellUtil::kFileAssociations[i] != NULL; i++) {
      std::wstring ext_key(ShellUtil::kRegClasses);
      file_util::AppendToPath(&ext_key, ShellUtil::kFileAssociations[i]);
      entries->push_front(new RegistryEntry(ext_key, html_prog_id));
    }

    // Protocols associations.
    std::wstring chrome_open = ShellUtil::GetChromeShellOpenCmd(chrome_exe);
    std::wstring chrome_icon = ShellUtil::GetChromeIcon(chrome_exe);
    for (int i = 0; ShellUtil::kProtocolAssociations[i] != NULL; i++) {
      std::wstring url_key(ShellUtil::kRegClasses);
      file_util::AppendToPath(&url_key, ShellUtil::kProtocolAssociations[i]);

      // <root hkey>\Software\Classes\<protocol>\DefaultIcon
      std::wstring icon_key = url_key + ShellUtil::kRegDefaultIcon;
      entries->push_front(new RegistryEntry(icon_key, chrome_icon));

      // <root hkey>\Software\Classes\<protocol>\shell\open\command
      std::wstring shell_key = url_key + ShellUtil::kRegShellOpen;
      entries->push_front(new RegistryEntry(shell_key, chrome_open));

      // <root hkey>\Software\Classes\<protocol>\shell\open\ddeexec
      std::wstring dde_key = url_key + L"\\shell\\open\\ddeexec";
      entries->push_front(new RegistryEntry(dde_key, L""));

      // <root hkey>\Software\Classes\<protocol>\shell\@
      std::wstring protocol_shell_key = url_key + ShellUtil::kRegShellPath;
      entries->push_front(new RegistryEntry(protocol_shell_key, L"open"));
    }

    // start->Internet shortcut.
    std::wstring start_menu(ShellUtil::kRegStartMenuInternet);
    BrowserDistribution* dist = BrowserDistribution::GetDistribution();
    entries->push_front(new RegistryEntry(start_menu,
                                          dist->GetApplicationName()));
    return true;
  }

  // Generate work_item tasks required to create current registry entry and
  // add them to the given work item list.
  void AddToWorkItemList(HKEY root, WorkItemList *items) const {
    items->AddCreateRegKeyWorkItem(root, _key_path);
    if (_is_string) {
      items->AddSetRegValueWorkItem(root, _key_path, _name, _value, true);
    } else {
      items->AddSetRegValueWorkItem(root, _key_path, _name, _int_value, true);
    }
  }

  // Checks if the current registry entry exists in HKLM registry and the value
  // is same.
  bool ExistsInHKLM() const {
    RegKey key(HKEY_LOCAL_MACHINE, _key_path.c_str());
    bool found = false;
    if (_is_string) {
      std::wstring read_value;
      found = (key.ReadValue(_name.c_str(), &read_value)) &&
              (read_value.size() == _value.size()) &&
              (std::equal(_value.begin(), _value.end(), read_value.begin(),
                          CaseInsensitiveCompare<wchar_t>()));
    } else {
      DWORD read_value;
      found = key.ReadValueDW(_name.c_str(), &read_value) &&
              read_value == _int_value;
    }
    key.Close();
    return found;
  }

  // Checks if the current registry entry exists in HKLM registry
  // (only the name).
  bool NameExistsInHKLM() const {
    RegKey key(HKEY_LOCAL_MACHINE, _key_path.c_str());
    bool found = false;
    if (_is_string) {
      std::wstring read_value;
      found = key.ReadValue(_name.c_str(), &read_value);
    } else {
      DWORD read_value;
      found = key.ReadValueDW(_name.c_str(), &read_value);
    }
    key.Close();
    return found;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(RegistryEntry);

  // Create a object that represent default value of a key
  RegistryEntry(const std::wstring& key_path, const std::wstring& value)
      : _key_path(key_path), _name(),
        _is_string(true), _value(value), _int_value(0) {
  }

  // Create a object that represent a key of type REG_SZ
  RegistryEntry(const std::wstring& key_path, const std::wstring& name,
                const std::wstring& value)
      : _key_path(key_path), _name(name),
        _is_string(true), _value(value), _int_value(0) {
  }

  // Create a object that represent a key of integer type
  RegistryEntry(const std::wstring& key_path, const std::wstring& name,
                DWORD value)
      : _key_path(key_path), _name(name),
        _is_string(false), _value(), _int_value(value) {
  }

  std::wstring _key_path;  // key path for the registry entry
  std::wstring _name;      // name of the registry entry
  bool _is_string;         // true if current registry entry is of type REG_SZ
  std::wstring _value;     // string value (useful if _is_string = true)
  DWORD _int_value;        // integer value (useful if _is_string = false)
};  // class RegistryEntry


// This method converts all the RegistryEntries from the given list to
// Set/CreateRegWorkItems and runs them using WorkItemList.
bool AddRegistryEntries(HKEY root, const std::list<RegistryEntry*>& entries) {
  scoped_ptr<WorkItemList> items(WorkItem::CreateWorkItemList());

  for (std::list<RegistryEntry*>::const_iterator itr = entries.begin();
       itr != entries.end(); ++itr)
    (*itr)->AddToWorkItemList(root, items.get());

  // Apply all the registry changes and if there is a problem, rollback
  if (!items->Do()) {
    items->Rollback();
    return false;
  }
  return true;
}

// This method checks if Chrome is already registered on the local machine.
// It gets all the required registry entries for Chrome and then checks if
// they exist in HKLM. Returns true if all the entries exist, otherwise false.
bool IsChromeRegistered(const std::wstring& chrome_exe,
                        const std::wstring& suffix) {
  bool registered = true;
  std::list<RegistryEntry*> entries;
  STLElementDeleter<std::list<RegistryEntry*>> entries_deleter(&entries);
  RegistryEntry::GetProgIdEntries(chrome_exe, suffix, &entries);
  RegistryEntry::GetSystemEntries(chrome_exe, suffix, &entries);
  for (std::list<RegistryEntry*>::const_iterator itr = entries.begin();
       itr != entries.end() && registered; ++itr) {
    // We do not need registered = registered && ... since the loop condition
    // is set to exit early.
    registered = (*itr)->ExistsInHKLM();
  }
  return registered;
}

// This method registers Chrome on Vista by launching eleavated setup.exe.
// That will show user standard Vista elevation prompt. If user accepts it
// the new process will make the necessary changes and return SUCCESS that
// we capture and return.
bool ElevateAndRegisterChrome(const std::wstring& chrome_exe,
                              const std::wstring& suffix) {
  std::wstring exe_path(file_util::GetDirectoryFromPath(chrome_exe));
  file_util::AppendToPath(&exe_path, installer_util::kSetupExe);
  if (!file_util::PathExists(FilePath::FromWStringHack(exe_path))) {
    BrowserDistribution* dist = BrowserDistribution::GetDistribution();
    HKEY reg_root = InstallUtil::IsPerUserInstall(chrome_exe.c_str()) ?
        HKEY_CURRENT_USER : HKEY_LOCAL_MACHINE;
    RegKey key(reg_root, dist->GetUninstallRegPath().c_str());
    key.ReadValue(installer_util::kUninstallStringField, &exe_path);
    CommandLine command_line = CommandLine::FromString(exe_path);
    exe_path = command_line.program();
  }
  if (file_util::PathExists(FilePath::FromWStringHack(exe_path))) {
    std::wstring params(L"--");
    params.append(installer_util::switches::kRegisterChromeBrowser);
    params.append(L"=\"" + chrome_exe + L"\"");
    if (!suffix.empty()) {
      params.append(L" --");
      params.append(installer_util::switches::kRegisterChromeBrowserSuffix);
      params.append(L"=\"" + suffix + L"\"");
    }

    CommandLine& browser_command_line = *CommandLine::ForCurrentProcess();
    if (browser_command_line.HasSwitch(switches::kChromeFrame)) {
      params.append(L" --");
      params.append(installer_util::switches::kChromeFrame);
    }

    DWORD ret_val = 0;
    InstallUtil::ExecuteExeAsAdmin(exe_path, params, &ret_val);
    if (ret_val == 0)
      return true;
  }
  return false;
}

// This method tries to figure out if another user has already registered her
// own copy of Chrome so that we can avoid overwriting it and append current
// user's login name to default browser registry entries. This function is
// not meant to detect all cases. It just tries to handle the most common case.
// All the conditions below have to be true for it to return true:
// - Software\Clients\StartMenuInternet\Chromium\"" key should have a valid
//   value.
// - The value should not be same as given value in |chrome_exe|
// - Finally to handle the default install path (C:\Document and Settings\
//   <user>\Local Settings\Application Data\Chromium\Application) the value
//   of the above key should differ from |chrome_exe| only in user name.
bool AnotherUserHasDefaultBrowser(const std::wstring& chrome_exe) {
  std::wstring reg_key(ShellUtil::kRegStartMenuInternet);
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  reg_key.append(L"\\" + dist->GetApplicationName() + ShellUtil::kRegShellOpen);
  RegKey key(HKEY_LOCAL_MACHINE, reg_key.c_str());
  std::wstring registry_chrome_exe;
  if (!key.ReadValue(L"", &registry_chrome_exe) ||
      registry_chrome_exe.length() < 2)
    return false;

  registry_chrome_exe = registry_chrome_exe.substr(1,
      registry_chrome_exe.length() - 2);
  if ((registry_chrome_exe.size() == chrome_exe.size()) &&
      (std::equal(chrome_exe.begin(), chrome_exe.end(),
                  registry_chrome_exe.begin(),
                  CaseInsensitiveCompare<wchar_t>())))
    return false;

  std::vector<std::wstring> v1, v2;
  SplitString(registry_chrome_exe, L'\\', &v1);
  SplitString(chrome_exe, L'\\', &v2);
  if (v1.size() == 0 || v2.size() == 0 || v1.size() != v2.size())
    return false;

  // Now check that only one of the values within two '\' chars differ.
  std::vector<std::wstring>::iterator itr1 = v1.begin();
  std::vector<std::wstring>::iterator itr2 = v2.begin();
  bool one_mismatch = false;
  for ( ; itr1 < v1.end() && itr2 < v2.end(); ++itr1, ++itr2) {
    std::wstring s1 = *itr1;
    std::wstring s2 = *itr2;
    if ((s1.size() != s2.size()) ||
        (!std::equal(s1.begin(), s1.end(),
                     s2.begin(), CaseInsensitiveCompare<wchar_t>()))) {
      if (one_mismatch)
        return false;
      else
        one_mismatch = true;
    }
  }
  return true;
}

}  // namespace


const wchar_t* ShellUtil::kRegDefaultIcon = L"\\DefaultIcon";
const wchar_t* ShellUtil::kRegShellPath = L"\\shell";
const wchar_t* ShellUtil::kRegShellOpen = L"\\shell\\open\\command";
const wchar_t* ShellUtil::kRegStartMenuInternet =
    L"Software\\Clients\\StartMenuInternet";
const wchar_t* ShellUtil::kRegClasses = L"Software\\Classes";
const wchar_t* ShellUtil::kRegRegisteredApplications =
    L"Software\\RegisteredApplications";
const wchar_t* ShellUtil::kRegVistaUrlPrefs =
    L"Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\"
    L"http\\UserChoice";
const wchar_t* ShellUtil::kAppPathsRegistryKey =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths";
const wchar_t* ShellUtil::kAppPathsRegistryPathName = L"Path";

#if defined(GOOGLE_CHROME_BUILD)
const wchar_t* ShellUtil::kChromeHTMLProgId = L"ChromeHTML";
const wchar_t* ShellUtil::kChromeHTMLProgIdDesc = L"Chrome HTML Document";
#else
const wchar_t* ShellUtil::kChromeHTMLProgId = L"ChromiumHTML";
const wchar_t* ShellUtil::kChromeHTMLProgIdDesc = L"Chromium HTML Document";
#endif

const wchar_t* ShellUtil::kFileAssociations[] = {L".htm", L".html", L".shtml",
    L".xht", L".xhtml", NULL};
const wchar_t* ShellUtil::kProtocolAssociations[] = {L"ftp", L"http", L"https",
    NULL};
const wchar_t* ShellUtil::kRegUrlProtocol = L"URL Protocol";

bool ShellUtil::AdminNeededForRegistryCleanup(const std::wstring& suffix) {
  bool cleanup_needed = false;
  std::list<RegistryEntry*> entries;
  STLElementDeleter<std::list<RegistryEntry*>> entries_deleter(&entries);
  RegistryEntry::GetProgIdEntries(L"chrome.exe", suffix, &entries);
  RegistryEntry::GetSystemEntries(L"chrome.exe", suffix, &entries);
  for (std::list<RegistryEntry*>::const_iterator itr = entries.begin();
       itr != entries.end() && !cleanup_needed; ++itr) {
    cleanup_needed = (*itr)->NameExistsInHKLM();
  }
  return cleanup_needed;
}

bool ShellUtil::CreateChromeDesktopShortcut(const std::wstring& chrome_exe,
                                            const std::wstring& description,
                                            int shell_change, bool alternate,
                                            bool create_new) {
  std::wstring shortcut_name;
  if (!ShellUtil::GetChromeShortcutName(&shortcut_name, alternate))
    return false;

  bool ret = true;
  if (shell_change & ShellUtil::CURRENT_USER) {
    std::wstring shortcut_path;
    if (ShellUtil::GetDesktopPath(false, &shortcut_path)) {
      file_util::AppendToPath(&shortcut_path, shortcut_name);
      ret = ShellUtil::UpdateChromeShortcut(chrome_exe, shortcut_path,
                                            description, create_new);
    } else {
      ret = false;
    }
  }
  if (shell_change & ShellUtil::SYSTEM_LEVEL) {
    std::wstring shortcut_path;
    if (ShellUtil::GetDesktopPath(true, &shortcut_path)) {
      file_util::AppendToPath(&shortcut_path, shortcut_name);
      // Note we need to call the create operation and then AND the result
      // with the create operation of user level shortcut.
      ret = ShellUtil::UpdateChromeShortcut(chrome_exe, shortcut_path,
                                            description, create_new) && ret;
    } else {
      ret = false;
    }
  }
  return ret;
}

bool ShellUtil::CreateChromeQuickLaunchShortcut(const std::wstring& chrome_exe,
                                                int shell_change,
                                                bool create_new) {
  std::wstring shortcut_name;
  if (!ShellUtil::GetChromeShortcutName(&shortcut_name, false))
    return false;

  bool ret = true;
  // First create shortcut for the current user.
  if (shell_change & ShellUtil::CURRENT_USER) {
    std::wstring user_ql_path;
    if (ShellUtil::GetQuickLaunchPath(false, &user_ql_path)) {
      file_util::AppendToPath(&user_ql_path, shortcut_name);
      ret = ShellUtil::UpdateChromeShortcut(chrome_exe, user_ql_path,
                                            L"", create_new);
    } else {
      ret = false;
    }
  }

  // Add a shortcut to Default User's profile so that all new user profiles
  // get it.
  if (shell_change & ShellUtil::SYSTEM_LEVEL) {
    std::wstring default_ql_path;
    if (ShellUtil::GetQuickLaunchPath(true, &default_ql_path)) {
      file_util::AppendToPath(&default_ql_path, shortcut_name);
      ret = ShellUtil::UpdateChromeShortcut(chrome_exe, default_ql_path,
                                            L"", create_new) && ret;
    } else {
      ret = false;
    }
  }

  return ret;
}

std::wstring ShellUtil::GetChromeIcon(const std::wstring& chrome_exe) {
  std::wstring chrome_icon(chrome_exe);
  chrome_icon.append(L",0");
  return chrome_icon;
}

std::wstring ShellUtil::GetChromeShellOpenCmd(const std::wstring& chrome_exe) {
  return L"\"" + chrome_exe + L"\" -- \"%1\"";
}

bool ShellUtil::GetChromeShortcutName(std::wstring* shortcut, bool alternate) {
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  shortcut->assign(alternate ? dist->GetAlternateApplicationName() :
                               dist->GetApplicationName());
  shortcut->append(L".lnk");
  return true;
}

bool ShellUtil::GetDesktopPath(bool system_level, std::wstring* path) {
  wchar_t desktop[MAX_PATH];
  int dir = system_level ? CSIDL_COMMON_DESKTOPDIRECTORY :
                           CSIDL_DESKTOPDIRECTORY;
  if (FAILED(SHGetFolderPath(NULL, dir, NULL, SHGFP_TYPE_CURRENT, desktop)))
    return false;
  *path = desktop;
  return true;
}

bool ShellUtil::GetQuickLaunchPath(bool system_level, std::wstring* path) {
  static const wchar_t* kQuickLaunchPath =
      L"Microsoft\\Internet Explorer\\Quick Launch";
  wchar_t qlaunch[MAX_PATH];
  if (system_level) {
    // We are accessing GetDefaultUserProfileDirectory this way so that we do
    // not have to declare dependency to Userenv.lib for chrome.exe
    typedef BOOL (WINAPI *PROFILE_FUNC)(LPWSTR, LPDWORD);
    HMODULE module = LoadLibrary(L"Userenv.dll");
    PROFILE_FUNC p = reinterpret_cast<PROFILE_FUNC>(GetProcAddress(module,
        "GetDefaultUserProfileDirectoryW"));
    DWORD size = _countof(qlaunch);
    if ((p == NULL) || ((p)(qlaunch, &size) != TRUE))
      return false;
    *path = qlaunch;
    if (win_util::GetWinVersion() >= win_util::WINVERSION_VISTA) {
      file_util::AppendToPath(path, L"AppData\\Roaming");
    } else {
      file_util::AppendToPath(path, L"Application Data");
    }
  } else {
    if (FAILED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL,
                               SHGFP_TYPE_CURRENT, qlaunch)))
      return false;
    *path = qlaunch;
  }
  file_util::AppendToPath(path, kQuickLaunchPath);
  return true;
}

void ShellUtil::GetRegisteredBrowsers(std::map<std::wstring,
                                      std::wstring>* browsers) {
  std::wstring base_key(ShellUtil::kRegStartMenuInternet);
  HKEY root = HKEY_LOCAL_MACHINE;
  for (RegistryKeyIterator iter(root, base_key.c_str()); iter.Valid(); ++iter) {
    std::wstring key = base_key + L"\\" + iter.Name();
    RegKey capabilities(root, (key + L"\\Capabilities").c_str());
    std::wstring name;
    if (!capabilities.Valid() ||
        !capabilities.ReadValue(L"ApplicationName", &name)) {
      RegKey base_key(root, key.c_str());
      if (!base_key.ReadValue(L"", &name))
        continue;
    }
    RegKey install_info(root, (key + L"\\InstallInfo").c_str());
    std::wstring command;
    if (!install_info.Valid() ||
        !install_info.ReadValue(L"ReinstallCommand", &command))
      continue;
    BrowserDistribution* dist = BrowserDistribution::GetDistribution();
    if (!name.empty() && !command.empty() &&
        name.find(dist->GetApplicationName()) == std::wstring::npos)
      (*browsers)[name] = command;
  }
}

bool ShellUtil::GetUserSpecificDefaultBrowserSuffix(std::wstring* entry) {
  wchar_t user_name[256];
  DWORD size = _countof(user_name);
  if (::GetUserName(user_name, &size) == 0)
    return false;
  entry->assign(L".");
  entry->append(user_name);

  std::wstring start_menu_entry(ShellUtil::kRegStartMenuInternet);
  BrowserDistribution* dist = BrowserDistribution::GetDistribution();
  start_menu_entry.append(L"\\" + dist->GetApplicationName() + *entry);
  RegKey key(HKEY_LOCAL_MACHINE, start_menu_entry.c_str());
  return key.Valid();
}

bool ShellUtil::MakeChromeDefault(int shell_change,
                                  const std::wstring& chrome_exe,
                                  bool elevate_if_not_admin) {
  ShellUtil::RegisterChromeBrowser(chrome_exe, L"", elevate_if_not_admin);

  bool ret = true;
  // First use the new "recommended" way on Vista to make Chrome default
  // browser.
  if (win_util::GetWinVersion() >= win_util::WINVERSION_VISTA) {
    LOG(INFO) << "Registering Chrome as default browser on Vista.";
    IApplicationAssociationRegistration* pAAR;
    HRESULT hr = CoCreateInstance(CLSID_ApplicationAssociationRegistration,
        NULL, CLSCTX_INPROC, __uuidof(IApplicationAssociationRegistration),
        (void**)&pAAR);
    if (SUCCEEDED(hr)) {
      BrowserDistribution* dist = BrowserDistribution::GetDistribution();
      std::wstring app_name = dist->GetApplicationName();
      std::wstring suffix;
      if (ShellUtil::GetUserSpecificDefaultBrowserSuffix(&suffix))
        app_name += suffix;

      hr = pAAR->SetAppAsDefaultAll(app_name.c_str());
      pAAR->Release();
    }
    if (!SUCCEEDED(hr)) {
      ret = false;
      LOG(ERROR) << "Could not make Chrome default browser.";
    }
  }

  // Now use the old way to associate Chrome with supported protocols and file
  // associations. This should not be required on Vista but since some
  // applications still read Software\Classes\http key directly, we have to do
  // this on Vista also.

  std::list<RegistryEntry*> entries;
  STLElementDeleter<std::list<RegistryEntry*>> entries_deleter(&entries);
  std::wstring suffix;
  if (!GetUserSpecificDefaultBrowserSuffix(&suffix))
    suffix = L"";
  RegistryEntry::GetUserEntries(chrome_exe, suffix, &entries);
  // Change the default browser for current user.
  if ((shell_change & ShellUtil::CURRENT_USER) &&
      !AddRegistryEntries(HKEY_CURRENT_USER, entries))
      ret = false;

  // Chrome as default browser at system level.
  if ((shell_change & ShellUtil::SYSTEM_LEVEL) &&
      !AddRegistryEntries(HKEY_LOCAL_MACHINE, entries))
    ret = false;

  // Send Windows notification event so that it can update icons for
  // file associations.
  SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
  return ret;
}

bool ShellUtil::RegisterChromeBrowser(const std::wstring& chrome_exe,
                                      const std::wstring& unique_suffix,
                                      bool elevate_if_not_admin) {
  // First figure out we need to append a suffix to the registry entries to
  // make them unique.
  std::wstring suffix;
  if (!unique_suffix.empty()) {
    suffix = unique_suffix;
  } else if (InstallUtil::IsPerUserInstall(chrome_exe.c_str()) &&
             !GetUserSpecificDefaultBrowserSuffix(&suffix) &&
             !AnotherUserHasDefaultBrowser(chrome_exe)) {
    suffix = L"";
  }

  // Check if Chromium is already registered with this suffix.
  if (IsChromeRegistered(chrome_exe, suffix))
    return true;

  // If user is an admin try to register and return the status.
  if (IsUserAnAdmin()) {
    std::list<RegistryEntry*> entries;
    STLElementDeleter<std::list<RegistryEntry*>> entries_deleter(&entries);
    RegistryEntry::GetProgIdEntries(chrome_exe, suffix, &entries);
    RegistryEntry::GetSystemEntries(chrome_exe, suffix, &entries);
    return AddRegistryEntries(HKEY_LOCAL_MACHINE, entries);
  }

  // If user is not an admin and OS is Vista, try to elevate and register.
  if (elevate_if_not_admin &&
      win_util::GetWinVersion() >= win_util::WINVERSION_VISTA &&
      ElevateAndRegisterChrome(chrome_exe, suffix))
    return true;

  // If we got to this point then all we can do is create ProgIds under HKCU
  // on XP as well as Vista.
  std::list<RegistryEntry*> entries;
  STLElementDeleter<std::list<RegistryEntry*>> entries_deleter(&entries);
  RegistryEntry::GetProgIdEntries(chrome_exe, L"", &entries);
  return AddRegistryEntries(HKEY_CURRENT_USER, entries);
}

bool ShellUtil::RemoveChromeDesktopShortcut(int shell_change, bool alternate) {
  std::wstring shortcut_name;
  if (!ShellUtil::GetChromeShortcutName(&shortcut_name, alternate))
    return false;

  bool ret = true;
  if (shell_change & ShellUtil::CURRENT_USER) {
    std::wstring shortcut_path;
    if (ShellUtil::GetDesktopPath(false, &shortcut_path)) {
      file_util::AppendToPath(&shortcut_path, shortcut_name);
      ret = file_util::Delete(shortcut_path, false);
    } else {
      ret = false;
    }
  }

  if (shell_change & ShellUtil::SYSTEM_LEVEL) {
    std::wstring shortcut_path;
    if (ShellUtil::GetDesktopPath(true, &shortcut_path)) {
      file_util::AppendToPath(&shortcut_path, shortcut_name);
      ret = file_util::Delete(shortcut_path, false) && ret;
    } else {
      ret = false;
    }
  }
  return ret;
}

bool ShellUtil::RemoveChromeQuickLaunchShortcut(int shell_change) {
  std::wstring shortcut_name;
  if (!ShellUtil::GetChromeShortcutName(&shortcut_name, false))
    return false;

  bool ret = true;
  // First remove shortcut for the current user.
  if (shell_change & ShellUtil::CURRENT_USER) {
    std::wstring user_ql_path;
    if (ShellUtil::GetQuickLaunchPath(false, &user_ql_path)) {
      file_util::AppendToPath(&user_ql_path, shortcut_name);
      ret = file_util::Delete(user_ql_path, false);
    } else {
      ret = false;
    }
  }

  // Delete shortcut in Default User's profile
  if (shell_change & ShellUtil::SYSTEM_LEVEL) {
    std::wstring default_ql_path;
    if (ShellUtil::GetQuickLaunchPath(true, &default_ql_path)) {
      file_util::AppendToPath(&default_ql_path, shortcut_name);
      ret = file_util::Delete(default_ql_path, false) && ret;
    } else {
      ret = false;
    }
  }

  return ret;
}

bool ShellUtil::UpdateChromeShortcut(const std::wstring& chrome_exe,
                                     const std::wstring& shortcut,
                                     const std::wstring& description,
                                     bool create_new) {
  std::wstring chrome_path = file_util::GetDirectoryFromPath(chrome_exe);

  if (create_new) {
    FilePath prefs_path(chrome_path);
    prefs_path = prefs_path.Append(installer_util::kDefaultMasterPrefs);
    scoped_ptr<DictionaryValue> prefs(
        installer_util::ParseDistributionPreferences(prefs_path));
    int icon_index = 0;
    installer_util::GetDistroIntegerPreference(prefs.get(),
        installer_util::master_preferences::kChromeShortcutIconIndex,
        &icon_index);
    return file_util::CreateShortcutLink(chrome_exe.c_str(),      // target
                                         shortcut.c_str(),        // shortcut
                                         chrome_path.c_str(),     // working dir
                                         NULL,                    // arguments
                                         description.c_str(),     // description
                                         chrome_exe.c_str(),      // icon file
                                         icon_index);             // icon index
  } else {
    return file_util::UpdateShortcutLink(chrome_exe.c_str(),      // target
                                         shortcut.c_str(),        // shortcut
                                         chrome_path.c_str(),     // working dir
                                         NULL,                    // arguments
                                         description.c_str(),     // description
                                         chrome_exe.c_str(),      // icon file
                                         0);                      // icon index
  }
}
