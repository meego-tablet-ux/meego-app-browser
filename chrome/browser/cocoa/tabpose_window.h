// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_COCOA_TABPOSE_WINDOW_H_
#define CHROME_BROWSER_COCOA_TABPOSE_WINDOW_H_
#pragma once

#import <Cocoa/Cocoa.h>

#include "base/scoped_cftyperef.h"

#include "base/scoped_nsobject.h"
#include "base/scoped_ptr.h"
#include "base/scoped_vector.h"

namespace tabpose {

class Tile;
class TileSet;

}

namespace tabpose {

enum WindowState {
  kFadingIn,
  kFadedIn,
  kFadingOut,
};

}  // namespace tabpose

class TabStripModel;

// A TabposeWindow shows an overview of open tabs and lets the user select a new
// active tab. The window blocks clicks on the tab strip and the download
// shelf. Every open browser window has its own overlay, and they are
// independent of each other.
@interface TabposeWindow : NSWindow {
 @private
  tabpose::WindowState state_;

  // The root layer added to the content view. Covers the whole window.
  CALayer* rootLayer_;  // weak

  // The layer showing the background layer. Covers the whole visible area.
  CALayer* bgLayer_;  // weak

  // The layer drawn behind the currently selected tile.
  CALayer* selectionHighlight_;  // weak

  // Colors used by the layers.
  scoped_cftyperef<CGColorRef> gray_;
  scoped_cftyperef<CGColorRef> darkBlue_;

  TabStripModel* tabStripModel_;  // weak

  // Stores all preview layers. The order in here matches the order in
  // the tabstrip model.
  scoped_nsobject<NSArray> allThumbnailLayers_;

  scoped_nsobject<NSArray> allFaviconLayers_;
  scoped_nsobject<NSArray> allTitleLayers_;

  // Manages the state of all layers.
  scoped_ptr<tabpose::TileSet> tileSet_;
}

// Shows a TabposeWindow on top of |parent|, with |rect| being the active area.
// If |slomo| is YES, then the appearance animation is shown in slow motion.
// The window blocks all keyboard and mouse events and releases itself when
// closed.
+ (id)openTabposeFor:(NSWindow*)parent
                rect:(NSRect)rect
               slomo:(BOOL)slomo
       tabStripModel:(TabStripModel*)tabStripModel;
@end

#endif  // CHROME_BROWSER_COCOA_TABPOSE_WINDOW_H_
