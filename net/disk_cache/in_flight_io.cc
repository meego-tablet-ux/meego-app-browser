// Copyright (c) 2006-2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/disk_cache/in_flight_io.h"

#include "base/logging.h"

namespace disk_cache {

// Runs on the IO thread.
void BackgroundIO::OnIOSignalled() {
  if (controller_)
    controller_->InvokeCallback(this, false);
}

void BackgroundIO::Cancel() {
  DCHECK(controller_);
  controller_ = NULL;
}

// Runs on the background thread.
void BackgroundIO::NotifyController() {
  controller_->OnIOComplete(this);
}

// ---------------------------------------------------------------------------

void InFlightIO::WaitForPendingIO() {
  while (!io_list_.empty()) {
    // Block the current thread until all pending IO completes.
    IOList::iterator it = io_list_.begin();
    InvokeCallback(*it, true);
  }
}

// Runs on a background thread.
void InFlightIO::OnIOComplete(BackgroundIO* operation) {
  callback_thread_->PostTask(FROM_HERE,
                             NewRunnableMethod(operation,
                                               &BackgroundIO::OnIOSignalled));
  operation->io_completed()->Signal();
}

// Runs on the IO thread.
void InFlightIO::InvokeCallback(BackgroundIO* operation, bool cancel_task) {
  operation->io_completed()->Wait();

  if (cancel_task)
    operation->Cancel();

  // Make sure that we remove the operation from the list before invoking the
  // callback (so that a subsequent cancel does not invoke the callback again).
  DCHECK(io_list_.find(operation) != io_list_.end());
  io_list_.erase(operation);
  OnOperationComplete(operation, cancel_task);
}

// Runs on the IO thread.
void InFlightIO::OnOperationPosted(BackgroundIO* operation) {
  io_list_.insert(operation);
}

}  // namespace disk_cache
