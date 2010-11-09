// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_EVENT_EXECUTOR_LINUX_H_
#define REMOTING_HOST_EVENT_EXECUTOR_LINUX_H_

#include "base/scoped_ptr.h"
#include "remoting/host/event_executor.h"

namespace remoting {

class EventExecutorLinuxPimpl;

// A class to generate events on Linux.
class EventExecutorLinux : public EventExecutor {
 public:
  EventExecutorLinux(Capturer* capturer);
  virtual ~EventExecutorLinux();

  virtual void HandleInputEvent(ChromotingClientMessage* message);

 private:
  scoped_ptr<EventExecutorLinuxPimpl> pimpl_;

  DISALLOW_COPY_AND_ASSIGN(EventExecutorLinux);
};

}  // namespace remoting

#endif  // REMOTING_HOST_EVENT_EXECUTOR_LINUX_H_
