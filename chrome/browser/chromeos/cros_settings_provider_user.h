// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_CROS_SETTINGS_PROVIDER_USER_H_
#define CHROME_BROWSER_CHROMEOS_CROS_SETTINGS_PROVIDER_USER_H_
#pragma once

#include <string>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "chrome/browser/chromeos/cros_settings_provider.h"
#include "chrome/browser/chromeos/login/signed_settings_helper.h"

class DictionaryValue;

namespace chromeos {

class UserCrosSettingsProvider : public CrosSettingsProvider,
                                 public SignedSettingsHelper::Callback {
 public:
  UserCrosSettingsProvider();
  virtual ~UserCrosSettingsProvider();

  // CrosSettingsProvider implementation.
  virtual void Set(const std::string& path, Value* in_value);
  virtual bool Get(const std::string& path, Value** out_value) const;
  virtual bool HandlesSetting(const std::string& path);

  // SignedSettingsHelper::Callback overrides.
  virtual void OnWhitelistCompleted(bool success, const std::string& email);
  virtual void OnUnwhitelistCompleted(bool success, const std::string& email);
  virtual void OnStorePropertyCompleted(
      bool success, const std::string& name, const std::string& value);
  virtual void OnRetrievePropertyCompleted(
      bool success, const std::string& name, const std::string& value);

  void WhitelistUser(const std::string& email);
  void UnwhitelistUser(const std::string& email);

 private:
  void StartFetchingBoolSetting(const std::string& name, bool default_value);
  void LoadUserWhitelist();

  scoped_ptr<DictionaryValue> cache_;
  bool current_user_is_owner_;

  DISALLOW_COPY_AND_ASSIGN(UserCrosSettingsProvider);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_CROS_SETTINGS_PROVIDER_USER_H_
