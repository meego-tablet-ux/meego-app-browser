// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#import "base/scoped_nsobject.h"
#import "chrome/browser/cocoa/about_ipc_controller.h"
#include "chrome/browser/cocoa/browser_test_helper.h"
#include "chrome/browser/cocoa/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if defined(IPC_MESSAGE_LOG_ENABLED)

namespace {

class AboutIPCControllerTest : public PlatformTest {
  CocoaTestHelper cocoa_helper_;  // Inits Cocoa, creates window, etc...
};

TEST_F(AboutIPCControllerTest, TestFilter) {
  scoped_nsobject<AboutIPCController> controller(
    [[AboutIPCController alloc] init]);
  [controller window];  // force nib load.
  IPC::LogData data;

  // Make sure generic names do NOT get filtered.
  std::wstring names[] = { L"PluginProcessingIsMyLife",
                           L"ViewMsgFoo",
                           L"NPObjectHell" };
  for (unsigned int i = 0; i < arraysize(names); i++) {
    data.message_name = names[i];
    scoped_nsobject<CocoaLogData> cdata([[CocoaLogData alloc]
                                          initWithLogData:data]);
    EXPECT_FALSE([controller filterOut:cdata.get()]);
  }

  // Flip a checkbox, see it filtered, flip back, all is fine.
  data.message_name = L"ViewMsgFoo";
  scoped_nsobject<CocoaLogData> cdata([[CocoaLogData alloc]
                                        initWithLogData:data]);
  [controller setDisplayViewMessages:NO];
  EXPECT_TRUE([controller filterOut:cdata.get()]);
  [controller setDisplayViewMessages:YES];
  EXPECT_FALSE([controller filterOut:cdata.get()]);
}

}  // namespace

#endif  // IPC_MESSAGE_LOG_ENABLED
