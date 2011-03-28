// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEEGOTOUCH_BROWSER_TOOLBAR_QT_H_
#define CHROME_BROWSER_MEEGOTOUCH_BROWSER_TOOLBAR_QT_H_

#include "ui/base/models/accelerator.h"
#include "base/scoped_ptr.h"
#include "chrome/browser/command_updater.h"
#include "chrome/browser/prefs/pref_member.h"
#include "chrome/browser/ui/toolbar/wrench_menu_model.h"
#include "chrome/browser/ui/meegotouch/back_forward_button_qt.h"
#include "content/common/notification_observer.h"
#include "content/common/notification_registrar.h"

#include "chrome/browser/ui/meegotouch/location_bar_view_qt.h"

class Browser;
class BrowserWindowQt;
class LocationBar;
class TabContents;
class BrowserToolbarQt;
class TabListQt;
class BrowserToolbarQtImpl;
class QGraphicsItem;

// View class that displays the GTK version of the toolbar and routes gtk
// events back to the Browser.
class BrowserToolbarQt  : public CommandUpdater::CommandObserver,
                                public ui::AcceleratorProvider
{

 public:
  explicit BrowserToolbarQt(Browser* browser, BrowserWindowQt* window);
  virtual ~BrowserToolbarQt();

  void TabSideBarToggle();
  // Create the contents of the toolbar. |top_level_window| is the GtkWindow
  // to which we attach our accelerators.
  void Init(Profile* profile);

    //set bookmark status for current webpage
  void SetStarred(bool is_starred);

  // Getter for the containing widget.
  QGraphicsItem* widget() {
    return NULL;
  }

  // Getter for associated browser object.
  Browser* browser() {
    return browser_;
  }

  BrowserWindowQt* window() {
      return window_;
  }

  virtual LocationBar* GetLocationBar() const;

  LocationBarViewQt* GetLocationBarView() { return location_bar_.get(); }

// Overridden from ui::AcceleratorProvider:
  virtual bool GetAcceleratorForCommandId(int id,
                                          ui::Accelerator* accelerator);

  // Overridden from CommandUpdater::CommandObserver:
  virtual void EnabledStateChangedForCommand(int id, bool enabled);

 Profile* profile() { return profile_; }
 void SetProfile(Profile* profile);

 // Message that we should react to a state change.
 void UpdateTabContents(TabContents* contents, bool should_restore_state);

 void ShowWrenchMenu();

 // handler when backward, forward and backforward buttons are tapped
 void bfButtonTapped();

 // handler when backward, forward and backforward buttons are tapped and held
 void bfButtonTappedAndHeld();

 // update backward, forward and backforward buttons
 void updateBfButton(int kind, bool active);

 void showHistory();

 // update title in Omnibox when out of focus
 void UpdateTitle();

 // update reload/stop button 
 void UpdateReloadStopState(bool is_loading, bool force);
 void ReloadButtonClicked();

 private:
  scoped_ptr<LocationBarViewQt> location_bar_;

  WrenchMenuModel wrench_menu_model_;

  // back, forward button
  BackForwardButtonQt back_forward_;

  Browser* browser_;
  BrowserWindowQt* window_;

  BrowserToolbarQtImpl* impl_;
  
  Profile* profile_;

  TabListQt* tab_sidebar_;
  bool _is_loading;

  DISALLOW_COPY_AND_ASSIGN(BrowserToolbarQt);
};

#endif  // CHROME_BROWSER_MEEGOTOUCH_BROWSER_TOOLBAR_QT_H_
