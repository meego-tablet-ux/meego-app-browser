// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/scoped_nsobject.h"
#import "chrome/browser/cocoa/toolbar_view.h"
#import "chrome/browser/cocoa/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class ToolbarViewTest : public testing::Test {
 public:
  ToolbarViewTest() {
    NSRect frame = NSMakeRect(0, 0, 100, 30);
    view_.reset([[ToolbarView alloc] initWithFrame:frame]);
    [cocoa_helper_.contentView() addSubview:view_.get()];
  }

  scoped_nsobject<ToolbarView> view_;
  CocoaTestHelper cocoa_helper_;  // Inits Cocoa, creates window, etc...
};

// Test adding/removing from the view hierarchy, mostly to ensure nothing
// leaks or crashes.
TEST_F(ToolbarViewTest, AddRemove) {
  EXPECT_EQ(cocoa_helper_.contentView(), [view_ superview]);
  [view_.get() removeFromSuperview];
  EXPECT_FALSE([view_ superview]);
}

// Test drawing, mostly to ensure nothing leaks or crashes.
TEST_F(ToolbarViewTest, Display) {
  [view_ display];
}

}  // namespace
