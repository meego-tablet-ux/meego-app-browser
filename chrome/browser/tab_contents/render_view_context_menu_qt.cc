// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/string_util.h"
#include "chrome/app/chrome_dll_resource.h"
#include "chrome/browser/renderer_host/render_widget_host_view_qt.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "webkit/glue/context_menu.h"
#include "chrome/browser/tab_contents/render_view_context_menu_qt.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"

RenderViewContextMenuQt::RenderViewContextMenuQt(
    TabContents* web_contents,
    const ContextMenuParams& params,
    unsigned int triggering_event_time)
    : RenderViewContextMenuSimple(web_contents, params),
      triggering_event_time_(triggering_event_time) {
}

RenderViewContextMenuQt::~RenderViewContextMenuQt() {
}

void RenderViewContextMenuQt::PlatformInit() {
#if defined(MTF)
  menu_qt_.reset(new MenuQt(this, &menu_model_));
#endif
#if 0
  if (params_.is_editable) {
    RenderWidgetHostViewQt* rwhv = static_cast<RenderWidgetHostViewQt*>(
        source_tab_contents_->GetRenderWidgetHostView());
    if (rwhv)
      rwhv->AppendInputMethodsContextMenu(menu_qt_.get());
  }
#endif
}

void RenderViewContextMenuQt::Popup() {
  RenderWidgetHostView* rwhv = source_tab_contents_->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->ShowingContextMenu(true);
#if defined(MTF)
  menu_qt_->Popup();
#endif
  Browser* browser = BrowserList::GetLastActive();
  BrowserWindowQt* browser_window = (BrowserWindowQt*)browser->window();
  gfx::Point p;
  browser_window->ShowContextMenu(&menu_model_, p);
}

void RenderViewContextMenuQt::Popup(const gfx::Point& point) {
  RenderWidgetHostView* rwhv = source_tab_contents_->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->ShowingContextMenu(true);
#if defined(MTF)
  menu_qt_->PopupAsContextAt(triggering_event_time_, point);
#endif
  Browser* browser = BrowserList::GetLastActive();
  BrowserWindowQt* browser_window = (BrowserWindowQt*)browser->window();
  browser_window->ShowContextMenu(&menu_model_, point);
}

void RenderViewContextMenuQt::StoppedShowing() {
  RenderWidgetHostView* rwhv = source_tab_contents_->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->ShowingContextMenu(false);
}
