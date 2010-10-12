// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/views/menu_locator.h"

#include "base/logging.h"
#include "chrome/browser/chromeos/views/domui_menu_widget.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/insets.h"
#include "views/screen.h"
#include "views/widget/widget.h"

namespace {

using chromeos::DOMUIMenuWidget;

// Menu's corner radious.
const int kMenuCornerRadius = 3;
const int kSubmenuOverlapPx = 1;

gfx::Rect GetBoundsOf(const views::Widget* widget) {
  gfx::Rect bounds;
  widget->GetBounds(&bounds, false);
  return bounds;
}

// Returns the Rect of the screen that contains the point (x, y).
gfx::Rect GetScreenRectAt(int x, int y) {
  return views::Screen::GetMonitorAreaNearestPoint(gfx::Point(x, y));
}

// MenuLocator for dropdown menu.
class DropDownMenuLocator : public chromeos::MenuLocator {
 public:
  explicit DropDownMenuLocator(const gfx::Point& origin)
      : origin_(origin) {
  }

 private:
  virtual SubmenuDirection GetSubmenuDirection() const {
    return DEFAULT;
  }

  virtual void Move(DOMUIMenuWidget* widget) {
    gfx::Rect bounds;
    widget->GetBounds(&bounds, false);
    widget->SetBounds(ComputeBounds(bounds.size()));
  }

  virtual void SetBounds(DOMUIMenuWidget* widget, const gfx::Size& size) {
    gfx::Size new_size(size);
    new_size.Enlarge(0, kMenuCornerRadius);
    widget->SetBounds(ComputeBounds(new_size));
  }

  gfx::Rect ComputeBounds(const gfx::Size& size) {
    gfx::Rect screen_rect = GetScreenRectAt(origin_.x(), origin_.y());
    int width = std::min(screen_rect.width(), size.width());
    // TODO(oshima):
    // Menu has to be shown above the button, which is not currently
    // possible with Menu2. I'll update Menu2 and this code
    // once this change is landed.
    // This needs to tell menu to show scroll button when necessary.
    int x = origin_.x() - width;
    int y = origin_.y();
    if (x + width > screen_rect.right())
      x = screen_rect.right() - width;
    if (y + size.height() > screen_rect.bottom())
      y = screen_rect.bottom() - size.height();
    return gfx::Rect(x, y, width, size.height());
  }

  virtual void GetInsets(gfx::Insets* insets) const {
    insets->Set(0, 0, kMenuCornerRadius, 0);
  }

  virtual const SkScalar* GetCorners() const {
    static const SkScalar corners[] = {
      0, 0,
      0, 0,
      kMenuCornerRadius, kMenuCornerRadius,
      kMenuCornerRadius, kMenuCornerRadius,
    };
    return corners;
  }

  gfx::Point origin_;

  DISALLOW_COPY_AND_ASSIGN(DropDownMenuLocator);
};

// MenuLocator for context menu.
class ContextMenuLocator : public chromeos::MenuLocator {
 public:
  explicit ContextMenuLocator(const gfx::Point& origin)
      : origin_(origin) {
  }

 private:
  virtual SubmenuDirection GetSubmenuDirection() const {
    return DEFAULT;
  }

  virtual void Move(DOMUIMenuWidget* widget) {
    gfx::Rect bounds;
    widget->GetBounds(&bounds, false);
    UpdateBounds(widget, bounds.size());
  }

  virtual void SetBounds(DOMUIMenuWidget* widget, const gfx::Size& size) {
    gfx::Size new_size(size);
    new_size.Enlarge(0, kMenuCornerRadius * 2);
    UpdateBounds(widget, new_size);
  }

  void UpdateBounds(DOMUIMenuWidget* widget, const gfx::Size& size) {
    gfx::Rect screen_rect = GetScreenRectAt(origin_.x(), origin_.y());
    int width = std::min(screen_rect.width(), size.width());
    int height = size.height();
    // TODO(oshima): Locator needs a preferred size so that
    // 1) we can tell height == screen_rect is the result of
    //    locator resizing it, or preferred size happens to be
    //    same hight of the screen (which is rare).
    // 2) when the menu is moved to place where it has more space, it can
    //    hide the scrollbar again. (which won't happen on chromeos now)
    if (height >= screen_rect.height()) {
      height = screen_rect.height();
      widget->EnableScroll(true);
    } else {
      widget->EnableScroll(false);
    }

    int x = origin_.x();
    int y = origin_.y();
    if (x + width > screen_rect.right())
      x = screen_rect.right() - width;
    if (y + height > screen_rect.bottom())
      y = screen_rect.bottom() - height;
    widget->SetBounds(gfx::Rect(x, y, width, height));
  }

  virtual const SkScalar* GetCorners() const {
    static const SkScalar corners[] = {
      kMenuCornerRadius, kMenuCornerRadius,
      kMenuCornerRadius, kMenuCornerRadius,
      kMenuCornerRadius, kMenuCornerRadius,
      kMenuCornerRadius, kMenuCornerRadius,
    };
    return corners;
  }

  virtual void GetInsets(gfx::Insets* insets) const {
    insets->Set(kMenuCornerRadius, 0, kMenuCornerRadius, 0);
  }

  gfx::Point origin_;

  DISALLOW_COPY_AND_ASSIGN(ContextMenuLocator);
};

// MenuLocator for submenu.
class SubMenuLocator : public chromeos::MenuLocator {
 public:
  SubMenuLocator(const DOMUIMenuWidget* parent,
                 MenuLocator::SubmenuDirection parent_direction,
                 int y)
      : parent_rect_(GetBoundsOf(parent)),
        parent_direction_(parent_direction),
        root_y_(parent_rect_.y() + y),
        corners_(NULL),
        direction_(DEFAULT) {
  }

 private:
  virtual SubmenuDirection GetSubmenuDirection() const {
    return direction_;
  }

  virtual void Move(DOMUIMenuWidget* widget) {
    gfx::Rect bounds;
    widget->GetBounds(&bounds, false);
    UpdateBounds(widget, bounds.size());
  }

  virtual void SetBounds(DOMUIMenuWidget* widget, const gfx::Size& size) {
    gfx::Size new_size(size);
    new_size.Enlarge(0, kMenuCornerRadius * 2);
    UpdateBounds(widget, new_size);
  }

  virtual const SkScalar* GetCorners() const {
    return corners_;
  }

  virtual void GetInsets(gfx::Insets* insets) const {
    insets->Set(kMenuCornerRadius, 0, kMenuCornerRadius, 0);
  }

  // Rounded corner definitions for right/left attached submenu.
  static const SkScalar kRightCorners[];
  static const SkScalar kLeftCorners[];

  void UpdateBounds(DOMUIMenuWidget* widget, const gfx::Size& size) {
    gfx::Rect screen_rect = GetScreenRectAt(parent_rect_.x(), root_y_);
    int width = std::min(screen_rect.width(), size.width());
    int height = size.height();
    if (height >= screen_rect.height()) {
      height = screen_rect.height();
      widget->EnableScroll(true);
    } else {
      widget->EnableScroll(false);
    }

    SubmenuDirection direction = parent_direction_;
    if (direction == DEFAULT)
      direction = RIGHT; // TOOD(oshima): support RTL
    // Adjust Y to fit the screen.
    int y = root_y_;
    if (root_y_ + height > screen_rect.bottom())
      y = screen_rect.bottom() - height;
    // Determine the attachment.
    // TODO(oshima):
    // Come up with better placement when menu is wide,
    // probably limit max width and let each menu scroll
    // horizontally when selected.
    int x = direction == RIGHT ?
        ComputeXToRight(screen_rect, width) :
        ComputeXToLeft(screen_rect, width);
    corners_ = direction_ == RIGHT ? kRightCorners : kLeftCorners;
    widget->SetBounds(gfx::Rect(x, y, width, height));
  }

  int ComputeXToRight(const gfx::Rect& screen_rect, int width) {
    if (parent_rect_.right() + width > screen_rect.right()) {
      if (parent_rect_.x() - width < screen_rect.x()) {
        // Place on the right to fit to the screen if no space on left
        direction_ = RIGHT;
        return screen_rect.right() - width;
      }
      direction_ = LEFT;
      return parent_rect_.x() - width + kSubmenuOverlapPx;
    } else {
      direction_ = RIGHT;
      return parent_rect_.right() - kSubmenuOverlapPx;
    }
  }

  int ComputeXToLeft(const gfx::Rect& screen_rect, int width) {
    if (parent_rect_.x() - width < screen_rect.x()) {
      if (parent_rect_.right() + width > screen_rect.right()) {
        // no space on right
        direction_ = LEFT;
        return parent_rect_.x();
      }
      corners_ = kRightCorners;
      direction_ = RIGHT;
      return parent_rect_.right() - kSubmenuOverlapPx;
    } else {
      corners_ = kLeftCorners;
      direction_ = LEFT;
      return parent_rect_.x() - width + kSubmenuOverlapPx;
    }
  }

  const gfx::Rect parent_rect_;

  const MenuLocator::SubmenuDirection parent_direction_;

  const int root_y_;

  SkScalar const* corners_;

  // The direction the this menu is attached to its parent. Submenu may still
  // choose different direction if there is no spece for that direction
  // (2nd turnaround).
  SubmenuDirection direction_;

  DISALLOW_COPY_AND_ASSIGN(SubMenuLocator);
};

// Rounded corners of the submenu attached to right side.
const SkScalar SubMenuLocator::kRightCorners[] = {
  0, 0,
  kMenuCornerRadius, kMenuCornerRadius,
  kMenuCornerRadius, kMenuCornerRadius,
  kMenuCornerRadius, kMenuCornerRadius,
};

// Rounded corners of the submenu attached to left side.
const SkScalar SubMenuLocator::kLeftCorners[] = {
  kMenuCornerRadius, kMenuCornerRadius,
  0, 0,
  kMenuCornerRadius, kMenuCornerRadius,
  kMenuCornerRadius, kMenuCornerRadius,
};


}  // namespace

namespace chromeos {

// static
MenuLocator* MenuLocator::CreateDropDownMenuLocator(const gfx::Point& p) {
  return new DropDownMenuLocator(p);
}

MenuLocator* MenuLocator::CreateContextMenuLocator(const gfx::Point& p) {
  return new ContextMenuLocator(p);
}

MenuLocator* MenuLocator::CreateSubMenuLocator(
    const DOMUIMenuWidget* parent,
    MenuLocator::SubmenuDirection parent_direction,
    int y) {
  return new SubMenuLocator(parent, parent_direction, y);
}

}  // namespace chromeos
