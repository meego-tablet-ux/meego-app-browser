// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/accessibility/browser_accessibility_mac.h"
#include "chrome/browser/cocoa/browser_accessibility.h"
#include "chrome/browser/cocoa/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"

@interface MockAccessibilityDelegate :
    NSObject<BrowserAccessibilityDelegateCocoa>

- (NSPoint)accessibilityPointInScreen:(BrowserAccessibilityCocoa*)accessibility;
- (void)doDefaultAction:(int32)accessibilityObjectId;
- (void)setAccessibilityFocus:(BOOL)focus
              accessibilityId:(int32)accessibilityObjectId;
- (NSWindow*)window;

@end

@implementation MockAccessibilityDelegate

- (NSPoint)accessibilityPointInScreen:
    (BrowserAccessibilityCocoa*)accessibility {
  return NSZeroPoint;
}
- (void)doDefaultAction:(int32)accessibilityObjectId {
}
- (void)setAccessibilityFocus:(BOOL)focus
              accessibilityId:(int32)accessibilityObjectId {
}
- (NSWindow*)window {
  return nil;
}

@end


class BrowserAccessibilityTest : public CocoaTest {
 public:
  virtual void SetUp() {
    CocoaTest::SetUp();
    WebAccessibility root;
    root.location.x = 0;
    root.location.y = 0;
    root.location.width = 500;
    root.location.height = 100;
    root.attributes[WebAccessibility::ATTR_HELP] = ASCIIToUTF16("HelpText");

    WebAccessibility child1;
    child1.name = ASCIIToUTF16("Child1");
    child1.location.x = 0;
    child1.location.y = 0;
    child1.location.width = 250;
    child1.location.height = 100;

    WebAccessibility child2;
    child2.location.x = 250;
    child2.location.y = 0;
    child2.location.width = 250;
    child2.location.height = 100;

    // TODO(dtseng): use BrowserAccessibilityManagerMac once it manages
    // these objects.
    BrowserAccessibility* rootBrowserAccessibility =
        BrowserAccessibility::Create();
    BrowserAccessibility* child1BrowserAccessibility =
        BrowserAccessibility::Create();
    BrowserAccessibility* child2BrowserAccessibility =
        BrowserAccessibility::Create();

    rootBrowserAccessibility->Initialize(NULL,
                                         NULL,
                                         0,
                                         0,
                                         root);
    child1BrowserAccessibility->Initialize(NULL,
                                           NULL,
                                           0,
                                           0,
                                           child1);
    child2BrowserAccessibility->Initialize(NULL,
                                           NULL,
                                           0,
                                           0,
                                           child2);

    rootBrowserAccessibility->AddChild(child1BrowserAccessibility);
    rootBrowserAccessibility->AddChild(child2BrowserAccessibility);

    delegate_.reset([[MockAccessibilityDelegate alloc] init]);
    accessibility_.reset(
        [[BrowserAccessibilityCocoa alloc]
            initWithObject:rootBrowserAccessibility
            delegate:delegate_
            parent:delegate_]);
  }

 protected:
  scoped_nsobject<MockAccessibilityDelegate> delegate_;
  scoped_nsobject<BrowserAccessibilityCocoa> accessibility_;
};

// Standard hit test.
TEST_F(BrowserAccessibilityTest, HitTestTest) {
  BrowserAccessibilityCocoa* firstChild =
      [accessibility_ accessibilityHitTest:NSMakePoint(50, 50)];
  EXPECT_NSEQ(@"Child1",
      [firstChild accessibilityAttributeValue:NSAccessibilityTitleAttribute]);
}

// Test doing a hit test on the edge of a child.
TEST_F(BrowserAccessibilityTest, EdgeHitTest) {
  BrowserAccessibilityCocoa* firstChild =
      [accessibility_ accessibilityHitTest:NSMakePoint(0, 0)];
  EXPECT_NSEQ(@"Child1",
      [firstChild accessibilityAttributeValue:NSAccessibilityTitleAttribute]);
}

// This will test a hit test with invalid coordinates.  It is assumed that
// the hit test has been narrowed down to this object or one of its children
// so it should return itself since it has no better hit result.
TEST_F(BrowserAccessibilityTest, InvalidHitTestCoordsTest) {
  BrowserAccessibilityCocoa* hitTestResult =
      [accessibility_ accessibilityHitTest:NSMakePoint(-50, 50)];
  EXPECT_NSEQ(accessibility_, hitTestResult);
}

// Test to ensure querying standard attributes works.
TEST_F(BrowserAccessibilityTest, BasicAttributeTest) {
  NSString* helpText = [accessibility_
      accessibilityAttributeValue:NSAccessibilityHelpAttribute];
  EXPECT_NSEQ(@"HelpText", helpText);
}

// Test querying for an invalid attribute to ensure it doesn't crash.
TEST_F(BrowserAccessibilityTest, InvalidAttributeTest) {
  NSString* shouldBeNil = [accessibility_
      accessibilityAttributeValue:@"NSAnInvalidAttribute"];
  EXPECT_TRUE(shouldBeNil == nil);
}
