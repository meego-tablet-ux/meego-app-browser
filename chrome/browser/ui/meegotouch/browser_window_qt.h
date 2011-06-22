// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_QT_BROWSER_WINDOW_QT_H_
#define CHROME_BROWSER_QT_BROWSER_WINDOW_QT_H_

#include <map>
#include "chrome/browser/ui/meegotouch/tab_contents_container_qt.h"
#include "chrome/browser/ui/meegotouch/browser_toolbar_qt.h"
#include "ui/gfx/point.h"
#include "ui/base/models/menu_model.h"

#include "ui/base/x/x11_util.h"
#include "base/scoped_ptr.h"
#include "base/timer.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/prefs/pref_member.h"
#include "chrome/browser/tabs/tab_strip_model.h"
#include "content/common/notification_registrar.h"
#include "ui/gfx/rect.h"
#include "content/common/notification_type.h"
#include "chrome/browser/ui/meegotouch/location_bar_view_qt.h"
#include "chrome/browser/ui/meegotouch/bookmark_qt.h"
#include "chrome/browser/ui/meegotouch/dialog_qt.h"
#include "chrome/browser/ui/meegotouch/select_file_dialog_qt.h"
#include "chrome/browser/ui/meegotouch/new_tab_ui_qt.h"
#include "chrome/browser/ui/meegotouch/fullscreen_exit_bubble_qt.h"
#include "chrome/browser/ui/meegotouch/bookmark_bubble_qt.h"
#include "chrome/browser/ui/meegotouch/downloads_handler_qt.h"
#include "chrome/browser/ui/meegotouch/find_bar_qt.h"
#include "chrome/browser/ui/meegotouch/crash_tab_qt.h"
#include "chrome/browser/ui/meegotouch/crash_modal_dialog_qt.h"
#include "chrome/browser/ui/meegotouch/ssl_dialog_qt.h"
#include "chrome/browser/ui/meegotouch/selection_handler_qt.h"

class LauncherWindow;
class QDeclarativeView;
class BookmarkBubbleQt;
class Browser;
class BrowserTitlebar;
class CustomDrawButton;
class LocationBar;
class MExpandedContainer;
class InfoBarContainerQt;
class MenuQt;
class PopupListQt;
class DialogQt;
class DialogQtModel;
class SelectFileDialogQtImpl;
class NewTabUIQt;
class FullscreenExitBubbleQt;
class DialogQtResultListener;
class BrowserWindowQtImpl;
class DownloadsQtHandler;
class CrashTabQt;
class CrashAppModalDialog;
class CrashTabQtModel;
class SSLDialogQt;
//
// An implementation of BrowserWindow for QT.
// Cross-platform code will interact with this object when
// it needs to manipulate the window.

class BrowserWindowQt : //public MApplicationWindow,
                         public BrowserWindow,
                         public NotificationObserver,
                         public TabStripModelObserver{
  friend class BrowserWindowQtImpl;
 public:
  explicit BrowserWindowQt(Browser* browser, QWidget* parent=0);
  virtual ~BrowserWindowQt();

  // Overridden from BrowserWindow
  virtual void Show();
  virtual void ShowInactive();
  virtual void SetBounds(const gfx::Rect& bounds) {DNOTIMPLEMENTED();};
  virtual void Close();
  virtual void Activate() {DNOTIMPLEMENTED();};
  virtual void Deactivate() {DNOTIMPLEMENTED();};
  virtual void FocusAppMenu() {DNOTIMPLEMENTED();};
  virtual void ShowCollectedCookiesDialog(TabContents* tab_contents) {DNOTIMPLEMENTED();};
  virtual bool IsActive() const {DNOTIMPLEMENTED(); return true;};
  virtual void FlashFrame() {DNOTIMPLEMENTED();};
  virtual gfx::NativeWindow GetNativeHandle() {DNOTIMPLEMENTED(); return NULL;};
  virtual BrowserWindowTesting* GetBrowserWindowTesting() {DNOTIMPLEMENTED(); return NULL;};
  virtual void ToolbarSizeChanged(bool is_animating) {DNOTIMPLEMENTED();};
  // No StatusBubble
  virtual StatusBubble* GetStatusBubble() {/*DNOTIMPLEMENTED();*/ return NULL;};
  virtual void SelectedTabToolbarSizeChanged(bool is_animating) {DNOTIMPLEMENTED();};
  virtual void SelectedTabExtensionShelfSizeChanged() {DNOTIMPLEMENTED();};
  virtual void UpdateTitleBar();
  virtual void ShelfVisibilityChanged() {DNOTIMPLEMENTED();};
  virtual void UpdateDevTools() {DNOTIMPLEMENTED();};
  virtual void UpdateLoadingAnimations(bool should_animate) {DNOTIMPLEMENTED();};
  virtual void SetStarredState(bool is_starred);
  virtual gfx::Rect GetRestoredBounds() const;
  virtual gfx::Rect GetBounds() const {return gfx::Rect();};
  virtual bool IsMaximized() const {DNOTIMPLEMENTED(); return false;};
  virtual void SetFullscreen(bool fullscreen);
  virtual bool IsFullscreen() const;
  virtual bool IsFullscreenBubbleVisible() const {DNOTIMPLEMENTED(); return false;};
  virtual LocationBar* GetLocationBar() const ;
  virtual void SetFocusToLocationBar(bool select_all) {DNOTIMPLEMENTED();};
  virtual void UpdateReloadStopState(bool is_loading, bool force);
  virtual void UpdateToolbar(TabContentsWrapper* contents,
                             bool should_restore_state);
  virtual void FocusToolbar() {DNOTIMPLEMENTED();};
  virtual void FocusPageAndAppMenus() {DNOTIMPLEMENTED();};
  virtual void FocusBookmarksToolbar() {DNOTIMPLEMENTED();};
  virtual void FocusChromeOSStatus() {DNOTIMPLEMENTED();};
  virtual void RotatePaneFocus(bool forwards) {DNOTIMPLEMENTED();};
  virtual bool IsBookmarkBarVisible() const {DNOTIMPLEMENTED(); return false;};
  virtual bool IsBookmarkBarAnimating() const {DNOTIMPLEMENTED(); return false;};
  virtual bool IsTabStripEditable() const {DNOTIMPLEMENTED(); return false;};
  virtual bool IsToolbarVisible() const {DNOTIMPLEMENTED(); return false;};
  virtual gfx::Rect GetRootWindowResizerRect() const {return gfx::Rect();};
  virtual void ConfirmAddSearchProvider(const TemplateURL* template_url,
                                        Profile* profile) {DNOTIMPLEMENTED();};
  virtual void ToggleBookmarkBar() {DNOTIMPLEMENTED();};
  virtual void ToggleExtensionShelf() {DNOTIMPLEMENTED();};
  virtual void ShowAboutChromeDialog() {DNOTIMPLEMENTED();};
  virtual void ShowUpdateChromeDialog() {DNOTIMPLEMENTED();};
  virtual void ShowTaskManager() {DNOTIMPLEMENTED();};
  virtual void ShowBackgroundPages() {DNOTIMPLEMENTED();};
  virtual void ShowBookmarkBubble(const GURL& url, bool already_bookmarked);
  virtual bool IsDownloadShelfVisible() const {DNOTIMPLEMENTED(); return false;};
  virtual DownloadShelf* GetDownloadShelf() {DNOTIMPLEMENTED(); return NULL;};
  virtual void ShowReportBugDialog() {DNOTIMPLEMENTED();};
  virtual void ShowClearBrowsingDataDialog() {DNOTIMPLEMENTED();};
  virtual void ShowImportDialog() {DNOTIMPLEMENTED();};
  virtual void ShowSearchEnginesDialog() {DNOTIMPLEMENTED();};
  virtual void ShowPasswordManager() {DNOTIMPLEMENTED();};
  virtual void ShowRepostFormWarningDialog(TabContents* tab_contents) {DNOTIMPLEMENTED();};
  virtual void ShowContentSettingsWindow(ContentSettingsType content_type,
                                         Profile* profile) {DNOTIMPLEMENTED();};
  virtual void ShowProfileErrorDialog(int message_id) {DNOTIMPLEMENTED();};
  virtual void ShowThemeInstallBubble() {DNOTIMPLEMENTED();};
  virtual void ConfirmBrowserCloseWithPendingDownloads();
  virtual void ShowHTMLDialog(HtmlDialogUIDelegate* delegate,
                              gfx::NativeWindow parent_window) {DNOTIMPLEMENTED();};
  virtual void UserChangedTheme() {DNOTIMPLEMENTED();};
  virtual int GetExtraRenderViewHeight() const {DNOTIMPLEMENTED(); return 0;};
  virtual void TabContentsFocused(TabContents* tab_contents) {DNOTIMPLEMENTED();};
  virtual void ShowPageInfo(Profile* profile,
                            const GURL& url,
                            const NavigationEntry::SSLStatus& ssl,
                            bool show_history) {DNOTIMPLEMENTED();};
  virtual void ShowPageMenu() {DNOTIMPLEMENTED();};
  virtual void ShowAppMenu() {DNOTIMPLEMENTED();};
  virtual void ShowDownloads();
  virtual void ShowCrashDialog(CrashTabQtModel* model, CrashAppModalDialog* app_modal);
  // No key accelerators
  virtual bool PreHandleKeyboardEvent(const NativeWebKeyboardEvent& event,
                                      bool* is_keyboard_shortcut) {/*DNOTIMPLEMENTED();*/ return false;};
  virtual void HandleKeyboardEvent(const NativeWebKeyboardEvent& event) {/*DNOTIMPLEMENTED();*/};
  virtual void ShowCreateWebAppShortcutsDialog(TabContentsWrapper* tab_contents) {DNOTIMPLEMENTED();};
  virtual void ShowCreateWebAppShortcutsDialog(TabContents*  tab_contents) {DNOTIMPLEMENTED();};
  virtual void ShowCreateChromeAppShortcutsDialog(Profile* profile,
                                                  const Extension* app) {DNOTIMPLEMENTED();};
  virtual void Cut() {DNOTIMPLEMENTED();};
  virtual void Copy() {DNOTIMPLEMENTED();};
  virtual void Paste() {DNOTIMPLEMENTED();};
  virtual void ToggleTabStripMode() {DNOTIMPLEMENTED();};
  virtual void PrepareForInstant();
  virtual void ShowInstant(TabContentsWrapper* preview){DNOTIMPLEMENTED();};
  virtual void HideInstant(bool instant_is_active) {DNOTIMPLEMENTED();};
  virtual gfx::Rect GetInstantBounds(){return gfx::Rect();};

  // Overridden from NotificationObserver:
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

  // Overridden from TabStripModelObserver:
  virtual void TabDetachedAt(TabContentsWrapper* contents, int index); 
  virtual void TabSelectedAt(TabContentsWrapper* old_contents,
                             TabContentsWrapper* new_contents,
                             int index,
                             bool user_gesture) ;
  virtual void TabInsertedAt(TabContentsWrapper* contents,
                             int index,
                             bool foreground);
  virtual void TabReplacedAt( TabStripModel* tab_strip_model,
                             TabContentsWrapper* old_contents,
                             TabContentsWrapper* new_contents,
                             int index);
  virtual void TabStripEmpty() {DNOTIMPLEMENTED();};

  LauncherWindow* window() {return window_;};

  QDeclarativeView* DeclarativeView();

  void ShowContextMenu(ui::MenuModel* model, gfx::Point p);
  void ShowDialog(DialogQtModel* model, DialogQtResultListener* listener);
  // Show or hide the bookmark bar.
  void MaybeShowBookmarkBar(TabContents* contents);
  bool IsBookmarkBarSupported() const;

  FindBarQt* GetFindBar();
  SelectFileDialogQtImpl* GetSelectFileDialog();
  PopupListQt* GetWebPopupList();
  SelectionHandlerQt* GetSelectionHandler();

  SSLDialogQt* GetSSLDialogQt();
  TabContentsContainerQt* GetTabContentsContainer() { return contents_container_.get(); }

  NewTabUIQt* GetNewTabUIQt(); 
 protected:
  bool CanClose();
  virtual void DestroyBrowser();
  void AddAction(const QString& str);
 private:
  void FadeForInstant(bool animate);
  void CancelInstantFade();

  // top level window
#if defined(MTF)
  MWindow* window_;
  MSceneWindow* main_page_;
  MWidget* container;
  MLayout *mainLayout;
  MLinearLayoutPolicy *policy;
#endif

  LauncherWindow* window_;
  
  scoped_ptr<TabContentsContainerQt> contents_container_;
  scoped_ptr<BrowserToolbarQt> toolbar_;
  scoped_ptr<MenuQt> menu_;
  scoped_ptr<DialogQt> dialog_;
  scoped_ptr<SelectFileDialogQtImpl> select_file_dialog_;
  scoped_ptr<FullscreenExitBubbleQt> fullscreen_exit_bubble_;
  scoped_ptr<BookmarkBarQt> bookmark_bar_;
  scoped_ptr<BookmarkOthersQt> bookmark_others_;
  scoped_ptr<BookmarkBubbleQt> bookmark_bubble_;
  scoped_ptr<InfoBarContainerQt> infobar_container_;
  scoped_ptr<Browser> browser_;
  scoped_ptr<NewTabUIQt> new_tab_;
  scoped_ptr<DownloadsQtHandler> download_handler_;
  scoped_ptr<PopupListQt> web_popuplist_;
  scoped_ptr<SSLDialogQt> ssl_dialog_;

  FindBarQt* find_bar_; // It will be automatically freed by find bar controller
  BookmarkListData* bookmarklist_data_; // The bookmark data shared between bookmark_bar_ and bookmark_others_
  scoped_ptr<CrashTabQt> crash_tab_;
  scoped_ptr<SelectionHandlerQt> selection_handler_;

  BrowserWindowQtImpl* impl_;
  NotificationRegistrar registrar_;  
 public:
  void InitWidget();
  void MinimizeWindow(void);

  public:
    void homeClicked();
    void refreshClicked();

  DISALLOW_COPY_AND_ASSIGN(BrowserWindowQt);
};

#endif  // CHROME_BROWSER_QT_BROWSER_WINDOW_QT_H_
