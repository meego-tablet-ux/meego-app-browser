// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/cocoa/bookmark_bar_toolbar_view.h"

#include "app/gfx/canvas_paint.h"
#include "app/theme_provider.h"
#include "base/gfx/rect.h"
#include "chrome/browser/browser_theme_provider.h"
#import "chrome/browser/cocoa/browser_window_controller.h"
#import "chrome/browser/cocoa/bookmark_bar_constants.h"
#import "chrome/browser/cocoa/bookmark_bar_controller.h"
#include "chrome/browser/ntp_background_util.h"
#import "third_party/GTM/AppKit/GTMTheme.h"

const CGFloat kBorderRadius = 3.0;

@interface BookmarkBarToolbarView (Private)
- (void)drawRectAsBubble:(NSRect)rect;
@end

@implementation BookmarkBarToolbarView

- (BOOL)isOpaque {
  return [controller_ isInState:bookmarks::kDetachedState];
}

- (void)drawRect:(NSRect)rect {
  if ([controller_ isInState:bookmarks::kDetachedState] ||
      [controller_ isAnimatingToState:bookmarks::kDetachedState] ||
      [controller_ isAnimatingFromState:bookmarks::kDetachedState]) {
    [self drawRectAsBubble:rect];
  } else {
    NSPoint phase = [self gtm_themePatternPhase];
    [[NSGraphicsContext currentContext] setPatternPhase:phase];
    [self drawBackground];
  }
}

- (void)drawRectAsBubble:(NSRect)rect {
  // The state of our morph; 1 is total bubble, 0 is the regular bar. We use it
  // to morph the bubble to a regular bar (shape and colour).
  CGFloat morph = [controller_ detachedMorphProgress];

  NSRect bounds = [self bounds];

  ThemeProvider* themeProvider = [controller_ themeProvider];
  if (!themeProvider)
    return;

  NSGraphicsContext* context = [NSGraphicsContext currentContext];
  [context saveGraphicsState];

  // Draw the background.
  {
    // CanvasPaint draws to the NSGraphicsContext during its destructor, so
    // explicitly scope this.
    //
    // Paint the entire bookmark bar, even if the damage rect is much smaller
    // because PaintBackgroundDetachedMode() assumes that area's origin is
    // (0, 0) and that its size is the size of the bookmark bar.
    //
    // In practice, this sounds worse than it is because redraw time is still
    // minimal compared to the pause between frames of animations. We were
    // already repainting the rest of the bookmark bar below without setting a
    // clip area, anyway. Also, the only time we weren't asked to redraw the
    // whole bookmark bar is when the find bar is drawn over it.
    gfx::CanvasPaint canvas(bounds, true);
    gfx::Rect area(0, 0, NSWidth(bounds), NSHeight(bounds));

    NtpBackgroundUtil::PaintBackgroundDetachedMode(themeProvider, &canvas,
        area, [controller_ currentTabContentsHeight]);
  }

  // Draw our bookmark bar border on top of the background.
  NSRect frameRect =
      NSMakeRect(
          morph * bookmarks::kNTPBookmarkBarPadding,
          morph * bookmarks::kNTPBookmarkBarPadding,
          NSWidth(bounds) - 2 * morph * bookmarks::kNTPBookmarkBarPadding,
          NSHeight(bounds) - 2 * morph * bookmarks::kNTPBookmarkBarPadding);
  // Now draw a bezier path with rounded rectangles around the area.
  frameRect = NSInsetRect(frameRect, morph * 0.5, morph * 0.5);
  NSBezierPath* border =
      [NSBezierPath bezierPathWithRoundedRect:frameRect
                                      xRadius:(morph * kBorderRadius)
                                      yRadius:(morph * kBorderRadius)];

  // Draw the rounded rectangle.
  NSColor* toolbarColor =
      [[self gtm_theme] backgroundColorForStyle:GTMThemeStyleToolBar
                                          state:GTMThemeStateActiveWindow];
  // workaround for default theme
  // TODO(alcor) next GTM update return nil for background color if not set;
  // http://crbug.com/25196
  if ([toolbarColor isEqual:[NSColor colorWithCalibratedWhite:0.5 alpha:1.0]])
    toolbarColor = nil;
  if (!toolbarColor)
    toolbarColor = [NSColor colorWithCalibratedWhite:0.9 alpha:1.0];
  [[toolbarColor colorWithAlphaComponent:morph] set];  // Set with opacity.
  [border fill];

  // Fade in/out the background.
  [context saveGraphicsState];
  [border setClip];
  CGContextRef cgContext = (CGContextRef)[context graphicsPort];
  CGContextBeginTransparencyLayer(cgContext, NULL);
  CGContextSetAlpha(cgContext, 1 - morph);
  [context setPatternPhase:[self gtm_themePatternPhase]];
  [self drawBackground];
  CGContextEndTransparencyLayer(cgContext);
  [context restoreGraphicsState];

  // Draw the border of the rounded rectangle.
  NSColor* borderColor =
      [[self gtm_theme] strokeColorForStyle:GTMThemeStyleToolBarButton
                                      state:GTMThemeStateActiveWindow];
  [[borderColor colorWithAlphaComponent:morph] set];  // Set with opacity.
  [border stroke];

  // Fade in/out the divider.
  // TODO(viettrungluu): It's not obvious that this divider lines up exactly
  // with |BackgroundGradientView|'s (in fact, it probably doesn't).
  [[[self strokeColor] colorWithAlphaComponent:(1 - morph)] set];
  NSBezierPath* divider = [NSBezierPath bezierPath];
  NSPoint dividerStart =
      NSMakePoint(morph * bookmarks::kNTPBookmarkBarPadding + morph * 0.5,
                  morph * bookmarks::kNTPBookmarkBarPadding + morph * 0.5);
  CGFloat dividerWidth =
      NSWidth(bounds) - 2 * morph * bookmarks::kNTPBookmarkBarPadding - 2 * 0.5;
  [divider moveToPoint:dividerStart];
  [divider relativeLineToPoint:NSMakePoint(dividerWidth, 0)];
  [divider stroke];

  // Restore the graphics context.
  [context restoreGraphicsState];
}

@end  // @implementation BookmarkBarToolbarView
