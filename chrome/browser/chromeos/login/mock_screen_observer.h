// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_MOCK_SCREEN_OBSERVER_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_MOCK_SCREEN_OBSERVER_H_

#include <string>

#include "chrome/browser/chromeos/login/screen_observer.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace chromeos {

// Interface that handles notifications received from any of login wizard
// screens.
class MockScreenObserver : public ScreenObserver {
 public:
  MockScreenObserver() {}
  virtual ~MockScreenObserver() {}
  MOCK_METHOD1(OnExit, void(ExitCodes));
  MOCK_METHOD2(OnSwitchLanguage, void(const std::string&,
                                      ScreenObserver::ExitCodes));
  MOCK_METHOD2(OnSetUserNamePassword,
               void(const std::string&, const std::string&));
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_MOCK_SCREEN_OBSERVER_H_
