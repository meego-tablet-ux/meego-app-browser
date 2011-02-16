// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/dom_ui/app_launcher_handler.h"

#include <string>
#include <vector>

#include "base/metrics/histogram.h"
#include "base/string_number_conversions.h"
#include "base/string_split.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/disposition_utils.h"
#include "chrome/browser/dom_ui/shown_sections_handler.h"
#include "chrome/browser/extensions/default_apps.h"
#include "chrome/browser/extensions/extension_prefs.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/extensions/extension_icon_set.h"
#include "chrome/common/extensions/extension_resource.h"
#include "chrome/common/notification_service.h"
#include "chrome/common/notification_type.h"
#include "chrome/common/url_constants.h"
#include "grit/browser_resources.h"
#include "grit/generated_resources.h"
#include "net/base/escape.h"
#include "ui/base/animation/animation.h"
#include "ui/gfx/rect.h"
#include "webkit/glue/window_open_disposition.h"

namespace {

// The URL prefixes used by the NTP to signal when the web store or an app
// has launched so we can record the proper histogram.
const char* kPingLaunchAppByID = "record-app-launch-by-id";
const char* kPingLaunchWebStore = "record-webstore-launch";
const char* kPingLaunchAppByURL = "record-app-launch-by-url";

const UnescapeRule::Type kUnescapeRules =
    UnescapeRule::NORMAL | UnescapeRule::URL_SPECIAL_CHARS;

std::string GetIconURL(const Extension* extension, Extension::Icons icon,
                       const std::string& default_val) {
  GURL url = extension->GetIconURL(icon, ExtensionIconSet::MATCH_EXACTLY);
  if (!url.is_empty())
    return url.spec();
  else
    return default_val;
}

extension_misc::AppLaunchBucket ParseLaunchSource(std::string launch_source) {
  int bucket_num = extension_misc::APP_LAUNCH_BUCKET_INVALID;
  base::StringToInt(launch_source, &bucket_num);
  extension_misc::AppLaunchBucket bucket =
      static_cast<extension_misc::AppLaunchBucket>(bucket_num);
  CHECK(bucket < extension_misc::APP_LAUNCH_BUCKET_BOUNDARY);
  return bucket;
}

}  // namespace

AppLauncherHandler::AppLauncherHandler(ExtensionService* extension_service)
    : extensions_service_(extension_service),
      promo_active_(false),
      ignore_changes_(false) {
}

AppLauncherHandler::~AppLauncherHandler() {}

// static
void AppLauncherHandler::CreateAppInfo(const Extension* extension,
                                       ExtensionPrefs* extension_prefs,
                                       DictionaryValue* value) {
  value->Clear();
  value->SetString("id", extension->id());
  value->SetString("name", extension->name());
  value->SetString("description", extension->description());
  value->SetString("launch_url", extension->GetFullLaunchURL().spec());
  value->SetString("options_url", extension->options_url().spec());
  value->SetString("icon_big", GetIconURL(
      extension, Extension::EXTENSION_ICON_LARGE,
      "chrome://theme/IDR_APP_DEFAULT_ICON"));
  value->SetString("icon_small", GetIconURL(
      extension, Extension::EXTENSION_ICON_BITTY,
      std::string("chrome://favicon/") + extension->GetFullLaunchURL().spec()));
  value->SetInteger("launch_container", extension->launch_container());
  value->SetInteger("launch_type",
      extension_prefs->GetLaunchType(extension->id(),
                                     ExtensionPrefs::LAUNCH_DEFAULT));

  int app_launch_index = extension_prefs->GetAppLaunchIndex(extension->id());
  if (app_launch_index == -1) {
    // Make sure every app has a launch index (some predate the launch index).
    app_launch_index = extension_prefs->GetNextAppLaunchIndex();
    extension_prefs->SetAppLaunchIndex(extension->id(), app_launch_index);
  }
  value->SetInteger("app_launch_index", app_launch_index);
}

// static
bool AppLauncherHandler::HandlePing(Profile* profile, const std::string& path) {
  std::vector<std::string> params;
  base::SplitString(path, '+', &params);

  // Check if the user launched an app from the most visited or recently
  // closed sections.
  if (kPingLaunchAppByURL == params.at(0)) {
    CHECK(params.size() == 3);
    RecordAppLaunchByURL(
        profile, params.at(1), ParseLaunchSource(params.at(2)));
    return true;
  }

  bool is_web_store_ping = kPingLaunchWebStore == params.at(0);
  bool is_app_launch_ping = kPingLaunchAppByID == params.at(0);

  if (!is_web_store_ping && !is_app_launch_ping)
    return false;

  CHECK(params.size() >= 2);

  bool is_promo_active = params.at(1) == "true";

  // At this point, the user must have used the app launcher, so we hide the
  // promo if its still displayed.
  if (is_promo_active)
    profile->GetExtensionService()->default_apps()->SetPromoHidden();

  if (is_web_store_ping) {
    RecordWebStoreLaunch(is_promo_active);
  }  else {
    CHECK(params.size() == 3);
    RecordAppLaunchByID(is_promo_active, ParseLaunchSource(params.at(2)));
  }

  return true;
}

WebUIMessageHandler* AppLauncherHandler::Attach(WebUI* web_ui) {
  // TODO(arv): Add initialization code to the Apps store etc.
  return WebUIMessageHandler::Attach(web_ui);
}

void AppLauncherHandler::RegisterMessages() {
  web_ui_->RegisterMessageCallback("getApps",
      NewCallback(this, &AppLauncherHandler::HandleGetApps));
  web_ui_->RegisterMessageCallback("launchApp",
      NewCallback(this, &AppLauncherHandler::HandleLaunchApp));
  web_ui_->RegisterMessageCallback("setLaunchType",
      NewCallback(this, &AppLauncherHandler::HandleSetLaunchType));
  web_ui_->RegisterMessageCallback("uninstallApp",
      NewCallback(this, &AppLauncherHandler::HandleUninstallApp));
  web_ui_->RegisterMessageCallback("hideAppsPromo",
      NewCallback(this, &AppLauncherHandler::HandleHideAppsPromo));
  web_ui_->RegisterMessageCallback("createAppShortcut",
      NewCallback(this, &AppLauncherHandler::HandleCreateAppShortcut));
  web_ui_->RegisterMessageCallback("reorderApps",
      NewCallback(this, &AppLauncherHandler::HandleReorderApps));
}

void AppLauncherHandler::Observe(NotificationType type,
                                 const NotificationSource& source,
                                 const NotificationDetails& details) {
  if (ignore_changes_)
    return;

  switch (type.value) {
    case NotificationType::EXTENSION_LOADED:
    case NotificationType::EXTENSION_UNLOADED:
    case NotificationType::EXTENSION_LAUNCHER_REORDERED:
      if (web_ui_->tab_contents())
        HandleGetApps(NULL);
      break;
    case NotificationType::PREF_CHANGED: {
      if (!web_ui_->tab_contents())
        break;

      DictionaryValue dictionary;
      FillAppDictionary(&dictionary);
      web_ui_->CallJavascriptFunction(L"appsPrefChangeCallback", dictionary);
      break;
    }
    default:
      NOTREACHED();
  }
}

void AppLauncherHandler::FillAppDictionary(DictionaryValue* dictionary) {
  ListValue* list = new ListValue();
  const ExtensionList* extensions = extensions_service_->extensions();
  for (ExtensionList::const_iterator it = extensions->begin();
       it != extensions->end(); ++it) {
    // Don't include the WebStore and other component apps.
    // The WebStore launcher gets special treatment in ntp/apps.js.
    if ((*it)->is_app() && (*it)->location() != Extension::COMPONENT) {
      DictionaryValue* app_info = new DictionaryValue();
      CreateAppInfo(*it, extensions_service_->extension_prefs(), app_info);
      list->Append(app_info);
    }
  }
  dictionary->Set("apps", list);

#if defined(OS_MACOSX)
  // App windows are not yet implemented on mac.
  dictionary->SetBoolean("disableAppWindowLaunch", true);
  dictionary->SetBoolean("disableCreateAppShortcut", true);
#endif

#if defined(OS_CHROMEOS)
  // Making shortcut does not make sense on ChromeOS because it does not have
  // a desktop.
  dictionary->SetBoolean("disableCreateAppShortcut", true);
#endif

  dictionary->SetBoolean(
      "showLauncher",
      extensions_service_->default_apps()->ShouldShowAppLauncher(
          extensions_service_->GetAppIds()));
}

void AppLauncherHandler::HandleGetApps(const ListValue* args) {
  DictionaryValue dictionary;

  // Tell the client whether to show the promo for this view. We don't do this
  // in the case of PREF_CHANGED because:
  //
  // a) At that point in time, depending on the pref that changed, it can look
  //    like the set of apps installed has changed, and we will mark the promo
  //    expired.
  // b) Conceptually, it doesn't really make sense to count a
  //    prefchange-triggered refresh as a promo 'view'.
  DefaultApps* default_apps = extensions_service_->default_apps();
  bool promo_just_expired = false;
  if (default_apps->ShouldShowPromo(extensions_service_->GetAppIds(),
                                    &promo_just_expired)) {
    dictionary.SetBoolean("showPromo", true);
    promo_active_ = true;
  } else {
    if (promo_just_expired) {
      ignore_changes_ = true;
      UninstallDefaultApps();
      ignore_changes_ = false;
      ShownSectionsHandler::SetShownSection(web_ui_->GetProfile()->GetPrefs(),
                                            THUMB);
    }
    dictionary.SetBoolean("showPromo", false);
    promo_active_ = false;
  }

  FillAppDictionary(&dictionary);
  web_ui_->CallJavascriptFunction(L"getAppsCallback", dictionary);

  // First time we get here we set up the observer so that we can tell update
  // the apps as they change.
  if (registrar_.IsEmpty()) {
    registrar_.Add(this, NotificationType::EXTENSION_LOADED,
        NotificationService::AllSources());
    registrar_.Add(this, NotificationType::EXTENSION_UNLOADED,
        NotificationService::AllSources());
    registrar_.Add(this, NotificationType::EXTENSION_LAUNCHER_REORDERED,
        NotificationService::AllSources());
  }
  if (pref_change_registrar_.IsEmpty()) {
    pref_change_registrar_.Init(
        extensions_service_->extension_prefs()->pref_service());
    pref_change_registrar_.Add(ExtensionPrefs::kExtensionsPref, this);
  }
}

void AppLauncherHandler::HandleLaunchApp(const ListValue* args) {
  std::string extension_id;
  std::string source;
  double left;
  double top;
  double width;
  double height;
  bool alt_key;
  bool ctrl_key;
  bool meta_key;
  bool shift_key;
  double button;

  CHECK(args->GetString(0, &extension_id));
  CHECK(args->GetString(1, &source));
  CHECK(args->GetDouble(2, &left));
  CHECK(args->GetDouble(3, &top));
  CHECK(args->GetDouble(4, &width));
  CHECK(args->GetDouble(5, &height));
  CHECK(args->GetBoolean(6, &alt_key));
  CHECK(args->GetBoolean(7, &ctrl_key));
  CHECK(args->GetBoolean(8, &meta_key));
  CHECK(args->GetBoolean(9, &shift_key));
  CHECK(args->GetDouble(10, &button));

  extension_misc::AppLaunchBucket launch_bucket = ParseLaunchSource(source);

  // The rect we get from the client is relative to the browser client viewport.
  // Offset the rect by the tab contents bounds.
  gfx::Rect rect(static_cast<int>(left), static_cast<int>(top),
                 static_cast<int>(width), static_cast<int>(height));
  gfx::Rect tab_contents_bounds;
  web_ui_->tab_contents()->GetContainerBounds(&tab_contents_bounds);
  rect.Offset(tab_contents_bounds.origin());

  const Extension* extension =
      extensions_service_->GetExtensionById(extension_id, false);
  DCHECK(extension);
  Profile* profile = extensions_service_->profile();

  // If the user pressed special keys when clicking, override the saved
  // preference for launch container.
  bool middle_button = (button == 1.0);
  WindowOpenDisposition disposition =
        disposition_utils::DispositionFromClick(middle_button, alt_key,
                                                ctrl_key, meta_key, shift_key);
  if (disposition == NEW_FOREGROUND_TAB || disposition == NEW_BACKGROUND_TAB) {
    // TODO(jamescook): Proper support for background tabs.
    Browser::OpenApplication(
        profile, extension, extension_misc::LAUNCH_TAB, NULL);
  } else if (disposition == NEW_WINDOW) {
    // Force a new window open.
    Browser::OpenApplication(
            profile, extension, extension_misc::LAUNCH_WINDOW, NULL);
  } else {
    // Look at preference to find the right launch container.  If no preference
    // is set, launch as a regular tab.
    extension_misc::LaunchContainer launch_container =
        extensions_service_->extension_prefs()->GetLaunchContainer(
            extension, ExtensionPrefs::LAUNCH_REGULAR);

    // To give a more "launchy" experience when using the NTP launcher, we close
    // it automatically.
    Browser* browser = BrowserList::GetLastActive();
    TabContents* old_contents = NULL;
    if (browser)
      old_contents = browser->GetSelectedTabContents();

    TabContents* new_contents = Browser::OpenApplication(
        profile, extension, launch_container, old_contents);

    if (new_contents != old_contents && browser->tab_count() > 1)
      browser->CloseTabContents(old_contents);
  }

  if (extension_id != extension_misc::kWebStoreAppId) {
    RecordAppLaunchByID(promo_active_, launch_bucket);
    extensions_service_->default_apps()->SetPromoHidden();
  }
}

void AppLauncherHandler::HandleSetLaunchType(const ListValue* args) {
  std::string extension_id;
  double launch_type;
  CHECK(args->GetString(0, &extension_id));
  CHECK(args->GetDouble(1, &launch_type));

  const Extension* extension =
      extensions_service_->GetExtensionById(extension_id, false);
  DCHECK(extension);

  extensions_service_->extension_prefs()->SetLaunchType(
      extension_id,
      static_cast<ExtensionPrefs::LaunchType>(
          static_cast<int>(launch_type)));
}

void AppLauncherHandler::HandleUninstallApp(const ListValue* args) {
  std::string extension_id = WideToUTF8(ExtractStringValue(args));
  const Extension* extension = extensions_service_->GetExtensionById(
      extension_id, false);
  if (!extension)
    return;

  if (!extension_id_prompting_.empty())
    return;  // Only one prompt at a time.

  extension_id_prompting_ = extension_id;
  GetExtensionInstallUI()->ConfirmUninstall(this, extension);
}

void AppLauncherHandler::HandleHideAppsPromo(const ListValue* args) {
  // If the user has intentionally hidden the promotion, we'll uninstall all the
  // default apps (we know the user hasn't installed any apps on their own at
  // this point, or the promotion wouldn't have been shown).
  UMA_HISTOGRAM_ENUMERATION(extension_misc::kAppsPromoHistogram,
                            extension_misc::PROMO_CLOSE,
                            extension_misc::PROMO_BUCKET_BOUNDARY);

  ShownSectionsHandler::SetShownSection(web_ui_->GetProfile()->GetPrefs(),
                                        THUMB);
  ignore_changes_ = true;
  UninstallDefaultApps();
  extensions_service_->default_apps()->SetPromoHidden();
  ignore_changes_ = false;
  HandleGetApps(NULL);
}

void AppLauncherHandler::HandleCreateAppShortcut(const ListValue* args) {
  std::string extension_id;
  if (!args->GetString(0, &extension_id)) {
    NOTREACHED();
    return;
  }

  const Extension* extension =
      extensions_service_->GetExtensionById(extension_id, false);
  CHECK(extension);

  Browser* browser = BrowserList::GetLastActive();
  if (!browser)
    return;
  browser->window()->ShowCreateChromeAppShortcutsDialog(
      browser->profile(), extension);
}

void AppLauncherHandler::HandleReorderApps(const ListValue* args) {
  std::vector<std::string> extension_ids;
  for (size_t i = 0; i < args->GetSize(); ++i) {
    std::string value;
    if (args->GetString(i, &value))
      extension_ids.push_back(value);
  }

  extensions_service_->extension_prefs()->SetAppLauncherOrder(extension_ids);
}

// static
void AppLauncherHandler::RecordWebStoreLaunch(bool promo_active) {
  UMA_HISTOGRAM_ENUMERATION(extension_misc::kAppLaunchHistogram,
                            extension_misc::APP_LAUNCH_NTP_WEBSTORE,
                            extension_misc::APP_LAUNCH_BUCKET_BOUNDARY);

  if (!promo_active) return;

  UMA_HISTOGRAM_ENUMERATION(extension_misc::kAppsPromoHistogram,
                            extension_misc::PROMO_LAUNCH_WEB_STORE,
                            extension_misc::PROMO_BUCKET_BOUNDARY);
}

// static
void AppLauncherHandler::RecordAppLaunchByID(
    bool promo_active, extension_misc::AppLaunchBucket bucket) {
  CHECK(bucket != extension_misc::APP_LAUNCH_BUCKET_INVALID);

  UMA_HISTOGRAM_ENUMERATION(extension_misc::kAppLaunchHistogram, bucket,
                            extension_misc::APP_LAUNCH_BUCKET_BOUNDARY);

  if (!promo_active) return;

  UMA_HISTOGRAM_ENUMERATION(extension_misc::kAppsPromoHistogram,
                            extension_misc::PROMO_LAUNCH_APP,
                            extension_misc::PROMO_BUCKET_BOUNDARY);
}

// static
void AppLauncherHandler::RecordAppLaunchByURL(
    Profile* profile,
    std::string escaped_url,
    extension_misc::AppLaunchBucket bucket) {
  CHECK(bucket != extension_misc::APP_LAUNCH_BUCKET_INVALID);

  GURL url(UnescapeURLComponent(escaped_url, kUnescapeRules));
  if (!profile->GetExtensionService()->IsInstalledApp(url))
    return;

  UMA_HISTOGRAM_ENUMERATION(extension_misc::kAppLaunchHistogram, bucket,
                            extension_misc::APP_LAUNCH_BUCKET_BOUNDARY);
}

void AppLauncherHandler::InstallUIProceed() {
  DCHECK(!extension_id_prompting_.empty());

  // The extension can be uninstalled in another window while the UI was
  // showing. Do nothing in that case.
  const Extension* extension =
      extensions_service_->GetExtensionById(extension_id_prompting_, true);
  if (!extension)
    return;

  extensions_service_->UninstallExtension(extension_id_prompting_,
                                          false /* external_uninstall */);
  extension_id_prompting_ = "";
}

void AppLauncherHandler::InstallUIAbort() {
  extension_id_prompting_ = "";
}

ExtensionInstallUI* AppLauncherHandler::GetExtensionInstallUI() {
  if (!install_ui_.get())
    install_ui_.reset(new ExtensionInstallUI(web_ui_->GetProfile()));
  return install_ui_.get();
}

void AppLauncherHandler::UninstallDefaultApps() {
  DefaultApps* default_apps = extensions_service_->default_apps();
  const ExtensionIdSet& app_ids = default_apps->default_apps();
  for (ExtensionIdSet::const_iterator iter = app_ids.begin();
       iter != app_ids.end(); ++iter) {
    if (extensions_service_->GetExtensionById(*iter, true))
      extensions_service_->UninstallExtension(*iter, false);
  }
}
