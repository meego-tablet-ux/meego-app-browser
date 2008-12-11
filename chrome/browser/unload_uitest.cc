// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/file_util.h"

#include "chrome/browser/automation/url_request_mock_http_job.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/automation/browser_proxy.h"
#include "chrome/test/ui/ui_test.h"
#include "net/url_request/url_request_unittest.h"

const std::string NOLISTENERS_HTML =
    "<html><head><title>nolisteners</title></head><body></body></html>";

const std::string UNLOAD_HTML =
    "<html><head><title>unload</title></head><body>"
    "<script>window.onunload=function(e){}</script></body></html>";

const std::string INFINITE_UNLOAD_HTML =
    "<html><head><title>infiniteunload</title></head><body>"
    "<script>window.onunload=function(e){while(true){}}</script>"
    "</body></html>";

const std::string INFINITE_BEFORE_UNLOAD_HTML =
    "<html><head><title>infinitebeforeunload</title></head><body>"
    "<script>window.onunload=function(e){while(true){}}</script>"
    "</body></html>";

const std::string INFINITE_UNLOAD_ALERT_HTML =
    "<html><head><title>infiniteunloadalert</title></head><body>"
    "<script>window.onunload=function(e){"
      "while(true) {}"
      "alert('foo');"
    "}</script></body></html>";

const std::string TWO_SECOND_UNLOAD_ALERT_HTML =
    "<html><head><title>twosecondunloadalert</title></head><body>"
    "<script>window.onunload=function(e){"
      "var start = new Date().getTime();"
      "while(new Date().getTime() - start < 2000) {}"
      "alert('foo');"
    "}</script></body></html>";

class UnloadTest : public UITest {
 public:
  void CheckTitle(const std::wstring& expected_title) {
    const int kCheckDelayMs = 100;
    int max_wait_time = 5000;
    while (max_wait_time > 0) {
      max_wait_time -= kCheckDelayMs;
      Sleep(kCheckDelayMs);
      if (expected_title == GetActiveTabTitle())
        break;
    }

    EXPECT_EQ(expected_title, GetActiveTabTitle());
  }

  void NavigateToDataURL(const std::string& html_content,
                         const std::wstring& expected_title) {
    NavigateToURL(GURL("data:text/html," + html_content));
    CheckTitle(expected_title);
  }

  void NavigateToNolistenersFileTwice() {
    NavigateToURL(
        URLRequestMockHTTPJob::GetMockUrl(L"title2.html"));
    CheckTitle(L"Title Of Awesomeness");
    NavigateToURL(
        URLRequestMockHTTPJob::GetMockUrl(L"title2.html"));
    CheckTitle(L"Title Of Awesomeness");
  }

  // Navigates to a URL asynchronously, then again synchronously. The first
  // load is purposely async to test the case where the user loads another
  // page without waiting for the first load to complete.
  void NavigateToNolistenersFileTwiceAsync() {
    // TODO(ojan): We hit a DCHECK in RenderViewHost::OnMsgShouldCloseACK
    // if we don't sleep here.
    Sleep(400);
    NavigateToURLAsync(
        URLRequestMockHTTPJob::GetMockUrl(L"title2.html"));
    Sleep(400);
    NavigateToURLAsync(
        URLRequestMockHTTPJob::GetMockUrl(L"title2.html"));

    CheckTitle(L"Title Of Awesomeness");
  }
  
  void LoadUrlAndQuitBrowser(const std::string& html_content,
                             const std::wstring& expected_title = L"") {
    scoped_ptr<BrowserProxy> browser(automation()->GetBrowserWindow(0));
    NavigateToDataURL(html_content, expected_title);
    bool application_closed = false;
    EXPECT_TRUE(CloseBrowser(browser.get(), &application_closed));
  }
};

// Navigate to a page with an infinite unload handler.
// Then two two async crosssite requests to ensure
// we don't get confused and think we're closing the tab.
TEST_F(UnloadTest, CrossSiteInfiniteUnloadAsync) {
  // Tests makes no sense in single-process mode since the renderer is hung.
  if (CommandLine().HasSwitch(switches::kSingleProcess))
    return;

  NavigateToDataURL(INFINITE_UNLOAD_HTML, L"infiniteunload");
  // Must navigate to a non-data URL to trigger cross-site codepath.
  NavigateToNolistenersFileTwiceAsync();
  ASSERT_TRUE(IsBrowserRunning());
}

// Navigate to a page with an infinite unload handler.
// Then two two sync crosssite requests to ensure
// we correctly nav to each one. 
TEST_F(UnloadTest, CrossSiteInfiniteUnloadSync) {
  // Tests makes no sense in single-process mode since the renderer is hung.
  if (CommandLine().HasSwitch(switches::kSingleProcess))
    return;

  NavigateToDataURL(INFINITE_UNLOAD_HTML, L"infiniteunload");
  // Must navigate to a non-data URL to trigger cross-site codepath.
  NavigateToNolistenersFileTwice();
  ASSERT_TRUE(IsBrowserRunning());
}

// Navigate to a page with an infinite beforeunload handler.
// Then two two async crosssite requests to ensure
// we don't get confused and think we're closing the tab.
TEST_F(UnloadTest, CrossSiteInfiniteBeforeUnloadAsync) {
  // Tests makes no sense in single-process mode since the renderer is hung.
  if (CommandLine().HasSwitch(switches::kSingleProcess))
    return;

  NavigateToDataURL(INFINITE_BEFORE_UNLOAD_HTML, L"infinitebeforeunload");
  // Must navigate to a non-data URL to trigger cross-site codepath.
  NavigateToNolistenersFileTwiceAsync();
  ASSERT_TRUE(IsBrowserRunning());
}

// Navigate to a page with an infinite beforeunload handler.
// Then two two sync crosssite requests to ensure
// we correctly nav to each one. 
TEST_F(UnloadTest, CrossSiteInfiniteBeforeUnloadSync) {
  // Tests makes no sense in single-process mode since the renderer is hung.
  if (CommandLine().HasSwitch(switches::kSingleProcess))
    return;

  NavigateToDataURL(INFINITE_BEFORE_UNLOAD_HTML, L"infinitebeforeunload");
  // Must navigate to a non-data URL to trigger cross-site codepath.
  NavigateToNolistenersFileTwice();
  ASSERT_TRUE(IsBrowserRunning());
}

// Tests closing the browser on a page with no unload listeners registered.
TEST_F(UnloadTest, BrowserCloseNoUnloadListeners) {
  LoadUrlAndQuitBrowser(NOLISTENERS_HTML, L"nolisteners");
}

// Tests closing the browser on a page with an unload listener registered.
TEST_F(UnloadTest, BrowserCloseUnload) {
  LoadUrlAndQuitBrowser(UNLOAD_HTML, L"unload");
}

// Tests closing the browser on a page with an unload listener registered where
// the unload handler has an infinite loop.
TEST_F(UnloadTest, BrowserCloseInfiniteUnload) {
  LoadUrlAndQuitBrowser(INFINITE_UNLOAD_HTML, L"infiniteunload");
}

// Tests closing the browser on a page with an unload listener registered where
// the unload handler has an infinite loop followed by an alert.
TEST_F(UnloadTest, BrowserCloseInfiniteUnloadAlert) {
  LoadUrlAndQuitBrowser(INFINITE_UNLOAD_ALERT_HTML, L"infiniteunloadalert");
}

// Tests closing the browser on a page with an unload listener registered where
// the unload handler has an 2 second long loop followed by an alert.
TEST_F(UnloadTest, BrowserCloseTwoSecondUnloadAlert) {
  LoadUrlAndQuitBrowser(TWO_SECOND_UNLOAD_ALERT_HTML, L"twosecondunloadalert");
}

// TODO(ojan): Test popping up an alert in the unload handler and test
// beforeunload. In addition add tests where we open all of these pages
// in the browser and then close it, as well as having two windows and
// closing only one of them.
