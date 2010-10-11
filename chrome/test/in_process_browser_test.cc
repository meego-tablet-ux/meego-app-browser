// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/in_process_browser_test.h"

#include "base/command_line.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/path_service.h"
#include "base/scoped_nsautorelease_pool.h"
#include "base/scoped_temp_dir.h"
#include "base/string_number_conversions.h"
#include "base/test/test_file_util.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browser_shutdown.h"
#include "chrome/browser/browser_window.h"
#include "chrome/browser/chrome_thread.h"
#include "chrome/browser/intranet_redirect_detector.h"
#include "chrome/browser/io_thread.h"
#include "chrome/browser/net/url_request_mock_util.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/profile_manager.h"
#include "chrome/browser/renderer_host/render_process_host.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/browser/tabs/tab_strip_model.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/main_function_params.h"
#include "chrome/common/notification_registrar.h"
#include "chrome/common/notification_type.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/test_launcher_utils.h"
#include "chrome/test/testing_browser_process.h"
#include "chrome/test/ui_test_utils.h"
#include "net/base/mock_host_resolver.h"
#include "net/test/test_server.h"
#include "sandbox/src/dep.h"

#if defined(OS_WIN)
#include "chrome/browser/views/frame/browser_view.h"
#endif

#if defined(OS_LINUX)
#include "base/environment.h"
#include "base/singleton.h"
#include "chrome/browser/renderer_host/render_sandbox_host_linux.h"
#include "chrome/browser/zygote_host_linux.h"

namespace {

// A helper class to do Linux-only initialization only once per process.
class LinuxHostInit {
 public:
  LinuxHostInit() {
    RenderSandboxHostLinux* shost = Singleton<RenderSandboxHostLinux>::get();
    shost->Init("");
    ZygoteHost* zhost = Singleton<ZygoteHost>::get();
    zhost->Init("");
  }
  ~LinuxHostInit() {}
};

}  // namespace
#endif

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/cros/cros_library.h"
#endif  // defined(OS_CHROMEOS)

namespace {

void InitializeBrowser(Browser* browser) {
  browser->AddSelectedTabWithURL(GURL(chrome::kAboutBlankURL),
                                 PageTransition::START_PAGE);

  // Wait for the page to finish loading.
  ui_test_utils::WaitForNavigation(
      &browser->GetSelectedTabContents()->controller());

  browser->window()->Show();
}

}  // namespace

extern int BrowserMain(const MainFunctionParams&);

const wchar_t kUnitTestShowWindows[] = L"show-windows";

// Passed as value of kTestType.
static const char kBrowserTestType[] = "browser";

InProcessBrowserTest::InProcessBrowserTest()
    : browser_(NULL),
      test_server_(net::TestServer::TYPE_HTTP,
                   FilePath(FILE_PATH_LITERAL("chrome/test/data"))),
      show_window_(false),
      dom_automation_enabled_(false),
      tab_closeable_state_watcher_enabled_(false),
      original_single_process_(false) {
}

InProcessBrowserTest::~InProcessBrowserTest() {
}

void InProcessBrowserTest::SetUp() {
  // Remember the command line.  Normally this doesn't matter, because the test
  // harness creates a new process for each test, but when the test harness is
  // running in single process mode, we can't let one test's command-line
  // changes (e.g. enabling DOM automation) affect other tests.
  // TODO(phajdan.jr): This save/restore logic is unnecessary.  Remove it.
  CommandLine* command_line = CommandLine::ForCurrentProcessMutable();
  original_command_line_.reset(new CommandLine(*command_line));

  // Update the information about user data directory location before calling
  // BrowserMain().  In some cases there will be no --user-data-dir switch (for
  // example, when debugging).  If there is no switch, do nothing.
  FilePath user_data_dir =
      command_line->GetSwitchValuePath(switches::kUserDataDir);
  if (user_data_dir.empty()) {
    // TODO(rohitrao): Create a ScopedTempDir here if people have problems.
    LOG(ERROR) << "InProcessBrowserTest is using the default user data dir.";
  } else {
    ASSERT_TRUE(test_launcher_utils::OverrideUserDataDir(user_data_dir));
  }

  // The unit test suite creates a testingbrowser, but we want the real thing.
  // Delete the current one. We'll install the testing one in TearDown.
  delete g_browser_process;
  g_browser_process = NULL;

  // Allow subclasses the opportunity to make changes to the default user data
  // dir before running any tests.
  SetUpUserDataDirectory();

  // Don't delete the resources when BrowserMain returns. Many ui classes
  // cache SkBitmaps in a static field so that if we delete the resource
  // bundle we'll crash.
  browser_shutdown::delete_resources_on_shutdown = false;

  // Allow subclasses the opportunity to make changes to the command line before
  // running any tests.
  SetUpCommandLine(command_line);

#if defined(OS_WIN)
  // Hide windows on show.
  if (!command_line->HasSwitch(kUnitTestShowWindows) && !show_window_)
    BrowserView::SetShowState(SW_HIDE);
#endif

  if (dom_automation_enabled_)
    command_line->AppendSwitch(switches::kDomAutomationController);

  command_line->AppendSwitchPath(switches::kUserDataDir, user_data_dir);

  // This is a Browser test.
  command_line->AppendSwitchASCII(switches::kTestType, kBrowserTestType);

  // Single-process mode is not set in BrowserMain so it needs to be processed
  // explicitly.
  original_single_process_ = RenderProcessHost::run_renderer_in_process();
  if (command_line->HasSwitch(switches::kSingleProcess))
    RenderProcessHost::set_run_renderer_in_process(true);

#if defined(OS_WIN)
  // The Windows sandbox requires that the browser and child processes are the
  // same binary.  So we launch browser_process.exe which loads chrome.dll
  command_line->AppendSwitchPath(switches::kBrowserSubprocessPath,
                                 command_line->GetProgram());
#else
  // Explicitly set the path of the binary used for child processes, otherwise
  // they'll try to use browser_tests which doesn't contain ChromeMain.
  FilePath subprocess_path;
  PathService::Get(base::FILE_EXE, &subprocess_path);
  subprocess_path = subprocess_path.DirName();
  subprocess_path = subprocess_path.AppendASCII(WideToASCII(
      chrome::kBrowserProcessExecutablePath));
#if defined(OS_MACOSX)
  // Recreate the real environment, run the helper within the app bundle.
  subprocess_path = subprocess_path.DirName().DirName();
  DCHECK_EQ(subprocess_path.BaseName().value(), "Contents");
  subprocess_path =
      subprocess_path.Append("Versions").Append(chrome::kChromeVersion);
  subprocess_path =
      subprocess_path.Append(chrome::kHelperProcessExecutablePath);
#endif
  command_line->AppendSwitchPath(switches::kBrowserSubprocessPath,
                                 subprocess_path);
#endif

  // If ncecessary, disable TabCloseableStateWatcher.
  if (!tab_closeable_state_watcher_enabled_)
    command_line->AppendSwitch(switches::kDisableTabCloseableStateWatcher);

  test_launcher_utils::PrepareBrowserCommandLineForTests(command_line);

#if defined(OS_CHROMEOS)
  chromeos::CrosLibrary::Get()->GetTestApi()->SetUseStubImpl();
#endif  // defined(OS_CHROMEOS)

  SandboxInitWrapper sandbox_wrapper;
  MainFunctionParams params(*command_line, sandbox_wrapper, NULL);
  params.ui_task =
      NewRunnableMethod(this, &InProcessBrowserTest::RunTestOnMainThreadLoop);

  host_resolver_ = new net::RuleBasedHostResolverProc(
      new IntranetRedirectHostResolverProc(NULL));

  // Something inside the browser does this lookup implicitly. Make it fail
  // to avoid external dependency. It won't break the tests.
  host_resolver_->AddSimulatedFailure("*.google.com");

  // See http://en.wikipedia.org/wiki/Web_Proxy_Autodiscovery_Protocol
  // We don't want the test code to use it.
  host_resolver_->AddSimulatedFailure("wpad");

  net::ScopedDefaultHostResolverProc scoped_host_resolver_proc(
      host_resolver_.get());

  SetUpInProcessBrowserTestFixture();

  // Before we run the browser, we have to hack the path to the exe to match
  // what it would be if Chrome was running, because it is used to fork renderer
  // processes, on Linux at least (failure to do so will cause a browser_test to
  // be run instead of a renderer).
  FilePath chrome_path;
  CHECK(PathService::Get(base::FILE_EXE, &chrome_path));
  chrome_path = chrome_path.DirName();
#if defined(OS_WIN)
  chrome_path = chrome_path.Append(chrome::kBrowserProcessExecutablePath);
#elif defined(OS_POSIX)
  chrome_path = chrome_path.Append(
      WideToASCII(chrome::kBrowserProcessExecutablePath));
#endif
  CHECK(PathService::Override(base::FILE_EXE, chrome_path));

#if defined(OS_LINUX)
  // Initialize the RenderSandbox and Zygote hosts. Apparently they get used
  // for InProcessBrowserTest, and this is not the normal browser startup path.
  Singleton<LinuxHostInit>::get();
#endif

  BrowserMain(params);
  TearDownInProcessBrowserTestFixture();
}

void InProcessBrowserTest::TearDown() {
  // Reinstall testing browser process.
  delete g_browser_process;
  g_browser_process = new TestingBrowserProcess();

  browser_shutdown::delete_resources_on_shutdown = true;

#if defined(OS_WIN)
  BrowserView::SetShowState(-1);
#endif

  *CommandLine::ForCurrentProcessMutable() = *original_command_line_;
  RenderProcessHost::set_run_renderer_in_process(original_single_process_);
}

// Creates a browser with a single tab (about:blank), waits for the tab to
// finish loading and shows the browser.
Browser* InProcessBrowserTest::CreateBrowser(Profile* profile) {
  Browser* browser = Browser::Create(profile);
  InitializeBrowser(browser);
  return browser;
}

Browser* InProcessBrowserTest::CreateBrowserForPopup(Profile* profile) {
  Browser* browser = Browser::CreateForType(Browser::TYPE_POPUP, profile);
  InitializeBrowser(browser);
  return browser;
}

void InProcessBrowserTest::RunTestOnMainThreadLoop() {
#if defined(OS_POSIX)
  // Restore default signal handler for SIGTERM, so when the out-of-process
  // test runner tries to terminate us, we don't catch it and possibly make it
  // look like a success (http://crbug.com/57578).
  signal(SIGTERM, SIG_DFL);
#endif  // defined(OS_POSIX)

  // On Mac, without the following autorelease pool, code which is directly
  // executed (as opposed to executed inside a message loop) would autorelease
  // objects into a higher-level pool. This pool is not recycled in-sync with
  // the message loops' pools and causes problems with code relying on
  // deallocation via an autorelease pool (such as browser window closure and
  // browser shutdown). To avoid this, the following pool is recycled after each
  // time code is directly executed.
  base::ScopedNSAutoreleasePool pool;

  // Pump startup related events.
  MessageLoopForUI::current()->RunAllPending();

  // In the long term it would be great if we could use a TestingProfile
  // here and only enable services you want tested, but that requires all
  // consumers of Profile to handle NULL services.
  Profile* profile = ProfileManager::GetDefaultProfile();
  if (!profile) {
    // We should only be able to get here if the profile already exists and
    // has been created.
    NOTREACHED();
    return;
  }
  pool.Recycle();

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      NewRunnableFunction(chrome_browser_net::SetUrlRequestMocksEnabled, true));

  browser_ = CreateBrowser(profile);
  pool.Recycle();

  // Pump any pending events that were created as a result of creating a
  // browser.
  MessageLoopForUI::current()->RunAllPending();

  RunTestOnMainThread();
  pool.Recycle();

  CleanUpOnMainThread();
  pool.Recycle();

  QuitBrowsers();
  pool.Recycle();
}

void InProcessBrowserTest::QuitBrowsers() {
  if (BrowserList::size() == 0)
    return;

  // Invoke CloseAllBrowsersAndExit on a running message loop.
  // CloseAllBrowsersAndExit exits the message loop after everything has been
  // shut down properly.
  MessageLoopForUI::current()->PostTask(
      FROM_HERE,
      NewRunnableFunction(&BrowserList::CloseAllBrowsersAndExit));
  ui_test_utils::RunMessageLoop();
}
