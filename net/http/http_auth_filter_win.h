// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_AUTH_FILTER_WIN_H_
#define NET_HTTP_HTTP_AUTH_FILTER_WIN_H_
#pragma once

#include <string>

#include "build/build_config.h"

#if defined(OS_WIN)
#include "base/string_util.h"

namespace net {

enum RegistryHiveType {
  CURRENT_USER,
  LOCAL_MACHINE
};

namespace http_auth {

// The common path to all the registry keys containing domain zone information.
extern const char16 kRegistryInternetSettings[];
extern const char16 kSettingsMachineOnly[];
extern const char16* kRegistryEntries[3];       // L"http", L"https", and L"*"

extern const char16* GetRegistryWhitelistKey();
// Override the whitelist key.  Passing in NULL restores the default value.
extern void SetRegistryWhitelistKey(const char16* new_whitelist_key);
extern bool UseOnlyMachineSettings();

}  // namespace http_auth

}  // namespace net
#endif  // OS_WIN

#endif  // NET_HTTP_HTTP_AUTH_FILTER_WIN_H_
