// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>
#import "base/cocoa_protocols_mac.h"
#include "base/scoped_nsobject.h"

class BookmarkModel;
class BookmarkNode;
@class BookmarkBubbleController;

// Controller for the bookmark bubble.  The bookmark bubble is a
// bubble that pops up when clicking on the STAR next to the URL to
// add or remove it as a bookmark.  This bubble allows for editing of
// the bookmark in various ways (name, folder, etc.)
@interface BookmarkBubbleController : NSWindowController<NSWindowDelegate> {
 @private
  NSWindow* parentWindow_;  // weak
  NSPoint topLeftForBubble_;

  // Both weak; owned by the current browser's profile
  BookmarkModel* model_;  // weak
  const BookmarkNode* node_;  // weak

  BOOL alreadyBookmarked_;

  IBOutlet NSTextField* bigTitle_;   // "Bookmark" or "Bookmark Added!"
  IBOutlet NSTextField* nameTextField_;
  IBOutlet NSPopUpButton* folderPopUpButton_;
}

@property (readonly, nonatomic) const BookmarkNode* node;

// |node| is the bookmark node we edit in this bubble.
// |alreadyBookmarked| tells us if the node was bookmarked before the
//   user clicked on the star.  (if NO, this is a brand new bookmark).
// The owner of this object is responsible for showing the bubble if
// it desires it to be visible on the screen.  It is not shown by the
// init routine.  Closing of the window happens implicitly on dealloc.
- (id)initWithParentWindow:(NSWindow*)parentWindow
          topLeftForBubble:(NSPoint)topLeftForBubble
                     model:(BookmarkModel*)model
                      node:(const BookmarkNode*)node
     alreadyBookmarked:(BOOL)alreadyBookmarked;

// Actions for buttons in the dialog.
- (IBAction)ok:(id)sender;
- (IBAction)remove:(id)sender;
- (IBAction)cancel:(id)sender;

// These actions send a -editBookmarkNode: action up the responder chain.
- (IBAction)edit:(id)sender;
- (IBAction)folderChanged:(id)sender;

@end


// Exposed only for unit testing.
@interface BookmarkBubbleController(ExposedForUnitTesting)
- (void)addFolderNodes:(const BookmarkNode*)parent
         toPopUpButton:(NSPopUpButton*)button;
- (void)setTitle:(NSString*)title parentFolder:(const BookmarkNode*)parent;
- (void)setParentFolderSelection:(const BookmarkNode*)parent;
+ (NSString*)chooseAnotherFolderString;
- (NSPopUpButton*)folderPopUpButton;
@end





