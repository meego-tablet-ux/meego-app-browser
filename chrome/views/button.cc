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

#include "chrome/views/button.h"

#include <atlbase.h>
#include <atlapp.h>

#include "base/gfx/image_operations.h"
#include "chrome/common/gfx/chrome_canvas.h"
#include "chrome/common/l10n_util.h"
#include "chrome/common/throb_animation.h"
#include "chrome/views/event.h"
#include "chrome/views/view_container.h"
#include "chrome/app/chrome_dll_resource.h"

#include "generated_resources.h"

namespace ChromeViews {

static const int kDefaultWidth = 16;   // Default button width if no theme.
static const int kDefaultHeight = 14;  // Default button height if no theme.

////////////////////////////////////////////////////////////////////////////////
//
// Button - constructors, destructors, initialization
//
////////////////////////////////////////////////////////////////////////////////

Button::Button() : BaseButton(),
                   h_alignment_(ALIGN_LEFT),
                   v_alignment_(ALIGN_TOP) {
  // By default, we request that the ChromeCanvas passed to our View::Paint()
  // implementation is flipped horizontally so that the button's bitmaps are
  // mirrored when the UI directionality is right-to-left.
  EnableCanvasFlippingForRTLUI(true);
}

Button::~Button() {
}

////////////////////////////////////////////////////////////////////////////////
//
// Button - properties
//
////////////////////////////////////////////////////////////////////////////////

void Button::SetImage(ButtonState aState, SkBitmap* anImage) {
  images_[aState] = anImage ? *anImage : SkBitmap();
}

void Button::SetImageAlignment(HorizontalAlignment h_align,
                               VerticalAlignment v_align) {
  h_alignment_ = h_align;
  v_alignment_ = v_align;
  SchedulePaint();
}

void Button::GetPreferredSize(CSize *result) {
  if (!images_[BS_NORMAL].isNull()) {
    result->cx = images_[BS_NORMAL].width();
    result->cy = images_[BS_NORMAL].height();
  } else {
    result->cx = kDefaultWidth;
    result->cy = kDefaultHeight;
  }
}

// Set the tooltip text for this button.
void Button::SetTooltipText(const std::wstring& text) {
  tooltip_text_ = text;
  TooltipTextChanged();
}

// Return the tooltip text currently used by this button.
std::wstring Button::GetTooltipText() const {
  return tooltip_text_;
}

////////////////////////////////////////////////////////////////////////////////
//
// Button - painting
//
////////////////////////////////////////////////////////////////////////////////

void Button::Paint(ChromeCanvas* canvas) {
  View::Paint(canvas);
  SkBitmap img = GetImageToPaint();

  if (!img.isNull()) {
    int x = 0, y = 0;

    if (h_alignment_ == ALIGN_CENTER)
      x = (GetWidth() - img.width()) / 2;
    else if (h_alignment_ == ALIGN_RIGHT)
      x = GetWidth() - img.width();

    if (v_alignment_ == ALIGN_MIDDLE)
      y = (GetHeight() - img.height()) / 2;
    else if (v_alignment_ == ALIGN_BOTTOM)
      y = GetHeight() - img.height();

    canvas->DrawBitmapInt(img, x, y);
  }
  PaintFocusBorder(canvas);
}

SkBitmap Button::GetImageToPaint() {
  SkBitmap img;

  if (!images_[BS_HOT].isNull() && hover_animation_->IsAnimating()) {
    img = gfx::ImageOperations::CreateBlendedBitmap(images_[BS_NORMAL],
              images_[BS_HOT], hover_animation_->GetCurrentValue());
  } else {
    img = images_[GetState()];
  }

  return !img.isNull() ? img : images_[BS_NORMAL];
}

////////////////////////////////////////////////////////////////////////////////
//
// Button - accessibility
//
////////////////////////////////////////////////////////////////////////////////

bool Button::GetAccessibleDefaultAction(std::wstring* action) {
  DCHECK(action);

  action->assign(l10n_util::GetString(IDS_ACCACTION_PRESS));
  return true;
}

bool Button::GetAccessibleRole(VARIANT* role) {
  DCHECK(role);

  role->vt = VT_I4;
  role->lVal = ROLE_SYSTEM_PUSHBUTTON;
  return true;
}

bool Button::GetTooltipText(int x, int y, std::wstring* tooltip) {
  if (tooltip_text_.empty())
    return false;

  *tooltip = tooltip_text_;
  return true;
}

////////////////////////////////////////////////////////////////////////////////
//
// ToggleButton
//
////////////////////////////////////////////////////////////////////////////////
ToggleButton::ToggleButton() : Button(), toggled_(false) {
}

ToggleButton::~ToggleButton() {
}

void ToggleButton::SetImage(ButtonState state, SkBitmap* image) {
  if (toggled_) {
    alternate_images_[state] = image ? *image : SkBitmap();
  } else {
    images_[state] = image ? *image : SkBitmap();
    if (state_ == state)
      SchedulePaint();
  }
}

void ToggleButton::SetToggledImage(ButtonState state, SkBitmap* image) {
  if (toggled_) {
    images_[state] = image ? *image : SkBitmap();
    if (state_ == state)
      SchedulePaint();
  } else {
    alternate_images_[state] = image ? *image : SkBitmap();
  }
}

bool ToggleButton::GetTooltipText(int x, int y, std::wstring* tooltip) {
  if (!toggled_ || toggled_tooltip_text_.empty())
    return Button::GetTooltipText(x, y, tooltip);

  *tooltip = toggled_tooltip_text_;
  return true;
}

void ToggleButton::SetToggled(bool toggled) {
  if (toggled == toggled_)
    return;

  for (int i = 0; i < kButtonStateCount; ++i) {
    SkBitmap temp = images_[i];
    images_[i] = alternate_images_[i];
    alternate_images_[i] = temp;
  }
  toggled_ = toggled;
  SchedulePaint();
}

void ToggleButton::SetToggledTooltipText(const std::wstring& tooltip) {
  toggled_tooltip_text_.assign(tooltip);
}
}  // namespace ChromeViews
