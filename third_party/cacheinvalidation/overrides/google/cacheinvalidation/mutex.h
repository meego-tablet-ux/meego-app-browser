// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GOOGLE_CACHEINVALIDATION_MUTEX_H_
#define GOOGLE_CACHEINVALIDATION_MUTEX_H_

#include "base/lock.h"
#include "base/logging.h"

namespace invalidation {

typedef ::Lock Mutex;

class MutexLock {
 public:
  explicit MutexLock(Mutex* m) : auto_lock_(*m) {}

 private:
  AutoLock auto_lock_;
  DISALLOW_COPY_AND_ASSIGN(MutexLock);
};

}  // namespace invalidation

#endif  // GOOGLE_CACHEINVALIDATION_MUTEX_H_
