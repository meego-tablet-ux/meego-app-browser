// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/cocoa/applescript/bookmark_node_applescript.h"

#include "base/logging.h"
#include "base/sys_string_conversions.h"
#import "base/scoped_nsobject.h"
#import "chrome/browser/app_controller_mac.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#import "chrome/browser/chrome_browser_application_mac.h"
#import "chrome/browser/cocoa/applescript/error_applescript.h"
#include "chrome/browser/profile.h"
#import "chrome/browser/cocoa/applescript/bookmark_item_applescript.h"

@interface BookmarkNodeAppleScript()
@property (nonatomic, copy) NSString* tempTitle;
@end

@implementation BookmarkNodeAppleScript

@synthesize tempTitle = tempTitle_;

- (id)init {
  if ((self = [super init])) {
    BookmarkModel* model = [self bookmarkModel];
    if (!model) {
      [self release];
      return nil;
    }

    scoped_nsobject<NSNumber> numID(
        [[NSNumber alloc] initWithLongLong:model->next_node_id()]);
    [self setUniqueID:numID];
    [self setTempTitle:@""];
  }
  return self;
}

- (void)dealloc {
  [tempTitle_ release];
  [super dealloc];
}


- (id)initWithBookmarkNode:(const BookmarkNode*)aBookmarkNode {
  if (!aBookmarkNode) {
    [self release];
    return nil;
  }

  if ((self = [super init])) {
    // It is safe to be weak, if a bookmark item/folder goes away
    // (eg user deleting a folder) the applescript runtime calls
    // bookmarkFolders/bookmarkItems in BookmarkFolderAppleScript
    // and this particular bookmark item/folder is never returned.
    bookmarkNode_ = aBookmarkNode;

    scoped_nsobject<NSNumber>  numID(
        [[NSNumber alloc] initWithLongLong:aBookmarkNode->id()]);
    [self setUniqueID:numID];
  }
  return self;
}

- (void)setBookmarkNode:(const BookmarkNode*)aBookmarkNode {
  DCHECK(aBookmarkNode);
  // It is safe to be weak, if a bookmark item/folder goes away
  // (eg user deleting a folder) the applescript runtime calls
  // bookmarkFolders/bookmarkItems in BookmarkFolderAppleScript
  // and this particular bookmark item/folder is never returned.
  bookmarkNode_ = aBookmarkNode;

  scoped_nsobject<NSNumber> numID(
      [[NSNumber alloc] initWithLongLong:aBookmarkNode->id()]);
  [self setUniqueID:numID];

  [self setTitle:[self tempTitle]];
}

- (NSString*)title {
  if (!bookmarkNode_)
    return tempTitle_;

  return base::SysWideToNSString(bookmarkNode_->GetTitle());
}

- (void)setTitle:(NSString*)aTitle {
  // If the scripter enters |make new bookmarks folder with properties
  // {title:"foo"}|, the node has not yet been created so title is stored in the
  // temp title.
  if (!bookmarkNode_) {
    [self setTempTitle:aTitle];
    return;
  }

  BookmarkModel* model = [self bookmarkModel];
  if (!model)
    return;

  model->SetTitle(bookmarkNode_, base::SysNSStringToUTF16(aTitle));
}

- (BookmarkModel*)bookmarkModel {
  AppController* appDelegate = [NSApp delegate];

  Profile* defaultProfile = [appDelegate defaultProfile];
  if (!defaultProfile) {
    AppleScript::SetError(AppleScript::errGetProfile);
    return NULL;
  }

  BookmarkModel* model = defaultProfile->GetBookmarkModel();
  if (!model->IsLoaded()) {
    AppleScript::SetError(AppleScript::errBookmarkModelLoad);
    return NULL;
  }

  return model;
}

@end
