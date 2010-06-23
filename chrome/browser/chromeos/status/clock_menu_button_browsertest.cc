// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/status/clock_menu_button.h"

#include "base/string_util.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/browser_window.h"
#include "chrome/browser/chromeos/frame/browser_view.h"
#include "chrome/browser/chromeos/status/browser_status_area_view.h"
#include "chrome/browser/chromeos/view_ids.h"
#include "chrome/browser/pref_member.h"
#include "chrome/browser/profile.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/in_process_browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "unicode/calendar.h"
#include "unicode/timezone.h"

namespace chromeos {

class ClockMenuButtonTest : public InProcessBrowserTest {
 protected:
  ClockMenuButtonTest() : InProcessBrowserTest() {}
  ClockMenuButton* GetClockMenuButton() {
    BrowserView* view = static_cast<BrowserView*>(browser()->window());
    return static_cast<BrowserStatusAreaView*>(view->
        GetViewByID(VIEW_ID_STATUS_AREA))->clock_view();
  }
};

IN_PROC_BROWSER_TEST_F(ClockMenuButtonTest, TimezoneTest) {
  ClockMenuButton* clock = GetClockMenuButton();
  ASSERT_TRUE(clock != NULL);
  // Update timezone and make sure clock text changes.
  std::wstring text_before = clock->text();
  StringPrefMember timezone;
  timezone.Init(prefs::kTimeZone, browser()->profile()->GetPrefs(), NULL);
  timezone.SetValue(ASCIIToWide("Asia/Hong_Kong"));
  std::wstring text_after = clock->text();
  EXPECT_NE(text_before, text_after);
}

}  // namespace chromeos
