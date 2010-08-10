// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/scoped_nsobject.h"
#include "base/sys_string_conversions.h"
#import "chrome/browser/cocoa/applescript/bookmark_applescript_utils_unittest.h"
#import "chrome/browser/cocoa/applescript/bookmark_folder_applescript.h"
#import "chrome/browser/cocoa/applescript/bookmark_item_applescript.h"
#import "chrome/browser/cocoa/applescript/constants_applescript.h"
#import "chrome/browser/cocoa/applescript/error_applescript.h"
#include "googleurl/src/gurl.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

typedef BookmarkAppleScriptTest BookmarkFolderAppleScriptTest;

namespace {

// Test all the bookmark folders within.
TEST_F(BookmarkFolderAppleScriptTest, BookmarkFolders) {
  NSArray* bookmarkFolders = [bookmarkBar_.get() bookmarkFolders];
  BookmarkFolderAppleScript* folder1 = [bookmarkFolders objectAtIndex:0];
  BookmarkFolderAppleScript* folder2 = [bookmarkFolders objectAtIndex:1];

  EXPECT_EQ(2U, [bookmarkFolders count]);

  EXPECT_NSEQ(@"f1", [folder1 title]);
  EXPECT_NSEQ(@"f2", [folder2 title]);
  EXPECT_EQ([folder1 container], bookmarkBar_.get());
  EXPECT_EQ([folder2 container], bookmarkBar_.get());
  EXPECT_NSEQ(AppleScript::kBookmarkFoldersProperty,
              [folder1 containerProperty]);
  EXPECT_NSEQ(AppleScript::kBookmarkFoldersProperty,
              [folder2 containerProperty]);
}

// Insert a new bookmark folder.
TEST_F(BookmarkFolderAppleScriptTest, InsertBookmarkFolder) {
  // Emulate what applescript would do when inserting a new bookmark folder.
  // Emulates a script like |set var to make new bookmark folder with
  // properties {title:"foo"}|.
  scoped_nsobject<BookmarkFolderAppleScript> bookmarkFolder(
      [[BookmarkFolderAppleScript alloc] init]);
  scoped_nsobject<NSNumber> var([[bookmarkFolder.get() uniqueID] copy]);
  [bookmarkFolder.get() setTitle:@"foo"];
  [bookmarkBar_.get() insertInBookmarkFolders:bookmarkFolder.get()];

  // Represents the bookmark folder after its added.
  BookmarkFolderAppleScript* bf =
      [[bookmarkBar_.get() bookmarkFolders] objectAtIndex:2];
  EXPECT_NSEQ(@"foo", [bf title]);
  EXPECT_EQ([bf container], bookmarkBar_.get());
  EXPECT_NSEQ(AppleScript::kBookmarkFoldersProperty,
              [bf containerProperty]);
  EXPECT_NSEQ(var.get(), [bf uniqueID]);
}

// Insert a new bookmark folder at a particular position.
TEST_F(BookmarkFolderAppleScriptTest, InsertBookmarkFolderAtPosition) {
  // Emulate what applescript would do when inserting a new bookmark folder.
  // Emulates a script like |set var to make new bookmark folder with
  // properties {title:"foo"} at after bookmark folder 1|.
  scoped_nsobject<BookmarkFolderAppleScript> bookmarkFolder(
      [[BookmarkFolderAppleScript alloc] init]);
  scoped_nsobject<NSNumber> var([[bookmarkFolder.get() uniqueID] copy]);
  [bookmarkFolder.get() setTitle:@"foo"];
  [bookmarkBar_.get() insertInBookmarkFolders:bookmarkFolder.get() atIndex:1];

  // Represents the bookmark folder after its added.
  BookmarkFolderAppleScript* bf =
      [[bookmarkBar_.get() bookmarkFolders] objectAtIndex:1];
  EXPECT_NSEQ(@"foo", [bf title]);
  EXPECT_EQ([bf container], bookmarkBar_.get());
  EXPECT_NSEQ(AppleScript::kBookmarkFoldersProperty, [bf containerProperty]);
  EXPECT_NSEQ(var.get(), [bf uniqueID]);
}

// Delete bookmark folders.
TEST_F(BookmarkFolderAppleScriptTest, DeleteBookmarkFolders) {
  unsigned int folderCount = 2, itemCount = 3;
  for (unsigned int i = 0; i < folderCount; ++i) {
    EXPECT_EQ(folderCount - i, [[bookmarkBar_.get() bookmarkFolders] count]);
    EXPECT_EQ(itemCount, [[bookmarkBar_.get() bookmarkItems] count]);
    [bookmarkBar_.get() removeFromBookmarkFoldersAtIndex:0];
  }
}

// Test all the bookmark items within.
TEST_F(BookmarkFolderAppleScriptTest, BookmarkItems) {
  NSArray* bookmarkItems = [bookmarkBar_.get() bookmarkItems];
  BookmarkItemAppleScript* item1 = [bookmarkItems objectAtIndex:0];
  BookmarkItemAppleScript* item2 = [bookmarkItems objectAtIndex:1];
  BookmarkItemAppleScript* item3 = [bookmarkItems objectAtIndex:2];

  EXPECT_EQ(3U, [bookmarkItems count]);

  EXPECT_NSEQ(@"a", [item1 title]);
  EXPECT_NSEQ(@"d", [item2 title]);
  EXPECT_NSEQ(@"h", [item3 title]);
  EXPECT_EQ([item1 container], bookmarkBar_.get());
  EXPECT_EQ([item2 container], bookmarkBar_.get());
  EXPECT_EQ([item3 container], bookmarkBar_.get());
  EXPECT_NSEQ(AppleScript::kBookmarkItemsProperty, [item1 containerProperty]);
  EXPECT_NSEQ(AppleScript::kBookmarkItemsProperty, [item2 containerProperty]);
  EXPECT_NSEQ(AppleScript::kBookmarkItemsProperty, [item3 containerProperty]);
}

// Insert a new bookmark item.
TEST_F(BookmarkFolderAppleScriptTest, InsertBookmarkItem) {
  // Emulate what applescript would do when inserting a new bookmark folder.
  // Emulates a script like |set var to make new bookmark item with
  // properties {title:"Google", URL:"http://google.com"}|.
  scoped_nsobject<BookmarkItemAppleScript> bookmarkItem(
      [[BookmarkItemAppleScript alloc] init]);
  scoped_nsobject<NSNumber> var([[bookmarkItem.get() uniqueID] copy]);
  [bookmarkItem.get() setTitle:@"Google"];
  [bookmarkItem.get() setURL:@"http://google.com"];
  [bookmarkBar_.get() insertInBookmarkItems:bookmarkItem.get()];

  // Represents the bookmark item after its added.
  BookmarkItemAppleScript* bi =
      [[bookmarkBar_.get() bookmarkItems] objectAtIndex:3];
  EXPECT_NSEQ(@"Google", [bi title]);
  EXPECT_EQ(GURL("http://google.com/"),
            GURL(base::SysNSStringToUTF8([bi URL])));
  EXPECT_EQ([bi container], bookmarkBar_.get());
  EXPECT_NSEQ(AppleScript::kBookmarkItemsProperty, [bi containerProperty]);
  EXPECT_NSEQ(var.get(), [bi uniqueID]);

  // Test to see no bookmark item is created when no/invlid URL is entered.
  scoped_nsobject<FakeScriptCommand> fakeScriptCommand(
      [[FakeScriptCommand alloc] init]);
  bookmarkItem.reset([[BookmarkItemAppleScript alloc] init]);
  [bookmarkBar_.get() insertInBookmarkItems:bookmarkItem.get()];
  EXPECT_EQ((int)AppleScript::errInvalidURL,
            [fakeScriptCommand.get() scriptErrorNumber]);
}

// Insert a new bookmark item at a particular position.
TEST_F(BookmarkFolderAppleScriptTest, InsertBookmarkItemAtPosition) {
  // Emulate what applescript would do when inserting a new bookmark item.
  // Emulates a script like |set var to make new bookmark item with
  // properties {title:"XKCD", URL:"http://xkcd.org}
  // at after bookmark item 1|.
  scoped_nsobject<BookmarkItemAppleScript> bookmarkItem(
      [[BookmarkItemAppleScript alloc] init]);
  scoped_nsobject<NSNumber> var([[bookmarkItem.get() uniqueID] copy]);
  [bookmarkItem.get() setTitle:@"XKCD"];
  [bookmarkItem.get() setURL:@"http://xkcd.org"];

  [bookmarkBar_.get() insertInBookmarkItems:bookmarkItem.get() atIndex:1];

  // Represents the bookmark item after its added.
  BookmarkItemAppleScript* bi =
      [[bookmarkBar_.get() bookmarkItems] objectAtIndex:1];
  EXPECT_NSEQ(@"XKCD", [bi title]);
  EXPECT_EQ(GURL("http://xkcd.org/"),
            GURL(base::SysNSStringToUTF8([bi URL])));
  EXPECT_EQ([bi container], bookmarkBar_.get());
  EXPECT_NSEQ(AppleScript::kBookmarkItemsProperty,
              [bi containerProperty]);
  EXPECT_NSEQ(var.get(), [bi uniqueID]);

  // Test to see no bookmark item is created when no/invlid URL is entered.
  scoped_nsobject<FakeScriptCommand> fakeScriptCommand(
      [[FakeScriptCommand alloc] init]);
  bookmarkItem.reset([[BookmarkItemAppleScript alloc] init]);
  [bookmarkBar_.get() insertInBookmarkItems:bookmarkItem.get() atIndex:1];
  EXPECT_EQ((int)AppleScript::errInvalidURL,
            [fakeScriptCommand.get() scriptErrorNumber]);
}

// Delete bookmark items.
TEST_F(BookmarkFolderAppleScriptTest, DeleteBookmarkItems) {
  unsigned int folderCount = 2, itemCount = 3;
  for (unsigned int i = 0; i < itemCount; ++i) {
    EXPECT_EQ(folderCount, [[bookmarkBar_.get() bookmarkFolders] count]);
    EXPECT_EQ(itemCount - i, [[bookmarkBar_.get() bookmarkItems] count]);
    [bookmarkBar_.get() removeFromBookmarkItemsAtIndex:0];
  }
}

// Set and get title.
TEST_F(BookmarkFolderAppleScriptTest, GetAndSetTitle) {
  NSArray* bookmarkFolders = [bookmarkBar_.get() bookmarkFolders];
  BookmarkFolderAppleScript* folder1 = [bookmarkFolders objectAtIndex:0];
  [folder1 setTitle:@"Foo"];
  EXPECT_NSEQ(@"Foo", [folder1 title]);
}

}  // namespace
