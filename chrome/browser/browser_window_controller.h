// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BROWSER_WINDOW_CONTROLLER_H_
#define CHROME_BROWSER_BROWSER_WINDOW_CONTROLLER_H_

// A class acting as the Objective-C controller for the Browser object. Handles
// interactions between Cocoa and the cross-platform code.

#import <Cocoa/Cocoa.h>

class Browser;
class BrowserWindow;
@class TabStripView;
@class TabContentsController;
@class TabStripController;

@interface BrowserWindowController :
    NSWindowController<NSUserInterfaceValidations> {
 @private
  Browser* browser_;
  BrowserWindow* windowShim_;
  TabStripController* tabStripController_;
  TabContentsController* contentsController_;

  IBOutlet NSBox* contentBox_;
  IBOutlet TabStripView* tabStripView_;

  // Views for the toolbar
  IBOutlet NSView* toolbarView_;
  IBOutlet NSTextField* urlBarView_;
}

// Load the browser window nib and do any Cocoa-specific initialization.
// Takes ownership of |browser|.
- (id)initWithBrowser:(Browser*)browser;

// call to make the browser go away from other places in the cross-platform
// code.
- (void)destroyBrowser;

// Access the C++ bridge between the NSWindow and the rest of Chromium
- (BrowserWindow*)browserWindow;

@end

#endif  // CHROME_BROWSER_BROWSER_WINDOW_CONTROLLER_H_
