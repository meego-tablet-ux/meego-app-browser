// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/cocoa/tab_strip_controller.h"

#include "app/l10n_util.h"
#include "base/sys_string_conversions.h"
#include "chrome/app/chrome_dll_resource.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/profile.h"
#import "chrome/browser/cocoa/tab_strip_view.h"
#import "chrome/browser/cocoa/tab_cell.h"
#import "chrome/browser/cocoa/tab_contents_controller.h"
#import "chrome/browser/cocoa/tab_controller.h"
#import "chrome/browser/cocoa/tab_strip_model_observer_bridge.h"
#import "chrome/browser/cocoa/tab_view.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/browser/tabs/tab_strip_model.h"
#include "grit/generated_resources.h"

@implementation TabStripController

- (id)initWithView:(TabStripView*)view
        switchView:(NSView*)switchView
             model:(TabStripModel*)model {
  DCHECK(view && switchView && model);
  if ((self = [super init])) {
    tabView_ = view;
    switchView_ = switchView;
    tabModel_ = model;
    bridge_.reset(new TabStripModelObserverBridge(tabModel_, self));
    tabContentsArray_.reset([[NSMutableArray alloc] init]);
    tabArray_.reset([[NSMutableArray alloc] init]);
    // Take the only child view present in the nib as the new tab button. For
    // some reason, if the view is present in the nib apriori, it draws
    // correctly. If we create it in code and add it to the tab view, it draws
    // with all sorts of crazy artifacts.
    newTabButton_ = [[tabView_ subviews] objectAtIndex:0];
    DCHECK([newTabButton_ isKindOfClass:[NSButton class]]);
    [newTabButton_ setTarget:nil];
    [newTabButton_ setAction:@selector(commandDispatch:)];
    [newTabButton_ setTag:IDC_NEW_TAB];

    [tabView_ setWantsLayer:YES];
  }
  return self;
}

+ (CGFloat)defaultTabHeight {
  return 24.0;
}

// Finds the associated TabContentsController at the given |index| and swaps
// out the sole child of the contentArea to display its contents.
- (void)swapInTabAtIndex:(NSInteger)index {
  TabContentsController* controller = [tabContentsArray_ objectAtIndex:index];

  // Resize the new view to fit the window
  NSView* newView = [controller view];
  NSRect frame = [switchView_ bounds];
  [newView setFrame:frame];

  // Remove the old view from the view hierarchy. We know there's only one
  // child of |switchView_| because we're the one who put it there. There
  // may not be any children in the case of a tab that's been closed, in
  // which case there's no swapping going on.
  NSArray* subviews = [switchView_ subviews];
  if ([subviews count]) {
    NSView* oldView = [subviews objectAtIndex:0];
    [switchView_ replaceSubview:oldView with:newView];
  } else {
    [switchView_ addSubview:newView];
  }
}

// Create a new tab view and set its cell correctly so it draws the way we want
// it to. It will be sized and positioned by |-layoutTabs| so there's no need to
// set the frame here. This also creates the view as hidden, it will be
// shown during layout.
- (TabController*)newTab {
  TabController* controller = [[[TabController alloc] init] autorelease];
  [controller setTarget:self];
  [controller setAction:@selector(selectTab:)];
  [[controller view] setHidden:YES];

  return controller;
}

// Returns the number of tabs in the tab strip. This is just the number
// of TabControllers we know about as there's a 1-to-1 mapping from these
// controllers to a tab.
- (NSInteger)numberOfTabViews {
  return [tabArray_ count];
}

// Returns the index of the subview |view|. Returns -1 if not present.
- (NSInteger)indexForTabView:(NSView*)view {
  NSInteger index = 0;
  for (TabController* current in tabArray_.get()) {
    if ([current view] == view)
      return index;
    ++index;
  }
  return -1;
}

// Returns the view at the given index, using the array of TabControllers to
// get the associated view. Returns nil if out of range.
- (NSView*)viewAtIndex:(NSUInteger)index {
  if (index >= [tabArray_ count])
    return NULL;
  return [[tabArray_ objectAtIndex:index] view];
}

// Called when the user clicks a tab. Tell the model the selection has changed,
// which feeds back into us via a notification.
- (void)selectTab:(id)sender {
  int index = [self indexForTabView:sender];
  if (index >= 0 && tabModel_->ContainsIndex(index))
    tabModel_->SelectTabContentsAt(index, true);
}

// Called when the user closes a tab.  Asks the model to close the tab.
- (void)closeTab:(id)sender {
  if ([self numberOfTabViews] > 1) {
    int index = [self indexForTabView:sender];
    if (index >= 0 && tabModel_->ContainsIndex(index))
      tabModel_->CloseTabContentsAt(index);
  } else {
    // Use the standard window close if this is the last tab
    // this prevents the tab from being removed from the model until after
    // the window dissapears
    [[tabView_ window] performClose:nil];
  }
}

- (void)insertPlaceholderForTab:(TabView*)tab
                          frame:(NSRect)frame
                  yStretchiness:(CGFloat)yStretchiness {
  placeholderTab_ = tab;
  placeholderFrame_ = frame;
  placeholderStretchiness_ = yStretchiness;
  [self layoutTabs];
}

// Lay out all tabs in the order of their TabContentsControllers, which matches
// the ordering in the TabStripModel. This call isn't that expensive, though
// it is O(n) in the number of tabs. Tabs will animate to their new position
// if the window is visible.
// TODO(pinkerton): Handle drag placeholders via proxy objects, perhaps a
// subclass of TabContentsController with everything stubbed out or by
// abstracting a base class interface.
// TODO(pinkerton): Note this doesn't do too well when the number of min-sized
// tabs would cause an overflow.
- (void)layoutTabs {
  const float kIndentLeavingSpaceForControls = 64.0;
  const float kTabOverlap = 20.0;
  const float kNewTabButtonOffset = 8.0;
  const float kMaxTabWidth = [TabController maxTabWidth];
  const float kMinTabWidth = [TabController minTabWidth];

  [NSAnimationContext beginGrouping];
  [[NSAnimationContext currentContext] setDuration:0.2];

  BOOL visible = [[tabView_ window] isVisible];
  float availableWidth =
      NSWidth([tabView_ frame]) - NSWidth([newTabButton_ frame]);
  float offset = kIndentLeavingSpaceForControls;
  const float baseTabWidth =
      MAX(MIN((availableWidth - offset) / [tabContentsArray_ count],
              kMaxTabWidth),
          kMinTabWidth);

  CGFloat minX = NSMinX(placeholderFrame_);

  NSUInteger i = 0;
  NSInteger gap = -1;
  for (TabController* tab in tabArray_.get()) {
    BOOL isPlaceholder = [[tab view] isEqual:placeholderTab_];
    NSRect tabFrame = [[tab view] frame];
    tabFrame.size.height = [[self class] defaultTabHeight];
    tabFrame.origin.y = 0;
    tabFrame.origin.x = offset;

    // If the tab is hidden, we consider it a new tab. We make it visible
    // and animate it in.
    BOOL newTab = [[tab view] isHidden];
    if (newTab) {
      [[tab view] setHidden:NO];
    }

    if (isPlaceholder) {
      tabFrame.origin.x = placeholderFrame_.origin.x;
      tabFrame.size.height += 10.0 * placeholderStretchiness_;
      [[tab view] setFrame:tabFrame];
      continue;
    } else {
      // If our left edge is to the left of the placeholder's left, but our mid
      // is to the right of it we should slide over to make space for it.
      if (placeholderTab_ && gap < 0 && NSMidX(tabFrame) > minX) {
        gap = i;
        offset += NSWidth(tabFrame);
        offset -= kTabOverlap;
        tabFrame.origin.x = offset;
      }

      // Animate the tab in by putting it below the horizon.
      if (newTab && visible) {
        [[tab view] setFrame:NSOffsetRect(tabFrame, 0, -NSHeight(tabFrame))];
      }

      id frameTarget = visible ? [[tab view] animator] : [tab view];
      tabFrame.size.width = [tab selected] ? kMaxTabWidth : baseTabWidth;
      [frameTarget setFrame:tabFrame];
    }

    if (offset < availableWidth) {
      offset += NSWidth(tabFrame);
      offset -= kTabOverlap;
    }
    i++;
  }

  // Move the new tab button into place
  [[newTabButton_ animator] setFrameOrigin:
      NSMakePoint(MIN(availableWidth, offset + kNewTabButtonOffset), 0)];
  if (i > 0) [[newTabButton_ animator] setHidden:NO];
  [NSAnimationContext endGrouping];
}

// Handles setting the title of the tab based on the given |contents|. Uses
// a canned string if |contents| is NULL.
- (void)setTabTitle:(NSViewController*)tab withContents:(TabContents*)contents {
  NSString* titleString = nil;
  if (contents)
    titleString = base::SysUTF16ToNSString(contents->GetTitle());
  if (![titleString length]) {
    titleString =
      base::SysWideToNSString(
          l10n_util::GetString(IDS_BROWSER_WINDOW_MAC_TAB_UNTITLED));
  }
  [tab setTitle:titleString];
}

// Called when a notification is received from the model to insert a new tab
// at |index|.
- (void)insertTabWithContents:(TabContents*)contents
                      atIndex:(NSInteger)index
                 inForeground:(bool)inForeground {
  DCHECK(contents);
  DCHECK(index == TabStripModel::kNoTab || tabModel_->ContainsIndex(index));

  // TODO(pinkerton): handle tab dragging in here

  // Make a new tab. Load the contents of this tab from the nib and associate
  // the new controller with |contents| so it can be looked up later.
  TabContentsController* contentsController =
      [[[TabContentsController alloc] initWithNibName:@"TabContents"
                                             contents:contents]
          autorelease];
  [tabContentsArray_ insertObject:contentsController atIndex:index];

  // Make a new tab and add it to the strip. Keep track of its controller.
  TabController* newController = [self newTab];
  [tabArray_ insertObject:newController atIndex:index];
  NSView* newView = [newController view];
  [newView setFrame:NSOffsetRect([newView frame],
                                 0, [[self class] defaultTabHeight])];

  [tabView_ addSubview:newView
            positioned:inForeground ? NSWindowAbove : NSWindowBelow
            relativeTo:nil];

  [self setTabTitle:newController withContents:contents];

  // We don't need to call |-layoutTabs| if the tab will be in the foreground
  // because it will get called when the new tab is selected by the tab model.
  if (!inForeground) {
    [self layoutTabs];
  }
}

// Called when a notification is received from the model to select a particular
// tab. Swaps in the toolbar and content area associated with |newContents|.
- (void)selectTabWithContents:(TabContents*)newContents
             previousContents:(TabContents*)oldContents
                      atIndex:(NSInteger)index
                  userGesture:(bool)wasUserGesture {
  // De-select all other tabs and select the new tab.
  int i = 0;
  for (TabController* current in tabArray_.get()) {
    [current setSelected:(i == index) ? YES : NO];
    ++i;
  }

  // Make this the top-most tab in the strips's z order.
  NSView* selectedTab = [self viewAtIndex:index];
  [tabView_ addSubview:selectedTab positioned:NSWindowAbove relativeTo:nil];

  // Tell the new tab contents it is about to become the selected tab. Here it
  // can do things like make sure the toolbar is up to date.
  TabContentsController* newController =
      [tabContentsArray_ objectAtIndex:index];
  [newController willBecomeSelectedTab];

  // Relayout for new tabs and to let the selected tab grow to be larger in
  // size than surrounding tabs if the user has many.
  [self layoutTabs];

  // Swap in the contents for the new tab
  [self swapInTabAtIndex:index];
}

// Called when a notification is received from the model that the given tab
// has gone away. Remove all knowledge about this tab and it's associated
// controller and remove the view from the strip.
- (void)tabDetachedWithContents:(TabContents*)contents
                        atIndex:(NSInteger)index {
  // Release the tab contents controller so those views get destroyed. This
  // will remove all the tab content Cocoa views from the hierarchy. A
  // subsequent "select tab" notification will follow from the model. To
  // tell us what to swap in in its absence.
  [tabContentsArray_ removeObjectAtIndex:index];

  // Remove the |index|th view from the tab strip
  NSView* tab = [self viewAtIndex:index];
  [tab removeFromSuperview];

  // Once we're totally done with the tab, delete its controller
  [tabArray_ removeObjectAtIndex:index];

  [self layoutTabs];
}

// Called when a notification is received from the model that the given tab
// has been updated. |loading| will be YES when we only want to update the
// throbber state, not anything else about the (partially) loading tab.
- (void)tabChangedWithContents:(TabContents*)contents
                       atIndex:(NSInteger)index
                   loadingOnly:(BOOL)loading {
  if (!loading)
    [self setTabTitle:[tabArray_ objectAtIndex:index] withContents:contents];

  TabContentsController* updatedController =
      [tabContentsArray_ objectAtIndex:index];
  [updatedController tabDidChange:contents];
}

// Called when a tab is moved (usually by drag&drop). Keep our parallel arrays
// in sync with the tab strip model.
- (void)tabMovedWithContents:(TabContents*)contents
                    fromIndex:(NSInteger)from
                      toIndex:(NSInteger)to {
  scoped_nsobject<TabContentsController> movedController(
      [[tabContentsArray_ objectAtIndex:from] retain]);
  [tabContentsArray_ removeObjectAtIndex:from];
  [tabContentsArray_ insertObject:movedController.get() atIndex:to];
  scoped_nsobject<TabView> movedView(
      [[tabArray_ objectAtIndex:from] retain]);
  [tabArray_ removeObjectAtIndex:from];
  [tabArray_ insertObject:movedView.get() atIndex:to];

  [self layoutTabs];
}

- (NSView *)selectedTabView {
  int selectedIndex = tabModel_->selected_index();
  return [self viewAtIndex:selectedIndex];
}

// Find the index based on the x coordinate of the placeholder. If there is
// no placeholder, this returns the end of the tab strip.
- (int)indexOfPlaceholder {
  double placeholderX = placeholderFrame_.origin.x;
  int index = 0;
  int location = 0;
  const int count = tabModel_->count();
  while (index < count) {
    NSView* curr = [self viewAtIndex:index];
    // The placeholder tab works by changing the frame of the tab being dragged
    // to be the bounds of the placeholder, so we need to skip it while we're
    // iterating, otherwise we'll end up off by one.  Note This only effects
    // dragging to the right, not to the left.
    if (curr == placeholderTab_) {
      index++;
      continue;
    }
    if (placeholderX <= NSMinX([curr frame]))
      break;
    index++;
    location++;
  }
  return location;
}

// Move the given tab at index |from| in this window to the location of the
// current placeholder.
- (void)moveTabFromIndex:(NSInteger)from {
  int toIndex = [self indexOfPlaceholder];
  tabModel_->MoveTabContentsAt(from, toIndex, true);
}

// Drop a given TabContents at the location of the current placeholder. If there
// is no placeholder, it will go at the end. Used when dragging from another
// window when we don't have access to the TabContents as part of our strip.
- (void)dropTabContents:(TabContents*)contents {
  int index = [self indexOfPlaceholder];

  // Insert it into this tab strip. We want it in the foreground and to not
  // inherit the current tab's group.
  tabModel_->InsertTabContentsAt(index, contents, true, false);
}

// Return the rect, in WebKit coordinates (flipped), of the window's grow box
// in the coordinate system of the content area of the currently selected tab.
- (NSRect)selectedTabGrowBoxRect {
  int selectedIndex = tabModel_->selected_index();
  if (selectedIndex == TabStripModel::kNoTab) {
    // When the window is initially being constructed, there may be no currently
    // selected tab, so pick the first one. If there aren't any, just bail with
    // an empty rect.
    selectedIndex = 0;
  }
  TabContentsController* selectedController =
      [tabContentsArray_ objectAtIndex:selectedIndex];
  if (!selectedController)
    return NSZeroRect;
  return [selectedController growBoxRect];
}

@end
