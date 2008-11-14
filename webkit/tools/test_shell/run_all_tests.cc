// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Run all of our test shell tests.  This is just an entry point
// to kick off gTest's RUN_ALL_TESTS().

#include "base/basictypes.h"

#if defined(OS_WIN)
#include <windows.h>
#include <commctrl.h>
#elif defined(OS_LINUX)
#include <gtk/gtk.h>
#endif

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/icu_util.h"
#include "base/message_loop.h"
#include "base/process_util.h"
#include "webkit/tools/test_shell/simple_resource_loader_bridge.h"
#include "webkit/tools/test_shell/test_shell.h"
#include "webkit/tools/test_shell/test_shell_test.h"
#include "testing/gtest/include/gtest/gtest.h"

const char* TestShellTest::kJavascriptDelayExitScript = 
  "<script>"
    "window.layoutTestController.waitUntilDone();"
    "window.addEventListener('load', function() {"
    "  var x = document.body.clientWidth;"  // Force a document layout
    "  window.layoutTestController.notifyDone();"
    "});"
  "</script>";

int main(int argc, char* argv[]) {
  base::EnableTerminationOnHeapCorruption();
  // Some unittests may use base::Singleton<>, thus we need to instanciate
  // the AtExitManager or else we will leak objects.
  base::AtExitManager at_exit_manager;  

#if defined(OS_LINUX)
  gtk_init(&argc, &argv);
#endif

#if defined(OS_POSIX)
  CommandLine::SetArgcArgv(argc, argv);
#endif

  TestShell::InitLogging(true, false);  // suppress error dialogs

#if defined(OS_WIN)
  // Some of the individual tests wind up calling TestShell::WaitTestFinished
  // which has a timeout in it.  For these tests, we don't care about a timeout
  // so just set it to be a really large number.  This is necessary because
  // when running under Purify, we were hitting those timeouts.
  TestShell::SetFileTestTimeout(USER_TIMER_MAXIMUM);
#endif

  // Initialize test shell in non-interactive mode, which will let us load one
  // request than automatically quit.
  TestShell::InitializeTestShell(false);

  // Allocate a message loop for this thread.  Although it is not used
  // directly, its constructor sets up some necessary state.
  MessageLoop main_message_loop;

  // Load ICU data tables
  icu_util::Initialize();

#if defined(OS_WIN)
  INITCOMMONCONTROLSEX InitCtrlEx;

  InitCtrlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
  InitCtrlEx.dwICC  = ICC_STANDARD_CLASSES;
  InitCommonControlsEx(&InitCtrlEx);
#endif

  // Run the actual tests
  testing::InitGoogleTest(&argc, argv);
  int result = RUN_ALL_TESTS();

  TestShell::ShutdownTestShell();
  TestShell::CleanupLogging();

  return result;
}
