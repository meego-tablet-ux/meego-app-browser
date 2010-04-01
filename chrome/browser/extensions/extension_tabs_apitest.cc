// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_apitest.h"

#include "chrome/browser/browser.h"
#include "chrome/browser/pref_service.h"
#include "chrome/browser/profile.h"
#include "chrome/common/pref_names.h"

// TODO(skerner): This test is flaky on chrome os: http://crbug.com/39843
// TODO(skerner): Crash observed on linux as well: http://crbug.com/39746
#if defined(OS_LINUX)
#define MAYBE_Tabs DISABLED_Tabs
#else
#define MAYBE_Tabs Tabs
#endif

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, MAYBE_Tabs) {
  StartHTTPServer();

  // The test creates a tab and checks that the URL of the new tab
  // is that of the new tab page.  Make sure the pref that controls
  // this is set.
  browser()->profile()->GetPrefs()->SetBoolean(
      prefs::kHomePageIsNewTabPage, true);

  ASSERT_TRUE(RunExtensionTest("tabs/basics")) << message_;
}

// chrome.tabs.captureVisibleTab fails on the 10.6 bots.
// http://crbug.com/37387
#if defined(OS_MACOSX)
#define MAYBE_CaptureVisibleTab DISABLED_CaptureVisible
#else
#define MAYBE_CaptureVisibleTab CaptureVisibleTab
#endif
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, MAYBE_CaptureVisibleTab) {
  StartHTTPServer();

  ASSERT_TRUE(RunExtensionTest("tabs/capture_visible_tab")) << message_;
}
