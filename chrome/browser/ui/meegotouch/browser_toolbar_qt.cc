// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/meegotouch/browser_toolbar_qt.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"
#include "chrome/browser/ui/meegotouch/back_forward_button_qt.h"

#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/singleton.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/browser.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "chrome/browser/tab_contents/tab_contents_view_qt.h"
#include "chrome/browser/ui/meegotouch/tab_list_qt.h"
#include "chrome/browser/upgrade_detector.h"
#include "chrome/common/chrome_switches.h"
#include "content/common/notification_details.h"
#include "content/common/notification_service.h"
#include "content/common/notification_type.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"

#include <QDeclarativeEngine>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDeclarativeItem>
#include <QGraphicsLineItem>

#undef signals
#include "ui/gfx/canvas_skia_paint.h"

class BrowserToolbarQtImpl: public QObject
{
  Q_OBJECT
 public:
  BrowserToolbarQtImpl(BrowserToolbarQt* toolbar):
      QObject(NULL),
      toolbar_(toolbar)
  {
  }

 public slots:
  void wrenchButtonClicked()
  {
    toolbar_->ShowWrenchMenu();
  }

  void tabButtonClicked()
  {
    TabContents* tab_contents = toolbar_->browser()->GetSelectedTabContents();
    toolbar_->TabSideBarToggle();
  }
  
  void closeButtonClicked()
  {
    toolbar_->browser()->ExecuteCommandWithDisposition(IDC_CLOSE_WINDOW, CURRENT_TAB);
  }

  void backwardButtonClicked()
  {
    toolbar_->browser()->ExecuteCommandWithDisposition(IDC_BACK, CURRENT_TAB);
  }

  void forwardButtonClicked()
  {
    toolbar_->browser()->ExecuteCommandWithDisposition(IDC_FORWARD, CURRENT_TAB);
  }
  
  // back/forward/back-forward button is tapped
  void bfButtonTapped()
  {
    toolbar_->bfButtonTapped();
  }

  // back/forward/back-forward button is tapped and held
  void bfButtonTappedAndHeld()
  {
    toolbar_->bfButtonTappedAndHeld();
  }

  void reloadButtonClicked()
  {
    //    toolbar_->browser()->ExecuteCommandWithDisposition(IDC_RELOAD, CURRENT_TAB);
    toolbar_->ReloadButtonClicked();
  }

  void starButtonClicked()
  {
    toolbar_->browser()->ExecuteCommandWithDisposition(IDC_BOOKMARK_PAGE, CURRENT_TAB);
  }

  void UpdateStarButton(bool is_starred)
  {
    emit updateStarButton(is_starred);
  }

  void ShowStarButton(bool show)
  {
    emit showStarButton(show);
  }

  void refreshBfButton(int kind, bool active)
  {
    emit updateBfButton(kind, active);
  }

  void showHistory( int count)
  {
    emit showHistoryStack(count);
  }

  void goButtonClicked()
  {
    toolbar_->window()->GetLocationBar()->AcceptInput();
  }

  void UpdateReloadButton(bool is_loading)
  {
    emit updateReloadButton(is_loading);
  }

Q_SIGNALS:
  // update SatButton
  void updateStarButton(bool is_starred);
  void showStarButton(bool show);

  // update backward, forward, back-forward buttons
  void updateBfButton(int kind, bool active);

  // update reload/stop button
  void updateReloadButton(bool is_loading);

  void showHistoryStack(int count);

 private:
  BrowserToolbarQt* toolbar_;
};

BrowserToolbarQt::BrowserToolbarQt(Browser* browser, BrowserWindowQt* window):
    location_bar_(new LocationBarViewQt(browser, window)),
    wrench_menu_model_(this, browser),
    back_forward_(this, browser, window),
    browser_(browser),
    window_(window),
    tab_sidebar_(new TabListQt(browser, window))
{
  browser_->command_updater()->AddCommandObserver(IDC_BACK, this);
  browser_->command_updater()->AddCommandObserver(IDC_FORWARD, this);
  browser_->command_updater()->AddCommandObserver(IDC_HOME, this);
  browser_->command_updater()->AddCommandObserver(IDC_BOOKMARK_PAGE, this);

  impl_ = new BrowserToolbarQtImpl(this);

  QDeclarativeView* view = window->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();
  context->setContextProperty("browserToolbarModel", impl_);
}

BrowserToolbarQt::~BrowserToolbarQt() {
  delete  impl_;
  delete tab_sidebar_;
}

void BrowserToolbarQt::Init(Profile* profile) {
  // Make sure to tell the location bar the profile before calling its Init.
  SetProfile(profile);
  location_bar_->Init(false);  
}

void BrowserToolbarQt::ShowWrenchMenu()
{
  TabStripModel* model = browser_->tabstrip_model();
  browser_->command_updater()->UpdateCommandEnabled(IDC_NEW_TAB, !model->IsReachTabsLimit());
 
  gfx::Point p;
  window_->ShowContextMenu(&wrench_menu_model_, p);
}

void BrowserToolbarQt::UpdateTabContents(TabContents* contents, 
					  bool should_restore_state) {
  location_bar_->Update(contents);
}

void BrowserToolbarQt::TabSideBarToggle()
{
  if (tab_sidebar_->isVisible() )
  {
    tab_sidebar_->Hide();
  } else
  {
    tab_sidebar_->Show();
  }
}
  
LocationBar* BrowserToolbarQt::GetLocationBar() const {
  return location_bar_.get();
}

bool BrowserToolbarQt::GetAcceleratorForCommandId(
    int id,
    ui::Accelerator* accelerator) {
  return false;
}

//// CommandUpdater::CommandObserver ---------------------------------------------
//
void BrowserToolbarQt::EnabledStateChangedForCommand(int id, bool enabled) {
  switch (id) {
  case IDC_BACK:
  case IDC_FORWARD:
    back_forward_.updateStatus();
    break;
  case IDC_HOME:
    break;
  }
  DNOTIMPLEMENTED();
}

void BrowserToolbarQt::SetProfile(Profile* profile) {
  if (profile == profile_)
    return;

  profile_ = profile;
  location_bar_->SetProfile(profile);
}

void BrowserToolbarQt::SetStarred(bool is_starred) {
 if (browser_->GetSelectedTabContents()->GetURL()==GURL(chrome::kChromeUINewTabURL)) {
   impl_->ShowStarButton(false);
 } else {
   impl_->ShowStarButton(true);
 } 
 impl_->UpdateStarButton(is_starred);
}

void BrowserToolbarQt::bfButtonTapped()
{
  back_forward_.tap();
}

void BrowserToolbarQt::bfButtonTappedAndHeld()
{
  back_forward_.tapAndHold();
}

void BrowserToolbarQt::updateBfButton(int kind, bool active)
{
  impl_->refreshBfButton(kind, active);
}

void BrowserToolbarQt::showHistory(int count)
{
  impl_->showHistory(count);
}

void BrowserToolbarQt::UpdateReloadStopState(bool is_loading, bool force)
{
  _is_loading = is_loading;
  impl_->UpdateReloadButton(is_loading);
}

void BrowserToolbarQt::ReloadButtonClicked()
{
  if (_is_loading) 
  {
    browser_->Stop();
    _is_loading = false;
    impl_->UpdateReloadButton(false);
  }
  else
    browser_->ExecuteCommandWithDisposition(IDC_RELOAD, CURRENT_TAB);
}

void BrowserToolbarQt::UpdateTitle()
{
  location_bar_->UpdateTitle();
}
#include "moc_browser_toolbar_qt.cc"
