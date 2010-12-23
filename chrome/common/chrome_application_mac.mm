// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/common/chrome_application_mac.h"

#include "base/logging.h"

@interface CrApplication ()
- (void)setHandlingSendEvent:(BOOL)handlingSendEvent;
@end

@implementation CrApplication
// Initialize NSApplication using the custom subclass.  Check whether NSApp
// was already initialized using another class, because that would break
// some things.
+ (NSApplication*)sharedApplication {
  NSApplication* app = [super sharedApplication];
  if (![NSApp isKindOfClass:self]) {
    LOG(ERROR) << "NSApp should be of type " << [[self className] UTF8String]
               << ", not " << [[NSApp className] UTF8String];
    DCHECK(false) << "NSApp is of wrong type";
  }
  return app;
}

- (id)init {
  if ((self = [super init])) {
    eventHooks_.reset([[NSMutableArray alloc] init]);
  }
  return self;
}

- (BOOL)isHandlingSendEvent {
  return handlingSendEvent_;
}

- (void)setHandlingSendEvent:(BOOL)handlingSendEvent {
  handlingSendEvent_ = handlingSendEvent;
}

- (void)sendEvent:(NSEvent*)event {
  chrome_application_mac::ScopedSendingEvent sendingEventScoper;
  for (id<CrApplicationEventHookProtocol> handler in eventHooks_.get()) {
    [handler hookForEvent:event];
  }
  [super sendEvent:event];
}

- (void)addEventHook:(id<CrApplicationEventHookProtocol>)handler {
  [eventHooks_ addObject:handler];
}

- (void)removeEventHook:(id<CrApplicationEventHookProtocol>)handler {
  [eventHooks_ removeObject:handler];
}

@end

namespace chrome_application_mac {

ScopedSendingEvent::ScopedSendingEvent()
    : app_(static_cast<CrApplication*>([CrApplication sharedApplication])),
      handling_([app_ isHandlingSendEvent]) {
  [app_ setHandlingSendEvent:YES];
}

ScopedSendingEvent::~ScopedSendingEvent() {
  [app_ setHandlingSendEvent:handling_];
}

void RegisterCrApp() {
  [CrApplication sharedApplication];
}

}  // namespace chrome_application_mac
