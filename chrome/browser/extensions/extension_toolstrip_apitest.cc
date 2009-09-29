// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_apitest.h"

// TEMPORARILY ENABLED TO GET DEBUG OUTPUT:
// TODO(rafaelw,erikkay) disabled due to flakiness
// BUG=22668 (probably the same bug)
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, Toolstrip) {
  ASSERT_TRUE(RunExtensionTest("toolstrip")) << message_;
}
