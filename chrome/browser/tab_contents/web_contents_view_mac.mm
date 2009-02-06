// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/tab_contents/web_contents_view_mac.h"

#include "chrome/browser/browser.h" // TODO(beng): this dependency is awful.
#include "chrome/browser/renderer_host/render_widget_host.h"
#include "chrome/browser/renderer_host/render_widget_host_view_mac.h"
#include "chrome/browser/tab_contents/web_contents.h"

#include "chrome/common/temp_scaffolding_stubs.h"

// static
WebContentsView* WebContentsView::Create(WebContents* web_contents) {
  return new WebContentsViewMac(web_contents);
}

WebContentsViewMac::WebContentsViewMac(WebContents* web_contents)
    : web_contents_(web_contents) {
}

WebContentsViewMac::~WebContentsViewMac() {
}

WebContents* WebContentsViewMac::GetWebContents() {
  return web_contents_;
}

void WebContentsViewMac::CreateView() {
  WebContentsViewCocoa* view =
      [[WebContentsViewCocoa alloc] initWithFrame:NSZeroRect];
  // Under GC, ObjC and CF retains/releases are no longer equivalent. So we
  // change our ObjC retain to a CF retain se we can use a scoped_cftyperef.
  CFRetain(view);
  [view release];
  cocoa_view_.reset(view);
}

RenderWidgetHostView* WebContentsViewMac::CreateViewForWidget(
    RenderWidgetHost* render_widget_host) {
  DCHECK(!render_widget_host->view());
  RenderWidgetHostViewMac* view =
      new RenderWidgetHostViewMac(render_widget_host);
  
  // Fancy layout comes later; for now just make it our size and resize it
  // with us.
  NSView* view_view = view->GetNativeView();
  [cocoa_view_.get() addSubview:view_view];
  [view_view setFrame:[cocoa_view_.get() bounds]];
  [view_view setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
  
  return view;
}

gfx::NativeView WebContentsViewMac::GetNativeView() const {
  return cocoa_view_.get();
}

gfx::NativeView WebContentsViewMac::GetContentNativeView() const {
  if (!web_contents_->render_widget_host_view())
    return NULL;
  return web_contents_->render_widget_host_view()->GetPluginNativeView();
}

gfx::NativeWindow WebContentsViewMac::GetTopLevelNativeView() const {
  return [cocoa_view_.get() window];
}

void WebContentsViewMac::GetContainerBounds(gfx::Rect* out) const {
  *out = gfx::Rect(NSRectToCGRect([cocoa_view_.get() frame]));
}

void WebContentsViewMac::StartDragging(const WebDropData& drop_data) {
  NOTIMPLEMENTED();
}

void WebContentsViewMac::OnContentsDestroy() {
  // TODO(avi):Close the find bar if any.
  if (find_bar_.get())
    find_bar_->Close();
}

void WebContentsViewMac::SetPageTitle(const std::wstring& title) {
  // Meaningless on the Mac; widgets don't have a "title" attribute
}

void WebContentsViewMac::Invalidate() {
  [cocoa_view_.get() setNeedsDisplay:YES];
}

void WebContentsViewMac::SizeContents(const gfx::Size& size) {
  // TODO(brettw) this is a hack and should be removed. See web_contents_view.h.
  NSRect rect = [cocoa_view_.get() frame];
  rect.size = NSSizeFromCGSize(size.ToCGSize());
  [cocoa_view_.get() setBounds:rect];
}

void WebContentsViewMac::FindInPage(const Browser& browser,
                                    bool find_next, bool forward_direction) {
  if (!find_bar_.get()) {
    // We want the Chrome top-level (Frame) window.
    NSWindow* window =
        static_cast<NSWindow*>(browser.window()->GetNativeHandle());
    find_bar_.reset(new FindBarMac(this, window));
  } else {
    find_bar_->Show();
  }

  if (find_next && !find_bar_->find_string().empty())
    find_bar_->StartFinding(forward_direction);
}

void WebContentsViewMac::HideFindBar(bool end_session) {
  if (find_bar_.get()) {
    if (end_session)
      find_bar_->EndFindSession();
    else
      find_bar_->DidBecomeUnselected();
  }
}

bool WebContentsViewMac::GetFindBarWindowInfo(gfx::Point* position,
                                              bool* fully_visible) const {
  if (!find_bar_.get() ||
      [find_bar_->GetView() isHidden]) {
    *position = gfx::Point(0, 0);
    *fully_visible = false;
    return false;
  }

  NSRect frame = [find_bar_->GetView() frame];
  *position = gfx::Point(frame.origin.x, frame.origin.y);
  *fully_visible = find_bar_->IsVisible() && !find_bar_->IsAnimating();
  return true;
}

void WebContentsViewMac::UpdateDragCursor(bool is_drop_target) {
  NOTIMPLEMENTED();
}

void WebContentsViewMac::TakeFocus(bool reverse) {
  [cocoa_view_.get() becomeFirstResponder];
}

void WebContentsViewMac::HandleKeyboardEvent(const WebKeyboardEvent& event) {
  // The renderer returned a keyboard event it did not process. TODO(avi):
  // reconstruct an NSEvent and feed it to the view.
  NOTIMPLEMENTED();
}

void WebContentsViewMac::OnFindReply(int request_id,
                                     int number_of_matches,
                                     const gfx::Rect& selection_rect,
                                     int active_match_ordinal,
                                     bool final_update) {
  if (find_bar_.get()) {
    find_bar_->OnFindReply(request_id, number_of_matches, selection_rect,
                           active_match_ordinal, final_update);
  }
}

void WebContentsViewMac::ShowContextMenu(const ContextMenuParams& params) {
  NOTIMPLEMENTED();
}

WebContents* WebContentsViewMac::CreateNewWindowInternal(
    int route_id,
    base::WaitableEvent* modal_dialog_event) {
  // Create the new web contents. This will automatically create the new
  // WebContentsView. In the future, we may want to create the view separately.
  WebContents* new_contents =
      new WebContents(web_contents_->profile(),
                      web_contents_->GetSiteInstance(),
                      web_contents_->render_view_factory_,
                      route_id,
                      modal_dialog_event);
  new_contents->SetupController(web_contents_->profile());
  WebContentsView* new_view = new_contents->view();

  new_view->CreateView();

  // TODO(brettw) it seems bogus that we have to call this function on the
  // newly created object and give it one of its own member variables.
  new_view->CreateViewForWidget(new_contents->render_view_host());
  return new_contents;
}

RenderWidgetHostView* WebContentsViewMac::CreateNewWidgetInternal(
    int route_id,
    bool activatable) {
  // Create the widget and its associated view.
  // TODO(brettw) can widget creation be cross-platform?
  RenderWidgetHost* widget_host =
      new RenderWidgetHost(web_contents_->process(), route_id);
  RenderWidgetHostViewMac* widget_view =
      new RenderWidgetHostViewMac(widget_host);

  // Some weird reparenting stuff happens here. TODO(avi): figure that out

  return widget_view;
}

void WebContentsViewMac::ShowCreatedWindowInternal(
    WebContents* new_web_contents,
    WindowOpenDisposition disposition,
    const gfx::Rect& initial_pos,
    bool user_gesture) {
  if (!new_web_contents->render_widget_host_view() ||
      !new_web_contents->process()->channel()) {
    // The view has gone away or the renderer crashed. Nothing to do.
    return;
  }

  // TODO(brettw) this seems bogus to reach into here and initialize the host.
  new_web_contents->render_view_host()->Init();
  web_contents_->AddNewContents(new_web_contents, disposition, initial_pos,
                                user_gesture);
}

void WebContentsViewMac::ShowCreatedWidgetInternal(
    RenderWidgetHostView* widget_host_view,
    const gfx::Rect& initial_pos) {
  RenderWidgetHost* widget_host = widget_host_view->GetRenderWidgetHost();
  if (!widget_host->process()->channel()) {
    // The view has gone away or the renderer crashed. Nothing to do.
    return;
  }
  
  // Reparenting magic goes here. TODO(avi): fix

  widget_host->Init();
}

@implementation WebContentsViewCocoa

// Tons of stuff goes here, where we grab events going on in Cocoaland and send
// them into the C++ system. TODO(avi): all that jazz

@end
