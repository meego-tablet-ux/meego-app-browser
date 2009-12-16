// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BOOKMARKS_BOOKMARK_MODEL_OBSERVER_H_
#define CHROME_BROWSER_BOOKMARKS_BOOKMARK_MODEL_OBSERVER_H_

class BookmarkModel;
class BookmarkNode;

// Observer for the BookmarkModel.
class BookmarkModelObserver {
 public:
  // Invoked when the model has finished loading.
  virtual void Loaded(BookmarkModel* model) = 0;

  // Invoked from the destructor of the BookmarkModel.
  virtual void BookmarkModelBeingDeleted(BookmarkModel* model) {}

  // Invoked when a node has moved.
  virtual void BookmarkNodeMoved(BookmarkModel* model,
                                 const BookmarkNode* old_parent,
                                 int old_index,
                                 const BookmarkNode* new_parent,
                                 int new_index) = 0;

  // Invoked when a node has been added.
  virtual void BookmarkNodeAdded(BookmarkModel* model,
                                 const BookmarkNode* parent,
                                 int index) = 0;

  // Invoked when a node has been removed, the item may still be starred though.
  // |parent| the parent of the node that was removed.
  // |old_index| the index of the removed node in |parent| before it was
  // removed.
  // |node| is the node that was removed.
  virtual void BookmarkNodeRemoved(BookmarkModel* model,
                                   const BookmarkNode* parent,
                                   int old_index,
                                   const BookmarkNode* node) = 0;

  // Invoked when the title or favicon of a node has changed.
  virtual void BookmarkNodeChanged(BookmarkModel* model,
                                   const BookmarkNode* node) = 0;

  // Invoked when a favicon has finished loading.
  virtual void BookmarkNodeFavIconLoaded(BookmarkModel* model,
                                         const BookmarkNode* node) = 0;

  // Invoked when the children (just direct children, not descendants) of
  // |node| have been reordered in some way, such as sorted.
  virtual void BookmarkNodeChildrenReordered(BookmarkModel* model,
                                             const BookmarkNode* node) = 0;

 protected:
  virtual ~BookmarkModelObserver() {}
};

#endif  // CHROME_BROWSER_BOOKMARKS_BOOKMARK_MODEL_OBSERVER_H_
