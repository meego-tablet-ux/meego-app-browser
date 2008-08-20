// Copyright 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// This file describes a central switchboard for notifications that might
// happen in various parts of the application, and allows users to register
// observers for various classes of events that they're interested in.

#ifndef CHROME_COMMON_NOTIFICATION_SERVICE_H__
#define CHROME_COMMON_NOTIFICATION_SERVICE_H__

#include <map>

#include "base/observer_list.h"
#include "base/thread_local_storage.h"
#include "base/values.h"
#include "chrome/common/notification_details.h"
#include "chrome/common/notification_source.h"
#include "chrome/common/notification_types.h"

class NotificationObserver;

class NotificationService {
 public:
  // Returns the NotificationService object for the current thread, or NULL if
  // none.
  static NotificationService* current() {
    return static_cast<NotificationService *>(tls_index_.Get());
  }

  // Normally instantiated when the thread is created.  Not all threads have
  // a NotificationService.  Only one instance should be created per thread.
  NotificationService();
  ~NotificationService();

  // Registers a NotificationObserver to be called whenever a matching
  // notification is posted.  Observer is a pointer to an object subclassing
  // NotificationObserver to be notified when an event matching the other two
  // parameters is posted to this service.  Type is the type of events to
  // be notified about (or NOTIFY_ALL to receive events of all types).
  // Source is a NotificationSource object (created using
  // "Source<classname>(pointer)"), if this observer only wants to
  // receive events from that object, or NotificationService::AllSources()
  // to receive events from all sources.
  //
  // A given observer can be registered only once for each combination of
  // type and source.  If the same object is registered more than once,
  // it must be removed for each of those combinations of type and source later.
  //
  // The caller retains ownership of the object pointed to by observer.
  void AddObserver(NotificationObserver* observer,
                   NotificationType type, const NotificationSource& source);

  // Removes the object pointed to by observer from receiving notifications
  // that match type and source.  If no object matching the parameters is
  // currently registered, this method is a no-op.
  void RemoveObserver(NotificationObserver* observer,
                      NotificationType type, const NotificationSource& source);

  // Synchronously posts a notification to all interested observers.
  // Source is a reference to a NotificationSource object representing
  // the object originating the notification (can be
  // NotificationService::AllSources(), in which case
  // only observers interested in all sources will be notified).
  // Details is a reference to an object containing additional data about
  // the notification.  If no additional data is needed, NoDetails() is used.
  // There is no particular order in which the observers will be notified.
  void Notify(NotificationType type,
              const NotificationSource& source,
              const NotificationDetails& details);

  // Returns a NotificationSource that represents all notification sources
  // (for the purpose of registering an observer for events from all sources).
  static Source<void> AllSources() { return Source<void>(NULL); }

  // Returns a NotificationDetails object that represents a lack of details
  // associated with a notification.  (This is effectively a null pointer.)
  static Details<void> NoDetails() { return Details<void>(NULL); }

 private:
  typedef ObserverList<NotificationObserver> NotificationObserverList;
  typedef std::map<uintptr_t, NotificationObserverList*> NotificationSourceMap;

  // Convenience function to determine whether a source has a
  // NotificationObserverList in the given map;
  static bool HasKey(const NotificationSourceMap& map,
                     const NotificationSource& source);

  // Keeps track of the observers for each type of notification.
  // Until we get a prohibitively large number of notification types,
  // a simple array is probably the fastest way to dispatch.
  NotificationSourceMap observers_[NOTIFICATION_TYPE_COUNT];

  // The thread local storage index, used for getting the current thread's
  // instance.
  static TLSSlot tls_index_;

#ifndef NDEBUG
  // Used to check to see that AddObserver and RemoveObserver calls are
  // balanced.
  int observer_counts_[NOTIFICATION_TYPE_COUNT];
#endif

  DISALLOW_EVIL_CONSTRUCTORS(NotificationService);
};

// This is the base class for notification observers.  When a matching
// notification is posted to the notification service, Observe is called.
class NotificationObserver {
 public:
  virtual ~NotificationObserver();

  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details) = 0;
};

#endif  // CHROME_COMMON_NOTIFICATION_SERVICE_H__
