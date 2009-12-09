// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "app/hi_res_timer_manager.h"
#include "app/l10n_util.h"
#include "app/resource_bundle.h"
#include "app/system_monitor.h"
#include "base/command_line.h"
#include "base/field_trial.h"
#include "base/histogram.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "base/platform_thread.h"
#include "base/process_util.h"
#include "base/scoped_nsautorelease_pool.h"
#include "base/stats_counters.h"
#include "base/string_util.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_counters.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/logging_chrome.h"
#include "chrome/common/main_function_params.h"
#include "chrome/common/net/net_resource_provider.h"
#include "chrome/renderer/renderer_main_platform_delegate.h"
#include "chrome/renderer/render_process.h"
#include "chrome/renderer/render_thread.h"
#include "grit/generated_resources.h"
#include "net/base/net_module.h"

#if defined(USE_LINUX_BREAKPAD)
#include "chrome/app/breakpad_linux.h"
#endif

#if defined(OS_MACOSX)
#include "ipc/ipc_switches.h"
#include "base/thread.h"
#include "chrome/common/mach_ipc_mac.h"
#endif

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
    ChildProcess::WaitForDebugger(L"Renderer");
  }
}

#if defined(OS_MACOSX)
class MachSendTask : public Task {
 public:
  MachSendTask(const std::string& channel_name) : channel_name_(channel_name) {}

  virtual void Run() {
    // TODO(thakis): Put these somewhere central.
    const int kMachPortMessageID = 57;
    const std::string kMachChannelPrefix = "com.Google.Chrome";

    const int kMachPortMessageSendWaitMs = 5000;
    std::string channel_name = kMachChannelPrefix + channel_name_;
printf("Creating send port %s\n", channel_name.c_str());
    MachPortSender sender(channel_name.c_str());
    MachSendMessage message(kMachPortMessageID);

    // add some ports to be translated for us
    message.AddDescriptor(mach_task_self());
    message.AddDescriptor(mach_host_self());

    kern_return_t result = sender.SendMessage(message,
        kMachPortMessageSendWaitMs);

    // TODO(thakis): Log error somewhere? (don't printf in any case :-P)
    fprintf(stderr, "send result: %lu\n", (unsigned long)result);
    if (result != KERN_SUCCESS)
      fprintf(stderr, "(Failed :-( )\n");
  }
 private:
  std::string channel_name_;
};

class MachSendThread : public base::Thread {
 public:
  MachSendThread() : base::Thread("MachSendThread") {}

  void DoIt() {
    DCHECK(message_loop());
    std::string name = CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
        switches::kProcessChannelID);
printf("main thread: %s\n", name.c_str());
    message_loop()->PostTask(
        FROM_HERE,
        new MachSendTask(name));
  }
};
#endif

// mainline routine for running as the Renderer process
int RendererMain(const MainFunctionParams& parameters) {
  const CommandLine& parsed_command_line = parameters.command_line_;
  base::ScopedNSAutoreleasePool* pool = parameters.autorelease_pool_;

#if defined(USE_LINUX_BREAKPAD)
  // Needs to be called after we have chrome::DIR_USER_DATA.
  InitCrashReporter();
#endif

  // Configure the network module so it has access to resources.
  net::NetModule::SetResourceProvider(chrome_common_net::NetResourceProvider);

  // This function allows pausing execution using the --renderer-startup-dialog
  // flag allowing us to attach a debugger.
  // Do not move this function down since that would mean we can't easily debug
  // whatever occurs before it.
  HandleRendererErrorTestParameters(parsed_command_line);

  RendererMainPlatformDelegate platform(parameters);

  StatsScope<StatsCounterTimer>
      startup_timer(chrome::Counters::renderer_main());

#if defined(OS_MACOSX)
  {
    MachSendThread mach_thread;
    CHECK(mach_thread.Start());
    mach_thread.DoIt();
  }
#endif

#if defined(OS_MACOSX)
  // As long as we use Cocoa in the renderer (for the forseeable future as of
  // now; see http://crbug.com/13890 for info) we need to have a UI loop.
  MessageLoop main_message_loop(MessageLoop::TYPE_UI);
#else
  // The main message loop of the renderer services doesn't have IO or UI tasks,
  // unless in-process-plugins is used.
  MessageLoop main_message_loop(RenderProcess::InProcessPlugins() ?
              MessageLoop::TYPE_UI : MessageLoop::TYPE_DEFAULT);
#endif

  std::wstring app_name = chrome::kBrowserAppName;
  PlatformThread::SetName(WideToASCII(app_name + L"_RendererMain").c_str());

  SystemMonitor system_monitor;
  HighResolutionTimerManager hi_res_timer_manager;

  platform.PlatformInitialize();

  bool no_sandbox = parsed_command_line.HasSwitch(switches::kNoSandbox);
  platform.InitSandboxTests(no_sandbox);

  // Initialize histogram statistics gathering system.
  // Don't create StatisticsRecorder in the single process mode.
  scoped_ptr<StatisticsRecorder> statistics;
  if (!StatisticsRecorder::WasStarted()) {
    statistics.reset(new StatisticsRecorder());
  }

  // Initialize statistical testing infrastructure.
  FieldTrialList field_trial;
  // Ensure any field trials in browser are reflected into renderer.
  if (parsed_command_line.HasSwitch(switches::kForceFieldTestNameAndValue)) {
    std::string persistent(WideToASCII(parsed_command_line.GetSwitchValue(
        switches::kForceFieldTestNameAndValue)));
    bool ret = field_trial.StringAugmentsState(persistent);
    DCHECK(ret);
  }

  {
#if !defined(OS_LINUX)
    // TODO(markus): Check if it is OK to unconditionally move this
    // instruction down.
    RenderProcess render_process;
    render_process.set_main_thread(new RenderThread());
#endif
    bool run_loop = true;
    if (!no_sandbox) {
      run_loop = platform.EnableSandbox();
    }
#if defined(OS_LINUX)
    RenderProcess render_process;
    render_process.set_main_thread(new RenderThread());
#endif

    platform.RunSandboxTests();

    startup_timer.Stop();  // End of Startup Time Measurement.

    if (run_loop) {
      if (pool)
        pool->Recycle();
      MessageLoop::current()->Run();
    }
  }
  platform.PlatformUninitialize();
  return 0;
}
