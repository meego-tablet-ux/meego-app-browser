// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_apitest.h"

class ExtensionModuleApiTest : public ExtensionApiTest {
};

IN_PROC_BROWSER_TEST_F(ExtensionModuleApiTest, Basics) {
  ASSERT_TRUE(RunExtensionTest("extension_module")) << message_;
}
