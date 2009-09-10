// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_UTIL_EVENT_SYS_H_
#define CHROME_BROWSER_SYNC_UTIL_EVENT_SYS_H_

#include "chrome/browser/sync/util/pthread_helpers_fwd.h"

// An abstract base class for listening to events.
//
// Don't inherit from this class yourself. Using NewEventListenerHookup() is
// much easier.
template <typename EventType>
class EventListener {
 public:
  virtual void HandleEvent(const EventType& event) = 0;
};

// See the -inl.h for details about the following.

template <typename EventTraits, typename NotifyLock = PThreadNoLock,
          typename ScopedNotifyLocker = PThreadScopedLock<NotifyLock> >
class EventChannel;

class EventListenerHookup;

template <typename EventChannel, typename CallbackObject,
          typename CallbackMethod>
EventListenerHookup* NewEventListenerHookup(EventChannel* channel,
                                            CallbackObject* cbobject,
                                            CallbackMethod cbmethod);

template <typename EventChannel, typename CallbackObject,
          typename CallbackMethod, typename CallbackArg0>
EventListenerHookup* NewEventListenerHookup(EventChannel* channel,
                                            CallbackObject* cbobject,
                                            CallbackMethod cbmethod,
                                            CallbackArg0 arg0);

#endif  // CHROME_BROWSER_SYNC_UTIL_EVENT_SYS_H_
