// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/command_line.h"
#include "chrome/browser/automation/ui_controls.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/debugger/devtools_client_host.h"
#include "chrome/browser/debugger/devtools_manager.h"
#include "chrome/browser/debugger/devtools_window.h"
#include "chrome/browser/renderer_host/render_view_host.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/notification_registrar.h"
#include "chrome/common/notification_service.h"
#include "chrome/test/in_process_browser_test.h"
#include "chrome/test/ui_test_utils.h"

namespace {

// Used to block until a dev tools client window's browser is closed.
class BrowserClosedObserver : public NotificationObserver {
 public:
  BrowserClosedObserver(Browser* browser) {
    registrar_.Add(this, NotificationType::BROWSER_CLOSED,
                   Source<Browser>(browser));
    ui_test_utils::RunMessageLoop();
  }

  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details) {
    MessageLoopForUI::current()->Quit();
  }

 private:
  NotificationRegistrar registrar_;
  DISALLOW_COPY_AND_ASSIGN(BrowserClosedObserver);
};

// The delay waited in some cases where we don't have a notifications for an
// action we take.
const int kActionDelayMs = 500;

const wchar_t kSimplePage[] = L"files/devtools/simple_page.html";

class DevToolsSanityTest : public InProcessBrowserTest {
 public:
  DevToolsSanityTest() {
    set_show_window(true);
    EnableDOMAutomation();
  }

 protected:
  void RunTest(const std::string& test_name) {
    OpenDevToolsWindow();
    std::string result;
    ASSERT_TRUE(
        ui_test_utils::ExecuteJavaScriptAndExtractString(
            client_contents_,
            L"",
            UTF8ToWide(StringPrintf("uiTests.runTest('%s')", test_name.c_str())),
            &result));
    EXPECT_EQ("[OK]", result);
    CloseDevToolsWindow();
  }

  void OpenDevToolsWindow() {
    HTTPTestServer* server = StartHTTPServer();
    GURL url = server->TestServerPageW(kSimplePage);
    ui_test_utils::NavigateToURL(browser(), url);

    TabContents* tab = browser()->GetTabContentsAt(0);
    inspected_rvh_ = tab->render_view_host();
    DevToolsManager* devtools_manager = g_browser_process->devtools_manager();
    devtools_manager->OpenDevToolsWindow(inspected_rvh_);

    DevToolsClientHost* client_host =
        devtools_manager->GetDevToolsClientHostFor(inspected_rvh_);
    window_ = client_host->AsDevToolsWindow();
    RenderViewHost* client_rvh = window_->GetRenderViewHost();
    client_contents_ = client_rvh->delegate()->GetAsTabContents();
    ui_test_utils::WaitForNavigation(&client_contents_->controller());
  }

  void CloseDevToolsWindow() {
    DevToolsManager* devtools_manager = g_browser_process->devtools_manager();
    devtools_manager->UnregisterDevToolsClientHostFor(inspected_rvh_);
    BrowserClosedObserver close_observer(window_->browser());
  }

  TabContents* client_contents_;
  DevToolsWindow* window_;
  RenderViewHost* inspected_rvh_;
};

// WebInspector opens.
IN_PROC_BROWSER_TEST_F(DevToolsSanityTest, TestHostIsPresent) {
  RunTest("testHostIsPresent");
}

// Tests elements panel basics.
IN_PROC_BROWSER_TEST_F(DevToolsSanityTest, TestElementsTreeRoot) {
  RunTest("testElementsTreeRoot");
}

// Tests resources panel basics.
IN_PROC_BROWSER_TEST_F(DevToolsSanityTest, TestMainResource) {
  RunTest("testMainResource");
}

}  // namespace
