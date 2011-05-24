// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/meegotouch/tab_contents_container_qt.h"

#include "base/i18n/rtl.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/browser/tab_contents/tab_contents_view.h"
#include "chrome/browser/renderer_host/render_widget_host_view_qt.h"
#include "content/browser/renderer_host/render_view_host.h"
#include "content/browser/renderer_host/render_widget_host.h"
#include "content/browser/renderer_host/rwhv_qt_widget.h"
#include "content/common/notification_service.h"
#include "ui/gfx/native_widget_types.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"

#include <QDeclarativeEngine>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDeclarativeItem>
#include <QGraphicsLineItem>
#include <QGraphicsWidget>

class TabContentsContainerQtImpl: public QObject
{
  Q_OBJECT;
 public:
  TabContentsContainerQtImpl(TabContentsContainerQt* container):
      container_(container)
  {}

 public slots:
  void viewportSizeChanged()
  {
    container_->ViewportSizeChanged();
  }

  void contentPosChanged()
  {
    container_->ContentPosChanged();
  }

 private:
  TabContentsContainerQt* container_; 
};
  
TabContentsContainerQt::TabContentsContainerQt(BrowserWindowQt* window)
    : tab_contents_(NULL),
      window_(window),
      impl_(NULL),
      in_orientation_(false)
{
}

TabContentsContainerQt::~TabContentsContainerQt() {
  if (impl_)
    delete impl_;
}

void TabContentsContainerQt::Init() {
  QDeclarativeView *view = window_->DeclarativeView();
  QDeclarativeItem *viewport_item = view->rootObject()->findChild<QDeclarativeItem*>("innerContent");
  QDeclarativeItem *webview_item = view->rootObject()->findChild<QDeclarativeItem*>("webView");
  
  if (viewport_item && webview_item)
  {
    DLOG(INFO) << "find items";
    webview_item_ = webview_item;
    viewport_item_ = viewport_item;

    impl_ = new TabContentsContainerQtImpl(this);
    impl_->connect(viewport_item_, SIGNAL(widthChanged()), impl_, SLOT(viewportSizeChanged()));
    impl_->connect(viewport_item_, SIGNAL(heightChanged()), impl_, SLOT(viewportSizeChanged()));

    impl_->connect(viewport_item_, SIGNAL(contentXChanged()), impl_, SLOT(contentPosChanged()));
    impl_->connect(viewport_item_, SIGNAL(contentYChanged()), impl_, SLOT(contentPosChanged()));

  }
}

void TabContentsContainerQt::ViewportSizeChanged()
{
  if (in_orientation_) {
    DLOG(INFO) << "in orientation, don't adjust contents size now";
    return;
  }

  if (!viewport_item_)
    return;
  
  if (tab_contents_) {
    RenderWidgetHostView* host_view = tab_contents_->GetRenderWidgetHostView();
    if (host_view)
    {
      // set preferred size
      QRectF contentRect = viewport_item_->boundingRect();
      gfx::Size size(int(contentRect.width()), int(contentRect.height()));
      DLOG(INFO) << "ViewportSizeChanged " << size.width() << ", " << size.height();
      host_view->SetPreferredSize(size);
    }
  }
}

void TabContentsContainerQt::ContentPosChanged()
{
  if (tab_contents_) {
    RenderWidgetHostView* host_view = tab_contents_->GetRenderWidgetHostView();
    if (host_view)
    {
      host_view->ScenePosChanged();
    }
  }
}

void TabContentsContainerQt::SetTabContents(TabContents* tab_contents) {
  bool result;
  
  if (!webview_item_)
    return;
  
  if (tab_contents_) {
    QGraphicsWidget* tab_widget = tab_contents_->GetNativeView();
    if (tab_widget)
    {
      ///\todo: hack to pass focus out event to old rwhv
      RenderWidgetHostView* view = tab_contents_->GetRenderWidgetHostView();
      if (view)
        view->Blur();

      tab_widget->hide();
    }
    
    registrar_.Remove(this, NotificationType::RENDER_VIEW_HOST_CHANGED,
        Source<NavigationController>(&tab_contents_->controller()));
    registrar_.Remove(this, NotificationType::TAB_CONTENTS_DESTROYED,
                      Source<TabContents>(tab_contents_));
    registrar_.Remove(this, NotificationType::NAV_ENTRY_COMMITTED,
                      Source<NavigationController>(&tab_contents_->controller()));
  }

  tab_contents_ = tab_contents;

  // When detaching the last tab of the browser SetTabContents is invoked
  // with NULL. Don't attempt to do anything in that case.
  if (tab_contents_) {
    registrar_.Add(this, NotificationType::RENDER_VIEW_HOST_CHANGED,
                   Source<NavigationController>(&tab_contents_->controller()));
    registrar_.Add(this, NotificationType::TAB_CONTENTS_DESTROYED,
                   Source<TabContents>(tab_contents_));
    registrar_.Add(this, NotificationType::NAV_ENTRY_COMMITTED,
                      Source<NavigationController>(&tab_contents_->controller()));

    QGraphicsWidget* tab_widget = tab_contents_->GetNativeView();
    if (tab_widget) {
      if(!tab_widget->parentItem())
      {
        tab_widget->setParentItem(webview_item_);
      }

      ViewportSizeChanged();
      
      ///\todo: hack to pass focus out event to old rwhv
      RenderWidgetHostView* view = tab_contents_->GetRenderWidgetHostView();
      if (view)
        view->Focus();

      tab_widget->show();
    }
  }
}

void TabContentsContainerQt::RestoreViewportProperty()
{
  QDeclarativeView *view = window_->DeclarativeView();
  QDeclarativeItem *viewport_item = view->rootObject()->findChild<QDeclarativeItem*>("innerContent");
  QDeclarativeItem *webview_item = view->rootObject()->findChild<QDeclarativeItem*>("webView");

  if(viewport_item) {
    viewport_item->setProperty("contentX", QVariant(0));
    viewport_item->setProperty("contentY", QVariant(0));
  }

  QRectF contentRect = viewport_item_->boundingRect();

  QGraphicsWidget* widget = tab_contents_->GetContentNativeView();

  if(widget) {
    RWHVQtWidget* rwhv = reinterpret_cast<RWHVQtWidget*>(widget);
    if(rwhv->scale() != 1.0){ rwhv->SetScaleFactor(1.0); }
  }

}

void TabContentsContainerQt::DetachTabContents(TabContents* tab_contents) {
  if (tab_contents_) {
    QGraphicsWidget* tab_widget = tab_contents_->GetNativeView();
    if (tab_widget)
    {
      tab_widget->setParentItem(NULL);
    }
  }
}
void TabContentsContainerQt::Observe(NotificationType type,
    const NotificationSource& source,
    const NotificationDetails& details) {
  if (type == NotificationType::RENDER_VIEW_HOST_CHANGED) {
    RenderViewHostSwitchedDetails* switched_details =
      Details<RenderViewHostSwitchedDetails>(details).ptr();
    RenderViewHostChanged(switched_details->old_host,
        switched_details->new_host);
  } else if (type == NotificationType::TAB_CONTENTS_DESTROYED) {
    TabContentsDestroyed(Source<TabContents>(source).ptr());
  } else if (type == NotificationType::NAV_ENTRY_COMMITTED) {
    NavigationController::LoadCommittedDetails&
      commited_details = *(Details<NavigationController::LoadCommittedDetails>(details).ptr());

    if(commited_details.entry->url() != commited_details.previous_url
        && !commited_details.is_in_page)
      RestoreViewportProperty();

  }else {
    NOTREACHED();
  }
}

void TabContentsContainerQt::RenderViewHostChanged(RenderViewHost* old_host,
                                                    RenderViewHost* new_host) {
    if(new_host == NULL) return;

    QRectF contentRect = viewport_item_->boundingRect();
    QRect  qrect = contentRect.toAlignedRect();
    
    gfx::Rect rect(0, 0, qrect.width(), qrect.height());
    new_host->SetPreferredSize(gfx::Size(rect.width(), rect.height()));
    
    RenderWidgetHostView* view = new_host->view();

    if (view)
    {
      view->Focus();
    }

    QGraphicsWidget* rwhv_widget = tab_contents_->GetContentNativeView();
    if(rwhv_widget) rwhv_widget->update(contentRect);
}

void TabContentsContainerQt::TabContentsDestroyed(TabContents* contents) {
  // Sometimes, a TabContents is destroyed before we know about it. This allows
  // us to clean up our state in case this happens.
  DCHECK(contents == tab_contents_);
  SetTabContents(NULL);
}

void TabContentsContainerQt::OrientationStart()
{
  in_orientation_ = true;
}

void TabContentsContainerQt::OrientationEnd()
{
  in_orientation_ = false;
  ViewportSizeChanged();
}

#include "moc_tab_contents_container_qt.cc"
