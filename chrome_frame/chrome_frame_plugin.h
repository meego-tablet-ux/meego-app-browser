// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_FRAME_CHROME_FRAME_PLUGIN_H_
#define CHROME_FRAME_CHROME_FRAME_PLUGIN_H_

#include <string>
#include <vector>

#include "base/ref_counted.h"
#include "base/win_util.h"
#include "chrome_frame/chrome_frame_automation.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_paths_internal.h"
#include "chrome_frame/simple_resource_loader.h"
#include "chrome_frame/utils.h"

#include "grit/chromium_strings.h"

#define IDC_ABOUT_CHROME_FRAME 40018

// A class to implement common functionality for all types of
// plugins: NPAPI. ActiveX and ActiveDoc
template <typename T>
class ChromeFramePlugin : public ChromeFrameDelegateImpl {
 public:
  ChromeFramePlugin()
      : ignore_setfocus_(false),
        is_privileged_(false) {
  }
  ~ChromeFramePlugin() {
    Uninitialize();
  }

BEGIN_MSG_MAP(T)
  MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
  MESSAGE_HANDLER(WM_SIZE, OnSize)
  MESSAGE_HANDLER(WM_PARENTNOTIFY, OnParentNotify)
END_MSG_MAP()

  bool Initialize() {
    DLOG(INFO) << __FUNCTION__;
    DCHECK(!automation_client_.get());
    automation_client_ = CreateAutomationClient();
    if (!automation_client_.get()) {
      NOTREACHED() << "new ChromeFrameAutomationClient";
      return false;
    }

    return true;
  }

  void Uninitialize() {
    DLOG(INFO) << __FUNCTION__;
    if (IsValid()) {
      automation_client_->Uninitialize();
      automation_client_ = NULL;
    }
  }

  bool InitializeAutomation(const std::wstring& profile_name,
                            const std::wstring& extra_chrome_arguments,
                            bool incognito, bool is_widget_mode,
                            const GURL& url, const GURL& referrer) {
    DCHECK(IsValid());
    DCHECK(launch_params_ == NULL);
    // We don't want to do incognito when privileged, since we're
    // running in browser chrome or some other privileged context.
    bool incognito_mode = !is_privileged_ && incognito;
    FilePath profile_path;
    GetProfilePath(profile_name, &profile_path);
    launch_params_ = new ChromeFrameLaunchParams(url, referrer, profile_path,
        profile_name, extra_chrome_arguments, incognito_mode, is_widget_mode);
    return automation_client_->Initialize(this, launch_params_);
  }

  // ChromeFrameDelegate implementation
  virtual WindowType GetWindow() const {
    return (static_cast<const T*>(this))->m_hWnd;
  }

  virtual void GetBounds(RECT* bounds) {
    if (bounds) {
      if (::IsWindow(GetWindow())) {
        (static_cast<T*>(this))->GetClientRect(bounds);
      }
    }
  }
  virtual std::string GetDocumentUrl() {
    return document_url_;
  }
  virtual void OnAutomationServerReady() {
    // Issue the extension automation request if we're privileged to
    // allow this control to handle extension requests from Chrome.
    if (is_privileged_ && IsValid())
      automation_client_->SetEnableExtensionAutomation(functions_enabled_);
  }

  virtual bool IsValid() const {
    return automation_client_.get() != NULL;
  }

  virtual void OnHostMoved() {
    if (IsValid())
      automation_client_->OnChromeFrameHostMoved();
  }

 protected:
  virtual void OnNavigationFailed(int tab_handle, int error_code,
                                  const GURL& gurl) {
    OnLoadFailed(error_code, gurl.spec());
  }

  virtual void OnHandleContextMenu(int tab_handle, HANDLE menu_handle,
                                   int align_flags,
                                   const IPC::ContextMenuParams& params) {
    if (!menu_handle || !automation_client_.get()) {
      NOTREACHED();
      return;
    }

    // TrackPopupMenuEx call will fail on IE on Vista running
    // in low integrity mode. We DO seem to be able to enumerate the menu
    // though, so just clone it and show the copy:
    HMENU copy = UtilCloneContextMenu(static_cast<HMENU>(menu_handle));
    if (!copy)
      return;

    T* self = static_cast<T*>(this);
    if (self->PreProcessContextMenu(copy)) {
      // In order for the context menu to handle keyboard input, give the
      // ActiveX window focus.
      ignore_setfocus_ = true;
      SetFocus(GetWindow());
      ignore_setfocus_ = false;
      UINT flags = align_flags | TPM_LEFTBUTTON | TPM_RETURNCMD | TPM_RECURSE;
      UINT selected = TrackPopupMenuEx(copy, flags, params.screen_x,
                                       params.screen_y, GetWindow(), NULL);
      // Menu is over now give focus back to chrome
      GiveFocusToChrome(false);
      if (IsValid() && selected != 0 &&
          !self->HandleContextMenuCommand(selected, params)) {
        automation_client_->SendContextMenuCommandToChromeFrame(selected);
      }
    }

    DestroyMenu(copy);
  }

  LRESULT OnSetFocus(UINT message, WPARAM wparam, LPARAM lparam,
                     BOOL& handled) {  // NO_LINT
    if (!ignore_setfocus_ && IsValid()) {
      GiveFocusToChrome(true);
    }
    return 0;
  }

  LRESULT OnSize(UINT message, WPARAM wparam, LPARAM lparam,
                 BOOL& handled) {  // NO_LINT
    handled = FALSE;
    // When we get resized, we need to resize the external tab window too.
    if (IsValid())
      automation_client_->Resize(LOWORD(lparam), HIWORD(lparam),
                                 SWP_NOACTIVATE | SWP_NOZORDER);
    return 0;
  }

  LRESULT OnParentNotify(UINT message, WPARAM wparam, LPARAM lparam,
                         BOOL& handled) {  // NO_LINT
    switch (LOWORD(wparam)) {
      case WM_LBUTTONDOWN:
      case WM_MBUTTONDOWN:
      case WM_RBUTTONDOWN:
      case WM_XBUTTONDOWN: {
        // If we got activated via mouse click on the external tab,
        // we need to update the state of this thread and tell the
        // browser that we now have the focus.
        HWND focus = ::GetFocus();
        HWND plugin_window = GetWindow();
        if (focus != plugin_window && !IsChild(plugin_window, focus)) {
          ignore_setfocus_ = true;
          SetFocus(plugin_window);
          ignore_setfocus_ = false;
        }
        break;
      }
    }

    return 0;
  }

  // Return true if context menu should be displayed. The menu could be
  // modified as well (enable/disable commands, add/remove items).
  // Override in most-derived class if needed.
  bool PreProcessContextMenu(HMENU menu) {
    // Add an "About" item.
    AppendMenu(menu, MF_STRING, IDC_ABOUT_CHROME_FRAME,
               SimpleResourceLoader::Get(IDS_CHROME_FRAME_MENU_ABOUT).c_str());
    return true;
  }

  // Return true if menu command is processed, otherwise the command will be
  // passed to Chrome for execution. Override in most-derived class if needed.
  bool HandleContextMenuCommand(UINT cmd,
                                const IPC::ContextMenuParams& params) {
    return false;
  }

  // Allow overriding the type of automation client used, for unit tests.
  virtual ChromeFrameAutomationClient* CreateAutomationClient() {
    return new ChromeFrameAutomationClient;
  }

  void GiveFocusToChrome(bool restore_focus_to_view) {
    if (IsValid()) {
      TabProxy* tab = automation_client_->tab();
      HWND chrome_window = automation_client_->tab_window();
      if (tab && ::IsWindow(chrome_window)) {
        DLOG(INFO) << "Setting initial focus";
        tab->SetInitialFocus(win_util::IsShiftPressed(),
                             restore_focus_to_view);
      }
    }
  }

  virtual void GetProfilePath(const std::wstring& profile_name,
                              FilePath* profile_path) {
    chrome::GetChromeFrameUserDataDirectory(profile_path);
    *profile_path = profile_path->Append(profile_name);
    DLOG(INFO) << __FUNCTION__ << ": " << profile_path->value();
  }

 protected:
  // Our gateway to chrome land
  scoped_refptr<ChromeFrameAutomationClient> automation_client_;

  // How we launched Chrome.
  scoped_refptr<ChromeFrameLaunchParams> launch_params_;

  // Url of the containing document.
  std::string document_url_;

  // We set this flag when we're taking the focus ourselves
  // and notifying the host browser that we're doing so.
  // When the flag is not set, we transfer the focus to chrome.
  bool ignore_setfocus_;

  // The plugin is privileged if it is:
  // * Invoked by a window running under the system principal in FireFox.
  // * Being hosted by a custom host exposing the SID_ChromeFramePrivileged
  //   service.
  //
  // When privileged, additional interfaces are made available to the user.
  bool is_privileged_;

  // List of functions to enable for automation, or a single entry "*" to
  // enable all functions for automation.  Ignored unless is_privileged_ is
  // true.  Defaults to the empty list, meaning automation will not be
  // turned on.
  std::vector<std::string> functions_enabled_;
};

#endif  // CHROME_FRAME_CHROME_FRAME_PLUGIN_H_

