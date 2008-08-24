// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VIEWS_BOOKMARK_BUBBLE_VIEW_H__
#define CHROME_BROWSER_VIEWS_BOOKMARK_BUBBLE_VIEW_H__

#include "base/gfx/rect.h"
#include "chrome/browser/views/info_bubble.h"
#include "chrome/views/combo_box.h"
#include "chrome/views/link.h"
#include "chrome/views/native_button.h"
#include "chrome/views/view.h"
#include "googleurl/src/gurl.h"

class Profile;

class BookmarkBarModel;
class BookmarkBarNode;

namespace ChromeViews {
class CheckBox;
class TextField;
}

// BookmarkBubbleView is a view intended to be used as the content of an
// InfoBubble. BookmarkBubbleView provides views for unstarring and editting
// the bookmark it is created with. Don't create a BookmarkBubbleView directly,
// instead use the static Show method.
class BookmarkBubbleView : public ChromeViews::View,
                           public ChromeViews::LinkController,
                           public ChromeViews::NativeButton::Listener,
                           public ChromeViews::ComboBox::Listener,
                           public InfoBubbleDelegate {
 public:
   static void Show(HWND parent,
                    const gfx::Rect& bounds,
                    InfoBubbleDelegate* delegate,
                    Profile* profile,
                    const GURL& url,
                    bool newly_bookmarked);

  virtual ~BookmarkBubbleView();

  // Overriden to force a layout.
  virtual void DidChangeBounds(const CRect& previous, const CRect& current);

  // Invoked after the bubble has been shown.
  virtual void BubbleShown();

  // Override to close on return.
  virtual bool AcceleratorPressed(const ChromeViews::Accelerator& accelerator);

 private:
  // Model for the combobox showing the list of folders to choose from. The
  // list always contains the bookmark bar, other node and parent. The list
  // also contains an extra item that shows the text 'Choose another folder...'.
  class RecentlyUsedFoldersModel : public ChromeViews::ComboBox::Model {
   public:
    RecentlyUsedFoldersModel(BookmarkBarModel* bb_model, BookmarkBarNode* node);

    // ComboBox::Model methods. Call through to nodes_.
    virtual int GetItemCount(ChromeViews::ComboBox* source);
    virtual std::wstring GetItemAt(ChromeViews::ComboBox* source, int index);

    // Returns the node at the specified index.
    BookmarkBarNode* GetNodeAt(int index);

    // Returns the index of the original parent folder.
    int node_parent_index() const { return node_parent_index_; }

   private:
    // Removes node from nodes_. Does nothing if node is not in nodes_.
    void RemoveNode(BookmarkBarNode* node);

    std::vector<BookmarkBarNode*> nodes_;
    int node_parent_index_;

    DISALLOW_EVIL_CONSTRUCTORS(RecentlyUsedFoldersModel);
  };

  // Creates a BookmarkBubbleView.
  // |title| is the title of the page. If newly_bookmarked is false, title is
  // ignored and the title of the bookmark is fetched from the database.
  BookmarkBubbleView(InfoBubbleDelegate* delegate,
                     Profile* profile,
                     const GURL& url,
                     bool newly_bookmarked);
  // Creates the child views.
  void Init();

  // Returns the title to display.
  std::wstring GetTitle();

  // LinkController method, either unstars the item or shows the bookmark
  // editor (depending upon which link was clicked).
  virtual void LinkActivated(ChromeViews::Link* source, int event_flags);

  // ButtonListener method, closes the bubble or opens the edit dialog.		
  virtual void ButtonPressed(ChromeViews::NativeButton* sender);

  // ComboBox::Listener method. Changes the parent of the bookmark.
  virtual void ItemChanged(ChromeViews::ComboBox* combo_box,
                           int prev_index,
                           int new_index);

  // InfoBubbleDelegate methods. These forward to the InfoBubbleDelegate
  // supplied in the constructor as well as sending out the necessary
  // notification.
  virtual void InfoBubbleClosing(InfoBubble* info_bubble);
  virtual bool CloseOnEscape();

  // Closes the bubble.
  void Close();

  // Removes the bookmark and closes the view.
  void RemoveBookmark();

  // Shows the BookmarkEditor.
  void ShowEditor();

  // Sets the title of the bookmark from the editor
  void SetNodeTitleFromTextField();

  // Delegate for the bubble, may be null.
  InfoBubbleDelegate* delegate_;

  // The profile.
  Profile* profile_;

  // The bookmark URL.
  const GURL url_;

  // Title of the bookmark. This is initially the title supplied to the
  // constructor, which is typically the title of the page.
  std::wstring title_;

  // If true, the page was just bookmarked.
  const bool newly_bookmarked_;

  RecentlyUsedFoldersModel parent_model_;

  // Link for removing/unstarring the bookmark.
  ChromeViews::Link* remove_link_;

  // Button to bring up the editor.
  ChromeViews::NativeButton* edit_button_;

  // Button to close the window.
  ChromeViews::NativeButton* close_button_;

  // TextField showing the title of the bookmark.
  ChromeViews::TextField* title_tf_;

  // ComboBox showing a handful of folders the user can choose from, including
  // the current parent.
  ChromeViews::ComboBox* parent_combobox_;

  DISALLOW_EVIL_CONSTRUCTORS(BookmarkBubbleView);
};

#endif  // CHROME_BROWSER_VIEWS_BOOKMARK_BUBBLE_VIEW_H__

