// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/tab_contents/web_contents_view.h"

#include "chrome/browser/renderer_host/render_process_host.h"
#include "chrome/browser/renderer_host/render_widget_host.h"
#include "chrome/browser/renderer_host/render_widget_host_view.h"
#include "chrome/browser/tab_contents/tab_contents_delegate.h"
#include "chrome/browser/tab_contents/web_contents.h"

WebContentsView::WebContentsView(WebContents* web_contents)
    : web_contents_(web_contents) {
}

void WebContentsView::RenderWidgetHostDestroyed(RenderWidgetHost* host) {
  for (PendingWidgetViews::iterator i = pending_widget_views_.begin();
       i != pending_widget_views_.end(); ++i) {
    if (host->view() == i->second) {
      pending_widget_views_.erase(i);
      return;
    }
  }
}

void WebContentsView::CreateNewWindow(int route_id,
                                      base::WaitableEvent* modal_dialog_event) {
  // Create the new web contents. This will automatically create the new
  // WebContentsView. In the future, we may want to create the view separately.
  WebContents* new_contents =
      new WebContents(web_contents()->profile(),
                      web_contents()->GetSiteInstance(),
                      route_id,
                      modal_dialog_event);
  new_contents->SetupController(web_contents()->profile());
  WebContentsView* new_view = new_contents->view();

  // TODO(brettw) it seems bogus that we have to call this function on the
  // newly created object and give it one of its own member variables.
  new_view->CreateViewForWidget(new_contents->render_view_host());

  // Save the created window associated with the route so we can show it later.
  pending_contents_[route_id] = new_contents;
}

void WebContentsView::CreateNewWidget(int route_id, bool activatable) {
  // Save the created widget associated with the route so we can show it later.
  pending_widget_views_[route_id] = CreateNewWidgetInternal(route_id,
                                                            activatable);
}

void WebContentsView::ShowCreatedWindow(int route_id,
                                        WindowOpenDisposition disposition,
                                        const gfx::Rect& initial_pos,
                                        bool user_gesture) {
  PendingContents::iterator iter = pending_contents_.find(route_id);
  if (iter == pending_contents_.end()) {
    DCHECK(false);
    return;
  }

  WebContents* new_web_contents = iter->second;
  pending_contents_.erase(route_id);

  if (!new_web_contents->render_widget_host_view() ||
      !new_web_contents->process()->channel()) {
    // The view has gone away or the renderer crashed. Nothing to do.
    return;
  }

  // TODO(brettw) this seems bogus to reach into here and initialize the host.
  new_web_contents->render_view_host()->Init();
  web_contents()->AddNewContents(new_web_contents, disposition, initial_pos,
                                 user_gesture);
}

void WebContentsView::ShowCreatedWidget(int route_id,
                                        const gfx::Rect& initial_pos) {
  PendingWidgetViews::iterator iter = pending_widget_views_.find(route_id);
  if (iter == pending_widget_views_.end()) {
    DCHECK(false);
    return;
  }

  RenderWidgetHostView* widget_host_view = iter->second;
  pending_widget_views_.erase(route_id);

  ShowCreatedWidgetInternal(widget_host_view, initial_pos);
}

RenderWidgetHostView* WebContentsView::CreateNewWidgetInternal(
    int route_id,
    bool activatable) {
  RenderWidgetHost* widget_host =
      new RenderWidgetHost(web_contents_->process(), route_id);
  RenderWidgetHostView* widget_view =
      RenderWidgetHostView::CreateViewForWidget(widget_host);
  widget_view->set_activatable(activatable);

  return widget_view;
}

void WebContentsView::ShowCreatedWidgetInternal(
    RenderWidgetHostView* widget_host_view,
    const gfx::Rect& initial_pos) {
  RenderWidgetHost* widget_host = widget_host_view->GetRenderWidgetHost();
  if (!widget_host->process()->channel()) {
    // The view has gone away or the renderer crashed. Nothing to do.
    return;
  }

  widget_host_view->InitAsPopup(
      web_contents_->render_widget_host_view(), initial_pos);
  web_contents_->delegate()->RenderWidgetShowing();
  widget_host->Init();
}
