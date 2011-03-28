// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_pump_qt.h"

#include <qabstracteventdispatcher.h>
#include <qevent.h>
#include <qapplication.h>
#include <QTimer>
#include <QSocketNotifier>

#include <fcntl.h>
#include <math.h>

#include "base/eintr_wrapper.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/threading/platform_thread.h"

//#define USE_WAKEUP_PIPE

namespace base {

MessagePumpForUIQt::MessagePumpForUIQt()
  : state_(NULL),
    qt_pump_(*this)
{
}

MessagePumpForUIQt::~MessagePumpForUIQt()
{
}

MessagePumpQt::MessagePumpQt(MessagePumpForUIQt &aPump)
  : pump_(aPump)
{
  int fds[2];
  CHECK_EQ(pipe(fds), 0);
  wakeup_pipe_read_  = fds[0];
  wakeup_pipe_write_ = fds[1];

  socket_notifier_ = new QSocketNotifier(wakeup_pipe_read_, QSocketNotifier::Read, this);
  QObject::connect(socket_notifier_, SIGNAL(activated(int)), this, SLOT(onActivated()));

  timer_ = new QTimer(this);
  timer_->setSingleShot(true);
  QObject::connect(timer_, SIGNAL(timeout()), this, SLOT(onTimeout()));
}

MessagePumpQt::~MessagePumpQt()
{
  close(wakeup_pipe_read_);
  close(wakeup_pipe_write_);
  delete socket_notifier_;

  if(timer_->isActive())
    timer_->stop();
  delete timer_;
}

void MessagePumpQt::timeout(int msecs)
 {
  //LOG(INFO) << "MessagePumpQt::timeout " << msecs;
  if(timer_->isActive())
    timer_->stop();
  timer_->start(msecs);
}

void MessagePumpQt::onTimeout()
{
  //LOG(INFO) << "MessagePumpQt::onTimeout";
  pump_.HandleTimeout();
}

void MessagePumpQt::activate()
{
#if !defined(USE_WAKEUP_PIPE)
  QAbstractEventDispatcher *dispatcher = QAbstractEventDispatcher::instance(qApp->thread());
  dispatcher->wakeUp();
#else
  char msg = '!';
  if (HANDLE_EINTR(write(wakeup_pipe_write_, &msg, 1)) != 1) {
    NOTREACHED() << "Could not write to the UI message loop wakeup pipe!";
  }
#endif
}
 
void MessagePumpQt::onActivated()
 {
  char msg;
  if (HANDLE_EINTR(read(wakeup_pipe_read_, &msg, 1)) != 1 || msg != '!') {
    NOTREACHED() << "Error reading from the wakeup pipe.";
  }
  
  pump_.HandleDispatch();
}

void MessagePumpForUIQt::Run(Delegate* delegate) {
  RunState state;
  state.delegate = delegate;
  state.should_quit = false;
  state.run_depth = state_ ? state_->run_depth + 1 : 1;
  // We really only do a single task for each iteration of the loop.  If we
  // have done something, assume there is likely something more to do.  This
  // will mean that we don't block on the message pump until there was nothing
  // more to do.  We also set this to true to make sure not to block on the
  // first iteration of the loop, so RunAllPending() works correctly.
  state.more_work_is_plausible = true;

  RunState* previous_state = state_;
  state_ = &state;

  // We run our own loop instead of using g_main_loop_quit in one of the
  // callbacks.  This is so we only quit our own loops, and we don't quit
  // nested loops run by others.  TODO(deanm): Is this what we want?

  bool more_work_is_plausible = true;

  while (!state_->should_quit) {
    QAbstractEventDispatcher *dispatcher = QAbstractEventDispatcher::instance(qApp->thread());
    if (!dispatcher)
      return;

    QEventLoop::ProcessEventsFlags flags;

    if(more_work_is_plausible)
      flags = QEventLoop::AllEvents;
    else
      flags = QEventLoop::AllEvents | QEventLoop::WaitForMoreEvents;
    
    more_work_is_plausible = dispatcher->processEvents(flags);

    more_work_is_plausible |= state_->delegate->DoWork();
    if (state_->should_quit)
      break;

    more_work_is_plausible |=
        state_->delegate->DoDelayedWork(&delayed_work_time_);
    if (state_->should_quit)
      break;

    if (more_work_is_plausible)
      continue;

    more_work_is_plausible = state_->delegate->DoIdleWork();
    if (state_->should_quit)
      break;

  }

  state_ = previous_state;
}

void MessagePumpForUIQt::HandleDispatch() {
  // We should only ever have a single message on the wakeup pipe, since we
  // are only signaled when the queue went from empty to non-empty.  The qApp
  // poll will tell us whether there was data, so this read shouldn't block.
  if (state_->should_quit)
    return;

  state_->more_work_is_plausible = false;

  if (state_->delegate->DoWork())
    state_->more_work_is_plausible = true;

  if (state_->should_quit)
    return;

  if (state_->delegate->DoDelayedWork(&delayed_work_time_))
    state_->more_work_is_plausible = true;
  if (state_->should_quit)
    return;

  // Don't do idle work if we think there are more important things
  // that we could be doing.
  if (state_->more_work_is_plausible)
    return;

  if (state_->delegate->DoIdleWork())
    state_->more_work_is_plausible = true;
  if (state_->should_quit)
    return;
}

void MessagePumpForUIQt::HandleTimeout()
{
  // If we are being called outside of the context of Run, then don't do
  // anything.  This could correspond to a MessageBox call or something of
  // that sort.
  if (!state_)
    return;

  state_->delegate->DoDelayedWork(&delayed_work_time_);
  if (!delayed_work_time_.is_null()) {
    // A bit gratuitous to set delayed_work_time_ again, but oh well.
    ScheduleDelayedWork(delayed_work_time_);
  }
}
  

void MessagePumpForUIQt::Quit() {
  if (state_) {
    state_->should_quit = true;
  } else {
    NOTREACHED() << "Quit called outside Run!";
  }
}

void MessagePumpForUIQt::ScheduleWork() {
  // This can be called on any thread, so we don't want to touch any state
  // variables as we would then need locks all over.  This ensures that if
  // we are sleeping in a poll that we will wake up.
  qt_pump_.activate();
}

int MessagePumpForUIQt::GetCurrentDelay() const {
  if (delayed_work_time_.is_null())
    return -1;

  // Be careful here.  TimeDelta has a precision of microseconds, but we want a
  // value in milliseconds.  If there are 5.5ms left, should the delay be 5 or
  // 6?  It should be 6 to avoid executing delayed work too early.
  double timeout = ceil((delayed_work_time_ - TimeTicks::Now()).InMillisecondsF());

  // If this value is negative, then we need to run delayed work soon.
  int delay = static_cast<int>(timeout);
  if (delay < 0)
    delay = 0;

  return delay;
}

void MessagePumpForUIQt::ScheduleDelayedWork(const TimeTicks& delayed_work_time) {
  // We need to wake up the loop in case the poll timeout needs to be
  // adjusted.  This will cause us to try to do work, but that's ok.
  delayed_work_time_ = delayed_work_time;

  int delay_msec = GetCurrentDelay();

  qt_pump_.timeout(delay_msec);
}

}  // namespace base
