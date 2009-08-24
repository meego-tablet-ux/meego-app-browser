// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_DEVTOOLS_BROWSERTEST_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_DEVTOOLS_BROWSERTEST_H_

#include "base/command_line.h"
#include "chrome/browser/extensions/extension_browsertest.h"

// Subclass of ExtensionBrowserTest that enables the devtools
// command line features.
class ExtensionDevToolsBrowserTest : public ExtensionBrowserTest {
 protected:
  virtual void SetUpCommandLine(CommandLine* command_line);

 private:

  NotificationRegistrar registrar_;
};

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_DEVTOOLS_BROWSERTEST_H_
