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

#include "chrome/views/native_control.h"

#include <atlbase.h>
#include <atlapp.h>
#include <atlcrack.h>
#include <atlframe.h>

#include "base/win_util.h"
#include "chrome/common/l10n_util.h"
#include "chrome/views/border.h"
#include "chrome/views/focus_manager.h"
#include "chrome/views/view_container.h"
#include "chrome/views/hwnd_view.h"
#include "chrome/views/background.h"
#include "base/gfx/native_theme.h"

namespace ChromeViews {

// Maps to the original WNDPROC for the controller window before we subclassed
// it.
static const wchar_t* const kHandlerKey =
    L"__CONTROL_ORIGINAL_MESSAGE_HANDLER__";

// Maps to the NativeControl.
static const wchar_t* const kNativeControlKey = L"__NATIVE_CONTROL__";

class NativeControlContainer : public CWindowImpl<NativeControlContainer,
                               CWindow,
                               CWinTraits<WS_CHILD | WS_CLIPSIBLINGS |
                                          WS_CLIPCHILDREN>> {
 public:

  explicit NativeControlContainer(NativeControl* parent) : parent_(parent),
                                                           control_(NULL) {
    Create(parent->GetViewContainer()->GetHWND());
    ::ShowWindow(m_hWnd, SW_SHOW);
  }

  virtual ~NativeControlContainer() {
  }

  // NOTE: If you add a new message, be sure and verify parent_ is valid before
  // calling into parent_.
  DECLARE_FRAME_WND_CLASS(L"ChromeViewsNativeControlContainer", NULL);
  BEGIN_MSG_MAP(NativeControlContainer);
    MSG_WM_CREATE(OnCreate);
    MSG_WM_ERASEBKGND(OnEraseBkgnd);
    MSG_WM_PAINT(OnPaint);
    MSG_WM_SIZE(OnSize);
    MSG_WM_NOTIFY(OnNotify);
    MSG_WM_COMMAND(OnCommand);
    MSG_WM_DESTROY(OnDestroy);
    MSG_WM_CONTEXTMENU(OnContextMenu);
    MSG_WM_CTLCOLORBTN(OnCtlColorBtn);
    MSG_WM_CTLCOLORSTATIC(OnCtlColorStatic)
  END_MSG_MAP();

  HWND GetControl() {
    return control_;
  }

  // Called when the parent is getting deleted. This control stays around until
  // it gets the OnFinalMessage call.
  void ResetParent() {
    parent_ = NULL;
  }

  void OnFinalMessage(HWND hwnd) {
    if (parent_)
      parent_->NativeControlDestroyed();
    delete this;
  }
 private:

  LRESULT OnCreate(LPCREATESTRUCT create_struct) {
    control_ = parent_->CreateNativeControl(m_hWnd);
    FocusManager::InstallFocusSubclass(control_, parent_);
    if (parent_->NotifyOnKeyDown()) {
      // We subclass the control hwnd so we get the WM_KEYDOWN messages.
      WNDPROC original_handler =
          win_util::SetWindowProc(control_,
                                  &NativeControl::NativeControlWndProc);
      SetProp(control_, kHandlerKey, original_handler);
      SetProp(control_, kNativeControlKey , parent_);
    }
    ::ShowWindow(control_, SW_SHOW);
    return 1;
  }

  LRESULT OnEraseBkgnd(HDC dc) {
    return 1;
  }

  void OnPaint(HDC ignore) {
    PAINTSTRUCT ps;
    HDC dc = ::BeginPaint(*this, &ps);
    ::EndPaint(*this, &ps);
  }

  void OnSize(int type, const CSize& sz) {
    ::MoveWindow(control_, 0, 0, sz.cx, sz.cy, TRUE);
  }

  LRESULT OnCommand(UINT code, int id, HWND source) {
    return parent_ ? parent_->OnCommand(code, id, source) : 0;
  }

  LRESULT OnNotify(int w_param, LPNMHDR l_param) {
    if (parent_)
      return parent_->OnNotify(w_param, l_param);
    else
      return 0;
  }

  void OnDestroy() {
    if (parent_)
      parent_->OnDestroy();
  }

  void OnContextMenu(HWND window, const CPoint& location) {
    if (parent_)
      parent_->OnContextMenu(location);
  }

  // We need to find an ancestor with a non-null background, and
  // ask it for a (solid color) brush that approximates
  // the background.  The caller will use this when drawing
  // the native control as a background color, particularly
  // for radiobuttons and XP style pushbuttons.
  LRESULT OnCtlColor(UINT msg, HDC dc, HWND control) {
    const View *ancestor = parent_;
    while (ancestor) {
      const Background *background = ancestor->GetBackground();
      if (background) {
        HBRUSH brush = background->GetNativeControlBrush();
        if (brush)
          return reinterpret_cast<LRESULT>(brush);
      }
      ancestor = ancestor->GetParent();
    }

    // COLOR_BTNFACE is the default for dialog box backgrounds.
    return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
  }

  LRESULT OnCtlColorBtn(HDC dc, HWND control) {
    return OnCtlColor(WM_CTLCOLORBTN, dc, control);
  }

  LRESULT OnCtlColorStatic(HDC dc, HWND control) {
    return OnCtlColor(WM_CTLCOLORSTATIC, dc, control);
  }

  NativeControl* parent_;
  HWND control_;
  DISALLOW_EVIL_CONSTRUCTORS(NativeControlContainer);
};

NativeControl::NativeControl() : hwnd_view_(NULL),
                                 container_(NULL),
                                 fixed_width_(-1),
                                 horizontal_alignment_(CENTER),
                                 fixed_height_(-1),
                                 vertical_alignment_(CENTER) {
  enabled_ = true;
  focusable_ = true;
}

NativeControl::~NativeControl() {
  if (container_) {
    container_->ResetParent();
    ::DestroyWindow(*container_);
  }
}

void NativeControl::ValidateNativeControl() {
  if (hwnd_view_ == NULL) {
    hwnd_view_ = new HWNDView();
    AddChildView(hwnd_view_);
  }

  if (!container_ && IsVisible()) {
    container_ = new NativeControlContainer(this);
    hwnd_view_->Attach(*container_);
    if (!enabled_)
      EnableWindow(GetNativeControlHWND(), enabled_);

    // This message ensures that the focus border is shown.
    ::SendMessage(container_->GetControl(),
                  WM_CHANGEUISTATE,
                  MAKELPARAM(UIS_CLEAR, UISF_HIDEFOCUS),
                  0);
  }
}

void NativeControl::ViewHierarchyChanged(bool is_add, View *parent,
                                         View *child) {
  if (is_add && GetViewContainer()) {
    ValidateNativeControl();
    Layout();
  }
}

void NativeControl::Layout() {
  if (!container_ && GetViewContainer())
    ValidateNativeControl();

  if (hwnd_view_) {
    CRect lb;
    GetLocalBounds(&lb, false);

    int x = lb.left;
    int y = lb.top;
    int width = lb.Width();
    int height = lb.Height();
    if (fixed_width_ > 0) {
      width = std::min(fixed_width_, width);
      switch (horizontal_alignment_) {
        case LEADING:
          // Nothing to do.
          break;
        case CENTER:
          x += (lb.Width() - width) / 2;
          break;
        case TRAILING:
          x = x + lb.Width() - width;
          break;
        default:
          NOTREACHED();
      }
    }

    if (fixed_height_ > 0) {
      height = std::min(fixed_height_, height);
      switch (vertical_alignment_) {
        case LEADING:
          // Nothing to do.
          break;
        case CENTER:
          y += (lb.Height() - height) / 2;
          break;
        case TRAILING:
          y = y + lb.Height() - height;
          break;
        default:
          NOTREACHED();
      }
    }

    hwnd_view_->SetBounds(x, y, width, height);
  }
}

void NativeControl::DidChangeBounds(const CRect& previous,
                                    const CRect& current) {
  Layout();
}

void NativeControl::Focus() {
  if (container_) {
    DCHECK(container_->GetControl());
    ::SetFocus(container_->GetControl());
  }
}

HWND NativeControl::GetNativeControlHWND() {
  if (container_)
    return container_->GetControl();
  else
    return NULL;
}

void NativeControl::NativeControlDestroyed() {
  if (hwnd_view_)
    hwnd_view_->Detach();
  container_ = NULL;
}

void NativeControl::SetVisible(bool f) {
  if (f != IsVisible()) {
    View::SetVisible(f);
    if (!f && container_) {
      ::DestroyWindow(*container_);
    } else if (f && !container_) {
      ValidateNativeControl();
    }
  }
}

void NativeControl::SetEnabled(bool enabled) {
  if (enabled_ != enabled) {
    View::SetEnabled(enabled);
    if (GetNativeControlHWND()) {
      EnableWindow(GetNativeControlHWND(), enabled_);
    }
  }
}

void NativeControl::Paint(ChromeCanvas* canvas) {
}

void NativeControl::VisibilityChanged(View* starting_from, bool is_visible) {
  SetVisible(is_visible);
}

void NativeControl::SetFixedWidth(int width, Alignment alignment) {
  DCHECK(width > 0);
  fixed_width_ = width;
  horizontal_alignment_ = alignment;
}

void NativeControl::SetFixedHeight(int height, Alignment alignment) {
  DCHECK(height > 0);
  fixed_height_ = height;
  vertical_alignment_ = alignment;
}

DWORD NativeControl::GetAdditionalExStyle() const {
  // If the UI for the view is mirrored, we should make sure we add the
  // extended window style for a right-to-left layout so the subclass creates
  // a mirrored HWND for the underlying control.
  DWORD ex_style = 0;
  if (UILayoutIsRightToLeft())
    ex_style |= l10n_util::GetExtendedStyles();

  return ex_style;
}

// static
LRESULT CALLBACK NativeControl::NativeControlWndProc(HWND window, UINT message,
                                                     WPARAM w_param,
                                                     LPARAM l_param) {
  HANDLE original_handler = GetProp(window, kHandlerKey);
  DCHECK(original_handler);
  NativeControl* native_control =
      static_cast<NativeControl*>(GetProp(window, kNativeControlKey));
  DCHECK(native_control);

  if (message == WM_KEYDOWN) {
    if (native_control->OnKeyDown(static_cast<int>(w_param)))
      return 0;
  } else if (message == WM_DESTROY) {
    win_util::SetWindowProc(window,
                            reinterpret_cast<WNDPROC>(original_handler));
    RemoveProp(window, kHandlerKey);
    RemoveProp(window, kNativeControlKey);
  }

  return CallWindowProc(reinterpret_cast<WNDPROC>(original_handler), window,
                        message, w_param, l_param);
}

}
