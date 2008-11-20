// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains the implementation of TestWebViewDelegate, which serves
// as the WebViewDelegate for the TestShellWebHost.  The host is expected to
// have initialized a MessageLoop before these methods are called.

#include "webkit/tools/test_shell/test_webview_delegate.h"

#include <objidl.h>
#include <shlobj.h>
#include <shlwapi.h>

#include "base/gfx/point.h"
#include "base/message_loop.h"
#include "base/string_util.h"
#include "base/trace_event.h"
#include "net/base/net_errors.h"
#include "webkit/glue/webdatasource.h"
#include "webkit/glue/webdropdata.h"
#include "webkit/glue/weberror.h"
#include "webkit/glue/webframe.h"
#include "webkit/glue/webpreferences.h"
#include "webkit/glue/weburlrequest.h"
#include "webkit/glue/webkit_glue.h"
#include "webkit/glue/webview.h"
#include "webkit/glue/plugins/plugin_list.h"
#include "webkit/glue/plugins/webplugin_delegate_impl.h"
#include "webkit/glue/window_open_disposition.h"
#include "webkit/tools/test_shell/drag_delegate.h"
#include "webkit/tools/test_shell/drop_delegate.h"
#include "webkit/tools/test_shell/test_navigation_controller.h"
#include "webkit/tools/test_shell/test_shell.h"

// WebViewDelegate -----------------------------------------------------------

TestWebViewDelegate::~TestWebViewDelegate() {
  if (custom_cursor_)
    DestroyIcon(custom_cursor_);
  RevokeDragDrop(shell_->webViewWnd());
}

WebPluginDelegate* TestWebViewDelegate::CreatePluginDelegate(
    WebView* webview,
    const GURL& url,
    const std::string& mime_type,
    const std::string& clsid,
    std::string* actual_mime_type) {
  HWND hwnd = GetContainingWindow(webview);
  if (!hwnd)
    return NULL;

  bool allow_wildcard = true;
  WebPluginInfo info;
  if (!NPAPI::PluginList::Singleton()->GetPluginInfo(url, mime_type, clsid,
                                                     allow_wildcard, &info,
                                                     actual_mime_type))
    return NULL;

  if (actual_mime_type && !actual_mime_type->empty())
    return WebPluginDelegateImpl::Create(info.file, *actual_mime_type, hwnd);
  else
    return WebPluginDelegateImpl::Create(info.file, mime_type, hwnd);
}

void TestWebViewDelegate::ShowJavaScriptAlert(const std::wstring& message) {
}

void TestWebViewDelegate::Show(WebWidget* webwidget, WindowOpenDisposition) {
  if (webwidget == shell_->webView()) {
    ShowWindow(shell_->mainWnd(), SW_SHOW);
    UpdateWindow(shell_->mainWnd());
  } else if (webwidget == shell_->popup()) {
    ShowWindow(shell_->popupWnd(), SW_SHOW);
    UpdateWindow(shell_->popupWnd());
  }
}

void TestWebViewDelegate::CloseWidgetSoon(WebWidget* webwidget) {
  if (webwidget == shell_->webView()) {
    PostMessage(shell_->mainWnd(), WM_CLOSE, 0, 0);
  } else if (webwidget == shell_->popup()) {
    shell_->ClosePopup();
  }
}

void TestWebViewDelegate::SetCursor(WebWidget* webwidget,
                                    const WebCursor& cursor) {
  if (WebWidgetHost* host = GetHostForWidget(webwidget)) {
    if (custom_cursor_) {
      DestroyIcon(custom_cursor_);
      custom_cursor_ = NULL;
    }
    if (cursor.IsCustom()) {
      custom_cursor_ = cursor.GetCustomCursor();
      host->SetCursor(custom_cursor_);
    } else {
      HINSTANCE mod_handle = GetModuleHandle(NULL);
      host->SetCursor(cursor.GetCursor(mod_handle));
    }
  }
}

void TestWebViewDelegate::GetWindowRect(WebWidget* webwidget,
                                        gfx::Rect* out_rect) {
  if (WebWidgetHost* host = GetHostForWidget(webwidget)) {
    RECT rect;
    ::GetWindowRect(host->view_handle(), &rect);
    *out_rect = gfx::Rect(rect);
  }
}

void TestWebViewDelegate::SetWindowRect(WebWidget* webwidget,
                                        const gfx::Rect& rect) {
  if (webwidget == shell_->webView()) {
    // ignored
  } else if (webwidget == shell_->popup()) {
    MoveWindow(shell_->popupWnd(),
               rect.x(), rect.y(), rect.width(), rect.height(), FALSE);
  }
}

void TestWebViewDelegate::GetRootWindowRect(WebWidget* webwidget,
                                            gfx::Rect* out_rect) {
  if (WebWidgetHost* host = GetHostForWidget(webwidget)) {
    RECT rect;
    HWND root_window = ::GetAncestor(host->view_handle(), GA_ROOT);
    ::GetWindowRect(root_window, &rect);
    *out_rect = gfx::Rect(rect);
  }
}

void TestWebViewDelegate::RunModal(WebWidget* webwidget) {
  Show(webwidget, NEW_WINDOW);

  WindowList* wl = TestShell::windowList();
  for (WindowList::const_iterator i = wl->begin(); i != wl->end(); ++i) {
    if (*i != shell_->mainWnd())
      EnableWindow(*i, FALSE);
  }

  shell_->set_is_modal(true);
  MessageLoop::current()->Run();

  for (WindowList::const_iterator i = wl->begin(); i != wl->end(); ++i)
    EnableWindow(*i, TRUE);
}

// Private methods -----------------------------------------------------------

void TestWebViewDelegate::SetPageTitle(const std::wstring& title) {
  // The Windows test shell, pre-refactoring, ignored this.  *shrug*
}

void TestWebViewDelegate::SetAddressBarURL(const GURL& url) {
  std::wstring url_string = UTF8ToWide(url.spec());
  SendMessage(shell_->editWnd(), WM_SETTEXT, 0,
              reinterpret_cast<LPARAM>(url_string.c_str()));
}

