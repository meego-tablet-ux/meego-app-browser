// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/automation/automation_provider.h"

#include "chrome/browser/automation/ui_controls.h"
#include "chrome/common/automation_messages.h"
#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"

void AutomationProvider::PrintAsync(int tab_handle) {
  NOTIMPLEMENTED();
}

void AutomationProvider::WindowSimulateDrag(
    int handle,
    const std::vector<gfx::Point>& drag_path,
    int flags,
    bool press_escape_en_route,
    IPC::Message* reply_message) {
  NOTIMPLEMENTED();
}
