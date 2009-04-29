// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_HOST_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_HOST_H_

#include "chrome/browser/renderer_host/render_view_host_delegate.h"
#include "chrome/browser/tab_contents/render_view_host_delegate_helper.h"

class Browser;
class Extension;
class ExtensionView;
class RenderWidgetHost;
class RenderWidgetHostView;
class WebContents;
struct WebPreferences;

// This class is the browser component of an extension component's RenderView.
// It handles setting up the renderer process, if needed, with special
// privileges available to extensions.  It may have a view to be shown in the
// in the browser UI, or it may be hidden.
class ExtensionHost : public RenderViewHostDelegate,
                      public RenderViewHostDelegate::View {
 public:
  ExtensionHost(Extension* extension, SiteInstance* site_instance);
  ~ExtensionHost();

  void set_view(ExtensionView* view) { view_ = view; }
  ExtensionView* view() const { return view_; }
  Extension* extension() { return extension_; }
  RenderViewHost* render_view_host() const { return render_view_host_; }
  SiteInstance* site_instance() const;
  bool did_stop_loading() const { return did_stop_loading_; }

  // Initializes our RenderViewHost by creating its RenderView and navigating
  // to the given URL.  Uses host_view for the RenderViewHost's view (can be
  // NULL).
  void CreateRenderView(const GURL& url, RenderWidgetHostView* host_view);

  // RenderViewHostDelegate
  // TODO(mpcomplete): GetProfile is unused.
  virtual Profile* GetProfile() const { return NULL; }
  virtual void RenderViewCreated(RenderViewHost* render_view_host);
  virtual void DidContentsPreferredWidthChange(const int pref_width);
  virtual WebPreferences GetWebkitPrefs();
  virtual void RunJavaScriptMessage(
      const std::wstring& message,
      const std::wstring& default_prompt,
      const GURL& frame_url,
      const int flags,
      IPC::Message* reply_msg,
      bool* did_suppress_message);
  virtual void DidStopLoading(RenderViewHost* render_view_host);
  virtual RenderViewHostDelegate::View* GetViewDelegate() const;
  virtual ExtensionFunctionDispatcher* CreateExtensionFunctionDispatcher(
      RenderViewHost *render_view_host, const std::string& extension_id);

  // RenderViewHostDelegate::View
  virtual void CreateNewWindow(int route_id,
                               base::WaitableEvent* modal_dialog_event);
  virtual void CreateNewWidget(int route_id, bool activatable);
  virtual void ShowCreatedWindow(int route_id,
                                 WindowOpenDisposition disposition,
                                 const gfx::Rect& initial_pos,
                                 bool user_gesture);
  virtual void ShowCreatedWidget(int route_id,
                                 const gfx::Rect& initial_pos);
  virtual void ShowContextMenu(const ContextMenuParams& params);
  virtual void StartDragging(const WebDropData& drop_data);
  virtual void UpdateDragCursor(bool is_drop_target);
  virtual void TakeFocus(bool reverse);
  virtual void HandleKeyboardEvent(const NativeWebKeyboardEvent& event);

 private:
  // If this ExtensionHost has a view, this returns the Browser that view is a
  // part of.  If this is a global background page, we use the active Browser
  // instead.
  Browser* GetBrowser();

  // The extension that we're hosting in this view.
  Extension* extension_;

  // Optional view that shows the rendered content in the UI.
  ExtensionView* view_;

  // The host for our HTML content.
  RenderViewHost* render_view_host_;

  // Common implementations of some RenderViewHostDelegate::View methods.
  RenderViewHostDelegateViewHelper delegate_view_helper_;

  // Whether the RenderWidget has reported that it has stopped loading.
  bool did_stop_loading_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionHost);
};

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_HOST_H_
