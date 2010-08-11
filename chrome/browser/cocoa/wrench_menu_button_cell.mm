// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/cocoa/wrench_menu_button_cell.h"

#include "base/scoped_nsobject.h"

@implementation WrenchMenuButtonCell

- (void)drawBezelWithFrame:(NSRect)frame inView:(NSView*)controlView {
  [NSGraphicsContext saveGraphicsState];

  // Inset the rect to match the appearance of the layout of interface builder.
  // The bounding rect of buttons is actually larger than the display rect shown
  // there.
  frame = NSInsetRect(frame, 0.0, 1.0);

  // Stroking the rect gives a weak stroke.  Filling and insetting gives a
  // strong, un-anti-aliased border.
  [[NSColor colorWithDeviceWhite:0.663 alpha:1.0] set];
  NSRectFill(frame);
  frame = NSInsetRect(frame, 1.0, 1.0);

  // The default state should be a subtle gray gradient.
  if (![self isHighlighted]) {
    NSColor* end = [NSColor colorWithDeviceWhite:0.922 alpha:1.0];
    scoped_nsobject<NSGradient> gradient(
        [[NSGradient alloc] initWithStartingColor:[NSColor whiteColor]
                                      endingColor:end]);
    [gradient drawInRect:frame angle:90.0];
  } else {
    // |+selectedMenuItemColor| appears to be a gradient, so just filling the
    // rect with that color produces the desired effect.
    [[NSColor selectedMenuItemColor] set];
    NSRectFill(frame);
  }

  [NSGraphicsContext restoreGraphicsState];
}

- (NSBackgroundStyle)interiorBackgroundStyle {
  if ([self isHighlighted])
    return NSBackgroundStyleDark;
  return [super interiorBackgroundStyle];
}

@end
