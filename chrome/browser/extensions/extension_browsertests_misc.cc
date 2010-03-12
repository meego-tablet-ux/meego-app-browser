// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/ref_counted.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/renderer_host/render_view_host.h"
#include "chrome/browser/extensions/autoupdate_interceptor.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/extension_host.h"
#include "chrome/browser/extensions/extension_process_manager.h"
#include "chrome/browser/extensions/extension_tabs_module.h"
#include "chrome/browser/extensions/extensions_service.h"
#include "chrome/browser/extensions/extension_updater.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/renderer_host/site_instance.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#if defined(TOOLKIT_VIEWS)
#include "chrome/browser/views/extensions/extension_shelf.h"
#include "chrome/browser/views/frame/browser_view.h"
#endif

#include "chrome/common/chrome_paths.h"
#include "chrome/common/extensions/extension_error_reporter.h"
#include "chrome/common/notification_service.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/ui_test_utils.h"
#include "net/base/net_util.h"

const std::string kSubscribePage = "/subscribe.html";
const std::wstring kFeedPage = L"files/feeds/feed.html";
const std::wstring kFeedPageMultiRel = L"files/feeds/feed_multi_rel.html";
const std::wstring kNoFeedPage = L"files/feeds/no_feed.html";
const std::wstring kValidFeed0 = L"files/feeds/feed_script.xml";
const std::wstring kValidFeed1 = L"files/feeds/feed1.xml";
const std::wstring kValidFeed2 = L"files/feeds/feed2.xml";
const std::wstring kValidFeed3 = L"files/feeds/feed3.xml";
const std::wstring kValidFeed4 = L"files/feeds/feed4.xml";
const std::wstring kValidFeed5 = L"files/feeds/feed5.xml";
const std::wstring kValidFeedNoLinks = L"files/feeds/feed_nolinks.xml";
const std::wstring kInvalidFeed1 = L"files/feeds/feed_invalid1.xml";
const std::wstring kInvalidFeed2 = L"files/feeds/feed_invalid2.xml";
const std::wstring kLocalization =
    L"files/extensions/browsertest/title_localized_pa/simple.html";
const std::wstring kHashPageA =
    L"files/extensions/api_test/page_action/hash_change/test_page_A.html";
const std::wstring kHashPageAHash = kHashPageA + L"#asdf";
const std::wstring kHashPageB =
    L"files/extensions/api_test/page_action/hash_change/test_page_B.html";

// Looks for an ExtensionHost whose URL has the given path component (including
// leading slash).  Also verifies that the expected number of hosts are loaded.
static ExtensionHost* FindHostWithPath(ExtensionProcessManager* manager,
                                       const std::string& path,
                                       int expected_hosts) {
  ExtensionHost* host = NULL;
  int num_hosts = 0;
  for (ExtensionProcessManager::const_iterator iter = manager->begin();
       iter != manager->end(); ++iter) {
    if ((*iter)->GetURL().path() == path) {
      EXPECT_FALSE(host);
      host = *iter;
    }
    num_hosts++;
  }
  EXPECT_EQ(expected_hosts, num_hosts);
  return host;
}

#if defined(OS_LINUX) && defined(TOOLKIT_VIEWS)
// See http://crbug.com/30151.
#define Toolstrip DISABLED_Toolstrip
#endif

// Tests that toolstrips initializes properly and can run basic extension js.
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, Toolstrip) {
  FilePath extension_test_data_dir = test_data_dir_.AppendASCII("good").
      AppendASCII("Extensions").AppendASCII("behllobkkfkfnphdnhnkndlbkcpglgmj").
      AppendASCII("1.0.0.0");
  ASSERT_TRUE(LoadExtension(extension_test_data_dir));

  // At this point, there should be three ExtensionHosts loaded because this
  // extension has two toolstrips and one background page. Find the one that is
  // hosting toolstrip1.html.
  ExtensionProcessManager* manager =
      browser()->profile()->GetExtensionProcessManager();
  ExtensionHost* host = FindHostWithPath(manager, "/toolstrip1.html", 3);

  // Tell it to run some JavaScript that tests that basic extension code works.
  bool result = false;
  ui_test_utils::ExecuteJavaScriptAndExtractBool(
      host->render_view_host(), L"", L"testTabsAPI()", &result);
  EXPECT_TRUE(result);

  // Test for compact language detection API. First navigate to a (static) html
  // file with a French sentence. Then, run the test API in toolstrip1.html to
  // actually call the language detection API through the existing extension,
  // and verify that the language returned is indeed French.
  FilePath language_url = extension_test_data_dir.AppendASCII(
      "french_sentence.html");
  ui_test_utils::NavigateToURL(browser(), net::FilePathToFileURL(language_url));

  ui_test_utils::ExecuteJavaScriptAndExtractBool(
      host->render_view_host(), L"", L"testTabsLanguageAPI()", &result);
  EXPECT_TRUE(result);
}

IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, ExtensionViews) {
  FilePath extension_test_data_dir = test_data_dir_.AppendASCII("good").
      AppendASCII("Extensions").AppendASCII("behllobkkfkfnphdnhnkndlbkcpglgmj").
      AppendASCII("1.0.0.0");
  ASSERT_TRUE(LoadExtension(extension_test_data_dir));

  // At this point, there should be three ExtensionHosts loaded because this
  // extension has two toolstrips and one background page. Find the one that is
  // hosting toolstrip1.html.
  ExtensionProcessManager* manager =
      browser()->profile()->GetExtensionProcessManager();
  ExtensionHost* host = FindHostWithPath(manager, "/toolstrip1.html", 3);

  FilePath gettabs_url = extension_test_data_dir.AppendASCII(
      "test_gettabs.html");
  ui_test_utils::NavigateToURL(
      browser(),
      GURL(gettabs_url.value()));

  bool result = false;
  ui_test_utils::ExecuteJavaScriptAndExtractBool(
      host->render_view_host(), L"", L"testgetToolstripsAPI()", &result);
  EXPECT_TRUE(result);

  result = false;
  ui_test_utils::ExecuteJavaScriptAndExtractBool(
      host->render_view_host(), L"", L"testgetBackgroundPageAPI()", &result);
  EXPECT_TRUE(result);

  ui_test_utils::NavigateToURL(
      browser(),
      GURL("chrome-extension://behllobkkfkfnphdnhnkndlbkcpglgmj/"
           "test_gettabs.html"));
  result = false;
  ui_test_utils::ExecuteJavaScriptAndExtractBool(
      host->render_view_host(), L"", L"testgetExtensionTabsAPI()", &result);
  EXPECT_TRUE(result);
}

#if defined(TOOLKIT_VIEWS)
// http://crbug.com/29897 - for other UI toolkits?

// Tests that the ExtensionShelf initializes properly, notices that
// an extension loaded and has a view available, and then sets that up
// properly.
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, Shelf) {
  // When initialized, there are no extension views and the preferred height
  // should be zero.
  BrowserView* browser_view = static_cast<BrowserView*>(browser()->window());
  ExtensionShelf* shelf = browser_view->extension_shelf();
  ASSERT_TRUE(shelf);
  EXPECT_EQ(shelf->GetChildViewCount(), 0);
  EXPECT_EQ(shelf->GetPreferredSize().height(), 0);

  ASSERT_TRUE(LoadExtension(
      test_data_dir_.AppendASCII("good").AppendASCII("Extensions")
                    .AppendASCII("behllobkkfkfnphdnhnkndlbkcpglgmj")
                    .AppendASCII("1.0.0.0")));

  // There should now be two extension views and preferred height of the view
  // should be non-zero.
  EXPECT_EQ(shelf->GetChildViewCount(), 2);
  EXPECT_NE(shelf->GetPreferredSize().height(), 0);
}
#endif  // defined(TOOLKIT_VIEWS)

// Tests that we can load extension pages into the tab area and they can call
// extension APIs.
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, TabContents) {
  ASSERT_TRUE(LoadExtension(
      test_data_dir_.AppendASCII("good").AppendASCII("Extensions")
                    .AppendASCII("behllobkkfkfnphdnhnkndlbkcpglgmj")
                    .AppendASCII("1.0.0.0")));

  ui_test_utils::NavigateToURL(
      browser(),
      GURL("chrome-extension://behllobkkfkfnphdnhnkndlbkcpglgmj/page.html"));

  bool result = false;
  ui_test_utils::ExecuteJavaScriptAndExtractBool(
      browser()->GetSelectedTabContents()->render_view_host(), L"",
      L"testTabsAPI()", &result);
  EXPECT_TRUE(result);

  // There was a bug where we would crash if we navigated to a page in the same
  // extension because no new render view was getting created, so we would not
  // do some setup.
  ui_test_utils::NavigateToURL(
      browser(),
      GURL("chrome-extension://behllobkkfkfnphdnhnkndlbkcpglgmj/page.html"));
  result = false;
  ui_test_utils::ExecuteJavaScriptAndExtractBool(
      browser()->GetSelectedTabContents()->render_view_host(), L"",
      L"testTabsAPI()", &result);
  EXPECT_TRUE(result);
}

// Tests that we can load page actions in the Omnibox.
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, PageAction) {
  HTTPTestServer* server = StartHTTPServer();

  // This page action will not show an icon, since it doesn't specify one but
  // is included here to test for a crash (http://crbug.com/25562).
  ASSERT_TRUE(LoadExtension(
      test_data_dir_.AppendASCII("browsertest")
                    .AppendASCII("crash_25562")));

  ASSERT_TRUE(LoadExtension(
      test_data_dir_.AppendASCII("subscribe_page_action")));

  ASSERT_TRUE(WaitForPageActionVisibilityChangeTo(0));

  // Navigate to the feed page.
  GURL feed_url = server->TestServerPageW(kFeedPage);
  ui_test_utils::NavigateToURL(browser(), feed_url);
  // We should now have one page action ready to go in the LocationBar.
  ASSERT_TRUE(WaitForPageActionVisibilityChangeTo(1));

  // Navigate to a page with no feed.
  GURL no_feed = server->TestServerPageW(kNoFeedPage);
  ui_test_utils::NavigateToURL(browser(), no_feed);
  // Make sure the page action goes away.
  ASSERT_TRUE(WaitForPageActionVisibilityChangeTo(0));
}

// Tests that we don't lose the page action icon on in-page navigations.
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, PageActionInPageNavigation) {
  HTTPTestServer* server = StartHTTPServer();

  FilePath extension_path(test_data_dir_.AppendASCII("api_test")
                                        .AppendASCII("page_action")
                                        .AppendASCII("hash_change"));
  ASSERT_TRUE(LoadExtension(extension_path));

  // Page action should become visible when we navigate here.
  GURL feed_url = server->TestServerPageW(kHashPageA);
  ui_test_utils::NavigateToURL(browser(), feed_url);
  ASSERT_TRUE(WaitForPageActionVisibilityChangeTo(1));

  // In-page navigation, page action should remain.
  feed_url = server->TestServerPageW(kHashPageAHash);
  ui_test_utils::NavigateToURL(browser(), feed_url);
  ASSERT_TRUE(WaitForPageActionVisibilityChangeTo(1));

  // Not an in-page navigation, page action should go away.
  feed_url = server->TestServerPageW(kHashPageB);
  ui_test_utils::NavigateToURL(browser(), feed_url);
  ASSERT_TRUE(WaitForPageActionVisibilityChangeTo(0));
}

// Tests that the location bar forgets about unloaded page actions.
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, UnloadPageAction) {
  HTTPTestServer* server = StartHTTPServer();

  FilePath extension_path(test_data_dir_.AppendASCII("subscribe_page_action"));
  ASSERT_TRUE(LoadExtension(extension_path));

  // Navigation prompts the location bar to load page actions.
  GURL feed_url = server->TestServerPageW(kFeedPage);
  ui_test_utils::NavigateToURL(browser(), feed_url);
  ASSERT_TRUE(WaitForPageActionCountChangeTo(1));

  UnloadExtension(last_loaded_extension_id_);

  // Make sure the page action goes away when it's unloaded.
  ASSERT_TRUE(WaitForPageActionCountChangeTo(0));
}

// Makes sure that the RSS detects RSS feed links, even when rel tag contains
// more than just "alternate".
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, RSSMultiRelLink) {
  HTTPTestServer* server = StartHTTPServer();

  ASSERT_TRUE(LoadExtension(
    test_data_dir_.AppendASCII("subscribe_page_action")));

  ASSERT_TRUE(WaitForPageActionVisibilityChangeTo(0));

  // Navigate to the feed page.
  GURL feed_url = server->TestServerPageW(kFeedPageMultiRel);
  ui_test_utils::NavigateToURL(browser(), feed_url);
  // We should now have one page action ready to go in the LocationBar.
  ASSERT_TRUE(WaitForPageActionVisibilityChangeTo(1));
}

// Tests that tooltips of a browser action icon can be specified using UTF8.
// See http://crbug.com/25349.
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, TitleLocalizationBrowserAction) {
  FilePath extension_path(test_data_dir_.AppendASCII("browsertest")
                                        .AppendASCII("title_localized"));
  ASSERT_TRUE(LoadExtension(extension_path));

  ExtensionsService* service = browser()->profile()->GetExtensionsService();
  const ExtensionList* extensions = service->extensions();
  ASSERT_EQ(1u, extensions->size());
  Extension* extension = extensions->at(0);

  EXPECT_STREQ(WideToUTF8(L"Hreggvi\u00F0ur: l10n browser action").c_str(),
               extension->description().c_str());
  EXPECT_STREQ(WideToUTF8(L"Hreggvi\u00F0ur is my name").c_str(),
               extension->name().c_str());
  int tab_id = ExtensionTabUtil::GetTabId(browser()->GetSelectedTabContents());
  EXPECT_STREQ(WideToUTF8(L"Hreggvi\u00F0ur").c_str(),
               extension->browser_action()->GetTitle(tab_id).c_str());
}

// Tests that tooltips of a page action icon can be specified using UTF8.
// See http://crbug.com/25349.
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, TitleLocalizationPageAction) {
  HTTPTestServer* server = StartHTTPServer();

  FilePath extension_path(test_data_dir_.AppendASCII("browsertest")
                                        .AppendASCII("title_localized_pa"));
  ASSERT_TRUE(LoadExtension(extension_path));

  // Any navigation prompts the location bar to load the page action.
  GURL url = server->TestServerPageW(kLocalization);
  ui_test_utils::NavigateToURL(browser(), url);
  ASSERT_TRUE(WaitForPageActionVisibilityChangeTo(1));

  ExtensionsService* service = browser()->profile()->GetExtensionsService();
  const ExtensionList* extensions = service->extensions();
  ASSERT_EQ(1u, extensions->size());
  Extension* extension = extensions->at(0);

  EXPECT_STREQ(WideToUTF8(L"Hreggvi\u00F0ur: l10n page action").c_str(),
               extension->description().c_str());
  EXPECT_STREQ(WideToUTF8(L"Hreggvi\u00F0ur is my name").c_str(),
               extension->name().c_str());
  int tab_id = ExtensionTabUtil::GetTabId(browser()->GetSelectedTabContents());
  EXPECT_STREQ(WideToUTF8(L"Hreggvi\u00F0ur").c_str(),
               extension->page_action()->GetTitle(tab_id).c_str());
}

GURL GetFeedUrl(HTTPTestServer* server, const std::wstring& feed_page,
                bool direct_url, std::string extension_id) {
  GURL feed_url = server->TestServerPageW(feed_page);
  if (direct_url) {
    // We navigate directly to the subscribe page for feeds where the feed
    // sniffing won't work, in other words, as is the case for malformed feeds.
    return GURL(std::string(chrome::kExtensionScheme) +
        chrome::kStandardSchemeSeparator +
        extension_id + std::string(kSubscribePage) + std::string("?") +
        feed_url.spec() + std::string("&synchronous"));
  } else {
    // Navigate to the feed content (which will cause the extension to try to
    // sniff the type and display the subscribe page in another tab.
    return GURL(feed_url.spec());
  }
}

static const wchar_t* jscript_feed_title =
    L"window.domAutomationController.send("
    L"  document.getElementById('title') ? "
    L"    document.getElementById('title').textContent : "
    L"    \"element 'title' not found\""
    L");";
static const wchar_t* jscript_anchor =
    L"window.domAutomationController.send("
    L"  document.getElementById('anchor_0') ? "
    L"    document.getElementById('anchor_0').textContent : "
    L"    \"element 'anchor_0' not found\""
    L");";
static const wchar_t* jscript_desc =
    L"window.domAutomationController.send("
    L"  document.getElementById('desc_0') ? "
    L"    document.getElementById('desc_0').textContent : "
    L"    \"element 'desc_0' not found\""
    L");";
static const wchar_t* jscript_error =
    L"window.domAutomationController.send("
    L"  document.getElementById('error') ? "
    L"    document.getElementById('error').textContent : "
    L"    \"No error\""
    L");";

bool ValidatePageElement(TabContents* tab,
                         const std::wstring& frame,
                         const std::wstring& javascript,
                         const std::string& expected_value) {
  std::string returned_value;
  std::string error;

  if (!ui_test_utils::ExecuteJavaScriptAndExtractString(
          tab->render_view_host(),
          frame,
          javascript, &returned_value))
    return false;

  EXPECT_STREQ(expected_value.c_str(), returned_value.c_str());
  return expected_value == returned_value;
}

// Navigates to a feed page and, if |sniff_xml_type| is set, wait for the
// extension to kick in, detect the feed and redirect to a feed preview page.
// |sniff_xml_type| is generally set to true if the feed is sniffable and false
// for invalid feeds.
void NavigateToFeedAndValidate(HTTPTestServer* server,
                               const std::wstring& url,
                               Browser* browser,
                               bool sniff_xml_type,
                               const std::string& expected_feed_title,
                               const std::string& expected_item_title,
                               const std::string& expected_item_desc,
                               const std::string& expected_error) {
  if (sniff_xml_type) {
    // TODO(finnur): Implement this is a non-flaky way.
  }

  // There should be only one extension in the list (ours). Get its id.
  ExtensionsService* service = browser->profile()->GetExtensionsService();
  ASSERT_EQ(1u, service->extensions()->size());
  Extension* extension = (*service->extensions())[0];
  std::string id = extension->id();

  // Navigate to the subscribe page directly.
  ui_test_utils::NavigateToURL(browser, GetFeedUrl(server, url, true, id));

  TabContents* tab = browser->GetSelectedTabContents();
  ASSERT_TRUE(ValidatePageElement(tab,
                                  L"",
                                  jscript_feed_title,
                                  expected_feed_title));
  ASSERT_TRUE(ValidatePageElement(tab,
                                  L"//html/body/div/iframe[1]",
                                  jscript_anchor,
                                  expected_item_title));
  ASSERT_TRUE(ValidatePageElement(tab,
                                  L"//html/body/div/iframe[1]",
                                  jscript_desc,
                                  expected_item_desc));
  ASSERT_TRUE(ValidatePageElement(tab,
                                  L"//html/body/div/iframe[1]",
                                  jscript_error,
                                  expected_error));
}

IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, FLAKY_ParseFeedValidFeed1) {
  HTTPTestServer* server = StartHTTPServer();

  ASSERT_TRUE(LoadExtension(
      test_data_dir_.AppendASCII("subscribe_page_action")));

  NavigateToFeedAndValidate(server, kValidFeed1, browser(), true,
                            "Feed for MyFeedTitle",
                            "Title 1",
                            "Desc",
                            "No error");
}

IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, FLAKY_ParseFeedValidFeed2) {
  HTTPTestServer* server = StartHTTPServer();

  ASSERT_TRUE(LoadExtension(
      test_data_dir_.AppendASCII("subscribe_page_action")));

  NavigateToFeedAndValidate(server, kValidFeed2, browser(), true,
                            "Feed for MyFeed2",
                            "My item title1",
                            "This is a summary.",
                            "No error");
}

IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, FLAKY_ParseFeedValidFeed3) {
  HTTPTestServer* server = StartHTTPServer();

  ASSERT_TRUE(LoadExtension(
      test_data_dir_.AppendASCII("subscribe_page_action")));

  NavigateToFeedAndValidate(server, kValidFeed3, browser(), true,
                            "Feed for Google Code buglist rss feed",
                            "My dear title",
                            "My dear content",
                            "No error");
}

IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, FLAKY_ParseFeedValidFeed4) {
  HTTPTestServer* server = StartHTTPServer();

  ASSERT_TRUE(LoadExtension(
      test_data_dir_.AppendASCII("subscribe_page_action")));

  NavigateToFeedAndValidate(server, kValidFeed4, browser(), true,
                            "Feed for Title chars <script> %23 stop",
                            "Title chars ",
                            "My dear content",
                            "No error");
}

IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, FLAKY_ParseFeedValidFeed0) {
  HTTPTestServer* server = StartHTTPServer();

  ASSERT_TRUE(LoadExtension(
      test_data_dir_.AppendASCII("subscribe_page_action")));

  // Try a feed with a link with an onclick handler (before r27440 this would
  // trigger a NOTREACHED).
  NavigateToFeedAndValidate(server, kValidFeed0, browser(), true,
                            "Feed for MyFeedTitle",
                            "Title 1",
                            "Desc VIDEO",
                            "No error");
}

IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, FLAKY_ParseFeedValidFeed5) {
  HTTPTestServer* server = StartHTTPServer();

  ASSERT_TRUE(LoadExtension(
      test_data_dir_.AppendASCII("subscribe_page_action")));

  // Feed with valid but mostly empty xml.
  NavigateToFeedAndValidate(server, kValidFeed5, browser(), true,
                            "Feed for Unknown feed name",
                            "element 'anchor_0' not found",
                            "element 'desc_0' not found",
                            "This feed contains no entries.");
}

IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, FLAKY_ParseFeedInvalidFeed1) {
  HTTPTestServer* server = StartHTTPServer();

  ASSERT_TRUE(LoadExtension(
      test_data_dir_.AppendASCII("subscribe_page_action")));

  // Try an empty feed.
  NavigateToFeedAndValidate(server, kInvalidFeed1, browser(), false,
                            "Feed for Unknown feed name",
                            "element 'anchor_0' not found",
                            "element 'desc_0' not found",
                            "Not a valid feed.");
}

IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, FLAKY_ParseFeedInvalidFeed2) {
  HTTPTestServer* server = StartHTTPServer();

  ASSERT_TRUE(LoadExtension(
      test_data_dir_.AppendASCII("subscribe_page_action")));

  // Try a garbage feed.
  NavigateToFeedAndValidate(server, kInvalidFeed2, browser(), false,
                            "Feed for Unknown feed name",
                            "element 'anchor_0' not found",
                            "element 'desc_0' not found",
                            "Not a valid feed.");
}

IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, FLAKY_ParseFeedInvalidFeed3) {
  HTTPTestServer* server = StartHTTPServer();

  ASSERT_TRUE(LoadExtension(
      test_data_dir_.AppendASCII("subscribe_page_action")));

  // Try a feed that doesn't exist.
  NavigateToFeedAndValidate(server, L"foo.xml", browser(), false,
                            "Feed for Unknown feed name",
                            "element 'anchor_0' not found",
                            "element 'desc_0' not found",
                            "Not a valid feed.");
}

IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, FLAKY_ParseFeedValidFeedNoLinks) {
  HTTPTestServer* server = StartHTTPServer();

  ASSERT_TRUE(LoadExtension(
      test_data_dir_.AppendASCII("subscribe_page_action")));

  // Valid feed but containing no links.
  NavigateToFeedAndValidate(server, kValidFeedNoLinks, browser(), true,
                            "Feed for MyFeedTitle",
                            "Title with no link",
                            "Desc",
                            "No error");
}

// Tests that an error raised during an async function still fires
// the callback, but sets chrome.extension.lastError.
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, LastError) {
  ASSERT_TRUE(LoadExtension(
      test_data_dir_.AppendASCII("browsertest").AppendASCII("last_error")));

  // Get the ExtensionHost that is hosting our toolstrip page.
  ExtensionProcessManager* manager =
      browser()->profile()->GetExtensionProcessManager();
  ExtensionHost* host = FindHostWithPath(manager, "/toolstrip.html", 1);

  bool result = false;
  ui_test_utils::ExecuteJavaScriptAndExtractBool(
      host->render_view_host(), L"", L"testLastError()", &result);
  EXPECT_TRUE(result);
}

// Helper function for common code shared by the 3 WindowOpen tests below.
static TabContents* WindowOpenHelper(Browser* browser, const GURL& start_url,
                                    const std::string& newtab_url) {
  ui_test_utils::NavigateToURL(browser, start_url);

  bool result = false;
  ui_test_utils::ExecuteJavaScriptAndExtractBool(
      browser->GetSelectedTabContents()->render_view_host(), L"",
      L"window.open('" + UTF8ToWide(newtab_url) + L"');"
      L"window.domAutomationController.send(true);", &result);
  EXPECT_TRUE(result);

  // Now the current tab should be the new tab.
  TabContents* newtab = browser->GetSelectedTabContents();
  GURL expected_url = start_url.Resolve(newtab_url);
  if (newtab->GetURL() != expected_url) {
    ui_test_utils::WaitForNavigation(
        &browser->GetSelectedTabContents()->controller());
  }
  EXPECT_EQ(newtab->GetURL(), expected_url);

  return newtab;
}

// Tests that an extension page can call window.open to an extension URL and
// the new window has extension privileges.
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, WindowOpenExtension) {
  ASSERT_TRUE(LoadExtension(
      test_data_dir_.AppendASCII("uitest").AppendASCII("window_open")));

  TabContents* newtab = WindowOpenHelper(
      browser(),
      GURL(std::string("chrome-extension://") + last_loaded_extension_id_ +
           "/test.html"),
      "newtab.html");

  bool result = false;
  ui_test_utils::ExecuteJavaScriptAndExtractBool(
      newtab->render_view_host(), L"", L"testExtensionApi()", &result);
  EXPECT_TRUE(result);
}

// Tests that if an extension page calls window.open to an invalid extension
// URL, the browser doesn't crash.
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, WindowOpenInvalidExtension) {
  ASSERT_TRUE(LoadExtension(
      test_data_dir_.AppendASCII("uitest").AppendASCII("window_open")));

  WindowOpenHelper(
      browser(),
      GURL(std::string("chrome-extension://") + last_loaded_extension_id_ +
           "/test.html"),
      "chrome-extension://thisissurelynotavalidextensionid/newtab.html");

  // If we got to this point, we didn't crash, so we're good.
}

// Tests that calling window.open from the newtab page to an extension URL
// does not give the new window extension privileges - because the opening page
// does not have extension privileges.
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, WindowOpenNoPrivileges) {
  ASSERT_TRUE(LoadExtension(
      test_data_dir_.AppendASCII("uitest").AppendASCII("window_open")));

  TabContents* newtab = WindowOpenHelper(
      browser(),
      GURL("about:blank"),
      std::string("chrome-extension://") + last_loaded_extension_id_ +
          "/newtab.html");

  // Extension API should fail.
  bool result = false;
  ui_test_utils::ExecuteJavaScriptAndExtractBool(
      newtab->render_view_host(), L"", L"testExtensionApi()", &result);
  EXPECT_FALSE(result);
}

#if !defined(OS_WIN)
// TODO(mpcomplete): http://crbug.com/29900 need cross platform plugin support.
#define MAYBE_PluginLoadUnload DISABLED_PluginLoadUnload
#else
#define MAYBE_PluginLoadUnload PluginLoadUnload
#endif

// Tests that a renderer's plugin list is properly updated when we load and
// unload an extension that contains a plugin.
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, MAYBE_PluginLoadUnload) {
  FilePath extension_dir =
      test_data_dir_.AppendASCII("uitest").AppendASCII("plugins");

  ui_test_utils::NavigateToURL(browser(),
      net::FilePathToFileURL(extension_dir.AppendASCII("test.html")));
  TabContents* tab = browser()->GetSelectedTabContents();

  // With no extensions, the plugin should not be loaded.
  bool result = false;
  ui_test_utils::ExecuteJavaScriptAndExtractBool(
      tab->render_view_host(), L"", L"testPluginWorks()", &result);
  EXPECT_FALSE(result);

  ExtensionsService* service = browser()->profile()->GetExtensionsService();
  ASSERT_TRUE(LoadExtension(extension_dir));
  EXPECT_EQ(1u, service->extensions()->size());
  // Now the plugin should be in the cache, but we have to reload the page for
  // it to work.
  ui_test_utils::ExecuteJavaScriptAndExtractBool(
      tab->render_view_host(), L"", L"testPluginWorks()", &result);
  EXPECT_FALSE(result);
  browser()->Reload();
  ui_test_utils::WaitForNavigationInCurrentTab(browser());
  ui_test_utils::ExecuteJavaScriptAndExtractBool(
      tab->render_view_host(), L"", L"testPluginWorks()", &result);
  EXPECT_TRUE(result);

  EXPECT_EQ(1u, service->extensions()->size());
  UnloadExtension(service->extensions()->at(0)->id());
  EXPECT_EQ(0u, service->extensions()->size());

  // Now the plugin should be out of the cache again, but existing pages will
  // still work until we reload them.

  ui_test_utils::ExecuteJavaScriptAndExtractBool(
      tab->render_view_host(), L"", L"testPluginWorks()", &result);
  EXPECT_TRUE(result);
  browser()->Reload();
  ui_test_utils::WaitForNavigationInCurrentTab(browser());

  ui_test_utils::ExecuteJavaScriptAndExtractBool(
      tab->render_view_host(), L"", L"testPluginWorks()", &result);
  EXPECT_FALSE(result);
}

// Used to simulate a click on the first button named 'Options'.
static const wchar_t* jscript_click_option_button =
    L"(function() { "
    L"  var button = document.evaluate(\"//button[text()='Options']\","
    L"      document, null, XPathResult.UNORDERED_NODE_SNAPSHOT_TYPE,"
    L"      null).snapshotItem(0);"
    L"  button.click();"
    L"  window.domAutomationController.send(0);"
    L"})();";

// Test that an extension with an options page makes an 'Options' button appear
// on chrome://extensions, and that clicking the button opens a new tab with the
// extension's options page.
// Disabled.  See http://crbug.com/26948 for details.
IN_PROC_BROWSER_TEST_F(ExtensionBrowserTest, DISABLED_OptionsPage) {
  // Install an extension with an options page.
  ASSERT_TRUE(InstallExtension(test_data_dir_.AppendASCII("options.crx"), 1));
  ExtensionsService* service = browser()->profile()->GetExtensionsService();
  const ExtensionList* extensions = service->extensions();
  ASSERT_EQ(1u, extensions->size());
  Extension* extension = extensions->at(0);

  // Go to the chrome://extensions page and click the Options button.
  ui_test_utils::NavigateToURL(browser(), GURL(chrome::kChromeUIExtensionsURL));
  TabStripModel* tab_strip = browser()->tabstrip_model();
  TabContents* extensions_tab = browser()->GetSelectedTabContents();
  ui_test_utils::ExecuteJavaScript(extensions_tab->render_view_host(), L"",
                                   jscript_click_option_button);

  // If the options page hasn't already come up, wait for it.
  if (tab_strip->count() == 1) {
    ui_test_utils::WaitForNewTab(browser());
  }
  ASSERT_EQ(2, tab_strip->count());

  EXPECT_EQ(extension->GetResourceURL("options.html"),
            tab_strip->GetTabContentsAt(1)->GetURL());
}
