// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VIEWS_DETACHABLE_TOOLBAR_VIEW_H_
#define CHROME_BROWSER_VIEWS_DETACHABLE_TOOLBAR_VIEW_H_

#include "views/view.h"

class SkBitmap;
struct SkRect;

// DetachableToolbarView contains functionality common to views that can detach
// from the Chrome frame, such as the BookmarkBarView and the Extension shelf.
class DetachableToolbarView : public views::View {
 public:
  // The color gradient start value close to the edge of the divider.
  static const SkColor kEdgeDividerColor;
  // The color gradient value for the middle of the divider.
  static const SkColor kMiddleDividerColor;

  DetachableToolbarView() {}
  virtual ~DetachableToolbarView() {}

  // Whether the view is currently detached from the Chrome frame.
  virtual bool IsDetached() const = 0;

  // Whether the shelf/bar is above the page or below it.
  virtual bool IsOnTop() const = 0;

  // Gets the current state of the resize animation (show/hide).
  virtual double GetAnimationValue() const = 0;

  // Paint the background (including the theme image behind content area) when
  // in bar/shelf is detached from the Chrome frame.
  static void PaintBackgroundDetachedMode(gfx::Canvas* canvas,
                                          views::View* view);

  // Paint the background (including the theme image behind content area) when
  // in bar/shelf is attached to the Chrome frame.
  static void PaintBackgroundAttachedMode(gfx::Canvas* canvas,
                                          views::View* view);

  // Calculate the rect for the content area of the bar/shelf. This is only
  // needed when the bar/shelf is detached from the Chrome frame (otherwise the
  // content area is the whole area of the bar/shelf. When detached, however,
  // only a small round rectangle is for drawing our content on. This calculates
  // how big this area is, where it is located within the shelf and how round
  // the edges should be.
  static void CalculateContentArea(double animation_state,
                                   double horizontal_padding,
                                   double vertical_padding,
                                   SkRect* rect,
                                   double* roundness,
                                   views::View* view);

  // Paint the horizontal border separating the shelf/bar from the page content.
  static void PaintHorizontalBorder(gfx::Canvas* canvas,
                                    DetachableToolbarView* view);

  // Paint the background of the content area (the surface behind the
  // bookmarks or extension toolstrips). |rect| is the rectangle to paint
  // the background within. |roundness| describes the roundness of the corners.
  static void PaintContentAreaBackground(gfx::Canvas* canvas,
                                         ThemeProvider* theme_provider,
                                         const SkRect& rect,
                                         double roundness);
  // Paint the border around the content area (when in detached mode).
  static void PaintContentAreaBorder(gfx::Canvas* canvas,
                                     ThemeProvider* theme_provider,
                                     const SkRect& rect,
                                     double roundness);

  // Paint a themed gradient divider at location |x|. The color of the divider
  // is a gradient starting with |top_color| at the top, and changing into
  // |middle_color| and then over to |bottom_color| as you go further down.
  static void PaintVerticalDivider(gfx::Canvas* canvas,
                                   int x,
                                   int height,
                                   int vertical_padding,
                                   const SkColor& top_color,
                                   const SkColor& middle_color,
                                   const SkColor& bottom_color);

  // Paint the theme background with the proper alignment.
  static void PaintThemeBackgroundTopAligned(gfx::Canvas* canvas,
                                             SkBitmap* ntp_background,
                                             int tiling,
                                             int alignment,
                                             int width,
                                             int height);
  static void PaintThemeBackgroundBottomAligned(gfx::Canvas* canvas,
                                                SkBitmap* ntp_background,
                                                int tiling,
                                                int alignment,
                                                int width,
                                                int height,
                                                int browser_height);
 private:
  DISALLOW_COPY_AND_ASSIGN(DetachableToolbarView);
};

#endif  // CHROME_BROWSER_VIEWS_DETACHABLE_TOOLBAR_VIEW_H_
