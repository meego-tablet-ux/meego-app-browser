// Copyright 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <windows.h>
#include <uxtheme.h>
#include <vsstyle.h>

#include "chrome/views/client_view.h"

#include "base/gfx/native_theme.h"
#include "chrome/browser/standard_layout.h"
#include "chrome/common/gfx/chrome_canvas.h"
#include "chrome/common/gfx/chrome_font.h"
#include "chrome/common/l10n_util.h"
#include "chrome/common/resource_bundle.h"
#include "chrome/common/win_util.h"
#include "chrome/views/window.h"
#include "generated_resources.h"

namespace {

// Updates any of the standard buttons according to the delegate.
void UpdateButtonHelper(ChromeViews::NativeButton* button_view,
                        ChromeViews::DialogDelegate* delegate,
                        ChromeViews::DialogDelegate::DialogButton button) {
  std::wstring label = delegate->GetDialogButtonLabel(button);
  if (!label.empty())
    button_view->SetLabel(label);
  button_view->SetEnabled(delegate->IsDialogButtonEnabled(button));
  button_view->SetVisible(delegate->IsDialogButtonVisible(button));
}

}  // namespace

namespace ChromeViews {

// static
ChromeFont ClientView::dialog_button_font_;
static const int kDialogMinButtonWidth = 75;
static const int kDialogButtonLabelSpacing = 16;
static const int kDialogButtonContentSpacing = 0;

// The group used by the buttons.  This name is chosen voluntarily big not to
// conflict with other groups that could be in the dialog content.
static const int kButtonGroup = 6666;

namespace {

// DialogButton ----------------------------------------------------------------

// DialogButtons is used for the ok/cancel buttons of the window. DialogButton
// forwrds AcceleratorPressed to the delegate.

class DialogButton : public NativeButton {
 public:
  DialogButton(Window* owner,
               DialogDelegate::DialogButton type,
               const std::wstring& title,
               bool is_default)
      : NativeButton(title, is_default), owner_(owner), type_(type) {
  }

  // Overriden to forward to the delegate.
  virtual bool AcceleratorPressed(const Accelerator& accelerator) {
    if (!owner_->window_delegate()->AsDialogDelegate()->
        AreAcceleratorsEnabled(type_)) {
      return false;
    }
    return NativeButton::AcceleratorPressed(accelerator);
  }

 private:
  Window* owner_;
  const DialogDelegate::DialogButton type_;

  DISALLOW_EVIL_CONSTRUCTORS(DialogButton);
};

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// ClientView, public:
ClientView::ClientView(Window* owner, View* contents_view)
    : ok_button_(NULL),
      cancel_button_(NULL),
      extra_view_(NULL),
      owner_(owner),
      contents_view_(contents_view) {
  DCHECK(owner_);
  InitClass();
}

ClientView::~ClientView() {
}

void ClientView::ShowDialogButtons() {
  if (!owner_->window_delegate())
    return;

  DialogDelegate* dd = owner_->window_delegate()->AsDialogDelegate();
  if (!dd)
    return;

  int buttons = dd->GetDialogButtons();

  if (buttons & DialogDelegate::DIALOGBUTTON_OK && !ok_button_) {
    std::wstring label =
        dd->GetDialogButtonLabel(DialogDelegate::DIALOGBUTTON_OK);
    if (label.empty())
      label = l10n_util::GetString(IDS_OK);
    ok_button_ = new DialogButton(
        owner_, DialogDelegate::DIALOGBUTTON_OK,
        label,
        (dd->GetDefaultDialogButton() & DialogDelegate::DIALOGBUTTON_OK) != 0);
    ok_button_->SetListener(this);
    ok_button_->SetGroup(kButtonGroup);
    if (!cancel_button_)
      ok_button_->AddAccelerator(Accelerator(VK_ESCAPE, false, false, false));
    AddChildView(ok_button_);
  }
  if (buttons & DialogDelegate::DIALOGBUTTON_CANCEL && !cancel_button_) {
    std::wstring label =
        dd->GetDialogButtonLabel(DialogDelegate::DIALOGBUTTON_CANCEL);
    if (label.empty()) {
      if (buttons & DialogDelegate::DIALOGBUTTON_OK) {
        label = l10n_util::GetString(IDS_CANCEL);
      } else {
        label = l10n_util::GetString(IDS_CLOSE);
      }
    }
    cancel_button_ = new DialogButton(
        owner_, DialogDelegate::DIALOGBUTTON_CANCEL,
        label,
        (dd->GetDefaultDialogButton() & DialogDelegate::DIALOGBUTTON_CANCEL)
        != 0);
    cancel_button_->SetListener(this);
    cancel_button_->SetGroup(kButtonGroup);
    cancel_button_->AddAccelerator(Accelerator(VK_ESCAPE, false, false, false));
    AddChildView(cancel_button_);
  }

  ChromeViews::View* extra_view = dd->GetExtraView();
  if (extra_view && !extra_view_) {
    extra_view_ = extra_view;
    extra_view_->SetGroup(kButtonGroup);
    AddChildView(extra_view_);
  }
  if (!buttons) {
    // Register the escape key as an accelerator which will close the window
    // if there are no dialog buttons.
    AddAccelerator(Accelerator(VK_ESCAPE, false, false, false));
  }
}

// Changing dialog labels will change button widths.
void ClientView::UpdateDialogButtons() {
  if (!owner_->window_delegate())
    return;

  DialogDelegate* dd = owner_->window_delegate()->AsDialogDelegate();
  if (!dd)
    return;

  int buttons = dd->GetDialogButtons();

  if (buttons & DialogDelegate::DIALOGBUTTON_OK)
    UpdateButtonHelper(ok_button_, dd, DialogDelegate::DIALOGBUTTON_OK);

  if (buttons & DialogDelegate::DIALOGBUTTON_CANCEL)
    UpdateButtonHelper(cancel_button_, dd, DialogDelegate::DIALOGBUTTON_CANCEL);

  LayoutDialogButtons();
  SchedulePaint();
}

bool ClientView::PointIsInSizeBox(const gfx::Point& point) {
  CPoint temp = point.ToPOINT();
  View::ConvertPointFromViewContainer(this, &temp);
  return size_box_bounds_.Contains(temp.x, temp.y);
}

////////////////////////////////////////////////////////////////////////////////
// ClientView, View overrides:

static void FillViewWithSysColor(ChromeCanvas* canvas, View* view,
                                 COLORREF color) {
  SkColor sk_color =
      SkColorSetRGB(GetRValue(color), GetGValue(color), GetBValue(color));
  canvas->FillRectInt(sk_color, 0, 0, view->GetWidth(), view->GetHeight());
}

void ClientView::Paint(ChromeCanvas* canvas) {
  if (!owner_->window_delegate())
    return;

  if (owner_->window_delegate()->AsDialogDelegate()) {
    FillViewWithSysColor(canvas, this, GetSysColor(COLOR_3DFACE));
  } else {
    // TODO(beng): (Cleanup) this should be COLOR_WINDOW but until the App
    //             Install wizard is updated to use the DialogDelegate somehow,
    //             we'll just use this value here.
    FillViewWithSysColor(canvas, this, GetSysColor(COLOR_3DFACE));
  }
}

void ClientView::PaintChildren(ChromeCanvas* canvas) {
  View::PaintChildren(canvas);
  if (!owner_->IsMaximized() && !owner_->IsMinimized())
    PaintSizeBox(canvas);
}

void ClientView::Layout() {
  if (has_dialog_buttons())
    LayoutDialogButtons();
  LayoutContentsView();
}

void ClientView::ViewHierarchyChanged(bool is_add, View* parent, View* child) {
  if (is_add && child == this) {
    // Only add the contents_view_ once, and only when we ourselves are added
    // to the view hierarchy, since some contents_view_s assume that when they
    // are added to the hierarchy a HWND exists, when it may not, since we are
    // not yet added...
    if (contents_view_ && contents_view_->GetParent() != this)
      AddChildView(contents_view_);
    // Can only add and update the dialog buttons _after_ they are added to the
    // view hierarchy since they are native controls and require the
    // ViewContainer's HWND.
    ShowDialogButtons();
    UpdateDialogButtons();
    Layout();
  }
}

void ClientView::DidChangeBounds(const CRect& prev, const CRect& next) {
  Layout();
}

void ClientView::GetPreferredSize(CSize* out) {
  DCHECK(out);
  contents_view_->GetPreferredSize(out);
  int button_height = 0;
  if (has_dialog_buttons()) {
    if (cancel_button_)
      button_height = cancel_button_->GetHeight();
    else
      button_height = ok_button_->GetHeight();
    // Account for padding above and below the button.
    button_height += kDialogButtonContentSpacing + kButtonVEdgeMargin;
  }
  out->cy += button_height;
}

bool ClientView::AcceleratorPressed(const Accelerator& accelerator) {
  DCHECK(accelerator.GetKeyCode() == VK_ESCAPE);  // We only expect Escape key.
  owner_->Close();
  return true;
}

////////////////////////////////////////////////////////////////////////////////
// ClientView, NativeButton::Listener implementation:

void ClientView::ButtonPressed(NativeButton* sender) {
  if (sender == ok_button_) {
    owner_->AcceptWindow();
  } else if (sender == cancel_button_) {
    owner_->CancelWindow();
  }
}

////////////////////////////////////////////////////////////////////////////////
// ClientView, private:

void ClientView::PaintSizeBox(ChromeCanvas* canvas) {
  if (!owner_->window_delegate())
    return;

  if (owner_->window_delegate()->CanResize() ||
      owner_->window_delegate()->CanMaximize()) {
    HDC dc = canvas->beginPlatformPaint();
    SIZE gripper_size = { 0, 0 };
    gfx::NativeTheme::instance()->GetThemePartSize(
        gfx::NativeTheme::STATUS, dc, SP_GRIPPER, 1, NULL, TS_TRUE,
        &gripper_size);

    // TODO(beng): (http://b/1085509) In "classic" rendering mode, there isn't
    //             a theme-supplied gripper. We should probably improvise
    //             something, which would also require changing |gripper_size|
    //             to have different default values, too...
    CRect gripper_bounds;
    GetLocalBounds(&gripper_bounds, false);
    gripper_bounds.left = gripper_bounds.right - gripper_size.cx;
    gripper_bounds.top = gripper_bounds.bottom - gripper_size.cy;
    size_box_bounds_ = gripper_bounds;
    gfx::NativeTheme::instance()->PaintStatusGripper(
        dc, SP_PANE, 1, 0, gripper_bounds);
    canvas->endPlatformPaint();
  }
}

int ClientView::GetButtonWidth(DialogDelegate::DialogButton button) {
  if (!owner_->window_delegate())
    return kDialogMinButtonWidth;

  DialogDelegate* dd = owner_->window_delegate()->AsDialogDelegate();
  DCHECK(dd);

  std::wstring button_label = dd->GetDialogButtonLabel(button);
  int string_width = dialog_button_font_.GetStringWidth(button_label);
  return std::max(string_width + kDialogButtonLabelSpacing,
                  kDialogMinButtonWidth);
}

void ClientView::LayoutDialogButtons() {
  CRect extra_bounds;
  if (cancel_button_) {
    CSize ps;
    cancel_button_->GetPreferredSize(&ps);
    CRect lb;
    GetLocalBounds(&lb, false);
    int button_width = GetButtonWidth(DialogDelegate::DIALOGBUTTON_CANCEL);
    CRect bounds;
    bounds.left = lb.right - button_width - kButtonHEdgeMargin;
    bounds.top = lb.bottom - ps.cy - kButtonVEdgeMargin;
    bounds.right = bounds.left + button_width;
    bounds.bottom = bounds.top + ps.cy;
    cancel_button_->SetBounds(bounds);
    // The extra view bounds are dependent on this button.
    extra_bounds.right = bounds.left;
    extra_bounds.top = bounds.top;
  }
  if (ok_button_) {
    CSize ps;
    ok_button_->GetPreferredSize(&ps);
    CRect lb;
    GetLocalBounds(&lb, false);
    int button_width = GetButtonWidth(DialogDelegate::DIALOGBUTTON_OK);
    int ok_button_right = lb.right - kButtonHEdgeMargin;
    if (cancel_button_)
      ok_button_right = cancel_button_->GetX() - kRelatedButtonHSpacing;
    CRect bounds;
    bounds.left = ok_button_right - button_width;
    bounds.top = lb.bottom - ps.cy - kButtonVEdgeMargin;
    bounds.right = ok_button_right;
    bounds.bottom = bounds.top + ps.cy;
    ok_button_->SetBounds(bounds);
    // The extra view bounds are dependent on this button.
    extra_bounds.right = bounds.left;
    extra_bounds.top = bounds.top;
  }
  if (extra_view_) {
    CSize ps;
    extra_view_->GetPreferredSize(&ps);
    CRect lb;
    GetLocalBounds(&lb, false);
    extra_bounds.left = lb.left + kButtonHEdgeMargin;
    extra_bounds.bottom = extra_bounds.top + ps.cy;
    extra_view_->SetBounds(extra_bounds);
  }
}

void ClientView::LayoutContentsView() {
  // We acquire a |contents_view_| ptr when we are constructed, but this can be
  // NULL (for testing purposes). Also, we explicitly don't immediately insert
  // the contents_view_ into the hierarchy until we ourselves are inserted,
  // because the contents_view_ may have initialization that relies on a HWND
  // at the time it is inserted into _any_ hierarchy. So this check is to
  // ensure the contents_view_ is in a valid state as a child of this window
  // before trying to lay it out to our size.
  if (contents_view_ && contents_view_->GetParent() == this) {
    int button_height = 0;
    if (has_dialog_buttons()) {
      if (cancel_button_)
        button_height = cancel_button_->GetHeight();
      else
        button_height = ok_button_->GetHeight();
      // Account for padding above and below the button.
      button_height += kDialogButtonContentSpacing + kButtonVEdgeMargin;
    }

    CRect lb;
    GetLocalBounds(&lb, false);
    lb.bottom = std::max(0, static_cast<int>(lb.bottom - button_height));
    contents_view_->SetBounds(lb);
    contents_view_->Layout();
  }
}

// static
void ClientView::InitClass() {
  static bool initialized = false;
  if (!initialized) {
    ResourceBundle& rb = ResourceBundle::GetSharedInstance();
    dialog_button_font_ = rb.GetFont(ResourceBundle::BaseFont);
    initialized = true;
  }
}

}
