// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_apitest.h"

#if defined(OS_LINUX) || defined(OS_MACOSX)
// See http://crbug.com/42943.
#define MAYBE_Storage FLAKY_Storage
#else
#define MAYBE_Storage Storage
#endif

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, MAYBE_Storage) {
  ASSERT_TRUE(RunExtensionTest("storage")) << message_;
}
