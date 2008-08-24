// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_TEST_ACCISSIBILITY_TAB_IMPL_H__
#define CHROME_TEST_ACCISSIBILITY_TAB_IMPL_H__

#include "constants.h"

#include <oleauto.h>

/////////////////////////////////////////////////////////////////////
// CTabImpl
// It is a wrapper to Tab specific functionalities.
// Note:
// In most of the tasks, keyboard messages are used for now.
// DoDefaultAction() will be called on accessibility objects,
// once implementation is available in chrome accessibility server.
// And keyboard messages will be tested using ApplyAccelerator().
/////////////////////////////////////////////////////////////////////

// Forward declaration.
class CBrowserImpl;

// Structure storing Tab parameters.
struct ChromeTab {
  INT64 index_;
  BSTR title_;
};

class CTabImpl {
 public:
  CTabImpl(): tab_(NULL), my_browser_(NULL) {
  }

 public:
  // Close this tab.
  bool Close(void);

  // Returns title of this tab.
  bool GetTitle(BSTR* title);

  // Sets the URL in address bar.
  bool SetAddressBarText(const BSTR text);

  // Sets the URL and navigates tab to the page.
  bool NavigateToURL(const BSTR url);

  // Find string by invoking Find Window.
  bool FindInPage(const BSTR find_text);

  // Reloads/Refreshes the tab-page.
  bool Reload(void);

  // Duplicates this tab.
  bool Duplicate(CTabImpl** tab);

  // Returns true of Authentication dialog is opena nd visible.
  bool IsAuthDialogVisible();

  // Invokes authentication dialog with specified user name and password.
  bool SetAuthDialog(const BSTR user_name, const BSTR password);

  // Cancels invoked authentication dialog.
  bool CancelAuthDialog(void);

  // Authenticates with the credentials set in authentication dialog and
  // closes it.
  bool UseAuthDialog(void);

  // Activates this tab.
  bool Activate(void);

  // Waits for specified time with the specified interval to get the tab
  // activated.
  bool WaitForTabToBecomeActive(const INT64 interval, const INT64 timeout);

  // Waits for specified time with the specified interval to get the tab-page
  // loaded with URL.
  bool WaitForTabToGetLoaded(const INT64 interval, const INT64 timeout);

  // Sets index of this tab.
  void PutIndex(INT64 index) {
    if (index < 0)
      return;
    if (!tab_)
      InitTabData();
    tab_->index_ = index;
  }

  // Sets title of this tab.
  void PutTitle(BSTR title) {
    if (!tab_)
      InitTabData();
     tab_->title_ = SysAllocString(title);
  }

  // Sets browser to which tab belongs.
  bool SetBrowser(CBrowserImpl *browser) {
    if (browser)
      my_browser_ = browser;
    else
      return false;

    return true;
  }

  // Initialize data specific to tab.
  ChromeTab* InitTabData() {
    if (tab_)
      CHK_DELETE(tab_);

    tab_ = new ChromeTab();
    if (!tab_)
      return NULL;

    memset(tab_, 0, sizeof(ChromeTab));
    return tab_;
  }

  // TODO
  bool IsSSLLockPresent(bool* present);
  bool IsSSLSoftError(bool* soft_err);
  bool OpenPageCertificateDialog(void);
  bool ClosePageCertificateDialog(void);
  bool GoBack(void);
  bool GoForward(void);

 private:
  // Structure to store tab data.
  ChromeTab *tab_;

  // Pointer to browser to which this tab belongs.
  CBrowserImpl *my_browser_;
};


#endif  // CHROME_TEST_ACCISSIBILITY_TAB_IMPL_H__

