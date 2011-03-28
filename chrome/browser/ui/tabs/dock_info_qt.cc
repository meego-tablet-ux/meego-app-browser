// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/tabs/dock_info.h"

#include "base/logging.h"
#include "base/task.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/browser_window.h"
#include "ui/gfx/native_widget_types.h"

bool DockInfo::GetWindowBounds(gfx::Rect* bounds) const {
  DNOTIMPLEMENTED();
  return true;
}

void DockInfo::SizeOtherWindowTo(const gfx::Rect& bounds) const {
  DNOTIMPLEMENTED();
}
