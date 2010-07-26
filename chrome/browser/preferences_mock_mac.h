// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PREFERENCES_MOCK_MAC_H_
#define CHROME_BROWSER_PREFERENCES_MOCK_MAC_H_
#pragma once

#include "base/scoped_cftyperef.h"
#include "chrome/browser/preferences_mac.h"

// Mock preferences wrapper for testing code that interacts with CFPreferences.
class MockPreferences : public MacPreferences {
 public:
  MockPreferences();
  virtual ~MockPreferences();

  virtual CFPropertyListRef CopyAppValue(CFStringRef key,
                                         CFStringRef applicationID);

  virtual Boolean AppValueIsForced(CFStringRef key, CFStringRef applicationID);

  // Adds a preference item with the given info to the test set.
  void AddTestItem(CFStringRef key, CFPropertyListRef value, bool is_forced);

 private:
  scoped_cftyperef<CFMutableDictionaryRef> values_;
  scoped_cftyperef<CFMutableSetRef> forced_;
};

#endif  // CHROME_BROWSER_PREFERENCES_MOCK_MAC_H_
