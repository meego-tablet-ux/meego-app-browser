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

#ifndef CHROME_VIEWS_DIALOG_CLIENT_VIEW_H_
#define CHROME_VIEWS_DIALOG_CLIENT_VIEW_H_

#include "chrome/common/gfx/chrome_font.h"
#include "chrome/views/client_view.h"
#include "chrome/views/native_button.h"

namespace ChromeViews {

class DialogDelegate;
class Window;

///////////////////////////////////////////////////////////////////////////////
// DialogClientView
//
//  This ClientView subclass provides the content of a typical dialog box,
//  including a strip of buttons at the bottom right of the window, default
//  accelerator handlers for accept and cancel, and the ability for the
//  embedded contents view to provide extra UI to be shown in the row of
//  buttons.
//
class DialogClientView : public ClientView,
                         public NativeButton::Listener {
 public:
  DialogClientView(Window* window, View* contents_view);
  virtual ~DialogClientView();

  // Adds the dialog buttons required by the supplied WindowDelegate to the
  // view.
  void ShowDialogButtons();

  // Updates the enabled state and label of the buttons required by the
  // supplied WindowDelegate
  void UpdateDialogButtons();

  // Accept the changes made in the window that contains this ClientView.
  void AcceptWindow();

  // Cancel the changes made in the window that contains this ClientView.
  void CancelWindow();

  // Accessors in case the user wishes to adjust these buttons.
  NativeButton* ok_button() const { return ok_button_; }
  NativeButton* cancel_button() const { return cancel_button_; }

  // Overridden from ClientView:
  virtual bool CanClose() const;
  virtual int NonClientHitTest(const gfx::Point& point);
  virtual DialogClientView* AsDialogClientView() { return this; }

 protected:
  // View overrides:
  virtual void Paint(ChromeCanvas* canvas);
  virtual void PaintChildren(ChromeCanvas* canvas);
  virtual void Layout();
  virtual void ViewHierarchyChanged(bool is_add, View* parent, View* child);
  virtual void DidChangeBounds(const CRect& prev, const CRect& next);
  virtual void GetPreferredSize(CSize* out);
  virtual bool AcceleratorPressed(const Accelerator& accelerator);

  // NativeButton::Listener implementation:
  virtual void ButtonPressed(NativeButton* sender);

 private:
  // Paint the size box in the bottom right corner of the window if it is
  // resizable.
  void PaintSizeBox(ChromeCanvas* canvas);

  // Returns the width of the specified dialog button using the correct font.
  int GetButtonWidth(int button) const;
  int DialogClientView::GetButtonsHeight() const;

  // Position and size various sub-views.
  void LayoutDialogButtons();
  void LayoutContentsView();

  bool has_dialog_buttons() const { return ok_button_ || cancel_button_; }

  // Returns the DialogDelegate for the window.
  DialogDelegate* GetDialogDelegate() const;

  // The dialog buttons.
  NativeButton* ok_button_;
  NativeButton* cancel_button_;

  // The button-level extra view, NULL unless the dialog delegate supplies one.
  View* extra_view_;

  // The layout rect of the size box, when visible.
  gfx::Rect size_box_bounds_;

  // True if the window was Accepted by the user using the OK button.
  bool accepted_;

  // Static resource initialization
  static void InitClass();
  static ChromeFont dialog_button_font_;

  DISALLOW_EVIL_CONSTRUCTORS(DialogClientView);
};

}

#endif  // #ifndef CHROME_VIEWS_DIALOG_CLIENT_VIEW_H_
