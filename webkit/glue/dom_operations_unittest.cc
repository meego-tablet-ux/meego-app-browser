// Copyright 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "base/file_util.h"
#include "base/path_service.h"
#include "base/string_util.h"
#include "net/base/net_util.h"
#include "net/url_request/url_request_context.h"
#include "webkit/glue/dom_operations.h"
#include "webkit/glue/webview.h"
#include "webkit/glue/webframe.h"
#include "webkit/tools/test_shell/simple_resource_loader_bridge.h"
#include "webkit/tools/test_shell/test_shell_test.h"

namespace {

class DomOperationsTests : public TestShellTest {
 public:
  // Test function GetAllSavableResourceLinksForCurrentPage with a web page.
  // We expect result of GetAllSavableResourceLinksForCurrentPage exactly
  // matches expected_resources_set.
  void GetSavableResourceLinksForPage(const std::wstring& page_file_path,
      const std::set<GURL>& expected_resources_set);

 protected:
  // testing::Test
  virtual void SetUp() {
    TestShellTest::SetUp();
  }

  virtual void TearDown() {
    TestShellTest::TearDown();
  }
};

}  // namespace


void DomOperationsTests::GetSavableResourceLinksForPage(
    const std::wstring& page_file_path,
    const std::set<GURL>& expected_resources_set) {
  // Convert local file path to file URL.
  GURL file_url = net::FilePathToFileURL(page_file_path);
  // Load the test file.
  test_shell_->ResetTestController();
  std::wstring file_wurl = ASCIIToWide(file_url.spec());
  test_shell_->LoadURL(file_wurl.c_str());
  test_shell_->WaitTestFinished();
  // Get all savable resource links for the page.
  std::vector<GURL> resources_list;
  std::vector<GURL> referrers_list;
  std::vector<GURL> frames_list;
  webkit_glue::SavableResourcesResult result(&resources_list,
                                             &referrers_list,
                                             &frames_list);

  GURL main_page_gurl(file_wurl);
  ASSERT_TRUE(webkit_glue::GetAllSavableResourceLinksForCurrentPage(
      test_shell_->webView(), main_page_gurl, &result));
  // Check all links of sub-resource
  for (std::vector<GURL>::const_iterator cit = resources_list.begin();
       cit != resources_list.end(); ++cit) {
    ASSERT_TRUE(expected_resources_set.find(*cit) !=
                expected_resources_set.end());
  }
  // Check all links of frame.
  for (std::vector<GURL>::const_iterator cit = frames_list.begin();
       cit != frames_list.end(); ++cit) {
    ASSERT_TRUE(expected_resources_set.find(*cit) !=
                expected_resources_set.end());
  }
}

// Test function GetAllSavableResourceLinksForCurrentPage with a web page
// which has valid savable resource links.
TEST_F(DomOperationsTests, GetSavableResourceLinksWithPageHasValidLinks) {
  std::set<GURL> expected_resources_set;
  // Set directory of test data.
  std::wstring page_file_path = data_dir_;
  file_util::AppendToPath(&page_file_path, L"dom_serializer");

  const wchar_t* expected_sub_resource_links[] = {
    L"file:///c:/yt/css/base_all-vfl36460.css",
    L"file:///c:/yt/js/base_all_with_bidi-vfl36451.js",
    L"file:///c:/yt/img/pixel-vfl73.gif"
  };
  const wchar_t* expected_frame_links[] = {
    L"youtube_1.htm",
    L"youtube_2.htm"
  };
  // Add all expected links of sub-resource to expected set.
  for (int i = 0; i < arraysize(expected_sub_resource_links); ++i)
    expected_resources_set.insert(GURL(expected_sub_resource_links[i]));
  // Add all expected links of frame to expected set.
  for (int i = 0; i < arraysize(expected_frame_links); ++i) {
    std::wstring expected_frame_url = page_file_path;
    file_util::AppendToPath(&expected_frame_url, expected_frame_links[i]);
    expected_resources_set.insert(
        net::FilePathToFileURL(expected_frame_url));
  }

  file_util::AppendToPath(&page_file_path, std::wstring(L"youtube_1.htm"));
  GetSavableResourceLinksForPage(page_file_path, expected_resources_set);
}

// Test function GetAllSavableResourceLinksForCurrentPage with a web page
// which does not have valid savable resource links.
TEST_F(DomOperationsTests, GetSavableResourceLinksWithPageHasInvalidLinks) {
  std::set<GURL> expected_resources_set;
  // Set directory of test data.
  std::wstring page_file_path = data_dir_;
  file_util::AppendToPath(&page_file_path, L"dom_serializer");

  const wchar_t* expected_frame_links[] = {
    L"youtube_2.htm"
  };
  // Add all expected links of frame to expected set.
  for (int i = 0; i < arraysize(expected_frame_links); ++i) {
    std::wstring expected_frame_url = page_file_path;
    file_util::AppendToPath(&expected_frame_url, expected_frame_links[i]);
    expected_resources_set.insert(
        net::FilePathToFileURL(expected_frame_url));
  }

  file_util::AppendToPath(&page_file_path, std::wstring(L"youtube_2.htm"));
  GetSavableResourceLinksForPage(page_file_path, expected_resources_set);
}

// Tests ParseIconSizes with various input.
TEST_F(DomOperationsTests, ParseIconSizes) {
  struct TestData {
    const std::wstring input;
    const bool expected_result;
    const bool is_any;
    const int expected_size_count;
    const int width1;
    const int height1;
    const int width2;
    const int height2;
  } data[] = {
    // Bogus input cases.
    { L"10",         false, false, 0, 0, 0, 0, 0 },
    { L"10 10",      false, false, 0, 0, 0, 0, 0 },
    { L"010",        false, false, 0, 0, 0, 0, 0 },
    { L" 010 ",      false, false, 0, 0, 0, 0, 0 },
    { L" 10x ",      false, false, 0, 0, 0, 0, 0 },
    { L" x10 ",      false, false, 0, 0, 0, 0, 0 },
    { L"any 10x10",  false, false, 0, 0, 0, 0, 0 },
    { L"",           false, false, 0, 0, 0, 0, 0 },
    { L"10ax11",     false, false, 0, 0, 0, 0, 0 },

    // Any.
    { L"any",        true, true, 0, 0, 0, 0, 0 },
    { L" any",       true, true, 0, 0, 0, 0, 0 },
    { L" any ",      true, true, 0, 0, 0, 0, 0 },

    // Sizes.
    { L"10x11",      true, false, 1, 10, 11, 0, 0 },
    { L" 10x11 ",    true, false, 1, 10, 11, 0, 0 },
    { L" 10x11 1x2", true, false, 2, 10, 11, 1, 2 },
  };
  for (size_t i = 0; i < arraysize(data); ++i) {
    bool is_any;
    std::vector<gfx::Size> sizes;
    const bool result =
        webkit_glue::ParseIconSizes(data[i].input, &sizes, &is_any);
    ASSERT_EQ(result, data[i].expected_result);
    if (result) {
      ASSERT_EQ(data[i].is_any, is_any);
      ASSERT_EQ(data[i].expected_size_count, sizes.size());
      if (sizes.size() > 0) {
        ASSERT_EQ(data[i].width1, sizes[0].width());
        ASSERT_EQ(data[i].height1, sizes[0].height());
      }
      if (sizes.size() > 1) {
        ASSERT_EQ(data[i].width2, sizes[1].width());
        ASSERT_EQ(data[i].height2, sizes[1].height());
      }
    }
  }
}
