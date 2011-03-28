// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/automation/testing_automation_provider.h"

#include "base/logging.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/automation/automation_window_tracker.h"

void TestingAutomationProvider::ActivateWindow(int handle) {
  NOTIMPLEMENTED();
}

void TestingAutomationProvider::IsWindowMaximized(int handle,
                                                  bool* is_maximized,
                                                  bool* success) {
  *success = false;
  NOTIMPLEMENTED();
}

void TestingAutomationProvider::TerminateSession(int handle, bool* success) {
  *success = false;
  NOTIMPLEMENTED();
}

#if !defined(TOOLKIT_VIEWS)
void TestingAutomationProvider::WindowGetViewBounds(int handle,
                                                    int view_id,
                                                    bool screen_coordinates,
                                                    bool* success,
                                                    gfx::Rect* bounds) {
  *success = false;
  NOTIMPLEMENTED();
}
#endif

void TestingAutomationProvider::GetWindowBounds(int handle,
                                                gfx::Rect* bounds,
                                                bool* result) {
  *result = false;
  NOTIMPLEMENTED();
}

void TestingAutomationProvider::SetWindowBounds(int handle,
                                                const gfx::Rect& bounds,
                                                bool* success) {
  *success = false;
  NOTIMPLEMENTED();
}

void TestingAutomationProvider::SetWindowVisible(int handle,
                                                 bool visible,
                                                 bool* result) {
  *result = false;
  NOTIMPLEMENTED();
}

void TestingAutomationProvider::GetWindowTitle(int handle, string16* text) {
  NOTIMPLEMENTED();
}
