// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/scoped_nsobject.h"
#import "chrome/browser/cocoa/download_item_button.h"
#import "chrome/browser/cocoa/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

// Make sure nothing leaks.
TEST(DownloadItemButtonTest, Create) {
  scoped_nsobject<DownloadItemButton> button;
  button.reset([[DownloadItemButton alloc]
      initWithFrame:NSMakeRect(0,0,500,500)]);

  // Test setter
  FilePath path("foo");
  [button.get() setDownload:path];
  EXPECT_EQ(path.value(), [button.get() download].value());
}
