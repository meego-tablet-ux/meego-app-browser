// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/time.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/pref_service.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/tab_contents/interstitial_page.h"
#include "chrome/browser/tab_contents/navigation_entry.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/common/pref_names.h"
#include "chrome/test/in_process_browser_test.h"
#include "chrome/test/ui_test_utils.h"
#include "net/base/cert_status_flags.h"
#include "net/test/test_server.h"

const wchar_t kDocRoot[] = L"chrome/test/data";

class SSLUITest : public InProcessBrowserTest {
 public:
  SSLUITest() {
    EnableDOMAutomation();
  }

  scoped_refptr<net::HTTPTestServer> PlainServer() {
    return net::HTTPTestServer::CreateServer(kDocRoot);
  }

  scoped_refptr<net::HTTPSTestServer> GoodCertServer() {
    return net::HTTPSTestServer::CreateGoodServer(kDocRoot);
  }

  scoped_refptr<net::HTTPSTestServer> BadCertServer() {
    return net::HTTPSTestServer::CreateExpiredServer(kDocRoot);
  }

  void CheckAuthenticatedState(TabContents* tab,
                               bool displayed_insecure_content) {
    NavigationEntry* entry = tab->controller().GetActiveEntry();
    ASSERT_TRUE(entry);
    EXPECT_EQ(NavigationEntry::NORMAL_PAGE, entry->page_type());
    EXPECT_EQ(SECURITY_STYLE_AUTHENTICATED, entry->ssl().security_style());
    EXPECT_EQ(0, entry->ssl().cert_status() & net::CERT_STATUS_ALL_ERRORS);
    EXPECT_EQ(displayed_insecure_content,
              entry->ssl().displayed_insecure_content());
    EXPECT_FALSE(entry->ssl().ran_insecure_content());
  }

  void CheckUnauthenticatedState(TabContents* tab) {
    NavigationEntry* entry = tab->controller().GetActiveEntry();
    ASSERT_TRUE(entry);
    EXPECT_EQ(NavigationEntry::NORMAL_PAGE, entry->page_type());
    EXPECT_EQ(SECURITY_STYLE_UNAUTHENTICATED, entry->ssl().security_style());
    EXPECT_EQ(0, entry->ssl().cert_status() & net::CERT_STATUS_ALL_ERRORS);
    EXPECT_FALSE(entry->ssl().displayed_insecure_content());
    EXPECT_FALSE(entry->ssl().ran_insecure_content());
  }

  void CheckAuthenticationBrokenState(TabContents* tab,
                                      int error,
                                      bool ran_insecure_content,
                                      bool interstitial) {
    NavigationEntry* entry = tab->controller().GetActiveEntry();
    ASSERT_TRUE(entry);
    EXPECT_EQ(interstitial ? NavigationEntry::INTERSTITIAL_PAGE :
                             NavigationEntry::NORMAL_PAGE,
              entry->page_type());
    EXPECT_EQ(SECURITY_STYLE_AUTHENTICATION_BROKEN,
              entry->ssl().security_style());
    // CERT_STATUS_UNABLE_TO_CHECK_REVOCATION doesn't lower the security style
    // to SECURITY_STYLE_AUTHENTICATION_BROKEN.
    ASSERT_NE(net::CERT_STATUS_UNABLE_TO_CHECK_REVOCATION, error);
    EXPECT_EQ(error, entry->ssl().cert_status() & net::CERT_STATUS_ALL_ERRORS);
    EXPECT_FALSE(entry->ssl().displayed_insecure_content());
    EXPECT_EQ(ran_insecure_content, entry->ssl().ran_insecure_content());
  }

  void CheckWorkerLoadResult(TabContents* tab, bool expectLoaded) {
    // Workers are async and we don't have notifications for them passing
    // messages since they do it between renderer and worker processes.
    // So have a polling loop, check every 200ms, timeout at 30s.
    const int timeout_ms = 200;
    base::Time timeToQuit = base::Time::Now() +
        base::TimeDelta::FromMilliseconds(30000);

    while (base::Time::Now() < timeToQuit) {
      bool workerFinished = false;
      ASSERT_TRUE(ui_test_utils::ExecuteJavaScriptAndExtractBool(
          tab->render_view_host(), std::wstring(),
          L"window.domAutomationController.send(IsWorkerFinished());",
          &workerFinished));

      if (workerFinished)
        break;

      // Wait a bit.
      MessageLoop::current()->PostDelayedTask(
          FROM_HERE, new MessageLoop::QuitTask, timeout_ms);
      ui_test_utils::RunMessageLoop();
    }

    bool actuallyLoadedContent = false;
    ASSERT_TRUE(ui_test_utils::ExecuteJavaScriptAndExtractBool(
        tab->render_view_host(), std::wstring(),
        L"window.domAutomationController.send(IsContentLoaded());",
        &actuallyLoadedContent));
    EXPECT_EQ(expectLoaded, actuallyLoadedContent);
  }

  void ProceedThroughInterstitial(TabContents* tab) {
    InterstitialPage* interstitial_page = tab->interstitial_page();
    ASSERT_TRUE(interstitial_page);
    interstitial_page->Proceed();
    // Wait for the navigation to be done.
    ui_test_utils::WaitForNavigation(&(tab->controller()));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SSLUITest);
};

// Visits a regular page over http.
IN_PROC_BROWSER_TEST_F(SSLUITest, TestHTTP) {
  scoped_refptr<net::HTTPTestServer> server = PlainServer();
  ASSERT_TRUE(server.get() != NULL);

  ui_test_utils::NavigateToURL(browser(),
                               server->TestServerPage("files/ssl/google.html"));

  CheckUnauthenticatedState(browser()->GetSelectedTabContents());
}

// Visits a page over http which includes broken https resources (status should
// be OK).
// TODO(jcampan): test that bad HTTPS content is blocked (otherwise we'll give
//                the secure cookies away!).
IN_PROC_BROWSER_TEST_F(SSLUITest, TestHTTPWithBrokenHTTPSResource) {
  scoped_refptr<net::HTTPTestServer> http_server = PlainServer();
  ASSERT_TRUE(http_server.get() != NULL);
  scoped_refptr<net::HTTPSTestServer> bad_https_server = BadCertServer();
  ASSERT_TRUE(bad_https_server.get() != NULL);

  ui_test_utils::NavigateToURL(browser(),
      http_server->TestServerPage("files/ssl/page_with_unsafe_contents.html"));

  CheckUnauthenticatedState(browser()->GetSelectedTabContents());
}

// Visits a page over OK https:
IN_PROC_BROWSER_TEST_F(SSLUITest, TestOKHTTPS) {
  scoped_refptr<net::HTTPSTestServer> https_server = GoodCertServer();
  ASSERT_TRUE(https_server.get() != NULL);

  ui_test_utils::NavigateToURL(browser(),
      https_server->TestServerPage("files/ssl/google.html"));

  CheckAuthenticatedState(browser()->GetSelectedTabContents(), false);
}

// Visits a page with https error and proceed:
IN_PROC_BROWSER_TEST_F(SSLUITest, TestHTTPSExpiredCertAndProceed) {
  scoped_refptr<net::HTTPSTestServer> bad_https_server = BadCertServer();
  ASSERT_TRUE(bad_https_server.get() != NULL);

  ui_test_utils::NavigateToURL(browser(),
      bad_https_server->TestServerPage("files/ssl/google.html"));

  TabContents* tab = browser()->GetSelectedTabContents();
  CheckAuthenticationBrokenState(tab, net::CERT_STATUS_DATE_INVALID, false,
                                 true);  // Interstitial showing

  ProceedThroughInterstitial(tab);

  CheckAuthenticationBrokenState(tab, net::CERT_STATUS_DATE_INVALID, false,
                                 false);  // No interstitial showing
}

// Visits a page with https error and don't proceed (and ensure we can still
// navigate at that point):
#if defined(OS_WIN)
// Disabled, flakily exceeds test timeout, http://crbug.com/43575.
#define MAYBE_TestHTTPSExpiredCertAndDontProceed \
    DISABLED_TestHTTPSExpiredCertAndDontProceed
#else
// Marked as flaky, see bug 40932.
#define MAYBE_TestHTTPSExpiredCertAndDontProceed \
    FLAKY_TestHTTPSExpiredCertAndDontProceed
#endif
IN_PROC_BROWSER_TEST_F(SSLUITest, MAYBE_TestHTTPSExpiredCertAndDontProceed) {
  scoped_refptr<net::HTTPTestServer> http_server = PlainServer();
  ASSERT_TRUE(http_server.get() != NULL);
  scoped_refptr<net::HTTPSTestServer> good_https_server = GoodCertServer();
  ASSERT_TRUE(good_https_server.get() != NULL);
  scoped_refptr<net::HTTPSTestServer> bad_https_server = BadCertServer();
  ASSERT_TRUE(bad_https_server.get() != NULL);

  // First navigate to an OK page.
  ui_test_utils::NavigateToURL(browser(),
      good_https_server->TestServerPage("files/ssl/google.html"));

  TabContents* tab = browser()->GetSelectedTabContents();
  NavigationEntry* entry = tab->controller().GetActiveEntry();
  ASSERT_TRUE(entry);

  GURL cross_site_url =
      bad_https_server->TestServerPage("files/ssl/google.html");
  // Change the host name from 127.0.0.1 to localhost so it triggers a
  // cross-site navigation so we can test http://crbug.com/5800 is gone.
  ASSERT_EQ("127.0.0.1", cross_site_url.host());
  GURL::Replacements replacements;
  std::string new_host("localhost");
  replacements.SetHostStr(new_host);
  cross_site_url = cross_site_url.ReplaceComponents(replacements);

  // Now go to a bad HTTPS page.
  ui_test_utils::NavigateToURL(browser(), cross_site_url);

  // An interstitial should be showing.
  CheckAuthenticationBrokenState(tab, net::CERT_STATUS_COMMON_NAME_INVALID,
                                 false, true);

  // Simulate user clicking "Take me back".
  InterstitialPage* interstitial_page = tab->interstitial_page();
  ASSERT_TRUE(interstitial_page);
  interstitial_page->DontProceed();

  // We should be back to the original good page.
  CheckAuthenticatedState(tab, false);

  // Try to navigate to a new page. (to make sure bug 5800 is fixed).
  ui_test_utils::NavigateToURL(browser(),
      http_server->TestServerPage("files/ssl/google.html"));
  CheckUnauthenticatedState(tab);
}

// Visits a page with https error and then goes back using Browser::GoBack.
IN_PROC_BROWSER_TEST_F(SSLUITest, TestHTTPSExpiredCertAndGoBackViaButton) {
  scoped_refptr<net::HTTPTestServer> http_server = PlainServer();
  ASSERT_TRUE(http_server.get() != NULL);
  scoped_refptr<net::HTTPSTestServer> bad_https_server = BadCertServer();
  ASSERT_TRUE(bad_https_server.get() != NULL);

  // First navigate to an HTTP page.
  ui_test_utils::NavigateToURL(browser(),
      http_server->TestServerPage("files/ssl/google.html"));
  TabContents* tab = browser()->GetSelectedTabContents();
  NavigationEntry* entry = tab->controller().GetActiveEntry();
  ASSERT_TRUE(entry);

  // Now go to a bad HTTPS page that shows an interstitial.
  ui_test_utils::NavigateToURL(browser(),
      bad_https_server->TestServerPage("files/ssl/google.html"));
  CheckAuthenticationBrokenState(tab, net::CERT_STATUS_DATE_INVALID, false,
                                 true);  // Interstitial showing

  // Simulate user clicking on back button (crbug.com/39248).
  browser()->GoBack(CURRENT_TAB);

  // We should be back at the original good page.
  EXPECT_FALSE(browser()->GetSelectedTabContents()->interstitial_page());
  CheckUnauthenticatedState(tab);
}

// Visits a page with https error and then goes back using GoToOffset.
// Marked as flaky, see bug 40932.
IN_PROC_BROWSER_TEST_F(SSLUITest, FLAKY_TestHTTPSExpiredCertAndGoBackViaMenu) {
  scoped_refptr<net::HTTPTestServer> http_server = PlainServer();
  ASSERT_TRUE(http_server.get() != NULL);
  scoped_refptr<net::HTTPSTestServer> bad_https_server = BadCertServer();
  ASSERT_TRUE(bad_https_server.get() != NULL);

  // First navigate to an HTTP page.
  ui_test_utils::NavigateToURL(browser(),
      http_server->TestServerPage("files/ssl/google.html"));
  TabContents* tab = browser()->GetSelectedTabContents();
  NavigationEntry* entry = tab->controller().GetActiveEntry();
  ASSERT_TRUE(entry);

  // Now go to a bad HTTPS page that shows an interstitial.
  ui_test_utils::NavigateToURL(browser(),
      bad_https_server->TestServerPage("files/ssl/google.html"));
  CheckAuthenticationBrokenState(tab, net::CERT_STATUS_DATE_INVALID, false,
                                 true);  // Interstitial showing

  // Simulate user clicking and holding on back button (crbug.com/37215).
  tab->controller().GoToOffset(-1);

  // We should be back at the original good page.
  EXPECT_FALSE(browser()->GetSelectedTabContents()->interstitial_page());
  CheckUnauthenticatedState(tab);
}

// Visits a page with https error and then goes forward using GoToOffset.
// Marked as flaky, see bug 40932.
IN_PROC_BROWSER_TEST_F(SSLUITest, FLAKY_TestHTTPSExpiredCertAndGoForward) {
  scoped_refptr<net::HTTPTestServer> http_server = PlainServer();
  ASSERT_TRUE(http_server.get() != NULL);
  scoped_refptr<net::HTTPSTestServer> bad_https_server = BadCertServer();
  ASSERT_TRUE(bad_https_server.get() != NULL);

  // First navigate to two HTTP pages.
  ui_test_utils::NavigateToURL(browser(),
      http_server->TestServerPage("files/ssl/google.html"));
  TabContents* tab = browser()->GetSelectedTabContents();
  NavigationEntry* entry1 = tab->controller().GetActiveEntry();
  ASSERT_TRUE(entry1);
  ui_test_utils::NavigateToURL(browser(),
      http_server->TestServerPage("files/ssl/blank_page.html"));
  NavigationEntry* entry2 = tab->controller().GetActiveEntry();
  ASSERT_TRUE(entry2);

  // Now go back so that a page is in the forward history.
  tab->controller().GoBack();
  ui_test_utils::WaitForNavigation(&(tab->controller()));
  ASSERT_TRUE(tab->controller().CanGoForward());
  NavigationEntry* entry3 = tab->controller().GetActiveEntry();
  ASSERT_TRUE(entry1 == entry3);

  // Now go to a bad HTTPS page that shows an interstitial.
  ui_test_utils::NavigateToURL(browser(),
      bad_https_server->TestServerPage("files/ssl/google.html"));
  CheckAuthenticationBrokenState(tab, net::CERT_STATUS_DATE_INVALID, false,
                                 true);  // Interstitial showing

  // Simulate user clicking and holding on forward button.
  tab->controller().GoToOffset(1);
  ui_test_utils::WaitForNavigation(&(tab->controller()));

  // We should be showing the second good page.
  EXPECT_FALSE(browser()->GetSelectedTabContents()->interstitial_page());
  CheckUnauthenticatedState(tab);
  EXPECT_FALSE(tab->controller().CanGoForward());
  NavigationEntry* entry4 = tab->controller().GetActiveEntry();
  EXPECT_TRUE(entry2 == entry4);
}

// Open a page with a HTTPS error in a tab with no prior navigation (through a
// link with a blank target).  This is to test that the lack of navigation entry
// does not cause any problems (it was causing a crasher, see
// http://crbug.com/19941).
IN_PROC_BROWSER_TEST_F(SSLUITest, TestHTTPSErrorWithNoNavEntry) {
  scoped_refptr<net::HTTPTestServer> http_server = PlainServer();
  ASSERT_TRUE(http_server.get() != NULL);
  scoped_refptr<net::HTTPSTestServer> bad_https_server = BadCertServer();
  ASSERT_TRUE(bad_https_server.get() != NULL);

  // Load a page with a link that opens a new window (therefore with no history
  // and no navigation entries).
  ui_test_utils::NavigateToURL(browser(),
      http_server->TestServerPage("files/ssl/page_with_blank_target.html"));

  bool success = false;

  ui_test_utils::WindowedNotificationObserver<NavigationController>
      load_stop_signal(NotificationType::LOAD_STOP, NULL);

  // Simulate clicking the link (and therefore navigating to that new page).
  // This will causes a new tab to be created.
  EXPECT_TRUE(ui_test_utils::ExecuteJavaScriptAndExtractBool(
      browser()->GetSelectedTabContents()->render_view_host(), std::wstring(),
      L"window.domAutomationController.send(navigateInNewTab());",
      &success));
  EXPECT_TRUE(success);

  // By the time we got a response, the new tab should have been created and be
  // the selected tab.
  EXPECT_EQ(2, browser()->tab_count());
  EXPECT_EQ(1, browser()->selected_index());

  // Since the navigation was initiated by the renderer (when we clicked on the
  // link) and since the main page network request failed, we won't get a
  // navigation entry committed.  So we'll just wait for the load to stop.
  load_stop_signal.WaitFor(
      &(browser()->GetSelectedTabContents()->controller()));

  // We should have an interstitial page showing.
  ASSERT_TRUE(browser()->GetSelectedTabContents()->interstitial_page());
}

//
// Insecure content
//

// Visits a page that displays insecure content.
IN_PROC_BROWSER_TEST_F(SSLUITest, TestDisplaysInsecureContent) {
  scoped_refptr<net::HTTPSTestServer> https_server = GoodCertServer();
  ASSERT_TRUE(https_server.get() != NULL);
  scoped_refptr<net::HTTPTestServer> http_server = PlainServer();
  ASSERT_TRUE(http_server.get() != NULL);

  // Load a page that displays insecure content.
  ui_test_utils::NavigateToURL(browser(), https_server->TestServerPage(
      "files/ssl/page_displays_insecure_content.html"));

  CheckAuthenticatedState(browser()->GetSelectedTabContents(), true);
}

// Visits a page that runs insecure content and tries to suppress the insecure
// content warnings by randomizing location.hash.
// Based on http://crbug.com/8706
IN_PROC_BROWSER_TEST_F(SSLUITest, TestRunsInsecuredContentRandomizeHash) {
  scoped_refptr<net::HTTPSTestServer> https_server = GoodCertServer();
  ASSERT_TRUE(https_server.get() != NULL);
  scoped_refptr<net::HTTPTestServer> http_server = PlainServer();
  ASSERT_TRUE(http_server.get() != NULL);

  ui_test_utils::NavigateToURL(browser(), https_server->TestServerPage(
      "files/ssl/page_runs_insecure_content.html"));

  CheckAuthenticationBrokenState(browser()->GetSelectedTabContents(), 0, true,
                                 false);
}

// Visits a page with unsafe content and make sure that:
// - frames content is replaced with warning
// - images and scripts are filtered out entirely
// Marked as flaky, see bug 40932.
IN_PROC_BROWSER_TEST_F(SSLUITest, FLAKY_TestUnsafeContents) {
  scoped_refptr<net::HTTPSTestServer> good_https_server = GoodCertServer();
  ASSERT_TRUE(good_https_server.get() != NULL);
  scoped_refptr<net::HTTPSTestServer> bad_https_server = BadCertServer();
  ASSERT_TRUE(bad_https_server.get() != NULL);

  ui_test_utils::NavigateToURL(browser(), good_https_server->TestServerPage(
      "files/ssl/page_with_unsafe_contents.html"));

  TabContents* tab = browser()->GetSelectedTabContents();
  // When the bad content is filtered, the state is expected to be
  // authenticated.
  CheckAuthenticatedState(tab, false);

  // Because of cross-frame scripting restrictions, we cannot access the iframe
  // content.  So to know if the frame was loaded, we just check if a popup was
  // opened (the iframe content opens one).
  // Note: because of bug 1115868, no constrained window is opened right now.
  //       Once the bug is fixed, this will do the real check.
  EXPECT_EQ(0, static_cast<int>(tab->constrained_window_count()));

  int img_width;
  EXPECT_TRUE(ui_test_utils::ExecuteJavaScriptAndExtractInt(
      tab->render_view_host(), std::wstring(),
      L"window.domAutomationController.send(ImageWidth());", &img_width));
  // In order to check that the image was not loaded, we check its width.
  // The actual image (Google logo) is 114 pixels wide, we assume the broken
  // image is less than 100.
  EXPECT_LT(img_width, 100);

  bool js_result = false;
  EXPECT_TRUE(ui_test_utils::ExecuteJavaScriptAndExtractBool(
      tab->render_view_host(), std::wstring(),
      L"window.domAutomationController.send(IsFooSet());", &js_result));
  EXPECT_FALSE(js_result);
}

// Visits a page with insecure content loaded by JS (after the initial page
// load).
IN_PROC_BROWSER_TEST_F(SSLUITest, TestDisplaysInsecureContentLoadedFromJS) {
  scoped_refptr<net::HTTPSTestServer> https_server = GoodCertServer();
  ASSERT_TRUE(https_server.get() != NULL);
  scoped_refptr<net::HTTPTestServer> http_server = PlainServer();
  ASSERT_TRUE(http_server.get() != NULL);

  ui_test_utils::NavigateToURL(browser(), https_server->TestServerPage(
      "files/ssl/page_with_dynamic_insecure_content.html"));

  TabContents* tab = browser()->GetSelectedTabContents();
  CheckAuthenticatedState(tab, false);

  // Load the insecure image.
  bool js_result = false;
  EXPECT_TRUE(ui_test_utils::ExecuteJavaScriptAndExtractBool(
      tab->render_view_host(), std::wstring(), L"loadBadImage();", &js_result));
  EXPECT_TRUE(js_result);

  // We should now have insecure content.
  CheckAuthenticatedState(tab, true);
}

// Visits two pages from the same origin: one that displays insecure content and
// one that doesn't.  The test checks that we do not propagate the insecure
// content state from one to the other.
IN_PROC_BROWSER_TEST_F(SSLUITest, TestDisplaysInsecureContentTwoTabs) {
  scoped_refptr<net::HTTPSTestServer> https_server = GoodCertServer();
  ASSERT_TRUE(https_server.get() != NULL);
  scoped_refptr<net::HTTPTestServer> http_server = PlainServer();
  ASSERT_TRUE(http_server.get() != NULL);

  ui_test_utils::NavigateToURL(browser(),
      https_server->TestServerPage("files/ssl/blank_page.html"));

  TabContents* tab1 = browser()->GetSelectedTabContents();

  // This tab should be fine.
  CheckAuthenticatedState(tab1, false);

  // Create a new tab.
  GURL url = https_server->TestServerPage(
      "files/ssl/page_displays_insecure_content.html");
  TabContents* tab2 = browser()->AddTabWithURL(url, GURL(),
      PageTransition::TYPED, 0, TabStripModel::ADD_SELECTED,
      tab1->GetSiteInstance(), std::string(), NULL);
  ui_test_utils::WaitForNavigation(&(tab2->controller()));

  // The new tab has insecure content.
  CheckAuthenticatedState(tab2, true);

  // The original tab should not be contaminated.
  CheckAuthenticatedState(tab1, false);
}

// Visits two pages from the same origin: one that runs insecure content and one
// that doesn't.  The test checks that we propagate the insecure content state
// from one to the other.
IN_PROC_BROWSER_TEST_F(SSLUITest, TestRunsInsecureContentTwoTabs) {
  scoped_refptr<net::HTTPSTestServer> https_server = GoodCertServer();
  ASSERT_TRUE(https_server.get() != NULL);
  scoped_refptr<net::HTTPTestServer> http_server = PlainServer();
  ASSERT_TRUE(http_server.get() != NULL);

  ui_test_utils::NavigateToURL(browser(),
      https_server->TestServerPage("files/ssl/blank_page.html"));

  TabContents* tab1 = browser()->GetSelectedTabContents();

  // This tab should be fine.
  CheckAuthenticatedState(tab1, false);

  // Create a new tab.
  GURL url =
      https_server->TestServerPage("files/ssl/page_runs_insecure_content.html");
  TabContents* tab2 = browser()->AddTabWithURL(url, GURL(),
      PageTransition::TYPED, 0, TabStripModel::ADD_SELECTED,
      tab1->GetSiteInstance(), std::string(), NULL);
  ui_test_utils::WaitForNavigation(&(tab2->controller()));

  // The new tab has insecure content.
  CheckAuthenticationBrokenState(tab2, 0, true, false);

  // Which means the origin for the first tab has also been contaminated with
  // insecure content.
  CheckAuthenticationBrokenState(tab1, 0, true, false);
}

// Visits a page with an image over http.  Visits another page over https
// referencing that same image over http (hoping it is coming from the webcore
// memory cache).
IN_PROC_BROWSER_TEST_F(SSLUITest, TestDisplaysCachedInsecureContent) {
  scoped_refptr<net::HTTPSTestServer> https_server = GoodCertServer();
  ASSERT_TRUE(https_server.get() != NULL);
  scoped_refptr<net::HTTPTestServer> http_server = PlainServer();
  ASSERT_TRUE(http_server.get() != NULL);

  ui_test_utils::NavigateToURL(browser(), http_server->TestServerPage(
      "files/ssl/page_displays_insecure_content.html"));
  TabContents* tab = browser()->GetSelectedTabContents();
  CheckUnauthenticatedState(tab);

  // Load again but over SSL.  It should be marked as displaying insecure
  // content (even though the image comes from the WebCore memory cache).
  ui_test_utils::NavigateToURL(browser(), https_server->TestServerPage(
      "files/ssl/page_displays_insecure_content.html"));
  CheckAuthenticatedState(tab, true);
}

// Visits a page with script over http.  Visits another page over https
// referencing that same script over http (hoping it is coming from the webcore
// memory cache).
IN_PROC_BROWSER_TEST_F(SSLUITest, TestRunsCachedInsecureContent) {
  scoped_refptr<net::HTTPSTestServer> https_server = GoodCertServer();
  ASSERT_TRUE(https_server.get() != NULL);
  scoped_refptr<net::HTTPTestServer> http_server = PlainServer();
  ASSERT_TRUE(http_server.get() != NULL);

  ui_test_utils::NavigateToURL(browser(),
      http_server->TestServerPage("files/ssl/page_runs_insecure_content.html"));
  TabContents* tab = browser()->GetSelectedTabContents();
  CheckUnauthenticatedState(tab);

  // Load again but over SSL.  It should be marked as displaying insecure
  // content (even though the image comes from the WebCore memory cache).
  ui_test_utils::NavigateToURL(browser(), https_server->TestServerPage(
      "files/ssl/page_runs_insecure_content.html"));
  CheckAuthenticationBrokenState(tab, 0, true, false);
}

#if defined(OS_WIN)
// See http://crbug.com/47170
#define MAYBE_TestCNInvalidStickiness FLAKY_TestCNInvalidStickiness
#else
#define MAYBE_TestCNInvalidStickiness TestCNInvalidStickiness
#endif

// This test ensures the CN invalid status does not 'stick' to a certificate
// (see bug #1044942) and that it depends on the host-name.
IN_PROC_BROWSER_TEST_F(SSLUITest, MAYBE_TestCNInvalidStickiness) {
  const std::string kLocalHost = "localhost";
  scoped_refptr<net::HTTPSTestServer> https_server =
      net::HTTPSTestServer::CreateMismatchedServer(kDocRoot);
  ASSERT_TRUE(https_server.get() != NULL);

  // First we hit the server with hostname, this generates an invalid policy
  // error.
  ui_test_utils::NavigateToURL(browser(),
      https_server->TestServerPage("files/ssl/google.html"));

  // We get an interstitial page as a result.
  TabContents* tab = browser()->GetSelectedTabContents();
  CheckAuthenticationBrokenState(tab, net::CERT_STATUS_COMMON_NAME_INVALID,
                                 false, true);  // Interstitial showing.

  ProceedThroughInterstitial(tab);

  CheckAuthenticationBrokenState(tab, net::CERT_STATUS_COMMON_NAME_INVALID,
                                 false, false);  // No interstitial showing.

  // Now we try again with the right host name this time.

  // Let's change the host-name in the url.
  GURL url = https_server->TestServerPage("files/ssl/google.html");
  std::string::size_type hostname_index = url.spec().find(kLocalHost);
  ASSERT_TRUE(hostname_index != std::string::npos);  // Test sanity check.
  std::string new_url;
  new_url.append(url.spec().substr(0, hostname_index));
  new_url.append(net::TestServerLauncher::kHostName);
  new_url.append(url.spec().substr(hostname_index + kLocalHost.size()));

  ui_test_utils::NavigateToURL(browser(), GURL(new_url));

  // Security state should be OK.
  CheckAuthenticatedState(tab, false);

  // Now try again the broken one to make sure it is still broken.
  ui_test_utils::NavigateToURL(browser(),
      https_server->TestServerPage("files/ssl/google.html"));

  // Since we OKed the interstitial last time, we get right to the page.
  CheckAuthenticationBrokenState(tab, net::CERT_STATUS_COMMON_NAME_INVALID,
                                 false, false);  // No interstitial showing.
}

// Test that navigating to a #ref does not change a bad security state.
IN_PROC_BROWSER_TEST_F(SSLUITest, TestRefNavigation) {
  scoped_refptr<net::HTTPSTestServer> bad_https_server = BadCertServer();
  ASSERT_TRUE(bad_https_server.get() != NULL);

  ui_test_utils::NavigateToURL(browser(),
      bad_https_server->TestServerPage("files/ssl/page_with_refs.html"));

  TabContents* tab = browser()->GetSelectedTabContents();
  CheckAuthenticationBrokenState(tab, net::CERT_STATUS_DATE_INVALID, false,
                                 true);  // Interstitial showing.

  ProceedThroughInterstitial(tab);

  CheckAuthenticationBrokenState(tab, net::CERT_STATUS_DATE_INVALID, false,
                                 false);  // No interstitial showing.

  // Now navigate to a ref in the page, the security state should not have
  // changed.
  ui_test_utils::NavigateToURL(browser(),
      bad_https_server->TestServerPage("files/ssl/page_with_refs.html#jp"));

  CheckAuthenticationBrokenState(tab, net::CERT_STATUS_DATE_INVALID, false,
                                 false);  // No interstitial showing.
}

// Tests that closing a page that has a unsafe pop-up does not crash the
// browser (bug #1966).
// TODO(jcampan): http://crbug.com/2136 disabled because the popup is not
//                opened as it is not initiated by a user gesture.
IN_PROC_BROWSER_TEST_F(SSLUITest, DISABLED_TestCloseTabWithUnsafePopup) {
  scoped_refptr<net::HTTPTestServer> http_server = PlainServer();
  ASSERT_TRUE(http_server.get() != NULL);
  scoped_refptr<net::HTTPSTestServer> bad_https_server = BadCertServer();
  ASSERT_TRUE(bad_https_server.get() != NULL);

  ui_test_utils::NavigateToURL(browser(),
      http_server->TestServerPage("files/ssl/page_with_unsafe_popup.html"));

  TabContents* tab1 = browser()->GetSelectedTabContents();
  // It is probably overkill to add a notification for a popup-opening, let's
  // just poll.
  for (int i = 0; i < 10; i++) {
    if (static_cast<int>(tab1->constrained_window_count()) > 0)
      break;
    MessageLoop::current()->PostDelayedTask(FROM_HERE,
                                            new MessageLoop::QuitTask(), 1000);
    ui_test_utils::RunMessageLoop();
  }
  ASSERT_EQ(1, static_cast<int>(tab1->constrained_window_count()));

  // Let's add another tab to make sure the browser does not exit when we close
  // the first tab.
  GURL url = http_server->TestServerPage("files/ssl/google.html");
  Browser* browser_used = NULL;
  TabContents* tab2 = browser()->AddTabWithURL(
      url, GURL(), PageTransition::TYPED, 0, TabStripModel::ADD_SELECTED, NULL,
      std::string(), &browser_used);
  ui_test_utils::WaitForNavigation(&(tab2->controller()));

  // Ensure that the tab was created in the correct browser.
  EXPECT_EQ(browser(), browser_used);

  // Close the first tab.
  browser()->CloseTabContents(tab1);
}

// Visit a page over bad https that is a redirect to a page with good https.
// Marked as flaky, see bug 40932.
IN_PROC_BROWSER_TEST_F(SSLUITest, FLAKY_TestRedirectBadToGoodHTTPS) {
  scoped_refptr<net::HTTPSTestServer> good_https_server = GoodCertServer();
  ASSERT_TRUE(good_https_server.get() != NULL);
  scoped_refptr<net::HTTPSTestServer> bad_https_server = BadCertServer();
  ASSERT_TRUE(bad_https_server.get() != NULL);

  GURL url1 = bad_https_server->TestServerPage("server-redirect?");
  GURL url2 = good_https_server->TestServerPage("files/ssl/google.html");

  ui_test_utils::NavigateToURL(browser(), GURL(url1.spec() + url2.spec()));

  TabContents* tab = browser()->GetSelectedTabContents();

  CheckAuthenticationBrokenState(tab, net::CERT_STATUS_DATE_INVALID, false,
                                 true);  // Interstitial showing.

  ProceedThroughInterstitial(tab);

  // We have been redirected to the good page.
  CheckAuthenticatedState(tab, false);
}

// Visit a page over good https that is a redirect to a page with bad https.
// Marked as flaky, see bug 40932.
IN_PROC_BROWSER_TEST_F(SSLUITest, FLAKY_TestRedirectGoodToBadHTTPS) {
  scoped_refptr<net::HTTPSTestServer> good_https_server = GoodCertServer();
  ASSERT_TRUE(good_https_server.get() != NULL);
  scoped_refptr<net::HTTPSTestServer> bad_https_server = BadCertServer();
  ASSERT_TRUE(bad_https_server.get() != NULL);

  GURL url1 = good_https_server->TestServerPage("server-redirect?");
  GURL url2 = bad_https_server->TestServerPage("files/ssl/google.html");
  ui_test_utils::NavigateToURL(browser(), GURL(url1.spec() + url2.spec()));

  TabContents* tab = browser()->GetSelectedTabContents();
  CheckAuthenticationBrokenState(tab, net::CERT_STATUS_DATE_INVALID, false,
                                 true);  // Interstitial showing.

  ProceedThroughInterstitial(tab);

  CheckAuthenticationBrokenState(tab, net::CERT_STATUS_DATE_INVALID, false,
                                 false);  // No interstitial showing.
}

// Visit a page over http that is a redirect to a page with good HTTPS.
IN_PROC_BROWSER_TEST_F(SSLUITest, TestRedirectHTTPToGoodHTTPS) {
  scoped_refptr<net::HTTPTestServer> http_server = PlainServer();
  ASSERT_TRUE(http_server.get() != NULL);
  scoped_refptr<net::HTTPSTestServer> good_https_server = GoodCertServer();
  ASSERT_TRUE(good_https_server.get() != NULL);

  TabContents* tab = browser()->GetSelectedTabContents();

  // HTTP redirects to good HTTPS.
  GURL http_url = http_server->TestServerPage("server-redirect?");
  GURL good_https_url =
      good_https_server->TestServerPage("files/ssl/google.html");

  ui_test_utils::NavigateToURL(browser(),
                               GURL(http_url.spec() + good_https_url.spec()));
  CheckAuthenticatedState(tab, false);
}

// Visit a page over http that is a redirect to a page with bad HTTPS.
IN_PROC_BROWSER_TEST_F(SSLUITest, FLAKY_TestRedirectHTTPToBadHTTPS) {
  scoped_refptr<net::HTTPTestServer> http_server = PlainServer();
  ASSERT_TRUE(http_server.get() != NULL);
  scoped_refptr<net::HTTPSTestServer> bad_https_server = BadCertServer();
  ASSERT_TRUE(bad_https_server.get() != NULL);

  TabContents* tab = browser()->GetSelectedTabContents();

  GURL http_url = http_server->TestServerPage("server-redirect?");
  GURL bad_https_url =
      bad_https_server->TestServerPage("files/ssl/google.html");
  ui_test_utils::NavigateToURL(browser(),
                               GURL(http_url.spec() + bad_https_url.spec()));
  CheckAuthenticationBrokenState(tab, net::CERT_STATUS_DATE_INVALID, false,
                                 true);  // Interstitial showing.

  ProceedThroughInterstitial(tab);

  CheckAuthenticationBrokenState(tab, net::CERT_STATUS_DATE_INVALID, false,
                                 false);  // No interstitial showing.
}

// Visit a page over https that is a redirect to a page with http (to make sure
// we don't keep the secure state).
// Marked as flaky, see bug 40932.
IN_PROC_BROWSER_TEST_F(SSLUITest, FLAKY_TestRedirectHTTPSToHTTP) {
  scoped_refptr<net::HTTPTestServer> http_server = PlainServer();
  ASSERT_TRUE(http_server.get() != NULL);
  scoped_refptr<net::HTTPSTestServer> https_server = GoodCertServer();
  ASSERT_TRUE(https_server.get() != NULL);

  GURL https_url = https_server->TestServerPage("server-redirect?");
  GURL http_url = http_server->TestServerPage("files/ssl/google.html");

  ui_test_utils::NavigateToURL(browser(),
                               GURL(https_url.spec() + http_url.spec()));
  CheckUnauthenticatedState(browser()->GetSelectedTabContents());
}

// Visits a page to which we could not connect (bad port) over http and https
// and make sure the security style is correct.
IN_PROC_BROWSER_TEST_F(SSLUITest, TestConnectToBadPort) {
  ui_test_utils::NavigateToURL(browser(), GURL("http://localhost:17"));
  CheckUnauthenticatedState(browser()->GetSelectedTabContents());

  // Same thing over HTTPS.
  ui_test_utils::NavigateToURL(browser(), GURL("https://localhost:17"));
  CheckUnauthenticatedState(browser()->GetSelectedTabContents());
}

//
// Frame navigation
//

// From a good HTTPS top frame:
// - navigate to an OK HTTPS frame
// - navigate to a bad HTTPS (expect unsafe content and filtered frame), then
//   back
// - navigate to HTTP (expect insecure content), then back
// Disabled, http://crbug.com/18626.
IN_PROC_BROWSER_TEST_F(SSLUITest, DISABLED_TestGoodFrameNavigation) {
  scoped_refptr<net::HTTPTestServer> http_server = PlainServer();
  ASSERT_TRUE(http_server.get() != NULL);
  scoped_refptr<net::HTTPSTestServer> good_https_server = GoodCertServer();
  ASSERT_TRUE(good_https_server.get() != NULL);
  scoped_refptr<net::HTTPSTestServer> bad_https_server = BadCertServer();
  ASSERT_TRUE(bad_https_server.get() != NULL);

  TabContents* tab = browser()->GetSelectedTabContents();
  ui_test_utils::NavigateToURL(browser(),
      good_https_server->TestServerPage("files/ssl/top_frame.html"));

  CheckAuthenticatedState(tab, false);

  bool success = false;
  // Now navigate inside the frame.
  EXPECT_TRUE(ui_test_utils::ExecuteJavaScriptAndExtractBool(
      tab->render_view_host(), std::wstring(),
      L"window.domAutomationController.send(clickLink('goodHTTPSLink'));",
      &success));
  EXPECT_TRUE(success);
  ui_test_utils::WaitForNavigation(&tab->controller());

  // We should still be fine.
  CheckAuthenticatedState(tab, false);

  // Now let's hit a bad page.
  EXPECT_TRUE(ui_test_utils::ExecuteJavaScriptAndExtractBool(
      tab->render_view_host(), std::wstring(),
      L"window.domAutomationController.send(clickLink('badHTTPSLink'));",
      &success));
  EXPECT_TRUE(success);
  ui_test_utils::WaitForNavigation(&tab->controller());

  // The security style should still be secure.
  CheckAuthenticatedState(tab, false);

  // And the frame should be blocked.
  bool is_content_evil = true;
  std::wstring content_frame_xpath(L"html/frameset/frame[2]");
  std::wstring is_evil_js(L"window.domAutomationController.send("
                          L"document.getElementById('evilDiv') != null);");
  EXPECT_TRUE(ui_test_utils::ExecuteJavaScriptAndExtractBool(
      tab->render_view_host(), content_frame_xpath, is_evil_js,
      &is_content_evil));
  EXPECT_FALSE(is_content_evil);

  // Now go back, our state should still be OK.
  tab->controller().GoBack();
  ui_test_utils::WaitForNavigation(&tab->controller());
  CheckAuthenticatedState(tab, false);

  // Navigate to a page served over HTTP.
  EXPECT_TRUE(ui_test_utils::ExecuteJavaScriptAndExtractBool(
      tab->render_view_host(), std::wstring(),
      L"window.domAutomationController.send(clickLink('HTTPLink'));",
      &success));
  EXPECT_TRUE(success);
  ui_test_utils::WaitForNavigation(&tab->controller());

  // Our state should be insecure.
  CheckAuthenticatedState(tab, true);

  // Go back, our state should be unchanged.
  tab->controller().GoBack();
  ui_test_utils::WaitForNavigation(&tab->controller());
  CheckAuthenticatedState(tab, true);
}

// From a bad HTTPS top frame:
// - navigate to an OK HTTPS frame (expected to be still authentication broken).
// Marked as flaky, see bug 40932.
IN_PROC_BROWSER_TEST_F(SSLUITest, FLAKY_TestBadFrameNavigation) {
  scoped_refptr<net::HTTPSTestServer> good_https_server = GoodCertServer();
  ASSERT_TRUE(good_https_server.get() != NULL);
  scoped_refptr<net::HTTPSTestServer> bad_https_server = BadCertServer();
  ASSERT_TRUE(bad_https_server.get() != NULL);

  TabContents* tab = browser()->GetSelectedTabContents();
  ui_test_utils::NavigateToURL(browser(),
      bad_https_server->TestServerPage("files/ssl/top_frame.html"));
  CheckAuthenticationBrokenState(tab, net::CERT_STATUS_DATE_INVALID, false,
                                 true);  // Interstitial showing

  ProceedThroughInterstitial(tab);

  // Navigate to a good frame.
  bool success = false;
  EXPECT_TRUE(ui_test_utils::ExecuteJavaScriptAndExtractBool(
      tab->render_view_host(), std::wstring(),
      L"window.domAutomationController.send(clickLink('goodHTTPSLink'));",
      &success));
  EXPECT_TRUE(success);
  ui_test_utils::WaitForNavigation(&tab->controller());

  // We should still be authentication broken.
  CheckAuthenticationBrokenState(tab, net::CERT_STATUS_DATE_INVALID, false,
                                 false);
}

// From an HTTP top frame, navigate to good and bad HTTPS (security state should
// stay unauthenticated).
#if defined(OS_WIN)
// Disabled, flakily exceeds test timeout, http://crbug.com/43437.
#define MAYBE_TestUnauthenticatedFrameNavigation \
      DISABLED_TestUnauthenticatedFrameNavigation
#else
// Marked as flaky, see bug 40932.
#define MAYBE_TestUnauthenticatedFrameNavigation \
      FLAKY_TestUnauthenticatedFrameNavigation
#endif
IN_PROC_BROWSER_TEST_F(SSLUITest, MAYBE_TestUnauthenticatedFrameNavigation) {
  scoped_refptr<net::HTTPTestServer> http_server = PlainServer();
  ASSERT_TRUE(http_server.get() != NULL);
  scoped_refptr<net::HTTPSTestServer> good_https_server = GoodCertServer();
  ASSERT_TRUE(good_https_server.get() != NULL);
  scoped_refptr<net::HTTPSTestServer> bad_https_server = BadCertServer();
  ASSERT_TRUE(bad_https_server.get() != NULL);

  TabContents* tab = browser()->GetSelectedTabContents();
  ui_test_utils::NavigateToURL(browser(),
      http_server->TestServerPage("files/ssl/top_frame.html"));
  CheckUnauthenticatedState(tab);

  // Now navigate inside the frame to a secure HTTPS frame.
  bool success = false;
  EXPECT_TRUE(ui_test_utils::ExecuteJavaScriptAndExtractBool(
      tab->render_view_host(), std::wstring(),
      L"window.domAutomationController.send(clickLink('goodHTTPSLink'));",
      &success));
  EXPECT_TRUE(success);
  ui_test_utils::WaitForNavigation(&tab->controller());

  // We should still be unauthenticated.
  CheckUnauthenticatedState(tab);

  // Now navigate to a bad HTTPS frame.
  EXPECT_TRUE(ui_test_utils::ExecuteJavaScriptAndExtractBool(
      tab->render_view_host(), std::wstring(),
      L"window.domAutomationController.send(clickLink('badHTTPSLink'));",
      &success));
  EXPECT_TRUE(success);
  ui_test_utils::WaitForNavigation(&tab->controller());

  // State should not have changed.
  CheckUnauthenticatedState(tab);

  // And the frame should have been blocked (see bug #2316).
  bool is_content_evil = true;
  std::wstring content_frame_xpath(L"html/frameset/frame[2]");
  std::wstring is_evil_js(L"window.domAutomationController.send("
                          L"document.getElementById('evilDiv') != null);");
  EXPECT_TRUE(ui_test_utils::ExecuteJavaScriptAndExtractBool(
      tab->render_view_host(), content_frame_xpath, is_evil_js,
      &is_content_evil));
  EXPECT_FALSE(is_content_evil);
}

// Marked as flaky, see bug 40932.
IN_PROC_BROWSER_TEST_F(SSLUITest, FLAKY_TestUnsafeContentsInWorkerFiltered) {
  scoped_refptr<net::HTTPSTestServer> good_https_server = GoodCertServer();
  ASSERT_TRUE(good_https_server.get() != NULL);
  scoped_refptr<net::HTTPSTestServer> bad_https_server = BadCertServer();
  ASSERT_TRUE(bad_https_server.get() != NULL);

  // This page will spawn a Worker which will try to load content from
  // BadCertServer.
  ui_test_utils::NavigateToURL(browser(), good_https_server->TestServerPage(
      "files/ssl/page_with_unsafe_worker.html"));
  TabContents* tab = browser()->GetSelectedTabContents();
  // Expect Worker not to load insecure content.
  CheckWorkerLoadResult(tab, false);
  // The bad content is filtered, expect the state to be authenticated.
  CheckAuthenticatedState(tab, false);
}

// Marked as flaky, see bug 40932.
IN_PROC_BROWSER_TEST_F(SSLUITest, FLAKY_TestUnsafeContentsInWorker) {
  scoped_refptr<net::HTTPSTestServer> good_https_server = GoodCertServer();
  ASSERT_TRUE(good_https_server.get() != NULL);
  scoped_refptr<net::HTTPSTestServer> bad_https_server = BadCertServer();
  ASSERT_TRUE(bad_https_server.get() != NULL);

  // Navigate to an unsafe site. Proceed with interstitial page to indicate
  // the user approves the bad certificate.
  ui_test_utils::NavigateToURL(browser(),
      bad_https_server->TestServerPage("files/ssl/blank_page.html"));
  TabContents* tab = browser()->GetSelectedTabContents();
  CheckAuthenticationBrokenState(tab, net::CERT_STATUS_DATE_INVALID, false,
                                 true);  // Interstitial showing
  ProceedThroughInterstitial(tab);
  CheckAuthenticationBrokenState(tab, net::CERT_STATUS_DATE_INVALID, false,
                                 false);  // No Interstitial

  // Navigate to safe page that has Worker loading unsafe content.
  // Expect content to load but be marked as auth broken due to running insecure
  // content.
  ui_test_utils::NavigateToURL(browser(), good_https_server->TestServerPage(
      "files/ssl/page_with_unsafe_worker.html"));
  CheckWorkerLoadResult(tab, true);  // Worker loads insecure content
  CheckAuthenticationBrokenState(tab, 0, true, false);
}

// TODO(jcampan): more tests to do below.

// Visit a page over https that contains a frame with a redirect.

// XMLHttpRequest insecure content in synchronous mode.

// XMLHttpRequest insecure content in asynchronous mode.

// XMLHttpRequest over bad ssl in synchronous mode.

// XMLHttpRequest over OK ssl in synchronous mode.
