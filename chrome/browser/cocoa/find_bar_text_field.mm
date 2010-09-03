// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/cocoa/find_bar_text_field.h"

#include "base/logging.h"
#import "chrome/browser/cocoa/find_bar_text_field_cell.h"
#import "chrome/browser/cocoa/view_id_util.h"

@implementation FindBarTextField

+ (Class)cellClass {
  return [FindBarTextFieldCell class];
}

- (void)awakeFromNib {
  DCHECK([[self cell] isKindOfClass:[FindBarTextFieldCell class]]);

  [self registerForDraggedTypes:
          [NSArray arrayWithObjects:NSStringPboardType, nil]];
}

- (FindBarTextFieldCell*)findBarTextFieldCell {
  DCHECK([[self cell] isKindOfClass:[FindBarTextFieldCell class]]);
  return static_cast<FindBarTextFieldCell*>([self cell]);
}

- (ViewID)viewID {
  return VIEW_ID_FIND_IN_PAGE_TEXT_FIELD;
}

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)info {
  // When a drag enters the text field, focus the field.  This will swap in the
  // field editor, which will then handle the drag itself.
  [[self window] makeFirstResponder:self];
  return NSDragOperationNone;
}

@end
