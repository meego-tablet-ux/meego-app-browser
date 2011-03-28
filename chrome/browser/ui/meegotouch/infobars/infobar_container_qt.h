// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GTK_INFOBAR_CONTAINER_QT_H_
#define CHROME_BROWSER_GTK_INFOBAR_CONTAINER_QT_H_
#pragma once

#include "base/basictypes.h"
#include "content/common/notification_observer.h"
#include "content/common/notification_registrar.h"

class InfoBarDelegate;
class Profile;
class BrowserWindowQt;
class TabContents;
class InfoBarContainerQtImpl;

class InfoBarContainerQt : public NotificationObserver {
 public:
  explicit InfoBarContainerQt(Profile* profile, BrowserWindowQt* window);
  virtual ~InfoBarContainerQt();

  // Changes the TabContents for which this container is showing InfoBars. Can
  // be NULL, in which case we will simply detach ourselves from the old tab
  // contents.
  void ChangeTabContents(TabContents* contents);

  // Remove the specified InfoBarDelegate from the selected TabContents. This
  // will notify us back and cause us to close the View. This is called from
  // the InfoBar's close button handler.
  void RemoveDelegate(InfoBarDelegate* delegate);

  // Returns the total pixel height of all infobars in this container that
  // are currently animating.
  int TotalHeightOfAnimatingBars() const;

 private:
  // Overridden from NotificationObserver:
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

  // Constructs the InfoBars needed to reflect the state of the current
  // TabContents associated with this container. No animations are run during
  // this process.
  void UpdateInfoBars();

  // Adds an InfoBar for the specified delegate, in response to a notification
  // from the selected TabContents.
  void AddInfoBar(InfoBarDelegate* delegate, bool animate);

  // Removes an InfoBar for the specified delegate, in response to a
  // notification from the selected TabContents. The InfoBar's disappearance
  // will be animated.
  void RemoveInfoBar(InfoBarDelegate* delegate, bool animate);

  NotificationRegistrar registrar_;

  // The profile for the browser that hosts this InfoBarContainer.
  Profile* profile_;
  BrowserWindowQt* window_;

  // The TabContents for which we are currently showing InfoBars.
  TabContents* tab_contents_;

  InfoBarContainerQtImpl* impl_;

  DISALLOW_COPY_AND_ASSIGN(InfoBarContainerQt);
};

#endif  // CHROME_BROWSER_GTK_INFOBAR_CONTAINER_QT_H_
