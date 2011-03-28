// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_MESSAGE_PUMP_QT_H_
#define BASE_MESSAGE_PUMP_QT_H_

#include <qobject.h>

#include "base/message_pump.h"
#include "base/time.h"

class QSocketNotifier;
class QTimer;

namespace base {

class MessagePumpForUIQt;

class MessagePumpQt : public QObject {
  Q_OBJECT

 public:
  MessagePumpQt(MessagePumpForUIQt &pump);
  ~MessagePumpQt();

  void timeout(int msecs);
  void activate();
  
 public slots:
  void onTimeout();
  void onActivated();
 
 private:
  base::MessagePumpForUIQt &pump_;

  int wakeup_pipe_read_;
  int wakeup_pipe_write_;
  QSocketNotifier* socket_notifier_;
  QTimer* timer_;
};

// This class implements a MessagePump needed for TYPE_UI MessageLoops on
// OS_LINUX platforms using QApplication event loop
class MessagePumpForUIQt : public MessagePump {

 public:
  MessagePumpForUIQt();
  ~MessagePumpForUIQt();

  virtual void Run(Delegate* delegate);
  virtual void Quit();
  virtual void ScheduleWork();
  virtual void ScheduleDelayedWork(const TimeTicks& delayed_work_time);

  // Internal methods used for processing the pump callbacks.  They are
  // public for simplicity but should not be used directly.
  // HandleDispatch is called after the poll has completed.
  void HandleDispatch();
  void HandleTimeout();
  int GetCurrentDelay() const;

 private:
  // We may make recursive calls to Run, so we save state that needs to be
  // separate between them in this structure type.
  struct RunState {
    Delegate* delegate;

    // Used to flag that the current Run() invocation should return ASAP.
    bool should_quit;

    // Used to count how many Run() invocations are on the stack.
    int run_depth;

    // Used internally for controlling whether we want a message pump
    // iteration to be blocking or not.
    bool more_work_is_plausible;
  };

  RunState* state_;

  // This is the time when we need to do delayed work.
  TimeTicks delayed_work_time_;

  // MessagePump implementation for Qt based on the GLib implement.
  // On Qt we use a QObject base class and the
  // default qApp in order to process events through QEventLoop.
  MessagePumpQt qt_pump_;

  DISALLOW_COPY_AND_ASSIGN(MessagePumpForUIQt);
};

}  // namespace base

#endif  // BASE_MESSAGE_PUMP_QT_H_
