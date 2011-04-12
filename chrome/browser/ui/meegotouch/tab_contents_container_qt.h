// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GTK_TAB_CONTENTS_CONTAINER_QT_H_
#define CHROME_BROWSER_GTK_TAB_CONTENTS_CONTAINER_QT_H_

#include "base/basictypes.h"
#include "content/common/notification_observer.h"
#include "content/common/notification_registrar.h"

class RenderViewHost;
class TabContents;
class BrowserWindowQt;
class QGraphicsItem;
class QDeclarativeItem;
class TabContentsContainerQtImpl;

class TabContentsContainerQt : public NotificationObserver
{
  friend class TabContentsContainerQtImpl;
 public:
  explicit TabContentsContainerQt(BrowserWindowQt* window);
  ~TabContentsContainerQt();

  void Init();

  // Make the specified tab visible.
  void SetTabContents(TabContents* tab_contents);
  TabContents* GetTabContents() const { return tab_contents_; }

  // Remove the tab from the hierarchy.
  void DetachTabContents(TabContents* tab_contents);

  // NotificationObserver implementation.
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

  QDeclarativeItem* widget() {return webview_item_;}

  void ViewportSizeChanged();
  
 private:
  // Add or remove observers for events that we care about.
  void AddObservers();
  void RemoveObservers();

  void RestoreViewportProperty();
  // Called when the RenderViewHost of the hosted TabContents has changed, e.g.
  // to show an interstitial page.
  void RenderViewHostChanged(RenderViewHost* old_host,
                             RenderViewHost* new_host);

  // Called when a TabContents is destroyed. This gives us a chance to clean
  // up our internal state if the TabContents is somehow destroyed before we
  // get notified.
  void TabContentsDestroyed(TabContents* contents);

  // Handler for |floating_|'s "set-floating-position" signal. During this
  // callback, we manually set the position of the status bubble.
  NotificationRegistrar registrar_;

  // The currently visible TabContents.
  TabContents* tab_contents_;

  QDeclarativeItem* webview_item_;
  QDeclarativeItem* viewport_item_;

  BrowserWindowQt* window_;

  TabContentsContainerQtImpl* impl_;

  DISALLOW_COPY_AND_ASSIGN(TabContentsContainerQt);
};

#endif  // CHROME_BROWSER_GTK_TAB_CONTENTS_CONTAINER_QT_H_
