// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_apitest.h"

#include "chrome/browser/notifications/desktop_notification_service.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/ui/browser.h"

// Fails and hoses bot, http://crbug.com/50060.
// Flaky, http://crbug.com/42314.
#if defined(OS_MACOSX)
#define MAYBE_Notifications DISABLED_Notifications
#else
#define MAYBE_Notifications FLAKY_Notifications
#endif
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, MAYBE_Notifications) {
#if defined(OS_LINUX) && defined(TOOLKIT_VIEWS)
  // Notifications not supported on linux/views yet.
#else
  ASSERT_TRUE(RunExtensionTest("notifications/has_not_permission")) << message_;
  ASSERT_TRUE(RunExtensionTest("notifications/has_permission_manifest"))
      << message_;
  browser()->profile()->GetDesktopNotificationService()
      ->GrantPermission(GURL(
          "chrome-extension://peoadpeiejnhkmpaakpnompolbglelel"));
  ASSERT_TRUE(RunExtensionTest("notifications/has_permission_prefs"))
      << message_;
#endif
}

