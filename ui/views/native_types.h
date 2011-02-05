// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_NATIVE_TYPES_H_
#define UI_VIEWS_NATIVE_TYPES_H_
#pragma once

#include "ui/gfx/native_widget_types.h"

namespace ui {

#if defined(OS_WIN)
typedef MSG NativeEvent;
#endif

}  // namespace ui

#endif  // UI_VIEWS_NATIVE_TYPES_H_

