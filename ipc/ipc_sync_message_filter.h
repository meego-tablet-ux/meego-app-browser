// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IPC_IPC_SYNC_MESSAGE_FILTER_H_
#define IPC_IPC_SYNC_MESSAGE_FILTER_H_

#include "base/basictypes.h"
#include "base/hash_tables.h"
#include "base/lock.h"
#include "base/ref_counted.h"
#include "base/waitable_event.h"
#include "ipc/ipc_channel_proxy.h"
#include "ipc/ipc_sync_message.h"

class MessageLoop;

#if defined(COMPILER_GCC)
// Allows us to use MessageLoop in a hash_map with gcc (MSVC is okay without
// specifying this).
namespace __gnu_cxx {
template<>
struct hash<MessageLoop*> {
  size_t operator()(MessageLoop* message_loop) const {
    return reinterpret_cast<size_t>(message_loop);
  }
};
}
#endif

namespace IPC {

class MessageReplyDeserializer;

// This MessageFilter allows sending synchronous IPC messages from a thread
// other than the listener thread associated with the SyncChannel.  It does not
// support fancy features that SyncChannel does, such as handling recursion or
// receiving messages while waiting for a response.  Note that this object can
// be used to send simultaneous synchronous messages from different threads.
class SyncMessageFilter : public ChannelProxy::MessageFilter,
                          public Message::Sender {
 public:
  explicit SyncMessageFilter(base::WaitableEvent* shutdown_event);
  virtual ~SyncMessageFilter();

  // Message::Sender implementation.
  virtual bool Send(Message* message);

  // ChannelProxy::MessageFilter implementation.
  virtual void OnFilterAdded(Channel* channel);
  virtual void OnChannelError();
  virtual void OnChannelClosing();
  virtual bool OnMessageReceived(const Message& message);

 private:
  void SendOnIOThread(Message* message);
  // Signal all the pending sends as done, used in an error condition.
  void SignalAllEvents();

  // The channel to which this filter was added.
  Channel* channel_;

  MessageLoop* listener_loop_;  // The process's main thread.
  MessageLoop* io_loop_;  // The message loop where the Channel lives.

  typedef base::hash_map<MessageLoop*, PendingSyncMsg*> PendingSyncMessages;
  PendingSyncMessages pending_sync_messages_;

  // Locks data members above.
  Lock lock_;

  base::WaitableEvent* shutdown_event_;

  DISALLOW_COPY_AND_ASSIGN(SyncMessageFilter);
};

}  // namespace IPC

#endif  // IPC_IPC_SYNC_MESSAGE_FILTER_H_
