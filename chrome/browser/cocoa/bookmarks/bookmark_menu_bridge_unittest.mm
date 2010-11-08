// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <AppKit/AppKit.h>

#import "base/scoped_nsobject.h"
#include "base/string16.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/cocoa/bookmarks/bookmark_menu_bridge.h"
#include "chrome/browser/cocoa/browser_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

class TestBookmarkMenuBridge : public BookmarkMenuBridge {
 public:
  TestBookmarkMenuBridge(Profile* profile)
      : BookmarkMenuBridge(profile),
        menu_([[NSMenu alloc] initWithTitle:@"test"]) {
  }
  virtual ~TestBookmarkMenuBridge() {}

  scoped_nsobject<NSMenu> menu_;

 protected:
  // Overridden from BookmarkMenuBridge.
  virtual NSMenu* BookmarkMenu() {
    return menu_;
  }
};

// TODO(jrg): see refactor comment in bookmark_bar_state_controller_unittest.mm
class BookmarkMenuBridgeTest : public PlatformTest {
 public:

   void SetUp() {
     bridge_.reset(new TestBookmarkMenuBridge(browser_test_helper_.profile()));
     EXPECT_TRUE(bridge_.get());
   }

  // We are a friend of BookmarkMenuBridge (and have access to
  // protected methods), but none of the classes generated by TEST_F()
  // are.  This (and AddNodeToMenu()) are simple wrappers to let
  // derived test classes have access to protected methods.
  void ClearBookmarkMenu(BookmarkMenuBridge* bridge, NSMenu* menu) {
    bridge->ClearBookmarkMenu(menu);
  }

  void InvalidateMenu()  { bridge_->InvalidateMenu(); }
  bool menu_is_valid()  { return bridge_->menuIsValid_; }

  void AddNodeToMenu(BookmarkMenuBridge* bridge, const BookmarkNode* root,
                     NSMenu* menu) {
    bridge->AddNodeToMenu(root, menu);
  }

  NSMenuItem* MenuItemForNode(BookmarkMenuBridge* bridge,
                              const BookmarkNode* node) {
    return bridge->MenuItemForNode(node);
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
  scoped_ptr<TestBookmarkMenuBridge> bridge_;
};

TEST_F(BookmarkMenuBridgeTest, TestBookmarkMenuAutoSeparator) {
  BookmarkModel* model = bridge_->GetBookmarkModel();
  bridge_->Loaded(model);
  NSMenu* menu = bridge_->menu_.get();
  bridge_->UpdateMenu(menu);
  // The bare menu after loading has a separator and an "Other Bookmarks"
  // submenu.
  EXPECT_EQ(2, [menu numberOfItems]);
  // Add a bookmark and reload and there should be 4 items: the previous
  // menu contents plus a new separator and the new bookmark.
  const BookmarkNode* parent = model->GetBookmarkBarNode();
  const char* url = "http://www.zim-bop-a-dee.com/";
  model->AddURL(parent, 0, ASCIIToUTF16("Bookmark"), GURL(url));
  bridge_->UpdateMenu(menu);
  EXPECT_EQ(4, [menu numberOfItems]);
  // Remove the new bookmark and reload and we should have 2 items again
  // because the separator should have been removed as well.
  model->Remove(parent, 0);
  bridge_->UpdateMenu(menu);
  EXPECT_EQ(2, [menu numberOfItems]);
}

// Test that ClearBookmarkMenu() removes all bookmark menus.
TEST_F(BookmarkMenuBridgeTest, TestClearBookmarkMenu) {
  NSMenu* menu = bridge_->menu_.get();

  AddItemToMenu(menu, @"hi mom", nil);
  AddItemToMenu(menu, @"not", @selector(openBookmarkMenuItem:));
  NSMenuItem* item = AddItemToMenu(menu, @"hi mom", nil);
  [item setSubmenu:[[[NSMenu alloc] initWithTitle:@"bar"] autorelease]];
  AddItemToMenu(menu, @"not", @selector(openBookmarkMenuItem:));
  AddItemToMenu(menu, @"zippy", @selector(length));
  [menu addItem:[NSMenuItem separatorItem]];

  ClearBookmarkMenu(bridge_.get(), menu);

  // Make sure all bookmark items are removed, all items with
  // submenus removed, and all separator items are gone.
  EXPECT_EQ(2, [menu numberOfItems]);
  for (NSMenuItem *item in [menu itemArray]) {
    EXPECT_NSNE(@"not", [item title]);
  }
}

// Test invalidation
TEST_F(BookmarkMenuBridgeTest, TestInvalidation) {
  BookmarkModel* model = bridge_->GetBookmarkModel();
  bridge_->Loaded(model);

  EXPECT_FALSE(menu_is_valid());
  bridge_->UpdateMenu(bridge_->menu_);
  EXPECT_TRUE(menu_is_valid());

  InvalidateMenu();
  EXPECT_FALSE(menu_is_valid());
  InvalidateMenu();
  EXPECT_FALSE(menu_is_valid());
  bridge_->UpdateMenu(bridge_->menu_);
  EXPECT_TRUE(menu_is_valid());
  bridge_->UpdateMenu(bridge_->menu_);
  EXPECT_TRUE(menu_is_valid());

  const BookmarkNode* parent = model->GetBookmarkBarNode();
  const char* url = "http://www.zim-bop-a-dee.com/";
  model->AddURL(parent, 0, ASCIIToUTF16("Bookmark"), GURL(url));

  EXPECT_FALSE(menu_is_valid());
  bridge_->UpdateMenu(bridge_->menu_);
  EXPECT_TRUE(menu_is_valid());
}

// Test that AddNodeToMenu() properly adds bookmark nodes as menus,
// including the recursive case.
TEST_F(BookmarkMenuBridgeTest, TestAddNodeToMenu) {
  string16 empty;
  NSMenu* menu = bridge_->menu_.get();

  BookmarkModel* model = bridge_->GetBookmarkModel();
  const BookmarkNode* root = model->GetBookmarkBarNode();
  EXPECT_TRUE(model && root);

  const char* short_url = "http://foo/";
  const char* long_url = "http://super-duper-long-url--."
    "that.cannot.possibly.fit.even-in-80-columns"
    "or.be.reasonably-displayed-in-a-menu"
    "without.looking-ridiculous.com/"; // 140 chars total

  // 3 nodes; middle one has a child, last one has a HUGE URL
  // Set their titles to be the same as the URLs
  const BookmarkNode* node = NULL;
  model->AddURL(root, 0, ASCIIToUTF16(short_url), GURL(short_url));
  bridge_->UpdateMenu(menu);
  int prev_count = [menu numberOfItems] - 1; // "extras" added at this point
  node = model->AddGroup(root, 1, empty);
  model->AddURL(root, 2, ASCIIToUTF16(long_url), GURL(long_url));

  // And the submenu fo the middle one
  model->AddURL(node, 0, empty, GURL("http://sub"));
  bridge_->UpdateMenu(menu);

  EXPECT_EQ((NSInteger)(prev_count+3), [menu numberOfItems]);

  // Verify the 1st one is there with the right action.
  NSMenuItem* item = [menu itemWithTitle:[NSString
                                           stringWithUTF8String:short_url]];
  EXPECT_TRUE(item);
  EXPECT_EQ(@selector(openBookmarkMenuItem:), [item action]);
  EXPECT_EQ(NO, [item hasSubmenu]);
  NSMenuItem* short_item = item;
  NSMenuItem* long_item = nil;

  // Now confirm we have 2 submenus (the one we added, plus "other")
  int subs = 0;
  for (item in [menu itemArray]) {
    if ([item hasSubmenu])
      subs++;
  }
  EXPECT_EQ(2, subs);

  for (item in [menu itemArray]) {
    if ([[item title] hasPrefix:@"http://super-duper"]) {
      long_item = item;
      break;
    }
  }
  EXPECT_TRUE(long_item);

  // Make sure a short title looks fine
  NSString* s = [short_item title];
  EXPECT_NSEQ([NSString stringWithUTF8String:short_url], s);

  // Make sure a super-long title gets trimmed
  s = [long_item title];
  EXPECT_TRUE([s length] < strlen(long_url));

  // Confirm tooltips and confirm they are not trimmed (like the item
  // name might be).  Add tolerance for URL fixer-upping;
  // e.g. http://foo becomes http://foo/)
  EXPECT_GE([[short_item toolTip] length], (2*strlen(short_url) - 5));
  EXPECT_GE([[long_item toolTip] length], (2*strlen(long_url) - 5));

  // Make sure the favicon is non-nil (should be either the default site
  // icon or a favicon, if present).
  EXPECT_TRUE([short_item image]);
  EXPECT_TRUE([long_item image]);
}

// Makes sure our internal map of BookmarkNode to NSMenuItem works.
TEST_F(BookmarkMenuBridgeTest, TestGetMenuItemForNode) {
  string16 empty;
  NSMenu* menu = bridge_->menu_.get();

  BookmarkModel* model = bridge_->GetBookmarkModel();
  const BookmarkNode* bookmark_bar = model->GetBookmarkBarNode();
  const BookmarkNode* root = model->AddGroup(bookmark_bar, 0, empty);
  EXPECT_TRUE(model && root);

  model->AddURL(root, 0, ASCIIToUTF16("Test Item"), GURL("http://test"));
  AddNodeToMenu(bridge_.get(), root, menu);
  EXPECT_TRUE(MenuItemForNode(bridge_.get(), root->GetChild(0)));

  model->AddURL(root, 1, ASCIIToUTF16("Test 2"), GURL("http://second-test"));
  AddNodeToMenu(bridge_.get(), root, menu);
  EXPECT_TRUE(MenuItemForNode(bridge_.get(), root->GetChild(0)));
  EXPECT_TRUE(MenuItemForNode(bridge_.get(), root->GetChild(1)));

  const BookmarkNode* removed_node = root->GetChild(0);
  EXPECT_EQ(2, root->GetChildCount());
  model->Remove(root, 0);
  EXPECT_EQ(1, root->GetChildCount());
  bridge_->UpdateMenu(menu);
  EXPECT_FALSE(MenuItemForNode(bridge_.get(), removed_node));
  EXPECT_TRUE(MenuItemForNode(bridge_.get(), root->GetChild(0)));

  const BookmarkNode empty_node(GURL("http://no-where/"));
  EXPECT_FALSE(MenuItemForNode(bridge_.get(), &empty_node));
  EXPECT_FALSE(MenuItemForNode(bridge_.get(), NULL));
}

// Test that Loaded() adds both the bookmark bar nodes and the "other" nodes.
TEST_F(BookmarkMenuBridgeTest, TestAddNodeToOther) {
  NSMenu* menu = bridge_->menu_.get();

  BookmarkModel* model = bridge_->GetBookmarkModel();
  const BookmarkNode* root = model->other_node();
  EXPECT_TRUE(model && root);

  const char* short_url = "http://foo/";
  model->AddURL(root, 0, ASCIIToUTF16(short_url), GURL(short_url));

  bridge_->UpdateMenu(menu);
  ASSERT_GT([menu numberOfItems], 0);
  NSMenuItem* other = [menu itemAtIndex:([menu numberOfItems]-1)];
  EXPECT_TRUE(other);
  EXPECT_TRUE([other hasSubmenu]);
  ASSERT_GT([[other submenu] numberOfItems], 0);
  EXPECT_NSEQ(@"http://foo/", [[[other submenu] itemAtIndex:0] title]);
}

TEST_F(BookmarkMenuBridgeTest, TestFavIconLoading) {
  NSMenu* menu = bridge_->menu_;

  BookmarkModel* model = bridge_->GetBookmarkModel();
  const BookmarkNode* root = model->GetBookmarkBarNode();
  EXPECT_TRUE(model && root);

  const BookmarkNode* node =
      model->AddURL(root, 0, ASCIIToUTF16("Test Item"),
                    GURL("http://favicon-test"));
  bridge_->UpdateMenu(menu);
  NSMenuItem* item = [menu itemWithTitle:@"Test Item"];
  EXPECT_TRUE([item image]);
  [item setImage:nil];
  bridge_->BookmarkNodeFavIconLoaded(model, node);
  EXPECT_TRUE([item image]);
}

TEST_F(BookmarkMenuBridgeTest, TestChangeTitle) {
  NSMenu* menu = bridge_->menu_;
  BookmarkModel* model = bridge_->GetBookmarkModel();
  const BookmarkNode* root = model->GetBookmarkBarNode();
  EXPECT_TRUE(model && root);

  const BookmarkNode* node =
      model->AddURL(root, 0, ASCIIToUTF16("Test Item"),
                    GURL("http://title-test"));
  bridge_->UpdateMenu(menu);
  NSMenuItem* item = [menu itemWithTitle:@"Test Item"];
  EXPECT_TRUE([item image]);

  model->SetTitle(node, ASCIIToUTF16("New Title"));

  item = [menu itemWithTitle:@"Test Item"];
  EXPECT_FALSE(item);
  item = [menu itemWithTitle:@"New Title"];
  EXPECT_TRUE(item);
}

