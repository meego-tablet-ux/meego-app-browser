// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/meegotouch/tab_contents_container_qt.h"

#include "base/i18n/rtl.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/browser/tab_contents/tab_contents_view.h"
#include "chrome/browser/renderer_host/render_widget_host_view_qt.h"
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
  void sizeChanged()
  {
    container_->ContainerSizeChanged();
  }

 private:
  TabContentsContainerQt* container_;
};
  
TabContentsContainerQt::TabContentsContainerQt(BrowserWindowQt* window)
    : tab_contents_(NULL),
      window_(window)
{
}

TabContentsContainerQt::~TabContentsContainerQt() {
  delete impl_;
}

void TabContentsContainerQt::Init() {
  QDeclarativeView *view = window_->DeclarativeView();
  QDeclarativeItem *item = view->rootObject()->findChild<QDeclarativeItem*>("innerContent");
  
  if (item)
  {
    LOG(INFO) << "find content item";
    widget_ = item;

    impl_ = new TabContentsContainerQtImpl(this);
    impl_->connect(item, SIGNAL(widthChanged()), impl_, SLOT(sizeChanged()));
    impl_->connect(item, SIGNAL(heightChanged()), impl_, SLOT(sizeChanged()));
  }
}

void TabContentsContainerQt::ContainerSizeChanged()
{
  if (tab_contents_) {
    QGraphicsWidget* tab_widget = tab_contents_->GetNativeView();
    if (tab_widget) {
      QRectF contentRect = widget_->boundingRect();
      tab_widget->setGeometry(0, 0, contentRect.width(), contentRect.height());
      gfx::Size size(int(contentRect.width()), int(contentRect.height()));

      tab_contents_->view()->SizeContents(size);
    }
  }
}

void TabContentsContainerQt::SetTabContents(TabContents* tab_contents) {
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
  }

  tab_contents_ = tab_contents;

  // When detaching the last tab of the browser SetTabContents is invoked
  // with NULL. Don't attempt to do anything in that case.
  if (tab_contents_) {
    registrar_.Add(this, NotificationType::RENDER_VIEW_HOST_CHANGED,
                   Source<NavigationController>(&tab_contents_->controller()));
    registrar_.Add(this, NotificationType::TAB_CONTENTS_DESTROYED,
                   Source<TabContents>(tab_contents_));

    QGraphicsWidget* tab_widget = tab_contents_->GetNativeView();
    if (tab_widget) {
      if(!tab_widget->parentItem())
      {
        tab_widget->setParentItem(widget_);
      }

      QRectF contentRect = widget_->boundingRect();
      tab_widget->setGeometry(0, 0, contentRect.width(), contentRect.height());
      gfx::Size size(int(contentRect.width()), int(contentRect.height()));

      tab_contents_->view()->SizeContents(size);

      ///\todo: hack to pass focus out event to old rwhv
      RenderWidgetHostView* view = tab_contents_->GetRenderWidgetHostView();
      if (view)
        view->Focus();

      tab_widget->show();
    }
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
  } else {
    NOTREACHED();
  }
}

void TabContentsContainerQt::RenderViewHostChanged(RenderViewHost* old_host,
                                                    RenderViewHost* new_host) {
  DNOTIMPLEMENTED();
}

void TabContentsContainerQt::TabContentsDestroyed(TabContents* contents) {
  // Sometimes, a TabContents is destroyed before we know about it. This allows
  // us to clean up our state in case this happens.
  DCHECK(contents == tab_contents_);
  SetTabContents(NULL);
}

#include "moc_tab_contents_container_qt.cc"
