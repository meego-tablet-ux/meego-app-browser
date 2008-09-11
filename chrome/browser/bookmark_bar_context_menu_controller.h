// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BOOKMARK_BAR_CONTEXT_MENU_CONTROLLER_H_
#define CHROME_BROWSER_BOOKMARK_BAR_CONTEXT_MENU_CONTROLLER_H_

#include "chrome/views/chrome_menu.h"
#include "chrome/browser/views/bookmark_bar_view.h"

class BookmarkNode;
class PageNavigator;

// BookmarkBarContextMenuController manages the context menus shown for the
// bookmark bar, items on the bookmark bar, and submens of folders on the
// bookmark bar.
class BookmarkBarContextMenuController : public ChromeViews::MenuDelegate,
    public BookmarkBarView::ModelChangedListener {
 public:
  BookmarkBarContextMenuController(BookmarkBarView* view,
                                   BookmarkNode* node);

  // Shows the menu at the specified place.
  void RunMenuAt(int x, int y);

  // ModelChangedListener method, cancels the menu.
  virtual void ModelChanged();

  // Returns the menu.
  ChromeViews::MenuItemView* menu() { return &menu_; }

  // Menu::Delegate methods.
  virtual void ExecuteCommand(int id);
  virtual bool IsItemChecked(int id) const;
  virtual bool IsCommandEnabled(int id) const;

  // IDs used for the menus. Public for testing.
  static const int always_show_command_id;
  static const int open_bookmark_id;
  static const int open_bookmark_in_new_window_id;
  static const int open_bookmark_in_new_tab_id;
  static const int open_all_bookmarks_id;
  static const int open_all_bookmarks_in_new_window_id;
  static const int edit_bookmark_id;
  static const int delete_bookmark_id;
  static const int add_bookmark_id;
  static const int new_folder_id;

 private:
  // Returns the parent node and visual_order to use when adding new
  // bookmarks/folders.
  BookmarkNode* GetParentAndVisualOrderForNewNode(int* visual_order);

  ChromeViews::MenuItemView menu_;
  BookmarkBarView* view_;
  BookmarkNode* node_;

  DISALLOW_EVIL_CONSTRUCTORS(BookmarkBarContextMenuController);
};

#endif  // CHROME_BROWSER_BOOKMARK_BAR_CONTEXT_MENU_CONTROLLER_H_

