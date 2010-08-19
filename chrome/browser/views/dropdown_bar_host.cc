// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/views/dropdown_bar_host.h"

#include "app/slide_animation.h"
#include "base/keyboard_codes.h"
#include "base/scoped_handle.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/view_ids.h"
#include "chrome/browser/views/dropdown_bar_view.h"
#include "chrome/browser/views/frame/browser_view.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/browser/tab_contents/tab_contents_view.h"
#include "gfx/path.h"
#include "gfx/scrollbar_size.h"
#include "views/focus/external_focus_tracker.h"
#include "views/focus/view_storage.h"
#include "views/widget/widget.h"

#if defined(OS_LINUX)
#include "app/scoped_handle_gtk.h"
#endif

using gfx::Path;

// static
bool DropdownBarHost::disable_animations_during_testing_ = false;

////////////////////////////////////////////////////////////////////////////////
// DropdownBarHost, public:

DropdownBarHost::DropdownBarHost(BrowserView* browser_view)
    : browser_view_(browser_view),
      animation_offset_(0),
      esc_accel_target_registered_(false),
      is_visible_(false) {
}

void DropdownBarHost::Init(DropdownBarView* view) {
  view_ = view;

  // Initialize the host.
  host_.reset(CreateHost());
  host_->InitWithWidget(browser_view_->GetWidget(), gfx::Rect());
  host_->SetContentsView(view_);

  // Start listening to focus changes, so we can register and unregister our
  // own handler for Escape.
  focus_manager_ =
      views::FocusManager::GetFocusManagerForNativeView(host_->GetNativeView());
  if (focus_manager_) {
    focus_manager_->AddFocusChangeListener(this);
  } else {
    // In some cases (see bug http://crbug.com/17056) it seems we may not have
    // a focus manager.  Please reopen the bug if you hit this.
    NOTREACHED();
  }

  // Start the process of animating the opening of the widget.
  animation_.reset(new SlideAnimation(this));
}

DropdownBarHost::~DropdownBarHost() {
  focus_manager_->RemoveFocusChangeListener(this);
  focus_tracker_.reset(NULL);
}

void DropdownBarHost::Show(bool animate) {
  // Stores the currently focused view, and tracks focus changes so that we can
  // restore focus when the dropdown widget is closed.
  focus_tracker_.reset(new views::ExternalFocusTracker(view_, focus_manager_));

  if (!animate || disable_animations_during_testing_) {
    is_visible_ = true;
    animation_->Reset(1);
    AnimationProgressed(animation_.get());
  } else {
    if (!is_visible_) {
      // Don't re-start the animation.
      is_visible_ = true;
      animation_->Reset();
      animation_->Show();
    }
  }
}

void DropdownBarHost::SetFocusAndSelection() {
  view_->SetFocusAndSelection(true);
}

bool DropdownBarHost::IsAnimating() const {
  return animation_->is_animating();
}

void DropdownBarHost::Hide(bool animate) {
  if (!IsVisible())
    return;
  if (animate && !disable_animations_during_testing_) {
    animation_->Reset(1.0);
    animation_->Hide();
  } else {
    StopAnimation();
    is_visible_ = false;
    host_->Hide();
  }
}

void DropdownBarHost::StopAnimation() {
  animation_->End();
}

bool DropdownBarHost::IsVisible() const {
  return is_visible_;
}

////////////////////////////////////////////////////////////////////////////////
// DropdownBarHost, views::FocusChangeListener implementation:
void DropdownBarHost::FocusWillChange(views::View* focused_before,
                                      views::View* focused_now) {
  // First we need to determine if one or both of the views passed in are child
  // views of our view.
  bool our_view_before = focused_before && view_->IsParentOf(focused_before);
  bool our_view_now = focused_now && view_->IsParentOf(focused_now);

  // When both our_view_before and our_view_now are false, it means focus is
  // changing hands elsewhere in the application (and we shouldn't do anything).
  // Similarly, when both are true, focus is changing hands within the dropdown
  // widget (and again, we should not do anything). We therefore only need to
  // look at when we gain initial focus and when we loose it.
  if (!our_view_before && our_view_now) {
    // We are gaining focus from outside the dropdown widget so we must register
    // a handler for Escape.
    RegisterAccelerators();
  } else if (our_view_before && !our_view_now) {
    // We are losing focus to something outside our widget so we restore the
    // original handler for Escape.
    UnregisterAccelerators();
  }
}

////////////////////////////////////////////////////////////////////////////////
// DropdownBarHost, AnimationDelegate implementation:

void DropdownBarHost::AnimationProgressed(const Animation* animation) {
  // First, we calculate how many pixels to slide the widget.
  gfx::Size pref_size = view_->GetPreferredSize();
  animation_offset_ = static_cast<int>((1.0 - animation_->GetCurrentValue()) *
                                       pref_size.height());

  // This call makes sure it appears in the right location, the size and shape
  // is correct and that it slides in the right direction.
  gfx::Rect dlg_rect = GetDialogPosition(gfx::Rect());
  SetDialogPosition(dlg_rect, false);

  // Let the view know if we are animating, and at which offset to draw the
  // edges.
  view_->set_animation_offset(animation_offset_);
  view_->SchedulePaint();
}

void DropdownBarHost::AnimationEnded(const Animation* animation) {
  // Place the dropdown widget in its fully opened state.
  animation_offset_ = 0;

  if (!animation_->IsShowing()) {
    // Animation has finished closing.
    host_->Hide();
    is_visible_ = false;
  } else {
    // Animation has finished opening.
  }
}

void DropdownBarHost::GetThemePosition(gfx::Rect* bounds) {
  *bounds = GetDialogPosition(gfx::Rect());
  gfx::Rect toolbar_bounds = browser_view_->GetToolbarBounds();
  gfx::Rect tab_strip_bounds = browser_view_->GetTabStripBounds();
  bounds->Offset(-toolbar_bounds.x(), -tab_strip_bounds.y());
}

////////////////////////////////////////////////////////////////////////////////
// DropdownBarHost protected:

void DropdownBarHost::ResetFocusTracker() {
  focus_tracker_.reset(NULL);
}

void DropdownBarHost::GetWidgetBounds(gfx::Rect* bounds) {
  DCHECK(bounds);
  *bounds = browser_view_->bounds();
}

void DropdownBarHost::UpdateWindowEdges(const gfx::Rect& new_pos) {
  // |w| is used to make it easier to create the part of the polygon that curves
  // the right side of the Find window. It essentially keeps track of the
  // x-pixel position of the right-most background image inside the view.
  // TODO(finnur): Let the view tell us how to draw the curves or convert
  // this to a CustomFrameWindow.
  int w = new_pos.width() - 6;  // -6 positions us at the left edge of the
                                // rightmost background image of the view.
  int h = new_pos.height();

  // This polygon array represents the outline of the background image for the
  // window. Basically, it encompasses only the visible pixels of the
  // concatenated find_dlg_LMR_bg images (where LMR = [left | middle | right]).
  const Path::Point polygon[] = {
      {0, 0}, {0, 1}, {2, 3}, {2, h - 3}, {4, h - 1},
        {4, h}, {w+0, h},
      {w+0, h - 1}, {w+1, h - 1}, {w+3, h - 3}, {w+3, 3}, {w+6, 0}
  };

  // Find the largest x and y value in the polygon.
  int max_x = 0, max_y = 0;
  for (size_t i = 0; i < arraysize(polygon); i++) {
    max_x = std::max(max_x, static_cast<int>(polygon[i].x));
    max_y = std::max(max_y, static_cast<int>(polygon[i].y));
  }

  // We then create the polygon and use SetWindowRgn to force the window to draw
  // only within that area. This region may get reduced in size below.
  Path path(polygon, arraysize(polygon));
  ScopedRegion region(path.CreateNativeRegion());

  // Are we animating?
  if (animation_offset() > 0) {
    // The animation happens in two steps: First, we clip the window and then in
    // GetWidgetPosition we offset the window position so that it still looks
    // attached to the toolbar as it grows. We clip the window by creating a
    // rectangle region (that gradually increases as the animation progresses)
    // and find the intersection between the two regions using CombineRgn.

    // |y| shrinks as the animation progresses from the height of the view down
    // to 0 (and reverses when closing).
    int y = animation_offset();
    // |y| shrinking means the animation (visible) region gets larger. In other
    // words: the rectangle grows upward (when the widget is opening).
    Path animation_path;
    SkRect animation_rect = { SkIntToScalar(0), SkIntToScalar(y),
                              SkIntToScalar(max_x), SkIntToScalar(max_y) };
    animation_path.addRect(animation_rect);
    ScopedRegion animation_region(animation_path.CreateNativeRegion());
    region.Set(Path::IntersectRegions(animation_region.Get(), region.Get()));

    // Next, we need to increase the region a little bit to account for the
    // curved edges that the view will draw to make it look like grows out of
    // the toolbar.
    Path::Point left_curve[] = {
      {0, y+0}, {0, y+1}, {2, y+3}, {2, y+0}, {0, y+0}
    };
    Path::Point right_curve[] = {
      {w+3, y+3}, {w+6, y+0}, {w+3, y+0}, {w+3, y+3}
    };

    // Combine the region for the curve on the left with our main region.
    Path left_path(left_curve, arraysize(left_curve));
    ScopedRegion r(left_path.CreateNativeRegion());
    region.Set(Path::CombineRegions(r.Get(), region.Get()));

    // Combine the region for the curve on the right with our main region.
    Path right_path(right_curve, arraysize(right_curve));
    region.Set(Path::CombineRegions(r.Get(), region.Get()));
  }

  // Now see if we need to truncate the region because parts of it obscures
  // the main window border.
  gfx::Rect widget_bounds;
  GetWidgetBounds(&widget_bounds);

  // Calculate how much our current position overlaps our boundaries. If we
  // overlap, it means we have too little space to draw the whole widget and
  // we allow overwriting the scrollbar before we start truncating our widget.
  //
  // TODO(brettw) this constant is evil. This is the amount of room we've added
  // to the window size, when we set the region, it can change the size.
  static const int kAddedWidth = 7;
  int difference = new_pos.right() - kAddedWidth - widget_bounds.right() -
      gfx::scrollbar_size() + 1;
  if (difference > 0) {
    Path::Point exclude[4];
    exclude[0].x = max_x - difference;  // Top left corner.
    exclude[0].y = 0;

    exclude[1].x = max_x;               // Top right corner.
    exclude[1].y = 0;

    exclude[2].x = max_x;               // Bottom right corner.
    exclude[2].y = max_y;

    exclude[3].x = max_x - difference;  // Bottom left corner.
    exclude[3].y = max_y;

    // Subtract this region from the original region.
    gfx::Path exclude_path(exclude, arraysize(exclude));
    ScopedRegion exclude_region(exclude_path.CreateNativeRegion());
    region.Set(Path::SubtractRegion(region.Get(), exclude_region.Get()));
  }

  // Window takes ownership of the region.
  host()->SetShape(region.release());
}

void DropdownBarHost::RegisterAccelerators() {
  DCHECK(!esc_accel_target_registered_);
  views::Accelerator escape(base::VKEY_ESCAPE, false, false, false);
  focus_manager_->RegisterAccelerator(escape, this);
  esc_accel_target_registered_ = true;
}

void DropdownBarHost::UnregisterAccelerators() {
  DCHECK(esc_accel_target_registered_);
  views::Accelerator escape(base::VKEY_ESCAPE, false, false, false);
  focus_manager_->UnregisterAccelerator(escape, this);
  esc_accel_target_registered_ = false;
}
