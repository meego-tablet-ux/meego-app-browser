// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browser.h"
#include "chrome/browser/browser_window.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_browser_event_router.h"
#include "chrome/browser/extensions/extension_tabs_module.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/location_bar.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/extensions/extension_action.h"
#include "chrome/test/ui_test_utils.h"

IN_PROC_BROWSER_TEST_F(ExtensionApiTest, PageAction) {
  StartHTTPServer();
  ASSERT_TRUE(RunExtensionTest("page_action/basics")) << message_;
  Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;
  {
    // Tell the extension to update the page action state.
    ResultCatcher catcher;
    ui_test_utils::NavigateToURL(browser(),
        GURL(extension->GetResourceURL("update.html")));
    ASSERT_TRUE(catcher.GetNextResult());
  }

  // Test that we received the changes.
  int tab_id =
      browser()->GetSelectedTabContents()->controller().session_id().id();
  ExtensionAction* action = extension->page_action();
  ASSERT_TRUE(action);
  EXPECT_EQ("Modified", action->GetTitle(tab_id));

  {
    // Simulate the page action being clicked.
    ResultCatcher catcher;
    int tab_id =
        ExtensionTabUtil::GetTabId(browser()->GetSelectedTabContents());
    ExtensionBrowserEventRouter::GetInstance()->PageActionExecuted(
        browser()->profile(), extension->id(), "", tab_id, "", 0);
    EXPECT_TRUE(catcher.GetNextResult());
  }

  {
    // Tell the extension to update the page action state again.
    ResultCatcher catcher;
    ui_test_utils::NavigateToURL(browser(),
        GURL(extension->GetResourceURL("update2.html")));
    ASSERT_TRUE(catcher.GetNextResult());
  }

  // Test that we received the changes.
  tab_id = browser()->GetSelectedTabContents()->controller().session_id().id();
  EXPECT_FALSE(action->GetIcon(tab_id).isNull());
}

// Test that calling chrome.pageAction.setPopup() can enable a popup.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, PageActionAddPopup) {
  // Load the extension, which has no default popup.
  ASSERT_TRUE(RunExtensionTest("page_action/add_popup")) << message_;
  Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  int tab_id = ExtensionTabUtil::GetTabId(browser()->GetSelectedTabContents());

  ExtensionAction* page_action = extension->page_action();
  ASSERT_TRUE(page_action)
      << "Page action test extension should have a page action.";

  ASSERT_FALSE(page_action->HasPopup(tab_id));

  // Simulate the page action being clicked.  The resulting event should
  // install a page action popup.
  {
    ResultCatcher catcher;
    ExtensionBrowserEventRouter::GetInstance()->PageActionExecuted(
        browser()->profile(), extension->id(), "action", tab_id, "", 1);
    ASSERT_TRUE(catcher.GetNextResult());
  }

  ASSERT_TRUE(page_action->HasPopup(tab_id))
      << "Clicking on the page action should have caused a popup to be added.";

  ASSERT_STREQ("/a_popup.html",
               page_action->GetPopupUrl(tab_id).path().c_str());

  // Now change the popup from a_popup.html to a_second_popup.html .
  // Load a page which removes the popup using chrome.pageAction.setPopup().
  {
    ResultCatcher catcher;
    ui_test_utils::NavigateToURL(
        browser(),
        GURL(extension->GetResourceURL("change_popup.html")));
    ASSERT_TRUE(catcher.GetNextResult());
  }

  ASSERT_TRUE(page_action->HasPopup(tab_id));
  ASSERT_STREQ("/another_popup.html",
               page_action->GetPopupUrl(tab_id).path().c_str());
}

// Test that calling chrome.pageAction.setPopup() can remove a popup.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, PageActionRemovePopup) {
  // Load the extension, which has a page action with a default popup.
  ASSERT_TRUE(RunExtensionTest("page_action/remove_popup")) << message_;
  Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  int tab_id = ExtensionTabUtil::GetTabId(browser()->GetSelectedTabContents());

  ExtensionAction* page_action = extension->page_action();
  ASSERT_TRUE(page_action)
      << "Page action test extension should have a page action.";

  ASSERT_TRUE(page_action->HasPopup(tab_id))
      << "Expect a page action popup before the test removes it.";

  // Load a page which removes the popup using chrome.pageAction.setPopup().
  {
    ResultCatcher catcher;
    ui_test_utils::NavigateToURL(
        browser(),
        GURL(extension->GetResourceURL("remove_popup.html")));
    ASSERT_TRUE(catcher.GetNextResult());
  }

  ASSERT_FALSE(page_action->HasPopup(tab_id))
      << "Page action popup should have been removed.";
}

// Tests old-style pageActions API that is deprecated but we don't want to
// break.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, OldPageActions) {
  ASSERT_TRUE(RunExtensionTest("page_action/old_api")) << message_;
  Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  // Have the extension enable the page action.
  {
    ResultCatcher catcher;
    ui_test_utils::NavigateToURL(browser(),
        GURL(extension->GetResourceURL("page.html")));
    ASSERT_TRUE(catcher.GetNextResult());
  }

  // Simulate the page action being clicked.
  {
    ResultCatcher catcher;
    int tab_id =
        ExtensionTabUtil::GetTabId(browser()->GetSelectedTabContents());
    ExtensionBrowserEventRouter::GetInstance()->PageActionExecuted(
        browser()->profile(), extension->id(), "action", tab_id, "", 1);
    EXPECT_TRUE(catcher.GetNextResult());
  }
}

// Tests popups in page actions.
IN_PROC_BROWSER_TEST_F(ExtensionApiTest, ShowPageActionPopup) {
  ASSERT_TRUE(RunExtensionTest("page_action/popup")) << message_;
  Extension* extension = GetSingleLoadedExtension();
  ASSERT_TRUE(extension) << message_;

  ASSERT_TRUE(WaitForPageActionVisibilityChangeTo(1));

  {
    ResultCatcher catcher;
    LocationBarTesting* location_bar =
        browser()->window()->GetLocationBar()->GetLocationBarForTesting();
    location_bar->TestPageActionPressed(0);
    ASSERT_TRUE(catcher.GetNextResult());
  }
}
