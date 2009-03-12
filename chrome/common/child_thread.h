// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_CHILD_THREAD_H_
#define CHROME_COMMON_CHILD_THREAD_H_

#include "base/thread.h"
#include "chrome/common/ipc_sync_channel.h"
#include "chrome/common/message_router.h"
#include "chrome/common/resource_dispatcher.h"

// Child processes's background thread should derive from this class.
class ChildThread : public IPC::Channel::Listener,
                    public IPC::Message::Sender,
                    public base::Thread {
 public:
  // Creates the thread.
  ChildThread(Thread::Options options);
  virtual ~ChildThread();

  // IPC::Message::Sender implementation:
  virtual bool Send(IPC::Message* msg);

  // See documentation on MessageRouter for AddRoute and RemoveRoute
  void AddRoute(int32 routing_id, IPC::Channel::Listener* listener);
  void RemoveRoute(int32 routing_id);

  MessageLoop* owner_loop() { return owner_loop_; }

  ResourceDispatcher* resource_dispatcher() {
    return resource_dispatcher_.get();
  }

 protected:
  friend class ChildProcess;

  // Starts the thread.
  bool Run();

  // Overrides the channel name.  Used for --single-process mode.
  void SetChannelName(const std::wstring& name) { channel_name_ = name; }

 protected:
  // The required stack size if V8 runs on a thread.
  static const size_t kV8StackSize;

  virtual void OnControlMessageReceived(const IPC::Message& msg) { }

  // Returns the one child thread.
  static ChildThread* current();

  IPC::SyncChannel* channel() { return channel_.get(); }

  // Thread implementation.
  virtual void Init();
  virtual void CleanUp();

 private:
  // IPC::Channel::Listener implementation:
  virtual void OnMessageReceived(const IPC::Message& msg);
  virtual void OnChannelError();

  // The message loop used to run tasks on the thread that started this thread.
  MessageLoop* owner_loop_;

  std::wstring channel_name_;
  scoped_ptr<IPC::SyncChannel> channel_;

  // Used only on the background render thread to implement message routing
  // functionality to the consumers of the ChildThread.
  MessageRouter router_;

  Thread::Options options_;

  // Handles resource loads for this process.
  // NOTE: this object lives on the owner thread.
  scoped_ptr<ResourceDispatcher> resource_dispatcher_;

  DISALLOW_EVIL_CONSTRUCTORS(ChildThread);
};

#endif  // CHROME_COMMON_CHILD_THREAD_H_
