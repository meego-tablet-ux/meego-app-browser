// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/file_util.h"
#include "base/path_service.h"
#include "build/build_config.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/automation/tab_proxy.h"
#include "chrome/test/ui/ui_test.h"
#include "net/base/net_util.h"
#include "webkit/glue/plugins/plugin_switches.h"

namespace {

// Platform-specific filename relative to the chrome executable.
#if defined(OS_WIN)
const wchar_t library_name[] = L"ppapi_tests.dll";
#elif defined(OS_MACOSX)
const char library_name[] = "ppapi_tests.plugin";
#elif defined(OS_POSIX)
const char library_name[] = "libppapi_tests.so";
#endif

}  // namespace

class PPAPITest : public UITest {
 public:
  PPAPITest() {
    // Append the switch to register the pepper plugin.
    // library name = <out dir>/<test_name>.<library_extension>
    // MIME type = application/x-ppapi-<test_name>
    FilePath plugin_dir;
    PathService::Get(base::DIR_EXE, &plugin_dir);

    FilePath plugin_lib = plugin_dir.Append(library_name);
    EXPECT_TRUE(file_util::PathExists(plugin_lib));

#if defined(OS_WIN)
    std::wstring pepper_plugin = plugin_lib.value();
#else
    std::wstring pepper_plugin = UTF8ToWide(plugin_lib.value());
#endif
    pepper_plugin.append(L";application/x-ppapi-tests");
    launch_arguments_.AppendSwitchWithValue(switches::kRegisterPepperPlugins,
                                            pepper_plugin);

    // The test sends us the result via a cookie.
    launch_arguments_.AppendSwitch(switches::kEnableFileCookies);

    // Some stuff is hung off of the testing interface which is not enabled
    // by default.
    launch_arguments_.AppendSwitch(switches::kEnablePepperTesting);
  }

  void RunTest(const FilePath::StringType& test_file_name) {
    FilePath test_path;
    PathService::Get(base::DIR_SOURCE_ROOT, &test_path);
    test_path = test_path.Append(FILE_PATH_LITERAL("third_party"));
    test_path = test_path.Append(FILE_PATH_LITERAL("ppapi"));
    test_path = test_path.Append(FILE_PATH_LITERAL("tests"));
    test_path = test_path.Append(test_file_name);

    // Sanity check the file name.
    EXPECT_TRUE(file_util::PathExists(test_path));

    GURL test_url = net::FilePathToFileURL(test_path);
    scoped_refptr<TabProxy> tab(GetActiveTab());
    ASSERT_TRUE(tab.get());
    ASSERT_TRUE(tab->NavigateToURL(test_url));
    std::string escaped_value =
        WaitUntilCookieNonEmpty(tab.get(), test_url,
            "COMPLETION_COOKIE", action_max_timeout_ms());
    EXPECT_STREQ("PASS", escaped_value.c_str());
  }
};

// TODO(brettw) fails on Mac, Linux 64 & Windows for unknown reasons.
TEST_F(PPAPITest, DeviceContext2D) {
  RunTest(FILE_PATH_LITERAL("test_device_context_2d.html"));
}

#if defined(OS_MACOSX)
// TODO(brettw) this fails on Mac for unknown reasons.
TEST_F(PPAPITest, DISABLED_ImageData) {
#else
TEST_F(PPAPITest, ImageData) {
#endif
  RunTest(FILE_PATH_LITERAL("test_image_data.html"));
}
