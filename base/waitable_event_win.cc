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

#include "base/waitable_event.h"

#include <windows.h>
#include "base/logging.h"

namespace base {

WaitableEvent::WaitableEvent(bool manual_reset, bool signaled)
    : event_(CreateEvent(NULL, manual_reset, signaled, NULL)) {
  // We're probably going to crash anyways if this is ever NULL, so we might as
  // well make our stack reports more informative by crashing here.
  CHECK(event_);
}

WaitableEvent::~WaitableEvent() {
  CloseHandle(event_);
}

void WaitableEvent::Reset() {
  ResetEvent(event_);
}

void WaitableEvent::Signal() {
  SetEvent(event_);
}

bool WaitableEvent::IsSignaled() {
  return TimedWait(TimeDelta::FromMilliseconds(0));
}

bool WaitableEvent::Wait() {
  DWORD result = WaitForSingleObject(event_, INFINITE);
  // It is most unexpected that this should ever fail.  Help consumers learn
  // about it if it should ever fail.
  DCHECK(result == WAIT_OBJECT_0) << "WaitForSingleObject failed";
  return result == WAIT_OBJECT_0;
}

bool WaitableEvent::TimedWait(const TimeDelta& max_time) {
  int32 timeout = static_cast<int32>(max_time.InMilliseconds());
  DWORD result = WaitForSingleObject(event_, timeout);
  switch (result) {
    case WAIT_OBJECT_0:
      return true;
    case WAIT_TIMEOUT:
      return false;
  }
  // It is most unexpected that this should ever fail.  Help consumers learn
  // about it if it should ever fail.
  NOTREACHED() << "WaitForSingleObject failed";
  return false;

}

}  // namespace base
