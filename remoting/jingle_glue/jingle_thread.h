// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_JINGLE_GLUE_JINGLE_THREAD_H_
#define REMOTING_JINGLE_GLUE_JINGLE_THREAD_H_

#include "base/message_loop.h"
#include "base/tracked_objects.h"
#include "base/synchronization/waitable_event.h"
#include "third_party/libjingle/source/talk/base/messagequeue.h"
#include "third_party/libjingle/source/talk/base/taskrunner.h"
#include "third_party/libjingle/source/talk/base/thread.h"

namespace buzz {
class XmppClient;
}

namespace remoting {

class TaskPump : public talk_base::MessageHandler,
                 public talk_base::TaskRunner {
 public:
  TaskPump();

  // TaskRunner methods.
  virtual void WakeTasks();
  virtual int64 CurrentTime();

  // MessageHandler methods.
  virtual void OnMessage(talk_base::Message* pmsg);
};

// TODO(sergeyu): This class should be changed to inherit from Chromiums
// base::Thread instead of libjingle's thread.
class JingleThread : public talk_base::Thread,
                     public talk_base::MessageHandler {
 public:
  JingleThread();
  virtual ~JingleThread();

  void Start();

  // Main function for the thread. Should not be called directly.
  virtual void Run();

  // Stop the thread.
  virtual void Stop();

  // Returns Chromiums message loop for this thread.
  // TODO(sergeyu): remove this method when we use base::Thread instead of
  // talk_base::Thread
  MessageLoop* message_loop();

  // Returns task pump if the thread is running, otherwise NULL is returned.
  TaskPump* task_pump();

 private:
  class JingleMessageLoop;
  class JingleMessagePump;

  friend class HeartbeatSenderTest;

  virtual void OnMessage(talk_base::Message* msg);

  TaskPump* task_pump_;
  base::WaitableEvent started_event_;
  base::WaitableEvent stopped_event_;
  MessageLoop* message_loop_;

  DISALLOW_COPY_AND_ASSIGN(JingleThread);
};

}  // namespace remoting

#endif  // REMOTING_JINGLE_GLUE_JINGLE_THREAD_H_
