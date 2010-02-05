// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/views/toolbar_star_toggle.h"

#include "app/resource_bundle.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/views/browser_dialogs.h"
#include "chrome/browser/views/toolbar_view.h"
#include "googleurl/src/gurl.h"
#include "grit/theme_resources.h"

using base::TimeTicks;

// The amount of time (in milliseconds) between when the bubble closes and when
// pressing on the button again does something. Yes, this is a hackish. I tried
// many different options, all to no avail:
// . Keying off mouse activation: this didn't work as there is no way to know
//   which window receives the activation. Additionally once the mouse
//   activation occurs we have no way to tie the next mouse event to the mouse
//   activation.
// . Watching all events as we dispatch them in the MessageLoop. Mouse
//   activation isn't an observable event though.
// Ideally we could use mouse capture for this, but we can't use mouse capture
// with the bubble because it has other native windows.
static const int64 kDisallowClickMS = 40;

ToolbarStarToggle::ToolbarStarToggle(views::ButtonListener* listener,
                                     ToolbarView* host)
    : ToggleImageButton(listener),
      host_(host),
      ignore_click_(false) {
}

void ToolbarStarToggle::ShowStarBubble(const GURL& url, bool newly_bookmarked) {
  gfx::Rect bounds(host_->GetLocationStackBounds());
  gfx::Point star_location;
  views::View::ConvertPointToScreen(this, &star_location);
  // The visual center of the star is not centered within the bounds.  The star
  // has a single central pixel; there are 13 pixels on the "inside" side of it
  // (toward the location bar) and 16 on the "outside".  This means we need to
  // shift the bounds one pixel toward the location bar in order to place the
  // star's outside edge at the horizontal center.  However, even this isn't
  // good enough in RTL mode, because the InfoBubble's arrow's central pixel is
  // drawn with its left edge on the target rect center-line in both LTR and RTL
  // modes.  So in RTL mode, we need to shift the bounds one more pixel left, in
  // order to place the star's central pixel on the right side of the bounds'
  // center-line, so that the arrow's center will line up.
  //
  // TODO: If the InfoBubble used mirroring transformations maybe this could
  // become symmetric (-1 : 1).
  bounds.set_x(star_location.x() + (UILayoutIsRightToLeft() ? -2 : 1));
  bounds.set_width(width());
  browser::ShowBookmarkBubbleView(host_->GetWindow(), bounds, this,
                                  host_->profile(), url, newly_bookmarked);
}

bool ToolbarStarToggle::OnMousePressed(const views::MouseEvent& e) {
  ignore_click_ = ((TimeTicks::Now() - bubble_closed_time_).InMilliseconds() <
                   kDisallowClickMS);
  return ToggleImageButton::OnMousePressed(e);
}

void ToolbarStarToggle::OnMouseReleased(const views::MouseEvent& e,
                                        bool canceled) {
  ToggleImageButton::OnMouseReleased(e, canceled);
  ignore_click_ = false;
}

void ToolbarStarToggle::OnDragDone() {
  ToggleImageButton::OnDragDone();
  ignore_click_ = false;
}

void ToolbarStarToggle::NotifyClick(const views::Event& event) {
  if (!ignore_click_ && !browser::IsBookmarkBubbleViewShowing())
    ToggleImageButton::NotifyClick(event);
}

SkBitmap ToolbarStarToggle::GetImageToPaint() {
  if (browser::IsBookmarkBubbleViewShowing()) {
    ResourceBundle &rb = ResourceBundle::GetSharedInstance();
    return *rb.GetBitmapNamed(IDR_STARRED_P);
  }
  return ImageButton::GetImageToPaint();
}

void ToolbarStarToggle::InfoBubbleClosing(InfoBubble* info_bubble,
                                          bool closed_by_escape) {
  SchedulePaint();
  bubble_closed_time_ = TimeTicks::Now();
}

bool ToolbarStarToggle::CloseOnEscape() {
  return true;
}
