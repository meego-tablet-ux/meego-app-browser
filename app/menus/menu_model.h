// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APP_MENUS_MENU_MODEL_H_
#define APP_MENUS_MENU_MODEL_H_
#pragma once

#include "base/scoped_ptr.h"
#include "base/string16.h"
#include "gfx/native_widget_types.h"

class SkBitmap;

namespace gfx {

class Font;

}  // namespace gfx

namespace menus {

class Accelerator;
class ButtonMenuItemModel;

// An interface implemented by an object that provides the content of a menu.
class MenuModel {
 public:
  virtual ~MenuModel() {}

  // The type of item.
  enum ItemType {
    TYPE_COMMAND,
    TYPE_CHECK,
    TYPE_RADIO,
    TYPE_SEPARATOR,
    TYPE_BUTTON_ITEM,
    TYPE_SUBMENU
  };

  // Returns true if any of the items within the model have icons. Not all
  // platforms support icons in menus natively and so this is a hint for
  // triggering a custom rendering mode.
  virtual bool HasIcons() const = 0;

  // Returns the index of the first item. This is 0 for most menus except the
  // system menu on Windows. |native_menu| is the menu to locate the start index
  // within. It is guaranteed to be reset to a clean default state.
  // IMPORTANT: If the model implementation returns something _other_ than 0
  //            here, it must offset the values for |index| it passes to the
  //            methods below by this number - this is NOT done automatically!
  virtual int GetFirstItemIndex(gfx::NativeMenu native_menu) const { return 0; }

  // Returns the number of items in the menu.
  virtual int GetItemCount() const = 0;

  // Returns the type of item at the specified index.
  virtual ItemType GetTypeAt(int index) const = 0;

  // Returns the command id of the item at the specified index.
  virtual int GetCommandIdAt(int index) const = 0;

  // Returns the label of the item at the specified index.
  virtual string16 GetLabelAt(int index) const = 0;

  // Returns true if the label at the specified index can change over the course
  // of the menu's lifetime. If this function returns true, the label of the
  // menu item will be updated each time the menu is shown.
  virtual bool IsLabelDynamicAt(int index) const = 0;

  // Returns the font use for the label at the specified index.
  // If NULL, then use default font.
  virtual const gfx::Font* GetLabelFontAt(int index) const { return NULL; }

  // Gets the acclerator information for the specified index, returning true if
  // there is a shortcut accelerator for the item, false otherwise.
  virtual bool GetAcceleratorAt(int index,
                                menus::Accelerator* accelerator) const = 0;

  // Returns the checked state of the item at the specified index.
  virtual bool IsItemCheckedAt(int index) const = 0;

  // Returns the id of the group of radio items that the item at the specified
  // index belongs to.
  virtual int GetGroupIdAt(int index) const = 0;

  // Gets the icon for the item at the specified index, returning true if there
  // is an icon, false otherwise.
  virtual bool GetIconAt(int index, SkBitmap* icon) const = 0;

  // Returns the model for a menu item with a line of buttons at |index|.
  virtual ButtonMenuItemModel* GetButtonMenuItemAt(int index) const = 0;

  // Returns the enabled state of the item at the specified index.
  virtual bool IsEnabledAt(int index) const = 0;

  // Returns the model for the submenu at the specified index.
  virtual MenuModel* GetSubmenuModelAt(int index) const = 0;

  // Called when the highlighted menu item changes to the item at the specified
  // index.
  virtual void HighlightChangedTo(int index) = 0;

  // Called when the item at the specified index has been activated.
  virtual void ActivatedAt(int index) = 0;

  // Called when the menu is about to be shown.
  virtual void MenuWillShow() {}

  // Retrieves the model and index that contains a specific command id. Returns
  // true if an item with the specified command id is found. |model| is inout,
  // and specifies the model to start searching from.
  static bool GetModelAndIndexForCommandId(int command_id, MenuModel** model,
                                           int* index);
};

}  // namespace

#endif  // APP_MENUS_MENU_MODEL_H_
