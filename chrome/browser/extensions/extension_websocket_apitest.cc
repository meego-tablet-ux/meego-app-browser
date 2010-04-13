// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/ui_test_utils.h"
#include "net/base/mock_host_resolver.h"

// Disable this test on Linux, because it fails
// http://crbug.com/40976
#if defined(OS_LINUX)
#define WebSocket DISABLED_WebSocket
#endif

// This test is disabled. See bug 41319
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, DISABLED_WebSocket) {
  FilePath websocket_root_dir;
  PathService::Get(chrome::DIR_TEST_DATA, &websocket_root_dir);
  websocket_root_dir = websocket_root_dir.AppendASCII("layout_tests")
      .AppendASCII("LayoutTests");
  ui_test_utils::TestWebSocketServer server(websocket_root_dir);
  ASSERT_TRUE(RunExtensionTest("websocket")) << message_;
}
