// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// These classes define a text field widget that can be used in the views UI
// toolkit.

#ifndef VIEWS_CONTROLS_TEXT_FIELD_H_
#define VIEWS_CONTROLS_TEXT_FIELD_H_

#include <string>

#include "app/gfx/font.h"
#include "base/basictypes.h"
#include "views/view.h"
#include "third_party/skia/include/core/SkColor.h"

namespace views {

class HWNDView;

// This class implements a ChromeView that wraps a native text (edit) field.
class TextField : public View {
 public:
  // Keystroke provides a platform-dependent way to send keystroke events.
  // Cross-platform code can use IsKeystrokeEnter/Escape to check for these
  // two common key events.
  // TODO(brettw) this should be cleaned up to be more cross-platform.
#if defined(OS_WIN)
  struct Keystroke {
    Keystroke(unsigned int m,
              wchar_t k,
              int r,
              unsigned int f)
        : message(m),
          key(k),
          repeat_count(r),
          flags(f) {
    }

    unsigned int message;
    wchar_t key;
    int repeat_count;
    unsigned int flags;
  };
#else
  struct Keystroke {
    // TODO(brettw) figure out what this should be on GTK.
  };
#endif

  // This defines the callback interface for other code to be notified of
  // changes in the state of a text field.
  class Controller {
   public:
    // This method is called whenever the text in the field changes.
    virtual void ContentsChanged(TextField* sender,
                                 const std::wstring& new_contents) = 0;

    // This method is called to get notified about keystrokes in the edit.
    // This method returns true if the message was handled and should not be
    // processed further. If it returns false the processing continues.
    virtual bool HandleKeystroke(TextField* sender,
                                 const TextField::Keystroke& keystroke) = 0;
  };

  enum StyleFlags {
    STYLE_DEFAULT = 0,
    STYLE_PASSWORD = 1<<0,
    STYLE_MULTILINE = 1<<1,
    STYLE_LOWERCASE = 1<<2
  };

  TextField::TextField()
      : native_view_(NULL),
        edit_(NULL),
        controller_(NULL),
        style_(STYLE_DEFAULT),
        read_only_(false),
        default_width_in_chars_(0),
        draw_border_(true),
        use_default_background_color_(true),
        num_lines_(1) {
    SetFocusable(true);
  }
  explicit TextField::TextField(StyleFlags style)
      : native_view_(NULL),
        edit_(NULL),
        controller_(NULL),
        style_(style),
        read_only_(false),
        default_width_in_chars_(0),
        draw_border_(true),
        use_default_background_color_(true),
        num_lines_(1) {
    SetFocusable(true);
  }
  virtual ~TextField();

  void ViewHierarchyChanged(bool is_add, View* parent, View* child);

  // Overridden for layout purposes
  virtual void Layout();
  virtual gfx::Size GetPreferredSize();

  // Controller accessors
  void SetController(Controller* controller);
  Controller* GetController() const;

  void SetReadOnly(bool read_only);
  bool IsReadOnly() const;

  bool IsPassword() const;

  // Whether the text field is multi-line or not, must be set when the text
  // field is created, using StyleFlags.
  bool IsMultiLine() const;

  virtual bool IsFocusable() const;
  virtual void AboutToRequestFocusFromTabTraversal(bool reverse);

  // Overridden from Chrome::View.
  virtual bool SkipDefaultKeyEventProcessing(const KeyEvent& e);

  virtual HWND GetNativeComponent();

  // Returns the text currently displayed in the text field.
  std::wstring GetText() const;

  // Sets the text currently displayed in the text field.
  void SetText(const std::wstring& text);

  // Appends the given string to the previously-existing text in the field.
  void AppendText(const std::wstring& text);

  virtual void Focus();

  // Causes the edit field to be fully selected.
  void SelectAll();

  // Clears the selection within the edit field and sets the caret to the end.
  void ClearSelection() const;

  StyleFlags GetStyle() const { return style_; }

  void SetBackgroundColor(SkColor color);
  void SetDefaultBackgroundColor();

  // Set the font.
  void SetFont(const gfx::Font& font);

  // Return the font used by this TextField.
  gfx::Font GetFont() const;

  // Sets the left and right margin (in pixels) within the text box. On Windows
  // this is accomplished by packing the left and right margin into a single
  // 32 bit number, so the left and right margins are effectively 16 bits.
  bool SetHorizontalMargins(int left, int right);

  // Should only be called on a multi-line text field. Sets how many lines of
  // text can be displayed at once by this text field.
  void SetHeightInLines(int num_lines);

  // Sets the default width of the text control. See default_width_in_chars_.
  void set_default_width_in_chars(int default_width) {
    default_width_in_chars_ = default_width;
  }

  // Removes the border from the edit box, giving it a 2D look.
  void RemoveBorder();

  // Disable the edit control.
  // NOTE: this does NOT change the read only property.
  void SetEnabled(bool enabled);

  // Provides a cross-platform way of checking whether a keystroke is one of
  // these common keys. Most code only checks keystrokes against these two keys,
  // so the caller can be cross-platform by implementing the platform-specific
  // parts in here.
  // TODO(brettw) we should use a more cross-platform representation of
  // keyboard events so these are not necessary.
  static bool IsKeystrokeEnter(const Keystroke& key);
  static bool IsKeystrokeEscape(const Keystroke& key);

 private:
  class Edit;

  // Invoked by the edit control when the value changes. This method set
  // the text_ member variable to the value contained in edit control.
  // This is important because the edit control can be replaced if it has
  // been deleted during a window close.
  void SyncText();

  // Reset the text field native control.
  void ResetNativeControl();

  // Resets the background color of the edit.
  void UpdateEditBackgroundColor();

  // This encapsulates the HWND of the native text field.
  HWNDView* native_view_;

  // This inherits from the native text field.
  Edit* edit_;

  // This is the current listener for events from this control.
  Controller* controller_;

  StyleFlags style_;

  gfx::Font font_;

  // NOTE: this is temporary until we rewrite TextField to always work whether
  // there is an HWND or not.
  // Used if the HWND hasn't been created yet.
  std::wstring text_;

  bool read_only_;

  // The default number of average characters for the width of this text field.
  // This will be reported as the "desired size". Defaults to 0.
  int default_width_in_chars_;

  // Whether the border is drawn.
  bool draw_border_;

  SkColor background_color_;

  bool use_default_background_color_;

  // The number of lines of text this textfield displays at once.
  int num_lines_;

 protected:
  // Calculates the insets for the text field.
  void CalculateInsets(gfx::Insets* insets);

  DISALLOW_COPY_AND_ASSIGN(TextField);
};

}  // namespace views

#endif  // VIEWS_CONTROLS_TEXT_FIELD_H_
