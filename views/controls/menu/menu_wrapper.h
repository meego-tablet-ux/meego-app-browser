// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef VIEWS_CONTROLS_MENU_MENU_WRAPPER_H_
#define VIEWS_CONTROLS_MENU_MENU_WRAPPER_H_
#pragma once

#include "gfx/native_widget_types.h"

namespace gfx {
class Point;
}

namespace views {

class Menu2;

// An interface for clients that want a notification when a menu is opened.
class MenuListener {
 public:
  // This will be called after the menu has actually opened.
  virtual void OnMenuOpened() = 0;
};

// An interface that wraps an object that implements a menu.
class MenuWrapper {
 public:
  // All of the possible actions that can result from RunMenuAt.
  enum MenuAction {
    MENU_ACTION_NONE,      // Menu cancelled, or never opened.
    MENU_ACTION_SELECTED,  // An item was selected.
    MENU_ACTION_PREVIOUS,  // User wants to navigate to the previous menu.
    MENU_ACTION_NEXT,      // User wants to navigate to the next menu.
  };

  virtual ~MenuWrapper() {}

  // Runs the menu at the specified point. This blocks until done.
  virtual void RunMenuAt(const gfx::Point& point, int alignment) = 0;

  // Cancels the active menu.
  virtual void CancelMenu() = 0;

  // Called when the model supplying data to this menu has changed, and the menu
  // must be rebuilt.
  virtual void Rebuild() = 0;

  // Called when the states of the items in the menu must be updated from the
  // model.
  virtual void UpdateStates() = 0;

  // Retrieve a native menu handle.
  virtual gfx::NativeMenu GetNativeMenu() const = 0;

  // Get the result of the last call to RunMenuAt to determine whether an
  // item was selected, the user navigated to a next or previous menu, or
  // nothing.
  virtual MenuAction GetMenuAction() const = 0;

  // Add a listener to receive a callback when the menu opens.
  virtual void AddMenuListener(MenuListener* listener) = 0;

  // Remove a menu listener.
  virtual void RemoveMenuListener(MenuListener* listener) = 0;

  // Sets the minimum width of the menu.
  virtual void SetMinimumWidth(int width) = 0;

  // Creates the appropriate instance of this wrapper for the current platform.
  static MenuWrapper* CreateWrapper(Menu2* menu);
};

}  // namespace views

#endif  // VIEWS_CONTROLS_MENU_MENU_WRAPPER_H_
