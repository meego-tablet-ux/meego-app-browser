// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/browser_window.h"

#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"

BrowserWindow* BrowserWindow::CreateBrowserWindow(Browser* browser) {
  BrowserWindowQt* window = new BrowserWindowQt(browser);
  return window;
}

FindBar* BrowserWindow::CreateFindBar(Browser* browser) {
  return NULL; //new FindBarQt(browser);
}
