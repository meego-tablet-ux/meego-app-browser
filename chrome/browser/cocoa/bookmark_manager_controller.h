// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>
#include "base/scoped_nsobject.h"
#include "base/scoped_ptr.h"

@class BookmarkGroupsController;
@class BookmarkItem;
@class BookmarkTreeController;
class BookmarkManagerBridge;
class BookmarkModel;
class BookmarkNode;
class Profile;

// Controller for the bookmark manager window. There is at most one instance.
@interface BookmarkManagerController : NSWindowController {
 @private
  IBOutlet NSTableView* groupsTable_;
  IBOutlet NSSearchField* toolbarSearchView_;
  IBOutlet BookmarkGroupsController* groupsController_;
  IBOutlet BookmarkTreeController* treeController_;

  Profile* profile_;  // weak
  scoped_ptr<BookmarkManagerBridge> bridge_;
  scoped_nsobject<NSMapTable> nodeMap_;
  scoped_nsobject<NSImage> folderIcon_;
  scoped_nsobject<NSImage> defaultFavIcon_;
}

// Opens the bookmark manager window, or brings it to the front if it's open.
+ (BookmarkManagerController*)showBookmarkManager:(Profile*)profile;

// The user Profile.
@property (readonly) Profile* profile;

// The BookmarkModel of the manager's Profile.
@property (readonly) BookmarkModel* bookmarkModel;

// Maps C++ BookmarkNode objects to Objective-C BookmarkItems.
- (BookmarkItem*)itemFromNode:(const BookmarkNode*)node;

@property (readonly) BookmarkItem* bookmarkBarItem;
@property (readonly) BookmarkItem* otherBookmarksItem;

// Returns a context menu for use with either table view pane.
// A new instance is created every time, so the caller can customize it.
- (NSMenu*)contextMenu;

// Called by the toolbar search field after the user changes its text.
- (IBAction)searchFieldChanged:(id)sender;

@end


// Exposed only for unit tests.
@interface BookmarkManagerController (UnitTesting)

- (void)forgetNode:(const BookmarkNode*)node;
@property (readonly) BookmarkGroupsController* groupsController;
@property (readonly) BookmarkTreeController* treeController;

@end
