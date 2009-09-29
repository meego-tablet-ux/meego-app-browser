// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <AppKit/AppKit.h>
#import "base/scoped_nsobject.h"
#include "chrome/app/chrome_dll_resource.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/cocoa/bookmark_menu_bridge.h"
#include "chrome/browser/cocoa/browser_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
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
    EXPECT_FALSE([[item title] isEqual:@"not"]);
  }
}

// Test that AddNodeToMenu() properly adds bookmark nodes as menus,
// including the recursive case.
TEST_F(BookmarkMenuBridgeTest, TestAddNodeToMenu) {
  std::wstring empty;
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
  model->AddURL(root, 0, ASCIIToWide(short_url), GURL(short_url));
  int prev_count = [menu numberOfItems] - 1; // "extras" added at this point
  node = model->AddGroup(root, 1, empty);
  model->AddURL(root, 2, ASCIIToWide(long_url), GURL(long_url));

  // And the submenu fo the middle one
  model->AddURL(node, 0, empty, GURL("http://sub"));

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
  EXPECT_TRUE([s isEqual:[NSString stringWithUTF8String:short_url]]);

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
  std::wstring empty;
  NSMenu* menu = bridge_->menu_.get();

  BookmarkModel* model = bridge_->GetBookmarkModel();
  const BookmarkNode* bookmark_bar = model->GetBookmarkBarNode();
  const BookmarkNode* root = model->AddGroup(bookmark_bar, 0, empty);
  EXPECT_TRUE(model && root);

  model->AddURL(root, 0, ASCIIToWide("Test Item"), GURL("http://test"));
  AddNodeToMenu(bridge_.get(), root, menu);
  EXPECT_TRUE(MenuItemForNode(bridge_.get(), root->GetChild(0)));

  model->AddURL(root, 1, ASCIIToWide("Test 2"), GURL("http://second-test"));
  AddNodeToMenu(bridge_.get(), root, menu);
  EXPECT_TRUE(MenuItemForNode(bridge_.get(), root->GetChild(0)));
  EXPECT_TRUE(MenuItemForNode(bridge_.get(), root->GetChild(1)));

  const BookmarkNode* removed_node = root->GetChild(0);
  EXPECT_EQ(2, root->GetChildCount());
  model->Remove(root, 0);
  EXPECT_EQ(1, root->GetChildCount());
  EXPECT_FALSE(MenuItemForNode(bridge_.get(), removed_node));
  EXPECT_TRUE(MenuItemForNode(bridge_.get(), root->GetChild(0)));

  const BookmarkNode empty_node(GURL("http://no-where/"));
  EXPECT_FALSE(MenuItemForNode(bridge_.get(), &empty_node));
  EXPECT_FALSE(MenuItemForNode(bridge_.get(), NULL));
}

// Test that Loaded() adds both the bookmark bar nodes and the "other" nodes.
TEST_F(BookmarkMenuBridgeTest, TestAddNodeToOther) {
  std::wstring empty;
  NSMenu* menu = bridge_->menu_.get();

  BookmarkModel* model = bridge_->GetBookmarkModel();
  const BookmarkNode* root = model->other_node();
  EXPECT_TRUE(model && root);

  const char* short_url = "http://foo/";
  model->AddURL(root, 0, ASCIIToWide(short_url), GURL(short_url));

  NSMenuItem* other = [menu itemAtIndex:([menu numberOfItems]-1)];
  EXPECT_TRUE(other);
  EXPECT_TRUE([other hasSubmenu]);
  EXPECT_TRUE([[[[other submenu] itemAtIndex:0] title] isEqual:@"http://foo/"]);
}

TEST_F(BookmarkMenuBridgeTest, TestFavIconLoading) {
  std::wstring empty;
  NSMenu* menu = bridge_->menu_;

  BookmarkModel* model = bridge_->GetBookmarkModel();
  const BookmarkNode* root = model->GetBookmarkBarNode();
  EXPECT_TRUE(model && root);

  const BookmarkNode* node =
      model->AddURL(root, 0, ASCIIToWide("Test Item"),
                    GURL("http://favicon-test"));
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
      model->AddURL(root, 0, L"Test Item",
                    GURL("http://title-test"));
  NSMenuItem* item = [menu itemWithTitle:@"Test Item"];
  EXPECT_TRUE([item image]);

  model->SetTitle(node, L"New Title");

  item = [menu itemWithTitle:@"Test Item"];
  EXPECT_FALSE(item);
  item = [menu itemWithTitle:@"New Title"];
  EXPECT_TRUE(item);
}

