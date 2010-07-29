// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/scoped_nsobject.h"
#include "base/utf_string_conversions.h"
#import "chrome/browser/cocoa/task_manager_mac.h"
#import "chrome/browser/cocoa/cocoa_test_helper.h"
#include "grit/generated_resources.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace {

class TestResource : public TaskManager::Resource {
 public:
  TestResource(const string16& title) : title_(title) {}
  virtual std::wstring GetTitle() const { return UTF16ToWide(title_); }
  virtual SkBitmap GetIcon() const { return SkBitmap(); }
  virtual base::ProcessHandle GetProcess() const {
    return base::GetCurrentProcessHandle();
  }
  virtual bool SupportNetworkUsage() const { return false; }
  virtual void SetSupportNetworkUsage() { NOTREACHED(); }
  virtual void Refresh() {}
  string16 title_;
};

}  // namespace

class TaskManagerWindowControllerTest : public CocoaTest {
};

// Test creation, to ensure nothing leaks or crashes.
TEST_F(TaskManagerWindowControllerTest, Init) {
  TaskManager task_manager;
  TaskManagerMac* bridge(new TaskManagerMac(&task_manager));
  TaskManagerWindowController* controller = bridge->cocoa_controller();

  // Releases the controller, which in turn deletes |bridge|.
  [controller close];
}

TEST_F(TaskManagerWindowControllerTest, Sort) {
  TaskManager task_manager;

  TestResource resource1(UTF8ToUTF16("zzz"));
  TestResource resource2(UTF8ToUTF16("zza"));

  task_manager.AddResource(&resource1);
  task_manager.AddResource(&resource2);  // Will be in the same group.

  TaskManagerMac* bridge(new TaskManagerMac(&task_manager));
  TaskManagerWindowController* controller = bridge->cocoa_controller();
  NSTableView* table = [controller tableView];
  ASSERT_EQ(2, [controller numberOfRowsInTableView:table]);

  // Test that table is sorted on title.
  NSTableColumn* title_column = [table tableColumnWithIdentifier:
      [NSNumber numberWithInt:IDS_TASK_MANAGER_PAGE_COLUMN]];
  NSCell* cell;
  cell = [controller tableView:table dataCellForTableColumn:title_column row:0];
  EXPECT_TRUE([@"zza" isEqualToString:[cell title]]);
  cell = [controller tableView:table dataCellForTableColumn:title_column row:1];
  EXPECT_TRUE([@"zzz" isEqualToString:[cell title]]);

  // Releases the controller, which in turn deletes |bridge|.
  [controller close];

  task_manager.RemoveResource(&resource1);
  task_manager.RemoveResource(&resource2);
}
