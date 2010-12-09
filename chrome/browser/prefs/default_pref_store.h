// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PREFS_DEFAULT_PREF_STORE_H_
#define CHROME_BROWSER_PREFS_DEFAULT_PREF_STORE_H_
#pragma once

#include <map>

#include "base/basictypes.h"
#include "chrome/browser/prefs/value_map_pref_store.h"

// This PrefStore keeps track of default preference values set when a
// preference is registered with the PrefService.
class DefaultPrefStore : public ValueMapPrefStore {
 public:
  DefaultPrefStore() {}
  virtual ~DefaultPrefStore() {}

  // Stores a new |value| for |key|. Assumes ownership of |value|.
  void SetDefaultValue(const std::string& key, Value* value) {
    SetValue(key, value);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(DefaultPrefStore);
};

#endif  // CHROME_BROWSER_PREFS_DEFAULT_PREF_STORE_H_
