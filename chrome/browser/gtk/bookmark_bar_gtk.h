// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GTK_BOOKMARK_BAR_GTK_H_
#define CHROME_BROWSER_GTK_BOOKMARK_BAR_GTK_H_

#include <gtk/gtk.h>

#include <string>
#include <vector>

#include "chrome/common/owned_widget_gtk.h"
#include "chrome/browser/bookmarks/bookmark_model.h"

class BookmarkContextMenu;
class Browser;
class CustomContainerButton;
class PageNavigator;
class Profile;

class BookmarkBarGtk : public BookmarkModelObserver {
 public:
  explicit BookmarkBarGtk(Profile* proifle, Browser* browser);
  virtual ~BookmarkBarGtk();

  // Resets the profile. This removes any buttons for the current profile and
  // recreates the models.
  void SetProfile(Profile* profile);

  // Returns the current profile.
  Profile* GetProfile() { return profile_; }

  // Returns the current browser.
  Browser* browser() const { return browser_; }

  // Sets the PageNavigator that is used when the user selects an entry on
  // the bookmark bar.
  void SetPageNavigator(PageNavigator* navigator);

  // Create the contents of the bookmark bar.
  void Init(Profile* profile);

  // Adds this GTK toolbar into a sizing box.
  void AddBookmarkbarToBox(GtkWidget* box);

  // Whether the current page is the New Tag Page (which requires different
  // rendering).
  bool OnNewTabPage();

  // Change the visibility of the bookmarks bar. (Starts out hidden, per GTK's
  // default behaviour).
  void Show();
  void Hide();

  // Returns true if the bookmarks bar preference is set to 'always show'.
  bool IsAlwaysShown();

 private:
  // Helper function that sets visual properties of GtkButton |button| to the
  // contents of |node|.
  void ConfigureButtonForNode(BookmarkNode* node,
                              GtkWidget* button);

  // Helper function which generates GtkToolItems for |bookmark_toolbar_|.
  void CreateAllBookmarkButtons(BookmarkNode* node);

  // Helper function which destroys all the bookmark buttons in the GtkToolbar.
  void RemoveAllBookmarkButtons();

  // Returns the number of buttons corresponding to starred urls/groups. This
  // is equivalent to the number of children the bookmark bar node from the
  // bookmark bar model has.
  int GetBookmarkButtonCount();

  // Overridden from BookmarkModelObserver:

  // Invoked when the bookmark bar model has finished loading. Creates a button
  // for each of the children of the root node from the model.
  virtual void Loaded(BookmarkModel* model);

  // Invoked when the model is being deleted.
  virtual void BookmarkModelBeingDeleted(BookmarkModel* model);

  // Invoked when a node has moved.
  virtual void BookmarkNodeMoved(BookmarkModel* model,
                                 BookmarkNode* old_parent,
                                 int old_index,
                                 BookmarkNode* new_parent,
                                 int new_index);
  virtual void BookmarkNodeAdded(BookmarkModel* model,
                                 BookmarkNode* parent,
                                 int index);
  virtual void BookmarkNodeRemoved(BookmarkModel* model,
                                   BookmarkNode* parent,
                                   int index);
  virtual void BookmarkNodeChanged(BookmarkModel* model,
                                   BookmarkNode* node);
  // Invoked when a favicon has finished loading.
  virtual void BookmarkNodeFavIconLoaded(BookmarkModel* model,
                                         BookmarkNode* node);
  virtual void BookmarkNodeChildrenReordered(BookmarkModel* model,
                                             BookmarkNode* node);

 private:
  GtkWidget* CreateBookmarkButton(BookmarkNode* node);
  GtkToolItem* CreateBookmarkToolItem(BookmarkNode* node);

  std::string BuildTooltip(BookmarkNode* node);

  // Finds the BookmarkNode from the model associated with |button|.
  BookmarkNode* GetNodeForToolButton(GtkWidget* button);

  // Creates and displays a popup menu for BookmarkNode |node|.
  void PopupMenuForNode(GtkWidget* sender, BookmarkNode* node,
                        GdkEventButton* event);

  // GtkButton callbacks
  static gboolean OnButtonPressed(GtkWidget* sender,
                                  GdkEventButton* event,
                                  BookmarkBarGtk* bar);
  static gboolean OnButtonReleased(GtkWidget* sender,
                                   GdkEventButton* event,
                                   BookmarkBarGtk* bar);
  static gboolean OnButtonExpose(GtkWidget* widget, GdkEventExpose* e,
                                 BookmarkBarGtk* button);
  static void OnButtonDragBegin(GtkWidget* widget,
                                GdkDragContext* drag_context,
                                BookmarkBarGtk* bar);
  static void OnButtonDragEnd(GtkWidget* button,
                              GdkDragContext* drag_context,
                              BookmarkBarGtk* bar);

  // GtkButton callbacks for folder buttons
  static gboolean OnFolderButtonReleased(GtkWidget* sender,
                                         GdkEventButton* event,
                                         BookmarkBarGtk* bar);

  // GtkToolbar callbacks
  static gboolean OnToolbarExpose(GtkWidget* widget, GdkEventExpose* event,
                                  BookmarkBarGtk* window);
  static gboolean OnToolbarDragMotion(GtkToolbar* toolbar,
                                      GdkDragContext* context,
                                      gint x,
                                      gint y,
                                      guint time,
                                      BookmarkBarGtk* bar);
  static void OnToolbarDragLeave(GtkToolbar* toolbar,
                                 GdkDragContext* context,
                                 guint time,
                                 BookmarkBarGtk* bar);
  static gboolean OnToolbarDragDrop(GtkWidget* toolbar,
                                    GdkDragContext* drag_context,
                                    gint x,
                                    gint y,
                                    guint time,
                                    BookmarkBarGtk* bar);

  Profile* profile_;

  // Used for opening urls.
  PageNavigator* page_navigator_;

  Browser* browser_;

  // Model providing details as to the starred entries/groups that should be
  // shown. This is owned by the Profile.
  BookmarkModel* model_;

  // Top level container that contains |bookmark_hbox_| and spacers.
  OwnedWidgetGtk container_;

  // Container that has all the individual members of
  // |current_bookmark_buttons_| as children.
  GtkWidget* bookmark_hbox_;

  // A GtkLabel to display when there are no bookmark buttons to display.
  GtkWidget* instructions_;

  // GtkToolbar which contains all the bookmark buttons.
  OwnedWidgetGtk bookmark_toolbar_;

  // The other bookmarks button.
  GtkWidget* other_bookmarks_button_;

  // Whether we should ignore the next button release event (because we were
  // dragging).
  bool ignore_button_release_;

  // The BookmarkNode from the model being dragged. NULL when we aren't
  // dragging.
  BookmarkNode* dragged_node_;

  // We create a GtkToolbarItem from |dragged_node_| for display.
  GtkToolItem* toolbar_drop_item_;

  // Whether we should show the instructional text in the bookmark bar.
  bool show_instructions_;

  // The last displayed right click menu, or NULL if no menus have been
  // displayed yet.
  scoped_ptr<BookmarkContextMenu> current_context_menu_;
};

#endif  // CHROME_BROWSER_GTK_BOOKMARK_BAR_GTK_H_
