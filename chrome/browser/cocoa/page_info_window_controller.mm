// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/cocoa/page_info_window_controller.h"

#include "app/resource_bundle.h"
#include "base/mac_util.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#import "chrome/browser/cocoa/nswindow_local_state.h"
#include "chrome/browser/cocoa/page_info_window_mac.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/pref_service.h"

@interface PageInfoWindowController (Private)
// Saves the window preference to the local state.
- (void)saveWindowPositionToLocalState;
@end

@implementation PageInfoWindowController
@synthesize identityImg = identityImg_;
@synthesize connectionImg = connectionImg_;
@synthesize historyImg = historyImg_;
@synthesize identityMsg = identityMsg_;
@synthesize connectionMsg = connectionMsg_;
@synthesize historyMsg = historyMsg_;
@synthesize enableCertButton = enableCertButton_;

- (id)init {
  NSBundle* bundle = mac_util::MainAppBundle();
  NSString* nibpath = [bundle pathForResource:@"PageInfo" ofType:@"nib"];
  if ((self = [super initWithWindowNibPath:nibpath owner:self])) {
    // Load the image refs.
    ResourceBundle& rb = ResourceBundle::GetSharedInstance();
    goodImg_.reset([rb.GetNSImageNamed(IDR_PAGEINFO_GOOD) retain]);
    badImg_.reset([rb.GetNSImageNamed(IDR_PAGEINFO_BAD) retain]);
  }
  return self;
}

- (void)awakeFromNib {
  if (g_browser_process && g_browser_process->local_state()) {
    // Get the positioning information.
    PrefService* prefs = g_browser_process->local_state();
    [[self window] restoreWindowPositionFromPrefs:prefs
                                   withPath:prefs::kPageInfoWindowPlacement];
  }

  // By default, assume we have no history information.
  [self setShowHistoryBox:NO];
}

- (void)dealloc {
  [identityImg_ release];
  [connectionImg_ release];
  [historyImg_ release];
  [identityMsg_ release];
  [connectionMsg_ release];
  [historyMsg_ release];
  [super dealloc];
}

- (void)setPageInfo:(PageInfoWindowMac*)pageInfo {
  pageInfo_.reset(pageInfo);
}

- (NSImage*)goodImg {
  return goodImg_.get();
}

- (NSImage*)badImg {
  return badImg_.get();
}

- (IBAction)showCertWindow:(id)sender {
  pageInfo_->ShowCertDialog(0);  // Pass it any int because it's ignored.
}

- (void)setShowHistoryBox:(BOOL)show {
  [historyBox_ setHidden:!show];

  NSWindow* window = [self window];
  NSRect frame = [window frame];

  const NSSize kPageInfoWindowSize = NSMakeSize(460, 235);
  const NSSize kPageInfoWindowWithHistorySize = NSMakeSize(460, 310);

  NSSize size = (show ? kPageInfoWindowWithHistorySize : kPageInfoWindowSize);

  // Just setting |size| will cause the window to grow upwards. Shift the
  // origin up by grow amount, which causes the window to grow downwards.
  frame.origin.y -= size.height - frame.size.height;
  frame.size = size;

  [window setFrame:frame display:YES animate:YES];
}

// If the page info window gets closed, we have nothing left to manage and we
// can clean ourselves up.
- (void)windowWillClose:(NSNotification*)notif {
  [self autorelease];
}

// The last page info window that was moved will determine the location of the
// next new one.
- (void)windowDidMove:(NSNotification*)notif {
  if (g_browser_process && g_browser_process->local_state())
    [[self window] saveWindowPositionToPrefs:g_browser_process->local_state()
                                    withPath:prefs::kPageInfoWindowPlacement];
}

@end
