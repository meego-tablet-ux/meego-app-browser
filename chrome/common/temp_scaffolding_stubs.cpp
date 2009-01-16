// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "temp_scaffolding_stubs.h"

#include "base/file_util.h"
#include "base/thread.h"
#include "base/path_service.h"
#include "base/singleton.h"
#include "chrome/browser/plugin_service.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_service.h"

BrowserProcessImpl::BrowserProcessImpl(CommandLine& command_line)
    : created_local_state_(), created_metrics_service_(),
      created_profile_manager_() {
  g_browser_process = this;
}

BrowserProcessImpl::~BrowserProcessImpl() {
  g_browser_process = NULL;
}

void BrowserProcessImpl::CreateLocalState() {
  DCHECK(!created_local_state_ && local_state_.get() == NULL);
  created_local_state_ = true;

  std::wstring local_state_path;
  PathService::Get(chrome::FILE_LOCAL_STATE, &local_state_path);
  local_state_.reset(new PrefService(local_state_path));
}

void BrowserProcessImpl::CreateMetricsService() {
  DCHECK(!created_metrics_service_ && metrics_service_.get() == NULL);
  created_metrics_service_ = true;

  metrics_service_.reset(new MetricsService);
}

void BrowserProcessImpl::CreateProfileManager() {
  DCHECK(!created_profile_manager_ && profile_manager_.get() == NULL);
  created_profile_manager_ = true;

  profile_manager_.reset(new ProfileManager());
}

MetricsService* BrowserProcessImpl::metrics_service() {
  if (!created_metrics_service_)
    CreateMetricsService();
  return metrics_service_.get();
}

ProfileManager* BrowserProcessImpl::profile_manager() {
  if (!created_profile_manager_)
    CreateProfileManager();
  return profile_manager_.get();
}

PrefService* BrowserProcessImpl::local_state() {
  if (!created_local_state_)
    CreateLocalState();
  return local_state_.get();
}

//--------------------------------------------------------------------------

static bool s_in_startup = false;

bool BrowserInit::ProcessCommandLine(const CommandLine& parsed_command_line,
                                     const std::wstring& cur_dir,
                                     PrefService* prefs, bool process_startup,
                                     Profile* profile, int* return_code) {
  return LaunchBrowser(parsed_command_line, profile, cur_dir,
                       process_startup, return_code);
}

bool BrowserInit::LaunchBrowser(const CommandLine& parsed_command_line,
                                Profile* profile, const std::wstring& cur_dir,
                                bool process_startup, int* return_code) {
  s_in_startup = process_startup;
  bool result = LaunchBrowserImpl(parsed_command_line, profile, cur_dir,
                                  process_startup, return_code);
  s_in_startup = false;
  return result;
}

bool BrowserInit::LaunchBrowserImpl(const CommandLine& parsed_command_line,
                                    Profile* profile,
                                    const std::wstring& cur_dir,
                                    bool process_startup,
                                    int* return_code) {
  DCHECK(profile);

  // LAUNCH BROWSER WITH PROFILE HERE!

  return true;
}

//--------------------------------------------------------------------------

UserDataManager* UserDataManager::instance_ = NULL;

void UserDataManager::Create() {
  DCHECK(!instance_);
  std::wstring user_data;
  PathService::Get(chrome::DIR_USER_DATA, &user_data);
  instance_ = new UserDataManager(user_data);
}

UserDataManager* UserDataManager::Get() {
  DCHECK(instance_);
  return instance_;
}

//--------------------------------------------------------------------------

std::wstring ProfileManager::GetDefaultProfileDir(
    const std::wstring& user_data_dir) {
  std::wstring default_profile_dir(user_data_dir);
  file_util::AppendToPath(&default_profile_dir, chrome::kNotSignedInProfile);
  return default_profile_dir;
}

std::wstring ProfileManager::GetDefaultProfilePath(
    const std::wstring &profile_dir) {
  std::wstring default_prefs_path(profile_dir);
  file_util::AppendToPath(&default_prefs_path, chrome::kPreferencesFilename);
  return default_prefs_path;
}

Profile* ProfileManager::GetDefaultProfile(const std::wstring& user_data_dir) {
  std::wstring default_profile_dir = GetDefaultProfileDir(user_data_dir);
  return new Profile(default_profile_dir);
}

//--------------------------------------------------------------------------

Profile::Profile(const std::wstring& path)
    : path_(path) {
}

std::wstring Profile::GetPrefFilePath() {
  std::wstring pref_file_path = path_;
  file_util::AppendToPath(&pref_file_path, chrome::kPreferencesFilename);
  return pref_file_path;
}

PrefService* Profile::GetPrefs() {
  if (!prefs_.get())
    prefs_.reset(new PrefService(GetPrefFilePath()));
  return prefs_.get();
}

//--------------------------------------------------------------------------

bool ShellIntegration::SetAsDefaultBrowser() {
  return true;
}

bool ShellIntegration::IsDefaultBrowser() {
  return true;
}

//--------------------------------------------------------------------------

namespace browser {
void RegisterAllPrefs(PrefService*, PrefService*) { }
}  // namespace browser

namespace browser_shutdown {
void ReadLastShutdownInfo()  { }
void Shutdown() { }
}

void OpenFirstRunDialog(Profile* profile) { }

//--------------------------------------------------------------------------

PluginService* PluginService::GetInstance() {
  return Singleton<PluginService>::get();
}

PluginService::PluginService()
    : main_message_loop_(MessageLoop::current()),
      resource_dispatcher_host_(NULL),
      ui_locale_(g_browser_process->GetApplicationLocale()),
      plugin_shutdown_handler_(NULL) {
}

PluginService::~PluginService() {
}

void PluginService::SetChromePluginDataDir(const std::wstring& data_dir) {
  AutoLock lock(lock_);
  chrome_plugin_data_dir_ = data_dir;
}

//--------------------------------------------------------------------------

void InstallJankometer(const CommandLine&) {
}
