// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/chrome_application_mac.h"

#import "base/scoped_nsobject.h"
#import "chrome/app/breakpad_mac.h"

namespace {

// Helper to make it easy to get crash keys right.
// TODO(shess): Find a better home for this.  app/breakpad_mac.h
// doesn't work.
class ScopedCrashKey {
 public:
  ScopedCrashKey(NSString* key, NSString* value)
      : crash_key_([key retain]) {
    SetCrashKeyValue(crash_key_.get(), value);
  }
  ~ScopedCrashKey() {
    ClearCrashKeyValue(crash_key_.get());
  }

 private:
  scoped_nsobject<NSString> crash_key_;
};

}  // namespace

@implementation CrApplication

// -terminate: is the entry point for orderly "quit" operations in Cocoa.
// This includes the application menu's quit menu item and keyboard
// equivalent, the application's dock icon menu's quit menu item, "quit" (not
// "force quit") in the Activity Monitor, and quits triggered by user logout
// and system restart and shutdown.
//
// The default NSApplication -terminate: implementation will end the process
// by calling exit(), and thus never leave the main run loop.  This is
// unsuitable for Chrome's purposes.  Chrome depends on leaving the main
// run loop to perform a proper orderly shutdown.  This design is ingrained
// in the application and the assumptions that its code makes, and is
// entirely reasonable and works well on other platforms, but it's not
// compatible with the standard Cocoa quit sequence.  Quits originated from
// within the application can be redirected to not use -terminate:, but
// quits from elsewhere cannot be.
//
// To allow the Cocoa-based Chrome to support the standard Cocoa -terminate:
// interface, and allow all quits to cause Chrome to shut down properly
// regardless of their origin, -terminate: is overriden.  The custom
// -terminate: does not end the application with exit().  Instead, it simply
// returns after posting the normal NSApplicationWillTerminateNotification
// notification.  The application is responsible for exiting on its own in
// whatever way it deems appropriate.  In Chrome's case, the main run loop will
// end and the applicaton will exit by returning from main().
//
// This implementation of -terminate: is scaled back and is not as
// fully-featured as the implementation in NSApplication, nor is it a direct
// drop-in replacement -terminate: in most applications.  It is
// purpose-specific to Chrome.
- (void)terminate:(id)sender {
  NSApplicationTerminateReply shouldTerminate = NSTerminateNow;
  SEL selector = @selector(applicationShouldTerminate:);
  if ([[self delegate] respondsToSelector:selector])
    shouldTerminate = [[self delegate] applicationShouldTerminate:self];

  // If shouldTerminate is NSTerminateLater, the application is expected to
  // call -replyToApplicationShouldTerminate: when it knows whether or not it
  // should terminate.  If the argument is YES,
  // -replyToApplicationShouldTerminate: will call -terminate:.  This will
  // result in another call to the delegate's -applicationShouldTerminate:,
  // which would be expected to return NSTerminateNow at that point.
  if (shouldTerminate != NSTerminateNow)
    return;

  [[NSNotificationCenter defaultCenter]
      postNotificationName:NSApplicationWillTerminateNotification
                    object:self];

  // Return, don't exit.  The application is responsible for exiting on its
  // own.
}

- (BOOL)sendAction:(SEL)anAction to:(id)aTarget from:(id)sender {
  // The Dock menu contains an automagic section where you can select
  // amongst open windows.  If a window is closed via JavaScript while
  // the menu is up, the menu item for that window continues to exist.
  // When a window is selected this method is called with the
  // now-freed window as |aTarget|.  Short-circuit the call if
  // |aTarget| is not a valid window.
  if (anAction == @selector(_selectWindow:)) {
    // Not using -[NSArray containsObject:] because |aTarget| may be a
    // freed object.
    BOOL found = NO;
    for (NSWindow* window in [self windows]) {
      if (window == aTarget) {
        found = YES;
        break;
      }
    }
    if (!found) {
      return NO;
    }
  }

  // When a Cocoa control is wired to a freed object, we get crashers
  // in the call to |super| with no useful information in the
  // backtrace.  Attempt to add some useful information.
  static const NSString* kActionKey = @"sendaction";

  // If the action is something generic like -commandDispatch:, then
  // the tag is essential.
  NSInteger tag = 0;
  if ([sender isKindOfClass:[NSControl class]]) {
    tag = [sender tag];
    if (tag == 0 || tag == -1) {
      tag = [sender selectedTag];
    }
  } else if ([sender isKindOfClass:[NSMenuItem class]]) {
    tag = [sender tag];
  }

  NSString* actionString = NSStringFromSelector(anAction);
  NSString* value =
        [NSString stringWithFormat:@"%@ tag %d sending %@ to %p",
                  [sender className], tag, actionString, aTarget];

  ScopedCrashKey key(kActionKey, value);
  return [super sendAction:anAction to:aTarget from:sender];
}

@end

namespace CrApplicationCC {

void Terminate() {
  [NSApp terminate:nil];
}

}  // namespace CrApplicationCC
