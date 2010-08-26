// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_management_api.h"

#include <map>
#include <string>

#include "base/string_number_conversions.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/extensions/extensions_service.h"
#include "chrome/common/extensions/extension_error_utils.h"

using base::IntToString;

const char kAppLaunchUrlKey[] = "appLaunchUrl";
const char kEnabledKey[] = "enabled";
const char kIconsKey[] = "icons";
const char kIdKey[] = "id";
const char kIsAppKey[] = "isApp";
const char kNameKey[] = "name";
const char kOptionsUrlKey[] = "optionsUrl";
const char kSizeKey[] = "size";
const char kUrlKey[] = "url";

const char kNoExtensionError[] = "No extension with id *";


ExtensionsService* ExtensionManagementFunction::service() {
  return profile()->GetExtensionsService();
}

static DictionaryValue* CreateExtensionInfo(const Extension& extension,
                                            bool enabled) {
  DictionaryValue* info = new DictionaryValue();
  info->SetString(kIdKey, extension.id());
  info->SetBoolean(kIsAppKey, extension.is_app());
  info->SetString(kNameKey, extension.name());
  info->SetBoolean(kEnabledKey, enabled);
  if (!extension.options_url().is_empty())
    info->SetString(kOptionsUrlKey,
                    extension.options_url().possibly_invalid_spec());
  if (extension.is_app())
    info->SetString(kAppLaunchUrlKey,
                    extension.GetFullLaunchURL().possibly_invalid_spec());

  const std::map<int, std::string>& icons = extension.icons();
  if (!icons.empty()) {
    ListValue* icon_list = new ListValue();
    std::map<int, std::string>::const_iterator icon_iter;
    for (icon_iter = icons.begin(); icon_iter != icons.end(); ++icon_iter) {
      DictionaryValue* icon_info = new DictionaryValue();
      GURL url = extension.GetResourceURL(icon_iter->second);
      icon_info->SetInteger(kSizeKey, icon_iter->first);
      icon_info->SetString(kUrlKey, url.possibly_invalid_spec());
      icon_list->Append(icon_info);
    }
    info->Set("icons", icon_list);
  }

  return info;
}

static void AddExtensionInfo(ListValue* list,
                             const ExtensionList& extensions,
                             bool enabled) {
  for (ExtensionList::const_iterator i = extensions.begin();
       i != extensions.end(); ++i) {
    const Extension& extension = **i;

    if (extension.location() == Extension::COMPONENT)
      continue;  // Skip built-in extensions.

    list->Append(CreateExtensionInfo(extension, enabled));
  }
}

bool GetAllExtensionsFunction::RunImpl() {
  ListValue* result = new ListValue();
  result_.reset(result);

  AddExtensionInfo(result, *service()->extensions(), true);
  AddExtensionInfo(result, *service()->disabled_extensions(), false);

  return true;
}

bool SetEnabledFunction::RunImpl() {
  std::string extension_id;
  bool enable;
  EXTENSION_FUNCTION_VALIDATE(args_->GetString(0, &extension_id));
  EXTENSION_FUNCTION_VALIDATE(args_->GetBoolean(1, &enable));

  if (!service()->GetExtensionById(extension_id, true)) {
    error_ = ExtensionErrorUtils::FormatErrorMessage(
        kNoExtensionError, extension_id);
    return false;
  }

  ExtensionPrefs* prefs = service()->extension_prefs();
  Extension::State state = prefs->GetExtensionState(extension_id);

  if (state == Extension::DISABLED && enable) {
    service()->EnableExtension(extension_id);
  } else if (state == Extension::ENABLED && !enable) {
    service()->DisableExtension(extension_id);
  }

  return true;
}

bool InstallFunction::RunImpl() {
  NOTIMPLEMENTED();
  return false;
}

bool UninstallFunction::RunImpl() {
  std::string extension_id;
  EXTENSION_FUNCTION_VALIDATE(args_->GetString(0, &extension_id));

  if (!service()->GetExtensionById(extension_id, true)) {
    error_ = ExtensionErrorUtils::FormatErrorMessage(
        kNoExtensionError, extension_id);
    return false;
  }

  service()->UninstallExtension(extension_id, false /* external_uninstall */);
  return true;
}
