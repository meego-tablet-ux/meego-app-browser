// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "views/controls/label.h"

#include <math.h>
#include <limits>

#include "app/gfx/canvas.h"
#include "app/gfx/font.h"
#include "app/gfx/text_elider.h"
#include "app/l10n_util.h"
#include "app/resource_bundle.h"
#include "base/logging.h"
#include "base/utf_string_conversions.h"
#include "gfx/color_utils.h"
#include "gfx/insets.h"
#include "views/background.h"

namespace views {

// static
const char Label::kViewClassName[] = "views/Label";
SkColor Label::kEnabledColor, Label::kDisabledColor;

static const int kFocusBorderPadding = 1;

Label::Label() {
  Init(L"", GetDefaultFont());
}

Label::Label(const std::wstring& text) {
  Init(text, GetDefaultFont());
}

Label::Label(const std::wstring& text, const gfx::Font& font) {
  Init(text, font);
}

void Label::Init(const std::wstring& text, const gfx::Font& font) {
  static bool initialized = false;
  if (!initialized) {
#if defined(OS_WIN)
    kEnabledColor = color_utils::GetSysSkColor(COLOR_WINDOWTEXT);
    kDisabledColor = color_utils::GetSysSkColor(COLOR_GRAYTEXT);
#else
    // TODO(beng): source from theme provider.
    kEnabledColor = SK_ColorBLACK;
    kDisabledColor = SK_ColorGRAY;
#endif

    initialized = true;
  }

  contains_mouse_ = false;
  font_ = font;
  text_size_valid_ = false;
  SetText(text);
  url_set_ = false;
  color_ = kEnabledColor;
  highlight_color_ = kEnabledColor;
  horiz_alignment_ = ALIGN_CENTER;
  is_multi_line_ = false;
  allow_character_break_ = false;
  collapse_when_hidden_ = false;
  rtl_alignment_mode_ = USE_UI_ALIGNMENT;
  paint_as_focused_ = false;
  has_focus_border_ = false;
  highlighted_ = false;
}

Label::~Label() {
}

gfx::Size Label::GetPreferredSize() {
  gfx::Size prefsize;

  // Return a size of (0, 0) if the label is not visible and if the
  // collapse_when_hidden_ flag is set.
  // TODO(munjal): This logic probably belongs to the View class. But for now,
  // put it here since putting it in View class means all inheriting classes
  // need ot respect the collapse_when_hidden_ flag.
  if (!IsVisible() && collapse_when_hidden_)
    return prefsize;

  if (is_multi_line_) {
    int w = width(), h = 0;
    gfx::Canvas::SizeStringInt(text_, font_, &w, &h, ComputeMultiLineFlags());
    // TODO(erikkay) With highlighted_ enabled, should we adjust the size
    // in the multi-line case?
    prefsize.SetSize(w, h);
  } else {
    prefsize = GetTextSize();
  }

  gfx::Insets insets = GetInsets();
  prefsize.Enlarge(insets.width(), insets.height());
  return prefsize;
}

int Label::GetBaseline() {
  return GetInsets().top() + font_.baseline();
}

int Label::ComputeMultiLineFlags() {
  int flags = gfx::Canvas::MULTI_LINE;
#if !defined(OS_WIN)
    // Don't ellide multiline labels on Linux.
    // Todo(davemoore): Do we depend on elliding multiline text?
    // Pango insists on limiting the number of lines to one if text is
    // ellided. You can get around this if you can pass a maximum height
    // but we don't currently have that data when we call the pango code.
    flags |= gfx::Canvas::NO_ELLIPSIS;
#endif
  if (allow_character_break_)
    flags |= gfx::Canvas::CHARACTER_BREAK;
  switch (horiz_alignment_) {
    case ALIGN_LEFT:
      flags |= gfx::Canvas::TEXT_ALIGN_LEFT;
      break;
    case ALIGN_CENTER:
      flags |= gfx::Canvas::TEXT_ALIGN_CENTER;
      break;
    case ALIGN_RIGHT:
      flags |= gfx::Canvas::TEXT_ALIGN_RIGHT;
      break;
  }
  return flags;
}

void Label::CalculateDrawStringParams(std::wstring* paint_text,
                                      gfx::Rect* text_bounds,
                                      int* flags) {
  DCHECK(paint_text && text_bounds && flags);

  if (url_set_) {
    // TODO(jungshik) : Figure out how to get 'intl.accept_languages'
    // preference and use it when calling ElideUrl.
    *paint_text = gfx::ElideUrl(url_, font_, width(), std::wstring());

    // An URLs is always treated as an LTR text and therefore we should
    // explicitly mark it as such if the locale is RTL so that URLs containing
    // Hebrew or Arabic characters are displayed correctly.
    //
    // Note that we don't check the View's UI layout setting in order to
    // determine whether or not to insert the special Unicode formatting
    // characters. We use the locale settings because an URL is always treated
    // as an LTR string, even if its containing view does not use an RTL UI
    // layout.
    if (l10n_util::GetTextDirection() == l10n_util::RIGHT_TO_LEFT)
      l10n_util::WrapStringWithLTRFormatting(paint_text);
  } else {
    *paint_text = text_;
  }

  if (is_multi_line_) {
    gfx::Insets insets = GetInsets();
    text_bounds->SetRect(insets.left(),
                         insets.top(),
                         width() - insets.width(),
                         height() - insets.height());
    *flags = ComputeMultiLineFlags();
  } else {
    *text_bounds = GetTextBounds();
    *flags = 0;
  }
}

void Label::Paint(gfx::Canvas* canvas) {
  PaintBackground(canvas);
  std::wstring paint_text;
  gfx::Rect text_bounds;
  int flags = 0;
  CalculateDrawStringParams(&paint_text, &text_bounds, &flags);
  if (highlighted_) {
    // Draw a second version of the string underneath the main one, but down
    // and to the right by a pixel to create a highlight.
    canvas->DrawStringInt(paint_text,
                          font_,
                          highlight_color_,
                          text_bounds.x() + 1,
                          text_bounds.y() + 1,
                          text_bounds.width(),
                          text_bounds.height());
  }
  canvas->DrawStringInt(paint_text,
                        font_,
                        color_,
                        text_bounds.x(),
                        text_bounds.y(),
                        text_bounds.width(),
                        text_bounds.height(),
                        flags);

  // The focus border always hugs the text, regardless of the label's bounds.
  if (HasFocus() || paint_as_focused_) {
    int w = text_bounds.width();
    int h = 0;
    gfx::Canvas::SizeStringInt(paint_text, font_, &w, &h, flags);
    gfx::Rect focus_rect = text_bounds;
    focus_rect.set_width(w);
    focus_rect.set_height(h);
    focus_rect.Inset(-kFocusBorderPadding, -kFocusBorderPadding);
    // If the label is a single line of text, then the computed text bound
    // corresponds directly to the text being drawn and no mirroring is needed
    // for the RTL case. For multiline text, the text bound is an estimation
    // and is recomputed in gfx::Canvas::SizeStringInt(). For multiline text
    // in RTL, we need to take mirroring into account when computing the focus
    // rectangle.
    int x = focus_rect.x();
    if (flags & gfx::Canvas::MULTI_LINE)
      x = MirroredLeftPointForRect(focus_rect);
    canvas->DrawFocusRect(x, focus_rect.y(), focus_rect.width(),
                          focus_rect.height());
  }
}

void Label::PaintBackground(gfx::Canvas* canvas) {
  const Background* bg = contains_mouse_ ? GetMouseOverBackground() : NULL;
  if (!bg)
    bg = background();
  if (bg)
    bg->Paint(canvas, this);
}

void Label::SetFont(const gfx::Font& font) {
  font_ = font;
  text_size_valid_ = false;
  SchedulePaint();
}

gfx::Font Label::GetFont() const {
  return font_;
}

void Label::SetText(const std::wstring& text) {
  text_ = text;
  url_set_ = false;
  text_size_valid_ = false;
  SchedulePaint();
}

void Label::SetURL(const GURL& url) {
  url_ = url;
  text_ = UTF8ToWide(url_.spec());
  url_set_ = true;
  text_size_valid_ = false;
  SchedulePaint();
}

const std::wstring Label::GetText() const {
  if (url_set_)
    return UTF8ToWide(url_.spec());
  else
    return text_;
}

const GURL Label::GetURL() const {
  if (url_set_)
    return url_;
  else
    return GURL(WideToUTF8(text_));
}

gfx::Size Label::GetTextSize() {
  if (!text_size_valid_) {
    // Multi-line labels need a boundary width (see GetHeightForWidth).
    DCHECK(!is_multi_line_);
    int h = 0, w = std::numeric_limits<int>::max();
    gfx::Canvas cc(0, 0, true);
    cc.SizeStringInt(text_, font_, &w, &h, 0);
    text_size_.SetSize(w, font_.height());
    if (highlighted_)
      text_size_.Enlarge(1, 1);
    text_size_valid_ = true;
  }

  return text_size_;
}

int Label::GetHeightForWidth(int w) {
  if (is_multi_line_) {
    gfx::Insets insets = GetInsets();
    w = std::max<int>(0, w - insets.width());
    int h = 0;
    gfx::Canvas cc(0, 0, true);
    cc.SizeStringInt(text_, font_, &w, &h, ComputeMultiLineFlags());
    return h + insets.height();
  }

  return View::GetHeightForWidth(w);
}

std::string Label::GetClassName() const {
  return kViewClassName;
}

void Label::SetColor(const SkColor& color) {
  color_ = color;
}

SkColor Label::GetColor() const {
  return color_;
}

void Label::SetDrawHighlighted(bool h) {
  highlighted_ = h;
  text_size_valid_ = false;
}

void Label::SetHorizontalAlignment(Alignment a) {
  // If the View's UI layout is right-to-left and rtl_alignment_mode_ is
  // USE_UI_ALIGNMENT, we need to flip the alignment so that the alignment
  // settings take into account the text directionality.
  if (UILayoutIsRightToLeft() && rtl_alignment_mode_ == USE_UI_ALIGNMENT) {
    if (a == ALIGN_LEFT)
      a = ALIGN_RIGHT;
    else if (a == ALIGN_RIGHT)
      a = ALIGN_LEFT;
  }
  if (horiz_alignment_ != a) {
    horiz_alignment_ = a;
    SchedulePaint();
  }
}

Label::Alignment Label::GetHorizontalAlignment() const {
  return horiz_alignment_;
}

void Label::SetRTLAlignmentMode(RTLAlignmentMode mode) {
  rtl_alignment_mode_ = mode;
}

Label::RTLAlignmentMode Label::GetRTLAlignmentMode() const {
  return rtl_alignment_mode_;
}

void Label::SetMultiLine(bool f) {
  if (f != is_multi_line_) {
    is_multi_line_ = f;
    SchedulePaint();
  }
}

void Label::SetAllowCharacterBreak(bool f) {
  if (f != allow_character_break_) {
    allow_character_break_ = f;
    SchedulePaint();
  }
}

bool Label::IsMultiLine() {
  return is_multi_line_;
}

void Label::SetTooltipText(const std::wstring& tooltip_text) {
  tooltip_text_ = tooltip_text;
}

bool Label::GetTooltipText(const gfx::Point& p, std::wstring* tooltip) {
  DCHECK(tooltip);

  // If a tooltip has been explicitly set, use it.
  if (!tooltip_text_.empty()) {
    tooltip->assign(tooltip_text_);
    return true;
  }

  // Show the full text if the text does not fit.
  if (!is_multi_line_ && font_.GetStringWidth(text_) > width()) {
    *tooltip = text_;
    return true;
  }
  return false;
}

void Label::OnMouseMoved(const MouseEvent& e) {
  UpdateContainsMouse(e);
}

void Label::OnMouseEntered(const MouseEvent& event) {
  UpdateContainsMouse(event);
}

void Label::OnMouseExited(const MouseEvent& event) {
  SetContainsMouse(false);
}

void Label::SetMouseOverBackground(Background* background) {
  mouse_over_background_.reset(background);
}

const Background* Label::GetMouseOverBackground() const {
  return mouse_over_background_.get();
}

void Label::SetEnabled(bool enabled) {
  if (enabled == enabled_)
    return;
  View::SetEnabled(enabled);
  SetColor(enabled ? kEnabledColor : kDisabledColor);
}

gfx::Insets Label::GetInsets() const {
  gfx::Insets insets = View::GetInsets();
  if (IsFocusable() || has_focus_border_)  {
    insets += gfx::Insets(kFocusBorderPadding, kFocusBorderPadding,
                          kFocusBorderPadding, kFocusBorderPadding);
  }
  return insets;
}

// static
gfx::Font Label::GetDefaultFont() {
  return ResourceBundle::GetSharedInstance().GetFont(ResourceBundle::BaseFont);
}

void Label::UpdateContainsMouse(const MouseEvent& event) {
  if (is_multi_line_) {
    gfx::Rect rect(width(), GetHeightForWidth(width()));
    SetContainsMouse(rect.Contains(event.x(), event.y()));
  } else {
    SetContainsMouse(GetTextBounds().Contains(event.x(), event.y()));
  }
}

void Label::SetContainsMouse(bool contains_mouse) {
  if (contains_mouse_ == contains_mouse)
    return;
  contains_mouse_ = contains_mouse;
  if (GetMouseOverBackground())
    SchedulePaint();
}

gfx::Rect Label::GetTextBounds() {
  gfx::Size text_size = GetTextSize();
  gfx::Insets insets = GetInsets();
  int avail_width = width() - insets.width();
  // Respect the size set by the owner view
  text_size.set_width(std::max(0, std::min(avail_width, text_size.width())));

  int text_y = insets.top() +
      (height() - text_size.height() - insets.height()) / 2;
  int text_x;
  switch (horiz_alignment_) {
    case ALIGN_LEFT:
      text_x = insets.left();
      break;
    case ALIGN_CENTER:
      // We put any extra margin pixel on the left rather than the right, since
      // GetTextExtentPoint32() can report a value one too large on the right.
      text_x = insets.left() + (avail_width + 1 - text_size.width()) / 2;
      break;
    case ALIGN_RIGHT:
      text_x = width() - insets.right() - text_size.width();
      break;
    default:
      NOTREACHED();
      text_x = 0;
      break;
  }
  return gfx::Rect(text_x, text_y, text_size.width(), text_size.height());
}

void Label::SizeToFit(int max_width) {
  DCHECK(is_multi_line_);

  std::vector<std::wstring> lines;
  SplitString(text_, L'\n', &lines);

  int label_width = 0;
  for (std::vector<std::wstring>::const_iterator iter = lines.begin();
       iter != lines.end(); ++iter) {
    label_width = std::max(label_width, font_.GetStringWidth(*iter));
  }

  gfx::Insets insets = GetInsets();
  label_width += insets.width();

  if (max_width > 0)
    label_width = std::min(label_width, max_width);

  SetBounds(x(), y(), label_width, 0);
  SizeToPreferredSize();
}

bool Label::GetAccessibleRole(AccessibilityTypes::Role* role) {
  DCHECK(role);

  *role = AccessibilityTypes::ROLE_TEXT;
  return true;
}

bool Label::GetAccessibleName(std::wstring* name) {
  DCHECK(name);
  *name = GetText();
  return !name->empty();
}

bool Label::GetAccessibleState(AccessibilityTypes::State* state) {
  DCHECK(state);

  *state = AccessibilityTypes::STATE_READONLY;
  return true;
}

}  // namespace views
