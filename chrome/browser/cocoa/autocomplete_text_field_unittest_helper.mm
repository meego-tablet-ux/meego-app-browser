// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/cocoa/autocomplete_text_field_unittest_helper.h"

#import "chrome/browser/cocoa/autocomplete_text_field.h"
#import "chrome/browser/cocoa/autocomplete_text_field_editor.h"
#include "testing/gtest/include/gtest/gtest.h"

@implementation AutocompleteTextFieldWindowTestDelegate

- (id)windowWillReturnFieldEditor:(NSWindow *)sender toObject:(id)anObject {
  EXPECT_TRUE([anObject isKindOfClass:[AutocompleteTextField class]]);

  if (editor_ == nil) {
    editor_.reset([[AutocompleteTextFieldEditor alloc] init]);
  }
  EXPECT_TRUE(editor_ != nil);

  // This needs to be called every time, otherwise notifications
  // aren't sent correctly.
  [editor_ setFieldEditor:YES];
  return editor_;
}

@end
