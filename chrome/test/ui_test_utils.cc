// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/ui_test_utils.h"

#include <vector>

#include "base/json/json_reader.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "base/process_util.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/dom_operation_notification_details.h"
#include "chrome/browser/download/download_manager.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/renderer_host/render_process_host.h"
#include "chrome/browser/renderer_host/render_view_host.h"
#include "chrome/browser/tab_contents/navigation_controller.h"
#include "chrome/browser/tab_contents/navigation_entry.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/extensions/extension_action.h"
#include "chrome/common/notification_registrar.h"
#include "chrome/common/notification_service.h"
#include "chrome/test/automation/javascript_execution_controller.h"
#if defined(TOOLKIT_VIEWS)
#include "views/focus/accelerator_handler.h"
#endif
#include "googleurl/src/gurl.h"
#include "net/base/net_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ui_test_utils {

namespace {

// Used to block until a navigation completes.
class NavigationNotificationObserver : public NotificationObserver {
 public:
  NavigationNotificationObserver(NavigationController* controller,
                                 int number_of_navigations)
      : navigation_started_(false),
        navigations_completed_(0),
        number_of_navigations_(number_of_navigations) {
    registrar_.Add(this, NotificationType::NAV_ENTRY_COMMITTED,
                   Source<NavigationController>(controller));
    registrar_.Add(this, NotificationType::LOAD_START,
                   Source<NavigationController>(controller));
    registrar_.Add(this, NotificationType::LOAD_STOP,
                   Source<NavigationController>(controller));
    RunMessageLoop();
  }

  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details) {
    if (type == NotificationType::NAV_ENTRY_COMMITTED ||
        type == NotificationType::LOAD_START) {
      navigation_started_ = true;
    } else if (type == NotificationType::LOAD_STOP) {
      if (navigation_started_ &&
          ++navigations_completed_ == number_of_navigations_) {
        navigation_started_ = false;
        MessageLoopForUI::current()->Quit();
      }
    }
  }

 private:
  NotificationRegistrar registrar_;

  // If true the navigation has started.
  bool navigation_started_;

  // The number of navigations that have been completed.
  int navigations_completed_;

  // The number of navigations to wait for.
  int number_of_navigations_;

  DISALLOW_COPY_AND_ASSIGN(NavigationNotificationObserver);
};

class DOMOperationObserver : public NotificationObserver {
 public:
  explicit DOMOperationObserver(RenderViewHost* render_view_host)
      : did_respond_(false) {
    registrar_.Add(this, NotificationType::DOM_OPERATION_RESPONSE,
                   Source<RenderViewHost>(render_view_host));
    ui_test_utils::RunMessageLoop();
  }

  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details) {
    DCHECK(type == NotificationType::DOM_OPERATION_RESPONSE);
    Details<DomOperationNotificationDetails> dom_op_details(details);
    response_ = dom_op_details->json();
    did_respond_ = true;
    MessageLoopForUI::current()->Quit();
  }

  bool GetResponse(std::string* response) {
    *response = response_;
    return did_respond_;
  }

  std::string response() const { return response_; }

 private:
  NotificationRegistrar registrar_;
  std::string response_;
  bool did_respond_;

  DISALLOW_COPY_AND_ASSIGN(DOMOperationObserver);
};

// DownloadsCompleteObserver waits for a given number of downloads to complete.
// Example usage:
//
//     ui_test_utils::NavigateToURL(browser(), zip_url);
//     DownloadsCompleteObserver wait_on_download(
//         browser()->profile()->GetDownloadManager(), 1);
//     /* |zip_url| download will be complete by this line. */
//
class DownloadsCompleteObserver : public DownloadManager::Observer,
                                  public DownloadItem::Observer {
 public:
  explicit DownloadsCompleteObserver(DownloadManager* download_manager,
                                    size_t wait_count)
      : download_manager_(download_manager),
        wait_count_(wait_count),
        waiting_(false) {
    download_manager_->AddObserver(this);
  }

  // CheckAllDownloadsComplete will be called when the DownloadManager
  // fires it's ModelChanged() call, and also when incomplete downloads
  // fire their OnDownloadUpdated().
  bool CheckAllDownloadsComplete() {
    if (downloads_.size() < wait_count_)
      return false;

    bool still_waiting = false;
    std::vector<DownloadItem*>::iterator it = downloads_.begin();
    for (; it != downloads_.end(); ++it) {
      // We always remove ourselves as an observer, then re-add if the download
      // isn't complete. This is to avoid having to track which downloads we
      // are currently observing. Removing has no effect if we are not currently
      // an observer.
      (*it)->RemoveObserver(this);
      if ((*it)->state() != DownloadItem::COMPLETE) {
        (*it)->AddObserver(this);
        still_waiting = true;
      }
    }

    if (still_waiting)
      return false;

    download_manager_->RemoveObserver(this);
    // waiting_ will have been set if not all downloads were complete on first
    // pass below in SetDownloads().
    if (waiting_)
      MessageLoopForUI::current()->Quit();
    return true;
  }

  // DownloadItem::Observer
  virtual void OnDownloadUpdated(DownloadItem* download) {
    if (download->state() == DownloadItem::COMPLETE) {
      CheckAllDownloadsComplete();
    }
  }

  virtual void OnDownloadFileCompleted(DownloadItem* download) { }
  virtual void OnDownloadOpened(DownloadItem* download) {}

  // DownloadManager::Observer
  virtual void ModelChanged() {
    download_manager_->GetDownloads(this, L"");
  }

  virtual void SetDownloads(std::vector<DownloadItem*>& downloads) {
    downloads_ = downloads;
    if (CheckAllDownloadsComplete())
      return;

    if (!waiting_) {
      waiting_ = true;
      ui_test_utils::RunMessageLoop();
    }
  }


 private:
  // The observed download manager.
  DownloadManager* download_manager_;

  // The current downloads being tracked.
  std::vector<DownloadItem*> downloads_;

  // The number of downloads to wait on completing.
  size_t wait_count_;

  // Whether an internal message loop has been started and must be quit upon
  // all downloads completing.
  bool waiting_;

  DISALLOW_COPY_AND_ASSIGN(DownloadsCompleteObserver);
};

// Used to block until an application modal dialog is shown.
class AppModalDialogObserver : public NotificationObserver {
 public:
  AppModalDialogObserver() : dialog_(NULL) {}

  AppModalDialog* WaitForAppModalDialog() {
    registrar_.Add(this, NotificationType::APP_MODAL_DIALOG_SHOWN,
                   NotificationService::AllSources());
    dialog_ = NULL;
    ui_test_utils::RunMessageLoop();
    DCHECK(dialog_);
    return dialog_;
  }

  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details) {
    if (type == NotificationType::APP_MODAL_DIALOG_SHOWN) {
      registrar_.Remove(this, NotificationType::APP_MODAL_DIALOG_SHOWN,
                        NotificationService::AllSources());
      dialog_ = Source<AppModalDialog>(source).ptr();
      MessageLoopForUI::current()->Quit();
    } else {
      NOTREACHED();
    }
  }

 private:
  NotificationRegistrar registrar_;

  AppModalDialog* dialog_;

  DISALLOW_COPY_AND_ASSIGN(AppModalDialogObserver);
};

template <class T>
class SimpleNotificationObserver : public NotificationObserver {
 public:
  SimpleNotificationObserver(NotificationType notification_type,
                             T* source) {
    registrar_.Add(this, notification_type, Source<T>(source));
    ui_test_utils::RunMessageLoop();
  }

  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details) {
    MessageLoopForUI::current()->Quit();
  }

 private:
  NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(SimpleNotificationObserver);
};

class LanguageDetectionNotificationObserver : public NotificationObserver {
 public:
  explicit LanguageDetectionNotificationObserver(TabContents* tab) {
    registrar_.Add(this, NotificationType::TAB_LANGUAGE_DETERMINED,
                  Source<TabContents>(tab));
    ui_test_utils::RunMessageLoop();
  }

  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details) {
    language_ = *(Details<std::string>(details).ptr());
    MessageLoopForUI::current()->Quit();
  }

  std::string language() const {
    return language_;
  }

 private:
  NotificationRegistrar registrar_;
  std::string language_;

  DISALLOW_COPY_AND_ASSIGN(LanguageDetectionNotificationObserver);
};

class FindInPageNotificationObserver : public NotificationObserver {
 public:
  explicit FindInPageNotificationObserver(TabContents* parent_tab)
      : parent_tab_(parent_tab),
        active_match_ordinal_(-1),
        number_of_matches_(0) {
    current_find_request_id_ = parent_tab->current_find_request_id();
    registrar_.Add(this, NotificationType::FIND_RESULT_AVAILABLE,
                   Source<TabContents>(parent_tab_));
    ui_test_utils::RunMessageLoop();
  }

  int active_match_ordinal() const { return active_match_ordinal_; }

  int number_of_matches() const { return number_of_matches_; }

  virtual void Observe(NotificationType type, const NotificationSource& source,
                       const NotificationDetails& details) {
    if (type == NotificationType::FIND_RESULT_AVAILABLE) {
      Details<FindNotificationDetails> find_details(details);
      if (find_details->request_id() == current_find_request_id_) {
        // We get multiple responses and one of those will contain the ordinal.
        // This message comes to us before the final update is sent.
        if (find_details->active_match_ordinal() > -1)
          active_match_ordinal_ = find_details->active_match_ordinal();
        if (find_details->final_update()) {
          number_of_matches_ = find_details->number_of_matches();
          MessageLoopForUI::current()->Quit();
        } else {
          DLOG(INFO) << "Ignoring, since we only care about the final message";
        }
      }
    } else {
      NOTREACHED();
    }
  }

 private:
  NotificationRegistrar registrar_;
  TabContents* parent_tab_;
  // We will at some point (before final update) be notified of the ordinal and
  // we need to preserve it so we can send it later.
  int active_match_ordinal_;
  int number_of_matches_;
  // The id of the current find request, obtained from TabContents. Allows us
  // to monitor when the search completes.
  int current_find_request_id_;

  DISALLOW_COPY_AND_ASSIGN(FindInPageNotificationObserver);
};

class InProcessJavaScriptExecutionController
    : public base::RefCounted<InProcessJavaScriptExecutionController>,
      public JavaScriptExecutionController {
 public:
  explicit InProcessJavaScriptExecutionController(
      RenderViewHost* render_view_host)
      : render_view_host_(render_view_host) {}

 protected:
  // Executes |script| and sets the JSON response |json|.
  bool ExecuteJavaScriptAndGetJSON(const std::string& script,
                                   std::string* json) {
    render_view_host_->ExecuteJavascriptInWebFrame(L"", UTF8ToWide(script));
    DOMOperationObserver dom_op_observer(render_view_host_);
    return dom_op_observer.GetResponse(json);
  }

  void FirstObjectAdded() {
    AddRef();
  }

  void LastObjectRemoved() {
    Release();
  }

 private:
  // Weak pointer to the associated RenderViewHost.
  RenderViewHost* render_view_host_;
};

}  // namespace

void RunMessageLoop() {
  MessageLoopForUI* loop = MessageLoopForUI::current();
  bool did_allow_task_nesting = loop->NestableTasksAllowed();
  loop->SetNestableTasksAllowed(true);
#if defined(TOOLKIT_VIEWS)
  views::AcceleratorHandler handler;
  loop->Run(&handler);
#elif defined(OS_LINUX)
  loop->Run(NULL);
#else
  loop->Run();
#endif
  loop->SetNestableTasksAllowed(did_allow_task_nesting);
}

bool GetCurrentTabTitle(const Browser* browser, string16* title) {
  TabContents* tab_contents = browser->GetSelectedTabContents();
  if (!tab_contents)
    return false;
  NavigationEntry* last_entry = tab_contents->controller().GetActiveEntry();
  if (!last_entry)
    return false;
  title->assign(last_entry->title());
  return true;
}

bool WaitForNavigationInCurrentTab(Browser* browser) {
  TabContents* tab_contents = browser->GetSelectedTabContents();
  if (!tab_contents)
    return false;
  WaitForNavigation(&tab_contents->controller());
  return true;
}

bool WaitForNavigationsInCurrentTab(Browser* browser,
                                    int number_of_navigations) {
  TabContents* tab_contents = browser->GetSelectedTabContents();
  if (!tab_contents)
    return false;
  WaitForNavigations(&tab_contents->controller(), number_of_navigations);
  return true;
}

void WaitForNavigation(NavigationController* controller) {
  WaitForNavigations(controller, 1);
}

void WaitForNavigations(NavigationController* controller,
                        int number_of_navigations) {
  NavigationNotificationObserver observer(controller, number_of_navigations);
}

void WaitForNewTab(Browser* browser) {
  SimpleNotificationObserver<Browser>
      new_tab_observer(NotificationType::TAB_ADDED, browser);
}

void WaitForBrowserActionUpdated(ExtensionAction* browser_action) {
  SimpleNotificationObserver<ExtensionAction>
      observer(NotificationType::EXTENSION_BROWSER_ACTION_UPDATED,
               browser_action);
}

void WaitForLoadStop(NavigationController* controller) {
  SimpleNotificationObserver<NavigationController>
      new_tab_observer(NotificationType::LOAD_STOP, controller);
}

void OpenURLOffTheRecord(Profile* profile, const GURL& url) {
  Browser::OpenURLOffTheRecord(profile, url);
  Browser* browser = BrowserList::FindBrowserWithType(
      profile->GetOffTheRecordProfile(), Browser::TYPE_NORMAL);
  WaitForNavigations(&browser->GetSelectedTabContents()->controller(), 1);
}

void NavigateToURL(Browser* browser, const GURL& url) {
  NavigateToURLBlockUntilNavigationsComplete(browser, url, 1);
}

void NavigateToURLBlockUntilNavigationsComplete(Browser* browser,
                                                const GURL& url,
                                                int number_of_navigations) {
  NavigationController* controller =
      &browser->GetSelectedTabContents()->controller();
  browser->OpenURL(url, GURL(), CURRENT_TAB, PageTransition::TYPED);
  WaitForNavigations(controller, number_of_navigations);
}

DOMElementProxyRef GetActiveDOMDocument(Browser* browser) {
  JavaScriptExecutionController* executor =
      new InProcessJavaScriptExecutionController(
          browser->GetSelectedTabContents()->render_view_host());
  DOMElementProxy* main_doc = NULL;
  executor->ExecuteJavaScriptAndParse("document;", &main_doc);
  return main_doc;
}

Value* ExecuteJavaScript(RenderViewHost* render_view_host,
                         const std::wstring& frame_xpath,
                         const std::wstring& original_script) {
  // TODO(jcampan): we should make the domAutomationController not require an
  //                automation id.
  std::wstring script = L"window.domAutomationController.setAutomationId(0);" +
      original_script;
  render_view_host->ExecuteJavascriptInWebFrame(frame_xpath, script);
  DOMOperationObserver dom_op_observer(render_view_host);
  std::string json = dom_op_observer.response();
  // Wrap |json| in an array before deserializing because valid JSON has an
  // array or an object as the root.
  json.insert(0, "[");
  json.append("]");

  scoped_ptr<Value> root_val(base::JSONReader::Read(json, true));
  if (!root_val->IsType(Value::TYPE_LIST))
    return NULL;

  ListValue* list = static_cast<ListValue*>(root_val.get());
  Value* result;
  if (!list || !list->GetSize() ||
      !list->Remove(0, &result))  // Remove gives us ownership of the value.
    return NULL;

  return result;
}

bool ExecuteJavaScriptAndExtractInt(RenderViewHost* render_view_host,
                                    const std::wstring& frame_xpath,
                                    const std::wstring& script,
                                    int* result) {
  DCHECK(result);
  scoped_ptr<Value> value(ExecuteJavaScript(render_view_host, frame_xpath,
                                            script));
  if (!value.get())
    return false;

  return value->GetAsInteger(result);
}

bool ExecuteJavaScriptAndExtractBool(RenderViewHost* render_view_host,
                                     const std::wstring& frame_xpath,
                                     const std::wstring& script,
                                     bool* result) {
  DCHECK(result);
  scoped_ptr<Value> value(ExecuteJavaScript(render_view_host, frame_xpath,
                                            script));
  if (!value.get())
    return false;

  return value->GetAsBoolean(result);
}

bool ExecuteJavaScriptAndExtractString(RenderViewHost* render_view_host,
                                       const std::wstring& frame_xpath,
                                       const std::wstring& script,
                                       std::string* result) {
  DCHECK(result);
  scoped_ptr<Value> value(ExecuteJavaScript(render_view_host, frame_xpath,
                                            script));
  if (!value.get())
    return false;

  return value->GetAsString(result);
}

GURL GetTestUrl(const std::wstring& dir, const std::wstring file) {
  FilePath path;
  PathService::Get(chrome::DIR_TEST_DATA, &path);
  path = path.Append(FilePath::FromWStringHack(dir));
  path = path.Append(FilePath::FromWStringHack(file));
  return net::FilePathToFileURL(path);
}

void WaitForDownloadCount(DownloadManager* download_manager, size_t count) {
  DownloadsCompleteObserver download_observer(download_manager, count);
}

AppModalDialog* WaitForAppModalDialog() {
  AppModalDialogObserver observer;
  return observer.WaitForAppModalDialog();
}

void CrashTab(TabContents* tab) {
  RenderProcessHost* rph = tab->render_view_host()->process();
  base::KillProcess(rph->GetHandle(), 0, false);
  SimpleNotificationObserver<RenderProcessHost>
      crash_observer(NotificationType::RENDERER_PROCESS_CLOSED, rph);
}

void WaitForFocusChange(RenderViewHost* rvh) {
  SimpleNotificationObserver<RenderViewHost>
      focus_observer(NotificationType::FOCUS_CHANGED_IN_PAGE, rvh);
}

void WaitForFocusInBrowser(Browser* browser) {
  SimpleNotificationObserver<Browser>
      focus_observer(NotificationType::FOCUS_RETURNED_TO_BROWSER,
      browser);
}

std::string WaitForLanguageDetection(TabContents* tab) {
  LanguageDetectionNotificationObserver observer(tab);
  return observer.language();
}

int FindInPage(TabContents* tab_contents, const string16& search_string,
               bool forward, bool match_case, int* ordinal) {
  tab_contents->StartFinding(search_string, forward, match_case);
  FindInPageNotificationObserver observer(tab_contents);
  if (ordinal)
    *ordinal = observer.active_match_ordinal();
  return observer.number_of_matches();
}

void RegisterAndWait(NotificationType::Type type,
                     NotificationObserver* observer) {
  NotificationRegistrar registrar;
  registrar.Add(observer, type, NotificationService::AllSources());
  RunMessageLoop();
}

TimedMessageLoopRunner::TimedMessageLoopRunner()
    : loop_(new MessageLoopForUI()),
      owned_(true),
      quit_loop_invoked_(false) {
}

TimedMessageLoopRunner::~TimedMessageLoopRunner() {
  if (owned_)
    delete loop_;
}

void TimedMessageLoopRunner::RunFor(int ms) {
  QuitAfter(ms);
  quit_loop_invoked_ = false;
  loop_->Run();
}

void TimedMessageLoopRunner::Quit() {
  quit_loop_invoked_ = true;
  loop_->PostTask(FROM_HERE, new MessageLoop::QuitTask);
}

void TimedMessageLoopRunner::QuitAfter(int ms) {
  quit_loop_invoked_ = true;
  loop_->PostDelayedTask(FROM_HERE, new MessageLoop::QuitTask, ms);
}

namespace {

void AppendToPythonPath(const FilePath& dir) {
#if defined(OS_WIN)
  const wchar_t kPythonPath[] = L"PYTHONPATH";
  // TODO(ukai): handle longer PYTHONPATH variables.
  wchar_t oldpath[4096];
  if (::GetEnvironmentVariable(kPythonPath, oldpath, arraysize(oldpath)) == 0) {
    ::SetEnvironmentVariableW(kPythonPath, dir.value().c_str());
  } else if (!wcsstr(oldpath, dir.value().c_str())) {
    std::wstring newpath(oldpath);
    newpath.append(L":");
    newpath.append(dir.value());
    SetEnvironmentVariableW(kPythonPath, newpath.c_str());
  }
#elif defined(OS_POSIX)
  const char kPythonPath[] = "PYTHONPATH";
  const char* oldpath = getenv(kPythonPath);
  if (!oldpath) {
    setenv(kPythonPath, dir.value().c_str(), 1);
  } else if (!strstr(oldpath, dir.value().c_str())) {
    std::string newpath(oldpath);
    newpath.append(":");
    newpath.append(dir.value());
    setenv(kPythonPath, newpath.c_str(), 1);
  }
#endif
}

}  // anonymous namespace

TestWebSocketServer::TestWebSocketServer(const FilePath& root_directory) {
  scoped_ptr<CommandLine> cmd_line(CreateWebSocketServerCommandLine());
  cmd_line->AppendSwitchWithValue("server", "start");
  cmd_line->AppendSwitch("register_cygwin");
  cmd_line->AppendSwitchWithValue("root", root_directory.ToWStringHack());
  temp_dir_.CreateUniqueTempDir();
  websocket_pid_file_ = temp_dir_.path().AppendASCII("websocket.pid");
  cmd_line->AppendSwitchWithValue("pidfile",
                                  websocket_pid_file_.ToWStringHack());
  SetPythonPath();
  base::LaunchApp(*cmd_line.get(), true, false, NULL);
}

CommandLine* TestWebSocketServer::CreatePythonCommandLine() {
#if defined(OS_WIN)
  // Get path to python interpreter
  FilePath python_runtime;
  if (!PathService::Get(base::DIR_SOURCE_ROOT, &python_runtime))
    return NULL;
  python_runtime = python_runtime
    .Append(FILE_PATH_LITERAL("third_party"))
    .Append(FILE_PATH_LITERAL("python_24"))
    .Append(FILE_PATH_LITERAL("python.exe"));
  return new CommandLine(python_runtime);
#elif defined(OS_POSIX)
  return new CommandLine(FilePath("python"));
#endif
}

void TestWebSocketServer::SetPythonPath() {
  FilePath scripts_path;
  PathService::Get(base::DIR_SOURCE_ROOT, &scripts_path);

  scripts_path = scripts_path
      .Append(FILE_PATH_LITERAL("third_party"))
      .Append(FILE_PATH_LITERAL("WebKit"))
      .Append(FILE_PATH_LITERAL("WebKitTools"))
      .Append(FILE_PATH_LITERAL("Scripts"));
  AppendToPythonPath(scripts_path);
}

CommandLine* TestWebSocketServer::CreateWebSocketServerCommandLine() {
  FilePath src_path;
  // Get to 'src' dir.
  PathService::Get(base::DIR_SOURCE_ROOT, &src_path);

  FilePath script_path(src_path);
  script_path = script_path.AppendASCII("third_party");
  script_path = script_path.AppendASCII("WebKit");
  script_path = script_path.AppendASCII("WebKitTools");
  script_path = script_path.AppendASCII("Scripts");
  script_path = script_path.AppendASCII("webkitpy");
  script_path = script_path.AppendASCII("layout_tests");
  script_path = script_path.AppendASCII("port");
  script_path = script_path.AppendASCII("websocket_server.py");

  CommandLine* cmd_line = CreatePythonCommandLine();
  cmd_line->AppendLooseValue(script_path.ToWStringHack());
  return cmd_line;
}

TestWebSocketServer::~TestWebSocketServer() {
  scoped_ptr<CommandLine> cmd_line(CreateWebSocketServerCommandLine());
  cmd_line->AppendSwitchWithValue("server", "stop");
  cmd_line->AppendSwitchWithValue("pidfile",
                                  websocket_pid_file_.ToWStringHack());
  base::LaunchApp(*cmd_line.get(), true, false, NULL);
}

}  // namespace ui_test_utils
