// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "views/controls/textfield/textfield.h"

#if defined(OS_LINUX)
#include <gdk/gdkkeysyms.h>
#endif

#include "app/gfx/insets.h"
#if defined(OS_WIN)
#include "app/win_util.h"
#include "base/win_util.h"
#endif

#include "base/string_util.h"
#include "views/controls/textfield/native_textfield_wrapper.h"
#include "views/widget/widget.h"

#if defined(OS_WIN)
// TODO(beng): this should be removed when the OS_WIN hack from
// ViewHierarchyChanged is removed.
#include "views/controls/textfield/native_textfield_win.h"
#endif

namespace views {

// static
const char Textfield::kViewClassName[] = "views/Textfield";

/////////////////////////////////////////////////////////////////////////////
// Textfield

Textfield::Textfield()
    : native_wrapper_(NULL),
      controller_(NULL),
      style_(STYLE_DEFAULT),
      read_only_(false),
      default_width_in_chars_(0),
      draw_border_(true),
      text_color_(SK_ColorBLACK),
      use_default_text_color_(true),
      background_color_(SK_ColorWHITE),
      use_default_background_color_(true),
      num_lines_(1),
      initialized_(false) {
  SetFocusable(true);
}

Textfield::Textfield(StyleFlags style)
    : native_wrapper_(NULL),
      controller_(NULL),
      style_(style),
      read_only_(false),
      default_width_in_chars_(0),
      draw_border_(true),
      text_color_(SK_ColorBLACK),
      use_default_text_color_(true),
      background_color_(SK_ColorWHITE),
      use_default_background_color_(true),
      num_lines_(1),
      initialized_(false) {
  SetFocusable(true);
}

Textfield::~Textfield() {
}

void Textfield::SetController(Controller* controller) {
  controller_ = controller;
}

Textfield::Controller* Textfield::GetController() const {
  return controller_;
}

void Textfield::SetReadOnly(bool read_only) {
  read_only_ = read_only;
  if (native_wrapper_) {
    native_wrapper_->UpdateReadOnly();
    native_wrapper_->UpdateTextColor();
    native_wrapper_->UpdateBackgroundColor();
  }
}

bool Textfield::IsPassword() const {
  return style_ & STYLE_PASSWORD;
}

bool Textfield::IsMultiLine() const {
  return !!(style_ & STYLE_MULTILINE);
}

void Textfield::SetText(const string16& text) {
  text_ = text;
  if (native_wrapper_)
    native_wrapper_->UpdateText();
}

void Textfield::AppendText(const string16& text) {
  text_ += text;
  if (native_wrapper_)
    native_wrapper_->AppendText(text);
}

void Textfield::SelectAll() {
  if (native_wrapper_)
    native_wrapper_->SelectAll();
}

string16 Textfield::GetSelectedText() const {
  if (native_wrapper_)
    return native_wrapper_->GetSelectedText();
  return string16();
}

void Textfield::ClearSelection() const {
  if (native_wrapper_)
    native_wrapper_->ClearSelection();
}

void Textfield::SetTextColor(SkColor color) {
  text_color_ = color;
  use_default_text_color_ = false;
  if (native_wrapper_)
    native_wrapper_->UpdateTextColor();
}

void Textfield::UseDefaultTextColor() {
  use_default_text_color_ = true;
  if (native_wrapper_)
    native_wrapper_->UpdateTextColor();
}

void Textfield::SetBackgroundColor(SkColor color) {
  background_color_ = color;
  use_default_background_color_ = false;
  if (native_wrapper_)
    native_wrapper_->UpdateBackgroundColor();
}

void Textfield::UseDefaultBackgroundColor() {
  use_default_background_color_ = true;
  if (native_wrapper_)
    native_wrapper_->UpdateBackgroundColor();
}

void Textfield::SetFont(const gfx::Font& font) {
  font_ = font;
  if (native_wrapper_)
    native_wrapper_->UpdateFont();
}

void Textfield::SetHorizontalMargins(int left, int right) {
  if (native_wrapper_)
    native_wrapper_->SetHorizontalMargins(left, right);
}

void Textfield::SetHeightInLines(int num_lines) {
  DCHECK(IsMultiLine());
  num_lines_ = num_lines;
}

void Textfield::RemoveBorder() {
  if (!draw_border_)
    return;

  draw_border_ = false;
  if (native_wrapper_)
    native_wrapper_->UpdateBorder();
}


void Textfield::CalculateInsets(gfx::Insets* insets) {
  DCHECK(insets);

  if (!draw_border_)
    return;

  // NOTE: One would think GetThemeMargins would return the insets we should
  // use, but it doesn't. The margins returned by GetThemeMargins are always
  // 0.

  // This appears to be the insets used by Windows.
  insets->Set(3, 3, 3, 3);
}

void Textfield::SyncText() {
  if (native_wrapper_)
    text_ = native_wrapper_->GetText();
}

////////////////////////////////////////////////////////////////////////////////
// Textfield, View overrides:

void Textfield::Layout() {
  if (native_wrapper_) {
    native_wrapper_->GetView()->SetBounds(GetLocalBounds(true));
    native_wrapper_->GetView()->Layout();
  }
}

gfx::Size Textfield::GetPreferredSize() {
  gfx::Insets insets;
  CalculateInsets(&insets);
  return gfx::Size(font_.GetExpectedTextWidth(default_width_in_chars_) +
                       insets.width(),
                   num_lines_ * font_.height() + insets.height());
}

bool Textfield::IsFocusable() const {
  return IsEnabled() && !read_only_;
}

void Textfield::AboutToRequestFocusFromTabTraversal(bool reverse) {
  SelectAll();
}

bool Textfield::SkipDefaultKeyEventProcessing(const KeyEvent& e) {
#if defined(OS_WIN)
  // TODO(hamaji): Figure out which keyboard combinations we need to add here,
  //               similar to LocationBarView::SkipDefaultKeyEventProcessing.
  const int c = e.GetCharacter();
  if (c == VK_BACK)
    return true;  // We'll handle BackSpace ourselves.

  // We don't translate accelerators for ALT + NumPad digit, they are used for
  // entering special characters.  We do translate alt-home.
  if (e.IsAltDown() && (c != VK_HOME) &&
      win_util::IsNumPadDigit(c, e.IsExtendedKey()))
    return true;
#endif
  return false;
}

void Textfield::SetEnabled(bool enabled) {
  View::SetEnabled(enabled);
  if (native_wrapper_)
    native_wrapper_->UpdateEnabled();
}

void Textfield::Focus() {
  if (native_wrapper_) {
    // Forward the focus to the wrapper if it exists.
    native_wrapper_->SetFocus();
  } else {
    // If there is no wrapper, cause the RootView to be focused so that we still
    // get keyboard messages.
    View::Focus();
  }
}

void Textfield::ViewHierarchyChanged(bool is_add, View* parent, View* child) {
  if (is_add && !native_wrapper_ && GetWidget() && !initialized_) {
    initialized_ = true;

    // The native wrapper's lifetime will be managed by the view hierarchy after
    // we call AddChildView.
    native_wrapper_ = NativeTextfieldWrapper::CreateWrapper(this);
    AddChildView(native_wrapper_->GetView());
    // TODO(beng): Move this initialization to NativeTextfieldWin once it
    //             subclasses NativeControlWin.
    native_wrapper_->UpdateText();
    native_wrapper_->UpdateTextColor();
    native_wrapper_->UpdateBackgroundColor();
    native_wrapper_->UpdateReadOnly();
    native_wrapper_->UpdateFont();
    native_wrapper_->UpdateEnabled();
    native_wrapper_->UpdateBorder();

#if defined(OS_WIN)
    // TODO(beng): remove this once NativeTextfieldWin subclasses
    // NativeControlWin. This is currently called to perform post-AddChildView
    // initialization for the wrapper. The GTK version subclasses things
    // correctly and doesn't need this.
    //
    // Remove the include for native_textfield_win.h above when you fix this.
    static_cast<NativeTextfieldWin*>(native_wrapper_)->AttachHack();
#endif
  }
}

std::string Textfield::GetClassName() const {
  return kViewClassName;
}

NativeTextfieldWrapper* Textfield::CreateWrapper() {
  NativeTextfieldWrapper* native_wrapper =
      NativeTextfieldWrapper::CreateWrapper(this);

  native_wrapper->UpdateText();
  native_wrapper->UpdateBackgroundColor();
  native_wrapper->UpdateReadOnly();
  native_wrapper->UpdateFont();
  native_wrapper->UpdateEnabled();
  native_wrapper->UpdateBorder();

  return native_wrapper;
}

base::KeyboardCode Textfield::Keystroke::GetKeyboardCode() const {
#if defined(OS_WIN)
  return static_cast<base::KeyboardCode>(key_);
#else
  return static_cast<base::KeyboardCode>(event_.keyval);
#endif
}

#if defined(OS_WIN)
bool Textfield::Keystroke::IsControlHeld() const {
  return win_util::IsCtrlPressed();
}

bool Textfield::Keystroke::IsShiftHeld() const {
  return win_util::IsShiftPressed();
}
#else
bool Textfield::Keystroke::IsControlHeld() const {
  return (event_.state & gtk_accelerator_get_default_mod_mask()) ==
      GDK_CONTROL_MASK;
}

bool Textfield::Keystroke::IsShiftHeld() const {
  return (event_.state & gtk_accelerator_get_default_mod_mask()) ==
      GDK_SHIFT_MASK;
}
#endif

}  // namespace views
