// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_CRASH_HANDLER_LINUX_H_
#define CHROME_RENDERER_CRASH_HANDLER_LINUX_H_

#include "build/build_config.h"

#if defined(OS_LINUX)

extern void EnableRendererCrashDumping();

#endif  // OS_LINUX

#endif  // CHROME_RENDERER_CRASH_HANDLER_LINUX_H_
