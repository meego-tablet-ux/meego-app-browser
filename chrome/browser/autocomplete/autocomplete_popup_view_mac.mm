// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autocomplete/autocomplete_popup_view_mac.h"

#include "app/resource_bundle.h"
#include "app/text_elider.h"
#include "base/stl_util-inl.h"
#include "base/sys_string_conversions.h"
#include "chrome/browser/autocomplete/autocomplete_edit.h"
#include "chrome/browser/autocomplete/autocomplete_edit_view_mac.h"
#include "chrome/browser/autocomplete/autocomplete_popup_model.h"
#include "chrome/browser/cocoa/event_utils.h"
#include "chrome/browser/cocoa/image_utils.h"
#include "gfx/rect.h"
#include "grit/theme_resources.h"
#include "skia/ext/skia_utils_mac.h"
#import "third_party/GTM/AppKit/GTMNSAnimation+Duration.h"

namespace {

// The size delta between the font used for the edit and the result
// rows.
const int kEditFontAdjust = -1;

// How much to adjust the cell sizing up from the default determined
// by the font.
const int kCellHeightAdjust = 7.0;

// How to round off the popup's corners.  Goal is to match star and go
// buttons.
const CGFloat kPopupRoundingRadius = 3.5;

// Gap between the field and the popup.
const CGFloat kPopupFieldGap = 2.0;

// How opaque the popup window should be.  This matches Windows (see
// autocomplete_popup_contents_view.cc, kGlassPopupTransparency).
const CGFloat kPopupAlpha = 240.0 / 255.0;

// How far to offset image column from the left.
const CGFloat kImageXOffset = 2.0;

// How far to offset the text column from the left.
const CGFloat kTextXOffset = 25.0;

// Animation duration when animating the popup window smaller.
const NSTimeInterval kShrinkAnimationDuration = 0.1;

// A very short animation duration used to cancel running animations.
const NSTimeInterval kCancelAnimationDuration = 0.00001;

// Maximum fraction of the popup width that can be used to display match
// contents.
const float kMaxContentsFraction = 0.7;

// NSEvent -buttonNumber for middle mouse button.
const static NSInteger kMiddleButtonNumber(2);

// The autocomplete field's visual border is slightly inset from the
// actual border so that it can spill a glow into the toolbar or
// something like that.  This is how much to inset vertically.
const CGFloat kFieldVisualInset = 1.0;

// The popup window has a single-pixel border in screen coordinates,
// which has to be backed out to line the borders up with the field
// borders.
const CGFloat kWindowBorderWidth = 1.0;

// |AutocompleteButtonCell| and |AutocompleteTextFieldCell| draw their
// text somewhat differently.  The image needs to be adjusted slightly
// downward to align with the text the same.
const CGFloat kImageBaselineAdjust = 1.0;

// Background colors for different states of the popup elements.
NSColor* BackgroundColor() {
  return [[NSColor controlBackgroundColor] colorWithAlphaComponent:kPopupAlpha];
}
NSColor* SelectedBackgroundColor() {
  return [[NSColor selectedControlColor] colorWithAlphaComponent:kPopupAlpha];
}
NSColor* HoveredBackgroundColor() {
  return [[NSColor controlHighlightColor] colorWithAlphaComponent:kPopupAlpha];
}

static NSColor* ContentTextColor() {
  return [NSColor blackColor];
}
static NSColor* URLTextColor() {
  return [NSColor colorWithCalibratedRed:0.0 green:0.55 blue:0.0 alpha:1.0];
}
static NSColor* DescriptionTextColor() {
  return [NSColor darkGrayColor];
}

}  // namespace

// Helper for MatchText() to allow sharing code between the contents
// and description cases.  Returns NSMutableAttributedString as a
// convenience for MatchText().
NSMutableAttributedString* AutocompletePopupViewMac::DecorateMatchedString(
    const std::wstring &matchString,
    const AutocompleteMatch::ACMatchClassifications &classifications,
    NSColor* textColor, gfx::Font& font) {
  // Cache for on-demand computation of the bold version of |font|.
  NSFont* boldFont = nil;

  // Start out with a string using the default style info.
  NSString* s = base::SysWideToNSString(matchString);
  NSDictionary* attributes = [NSDictionary dictionaryWithObjectsAndKeys:
                                  font.nativeFont(), NSFontAttributeName,
                                  textColor, NSForegroundColorAttributeName,
                                  nil];
  NSMutableAttributedString* as =
      [[[NSMutableAttributedString alloc] initWithString:s
                                              attributes:attributes]
        autorelease];

  // Mark up the runs which differ from the default.
  for (ACMatchClassifications::const_iterator i = classifications.begin();
       i != classifications.end(); ++i) {
    const BOOL isLast = (i+1) == classifications.end();
    const size_t nextOffset = (isLast ? matchString.length() : (i+1)->offset);
    const NSInteger location = static_cast<NSInteger>(i->offset);
    const NSInteger length = static_cast<NSInteger>(nextOffset - i->offset);
    const NSRange range = NSMakeRange(location, length);

    if (0 != (i->style & ACMatchClassification::URL)) {
      [as addAttribute:NSForegroundColorAttributeName
                 value:URLTextColor() range:range];
    }

    if (0 != (i->style & ACMatchClassification::MATCH)) {
      if (!boldFont) {
        NSFontManager* fontManager = [NSFontManager sharedFontManager];
        boldFont = [fontManager convertFont:font.nativeFont()
                                toHaveTrait:NSBoldFontMask];
      }
      [as addAttribute:NSFontAttributeName value:boldFont range:range];
    }
  }

  return as;
}

NSMutableAttributedString* AutocompletePopupViewMac::ElideString(
    NSMutableAttributedString* aString,
    const std::wstring originalString,
    const gfx::Font& font,
    const float width) {
  // If it already fits, nothing to be done.
  if ([aString size].width <= width) {
    return aString;
  }

  // If ElideText() decides to do nothing, nothing to be done.
  const std::wstring elided(ElideText(originalString, font, width, false));
  if (0 == elided.compare(originalString)) {
    return aString;
  }

  // If everything was elided away, clear the string.
  if (elided.size() == 0) {
    [aString deleteCharactersInRange:NSMakeRange(0, [aString length])];
    return aString;
  }

  // The ellipses should be the last character, and everything before
  // that should match the original string.
  const size_t i(elided.size() - 1);
  DCHECK(0 != elided.compare(0, i, originalString));

  // Replace the end of |aString| with the ellipses from |elided|.
  NSString* s = base::SysWideToNSString(elided.substr(i));
  [aString replaceCharactersInRange:NSMakeRange(i, [aString length] - i)
                         withString:s];

  return aString;
}

// Return the text to show for the match, based on the match's
// contents and description.  Result will be in |font|, with the
// boldfaced version used for matches.
NSAttributedString* AutocompletePopupViewMac::MatchText(
    const AutocompleteMatch& match, gfx::Font& font, float cellWidth) {
  NSMutableAttributedString *as =
      DecorateMatchedString(match.contents,
                            match.contents_class,
                            ContentTextColor(), font);

  // If there is a description, append it, separated from the contents
  // with an en dash, and decorated with a distinct color.
  if (!match.description.empty()) {
    // Make sure the current string fits w/in kMaxContentsFraction of
    // the cell to make sure the description will be at least
    // partially visible.
    // TODO(shess): Consider revising our NSCell subclass to have two
    // bits and just draw them right, rather than truncating here.
    const float textWidth = cellWidth - kTextXOffset;
    as = ElideString(as, match.contents, font,
                     textWidth * kMaxContentsFraction);

    NSDictionary* attributes =
        [NSDictionary dictionaryWithObjectsAndKeys:
             font.nativeFont(), NSFontAttributeName,
             ContentTextColor(), NSForegroundColorAttributeName,
             nil];
    NSString* rawEnDash = [NSString stringWithFormat:@" %C ", 0x2013];
    NSAttributedString* enDash =
        [[[NSAttributedString alloc] initWithString:rawEnDash
                                         attributes:attributes] autorelease];

    NSAttributedString* description =
        DecorateMatchedString(match.description, match.description_class,
                              DescriptionTextColor(), font);

    [as appendAttributedString:enDash];
    [as appendAttributedString:description];
  }

  NSMutableParagraphStyle* style =
      [[[NSMutableParagraphStyle alloc] init] autorelease];
  [style setLineBreakMode:NSLineBreakByTruncatingTail];
  [style setTighteningFactorForTruncation:0.0];
  [as addAttribute:NSParagraphStyleAttributeName value:style
             range:NSMakeRange(0, [as length])];

  return as;
}

// AutocompleteButtonCell overrides how backgrounds are displayed to
// handle hover versus selected.  So long as we're in there, it also
// provides some default initialization.

@interface AutocompleteButtonCell : NSButtonCell {
}
@end

// AutocompleteMatrix sets up a tracking area to implement hover by
// highlighting the cell the mouse is over.

@interface AutocompleteMatrix : NSMatrix {
 @private
  // Target for click and middle-click.
  AutocompletePopupViewMac* popupView_;  // weak, owns us.
}

// Create a zero-size matrix initializing |popupView_|.
- initWithPopupView:(AutocompletePopupViewMac*)popupView;

// Set |popupView_|.
- (void)setPopupView:(AutocompletePopupViewMac*)popupView;

// Return the currently highlighted row.  Returns -1 if no row is
// highlighted.
- (NSInteger)highlightedRow;

@end

AutocompletePopupViewMac::AutocompletePopupViewMac(
    AutocompleteEditViewMac* edit_view,
    AutocompleteEditModel* edit_model,
    Profile* profile,
    NSTextField* field)
    : model_(new AutocompletePopupModel(this, edit_model, profile)),
      edit_view_(edit_view),
      field_(field),
      popup_(nil) {
  DCHECK(edit_view);
  DCHECK(edit_model);
  DCHECK(profile);
  edit_model->SetPopupModel(model_.get());
}

AutocompletePopupViewMac::~AutocompletePopupViewMac() {
  // Destroy the popup model before this object is destroyed, because
  // it can call back to us in the destructor.
  model_.reset();

  // Break references to |this| because the popup may not be
  // deallocated immediately.
  AutocompleteMatrix* matrix = [popup_ contentView];
  DCHECK(matrix == nil || [matrix isKindOfClass:[AutocompleteMatrix class]]);
  [matrix setPopupView:NULL];
}

bool AutocompletePopupViewMac::IsOpen() const {
  return [popup_ isVisible] ? true : false;
}

void AutocompletePopupViewMac::CreatePopupIfNeeded() {
  if (!popup_) {
    popup_.reset([[NSWindow alloc] initWithContentRect:NSZeroRect
                                             styleMask:NSBorderlessWindowMask
                                               backing:NSBackingStoreBuffered
                                                 defer:YES]);
    [popup_ setMovableByWindowBackground:NO];
    // The window shape is determined by the content view
    // (AutocompleteMatrix).
    [popup_ setAlphaValue:1.0];
    [popup_ setOpaque:NO];
    [popup_ setBackgroundColor:[NSColor clearColor]];
    [popup_ setHasShadow:YES];
    [popup_ setLevel:NSNormalWindowLevel];

    scoped_nsobject<AutocompleteMatrix> matrix(
        [[AutocompleteMatrix alloc] initWithPopupView:this]);
    [popup_ setContentView:matrix];
  }
}

void AutocompletePopupViewMac::PositionPopup(const CGFloat matrixHeight) {
  // Calculate the popup's position on the screen.  It should abut the
  // field's visual border vertically, and be below the bounds
  // horizontally.

  // Start with the field's rect on the screen.
  NSRect popupFrame = NSInsetRect([field_ bounds], 0.0, kFieldVisualInset);
  popupFrame = [field_ convertRect:popupFrame toView:nil];
  popupFrame.origin = [[field_ window] convertBaseToScreen:popupFrame.origin];

  // Size to fit the matrix, and shift down by the size plus the top
  // window border.  Would prefer -convertSize:fromView: to
  // -userSpaceScaleFactor for the scale conversion, but until the
  // window is on-screen that doesn't work right (bug?).
  popupFrame.size.height = matrixHeight * [popup_ userSpaceScaleFactor];
  popupFrame.origin.y -= NSHeight(popupFrame) + kWindowBorderWidth;

  // Inset to account for the horizontal border drawn by the window.
  popupFrame = NSInsetRect(popupFrame, kWindowBorderWidth, 0.0);

  // Leave a gap between the popup and the field.
  popupFrame.origin.y -= kPopupFieldGap * [popup_ userSpaceScaleFactor];

  // Do nothing if the popup is already animating to the given |frame|.
  if (NSEqualRects(popupFrame, targetPopupFrame_))
    return;

  NSRect currentPopupFrame = [popup_ frame];
  targetPopupFrame_ = popupFrame;

  // Animate the frame change if the only change is that the height got smaller.
  // Otherwise, resize immediately.
  bool animate = (NSHeight(popupFrame) < NSHeight(currentPopupFrame) &&
                  NSWidth(popupFrame) == NSWidth(currentPopupFrame));
  NSTimeInterval duration =
      (animate ? kShrinkAnimationDuration: kCancelAnimationDuration);

  [NSAnimationContext beginGrouping];
  // Don't use the GTM additon for the "Steve" slowdown because this can happen
  // async from user actions and the effects could be a surprise.
  [[NSAnimationContext currentContext] setDuration:duration];
  [[popup_ animator] setFrame:popupFrame display:YES];
  [NSAnimationContext endGrouping];

  if (!IsOpen())
    [[field_ window] addChildWindow:popup_ ordered:NSWindowAbove];
}

NSImage* AutocompletePopupViewMac::ImageForMatch(
    const AutocompleteMatch& match) {
  const SkBitmap* bitmap = model_->GetSpecialIconForMatch(match);
  if (bitmap)
    return gfx::SkBitmapToNSImage(*bitmap);

  const int resource_id = match.starred ?
      IDR_OMNIBOX_STAR : AutocompleteMatch::TypeToIcon(match.type);
  return AutocompleteEditViewMac::ImageForResource(resource_id);
}

void AutocompletePopupViewMac::UpdatePopupAppearance() {
  DCHECK([NSThread isMainThread]);
  const AutocompleteResult& result = model_->result();
  if (result.empty()) {
    [[popup_ parentWindow] removeChildWindow:popup_];
    [popup_ orderOut:nil];

    // Break references to |this| because the popup may not be
    // deallocated immediately.
    AutocompleteMatrix* matrix = [popup_ contentView];
    DCHECK(matrix == nil || [matrix isKindOfClass:[AutocompleteMatrix class]]);
    [matrix setPopupView:NULL];

    popup_.reset(nil);

    return;
  }

  CreatePopupIfNeeded();

  // The popup's font is a slightly smaller version of the field's.
  NSFont* fieldFont = AutocompleteEditViewMac::GetFieldFont();
  const CGFloat resultFontSize = [fieldFont pointSize] + kEditFontAdjust;
  gfx::Font resultFont = gfx::Font::CreateFont(
      base::SysNSStringToWide([fieldFont fontName]), (int)resultFontSize);

  AutocompleteMatrix* matrix = [popup_ contentView];

  // Calculate the width of the matrix based on backing out the
  // popup's border from the width of the field.  Would prefer to use
  // [matrix convertSize:fromView:] for converting from screen size,
  // but that doesn't work until the popup is on-screen (bug?).
  const NSRect fieldRectBase = [field_ convertRect:[field_ bounds] toView:nil];
  const CGFloat popupWidth = NSWidth(fieldRectBase) - 2 * kWindowBorderWidth;
  DCHECK_GT(popupWidth, 0.0);
  const CGFloat matrixWidth = popupWidth / [popup_ userSpaceScaleFactor];

  // Load the results into the popup's matrix.
  const size_t rows = model_->result().size();
  DCHECK_GT(rows, 0U);
  [matrix renewRows:rows columns:1];
  for (size_t ii = 0; ii < rows; ++ii) {
    AutocompleteButtonCell* cell = [matrix cellAtRow:ii column:0];
    const AutocompleteMatch& match = model_->result().match_at(ii);
    [cell setImage:ImageForMatch(match)];
    [cell setAttributedTitle:MatchText(match, resultFont, matrixWidth)];
  }

  // Set the cell size to fit a line of text in the cell's font.  All
  // cells should use the same font and each should layout in one
  // line, so they should all be about the same height.
  const NSSize cellSize = [[matrix cellAtRow:0 column:0] cellSize];
  DCHECK_GT(cellSize.height, 0.0);
  const CGFloat cellHeight = cellSize.height + kCellHeightAdjust;
  [matrix setCellSize:NSMakeSize(matrixWidth, cellHeight)];

  // Update the selection before placing (and displaying) the window.
  PaintUpdatesNow();

  // Calculate the matrix size manually rather than using -sizeToCells
  // because actually resizing the matrix messed up the popup size
  // animation.
  DCHECK_EQ([matrix intercellSpacing].height, 0.0);
  PositionPopup(rows * cellHeight);
}

// This is only called by model in SetSelectedLine() after updating
// everything.  Popup should already be visible.
void AutocompletePopupViewMac::PaintUpdatesNow() {
  AutocompleteMatrix* matrix = [popup_ contentView];
  [matrix selectCellAtRow:model_->selected_line() column:0];
}

AutocompletePopupModel* AutocompletePopupViewMac::GetModel() {
  return model_.get();
}

void AutocompletePopupViewMac::OpenURLForRow(int row, bool force_background) {
  DCHECK_GE(row, 0);

  WindowOpenDisposition disposition = NEW_BACKGROUND_TAB;
  if (!force_background) {
    disposition =
        event_utils::WindowOpenDispositionFromNSEvent([NSApp currentEvent]);
  }

  // OpenURL() may close the popup, which will clear the result set
  // and, by extension, |match| and its contents.  So copy the
  // relevant strings out to make sure they stay alive until the call
  // completes.
  const AutocompleteMatch& match = model_->result().match_at(row);
  const GURL url(match.destination_url);
  std::wstring keyword;
  const bool is_keyword_hint = model_->GetKeywordForMatch(match, &keyword);
  edit_view_->OpenURL(url, disposition, match.transition, GURL(), row,
                      is_keyword_hint ? std::wstring() : keyword);
}

@implementation AutocompleteButtonCell

- init {
  self = [super init];
  if (self) {
    [self setImagePosition:NSImageLeft];
    [self setBordered:NO];
    [self setButtonType:NSRadioButton];

    // Without this highlighting messes up white areas of images.
    [self setHighlightsBy:NSNoCellMask];
  }
  return self;
}

- (NSColor*)backgroundColor {
  if ([self state] == NSOnState) {
    return SelectedBackgroundColor();
  } else if ([self isHighlighted]) {
    return HoveredBackgroundColor();
  }
  return BackgroundColor();
}

// The default NSButtonCell drawing leaves the image flush left and
// the title next to the image.  This spaces things out to line up
// with the star button and autocomplete field.
- (void)drawInteriorWithFrame:(NSRect)cellFrame inView:(NSView *)controlView {
  [[self backgroundColor] set];
  NSRectFill(cellFrame);

  // Put the image centered vertically but in a fixed column.
  NSImage* image = [self image];
  if (image) {
    NSRect imageRect = cellFrame;
    imageRect.size = [image size];
    imageRect.origin.y += kImageBaselineAdjust +
        floor((NSHeight(cellFrame) - NSHeight(imageRect)) / 2);
    imageRect.origin.x += kImageXOffset;
    [image drawInRect:imageRect
             fromRect:NSZeroRect  // Entire image
            operation:NSCompositeSourceOver
             fraction:1.0
         neverFlipped:YES];
  }

  // Adjust the title position to be lined up under the field's text.
  NSAttributedString* title = [self attributedTitle];
  if (title) {
    NSRect titleRect = cellFrame;
    titleRect.size.width -= kTextXOffset;
    titleRect.origin.x += kTextXOffset;
    [self drawTitle:title withFrame:titleRect inView:controlView];
  }
}

@end

@implementation AutocompleteMatrix



// Remove all tracking areas and initialize the one we want.  Removing
// all might be overkill, but it's unclear why there would be others
// for the popup window.
- (void)resetTrackingArea {
  for (NSTrackingArea* trackingArea in [self trackingAreas]) {
    [self removeTrackingArea:trackingArea];
  }

  // TODO(shess): Consider overriding -acceptsFirstMouse: and changing
  // NSTrackingActiveInActiveApp to NSTrackingActiveAlways.
  NSTrackingAreaOptions options = NSTrackingMouseEnteredAndExited;
  options |= NSTrackingMouseMoved;
  options |= NSTrackingActiveInActiveApp;
  options |= NSTrackingInVisibleRect;

  scoped_nsobject<NSTrackingArea> trackingArea(
      [[NSTrackingArea alloc] initWithRect:[self frame]
                                   options:options
                                     owner:self
                                  userInfo:nil]);
  [self addTrackingArea:trackingArea];
}

- (void)updateTrackingAreas {
  [self resetTrackingArea];
  [super updateTrackingAreas];
}

- initWithPopupView:(AutocompletePopupViewMac*)popupView {
  self = [super initWithFrame:NSZeroRect];
  if (self) {
    popupView_ = popupView;

    [self setCellClass:[AutocompleteButtonCell class]];

    // Cells pack with no spacing.
    [self setIntercellSpacing:NSMakeSize(0.0, 0.0)];

    [self setDrawsBackground:YES];
    [self setBackgroundColor:BackgroundColor()];
    [self renewRows:0 columns:1];
    [self setAllowsEmptySelection:YES];
    [self setMode:NSRadioModeMatrix];
    [self deselectAllCells];

    [self resetTrackingArea];
  }
  return self;
}

- (void)setPopupView:(AutocompletePopupViewMac*)popupView {
  popupView_ = popupView;
}

- (void)highlightRowAt:(NSInteger)rowIndex {
  // highlightCell will be nil if rowIndex is out of range, so no cell
  // will be highlighted.
  NSCell* highlightCell = [self cellAtRow:rowIndex column:0];

  for (NSCell* cell in [self cells]) {
    [cell setHighlighted:(cell == highlightCell)];
  }
}

- (void)highlightRowUnder:(NSEvent*)theEvent {
  NSPoint point = [self convertPoint:[theEvent locationInWindow] fromView:nil];
  NSInteger row, column;
  if ([self getRow:&row column:&column forPoint:point]) {
    [self highlightRowAt:row];
  } else {
    [self highlightRowAt:-1];
  }
}

// Callbacks from NSTrackingArea.
- (void)mouseEntered:(NSEvent*)theEvent {
  [self highlightRowUnder:theEvent];
}
- (void)mouseMoved:(NSEvent*)theEvent {
  [self highlightRowUnder:theEvent];
}
- (void)mouseExited:(NSEvent*)theEvent {
  [self highlightRowAt:-1];
}

// The tracking area events aren't forwarded during a drag, so handle
// highlighting manually for middle-click and middle-drag.
- (void)otherMouseDown:(NSEvent*)theEvent {
  if ([theEvent buttonNumber] == kMiddleButtonNumber) {
    [self highlightRowUnder:theEvent];
  }
  [super otherMouseDown:theEvent];
}
- (void)otherMouseDragged:(NSEvent*)theEvent {
  if ([theEvent buttonNumber] == kMiddleButtonNumber) {
    [self highlightRowUnder:theEvent];
  }
  [super otherMouseDragged:theEvent];
}

- (void)otherMouseUp:(NSEvent*)theEvent {
  // Only intercept middle button.
  if ([theEvent buttonNumber] != kMiddleButtonNumber) {
    [super otherMouseUp:theEvent];
    return;
  }

  // -otherMouseDragged: should always have been called at this
  // location, but make sure the user is getting the right feedback.
  [self highlightRowUnder:theEvent];

  const NSInteger highlightedRow = [self highlightedRow];
  if (highlightedRow != -1) {
    DCHECK(popupView_);
    popupView_->OpenURLForRow(highlightedRow, true);
  }
}

// Select cell under |theEvent|, returning YES if a selection is made.
- (BOOL)selectCellForEvent:(NSEvent*)theEvent {
  NSPoint point = [self convertPoint:[theEvent locationInWindow] fromView:nil];

  NSInteger row, column;
  if ([self getRow:&row column:&column forPoint:point]) {
    DCHECK_EQ(column, 0);
    [self selectCellAtRow:row column:column];
    return YES;
  }
  return NO;
}

// Track the mouse until released, keeping the cell under the mouse
// selected.  If the mouse wanders off-view, revert to the
// originally-selected cell.  If the mouse is released over a cell,
// call |popupView_| to open the row's URL.
- (void)mouseDown:(NSEvent*)theEvent {
  NSCell* selectedCell = [self selectedCell];

  // Clear any existing highlight.
  [self highlightRowAt:-1];

  do {
    if (![self selectCellForEvent:theEvent]) {
      [self selectCell:selectedCell];
    }

    const NSUInteger mask = NSLeftMouseUpMask | NSLeftMouseDraggedMask;
    theEvent = [[self window] nextEventMatchingMask:mask];
  } while ([theEvent type] == NSLeftMouseDragged);

  // Do not message |popupView_| if released outside view.
  if (![self selectCellForEvent:theEvent]) {
    [self selectCell:selectedCell];
  } else {
    const NSInteger selectedRow = [self selectedRow];
    DCHECK_GE(selectedRow, 0);

    DCHECK(popupView_);
    popupView_->OpenURLForRow(selectedRow, false);
  }
}

- (NSInteger)highlightedRow {
  NSArray* cells = [self cells];
  const NSUInteger count = [cells count];
  for(NSUInteger i = 0; i < count; ++i) {
    if ([[cells objectAtIndex:i] isHighlighted]) {
      return i;
    }
  }
  return -1;
}

- (BOOL)isOpaque {
  return NO;
}

// This handles drawing the decorations of the rounded popup window,
// calling on NSMatrix to draw the actual contents.
- (void)drawRect:(NSRect)rect {
  NSBezierPath* path =
     [NSBezierPath bezierPathWithRoundedRect:[self bounds]
                                     xRadius:kPopupRoundingRadius
                                     yRadius:kPopupRoundingRadius];

  // Draw the matrix clipped to our border.
  [NSGraphicsContext saveGraphicsState];
  [path addClip];
  [super drawRect:rect];
  [NSGraphicsContext restoreGraphicsState];
}

@end
