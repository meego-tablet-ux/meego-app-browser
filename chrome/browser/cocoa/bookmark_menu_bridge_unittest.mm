// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <AppKit/AppKit.h>
#include "chrome/app/chrome_dll_resource.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/cocoa/bookmark_menu_bridge.h"
#include "chrome/browser/cocoa/browser_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

// TODO(jrg): see refactor comment in bookmark_bar_state_controller_unittest.mm
class BookmarkMenuBridgeTest : public testing::Test {
 public:

  // We are a friend of BookmarkMenuBridge (and have access to
  // protected methods), but none of the classes generated by TEST_F()
  // are.  This (and AddNodeToMenu()) are simple wrappers to let
  // derived test classes have access to protected methods.
  void ClearBookmarkMenu(BookmarkMenuBridge* bridge, NSMenu* menu) {
    bridge->ClearBookmarkMenu(menu);
  }

  void AddNodeToMenu(BookmarkMenuBridge* bridge, BookmarkNode* root,
                     NSMenu* menu) {
    bridge->AddNodeToMenu(root, menu);
  }

  NSMenuItem* AddItemToMenu(NSMenu *menu, NSString *title, SEL selector) {
    NSMenuItem *item = [[[NSMenuItem alloc] initWithTitle:title action:NULL
                                            keyEquivalent:@""] autorelease];
    if (selector)
      [item setAction:selector];
    [menu addItem:item];
    return item;
  }

  BrowserTestHelper browser_test_helper_;
};


// Test that ClearBookmarkMenu() removes all bookmark menus.
TEST_F(BookmarkMenuBridgeTest, TestClearBookmarkMenu) {
  scoped_ptr<BookmarkMenuBridge> bridge(new BookmarkMenuBridge());
  EXPECT_TRUE(bridge.get());

  NSMenu* menu = [[[NSMenu alloc] initWithTitle:@"foo"] autorelease];

  AddItemToMenu(menu, @"hi mom", nil);
  AddItemToMenu(menu, @"not", @selector(openBookmarkMenuItem:));
  NSMenuItem* item = AddItemToMenu(menu, @"hi mom", nil);
  [item setSubmenu:[[[NSMenu alloc] initWithTitle:@"bar"] autorelease]];
  AddItemToMenu(menu, @"not", @selector(openBookmarkMenuItem:));
  AddItemToMenu(menu, @"zippy", @selector(length));

  ClearBookmarkMenu(bridge.get(), menu);

  // Make sure all bookmark items are removed, and all items with
  // submenus removed.
  EXPECT_EQ(2, [menu numberOfItems]);
  for (NSMenuItem *item in [menu itemArray]) {
    EXPECT_FALSE([[item title] isEqual:@"not"]);
  }
}

// Test that AddNodeToMenu() properly adds bookmark nodes as menus,
// including the recursive case.
TEST_F(BookmarkMenuBridgeTest, TestAddNodeToMenu) {
  std::wstring empty;
  Profile* profile = browser_test_helper_.profile();

  scoped_ptr<BookmarkMenuBridge> bridge(new BookmarkMenuBridge());
  EXPECT_TRUE(bridge.get());

  NSMenu* menu = [[[NSMenu alloc] initWithTitle:@"foo"] autorelease];

  BookmarkModel* model = profile->GetBookmarkModel();
  BookmarkNode* bookmark_bar = model->GetBookmarkBarNode();
  BookmarkNode* root = model->AddGroup(bookmark_bar, 0, empty);
  EXPECT_TRUE(model && root);

  const char* short_url = "http://foo/";
  const char* long_url = "http://super-duper-long-url--."
    "that.cannot.possibly.fit.even-in-80-columns"
    "or.be.reasonably-displayed-in-a-menu"
    "without.looking-ridiculous.com/"; // 140 chars total

  // 3 nodes; middle one has a child, last one has a HUGE URL
  // Set their titles to be the same as the URLs
  BookmarkNode* node = NULL;
  model->AddURL(root, 0, ASCIIToWide(short_url), GURL(short_url));
  node = model->AddGroup(root, 1, empty);
  model->AddURL(root, 2, ASCIIToWide(long_url), GURL(long_url));

  // And the submenu fo the middle one
  model->AddURL(node, 0, empty, GURL("http://sub"));

  // Add to the NSMenu, then confirm it looks good
  AddNodeToMenu(bridge.get(), root, menu);

  EXPECT_EQ(3, [menu numberOfItems]);

  // Confirm for just the first and last (index 0 and 2)
  for (int x=0; x<3; x+=2) {
    EXPECT_EQ(@selector(openBookmarkMenuItem:), [[menu itemAtIndex:x] action]);
    EXPECT_EQ(NO, [[menu itemAtIndex:x] hasSubmenu]);
  }

  // Now confirm the middle one (index 1)
  NSMenuItem* middle = [menu itemAtIndex:1];
  EXPECT_NE(NO, [middle hasSubmenu]);
  EXPECT_EQ(1, [[middle submenu] numberOfItems]);

  // Confirm 1st and 3rd item have the same action (that we specified).
  // Make sure the submenu item does NOT have an bookmark action.
  EXPECT_EQ([[menu itemAtIndex:0] action], [[menu itemAtIndex:2] action]);
  EXPECT_NE([[menu itemAtIndex:0] action], [[menu itemAtIndex:1] action]);

  // Make sure a short title looks fine
  NSString* s = [[menu itemAtIndex:0] title];
  EXPECT_TRUE([s isEqual:[NSString stringWithUTF8String:short_url]]);

  // Make sure a super-long title gets trimmed
  s = [[menu itemAtIndex:0] title];
  EXPECT_TRUE([s length] < strlen(long_url));

  // Confirm tooltips and confirm they are not trimmed (like the item
  // name might be).  Add tolerance for URL fixer-upping;
  // e.g. http://foo becomes http://foo/)
  EXPECT_GE([[[menu itemAtIndex:0] toolTip] length], (2*strlen(short_url) - 5));
  EXPECT_GE([[[menu itemAtIndex:2] toolTip] length], (2*strlen(long_url) - 5));
}
