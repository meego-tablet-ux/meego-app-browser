// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_COCOA_TAB_STRIP_CONTROLLER_H_
#define CHROME_BROWSER_COCOA_TAB_STRIP_CONTROLLER_H_

#import <Cocoa/Cocoa.h>

#include "base/scoped_nsobject.h"
#include "base/scoped_ptr.h"
#import "chrome/browser/cocoa/tab_controller_target.h"
#import "chrome/browser/cocoa/url_drop_target.h"
#import "third_party/GTM/AppKit/GTMWindowSheetController.h"

@class TabContentsController;
@class TabView;
@class TabStripView;

class Browser;
class ConstrainedWindowMac;
class TabStripModelObserverBridge;
class TabStripModel;
class TabContents;
class ToolbarModel;

// A class that handles managing the tab strip in a browser window. It uses
// a supporting C++ bridge object to register for notifications from the
// TabStripModel. The Obj-C part of this class handles drag and drop and all
// the other Cocoa-y aspects.
//
// For a full description of the design, see
// http://www.chromium.org/developers/design-documents/tab-strip-mac
@interface TabStripController :
  NSObject<TabControllerTarget,
           URLDropTargetController,
           GTMWindowSheetControllerDelegate> {
 @private
  TabContents* currentTab_;   // weak, tab for which we're showing state
  scoped_nsobject<TabStripView> tabStripView_;
  NSView* switchView_;  // weak
  scoped_nsobject<NSView> dragBlockingView_;  // avoid bad window server drags
  NSButton* newTabButton_;  // weak, obtained from the nib.

  // Tracks the newTabButton_ for rollovers.
  scoped_nsobject<NSTrackingArea> newTabTrackingArea_;
  scoped_ptr<TabStripModelObserverBridge> bridge_;
  Browser* browser_;  // weak
  TabStripModel* tabStripModel_;  // weak

  // Access to the TabContentsControllers (which own the parent view
  // for the toolbar and associated tab contents) given an index. Call
  // |indexFromModelIndex:| to convert a |tabStripModel_| index to a
  // |tabContentsArray_| index. Do NOT assume that the indices of
  // |tabStripModel_| and this array are identical, this is e.g. not true while
  // tabs are animating closed (closed tabs are removed from |tabStripModel_|
  // immediately, but from |tabContentsArray_| only after their close animation
  // has completed).
  scoped_nsobject<NSMutableArray> tabContentsArray_;
  // An array of TabControllers which manage the actual tab views. See note
  // above |tabContentsArray_|. |tabContentsArray_| and |tabArray_| always
  // contain objects belonging to the same tabs at the same indices.
  scoped_nsobject<NSMutableArray> tabArray_;

  // Set of TabControllers that are currently animating closed.
  scoped_nsobject<NSMutableSet> closingControllers_;

  // These values are only used during a drag, and override tab positioning.
  TabView* placeholderTab_;  // weak. Tab being dragged
  NSRect placeholderFrame_;  // Frame to use
  CGFloat placeholderStretchiness_; // Vertical force shown by streching tab.
  NSRect droppedTabFrame_;  // Initial frame of a dropped tab, for animation.
  // Frame targets for all the current views.
  // target frames are used because repeated requests to [NSView animator].
  // aren't coalesced, so we store frames to avoid redundant calls.
  scoped_nsobject<NSMutableDictionary> targetFrames_;
  NSRect newTabTargetFrame_;
  // If YES, do not show the new tab button during layout.
  BOOL forceNewTabButtonHidden_;
  // YES if we've successfully completed the initial layout. When this is
  // NO, we probably don't want to do any animation because we're just coming
  // into being.
  BOOL initialLayoutComplete_;

  // Width available for resizing the tabs (doesn't include the new tab
  // button). Used to restrict the available width when closing many tabs at
  // once to prevent them from resizing to fit the full width. If the entire
  // width should be used, this will have a value of |kUseFullAvailableWidth|.
  float availableResizeWidth_;
  // A tracking area that's the size of the tab strip used to be notified
  // when the mouse moves in the tab strip
  scoped_nsobject<NSTrackingArea> trackingArea_;
  TabView* hoveredTab_;  // weak. Tab that the mouse is hovering over

  // Array of subviews which are permanent (and which should never be removed),
  // such as the new-tab button, but *not* the tabs themselves.
  scoped_nsobject<NSMutableArray> permanentSubviews_;

  // The default favicon, so we can use one copy for all buttons.
  scoped_nsobject<NSImage> defaultFavIcon_;

  // The amount by which to indent the tabs on the left (to make room for the
  // red/yellow/green buttons).
  CGFloat indentForControls_;

  // Manages per-tab sheets.
  scoped_nsobject<GTMWindowSheetController> sheetController_;

  // Is the mouse currently inside the strip;
  BOOL mouseInside_;
}

@property(nonatomic) CGFloat indentForControls;

// Initialize the controller with a view and browser that contains
// everything else we'll need. |switchView| is the view whose contents get
// "switched" every time the user switches tabs. The children of this view
// will be released, so if you want them to stay around, make sure
// you have retained them.
- (id)initWithView:(TabStripView*)view
        switchView:(NSView*)switchView
           browser:(Browser*)browser;

// Return the view for the currently selected tab.
- (NSView*)selectedTabView;

// Set the frame of the selected tab, also updates the internal frame dict.
- (void)setFrameOfSelectedTab:(NSRect)frame;

// Move the given tab at index |from| in this window to the location of the
// current placeholder.
- (void)moveTabFromIndex:(NSInteger)from;

// Drop a given TabContents at the location of the current placeholder. If there
// is no placeholder, it will go at the end. Used when dragging from another
// window when we don't have access to the TabContents as part of our strip.
// |frame| is in the coordinate system of the tab strip view and represents
// where the user dropped the new tab so it can be animated into its correct
// location when the tab is added to the model.
- (void)dropTabContents:(TabContents*)contents withFrame:(NSRect)frame;

// Returns the index of the subview |view|. Returns -1 if not present. Takes
// closing tabs into account such that this index will correctly match the tab
// model. If |view| is in the process of closing, returns -1, as closing tabs
// are no longer in the model.
- (NSInteger)modelIndexForTabView:(NSView*)view;

// Return the view at a given index.
- (NSView*)viewAtIndex:(NSUInteger)index;

// Set the placeholder for a dragged tab, allowing the |frame| and |strechiness|
// to be specified. This causes this tab to be rendered in an arbitrary position
- (void)insertPlaceholderForTab:(TabView*)tab
                          frame:(NSRect)frame
                  yStretchiness:(CGFloat)yStretchiness;

// Returns whether or not |tab| can still be fully seen in the tab strip or if
// its current position would cause it be obscured by things such as the edge
// of the window or the window decorations. Returns YES only if the entire tab
// is visible.
- (BOOL)isTabFullyVisible:(TabView*)tab;

// Show or hide the new tab button. The button is hidden immediately, but
// waits until the next call to |-layoutTabs| to show it again.
- (void)showNewTabButton:(BOOL)show;

// Force the tabs to rearrange themselves to reflect the current model.
- (void)layoutTabs;

// Are we in rapid (tab) closure mode? I.e., is a full layout deferred (while
// the user closes tabs)? Needed to overcome missing clicks during rapid tab
// closure.
- (BOOL)inRapidClosureMode;

// Returns YES if the user is allowed to drag tabs on the strip at this moment.
// For example, this returns NO if there are any pending tab close animtations.
- (BOOL)tabDraggingAllowed;

// Default height for tabs.
+ (CGFloat)defaultTabHeight;

// Default indentation for tabs (see |indentForControls_|).
+ (CGFloat)defaultIndentForControls;

// Returns the (lazily created) window sheet controller of this window. Used
// for the per-tab sheets.
- (GTMWindowSheetController*)sheetController;

// Destroys the window sheet controller of this window, if it exists.  The sheet
// controller can be recreated by a subsequent call to |-sheetController|.  Must
// not be called if any sheets are currently open.
// TODO(viettrungluu): This is temporary code needed to allow sheets to work
// (read: not crash) in fullscreen mode.  Once GTMWindowSheetController is
// modified to support moving sheets between windows, this code can go away.
// http://crbug.com/19093.
- (void)destroySheetController;

// Returns the currently active TabContentsController.
- (TabContentsController*)activeTabContentsController;

  // See comments in browser_window_controller.h for documentation about these
  // functions.
- (void)attachConstrainedWindow:(ConstrainedWindowMac*)window;
- (void)removeConstrainedWindow:(ConstrainedWindowMac*)window;
- (void)updateDevToolsForContents:(TabContents*)contents;

@end

// Notification sent when the number of tabs changes. The object will be this
// controller.
extern NSString* const kTabStripNumberOfTabsChanged;

#endif  // CHROME_BROWSER_COCOA_TAB_STRIP_CONTROLLER_H_
