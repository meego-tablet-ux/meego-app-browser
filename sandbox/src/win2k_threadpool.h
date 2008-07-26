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

#ifndef SANDBOX_SRC_WIN2K_THREADPOOL_H__
#define SANDBOX_SRC_WIN2K_THREADPOOL_H__

#include <list>
#include <algorithm>
#include "sandbox/src/crosscall_server.h"

namespace sandbox {

// Win2kThreadPool a simple implementation of a thread provider as required
// for the sandbox IPC subsystem. See sandbox\crosscall_server.h for the details
// and requirements of this interface.
//
// Implementing the thread provider as a thread pool is desirable in the case
// of shared memory IPC because it can generate a large number of waitable
// events: as many as channels. A thread pool does not create a thread per
// event, instead maintains a few idle threads but can create more if the need
// arises.
//
// This implementation simply thunks to the nice thread pool API of win2k.
class Win2kThreadPool : public  ThreadProvider {
 public:
  Win2kThreadPool() {
    ::InitializeCriticalSection(&lock_);
  }
  ~Win2kThreadPool();

  virtual bool RegisterWait(const void* client, HANDLE waitable_object,
                            CrossCallIPCCallback callback,
                            void* context);

  virtual bool UnRegisterWaits(void* cookie);

  // Returns the total number of non-released wait objects associated with
  // the thread pool.
  size_t OutstandingWaits();

 private:
  // record to keep track of a wait and its associated cookie.
  struct PoolObject {
    const void* cookie;
    HANDLE wait;
  };
  // The list of pool wait objects.
  typedef std::list<PoolObject> PoolObjects;
  PoolObjects pool_objects_;
  // This lock protects the list of pool wait objects.
  CRITICAL_SECTION lock_;
  DISALLOW_EVIL_CONSTRUCTORS(Win2kThreadPool);
};

}  // namespace sandbox

#endif  // SANDBOX_SRC_WIN2K_THREADPOOL_H__
