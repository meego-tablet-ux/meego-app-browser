// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_CHILD_TRACE_MESSAGE_FILTER_H_
#define CONTENT_COMMON_CHILD_TRACE_MESSAGE_FILTER_H_

#include <string>

#include "base/process.h"
#include "ipc/ipc_channel_proxy.h"

// This class sends and receives trace messages on child processes.
class ChildTraceMessageFilter : public IPC::ChannelProxy::MessageFilter {
 public:
  ChildTraceMessageFilter();
  virtual ~ChildTraceMessageFilter();

  // IPC::ChannelProxy::MessageFilter implementation.
  virtual void OnFilterAdded(IPC::Channel* channel);
  virtual bool OnMessageReceived(const IPC::Message& message);

 private:
  // Message handlers.
  void OnBeginTracing();
  void OnEndTracing();
  void OnGetTraceBufferPercentFull();

  // Callback from trace subsystem.
  void OnTraceDataCollected(const std::string& data);
  void OnTraceBufferFull();

  IPC::Channel* channel_;

  DISALLOW_COPY_AND_ASSIGN(ChildTraceMessageFilter);
};


#endif  // CONTENT_COMMON_CHILD_TRACE_MESSAGE_FILTER_H_

