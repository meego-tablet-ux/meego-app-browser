// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/cocoa/bookmark_bar_folder_controller.h"
#include "base/mac_util.h"
#include "base/sys_string_conversions.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "chrome/browser/bookmarks/bookmark_utils.h"
#import "chrome/browser/browser_theme_provider.h"
#import "chrome/browser/cocoa/bookmark_bar_constants.h"  // namespace bookmarks
#import "chrome/browser/cocoa/bookmark_bar_controller.h" // namespace bookmarks
#import "chrome/browser/cocoa/bookmark_bar_folder_view.h"
#import "chrome/browser/cocoa/bookmark_bar_folder_button_cell.h"
#import "chrome/browser/cocoa/bookmark_bar_folder_hover_state.h"
#import "chrome/browser/cocoa/bookmark_folder_target.h"
#import "chrome/browser/cocoa/browser_window_controller.h"
#import "chrome/browser/cocoa/event_utils.h"


namespace {

// Frequency of the scrolling timer in seconds.
const NSTimeInterval kBookmarkBarFolderScrollInterval = 0.2;

// Amount to scroll by per timer fire.  We scroll rather slowly; to
// accomodate we do 2 at a time.
const CGFloat kBookmarkBarFolderScrollAmount =
    2 * (bookmarks::kBookmarkButtonHeight +
         bookmarks::kBookmarkVerticalPadding);

// Amount to scroll for each scroll wheel delta.
const CGFloat kBookmarkBarFolderScrollWheelAmount =
    1 * (bookmarks::kBookmarkButtonHeight +
         bookmarks::kBookmarkVerticalPadding);

// When constraining a scrolling bookmark bar folder window to the
// screen, shrink the "constrain" by this much vertically.  Currently
// this is 0.0 to avoid a problem with tracking areas leaving the
// window, but should probably be 8.0 or something.
// TODO(jrg): http://crbug.com/36225
const CGFloat kScrollWindowVerticalMargin = 0.0;

}  // namespace

@interface BookmarkBarFolderController(Private)
- (void)configureWindow;
- (void)addOrUpdateScrollTracking;
- (void)removeScrollTracking;
- (void)endScroll;
- (void)addScrollTimerWithDelta:(CGFloat)delta;
@end

@implementation BookmarkBarFolderController

- (id)initWithParentButton:(BookmarkButton*)button
          parentController:(BookmarkBarFolderController*)parentController
             barController:(BookmarkBarController*)barController {
  NSString* nibPath =
      [mac_util::MainAppBundle() pathForResource:@"BookmarkBarFolderWindow"
                                          ofType:@"nib"];
  if ((self = [super initWithWindowNibPath:nibPath owner:self])) {
    parentButton_.reset([button retain]);
    parentController_.reset([parentController retain]);
    barController_ = barController;  // WEAK
    buttons_.reset([[NSMutableArray alloc] init]);
    folderTarget_.reset([[BookmarkFolderTarget alloc] initWithController:self]);
    [self configureWindow];
    hoverState_.reset([[BookmarkBarFolderHoverState alloc] init]);
    if (scrollable_)
      [self addOrUpdateScrollTracking];
  }
  return self;
}

- (void)dealloc {
  [self removeScrollTracking];
  [self endScroll];
  // Note: we don't need to
  //   [NSObject cancelPreviousPerformRequestsWithTarget:self];
  // Because all of our performSelector: calls use withDelay: which
  // retains us.
  [super dealloc];
}

// Overriden from NSWindowController to call childFolderWillShow: before showing
// the window.
- (void)showWindow:(id)sender {
  [parentController_ childFolderWillShow:self];
  [super showWindow:sender];
}

- (BookmarkButtonCell*)cellForBookmarkNode:(const BookmarkNode*)child {
  NSImage* image = child ? [barController_ favIconForNode:child] : nil;
  NSMenu* menu = child ? child->is_folder() ? folderMenu_ : buttonMenu_ : nil;
  BookmarkBarFolderButtonCell* cell =
      [BookmarkBarFolderButtonCell buttonCellForNode:child
                                         contextMenu:menu
                                            cellText:nil
                                           cellImage:image];
  return cell;
}

// Redirect to our logic shared with BookmarkBarController.
- (IBAction)openBookmarkFolderFromButton:(id)sender {
  [folderTarget_ openBookmarkFolderFromButton:sender];
}

// Create a bookmark button for the given node using frame.
//
// If |node| is NULL this is an "(empty)" button.
// Does NOT add this button to our button list.
// Returns an autoreleased button.
// Adjusts the input frame width as appropriate.
//
// TODO(jrg): combine with addNodesToButtonList: code from
// bookmark_bar_controller.mm, and generalize that to use both x and y
// offsets.
// http://crbug.com/35966
- (BookmarkButton*)makeButtonForNode:(const BookmarkNode*)node
                               frame:(NSRect)frame {
  BookmarkButtonCell* cell = [self cellForBookmarkNode:node];
  DCHECK(cell);

  // We must decide if we draw the folder arrow before we ask the cell
  // how big it needs to be.
  if (node && node->is_folder()) {
    // Warning when combining code with bookmark_bar_controller.mm:
    // this call should NOT be made for the bar buttons; only for the
    // subfolder buttons.
    [cell setDrawFolderArrow:YES];
  }

  // The "+2" is needed because, sometimes, Cocoa is off by a tad when
  // returning the value it thinks it needs.
  CGFloat desired = [cell cellSize].width + 2;
  frame.size.width = std::min(
    std::max(bookmarks::kBookmarkMenuButtonMinimumWidth, desired),
    bookmarks::kBookmarkMenuButtonMaximumWidth);

  BookmarkButton* button = [[[BookmarkButton alloc] initWithFrame:frame]
                               autorelease];
  DCHECK(button);

  [button setCell:cell];
  [button setDelegate:self];
  if (node) {
    if (node->is_folder()) {
      [button setTarget:self];
      [button setAction:@selector(openBookmarkFolderFromButton:)];
    } else {
      // Make the button do something.
      [button setTarget:self];
      [button setAction:@selector(openBookmark:)];
      // Add a tooltip.
      NSString* title = base::SysWideToNSString(node->GetTitle());
      std::string urlString = node->GetURL().possibly_invalid_spec();
      NSString* tooltip = [NSString stringWithFormat:@"%@\n%s", title,
                                    urlString.c_str()];
      [button setToolTip:tooltip];
    }
  } else {
    [button setEnabled:NO];
    [button setBordered:NO];
  }
  return button;
}

// Exposed for testing.
- (NSView*)mainView {
  return mainView_;
}

- (BookmarkBarFolderController*)folderController {
  return folderController_;
}

- (id)folderTarget {
  return folderTarget_.get();
}

// Compute and return the top left point of our window (screen
// coordinates).  The top left is positioned in a manner similar to
// cascading menus.
- (NSPoint)windowTopLeft {
  NSPoint newWindowTopLeft;
  if (![parentController_ isKindOfClass:[self class]]) {
    // If we're not popping up from one of ourselves, we must be
    // popping up from the bookmark bar itself.  In this case, start
    // BELOW the parent button.  Our left is the button left; our top
    // is bottom of button's parent view.
    NSPoint buttonBottomLeftInScreen =
        [[parentButton_ window]
            convertBaseToScreen:[parentButton_
                                    convertPoint:NSZeroPoint toView:nil]];
    NSPoint bookmarkBarBottomLeftInScreen =
        [[parentButton_ window]
            convertBaseToScreen:[[parentButton_ superview]
                                    convertPoint:NSZeroPoint toView:nil]];
    newWindowTopLeft = NSMakePoint(buttonBottomLeftInScreen.x,
                                   bookmarkBarBottomLeftInScreen.y);
  } else {
    // Our parent controller is another BookmarkBarFolderController.
    // In this case, start ot the RIGHT of the parent button.
    // Start to RIGHT of the button.
    // TODO(jrg): If too far to right, pop left again.
    // http://crbug.com/36225
    newWindowTopLeft.x = NSMaxX([[parentButton_ window] frame]);
    NSPoint top = NSMakePoint(0, (NSMaxY([parentButton_ frame]) +
                                  bookmarks::kBookmarkVerticalPadding));
    NSPoint topOfWindow =
        [[parentButton_ window]
            convertBaseToScreen:[[parentButton_ superview]
                                    convertPoint:top toView:nil]];
    newWindowTopLeft.y = topOfWindow.y;
  }
  return newWindowTopLeft;
}

// Set our window level to the right spot so we're above the menubar, dock, etc.
// Factored out so we can override/noop in a unit test.
- (void)configureWindowLevel {
  [[self window] setLevel:NSPopUpMenuWindowLevel];
}

// Determine window size and position.
// Create buttons for all our nodes.
// TODO(jrg): break up into more and smaller routines for easier unit testing.
- (void)configureWindow {
  NSPoint newWindowTopLeft = [self windowTopLeft];
  const BookmarkNode* node = [parentButton_ bookmarkNode];
  DCHECK(node);
  int startingIndex = [[parentButton_ cell] startingChildIndex];
  DCHECK_LE(startingIndex, node->GetChildCount());
  // Must have at least 1 button (for "empty")
  int buttons = std::max(node->GetChildCount() - startingIndex, 1);

  // Prelim height of the window.  We'll trim later as needed.
  int height = buttons * bookmarks::kBookmarkButtonHeight;
  // We'll need this soon...
  [self window];

  // TODO(jrg): combine with frame code in bookmark_bar_controller.mm
  // http://crbug.com/35966
  NSRect buttonsOuterFrame = NSMakeRect(
    bookmarks::kBookmarkHorizontalPadding,
    height - (bookmarks::kBookmarkBarHeight -
              bookmarks::kBookmarkHorizontalPadding),
    bookmarks::kDefaultBookmarkWidth,
    (bookmarks::kBookmarkBarHeight -
     2 * bookmarks::kBookmarkVerticalPadding));

  // TODO(jrg): combine with addNodesToButtonList: code from
  // bookmark_bar_controller.mm (but use y offset)
  // http://crbug.com/35966
  if (!node->GetChildCount()) {
    // If no children we are the empty button.
    BookmarkButton* button = [self makeButtonForNode:nil
                                               frame:buttonsOuterFrame];
    [buttons_ addObject:button];
    [mainView_ addSubview:button];
  } else {
    for (int i = startingIndex;
         i < node->GetChildCount();
         i++) {
      const BookmarkNode* child = node->GetChild(i);
      BookmarkButton* button = [self makeButtonForNode:child
                                                 frame:buttonsOuterFrame];
      [buttons_ addObject:button];
      [mainView_ addSubview:button];
      buttonsOuterFrame.origin.y -= bookmarks::kBookmarkBarHeight;
    }
  }

  // Now that we have all our buttons we can determine the real size
  // of our window.
  CGFloat width = 0.0;
  for (BookmarkButton* button in buttons_.get()) {
    width = std::max(width, NSWidth([button bounds]));
  }
  width = std::min(width, bookmarks::kBookmarkMenuButtonMaximumWidth);

  // Things look and feel more menu-like if all the buttons are the
  // full width of the window, especially if there are submenus.
  for (BookmarkButton* button in buttons_.get()) {
    NSRect buttonFrame = [button frame];
    buttonFrame.size.width = width;
    [button setFrame:buttonFrame];
  }
  width += (2 * bookmarks::kBookmarkVerticalPadding);

  // Finally, set our window size (make sure it fits on screen).
  NSRect windowFrame = NSMakeRect(newWindowTopLeft.x,
                                  newWindowTopLeft.y - height,
                                  width,
                                  height);

  // Make the window fit on screen, with a distance of at least |padding| from
  // the sides.
  DCHECK([[self window] screen]);
  NSRect screenFrame = [[[self window] screen] frame];
  const CGFloat padding = 8;
  if (NSMaxX(windowFrame) + padding > NSMaxX(screenFrame))
    windowFrame.origin.x -= NSMaxX(windowFrame) + padding - NSMaxX(screenFrame);
  // No 'else' to provide preference for the left side of the menu
  // being visible if neither one fits.  Wish I had an "bool isL2R()"
  // function right here.
  if (NSMinX(windowFrame) - padding < NSMinX(screenFrame))
    windowFrame.origin.x += NSMinX(screenFrame) - NSMinX(windowFrame) + padding;

  // Make the scrolled content be the right size (full size).
  NSRect mainViewFrame = NSMakeRect(0, 0,
                                    NSWidth(windowFrame),
                                    NSHeight(windowFrame));
  [mainView_ setFrame:mainViewFrame];

  // Make sure the window fits on the screen.  If not, constrain.
  // We'll scroll to allow the user to see all the content.
  screenFrame = NSInsetRect(screenFrame, 0, kScrollWindowVerticalMargin);
  if (!NSContainsRect(screenFrame, windowFrame)) {
    scrollable_ = YES;
    windowFrame = NSIntersectionRect(screenFrame, windowFrame);
  }
  [[self window] setFrame:windowFrame display:YES];
  if (scrollable_) {
    [mainView_ scrollPoint:NSMakePoint(0, (NSHeight(mainViewFrame) -
                                           NSHeight(windowFrame)))];
  }

  // Finally pop me up.
  [self configureWindowLevel];
}

// Start a "scroll up" timer.
- (void)beginScrollWindowUp {
  [self addScrollTimerWithDelta:kBookmarkBarFolderScrollAmount];
}

// Start a "scroll down" timer.
- (void)beginScrollWindowDown {
  [self addScrollTimerWithDelta:-kBookmarkBarFolderScrollAmount];
}

// End a scrolling timer.  Can be called excessively with no harm.
- (void)endScroll {
  if (scrollTimer_) {
    [scrollTimer_ invalidate];
    scrollTimer_ = nil;
    verticalScrollDelta_ = 0;
  }
}

// Perform a single scroll of the specified amount.
// Scroll up:
// Scroll the documentView by the growth amount.
// If we cannot grow the window, simply scroll the documentView.
// If we can grow the window up without falling off the screen, do it.
// Scroll down:
// Never change the window size; only scroll the documentView.
- (void)performOneScroll:(CGFloat)delta {
  NSRect windowFrame = [[self window] frame];
  NSRect screenFrame = [[[self window] screen] frame];

  // First scroll the "document" area.
  NSPoint scrollPosition = [scrollView_ documentVisibleRect].origin;
  scrollPosition.y -= delta;
  [[scrollView_ documentView] scrollPoint:scrollPosition];

  // On 10.6 event dispatch for an NSButtonCell's
  // showsBorderOnlyWhileMouseInside seems broken if scrolling the
  // view that contains the button.  It appears that a mouseExited:
  // gets lost, so the button stays highlit forever.  We accomodate
  // here.
  if (buttonThatMouseIsIn_) {
    [[buttonThatMouseIsIn_ cell] setShowsBorderOnlyWhileMouseInside:NO];
    [[buttonThatMouseIsIn_ cell] setShowsBorderOnlyWhileMouseInside:YES];
  }

  // We update the window size after shifting the scroll to avoid a race.
  CGFloat screenHeightMinusMargin = (NSHeight(screenFrame) -
                                     (2 * kScrollWindowVerticalMargin));
  if (delta) {
    // If we can, grow the window (up).
    if (NSHeight(windowFrame) < screenHeightMinusMargin) {
      CGFloat growAmount = delta;
      // Don't scroll more than enough to "finish".
      if (scrollPosition.y < 0)
        growAmount += scrollPosition.y;
      windowFrame.size.height += growAmount;
      windowFrame.size.height = std::min(NSHeight(windowFrame),
                                         screenHeightMinusMargin);
      // Don't allow scrolling to make the window smaller, ever.  This
      // conditional is important when processing scrollWheel events.
      if (windowFrame.size.height > [[self window] frame].size.height) {
        [[self window] setFrame:windowFrame display:YES];
        [self addOrUpdateScrollTracking];
      }
    }
  }

  // If we're at either end, happiness.
  if ((scrollPosition.y <= 0) ||
      ((scrollPosition.y + NSHeight(windowFrame) >=
        NSHeight([mainView_ frame])) &&
       (windowFrame.size.height == screenHeightMinusMargin))) {
    [self endScroll];

    // If the entire view is now visible the window is no longer scrollable.
    if (NSHeight([mainView_ visibleRect]) == NSHeight([mainView_ bounds])) {
      scrollable_ = NO;
      [self removeScrollTracking];
    }
  }
}

// Perform a scroll of the window on the screen.
// Called by a timer when scrolling.
- (void)performScroll:(NSTimer*)timer {
  DCHECK(verticalScrollDelta_);
  [self performOneScroll:verticalScrollDelta_];
}


// Add a timer to fire at a regular interveral which scrolls the
// window vertically |delta|.
- (void)addScrollTimerWithDelta:(CGFloat)delta {
  if (scrollTimer_ && verticalScrollDelta_ == delta)
    return;
  [self endScroll];
  verticalScrollDelta_ = delta;
  scrollTimer_ =
      [NSTimer scheduledTimerWithTimeInterval:kBookmarkBarFolderScrollInterval
                                       target:self
                                     selector:@selector(performScroll:)
                                     userInfo:nil
                                      repeats:YES];
}

// Called as a result of our tracking area.  Warning: on the main
// screen (of a single-screened machine), the minimum mouse y value is
// 1, not 0.  Also, we do not get events when the mouse is above the
// menubar (to be fixed by setting the proper window level; see
// initializer).
- (void)mouseMoved:(NSEvent*)theEvent {
  DCHECK([theEvent window] == [self window]);

  NSPoint eventScreenLocation =
      [[theEvent window] convertBaseToScreen:[theEvent locationInWindow]];

  NSRect visibleRect = [[[self window] screen] visibleFrame];
  CGFloat closeToTopOfScreen = NSMaxY(visibleRect) -
      kBookmarkBarFolderScrollAmount;
  CGFloat closeToBottomOfScreen = NSMinY(visibleRect) +
      kBookmarkBarFolderScrollAmount;

  if (eventScreenLocation.y <= closeToBottomOfScreen) {
    [self beginScrollWindowUp];
  } else if (eventScreenLocation.y > closeToTopOfScreen) {
    [self beginScrollWindowDown];
  } else {
    [self endScroll];
  }
}

- (void)mouseExited:(NSEvent*)theEvent {
  [self endScroll];
}

// Add a tracking area so we know when the mouse is pinned to the top
// or bottom of the screen.  If that happens, and if the mouse
// position overlaps the window, scroll it.
- (void)addOrUpdateScrollTracking {
  [self removeScrollTracking];
  NSView* view = [[self window] contentView];
  scrollTrackingArea_.reset([[NSTrackingArea alloc]
                              initWithRect:[view bounds]
                                   options:(NSTrackingMouseMoved |
                                            NSTrackingMouseEnteredAndExited |
                                            NSTrackingActiveAlways)
                                     owner:self
                                  userInfo:nil]);
    [view addTrackingArea:scrollTrackingArea_];
}

// Remove the tracking area associated with scrolling.
- (void)removeScrollTracking {
  if (scrollTrackingArea_.get()) {
    [[[self window] contentView] removeTrackingArea:scrollTrackingArea_];
  }
  scrollTrackingArea_.reset();
}

- (ThemeProvider*)themeProvider {
  return [parentController_ themeProvider];
}

- (void)childFolderWillShow:(id<BookmarkButtonControllerProtocol>)child {
  // Do nothing.
}

- (void)childFolderWillClose:(id<BookmarkButtonControllerProtocol>)child {
  // Do nothing.
}

// Recursively close all bookmark folders.
- (void)closeAllBookmarkFolders {
  // Closing the top level implicitly closes all children.
  [barController_ closeAllBookmarkFolders];
}

// Close our bookmark folder (a sub-controller) if we have one.
- (void)closeBookmarkFolder:(id)sender {
  // folderController_ may be nil but that's OK.
  [[folderController_ window] close];
  folderController_ = nil;
}

// Delegate callback.
- (void)windowWillClose:(NSNotification*)notification {
  [parentController_ childFolderWillClose:self];
  [self closeBookmarkFolder:self];
  [self autorelease];
}

- (BookmarkButton*)parentButton {
  return parentButton_.get();
}

// Delegate method. Shared implementation with BookmarkBarController.
- (void)fillPasteboard:(NSPasteboard*)pboard
       forDragOfButton:(BookmarkButton*)button {
  [[self folderTarget] fillPasteboard:pboard forDragOfButton:button];
}

// Find something like std::is_between<T>?  I can't believe one doesn't exist.
// http://crbug.com/35966
static BOOL ValueInRangeInclusive(CGFloat low, CGFloat value, CGFloat high) {
  return ((value >= low) && (value <= high));
}

// Return the proposed drop target for a hover open button, or nil if none.
//
// TODO(jrg): this is just like the version in
// bookmark_bar_controller.mm, but vertical instead of horizontal.
// Generalize to be axis independent then share code.
// http://crbug.com/35966
// Get UI review on "middle half" ness.
// http://crbug.com/36276
- (BookmarkButton*)buttonForDroppingOnAtPoint:(NSPoint)point {
  for (BookmarkButton* button in buttons_.get()) {
    // No early break -- makes no assumption about button ordering.

    // Intentionally NOT using NSPointInRect() so that scrolling into
    // a submenu doesn't cause it to be closed.
    if (ValueInRangeInclusive(NSMinY([button frame]),
                              point.y,
                              NSMaxY([button frame]))) {

      // Over a button but let's be a little more specific
      // (e.g. over the middle half).
      NSRect frame = [button frame];
      NSRect middleHalfOfButton = NSInsetRect(frame, 0, frame.size.height / 4);
      if (ValueInRangeInclusive(NSMinY(middleHalfOfButton),
                                point.y,
                                NSMaxY(middleHalfOfButton))) {
        // It makes no sense to drop on a non-folder; there is no hover.
        if (![button isFolder])
          return nil;
        // Got it!
        return button;
      } else {
        // Over a button but not over the middle half.
        return nil;
      }
    }
  }
  // Not hovering over a button.
  return nil;
}

// Most of the work (e.g. drop indicator) is taken care of in the
// folder_view.  Here we handle hover open issues for subfolders.
// Caution: there are subtle differences between this one and
// bookmark_bar_controller.mm's version.
- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)info {
  NSPoint currentLocation = [info draggingLocation];
  BookmarkButton* button = [self buttonForDroppingOnAtPoint:currentLocation];

  // Delegate handling of dragging over a button to the |hoverState_| member.
  return [hoverState_ draggingEnteredButton:button];
}

// Unlike bookmark_bar_controller, we need to keep track of dragging state.
// We also need to make sure we cancel the delayed hover close.
- (void)draggingExited:(id<NSDraggingInfo>)info {
  // NOT the same as a cancel --> we may have moved the mouse into the submenu.
  // Delegate handling of the hover button to the |hoverState_| member.
  [hoverState_ draggingExited];
}

- (BOOL)dragShouldLockBarVisibility {
  return [parentController_ dragShouldLockBarVisibility];
}

- (NSWindow*)browserWindow {
  return [parentController_ browserWindow];
}

// TODO(jrg): again we have code dup, sort of, with
// bookmark_bar_controller.mm, but the axis is changed.  One minor
// difference is accomodation for the "empty" button (which may not
// exist in the future).
// http://crbug.com/35966
- (int)indexForDragOfButton:(BookmarkButton*)sourceButton
                    toPoint:(NSPoint)point {
  DCHECK([sourceButton isKindOfClass:[BookmarkButton class]]);

  // Identify which buttons we are between.  For now, assume a button
  // location is at the center point of its view, and that an exact
  // match means "place before".
  // TODO(jrg): revisit position info based on UI team feedback.
  // dropLocation is in bar local coordinates.
  // http://crbug.com/36276
  NSPoint dropLocation =
      [mainView_ convertPoint:point
                     fromView:[[self window] contentView]];
  BookmarkButton* buttonToTheTopOfDraggedButton = nil;
  // Buttons are laid out in this array from top to bottom (screen
  // wise), which means "biggest y" --> "smallest y".
  for (BookmarkButton* button in buttons_.get()) {
    CGFloat midpoint = NSMidY([button frame]);
    if (dropLocation.y > midpoint) {
      break;
    }
    buttonToTheTopOfDraggedButton = button;
  }

  // TODO(jrg): On Windows, dropping onto (empty) highlights the
  // entire drop location and does not use an insertion point.
  // http://crbug.com/35967
  if (!buttonToTheTopOfDraggedButton) {
    // We are at the very top (we broke out of the loop on the first try).
    return 0;
  }
  if ([buttonToTheTopOfDraggedButton isEmpty]) {
    // There is a button but it's an empty placeholder.
    // Default to inserting on top of it.
    return 0;
  }
  const BookmarkNode* beforeNode = [buttonToTheTopOfDraggedButton
                                       bookmarkNode];
  DCHECK(beforeNode);
  // Be careful if the number of buttons != number of nodes.
  return ((beforeNode->GetParent()->IndexOfChild(beforeNode) + 1) -
          [[parentButton_ cell] startingChildIndex]);
}

- (BookmarkModel*)bookmarkModel {
  return [barController_ bookmarkModel];
}

// TODO(jrg): ARGH more code dup.
// http://crbug.com/35966
- (BOOL)dragButton:(BookmarkButton*)sourceButton
                to:(NSPoint)point
              copy:(BOOL)copy {
  DCHECK([sourceButton isKindOfClass:[BookmarkButton class]]);

  const BookmarkNode* sourceNode = [sourceButton bookmarkNode];
  DCHECK(sourceNode);

  // Drop destination.
  const BookmarkNode* destParent = NULL;
  int destIndex = 0;

  // First check if we're dropping on a button.  If we have one, and
  // it's a folder, drop in it.
  BookmarkButton* button = [self buttonForDroppingOnAtPoint:point];
  if ([button isFolder]) {
    destParent = [button bookmarkNode];
    // Drop it at the end.
    destIndex = [button bookmarkNode]->GetChildCount();
  } else {
    // Else we're dropping somewhere in the folder, so find the right spot.
    destParent = [parentButton_ bookmarkNode];
    destIndex = [self indexForDragOfButton:sourceButton toPoint:point];
    // Be careful if the number of buttons != number of nodes.
    destIndex += [[parentButton_ cell] startingChildIndex];
  }

  if (copy)
    [self bookmarkModel]->Copy(sourceNode, destParent, destIndex);
  else
    [self bookmarkModel]->Move(sourceNode, destParent, destIndex);

  [self closeAllBookmarkFolders];  // For a hover open, if needed.

  // Movement of a node triggers observers (like us) to rebuild the
  // bar so we don't have to do so explicitly.

  return YES;
}

// Return YES if we should show the drop indicator, else NO.
// TODO(jrg): ARGH code dup!
// http://crbug.com/35966
- (BOOL)shouldShowIndicatorShownForPoint:(NSPoint)point {
  return ![self buttonForDroppingOnAtPoint:point];
}


// Return the y position for a drop indicator.
//
// TODO(jrg): again we have code dup, sort of, with
// bookmark_bar_controller.mm, but the axis is changed.
// http://crbug.com/35966
- (CGFloat)indicatorPosForDragOfButton:(BookmarkButton*)sourceButton
                               toPoint:(NSPoint)point {
  CGFloat y = 0;
  int destIndex = [self indexForDragOfButton:sourceButton toPoint:point];
  int numButtons = static_cast<int>([buttons_ count]);

  // If it's a drop strictly between existing buttons or at the very beginning
  if (destIndex >= 0 && destIndex < numButtons) {
    // ... put the indicator right between the buttons.
    BookmarkButton* button =
        [buttons_ objectAtIndex:static_cast<NSUInteger>(destIndex)];
    DCHECK(button);
    NSRect buttonFrame = [button frame];
    y = buttonFrame.origin.y +
        buttonFrame.size.height  +
        0.5 * bookmarks::kBookmarkVerticalPadding;

  // If it's a drop at the end (past the last button, if there are any) ...
  } else if (destIndex == numButtons) {
    // and if it's past the last button ...
    if (numButtons > 0) {
      // ... find the last button, and put the indicator below it.
      BookmarkButton* button =
          [buttons_ objectAtIndex:static_cast<NSUInteger>(destIndex - 1)];
      DCHECK(button);
      NSRect buttonFrame = [button frame];
      y = buttonFrame.origin.y -
          0.5 * bookmarks::kBookmarkVerticalPadding;

    }
  } else {
    NOTREACHED();
  }

  return y;
}

// Close the old hover-open bookmark folder, and open a new one.  We
// do both in one step to allow for a delay in closing the old one.
// See comments above kDragHoverCloseDelay (bookmark_bar_controller.h)
// for more details.
- (void)openBookmarkFolderFromButtonAndCloseOldOne:(id)sender {
  // If an old submenu exists, close it immediately.
  [self closeBookmarkFolder:sender];

  // Open a new one if meaningful.
  if ([sender isFolder])
    [folderTarget_ openBookmarkFolderFromButton:sender];
}

// Called from BookmarkButton.
// Unlike bookmark_bar_controller's version, we DO default to being enabled.
- (void)mouseEnteredButton:(id)sender event:(NSEvent*)event {
  buttonThatMouseIsIn_ = sender;

  // Cancel a previous hover if needed.
  [NSObject cancelPreviousPerformRequestsWithTarget:self];

  // If already opened, then we exited but re-entered the button
  // (without entering another button open), do nothing.
  if ([folderController_ parentButton] == sender)
    return;

  [self performSelector:@selector(openBookmarkFolderFromButtonAndCloseOldOne:)
             withObject:sender
             afterDelay:bookmarks::kHoverOpenDelay];
}

// Called from the BookmarkButton
- (void)mouseExitedButton:(id)sender event:(NSEvent*)event {
  if (buttonThatMouseIsIn_ == sender)
    buttonThatMouseIsIn_ = nil;

  // Stop any timer about opening a new hover-open folder.

  // Since a performSelector:withDelay: on self retains self, it is
  // possible that a cancelPreviousPerformRequestsWithTarget: reduces
  // the refcount to 0, releasing us.  That's a bad thing to do while
  // this object (or others it may own) is in the event chain.  Thus
  // we have a retain/autorelease.
  [self retain];
  [NSObject cancelPreviousPerformRequestsWithTarget:self];
  [self autorelease];
}

- (void)openAll:(const BookmarkNode*)node
    disposition:(WindowOpenDisposition)disposition {
  [parentController_ openAll:node disposition:disposition];
}

// Add a new folder controller as triggered by the given folder button.
- (void)addNewFolderControllerWithParentButton:(BookmarkButton*)parentButton {
  if (folderController_)
    [self closeAllBookmarkFolders];

  // Folder controller, like many window controllers, owns itself.
  folderController_ =
      [[BookmarkBarFolderController alloc] initWithParentButton:parentButton
                                               parentController:self
                                                  barController:barController_];
  [folderController_ showWindow:self];
}

- (NSArray*)buttons {
  return buttons_.get();
}

- (void)close {
  [folderController_ close];
  [super close];
}

- (void)scrollWheel:(NSEvent *)theEvent {
  if (scrollable_) {
    // We go negative since an NSScrollView has a flipped coordinate frame.
    CGFloat amt = kBookmarkBarFolderScrollWheelAmount * -[theEvent deltaY];
    [self performOneScroll:amt];
  }
}

#pragma mark Methods Forwarded to BookmarkBarController

- (IBAction)cutBookmark:(id)sender {
  [barController_ cutBookmark:sender];
}

- (IBAction)copyBookmark:(id)sender {
  [barController_ cutBookmark:sender];
}

- (IBAction)pasteBookmark:(id)sender {
  [barController_ pasteBookmark:sender];
}

- (IBAction)deleteBookmark:(id)sender {
  [barController_ deleteBookmark:sender];
}

- (IBAction)openBookmark:(id)sender {
  [barController_ openBookmark:sender];
}

- (IBAction)addFolder:(id)sender {
  [barController_ addFolder:sender];
}

- (IBAction)addPage:(id)sender {
  [barController_ addPage:sender];
}

- (IBAction)editBookmark:(id)sender {
  [barController_ editBookmark:sender];
}

- (IBAction)openAllBookmarks:(id)sender {
  [barController_ openAllBookmarks:sender];
}

- (IBAction)openAllBookmarksIncognitoWindow:(id)sender {
  [barController_ openAllBookmarksIncognitoWindow:sender];
}

- (IBAction)openAllBookmarksNewWindow:(id)sender {
  [barController_ openAllBookmarksNewWindow:sender];
}

- (IBAction)openBookmarkInIncognitoWindow:(id)sender {
  [barController_ openBookmarkInIncognitoWindow:sender];
}

- (IBAction)openBookmarkInNewForegroundTab:(id)sender {
  [barController_ openBookmarkInNewForegroundTab:sender];
}

- (IBAction)openBookmarkInNewWindow:(id)sender {
  [barController_ openBookmarkInNewWindow:sender];
}


@end  // BookmarkBarFolderController
