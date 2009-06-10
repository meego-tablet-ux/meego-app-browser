// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_COCOA_GROW_BOX_VIEW_H_
#define CHROME_BROWSER_COCOA_GROW_BOX_VIEW_H_

#import <Cocoa/Cocoa.h>

#include "base/scoped_nsobject.h"

// A class that handles drawing and mouse tracking to replace the standard
// window "grow box" grippy in cases where the OS one won't do. Relies on
// an image named "grow_box" be included in the application's resources. The
// box should be 15x15 in size, but that's not a requirement.

@interface GrowBoxView : NSView {
 @private
  scoped_nsobject<NSImage> image_;  // grow box image
  NSPoint startPoint_;  // location of the mouseDown event.
  NSRect startFrame_;  // original window frame.
  NSRect clipRect_;  // constrain the resized window to this frame.
}

@end

#endif  // CHROME_BROWSER_COCOA_GROW_BOX_VIEW_H_
