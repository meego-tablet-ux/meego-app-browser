// Copyright (c) 2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browser_main.h"
#include "chrome/browser/browser_main_qt.h"
#include "ui/base/x/x11_util.h"
#include "ui/base/x/x11_util_internal.h"

#include "base/command_line.h"
#include "base/debug/debugger.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/browser_main_win.h"
#include "chrome/browser/metrics/metrics_service.h"
#include "content/common/result_codes.h"

#if defined(USE_LINUX_BREAKPAD)
#include "chrome/app/breakpad_linux.h"
#endif

// Indicates that we're currently responding to an IO error (by shutting down).
bool g_in_x11_io_error_handler = false;

int BrowserX11ErrorHandler(Display* d, XErrorEvent* error) {
  if (!g_in_x11_io_error_handler)
    MessageLoop::current()->PostTask(
        FROM_HERE,
        NewRunnableFunction(ui::LogErrorEventDescription, d, *error));
  return 0;
}

int BrowserX11IOErrorHandler(Display* d) {
  // If there's an IO error it likely means the X server has gone away
  if (!g_in_x11_io_error_handler) {
    g_in_x11_io_error_handler = true;
    LOG(ERROR) << "X IO Error detected";
    BrowserList::SessionEnding();
  }

  return 0;
}

void WillInitializeMainMessageLoop(const MainFunctionParams& parameters) {
}

void DidEndMainMessageLoop() {
}

void RecordBreakpadStatusUMA(MetricsService* metrics) {
#if defined(USE_LINUX_BREAKPAD)
  metrics->RecordBreakpadRegistration(IsCrashReporterEnabled());
#else
  metrics->RecordBreakpadRegistration(false);
#endif
  metrics->RecordBreakpadHasDebugger(base::debug::BeingDebugged());
}

void WarnAboutMinimumSystemRequirements() {
  // Nothing to warn about on GTK right now.
}

// From browser_main_win.h, stubs until we figure out the right thing...

int DoUninstallTasks(bool chrome_still_running) {
  return ResultCodes::NORMAL_EXIT;
}

int HandleIconsCommands(const CommandLine &parsed_command_line) {
  return 0;
}

bool CheckMachineLevelInstall() {
  return false;
}

void PrepareRestartOnCrashEnviroment(const CommandLine &parsed_command_line) {
}

void SetBrowserX11ErrorHandlers() {
  // Set up error handlers to make sure profile gets written if X server
  // goes away.
  ui::SetX11ErrorHandlers(
      BrowserX11ErrorHandler,
      BrowserX11IOErrorHandler);
}

#if !defined(OS_CHROMEOS)
// static
BrowserMainParts* BrowserMainParts::CreateBrowserMainParts(
    const MainFunctionParams& parameters) {
  NOTIMPLEMENTED();
  return NULL;
}
#endif
