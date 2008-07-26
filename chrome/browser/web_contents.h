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

#ifndef CHROME_BROWSER_WEB_CONTENTS_H__
#define CHROME_BROWSER_WEB_CONTENTS_H__

#include <hash_map>

#include "base/scoped_handle.h"
#include "chrome/common/win_util.h"
#include "chrome/browser/fav_icon_helper.h"
#include "chrome/browser/printing/print_view_manager.h"
#include "chrome/browser/render_view_host_delegate.h"
#include "chrome/browser/save_package.h"
#include "chrome/browser/shell_dialogs.h"
#include "chrome/browser/tab_contents.h"
#include "chrome/browser/web_app.h"
#include "chrome/views/hwnd_view_container.h"

class FindInPageController;
class InterstitialPageDelegate;
class NavigationProfiler;
class PasswordManager;
class PluginInstaller;
class RenderViewHost;
class RenderViewHostFactory;
class RenderWidgetHost;
class RenderWidgetHostHWND;
class SadTabView;
struct WebDropData;
class WebDropTarget;

class WebContents : public TabContents,
                    public RenderViewHostDelegate,
                    public ChromeViews::HWNDViewContainer,
                    public SelectFileDialog::Listener,
                    public WebApp::Observer {
 public:
  // If instance is NULL, then creates a new process for this view.  Otherwise
  // initialize with a process already created for a different WebContents.
  // This will share the process between views in the same instance.  If
  // render_view_factory is NULL, this will create RenderViewHost objects
  // directly.
  WebContents(Profile* profile,
              SiteInstance* instance,
              RenderViewHostFactory* render_view_factory,
              int routing_id,
              HANDLE modal_dialog_event);

  static void RegisterUserPrefs(PrefService* prefs);

  virtual void CreateView(HWND parent_hwnd, const gfx::Rect& initial_bounds);
  virtual HWND GetContainerHWND() const { return GetHWND(); }
  virtual void GetContainerBounds(gfx::Rect *out) const;
  virtual void ShowContents();
  virtual void HideContents();
  virtual void SizeContents(const gfx::Size& size);

  // TabContents
  virtual WebContents* AsWebContents() { return this; }
  virtual bool Navigate(const NavigationEntry& entry, bool reload);
  virtual void Stop();
  virtual void DidBecomeSelected();
  virtual void WasHidden();
  virtual void Destroy();
  virtual SkBitmap GetFavIcon();
  virtual std::wstring GetStatusText() const;

  // Find functions
  virtual bool CanFind() const { return true; }
  virtual void StartFinding(int request_id,
                            const std::wstring& search_string,
                            bool forward,
                            bool match_case,
                            bool find_next);
  virtual void StopFinding(bool clear_selection);
  virtual void OpenFindInPageWindow(const Browser& browser);
  virtual void ReparentFindWindow(HWND new_parent);
  virtual bool AdvanceFindSelection(bool forward_direction);

  // Text zoom
  virtual void AlterTextSize(text_zoom::TextSize size);

  // Change encoding of page.
  virtual void SetPageEncoding(const std::wstring& encoding_name);

  bool is_starred() const { return is_starred_; }

  // Set whether the contents should block javascript message boxes or not.
  // Default is not to block any message boxes.
  void SetSuppressJavascriptMessageBoxes(bool suppress_javascript_messages);

  // Return true if the WebContents is doing performance profiling
  bool is_profiling() const { return is_profiling_; }

  // Check with the global navigation profiler on whether to enable
  // profiling. Return true if profiling needs to be enabled, return
  // false otherwise.
  bool EnableProfiling();

  // Return the global navigation profiler.
  NavigationProfiler* GetNavigationProfiler();

  // Overridden from TabContents to remember at what time the download bar was
  // shown.
  void SetDownloadShelfVisible(bool visible);

  // Returns the SavePackage which manages the page saving job.
  inline SavePackage* get_save_package() const { return save_package_.get(); }

  // Whether or not the info bar is visible. This delegates to
  // the ChromeFrame method InfoBarVisibilityChanged.
  void SetInfoBarVisible(bool visible);
  virtual bool IsInfoBarVisible() { return info_bar_visible_; }

  // Whether or not the FindInPage bar is visible.
  void SetFindInPageVisible(bool visible);

  // Create the InfoBarView and returns it if none has been created.
  // Just returns existing InfoBarView if it is already created.
  virtual InfoBarView* GetInfoBarView();

  // Prepare for saving page.
  void OnSavePage();

  // Save page with the main HTML file path, the directory for saving resources,
  // and the save type: HTML only or complete web page.
  void SavePage(const std::wstring& main_file, const std::wstring& dir_path,
                SavePackage::SavePackageType save_type);

  // Get all savable resource links from current webpage, include main
  // frame and sub-frame.
  void GetAllSavableResourceLinksForCurrentPage(const GURL& page_url);

  // Get html data by serializing all frames of current page with lists
  // which contain all resource links that have local copy.
  // The parameter links contain original URLs of all saved links.
  // The parameter local_paths contain corresponding local file paths of
  // all saved links, which matched with vector:links one by one.
  // The parameter local_directory_name is relative path of directory which
  // contain all saved auxiliary files included all sub frames and resouces.
  void GetSerializedHtmlDataForCurrentPageWithLocalLinks(
      const std::vector<std::wstring>& links,
      const std::vector<std::wstring>& local_paths,
      const std::wstring& local_directory_name);

  // Locates a sub frame with given xpath and executes the given
  // javascript in its context.
  void ExecuteJavascriptInWebFrame(const std::wstring& frame_xpath,
                                   const std::wstring& jscript);

  // Locates a sub frame with given xpath and logs a message to its
  // console.
  void AddMessageToConsole(const std::wstring& frame_xpath,
                           const std::wstring& message,
                           ConsoleMessageLevel level);

  // Request the corresponding render view to perform these operations
  void Undo();
  void Redo();
  void Replace(const std::wstring& text);
  void Delete();
  void SelectAll();

  // Sets the WebApp for this WebContents.
  void SetWebApp(WebApp* web_app);
  WebApp* web_app() { return web_app_.get(); }

  // Return whether this tab contents was created to contain an application.
  bool IsWebApplication() const;

  // Tell Gears to create a shortcut for the current page.
  void CreateShortcut();

  // Tell the render view to perform a file upload. |form| is the name or ID of
  // the form that should be used to perform the upload. |file| is the name or
  // ID of the file input that should be set to |file_path|. |submit| is the
  // name or ID of the submit button. If non empty, the submit button will be
  // pressed. If not, the form will be filled with the information but the user
  // will perform the post operation.
  //
  // |other_values| contains a list of key value pairs separated by '\n'.
  // Each key value pair is of the form key=value where key is a form name or
  // ID and value is the desired value.
  void StartFileUpload(const std::wstring& file_path,
                       const std::wstring& form,
                       const std::wstring& file,
                       const std::wstring& submit,
                       const std::wstring& other_values);

  // JavascriptMessageBoxHandler calls this when the dialog is closed.
  void OnJavaScriptMessageBoxClosed(IPC::Message* reply_msg, bool success,
    const std::wstring& prompt);

  void CopyImageAt(int x, int y);
  void InspectElementAt(int x, int y);
  void ShowJavaScriptConsole();
  void AllowDomAutomationBindings();

  // Tell the render view to fill in a form and optionally submit it.
  void FillForm(const FormData& form);

  // Tell the render view to fill a password form and trigger autocomplete
  // in the case of multiple matching logins.
  void FillPasswordForm(const PasswordFormDomManager::FillData& form_data);

  // D&d drop target messages that get forwarded on to the render view host.
  void DragTargetDragEnter(const WebDropData& drop_data,
                           const gfx::Point& client_pt,
                           const gfx::Point& screen_pt);
  void DragTargetDragOver(const gfx::Point& client_pt,
                          const gfx::Point& screen_pt);
  void DragTargetDragLeave();
  void DragTargetDrop(const gfx::Point& client_pt,
                      const gfx::Point& screen_pt);

  // Called by PluginInstaller to start installation of missing plugin.
  void InstallMissingPlugin();

  // Returns the PasswordManager, creating it if necessary.
  PasswordManager* GetPasswordManager();

  // Returns the PluginInstaller, creating it if necessary.
  PluginInstaller* GetPluginInstaller();

  // Return the currently active RenderProcessHost, RenderViewHost, and
  // SiteInstance, respectively.  Each of these may change over time.  Callers
  // should be aware that the SiteInstance could be deleted if its ref count
  // drops to zero (i.e., if all RenderViewHosts and NavigationEntries that
  // use it are deleted).
  RenderProcessHost* process() const;
  RenderViewHost* render_view_host() const;
  SiteInstance* site_instance() const;

  // Overridden from TabContents to return the window of the
  // RenderWidgetHostView.
  virtual HWND GetContentHWND();

  // Handling the drag and drop of files into the content area.
  virtual bool CanDisplayFile(const std::wstring& full_path);

  // Displays asynchronously a print preview (generated by the renderer) if not
  // already displayed and ask the user for its preferred print settings with
  // the "Print..." dialog box. (managed by the print worker thread).
  // TODO(maruel):  Creates a snapshot of the renderer to be used for the new
  // tab for the printing facility.
  void PrintPreview();

  // Prints the current document immediately. Since the rendering is
  // asynchronous, the actual printing will not be completed on the return of
  // this function. Returns false if printing is impossible at the moment.
  bool PrintNow();

  virtual void WillCaptureContents();
  virtual void DidCaptureContents();

  virtual void Cut();
  virtual void Copy();
  virtual void Paste();

  // Returns whether we are currently showing an interstitial page.
  bool IsShowingInterstitialPage() const;

  // Displays the specified html in the current page. This method can be used to
  // show temporary pages (such as security error pages).  It can be hidden by
  // calling HideInterstitialPage, in which case the original page is restored.
  // An optional delegate may be passed, it is not owned by the WebContents.
  void ShowInterstitialPage(const std::string& html_text,
                            InterstitialPageDelegate* delegate);

  // Reverts from the interstitial page to the original page.
  // If |wait_for_navigation| is true, the interstitial page is removed when
  // the original page has transitioned to the new contents.  This is useful
  // when you want to hide the interstitial page as you navigate to a new page.
  // Hiding the interstitial page right away would show the previous displayed
  // page.  If |proceed| is true, the WebContents will expect the navigation
  // to complete.  If not, it will revert to the last shown page.
  void HideInterstitialPage(bool wait_for_navigation, bool proceed);

  // Allows the WebContents to react when a cross-site response is ready to be
  // delivered to a pending RenderViewHost.  We must first run the onunload
  // handler of the old RenderViewHost before we can allow it to proceed.
  void OnCrossSiteResponse(int new_render_process_host_id,
                           int new_request_id);

  // Returns true if the active NavigationEntry's page_id equals page_id.
  bool IsActiveEntry(int32 page_id);

  // RenderViewHost states.  These states represent whether a cross-site
  // request is pending (in the new process model) and whether an interstitial
  // page is being shown.  These are public to give easy access to unit tests.
  enum RendererState {
    // NORMAL: just showing a page normally.
    // render_view_host_ is showing a page.
    // pending_render_view_host_ is NULL.
    // original_render_view_host_ is NULL.
    // interstitial_render_view_host_ is NULL.
    NORMAL = 0,
    // PENDING: creating a new RenderViewHost for a cross-site navigation.
    // Never used when --process-per-tab is specified.
    // render_view_host_ is showing a page.
    // pending_render_view_host_ is loading a page in the background.
    // original_render_view_host_ is NULL.
    // interstitial_render_view_host_ is NULL.
    PENDING,
    // ENTERING_INTERSTITIAL: an interstitial RenderViewHost has been created.
    // and will be shown as soon as it calls DidNavigate.
    // render_view_host_ is showing a page.
    // pending_render_view_host_ is either NULL or suspended in the background.
    // original_render_view_host_ is NULL.
    // interstitial_render_view_host_ is loading in the background.
    ENTERING_INTERSTITIAL,
    // INTERSTITIAL: Showing an interstitial page.
    // render_view_host_ is showing the interstitial.
    // pending_render_view_host_ is either NULL or suspended in the background.
    // original_render_view_host_ is the hidden original page.
    // interstitial_render_view_host_ is NULL.
    INTERSTITIAL,
    // LEAVING_INTERSTITIAL: interstitial is still showing, but we are
    // navigating to a new page that will replace it.
    // render_view_host_ is showing the interstitial.
    // pending_render_view_host_ is either NULL or loading a page.
    // original_render_view_host_ is hidden and possibly loading a page.
    // interstitial_render_view_host_ is NULL.
    LEAVING_INTERSTITIAL
  };

  const std::string& contents_mime_type() const {
    return contents_mime_type_;
  }

  // Returns true if this WebContents will notify about disconnection.
  bool notify_disconnection() const { return notify_disconnection_; }

  // Are we showing the POST interstitial page?
  //
  // NOTE: the POST interstitial does NOT result in a separate RenderViewHost.
  bool showing_repost_interstitial() { return showing_repost_interstitial_; }

  // Accessors to the the interstitial delegate, that is optionaly set when
  // an interstitial page is shown.
  InterstitialPageDelegate* interstitial_page_delegate() const {
    return interstitial_delegate_;
  }
  void set_interstitial_delegate(InterstitialPageDelegate* delegate) {
    interstitial_delegate_ = delegate;
  }

 protected:
  FRIEND_TEST(WebContentsTest, OnMessageReceived);

  // Should be deleted via CloseContents.
  virtual ~WebContents();

  // RenderViewHostDelegate
  virtual Profile* GetProfile() const;

  virtual void CreateView(int route_id, HANDLE modal_dialog_event);
  virtual void CreateWidget(int route_id);
  virtual void ShowView(int route_id,
                        WindowOpenDisposition disposition,
                        const gfx::Rect& initial_pos,
                        bool user_gesture);
  virtual void ShowWidget(int route_id, const gfx::Rect& initial_pos);
  virtual void RendererReady(RenderViewHost* render_view_host);
  virtual void RendererGone(RenderViewHost* render_view_host);
  virtual void DidNavigate(RenderViewHost* render_view_host,
                           const ViewHostMsg_FrameNavigate_Params& params);
  virtual void UpdateRenderViewSize();
  virtual void UpdateState(RenderViewHost* render_view_host,
                           int32 page_id,
                           const GURL& url,
                           const std::wstring& title,
                           const std::string& state);
  virtual void UpdateTitle(RenderViewHost* render_view_host,
                           int32 page_id,
                           const std::wstring& title);
  virtual void UpdateEncoding(RenderViewHost* render_view_host,
                              const std::wstring& encoding_name);
  virtual void UpdateTargetURL(int32 page_id, const GURL& url);
  virtual void UpdateThumbnail(const GURL& url,
                               const SkBitmap& bitmap,
                               const ThumbnailScore& score);
  virtual void Close(RenderViewHost* render_view_host);
  virtual void RequestMove(const gfx::Rect& new_bounds);
  virtual void DidStartLoading(RenderViewHost* render_view_host, int32 page_id);
  virtual void DidStopLoading(RenderViewHost* render_view_host, int32 page_id);
  virtual void DidStartProvisionalLoadForFrame(RenderViewHost* render_view_host,
                                               bool is_main_frame,
                                               const GURL& url);
  virtual void DidRedirectProvisionalLoad(int32 page_id,
                                          const GURL& source_url,
                                          const GURL& target_url);
  virtual void DidLoadResourceFromMemoryCache(const GURL& url,
                                              const std::string& security_info);
  virtual void DidFailProvisionalLoadWithError(
      RenderViewHost* render_view_host,
      bool is_main_frame,
      int error_code,
      const GURL& url,
      bool showing_repost_interstitial);
  virtual void FindReply(int request_id,
                         int number_of_matches,
                         const gfx::Rect& selection_rect,
                         int active_match_ordinal,
                         bool final_update);
  virtual void UpdateFavIconURL(RenderViewHost* render_view_host,
                                int32 page_id, const GURL& icon_url);
  virtual void DidDownloadImage(RenderViewHost* render_view_host,
                                int id,
                                const GURL& image_url,
                                bool errored,
                                const SkBitmap& image);
  virtual void ShowContextMenu(const ViewHostMsg_ContextMenu_Params& params);
  virtual void StartDragging(const WebDropData& drop_data);
  virtual void UpdateDragCursor(bool is_drop_target);
  virtual void RequestOpenURL(const GURL& url,
                              WindowOpenDisposition disposition);
  virtual void DomOperationResponse(const std::string& json_string,
                                    int automation_id);
  virtual void GoToEntryAtOffset(int offset);
  virtual void GetHistoryListCount(int* back_list_count,
                                   int* forward_list_count);
  virtual void RunFileChooser(const std::wstring& default_file);
  virtual void RunJavaScriptMessage(const std::wstring& message,
                                    const std::wstring& default_prompt,
                                    const int flags,
                                    IPC::Message* reply_msg);
  virtual void RunBeforeUnloadConfirm(const std::wstring& message,
                                      IPC::Message* reply_msg);
  virtual void ShowModalHTMLDialog(const GURL& url, int width, int height,
                                   const std::string& json_arguments,
                                   IPC::Message* reply_msg);
  virtual void PasswordFormsSeen(const std::vector<PasswordForm>& forms);
  virtual void TakeFocus(bool reverse);
  virtual void DidGetPrintedPagesCount(int cookie, int number_pages);
  virtual void DidPrintPage(const ViewHostMsg_DidPrintPage_Params& params);
  virtual GURL GetAlternateErrorPageURL() const;
  virtual WebPreferences GetWebkitPrefs();
  virtual void OnMissingPluginStatus(int status);
  virtual void OnCrashedPlugin(const std::wstring& plugin_path);
  virtual void OnJSOutOfMemory();
  virtual void OnReceivedSavableResourceLinksForCurrentPage(
      const std::vector<GURL>& resources_list,
      const std::vector<GURL>& referrers_list,
      const std::vector<GURL>& frames_list);
  virtual void OnReceivedSerializedHtmlData(const GURL& frame_url,
                                            const std::string& data,
                                            int32 status);
  virtual void ShouldClosePage(bool proceed);
  virtual bool CanBlur() const;
  virtual void RendererUnresponsive(RenderViewHost* render_view_host);
  virtual void RendererResponsive(RenderViewHost* render_view_host);
  virtual void LoadStateChanged(const GURL& url, net::LoadState load_state);

  // Notification that a page has an OpenSearch description document available
  // at url. This checks to see if we should generate a keyword based on the
  // OSDD, and if necessary uses TemplateURLFetcher to download the OSDD
  // and create a keyword.
  virtual void PageHasOSDD(RenderViewHost* render_view_host,
                           int32 page_id, const GURL& url, bool autodetected);

  virtual void OnDidGetApplicationInfo(
      int32 page_id,
      const webkit_glue::WebApplicationInfo& info);

  // Overridden from TabContents.
  virtual void SetInitialFocus(bool reverse);

  // Handle reply from inspect element request
  virtual void InspectElementReply(int num_resources);

  // Handle keyboard events not processed by the renderer.
  virtual void HandleKeyboardEvent(const WebKeyboardEvent& event);

  // Notifies the RenderWidgetHost instance about the fact that the
  // page is loading, or done loading and calls the base implementation.
  void SetIsLoading(bool is_loading, LoadNotificationDetails* details);

  // Overridden from SelectFileDialog::Listener:
  virtual void FileSelected(const std::wstring& path, void* params);
  virtual void FileSelectionCanceled(void* params);

  // This method initializes the given renderer if necessary and creates the
  // view ID corresponding to this view host. If this method is not called and
  // the process is not shared, then the WebContents will act as though the
  // renderer is not running (i.e., it will render "sad tab").
  // This method is automatically called from LoadURL.
  //
  // If you are attaching to an already-existing RenderView, you should call
  // InitWithExistingID.
  virtual bool CreateRenderView(RenderViewHost* render_view_host);

 private:
  friend class TestWebContents;

  // When CreateShortcut is invoked RenderViewHost::GetApplicationInfo is
  // invoked. CreateShortcut caches the state of the page needed to create the
  // shortcut in PendingInstall. When OnDidGetApplicationInfo is invoked, it
  // uses the information from PendingInstall and the WebApplicationInfo
  // to create the shortcut.
  class GearsCreateShortcutCallbackFunctor;
  struct PendingInstall {
    int32 page_id;
    SkBitmap icon;
    std::wstring title;
    GURL url;
    // This object receives the GearsCreateShortcutCallback and routes the
    // message back to the WebContents, if we haven't been deleted.
    GearsCreateShortcutCallbackFunctor* callback_functor;
  };


  bool ScrollZoom(int scroll_type);
  void WheelZoom(int distance);

  // Creates a RenderViewHost using render_view_factory_ (or directly, if the
  // factory is NULL).
  RenderViewHost* CreateRenderViewHost(SiteInstance* instance,
                                       RenderViewHostDelegate* delegate,
                                       int routing_id,
                                       HANDLE modal_dialog_event);

  // Returns whether this tab should transition to a new renderer for
  // cross-site URLs.  Enabled unless we see the --process-per-tab command line
  // switch.  Can be overridden in unit tests.
  virtual bool ShouldTransitionCrossSite();

  // Returns an appropriate SiteInstance object for the given NavigationEntry,
  // possibly reusing the current SiteInstance.
  // Never called if --process-per-tab is used.
  SiteInstance* GetSiteInstanceForEntry(const NavigationEntry& entry,
                                        SiteInstance* curr_instance);

  // Prevent the interstitial page from proceeding after we start navigating
  // away from it.  If |stop_request| is true, abort the pending requests
  // immediately, because we are navigating away.
  void DisableInterstitialProceed(bool stop_request);

  // Helper method to create a pending RenderViewHost for a cross-site
  // navigation.  Used in the new process model.
  bool CreatePendingRenderView(SiteInstance* instance);

  // Replaces the currently shown render_view_host_ with the RenderViewHost in
  // the field pointed to by |new_render_view_host|, and then NULLs the field.
  // Callers should only pass pointers to the pending_render_view_host_,
  // interstitial_render_view_host_, or original_render_view_host_ fields of
  // this object.  If |destroy_after|, this method will call
  // ScheduleDeferredDestroy on the previous render_view_host_.
  void SwapToRenderView(RenderViewHost** new_render_view_host,
                        bool destroy_after);

  // Destroys the RenderViewHost in the field pointed to by |render_view_host|,
  // and then NULLs the field.  Callers should only pass pointers to the
  // pending_render_view_host_, interstitial_render_view_host_, or
  // original_render_view_host_ fields of this object.
  void CancelRenderView(RenderViewHost** render_view_host);

  // Backend for LoadURL that optionally creates a history entry. The
  // transition type will be ignored if a history entry is not created.
  void LoadURL(const std::wstring& url, bool create_history_entry,
               PageTransition::Type transition);

  // Windows Event handlers
  virtual void OnDestroy();
  virtual void OnHScroll(int scroll_type, short position, HWND scrollbar);
  virtual void OnMouseLeave();
  virtual LRESULT OnMouseRange(UINT msg, WPARAM w_param, LPARAM l_param);
  virtual void OnPaint(HDC junk_dc);
  virtual LRESULT OnReflectedMessage(UINT msg, WPARAM w_param, LPARAM l_param);
  virtual void OnSetFocus(HWND window);
  virtual void OnVScroll(int scroll_type, short position, HWND scrollbar);
  virtual void OnWindowPosChanged(WINDOWPOS* window_pos);

  // Callback from HistoryService for our request for a favicon.
  void OnFavIconData(HistoryService::Handle handle,
                     bool know_favicon,
                     scoped_refptr<RefCountedBytes> data,
                     bool expired);

  // NotificationObserver implementation.
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

  // Helper functions for sending notifications.
  void NotifySwapped();
  void NotifyConnected();
  void NotifyDisconnected();

  // Called by OnMsgNavigate to update history state.
  virtual void UpdateHistoryForNavigation(const GURL& display_url,
      const ViewHostMsg_FrameNavigate_Params& params);

  // If params has a searchable form, this tries to create a new keyword.
  void GenerateKeywordIfNecessary(
      const ViewHostMsg_FrameNavigate_Params& params);

  // Sets up the View that holds the rendered web page, receives messages for
  // it and contains page plugins.
  RenderWidgetHostHWND* CreatePageView(RenderViewHost* render_view_host);

  // Cleans up after an interstitial page is hidden, including removing the
  // interstitial's NavigationEntry.
  void InterstitialPageGone();

  // Convenience method that returns true if the specified RenderViewHost is
  // this WebContents' interstitial page RenderViewHost.
  bool IsInterstitialRenderViewHost(RenderViewHost* render_view_host) const;

  // Navigation helpers --------------------------------------------------------
  //
  // These functions are helpers for Navigate() and DidNavigate().

  // Creates a new navigation entry (malloced, the caller will have to free)
  // for the given committed load. Used by DidNavigate. Will not return NULL.
  NavigationEntry* CreateNavigationEntryForCommit(
      const ViewHostMsg_FrameNavigate_Params& params);

  // Handles post-navigation tasks specific to some set of frames. DidNavigate()
  // calls these with newly created navigation entry for this navigation BEFORE
  // that entry has been committed to the navigation controller. The functions
  // can update the entry as needed.
  //
  // First the frame-specific version (main or sub) will be called to update the
  // entry as needed after it was created by CreateNavigationEntryForCommit.
  //
  // Then DidNavigateAnyFramePreCommit will be called with the now-complete
  // entry for further processing that is not specific to the type of frame.
  void DidNavigateMainFramePreCommit(
      const ViewHostMsg_FrameNavigate_Params& params,
      NavigationEntry* entry);
  void DidNavigateSubFramePreCommit(
      const ViewHostMsg_FrameNavigate_Params& params,
      NavigationEntry* entry);
  void DidNavigateAnyFramePreCommit(
      const ViewHostMsg_FrameNavigate_Params& params,
      NavigationEntry* entry);

  // Handles post-navigation tasks in DidNavigate AFTER the entry has been
  // committed to the navigation controller. See WillNavigate* above. Note that
  // the navigation entry is not provided since it may be invalid/changed after
  // being committed.
  void DidNavigateMainFramePostCommit(
      const ViewHostMsg_FrameNavigate_Params& params);
  void DidNavigateAnyFramePostCommit(
      RenderViewHost* render_view_host,
      const ViewHostMsg_FrameNavigate_Params& params);

  // Helper method to update the RendererState on a call to [Did]Navigate.
  RenderViewHost* UpdateRendererStateNavigate(const NavigationEntry& entry);
  void UpdateRendererStateDidNavigate(RenderViewHost* render_view_host);

  // Called when navigating the main frame to close all child windows if the
  // domain is changing.
  void MaybeCloseChildWindows(const ViewHostMsg_FrameNavigate_Params& params);

  // Broadcasts a notification for the provisional load committing, used by
  // DidNavigate.
  void BroadcastProvisionalLoadCommit(
      RenderViewHost* render_view_host,
      const ViewHostMsg_FrameNavigate_Params& params);

  // Convenience method that returns true if navigating to the specified URL
  // from the current one is an in-page navigation (jumping to a ref in the
  // page).
  bool IsInPageNavigation(const GURL& url) const;

  // Updates the starred state from the bookmark bar model. If the state has
  // changed, the delegate is notified.
  void UpdateStarredStateForCurrentURL();

  // Send the alternate error page URL to the renderer. This method is virtual
  // so special html pages can override this (e.g., the new tab page).
  virtual void UpdateAlternateErrorPageURL();

  // Send webkit specific settings to the renderer.
  void UpdateWebPreferences();

  // Return whether the optional web application is active for the current URL.
  // Call this method to check if web app properties are in effect.
  //
  // Note: This method should be used for presentation but not security. The app
  // is always active if the containing window is a web application.
  bool IsWebApplicationActive() const;

  // WebApp::Observer method. Invoked when the set of images contained in the
  // web app changes. Notifies the delegate our favicon has changed.
  virtual void WebAppImagesChanged(WebApp* web_app);

  // Called when the user dismisses the shortcut creation dialog.  'success' is
  // true if the shortcut was created.
  void OnGearsCreateShortcutDone(const GearsShortcutData& shortcut_data,
                                 bool success);

  // If our controller was restored and the page id is > than the site
  // instance's page id, the site instances page id is updated as well as the
  // renderers max page id.
  void UpdateMaxPageIDIfNecessary(SiteInstance* site_instance,
                                  RenderViewHost* rvh);

  // Profiling -----------------------------------------------------------------

  // Logs the commit of the load for profiling purposes. Used by DidNavigate.
  void HandleProfilingForDidNavigate(
      const ViewHostMsg_FrameNavigate_Params& params);

  // If performance profiling is enabled, save current PageLoadTracker entry
  // to visited page list.
  void SaveCurrentProfilingEntry();

  // If performance profiling is enabled, create a new PageLoadTracker entry
  // when navigating to a new page.
  void CreateNewProfilingEntry(const GURL& url);

  // Enumerate and 'un-parent' any plugin windows that are children
  // of this web contents.
  void DetachPluginWindows();
  static BOOL CALLBACK EnumPluginWindowsCallback(HWND window, LPARAM param);

  // Data ----------------------------------------------------------------------

  // Factory for creating RenderViewHosts.  This is useful for unit tests.  If
  // this is NULL, just create a RenderViewHost directly.
  RenderViewHostFactory* render_view_factory_;

  // Our RenderView host. This object is responsible for all communication with
  // a child RenderView instance.  Note that this can be the page render view
  // host or the interstitial RenderViewHost if the RendererState is
  // INTERSTITIAL or LEAVING_INTERSTITIAL.
  RenderViewHost* render_view_host_;

  // This var holds the original RenderViewHost when the interstitial page is
  // showing (the RendererState is INTERSTITIAL or LEAVING_INTERSTITIAL).  It
  // is NULL otherwise.
  RenderViewHost* original_render_view_host_;

  // The RenderViewHost of the interstitial page.  This is non NULL when the
  // the RendererState is ENTERING_INTERSTITIAL.
  RenderViewHost* interstitial_render_view_host_;

  // A RenderViewHost used to load a cross-site page.  This remains hidden
  // during the PENDING RendererState until it calls DidNavigate.  It can also
  // exist if an interstitial page is shown.
  RenderViewHost* pending_render_view_host_;

  // Indicates if we are in the process of swapping our RenderViewHost.  This
  // allows us to switch to interstitial pages in different RenderViewHosts.
  // In the new process model, this also allows us to render pages from
  // different SiteInstances in different processes, all within the same tab.
  RendererState renderer_state_;

  // Handles print preview and print job for this contents.
  printing::PrintViewManager printing_;

  // Indicates whether we should notify about disconnection of this
  // WebContents. This is used to ensure disconnection notifications only
  // happen if a connection notification has happened and that they happen only
  // once.
  bool notify_disconnection_;

  // When a navigation occurs (and is committed), we record its URL. This lets
  // us see where we are navigating from.
  GURL last_url_;

  // Maps from handle to page_id.
  typedef std::map<HistoryService::Handle, int32> HistoryRequestMap;
  HistoryRequestMap history_requests_;

  // Whether the WebContents is doing performance profiling
  bool is_profiling_;

  // System time at which the current load was started.
  TimeTicks current_load_start_;

  // Whether we have a (non-empty) title for the current page.
  // Used to prevent subsequent title updates from affecting history.
  bool has_page_title_;

  // SavePackage, lazily created.
  scoped_refptr<SavePackage> save_package_;

  // InfoBarView, lazily created.
  scoped_ptr<InfoBarView> info_bar_view_;

  // Whether the info bar view is visible.
  bool info_bar_visible_;

  // Handles communication with the FindInPage popup.
  scoped_ptr<FindInPageController> find_in_page_controller_;

  // Tracks our pending CancelableRequests. This maps pending requests to
  // page IDs so that we know whether a given callback still applies. The
  // page ID -1 means no page ID was set.
  CancelableRequestConsumerT<int32, -1> cancelable_consumer_;

  // Whether the current URL is starred
  bool is_starred_;

  // Handle to an event that's set when the page is showing a message box (or
  // equivalent constrained window).  Plugin processes check this to know if
  // they should pump messages then.
  ScopedHandle message_box_active_;

  // PasswordManager, lazily created.
  scoped_ptr<PasswordManager> password_manager_;

  // PluginInstaller, lazily created.
  scoped_ptr<PluginInstaller> plugin_installer_;

  // A drop target object that handles drags over this WebContents.
  scoped_refptr<WebDropTarget> drop_target_;

  // The SadTab renderer.
  scoped_ptr<SadTabView> sad_tab_;

  // This flag is true while we are in the photo-booth.  See dragged_tab.cc.
  bool capturing_contents_;

  // Handles downloading favicons.
  FavIconHelper fav_icon_helper_;

  // Dialog box used for choosing files to upload from file form fields.
  scoped_refptr<SelectFileDialog> select_file_dialog_;

  // Info bar for crashed plugin message.
  // IMPORTANT: This instance is owned by the InfoBarView. It is valid
  // only if InfoBarView::GetChildIndex for this view is valid.
  InfoBarMessageView* crashed_plugin_info_bar_;

  // The time that the last javascript message was dismissed.
  TimeTicks last_javascript_message_dismissal_;

  // True if the user has decided to block future javascript messages. This is
  // reset on navigations to false on navigations.
  bool suppress_javascript_messages_;

  // When a navigation occurs, we record its contents MIME type. It can be
  // used to check whether we can do something for some special contents.
  std::string contents_mime_type_;

  PendingInstall pending_install_;

  // The last time that the download shelf was made visible.
  TimeTicks last_download_shelf_show_;

  // The current load state and the URL associated with it.
  net::LoadState load_state_;
  std::wstring load_state_host_;

  // These maps hold on to the pages/widgets that we created on behalf of the
  // renderer that haven't shown yet.
  typedef stdext::hash_map<int, WebContents*> PendingViews;
  PendingViews pending_views_;

  typedef stdext::hash_map<int, RenderWidgetHost*> PendingWidgets;
  PendingWidgets pending_widgets_;

  // Non-null if we're displaying content for a web app.
  scoped_refptr<WebApp> web_app_;

  // See comment above showing_repost_interstitial().
  bool showing_repost_interstitial_;

  // An optional delegate used when an interstitial page is shown that gets
  // notified when the state of the interstitial changes.
  InterstitialPageDelegate* interstitial_delegate_;

  DISALLOW_EVIL_CONSTRUCTORS(WebContents);
};

#endif  // CHROME_BROWSER_WEB_CONTENTS_H__
