// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/common/chrome_switches.h"

#if defined(TOOLKIT_VIEWS)
#define MAYBE_Infobars Infobars
#elif defined(OS_MACOSX)
// Temporarily marked as FAILS on OSX. See http://crbug.com/60990 for details.
#define MAYBE_Infobars FAILS_Infobars
#else
// Need to finish port to Linux. See http://crbug.com/39916 for details.
#define MAYBE_Infobars DISABLED_Infobars
#endif

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, MAYBE_Infobars) {
  // TODO(finnur): Remove once infobars are no longer experimental (bug 39511).
  CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kEnableExperimentalExtensionApis);

  ASSERT_TRUE(RunExtensionTest("infobars")) << message_;
}
