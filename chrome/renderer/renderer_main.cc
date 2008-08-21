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

#include "base/command_line.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "base/platform_thread.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_counters.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/env_util.h"
#include "chrome/common/l10n_util.h"
#include "chrome/common/logging_chrome.h"
#include "chrome/common/resource_bundle.h"
#include "chrome/renderer/render_process.h"
#include "chrome/test/injection_test_dll.h"
#include "sandbox/src/sandbox.h"

#include "generated_resources.h"

// This function provides some ways to test crash and assertion handling
// behavior of the renderer.
static void HandleRendererErrorTestParameters(const CommandLine& command_line) {
  // This parameter causes an assertion.
  if (command_line.HasSwitch(switches::kRendererAssertTest)) {
    DCHECK(false);
  }

  // This parameter causes a null pointer crash (crash reporter trigger).
  if (command_line.HasSwitch(switches::kRendererCrashTest)) {
    int* bad_pointer = NULL;
    *bad_pointer = 0;
  }

  if (command_line.HasSwitch(switches::kRendererStartupDialog)) {
    std::wstring title = l10n_util::GetString(IDS_PRODUCT_NAME);
    title += L" renderer";  // makes attaching to process easier
    MessageBox(NULL, L"renderer starting...", title.c_str(),
               MB_OK | MB_SETFOREGROUND);
  }
}

// mainline routine for running as the Rendererer process
int RendererMain(CommandLine &parsed_command_line, int show_command,
                 sandbox::TargetServices* target_services)
{
  StatsScope<StatsCounterTimer>
      startup_timer(chrome::Counters::renderer_main());

  PlatformThread::SetName(PlatformThread::CurrentId(), "Chrome_RendererMain");

  CoInitialize(NULL);

  DLOG(INFO) << "Started renderer with " <<
    parsed_command_line.command_line_string();

  HMODULE sandbox_test_module = NULL;
  bool no_sandbox = parsed_command_line.HasSwitch(switches::kNoSandbox);
  if (target_services && !no_sandbox) {
    // The command line might specify a test dll to load.
    if (parsed_command_line.HasSwitch(switches::kTestSandbox)) {
      std::wstring test_dll_name =
        parsed_command_line.GetSwitchValue(switches::kTestSandbox);
      sandbox_test_module = LoadLibrary(test_dll_name.c_str());
      DCHECK(sandbox_test_module);
    }
  }

  HandleRendererErrorTestParameters(parsed_command_line);

  std::wstring channel_name =
    parsed_command_line.GetSwitchValue(switches::kProcessChannelID);
  if (RenderProcess::GlobalInit(channel_name)) {
    bool run_loop = true;
    if (!no_sandbox) {
      if (target_services) {
        target_services->LowerToken();
      } else {
        run_loop = false;
      }
    }

    if (sandbox_test_module) {
      RunRendererTests run_security_tests =
          reinterpret_cast<RunRendererTests>(GetProcAddress(sandbox_test_module,
                                                            kRenderTestCall));
      DCHECK(run_security_tests);
      if (run_security_tests) {
        int test_count = 0;
        DLOG(INFO) << "Running renderer security tests";
        BOOL result = run_security_tests(&test_count);
        DCHECK(result) << "Test number " << test_count << " has failed.";
        // If we are in release mode, debug or crash the process.
        if (!result)
          __debugbreak();
      }
    }

    startup_timer.Stop();  // End of Startup Time Measurement.

    if (run_loop) {
      // Load the accelerator table from the browser executable and tell the
      // message loop to use it when translating messages.
      MessageLoop::current()->Run();
    }
  }
  RenderProcess::GlobalCleanup();

  CoUninitialize();
  return 0;
}
