// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_TEMP_SCAFFOLDING_STUBS_H_
#define CHROME_COMMON_TEMP_SCAFFOLDING_STUBS_H_

// This file provides declarations and stub definitions for classes we encouter
// during the porting effort. It is not meant to be permanent, and classes will
// be removed from here as they are fleshed out more completely.

#include <list>
#include <string>

#include "base/basictypes.h"
#include "base/clipboard.h"
#include "base/file_path.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/ref_counted.h"
#include "base/gfx/native_widget_types.h"
#include "base/gfx/rect.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/cache_manager_host.h"
#include "chrome/browser/cancelable_request.h"
#include "chrome/browser/download/save_types.h"
#include "chrome/browser/history/download_types.h"
#include "chrome/browser/history/history.h"
#include "chrome/browser/renderer_host/resource_handler.h"
#include "chrome/browser/safe_browsing/safe_browsing_util.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "chrome/browser/search_engines/template_url.h"
#include "chrome/browser/sessions/session_id.h"
#include "chrome/browser/ssl/ssl_error_info.h"
#include "chrome/browser/ssl/ssl_manager.h"
#include "chrome/browser/tab_contents/infobar_delegate.h"
#include "chrome/browser/tab_contents/navigation_entry.h"
#include "chrome/browser/tab_contents/page_navigator.h"
#include "chrome/browser/tab_contents/tab_contents_type.h"
#include "chrome/browser/renderer_host/render_view_host.h"
#include "chrome/browser/renderer_host/render_widget_host.h"
#include "chrome/browser/renderer_host/render_view_host_delegate.h"
#include "chrome/common/child_process_info.h"
#include "chrome/common/navigation_types.h"
#include "chrome/common/notification_service.h"
#include "chrome/common/page_transition_types.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/pref_service.h"
#include "googleurl/src/gurl.h"
#include "net/base/load_states.h"
#include "skia/include/SkBitmap.h"
#include "webkit/glue/password_form.h"
#include "webkit/glue/webplugin.h"
#include "webkit/glue/window_open_disposition.h"

class Browser;
class CommandLine;
class ConstrainedWindow;
class CPCommandInterface;
class DOMUIHost;
class DownloadManager;
class HistoryService;
class LoginHandler;
class MetricsService;
class MixedContentHandler;
class ModalHtmlDialogDelegate;
class NavigationController;
class NavigationEntry;
class NotificationService;
class PluginService;
class ProfileManager;
class Profile;
class RenderProcessHost;
class RenderWidgetHelper;
class ResourceMessageFilter;
class SessionBackend;
class SessionCommand;
class SessionID;
class SiteInstance;
class SpellChecker;
class TabContents;
class TabContentsDelegate;
class TabContentsFactory;
class TabNavigation;
struct ThumbnailScore;
class Task;
class TemplateURL;
class TemplateURLRef;
class URLRequest;
class URLRequestContext;
class UserScriptMaster;
class VisitedLinkMaster;
class WebContents;
class WebContentsView;
struct WebPluginGeometry;
class WebPreferences;

namespace base {
class Thread;
}

namespace IPC {
class Message;
}

namespace net {
class AuthChallengeInfo;
class IOBuffer;
class X509Certificate;
}

//---------------------------------------------------------------------------
// These stubs are for Browser_main()

#if defined(OS_MACOSX)
// TODO(port): needs an implementation of ProcessSingleton.
class ProcessSingleton {
 public:
  explicit ProcessSingleton(const FilePath& user_data_dir) { }
  ~ProcessSingleton() { }
  bool NotifyOtherProcess() {
    NOTIMPLEMENTED();
    return false;
  }
  void HuntForZombieChromeProcesses() { NOTIMPLEMENTED(); }
  void Create() { NOTIMPLEMENTED(); }
  void Lock() { NOTIMPLEMENTED(); }
  void Unlock() { NOTIMPLEMENTED(); }
};
#endif  // defined(OS_MACOSX)

class GoogleUpdateSettings {
 public:
  static bool GetCollectStatsConsent() {
    NOTIMPLEMENTED();
    return false;
  }
  static bool SetCollectStatsConsent(bool consented) {
    NOTIMPLEMENTED();
    return false;
  }
  static bool GetBrowser(std::wstring* browser) {
    NOTIMPLEMENTED();
    return false;
  }
  static bool GetLanguage(std::wstring* language) {
    NOTIMPLEMENTED();
    return false;
  }
  static bool GetBrand(std::wstring* brand) {
    NOTIMPLEMENTED();
    return false;
  }
  static bool GetReferral(std::wstring* referral) {
    NOTIMPLEMENTED();
    return false;
  }
  static bool ClearReferral() {
    NOTIMPLEMENTED();
    return false;
  }
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(GoogleUpdateSettings);
};

class AutomationProviderList {
 public:
  static AutomationProviderList* GetInstance() {
    NOTIMPLEMENTED();
    return NULL;
  }
};

namespace browser {
void RegisterAllPrefs(PrefService*, PrefService*);
}

void OpenFirstRunDialog(Profile* profile);

void InstallJankometer(const CommandLine&);

GURL NewTabUIURL();

//---------------------------------------------------------------------------
// These stubs are for BrowserProcessImpl

class ClipboardService : public Clipboard {
 public:
};

class CancelableTask;
class ViewMsg_Print_Params;

namespace printing {

class PrintingContext {
 public:
  enum Result { OK, CANCEL, FAILED };
};

class PrintSettings {
 public:
  void RenderParams(ViewMsg_Print_Params* params) const { NOTIMPLEMENTED(); }
  int dpi() const { NOTIMPLEMENTED(); return 92; }
};

class PrinterQuery : public base::RefCountedThreadSafe<PrinterQuery> {
 public:
  enum GetSettingsAskParam {
    DEFAULTS,
    ASK_USER,
  };

  void GetSettings(GetSettingsAskParam ask_user_for_settings,
                   int parent_window,
                   int expected_page_count,
                   CancelableTask* callback) { NOTIMPLEMENTED(); }
  PrintingContext::Result last_status() { return PrintingContext::FAILED; }
  const PrintSettings& settings() { NOTIMPLEMENTED(); return settings_; }
  int cookie() { NOTIMPLEMENTED(); return 0; }
  void StopWorker() { NOTIMPLEMENTED(); }

 private:
  PrintSettings settings_;
};

class PrintJobManager {
 public:
  void OnQuit() { NOTIMPLEMENTED(); }
  void PopPrinterQuery(int document_cookie, scoped_refptr<PrinterQuery>* job) {
    NOTIMPLEMENTED();
  }
  void QueuePrinterQuery(PrinterQuery* job) { NOTIMPLEMENTED(); }
};

}  // namespace printing

struct DownloadBuffer {
  Lock lock;
  typedef std::pair<net::IOBuffer*, int> Contents;
  std::vector<Contents> contents;
};

class DownloadItem {
public:
  void Remove(bool delete_file) { NOTIMPLEMENTED(); }
  void Update(int64 bytes_so_far) { NOTIMPLEMENTED(); }
  void Cancel(bool update_history) { NOTIMPLEMENTED(); }
  void Finished(int64 size) { NOTIMPLEMENTED(); }
  void set_total_bytes(int64 total_bytes) { NOTIMPLEMENTED(); }
  enum DownloadState {
    IN_PROGRESS,
    COMPLETE,
    CANCELLED,
    REMOVING
  };
};

class DownloadFileManager
    : public base::RefCountedThreadSafe<DownloadFileManager> {
 public:
  DownloadFileManager(MessageLoop* ui_loop, ResourceDispatcherHost* rdh) {
    NOTIMPLEMENTED();
  }
  void Initialize() { NOTIMPLEMENTED(); }
  void Shutdown() { NOTIMPLEMENTED(); }
  MessageLoop* file_loop() const {
    NOTIMPLEMENTED();
    return NULL;
  }
  int GetNextId() {
    NOTIMPLEMENTED();
    return 0;
  }
  void StartDownload(DownloadCreateInfo* info) { NOTIMPLEMENTED(); }
  void UpdateDownload(int id, DownloadBuffer* buffer) { NOTIMPLEMENTED(); }
  void DownloadFinished(int id, DownloadBuffer* buffer) { NOTIMPLEMENTED(); }
};

class DownloadRequestManager
    : public base::RefCountedThreadSafe<DownloadRequestManager> {
 public:
  DownloadRequestManager(MessageLoop* io_loop, MessageLoop* ui_loop) {
    NOTIMPLEMENTED();
  }
  class Callback {
   public:
    virtual void ContinueDownload() = 0;
    virtual void CancelDownload() = 0;
  };
  void CanDownloadOnIOThread(int render_process_host_id,
                             int render_view_id,
                             Callback* callback) {
    NOTIMPLEMENTED();
  }
};

namespace sandbox {

class BrokerServices {
 public:
  void Init() { NOTIMPLEMENTED(); }
};

}  // namespace sandbox

class IconManager {
};

struct ViewHostMsg_DidPrintPage_Params;

namespace views {

class AcceleratorHandler {
};

class TableModelObserver {
 public:
  virtual void OnModelChanged() = 0;
  virtual void OnItemsChanged(int, int) = 0;
  virtual void OnItemsAdded(int, int) = 0;
  virtual void OnItemsRemoved(int, int) = 0;
};

class TableModel {
 public:
  int CompareValues(int row1, int row2, int column_id) {
    NOTIMPLEMENTED();
    return 0;
  }
  virtual int RowCount() = 0;
};

}  // namespace views

class Menu {
 public:
  class Delegate {
  };
};

//---------------------------------------------------------------------------
// These stubs are for Browser

#if defined(OS_MACOSX)
class StatusBubble {
 public:
  void SetStatus(const std::wstring&) { NOTIMPLEMENTED(); }
  void Hide() { NOTIMPLEMENTED(); }
  void SetURL(const GURL&, const std::wstring&) { NOTIMPLEMENTED(); }
};
#endif

class DebuggerWindow : public base::RefCountedThreadSafe<DebuggerWindow> {
 public:
};

class FaviconStatus {
 public:
  const GURL& url() const { return url_; }
 private:
  GURL url_;
};

class TabContents : public PageNavigator, public NotificationObserver {
 public:
  enum InvalidateTypes {
    INVALIDATE_URL = 1,
    INVALIDATE_TITLE = 2,
    INVALIDATE_FAVICON = 4,
    INVALIDATE_LOAD = 8,
    INVALIDATE_EVERYTHING = 0xFFFFFFFF
  };
  TabContents(TabContentsType type) 
      : type_(type), is_active_(true), is_loading_(false),
        is_being_destroyed_(false), controller_(),
        delegate_(), max_page_id_(-1) { }
  virtual ~TabContents() { }
  NavigationController* controller() const { return controller_; }
  void set_controller(NavigationController* c) { controller_ = c; }
  virtual WebContents* AsWebContents() { return NULL; }
  WebContents* AsWebContents() const {
    return const_cast<TabContents*>(this)->AsWebContents();
  }
  virtual SkBitmap GetFavIcon() const;
  const GURL& GetURL() const;
  virtual const std::wstring& GetTitle() const;
  TabContentsType type() const { return type_; }
  void set_type(TabContentsType type) { type_ = type; }
  virtual void Focus() { NOTIMPLEMENTED(); }
  virtual void Stop() { NOTIMPLEMENTED(); }
  Profile* profile() const;
  virtual void CloseContents();
  virtual void SetupController(Profile* profile);
  bool WasHidden() {
    NOTIMPLEMENTED();
    return false;
  }
  virtual void RestoreFocus() { NOTIMPLEMENTED(); }
  static TabContentsType TypeForURL(GURL* url);
  static TabContents* CreateWithType(TabContentsType type,
                                     Profile* profile,
                                     SiteInstance* instance);
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details) { NOTIMPLEMENTED(); }
  virtual void DidBecomeSelected() { NOTIMPLEMENTED(); }
  virtual void SetDownloadShelfVisible(bool) { NOTIMPLEMENTED(); }
  virtual void Destroy();
  virtual void SetIsLoading(bool, LoadNotificationDetails*);
  virtual void SetIsCrashed(bool) { NOTIMPLEMENTED(); }
  bool capturing_contents() const {
    NOTIMPLEMENTED();
    return false;
  }
  void set_capturing_contents(bool) { NOTIMPLEMENTED(); }
  bool is_active() const { return is_active_; }
  void set_is_active(bool active) { is_active_ = active; }
  bool is_loading() const { return is_loading_; }
  bool is_being_destroyed() const { return is_being_destroyed_; }
  void SetNotWaitingForResponse() { NOTIMPLEMENTED(); }
  void NotifyNavigationStateChanged(unsigned int);
  TabContentsDelegate* delegate() const { return delegate_; }
  void set_delegate(TabContentsDelegate* d) { delegate_ = d; }
  void AddInfoBar(InfoBarDelegate*) { NOTIMPLEMENTED(); }
  virtual void OpenURL(const GURL&, const GURL&, WindowOpenDisposition,
               PageTransition::Type);
  void AddNewContents(TabContents* new_contents,
                      WindowOpenDisposition disposition,
                      const gfx::Rect& initial_pos,
                      bool user_gesture) { NOTIMPLEMENTED(); }
  virtual void Activate() { NOTIMPLEMENTED(); }
  virtual bool SupportsURL(GURL*);
  virtual SiteInstance* GetSiteInstance() const { return NULL; }
  int32 GetMaxPageID();
  void UpdateMaxPageID(int32);
  virtual bool NavigateToPendingEntry(bool) { NOTIMPLEMENTED(); return true; }
  virtual DOMUIHost* AsDOMUIHost() { NOTIMPLEMENTED(); return NULL; }
  virtual std::wstring GetStatusText() const { return std::wstring(); }
  static void RegisterUserPrefs(PrefService* prefs) {
    prefs->RegisterBooleanPref(prefs::kBlockPopups, false);
  }
  static void MigrateShelfView(TabContents* from, TabContents* to) {
    NOTIMPLEMENTED();
  }
  virtual void CreateView() {}
  virtual gfx::NativeView GetNativeView() const { return NULL; }
  static TabContentsFactory* RegisterFactory(TabContentsType type,
                                             TabContentsFactory* factory);
 protected:
  typedef std::vector<ConstrainedWindow*> ConstrainedWindowList;
  ConstrainedWindowList child_windows_;
 private:
  TabContentsType type_;
  bool is_active_;
  bool is_loading_;
  bool is_being_destroyed_;
  GURL url_;
  std::wstring title_;
  NavigationController* controller_;
  TabContentsDelegate* delegate_;
  int32 max_page_id_;
};

class SelectFileDialog : public base::RefCountedThreadSafe<SelectFileDialog> {
 public:
  enum Type {
    SELECT_FOLDER,
    SELECT_SAVEAS_FILE,
    SELECT_OPEN_FILE,
    SELECT_OPEN_MULTI_FILE
  };
  class Listener {
   public:
  };
  void ListenerDestroyed() { NOTIMPLEMENTED(); }
  void SelectFile(Type, const std::wstring&, const std::wstring&,
                  const std::wstring&, const std::wstring&, gfx::NativeWindow,
                  void*) { NOTIMPLEMENTED(); }
  static SelectFileDialog* Create(WebContents*) {
    NOTIMPLEMENTED();
    return new SelectFileDialog;
  }
};

class DockInfo {
 public:
  bool GetNewWindowBounds(gfx::Rect*, bool*) const {
    NOTIMPLEMENTED();
    return false;
  }
  void AdjustOtherWindowBounds() const { NOTIMPLEMENTED(); }
};

class ToolbarModel {
 public:
};

class WindowSizer {
 public:
  static void GetBrowserWindowBounds(const std::wstring& app_name,
                                     const gfx::Rect& specified_bounds,
                                     gfx::Rect* window_bounds,
                                     bool* maximized) { NOTIMPLEMENTED(); }
};

//---------------------------------------------------------------------------
// These stubs are for Profile

class DownloadManager : public base::RefCountedThreadSafe<DownloadManager> {
 public:
  bool Init(Profile* profile) {
    NOTIMPLEMENTED();
    return true;
  }
  void DownloadUrl(const GURL& url, const GURL& referrer,
                   WebContents* web_contents) { NOTIMPLEMENTED(); }
  int RemoveDownloadsBetween(const base::Time remove_begin,
                             const base::Time remove_end) {
    NOTIMPLEMENTED();
    return 0;
  }
  void ClearLastDownloadPath() { NOTIMPLEMENTED(); }
  int in_progress_count() {
    NOTIMPLEMENTED();
    return 0;
  }
  void GenerateSafeFilename(const std::string& mime_type,
                            FilePath* file_name) {
    NOTIMPLEMENTED();
  }
};

class TemplateURLFetcher {
 public:
  explicit TemplateURLFetcher(Profile* profile) { }
  bool Init(Profile* profile) {
    NOTIMPLEMENTED();
    return true;
  }
  void ScheduleDownload(const std::wstring&, const GURL&, const GURL&,
                        const gfx::NativeView, bool) { NOTIMPLEMENTED(); }
};

namespace base {
class SharedMemory;
}

class Encryptor {
 public:
  static bool EncryptWideString(const std::wstring& plaintext,
                                std::string* ciphertext) {
    NOTIMPLEMENTED();
    return false;
  }

  static bool DecryptWideString(const std::string& ciphertext,
                                std::wstring* plaintext) {
    NOTIMPLEMENTED();
    return false;
  }
};

class SpellChecker : public base::RefCountedThreadSafe<SpellChecker> {
 public:
  typedef std::wstring Language;
  typedef std::vector<Language> Languages;
  SpellChecker(const std::wstring& dict_dir,
               const Language& language,
               URLRequestContext* request_context,
               const std::wstring& custom_dictionary_file_name) {}

  bool SpellCheckWord(const wchar_t* in_word,
                     int in_word_len,
                     int* misspelling_start,
                     int* misspelling_len,
                    std::vector<std::wstring>* optional_suggestions) {
    NOTIMPLEMENTED();
    return true;
  }
  static int GetSpellCheckLanguagesToDisplayInContextMenu(
      Profile* profile,
      Languages* display_languages) {
    NOTIMPLEMENTED();
    return 0;
  }
};

class WebAppLauncher {
 public:
  static void Launch(Profile* profile, const GURL& url) {
    NOTIMPLEMENTED();
  }
};

//---------------------------------------------------------------------------
// These stubs are for WebContents

class WebApp : public base::RefCountedThreadSafe<WebApp> {
 public:
  class Observer {
   public:
  };
  void AddObserver(Observer* obs) { NOTIMPLEMENTED(); }
  void RemoveObserver(Observer* obs) { NOTIMPLEMENTED(); }
  void SetWebContents(WebContents*) { NOTIMPLEMENTED(); }
  SkBitmap GetFavIcon() {
    NOTIMPLEMENTED();
    return SkBitmap();
  }
};

namespace printing {
class PrintViewManager {
 public:
  PrintViewManager(WebContents&) { }
  void Stop() { NOTIMPLEMENTED(); }
  void Destroy() { NOTIMPLEMENTED(); }
  bool OnRendererGone(RenderViewHost*) {
    NOTIMPLEMENTED();
    return true;  // Assume for now that all renderer crashes are important.
  }
  void DidGetPrintedPagesCount(int, int) { NOTIMPLEMENTED(); }
  void DidPrintPage(const ViewHostMsg_DidPrintPage_Params&) {
    NOTIMPLEMENTED();
  }
};
}

class PluginInstaller {
 public:
  PluginInstaller(WebContents*) { }
};

class ChildProcessHost : public ChildProcessInfo {
 public:
  class Iterator {
   public:
    explicit Iterator(ProcessType type) { NOTIMPLEMENTED(); }
    ChildProcessInfo* operator->() { return *iterator_; }
    ChildProcessInfo* operator*() { return *iterator_; }
    ChildProcessInfo* operator++() { return NULL; }
    bool Done() {
      NOTIMPLEMENTED();
      return true;
    }
   private:
    std::list<ChildProcessInfo*>::iterator iterator_;
  };
 protected:
  ChildProcessHost(ProcessType type, MessageLoop* main_message_loop)
      : ChildProcessInfo(type) {
    NOTIMPLEMENTED();
  }
};

class PluginProcessHost : public ChildProcessHost {
 public:
  explicit PluginProcessHost(MessageLoop* main_message_loop)
      : ChildProcessHost(PLUGIN_PROCESS, main_message_loop) {
    NOTIMPLEMENTED();
  }
  bool Init(const WebPluginInfo& info,
            const std::string& activex_clsid,
            const std::wstring& locale) {
    NOTIMPLEMENTED();
    return false;
  }
  void OpenChannelToPlugin(ResourceMessageFilter* renderer_message_filter,
                           const std::string& mime_type,
                           IPC::Message* reply_msg) {
    NOTIMPLEMENTED();
  }
  static void ReplyToRenderer(ResourceMessageFilter* renderer_message_filter,
                              const std::wstring& channel,
                              const FilePath& plugin_path,
                              IPC::Message* reply_msg) {
    NOTIMPLEMENTED();
  }
  void Shutdown() { NOTIMPLEMENTED(); }
  const WebPluginInfo& info() const { return info_; }
 private:
  WebPluginInfo info_;
  DISALLOW_EVIL_CONSTRUCTORS(PluginProcessHost);
};

class HungRendererWarning {
 public:
  static void HideForWebContents(WebContents*) { NOTIMPLEMENTED(); }
  static void ShowForWebContents(WebContents*) { NOTIMPLEMENTED(); }
};

class ConstrainedWindow {
 public:
  bool WasHidden() {
    NOTIMPLEMENTED();
    return false;
  }
  void DidBecomeSelected() { NOTIMPLEMENTED(); }
  void CloseConstrainedWindow() { NOTIMPLEMENTED(); }
};

class HtmlDialogContentsDelegate {
 public:
};

class ModalHtmlDialogDelegate : public HtmlDialogContentsDelegate {
 public:
  ModalHtmlDialogDelegate(const GURL&, int, int, const std::string&,
                          IPC::Message*, WebContents*) { }
};

class CharacterEncoding {
 public:
  static std::wstring GetCanonicalEncodingNameByAliasName(
      const std::wstring&) {
    NOTIMPLEMENTED();
    return L"";
  }
};

#if defined(OS_MACOSX)
class FindBarMac {
 public:
  FindBarMac(WebContentsView*, gfx::NativeWindow) { }
  void Show() { }
  void Close() { }
  void StartFinding(bool&) { }
  void EndFindSession() { }
  void DidBecomeUnselected() { }
  bool IsVisible() { return false; }
  bool IsAnimating() { return false; }
  gfx::NativeView GetView() { return nil; }
  std::string find_string() { return ""; }
  void OnFindReply(int, int, const gfx::Rect&, int, bool) { }
};
#endif

class LoginHandler {
 public:
  void SetAuth(const std::wstring& username,
               const std::wstring& password) {
    NOTIMPLEMENTED();
  }
  void CancelAuth() { NOTIMPLEMENTED(); }
  void OnRequestCancelled() { NOTIMPLEMENTED(); }
};

LoginHandler* CreateLoginPrompt(net::AuthChallengeInfo* auth_info,
                                URLRequest* request,
                                MessageLoop* ui_loop);

class ExternalProtocolHandler {
 public:
  static void LaunchUrl(const GURL& url, int render_process_host_id,
                        int tab_contents_id) {
    NOTIMPLEMENTED();
  }
};

class RepostFormWarningDialog {
 public:
  static void RunRepostFormWarningDialog(NavigationController*) { }
  virtual ~RepostFormWarningDialog() { }
};

class PageInfoWindow {
 public:
  enum TabID {
    GENERAL = 0,
    SECURITY,
  };
  static void CreatePageInfo(Profile* profile, NavigationEntry* nav_entry,
                             gfx::NativeView parent_hwnd, TabID tab) {
    NOTIMPLEMENTED();
  }
  static void CreateFrameInfo(Profile* profile, const GURL& url,
                              const NavigationEntry::SSLStatus& ssl,
                              gfx::NativeView parent_hwnd, TabID tab) {
    NOTIMPLEMENTED();
  }
};

class FontsLanguagesWindowView {
 public:
  explicit FontsLanguagesWindowView(Profile* profile) { NOTIMPLEMENTED(); }
  void SelectLanguagesTab() { NOTIMPLEMENTED(); }
};

#endif  // CHROME_COMMON_TEMP_SCAFFOLDING_STUBS_H_
