// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/appcache/appcache_thread.h"

namespace appcache {

// static
int AppCacheThread::db_;
int AppCacheThread::io_;
MessageLoop* AppCacheThread::disk_cache_thread_;

}  // namespace appcache
