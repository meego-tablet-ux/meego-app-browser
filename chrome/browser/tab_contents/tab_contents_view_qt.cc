// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/tab_contents/tab_contents_view_qt.h"
#include <iostream>

#include "base/pickle.h"
#include "base/string_util.h"
#include "build/build_config.h"
#include "chrome/browser/download/download_shelf.h"
#include "content/browser/renderer_host/render_view_host.h"
#include "content/browser/renderer_host/render_view_host_factory.h"
#include "content/browser/renderer_host/rwhv_qt_widget.h"
#include "content/browser/tab_contents/interstitial_page.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/browser/tab_contents/tab_contents_delegate.h"
//#include "chrome/browser/tab_contents/web_drag_dest_gtk.h"
#include "content/common/notification_service.h"
#include "content/common/notification_source.h"
#include "content/common/notification_type.h"
#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/size.h"
#include "webkit/glue/webdropdata.h"
#undef signals
#include "chrome/browser/tab_contents/render_view_context_menu_qt.h"
#include "chrome/browser/renderer_host/render_widget_host_view_qt.h"
#include "chrome/browser/ui/meegotouch/qt_util.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"
#include "chrome/browser/ui/meegotouch/popup_list_qt.h"
#include "webkit/glue/webmenuitem.h"
#include "chrome/browser/browser_list.h"

#include <QGraphicsSceneContextMenuEvent>
#include <QOrientationReading>

using WebKit::WebDragOperation;
using WebKit::WebDragOperationsMask;

// static
TabContentsView* TabContentsView::Create(TabContents* tab_contents) {
  return new TabContentsViewQt(tab_contents);
}

TabContentsViewQt::TabContentsViewQt(TabContents* tab_contents)
    : TabContentsView(tab_contents),
      container_(new QGraphicsWidget),
      rwhv_view_(NULL) {
}

void TabContentsViewQt::OnResize()
{
  LOG(INFO) << "rwhv_view_ size changed to = " << rwhv_view_->geometry().width()
             << ", " << rwhv_view_->geometry().height();
  LOG(INFO) << "TabContentsViewQt::widget size changed to =" << container_->geometry().width()
             << ", " << container_->geometry().height();
  RenderWidgetHostView* rwhv = tab_contents()->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->SetSize(gfx::Size(container_->geometry().width(), container_->geometry().height()));
}


TabContentsViewQt::~TabContentsViewQt() {
  DNOTIMPLEMENTED();
}

void TabContentsViewQt::AttachConstrainedWindow(
    ConstrainedWindowQt* constrained_window) {
  DNOTIMPLEMENTED();
}

void TabContentsViewQt::RemoveConstrainedWindow(
    ConstrainedWindowQt* constrained_window) {
  DNOTIMPLEMENTED();
}

void TabContentsViewQt::CreateView(const gfx::Size& initial_size) {
  LOG(INFO) << "CreateView : " << initial_size.width() << initial_size.height();
}

RenderWidgetHostView* TabContentsViewQt::CreateViewForWidget(
    RenderWidgetHost* render_container_host) {
  RenderWidgetHostViewQt* view = new RenderWidgetHostViewQt(render_container_host);
  view->InitAsChild();
  
  rwhv_view_ = view->GetNativeView();

  InsertIntoContentArea(rwhv_view_);

  rwhv_ = view;
  
  return view;
}

void TabContentsViewQt::InsertIntoContentArea(QGraphicsWidget* widget) {
  widget->setParentItem(container_);
}


gfx::NativeView TabContentsViewQt::GetNativeView() const {
  //return container_;
  return container_;
}

gfx::NativeView TabContentsViewQt::GetContentNativeView() const {
  // get native view of RenderWidgetHostView
  return rwhv_view_;
}

gfx::NativeWindow TabContentsViewQt::GetTopLevelNativeWindow() const {
  DNOTIMPLEMENTED();
  return NULL;
}

void TabContentsViewQt::GetContainerBounds(gfx::Rect* out) const {
  int x = 0;
  int y = 0;
  
  QRectF contentRect;
  if (container_->parentItem())
    contentRect = container_->parentItem()->boundingRect();
  out->SetRect(x, y,
               int(contentRect.width()), int(contentRect.height()));//requested_size_.width(), requested_size_.height());
  LOG(INFO) << "TabContentsViewQt::GetContainerBounds " << out->x() << out->y()
             << out->width() << out->height();
}

void TabContentsViewQt::SetPageTitle(const std::wstring& title) {
  // Set the window name to include the page title so it's easier to spot
  // when debugging (e.g. via xwininfo -tree).
  DNOTIMPLEMENTED();
}

void TabContentsViewQt::OnTabCrashed(base::TerminationStatus status,
                                      int error_code) {
  DNOTIMPLEMENTED();
}

void TabContentsViewQt::SizeContents(const gfx::Size& size) {
  // We don't need to manually set the size of of widgets in GTK+, but we do
  // need to pass the sizing information on to the RWHV which will pass the
  // sizing information on to the renderer.
  LOG(INFO) << "-------" << __PRETTY_FUNCTION__ << std::endl;
  requested_size_ = size;
  RenderWidgetHostView* rwhv = tab_contents()->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->SetSize(size);
}

void TabContentsViewQt::Focus() {
    if (tab_contents()->showing_interstitial_page() ) {
      tab_contents()->interstitial_page()->Focus();
    } else {
      gfx::NativeView widget = GetContentNativeView();
      if (widget)
	widget->setFocus();
      QGraphicsItem *parent = widget->parentItem();
      while (parent) {
	if (parent->flags() & QGraphicsItem::ItemIsFocusScope)
	  parent->setFocus(Qt::OtherFocusReason);
	parent = parent->parentItem();
      }
    }
}

void TabContentsViewQt::SetInitialFocus() {
  if (tab_contents()->FocusLocationBarByDefault() )
    tab_contents()->SetFocusToLocationBar(false);
  else
    Focus();
}

void TabContentsViewQt::StoreFocus() {
  DNOTIMPLEMENTED();
}

void TabContentsViewQt::RestoreFocus() {
  DNOTIMPLEMENTED();
}

void TabContentsViewQt::GetViewBounds(gfx::Rect* out) const {
  DNOTIMPLEMENTED();
}

void TabContentsViewQt::UpdateDragCursor(WebDragOperation operation) {
  DNOTIMPLEMENTED();
}

void TabContentsViewQt::GotFocus() {
  // This is only used in the views FocusManager stuff but it bleeds through
  // all subclasses. http://crbug.com/21875
  DNOTIMPLEMENTED();
}

// This is called when we the renderer asks us to take focus back (i.e., it has
// iterated past the last focusable element on the page).
void TabContentsViewQt::TakeFocus(bool reverse) {
  DNOTIMPLEMENTED();
}

void TabContentsViewQt::Observe(NotificationType type,
                                 const NotificationSource& source,
                                 const NotificationDetails& details) {
  DNOTIMPLEMENTED();
}

void TabContentsViewQt::ShowContextMenu(const ContextMenuParams& params) {
  context_menu_.reset(new RenderViewContextMenuQt(tab_contents(), params, 0));
  context_menu_->Init();
#if 1
  gfx::Rect bounds;
  GetContainerBounds(&bounds);
  gfx::Point point = bounds.origin();
  point.Offset(params.x, params.y);

  //QtMobility::QOrientationReading::Orientation angle = static_cast<RWHVQtWidget*>(rwhv_view_)->orientationAngle();
  //QPoint offset = MapScenePosToOrientationAngle(rwhv_view_->scenePos().toPoint(), angle);
  //point.Offset(offset.x(), offset.y());

  context_menu_->Popup(point);
#else
  // we choose to pop up context menu at the central position
  context_menu_->Popup();
#endif
}

void TabContentsViewQt::ShowPopupMenu(const gfx::Rect& bounds,
                                       int item_height,
                                       double item_font_size,
                                       int selected_item,
                                       const std::vector<WebMenuItem>& items,
                                       bool right_aligned) {
  std::vector<WebMenuItem>::const_iterator iter;
  
  for(iter = items.begin(); iter != items.end(); ++iter)  {
    DLOG(INFO) << ">> " << (*iter).label;
  }

  Browser* browser = BrowserList::GetLastActive();
  BrowserWindowQt* browser_window = (BrowserWindowQt*)browser->window();

  PopupListQt* popup_list = browser_window->GetWebPopupList();
  popup_list->PopulateMenuItemData(selected_item, items);
  popup_list->SetHeaderBounds(bounds);
  popup_list->setCurrentView(this);
  popup_list->show();

}


void TabContentsViewQt::selectPopupItem(int index)
{
  RenderViewHost* host = tab_contents()->render_view_host();
  if (host) {
    if (index >= 0) {
      host->DidSelectPopupMenuItem(index);
    } else if (index == -1) {
      host->DidCancelPopupMenu();
    } else {
      DLOG(ERROR) << "Invalid Index";
    }
  }
}


// Render view DnD -------------------------------------------------------------

void TabContentsViewQt::StartDragging(const WebDropData& drop_data,
                                       WebDragOperationsMask ops,
                                       const SkBitmap& image,
                                       const gfx::Point& image_offset) {
  DNOTIMPLEMENTED();
  ///\todo We don't support drag yet.
  // Send drag ended message to backend so that it won't stop processing other input events
  if (tab_contents()->render_view_host())
      tab_contents()->render_view_host()->DragSourceSystemDragEnded();
}
