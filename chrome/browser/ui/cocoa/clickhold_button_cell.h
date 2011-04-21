// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_CLICKHOLD_BUTTON_CELL_H_
#define CHROME_BROWSER_UI_COCOA_CLICKHOLD_BUTTON_CELL_H_
#pragma once

#import <Cocoa/Cocoa.h>

#include "base/memory/scoped_nsobject.h"
#import "chrome/browser/ui/cocoa/gradient_button_cell.h"

// A button cell that implements "click hold" behavior after a specified delay
// or after dragging. If click-hold is never enabled (e.g., if
// |-setEnableClickHold:| is never called), this behaves like a normal button.
@interface ClickHoldButtonCell : GradientButtonCell {
 @private
  BOOL enableClickHold_;
  NSTimeInterval clickHoldTimeout_;
  id clickHoldTarget_;                  // Weak.
  SEL clickHoldAction_;
  BOOL trackOnlyInRect_;
  BOOL activateOnDrag_;
}

// Enable click-hold? Default: NO.
@property(assign, nonatomic) BOOL enableClickHold;

// Timeout is in seconds (at least 0.0, at most 5; 0.0 means that the button
// will always have its click-hold action activated immediately on press).
// Default: 0.25 (a guess at a Cocoa-ish value).
@property(assign, nonatomic) NSTimeInterval clickHoldTimeout;

// Track only in the frame rectangle? Default: NO.
@property(assign, nonatomic) BOOL trackOnlyInRect;

// Activate (click-hold) immediately on a sufficiently-large drag (if not,
// always wait for timeout)? Default: YES.
@property(assign, nonatomic) BOOL activateOnDrag;

// Defines what to do when click-held (as per usual action/target).
@property(assign, nonatomic) id clickHoldTarget;
@property(assign, nonatomic) SEL clickHoldAction;

@end  // @interface ClickHoldButtonCell

#endif  // CHROME_BROWSER_UI_COCOA_CLICKHOLD_BUTTON_CELL_H_
