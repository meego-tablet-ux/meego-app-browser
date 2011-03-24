// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_URL_REQUEST_URL_REQUEST_THROTTLER_ENTRY_INTERFACE_H_
#define NET_URL_REQUEST_URL_REQUEST_THROTTLER_ENTRY_INTERFACE_H_
#pragma once

#include "base/basictypes.h"
#include "base/ref_counted.h"
#include "base/time.h"

namespace net {

class URLRequestThrottlerHeaderInterface;

// Interface provided on entries of the URL request throttler manager.
class URLRequestThrottlerEntryInterface
    : public base::RefCountedThreadSafe<URLRequestThrottlerEntryInterface> {
 public:
  URLRequestThrottlerEntryInterface() {}

  // Returns true when we have encountered server errors and are doing
  // exponential back-off.
  // URLRequestHttpJob checks this method prior to every request; it
  // cancels requests if this method returns true.
  virtual bool IsDuringExponentialBackoff() const = 0;

  // Calculates a recommended sending time for the next request and reserves it.
  // The sending time is not earlier than the current exponential back-off
  // release time or |earliest_time|. Moreover, the previous results of
  // the method are taken into account, in order to make sure they are spread
  // properly over time.
  // Returns the recommended delay before sending the next request, in
  // milliseconds. The return value is always positive or 0.
  // Although it is not mandatory, respecting the value returned by this method
  // is helpful to avoid traffic overload.
  virtual int64 ReserveSendingTimeForNextRequest(
      const base::TimeTicks& earliest_time) = 0;

  // Returns the time after which requests are allowed.
  virtual base::TimeTicks GetExponentialBackoffReleaseTime() const = 0;

  // This method needs to be called each time a response is received.
  virtual void UpdateWithResponse(
      const URLRequestThrottlerHeaderInterface* response) = 0;

  // Lets higher-level modules, that know how to parse particular response
  // bodies, notify of receiving malformed content for the given URL. This will
  // be handled by the throttler as if an HTTP 5xx response had been received to
  // the request, i.e. it will count as a failure.
  virtual void ReceivedContentWasMalformed() = 0;

 protected:
  friend class base::RefCountedThreadSafe<URLRequestThrottlerEntryInterface>;
  virtual ~URLRequestThrottlerEntryInterface() {}

 private:
  friend class base::RefCounted<URLRequestThrottlerEntryInterface>;
  DISALLOW_COPY_AND_ASSIGN(URLRequestThrottlerEntryInterface);
};

}  // namespace net

#endif  // NET_URL_REQUEST_URL_REQUEST_THROTTLER_ENTRY_INTERFACE_H_
