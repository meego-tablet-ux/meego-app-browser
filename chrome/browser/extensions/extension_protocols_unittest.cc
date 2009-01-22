// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_protocols.h"
#include "testing/gtest/include/gtest/gtest.h"

class ExtensionProtocolsTest : public testing::Test {
};

TEST(ExtensionProtocolsTest, GetPathForExtensionResource) {
#if defined(OS_WIN)
  FilePath extension_path(FILE_PATH_LITERAL("C:\\myextension"));
  EXPECT_EQ(std::wstring(L"C:\\myextension\\foo\\bar.gif"),
            GetPathForExtensionResource(extension_path, "/foo/bar.gif").value());
  EXPECT_EQ(std::wstring(L"C:\\myextension\\"),
            GetPathForExtensionResource(extension_path, "/").value());
  EXPECT_EQ(std::wstring(L"C:\\myextension\\c:\\foo.gif"),
            GetPathForExtensionResource(extension_path, "/c:/foo.gif").value());
  EXPECT_EQ(std::wstring(L""),
            GetPathForExtensionResource(extension_path, "/../foo.gif").value());
#else
  FilePath extension_path(FILE_PATH_LITERAL("/myextension"));
  EXPECT_EQ(std::wstring("/myextension/foo/bar.gif"),
            GetPathForExtensionResource(extension_path, "/foo/bar.gif").value());
  EXPECT_EQ(std::wstring("/myextension/"),
            GetPathForExtensionResource(extension_path, "/").value());
  EXPECT_EQ(std::wstring(""),
            GetPathForExtensionResource(extension_path, "/../foo.gif").value());
#endif


}
