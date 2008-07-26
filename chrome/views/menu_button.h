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

#ifndef CHROME_VIEWS_MENU_BUTTON_H__
#define CHROME_VIEWS_MENU_BUTTON_H__

#include <windows.h>

#include "chrome/common/gfx/chrome_font.h"
#include "chrome/views/background.h"
#include "chrome/views/text_button.h"
#include "base/time.h"

namespace ChromeViews {

class MouseEvent;
class ViewMenuDelegate;


////////////////////////////////////////////////////////////////////////////////
//
// MenuButton
//
//  A button that shows a menu when the left mouse button is pushed
//
////////////////////////////////////////////////////////////////////////////////
class MenuButton : public TextButton {
 public:
  //
  // Create a Button
  MenuButton(const std::wstring& text,
             ViewMenuDelegate* menu_delegate,
             bool show_menu_marker);
  virtual ~MenuButton();

  // Activate the button (called when the button is pressed).
  virtual bool Activate();

  // Overridden to take into account the potential use of a drop marker.
  void GetPreferredSize(CSize* result);
  virtual void Paint(ChromeCanvas* canvas, bool for_drag);

  // These methods are overriden to implement a simple push button
  // behavior
  virtual bool OnMousePressed(const ChromeViews::MouseEvent& e);
  void OnMouseReleased(const ChromeViews::MouseEvent& e, bool canceled);
  virtual bool OnKeyReleased(const KeyEvent& e);
  virtual void OnMouseExited(const MouseEvent& event);

  // Returns the MSAA default action of the current view. The string returned
  // describes the default action that will occur when executing
  // IAccessible::DoDefaultAction.
  bool GetAccessibleDefaultAction(std::wstring* action);

  // Returns the MSAA role of the current view. The role is what assistive
  // technologies (ATs) use to determine what behavior to expect from a given
  // control.
  bool GetAccessibleRole(VARIANT* role);

  // Returns the MSAA state of the current view. Sets the input VARIANT
  // appropriately, and returns true if a change was performed successfully.
  // Overriden from View.
  virtual bool GetAccessibleState(VARIANT* state);

 protected:
  // true if the menu is currently visible.
  bool menu_visible_;

 private:

  // Compute the maximum X coordinate for the current screen. MenuButtons
  // use this to make sure a menu is never shown off screen.
  int GetMaximumScreenXCoordinate();

  DISALLOW_EVIL_CONSTRUCTORS(MenuButton);

  // We use a time object in order to keep track of when the menu was closed.
  // The time is used for simulating menu behavior for the menu button; that
  // is, if the menu is shown and the button is pressed, we need to close the
  // menu. There is no clean way to get the second click event because the
  // menu is displayed using a modal loop and, unlike regular menus in Windows,
  // the button is not part of the displayed menu.
  Time menu_closed_time_;

  // The associated menu's resource identifier.
  ViewMenuDelegate* menu_delegate_;

  // Whether or not we're showing a drop marker.
  bool show_menu_marker_;

  friend class TextButtonBackground;
};

} // namespace

#endif  // CHROME_VIEWS_MENU_BUTTON_H__
