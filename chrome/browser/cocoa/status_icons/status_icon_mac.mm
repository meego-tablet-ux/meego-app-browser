// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/cocoa/status_icons/status_icon_mac.h"

#include "base/logging.h"
#include "base/sys_string_conversions.h"
#include "skia/ext/skia_utils_mac.h"
#include "third_party/skia/include/core/SkBitmap.h"

@interface StatusItemController : NSObject {
  StatusIconMac* statusIcon_; // weak
}
- initWithIcon:(StatusIconMac*)icon;
- (void)handleClick:(id)sender;

@end // @interface StatusItemController

@implementation StatusItemController

- (id)initWithIcon:(StatusIconMac*)icon {
  statusIcon_ = icon;
  return self;
}

- (void)handleClick:(id)sender {
  // Pass along the click notification to our owner.
  DCHECK(statusIcon_);
  statusIcon_->HandleClick();
}

@end

StatusIconMac::StatusIconMac()
    : item_(NULL) {
  controller_.reset([[StatusItemController alloc] initWithIcon:this]);
}

StatusIconMac::~StatusIconMac() {
  // Remove the status item from the status bar.
  if (item_)
    [[NSStatusBar systemStatusBar] removeStatusItem:item_];
}

NSStatusItem* StatusIconMac::item() {
  if (!item_.get()) {
    // Create a new status item.
    item_.reset([[[NSStatusBar systemStatusBar]
                  statusItemWithLength:NSSquareStatusItemLength] retain]);
    [item_ setEnabled:YES];
    [item_ setTarget:controller_];
    [item_ setAction:@selector(handleClick:)];
  }
  return item_.get();
}

void StatusIconMac::SetImage(const SkBitmap& bitmap) {
  if (!bitmap.isNull()) {
    NSImage* image = gfx::SkBitmapToNSImage(bitmap);
    if (image)
      [item() setImage:image];
  }
}

void StatusIconMac::SetToolTip(const string16& tool_tip) {
  [item() setToolTip:base::SysUTF16ToNSString(tool_tip)];
}

void StatusIconMac::AddObserver(StatusIconObserver* observer) {
  observers_.push_back(observer);
}

void StatusIconMac::RemoveObserver(StatusIconObserver* observer) {
  std::vector<StatusIconObserver*>::iterator iter =
      std::find(observers_.begin(), observers_.end(), observer);
  if (iter != observers_.end())
    observers_.erase(iter);
}

void StatusIconMac::HandleClick() {
  // Walk observers, call callback for each one.
  for (std::vector<StatusIconObserver*>::const_iterator iter =
           observers_.begin();
       iter != observers_.end();
       ++iter) {
    StatusIconObserver* observer = *iter;
    observer->OnClicked();
  }
}

// static factory method
StatusIcon* StatusIcon::Create() {
  return new StatusIconMac();
}
