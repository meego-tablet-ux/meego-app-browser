// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <QDir>

#include "chrome/browser/ui/meegotouch/browser_window_qt.h"
#include "chrome/browser/ui/meegotouch/tab_contents_container_qt.h"
#include "chrome/browser/ui/meegotouch/infobars/infobar_container_qt.h"
#include "chrome/browser/ui/meegotouch/fullscreen_exit_bubble_qt.h"
#include "chrome/browser/ui/meegotouch/menu_qt.h"
#include "chrome/browser/ui/meegotouch/bookmark_bubble_qt.h"
#include "chrome/browser/ui/meegotouch/download_in_progress_dialog_qt.h"
#include "chrome/browser/ui/meegotouch/popup_list_qt.h"
#include "chrome/browser/qt/browser-service/BrowserServiceWrapper.h"
#include <string>
#include "base/utf_string_conversions.h"

#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "base/base_paths.h"
#include "base/command_line.h"
//#include "base/keyboard_codes.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "base/scoped_ptr.h"
#include "base/memory/singleton.h"
#include "base/string_util.h"
#include "base/time.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/app_modal_dialogs/app_modal_dialog_queue.h"
#include "chrome/browser/autocomplete/autocomplete_edit_view.h"
#include "chrome/browser/bookmarks/bookmark_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/debugger/devtools_window.h"
#include "chrome/browser/download/download_item_model.h"
#include "chrome/browser/download/download_manager.h"
#include "chrome/browser/ui/find_bar/find_bar_controller.h"
#include "chrome/browser/ui/omnibox/location_bar.h"
#include "chrome/browser/ui/tab_contents/tab_contents_wrapper.h"
#include "chrome/browser/page_info_window.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "content/browser/renderer_host/render_view_host.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/browser/tab_contents/tab_contents_view.h"
#include "content/browser/plugin_service.h"
#include "chrome/browser/ui/window_sizer.h"
#include "chrome/browser/net/url_fixer_upper.h"
#include "chrome/common/chrome_switches.h"
#include "content/common/notification_service.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/rect.h"
#include "grit/app_resources.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"

#include <QApplication>
#include <QDBusConnection>
#include <QDBusError>
#include <QDebug>
#include <QDeclarativeEngine>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDeclarativeNetworkAccessManagerFactory>
#include <QDesktopWidget>
#include <QFile>
#include <QFileInfo>
#include <QGLFormat>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QResizeEvent>
#include <QTimer>
#include <QTextStream>
#include <QTranslator>
#include <QVariant>
#include <QX11Info>

#include <QtCore/QMetaObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

#include <QDeclarativeContext>
#include <QDeclarativeView>

#if !defined(BUILD_QML_PLUGIN)
#include <launcherwindow.h>
#include <launcherapp.h>
#else
#include "chrome/browser/browser_object_qt.h"
#include <QGenericReturnArgument>
#endif

#include <QOrientationSensor>
#include <QOrientationFilter>
#include <QOrientationReading>
#include <QPluginLoader>
#include <QInputContext>

QTM_USE_NAMESPACE

// Copied from libmeegotouch, which we don't link against.  We need it
// defined so we can connect a signal to the MInputContext object
// (loaded from a plugin) that uses this type.
namespace M {
    enum OrientationAngle { Angle0=0, Angle90=90, Angle180=180, Angle270=270 };
}

class BrowserWindowQtImpl : public QObject
{
  Q_OBJECT;
 public:
  BrowserWindowQtImpl(BrowserWindowQt* window):
      QObject(),
      window_(window),
      browserWindowShow_(false)
  {
  }
  void HideAllPanel()
  {
     emit hideAllPanel();
  }
  void ShowBookmarks(bool is_show)
  {
     if(is_show)
       window_->bookmark_bar_->ShowBookmarkManager();
     emit showBookmarks(is_show);
  }
  void ShowDownloads(bool is_show)
  {
     emit showDownloads(is_show);
  }

 Q_SIGNALS:
   void hideAllPanel();
   void showBookmarks(bool is_show);
   void showDownloads(bool is_show);
   void browserWindowShow();

 public Q_SLOTS:
#if !defined(BUILD_QML_PLUGIN)
  void onCalled(const QStringList& parameters)
  {
    for (int i = 0 ; i < parameters.size(); i++)
    {
      if(parameters[i] == "restore")
          continue;
      DLOG(INFO) << "BrowserWindowQtImpl::onCalled " << parameters[i].toStdString();
      window_->browser_->OpenURL(URLFixerUpper::FixupURL(parameters[i].toStdString(), std::string()),
                                 GURL(), NEW_FOREGROUND_TAB, PageTransition::LINK);
    }
  }
#else
 void onCalled(const QStringList& parameters)
 {
   for (int i = 0; i < parameters.size(); i++)
   {
     // Only care about URL parameter which is not started with '--'
     if(parameters[i].startsWith("--")) continue;
      window_->browser_->OpenURL(URLFixerUpper::FixupURL(parameters[i].toStdString(), std::string()),
                                 GURL(), NEW_FOREGROUND_TAB, PageTransition::LINK);
   }
 }
#endif

  void OrientationStart()
  {
    window_->GetTabContentsContainer()->OrientationStart();
  }

  void OrientationEnd(int orientation)
  {
#if !defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        NewRunnableFunction(PluginService::OnOrientationChanged,
                            orientation));
#endif
    window_->GetTabContentsContainer()->OrientationEnd();
  }

  /*Get LauncherApp 's foreground WinId change signal*/
  void handleForegroundWindowChange()
  {
#if !defined(BUILD_QML_PLUGIN)
    LauncherApp *app = static_cast<LauncherApp *>(qApp);
#else
    QApplication* app = g_browser_object->getApplication();
#endif
    int app_win_id = app->property("foregroundWindow").toInt();
    
    QDeclarativeView* view = window_->window();
    int view_win_id = view->property("winId").toInt();

    /*IPC to render tab for media player control*/
    if(app_win_id != view_win_id){
      window_->OnForegroundChanged();
    }
  }

 protected:
  bool eventFilter(QObject *obj, QEvent *event)
  {
    if (event->type() == QEvent::Close) {
      window_->browser_->ExecuteCommandWithDisposition(IDC_CLOSE_WINDOW, CURRENT_TAB);
    } else if (!browserWindowShow_ && event->type() == QEvent::UpdateRequest) {
      browserWindowShow_ = true;
      emit browserWindowShow();
    }
    if(event->type() == QEvent::WindowStateChange && 
        window_->window()->windowState() == Qt::WindowMinimized) {
      NotificationService::current()->Notify(
        NotificationType::BROWSER_WINDOW_MINIMIZED,
        Source<BrowserWindow>(window_),
        NotificationService::NoDetails());
    }
    return QObject::eventFilter(obj, event);
  }

 private:
  BrowserWindowQt* window_;
  bool browserWindowShow_;
};

class OrientationSensorFilter : public QOrientationFilter
{
    bool filter(QOrientationReading *reading)
    {
        int qmlOrient;
        M::OrientationAngle qtOrient;
        switch (reading->orientation())
        {
        case QOrientationReading::LeftUp:
            qtOrient = M::Angle270;
            qmlOrient = 2;
            break;
        case QOrientationReading::TopDown:
            qtOrient = M::Angle180;
            qmlOrient = 3;
            break;
        case QOrientationReading::RightUp:
            qtOrient = M::Angle90;
            qmlOrient = 0;
            break;
        default: // assume QOrientationReading::TopUp
            qtOrient = M::Angle0;
            qmlOrient = 1;
            break;
        }

        for(int i = 0; i < listeners_.size(); i++) {
          listeners_[i]->OrientationStart();
        }
        qApp->setProperty("orientation", QVariant(qmlOrient));
        // Need to tell the MInputContext plugin to rotate the VKB too
        QMetaObject::invokeMethod(qApp->inputContext(),
                                  "notifyOrientationChange",
                                  Q_ARG(M::OrientationAngle, qtOrient));
        return false;
    }
public:
    void addListener(BrowserWindowQtImpl *listener)
    {
      listeners_.append(listener);
    }

    void removeListener(BrowserWindowQtImpl * listener)
    {
      int index = listeners_.indexOf(listener);
      if (index >= 0) {
        listeners_.remove(index);
      }
    }
private:
    QVector<BrowserWindowQtImpl *> listeners_;
};

static OrientationSensorFilter *filter = NULL;

BrowserWindowQt::BrowserWindowQt(Browser* browser, QWidget* parent):
  browser_(browser)
{
  impl_ = new BrowserWindowQtImpl(this);
  InitWidget();
  registrar_.Add(this, NotificationType::BOOKMARK_BAR_VISIBILITY_PREF_CHANGED,
                 NotificationService::AllSources());
  browser_->tabstrip_model()->AddObserver(this);
}

BrowserWindowQt::~BrowserWindowQt()
{

  //delete main_page_;
  //delete container;
  if (filter) {
    filter->removeListener(impl_);
  }

  delete impl_;
  browser_->tabstrip_model()->RemoveObserver(this);

  delete bookmarklist_data_;
}

QDeclarativeView* BrowserWindowQt::DeclarativeView()
{
  return window_;
}

void BrowserWindowQt::InitWidget()
{
  QDeclarativeContext* context;
  QApplication* app;

#if !defined(BUILD_QML_PLUGIN)
  extern LauncherWindow* g_main_window;
  app = qApp;
  window_ = g_main_window;
  context = window_->rootContext();

  // Set modal as NULL to avoid QML warnings
  bool fullscreen = false;
  context->setContextProperty("is_fullscreen", fullscreen);
  context->setContextProperty("browserWindow", impl_);

  impl_->connect(window_, SIGNAL(call(const QStringList&)),
          impl_, SLOT(onCalled(const QStringList&)));

  //hardcode appname for multiprocess
  QString mainQml = "meego-app-browser/exemain.qml";
  QString sharePath;
  if (QFile::exists(mainQml))
  {
    sharePath = QDir::currentPath() + "/";
  }
  else
  {
    sharePath = QString("/usr/share/");
    if (!QFile::exists(sharePath + mainQml))
    {
      qFatal("%s does not exist!", mainQml.toUtf8().data());
    }
  }

#else
  DCHECK(g_browser_object);
  app = g_browser_object->getApplication();
  window_ = g_browser_object->getDeclarativeView();
  context = window_->rootContext();
  context->setContextProperty("browserWindow", impl_);
  impl_->connect(g_browser_object, SIGNAL(call(const QStringList&)),
          impl_, SLOT(onCalled(const QStringList&)));
#endif
  
  impl_->connect(app, SIGNAL(foregroundWindowChanged()),
                 impl_, SLOT(handleForegroundWindowChange()));

  window_->installEventFilter(impl_);
  
  // Expose the DPI to QML
  context->setContextProperty("dpiX", app->desktop()->logicalDpiX());
  context->setContextProperty("dpiY", app->desktop()->logicalDpiY());
  
  contents_container_.reset(new TabContentsContainerQt(this));
  toolbar_.reset(new BrowserToolbarQt(browser_.get(), this));
  menu_.reset(new MenuQt(this));
  dialog_.reset(new DialogQt(this));
  select_file_dialog_.reset(new SelectFileDialogQtImpl(this));
  fullscreen_exit_bubble_.reset(new FullscreenExitBubbleQt(this, false));
  bookmarklist_data_ = new BookmarkListData();
  bookmark_bar_.reset(new BookmarkBarQt(this, browser_->profile(), browser_.get(), bookmarklist_data_));
  bookmark_others_.reset(new BookmarkOthersQt(this, browser_->profile(), browser_.get(), bookmarklist_data_));
  infobar_container_.reset(new InfoBarContainerQt(browser_->profile(), this));
  find_bar_ = new FindBarQt(browser_.get(), this);
  ssl_dialog_.reset(new SSLDialogQt(this));
  new_tab_.reset(new NewTabUIQt(browser_.get(), this));
  bookmark_bubble_.reset(new BookmarkBubbleQt(this, browser_.get(), browser_->profile()));
  web_popuplist_.reset(new PopupListQt(this));
  crash_tab_.reset(new CrashTabQt(this));
  selection_handler_.reset(new SelectionHandlerQt(this));
  
  DownloadManager* dlm = browser_->profile()->GetDownloadManager();
  download_handler_.reset(new DownloadsQtHandler(this, browser_.get(), dlm));

#if !defined(BUILD_QML_PLUGIN)
  window_->setSource(QUrl(sharePath + mainQml));
#endif
  toolbar_->enableEvents();

  // any item object binding code should be after set source
  contents_container_->Init();
  toolbar_->Init(browser_->profile());
  bookmark_others_->Init(browser_->profile());
  bookmark_bar_->Init(browser_->profile(), bookmark_others_.get());
  window_->show();
  download_handler_->Init();
  //QGestureRecognizer::unregisterRecognizer(Qt::PanGesture);
  //QGestureRecognizer::registerRecognizer(new MPanRecognizer());

  // start the orientation sensor, used by QML window and rwhv
  static QOrientationSensor *sensor = NULL;
  if (sensor == NULL) {
    sensor = new QOrientationSensor;
    filter = new OrientationSensorFilter;
    sensor->addFilter(filter);
    sensor->start();
  }
  if (filter) {
    filter->addListener(impl_);
  }

  //Init TopSitesCache
  browser_->profile()->GetTopSites();

  if(browser_->type()==Browser::TYPE_APP)
    return;

  BrowserServiceWrapper* service = BrowserServiceWrapper::GetInstance();
  service->Init(browser_.get());
}

void BrowserWindowQt::Observe(NotificationType type,
                               const NotificationSource& source,
                               const NotificationDetails& details) {
  if (type == NotificationType::BOOKMARK_BAR_VISIBILITY_PREF_CHANGED) {
      MaybeShowBookmarkBar(browser_->GetSelectedTabContents());
  }
}

bool BrowserWindowQt::IsBookmarkBarSupported() const {
  return browser_->SupportsWindowFeature(Browser::FEATURE_BOOKMARKBAR);
}

void BrowserWindowQt::MaybeShowBookmarkBar(TabContents* contents) {

  bool show_bar;
  if (contents) {
    PrefService* prefs = contents->profile()->GetPrefs();
    show_bar = prefs->GetBoolean(prefs::kShowBookmarkBar);
    if (IsBookmarkBarSupported()) {
      bookmark_bar_->NotifyToMayShowBookmarkBar(show_bar);
    }
  }
}


void BrowserWindowQt::ComposeEmbededFlashWindow(const gfx::Rect &r) {
#ifndef MEEGO_FORCE_FULLSCREEN_PLUGIN
  TabContents* contents = contents_container_->GetTabContents();
  if (contents) {
    RenderWidgetHostView* rwhv = contents->GetRenderWidgetHostView();
    if (rwhv)
      rwhv->ComposeEmbededFlashWindow(r);
  }
#endif
}

void BrowserWindowQt::ReshowEmbededFlashWindow() {
#ifndef MEEGO_FORCE_FULLSCREEN_PLUGIN
  if (GetTabContentsContainer()->in_orientation())
    return;
  TabContents* contents = contents_container_->GetTabContents();
  if (contents) {
    RenderWidgetHostView* rwhv = contents->GetRenderWidgetHostView();
    if (rwhv)
      rwhv->ReShowEmbededFlashWindow();
  }
#endif
}

void BrowserWindowQt::ShowContextMenu(ui::MenuModel* model, gfx::Point p)
{
  menu_->SetModel(model);
  menu_->PopupAt(p);
}

void BrowserWindowQt::ShowDialog(DialogQtModel* model, DialogQtResultListener* listener)
{
  dialog_->SetModelAndListener(model, listener);
  dialog_->Popup();

  // TODO: compose embeded flash window with correct rect
  gfx::Rect rect(0, 0, 0, 0);
  ComposeEmbededFlashWindow(rect);
}

void BrowserWindowQt::Show()
{
  BrowserList::SetLastActive(browser_.get());
  window_->show();
  window_->raise();
}

void BrowserWindowQt::ShowInactive()
{
  window_->show();
  window_->raise();
}

void BrowserWindowQt::OnForegroundChanged()
{
  if (browser_->GetSelectedTabContents()) {
    TabContents* contents = contents_container_->GetTabContents();
    if (contents) {
      contents->render_view_host()->BackgroundPolicy();
    }
  }
  
  return;
}

void BrowserWindowQt::Close()
{
  if (!CanClose())
    return;

  // Browser::SaveWindowPlacement is used for session restore.
  if (browser_->ShouldSaveWindowPlacement())
    browser_->SaveWindowPlacement(GetRestoredBounds(), IsMaximized());

  window_->close();
  
  MessageLoop::current()->PostTask(FROM_HERE,
                                   new DeleteTask<BrowserWindowQt>(this));

}

void BrowserWindowQt::UpdateReloadStopState(bool is_loading, bool force)
{
  toolbar_->UpdateReloadStopState(is_loading, force);
}

void BrowserWindowQt::UpdateTitleBar() {
  string16 title = browser_->GetWindowTitleForCurrentTab();
  //  main_page_->setTitle(QString::fromUtf8(UTF16ToUTF8(title).c_str()));
  // No Titlebar in QT chromium
  if (browser_->GetSelectedTabContents())
    toolbar_->UpdateTitle();
  return;
}

void BrowserWindowQt::MinimizeWindow()
{
  DCHECK(QMetaObject::invokeMethod(window_, "goHome", Qt::DirectConnection));
}

void BrowserWindowQt::TabDetachedAt(TabContentsWrapper* contents, int index) {
  // We use index here rather than comparing |contents| because by this time
  // the model has already removed |contents| from its list, so
  // browser_->GetSelectedTabContents() will return NULL or something else.
  if (index == browser_->tabstrip_model()->active_index())
      infobar_container_->ChangeTabContents(NULL);
  //  contents_container_->DetachTabContents(contents);
  //  UpdateDevToolsForContents(NULL);
  if (contents->tab_contents()->GetURL() == GURL(chrome::kChromeUIDownloadsURL)) {
    impl_->ShowDownloads(false);
  } else if (contents->tab_contents()->GetURL() == GURL(chrome::kChromeUIBookmarksURL)) {
    impl_->ShowBookmarks(false);
  }

}

void BrowserWindowQt::TabSelectedAt(TabContentsWrapper* old_contents,
                                     TabContentsWrapper* new_contents,
                                     int index,
                                     bool user_gesture) {
  //  DCHECK(old_contents != new_contents);
  //
  //  if (old_contents && !old_contents->is_being_destroyed())
  //    old_contents->view()->StoreFocus();
  //
  // Update various elements that are interested in knowing the current
  // TabContents.

  infobar_container_->ChangeTabContents(new_contents->tab_contents());
  // UpdateDevToolsForContents(new_contents);

  // TODO(estade): after we manage browser activation, add a check to make sure
  // we are the active browser before calling RestoreFocus().
  //  if (!browser_->tabstrip_model()->closing_all()) {
  //    new_contents->view()->RestoreFocus();
  //    if (new_contents->find_ui_active())
  //      browser_->GetFindBarController()->find_bar()->SetFocusAndSelection();
  //  }
  //
  //  // Update all the UI bits.
  UpdateTitleBar();
  //  UpdateUIForContents(new_contents->tab_contents());

  if(old_contents) 
    old_contents->tab_contents()->WasHidden();

  new_contents->tab_contents()->DidBecomeSelected();

    
  UpdateToolbar(new_contents, true);
  contents_container_->SetTabContents(new_contents->tab_contents());

  if (new_contents->tab_contents()->GetURL() == GURL(chrome::kChromeUIDownloadsURL)) {
    impl_->ShowDownloads(true);
  } else if (new_contents->tab_contents()->GetURL() == GURL(chrome::kChromeUIBookmarksURL)) {
    impl_->ShowBookmarks(true);
  }else {
    impl_->ShowDownloads(false);
    impl_->ShowBookmarks(false);
  }

}

void BrowserWindowQt::TabReplacedAt( TabStripModel* tab_strip_model,
                             TabContentsWrapper* old_contents,
                             TabContentsWrapper* new_contents,
                             int index)
{
  if (new_contents->tab_contents()->GetURL() != GURL(chrome::kChromeUIDownloadsURL)) {
    impl_->ShowDownloads(false);
  } 
  if (new_contents->tab_contents()->GetURL() != GURL(chrome::kChromeUIBookmarksURL)) {
    impl_->ShowBookmarks(false);
  }
}
void BrowserWindowQt::TabInsertedAt(TabContentsWrapper* contents,
                             int index,
                             bool foreground){
  impl_->HideAllPanel();
}

LocationBar* BrowserWindowQt::GetLocationBar() const
{
  return toolbar_->GetLocationBar();
}

void BrowserWindowQt::UpdateToolbar(TabContentsWrapper* contents, 
                                     bool should_restore_state) {
  toolbar_->UpdateTabContents(contents->tab_contents(), should_restore_state);
}

gfx::Rect BrowserWindowQt::GetRestoredBounds() const
{
  QRect rect = window_->geometry();
  gfx::Rect out;
  out.SetRect(int(rect.x()), int(rect.y()),
	      int(rect.width()), int(rect.height()));
  return out;
};

void BrowserWindowQt::SetFullscreen(bool fullscreen) {
  fullscreen_exit_bubble_->SetFullscreen(fullscreen);
}

bool BrowserWindowQt::IsFullscreen() const{
  return fullscreen_exit_bubble_->IsFullscreen();
}

void BrowserWindowQt::DestroyBrowser()
{
  browser_.reset();
}

bool BrowserWindowQt::CanClose(){
  // Give beforeunload handlers the chance to cancel the close before we hide
  // the window below.
  if (!browser_->ShouldCloseWindow())
    return false;

  if (!browser_->tabstrip_model()->empty()) {
    // Tab strip isn't empty.  Hide the window (so it appears to have closed
    // immediately) and close all the tabs, allowing the renderers to shut
    // down. When the tab strip is empty we'll be called back again.
    browser_->OnWindowClosing();
    return false;
  }

  return true;
}

void BrowserWindowQt::ConfirmBrowserCloseWithPendingDownloads()
{
  DownloadInProgressDialogQt* confirmDialog = new DownloadInProgressDialogQt(browser_.get());
  confirmDialog->show();
  //browser_->InProgressDownloadResponse(true);
}

void BrowserWindowQt::SetStarredState(bool is_starred) {
  toolbar_->SetStarred(is_starred);
}

void BrowserWindowQt::PrepareForInstant() {
  TabContents* contents = contents_container_->GetTabContents();
  if (contents)
    FadeForInstant(true);
}

void BrowserWindowQt::ShowBookmarkBubble(const GURL& url, bool already_bookmarked)
{
  bookmark_bubble_.reset(new BookmarkBubbleQt(this, browser_.get(), browser_->profile(), url, already_bookmarked));  
  gfx::Point p(-1, -1);
  bookmark_bubble_->PopupAt(p);
}

void BrowserWindowQt::ShowDownloads()
{
  download_handler_->Show();
}

void BrowserWindowQt::ShowCrashDialog(CrashTabQtModel* model, CrashAppModalDialog* app_modal){
  crash_tab_->SetModelAndAppModal(model, app_modal);
  crash_tab_->Popup();
}

FindBarQt* BrowserWindowQt::GetFindBar()
{
  return find_bar_;
}

void BrowserWindowQt::ShowSSLDialogQt(SSLAppModalDialog* model)
{
  ssl_dialog_.get()->SetModel(model);
  ssl_dialog_.get()->Show();
}

NewTabUIQt* BrowserWindowQt::GetNewTabUIQt()
{
  return new_tab_.get(); 
}

SelectFileDialogQtImpl* BrowserWindowQt::GetSelectFileDialog()
{
  return select_file_dialog_.get();
}

SelectionHandlerQt* BrowserWindowQt::GetSelectionHandler()
{
    return selection_handler_.get();
}


PopupListQt* BrowserWindowQt::GetWebPopupList()
{
    return web_popuplist_.get();
}

void BrowserWindowQt::FadeForInstant(bool animate) {
  DCHECK(contents_container_->GetTabContents());
  RenderWidgetHostView* rwhv =
      contents_container_->GetTabContents()->GetRenderWidgetHostView();
  if (rwhv) {
    SkColor whitish = SkColorSetARGB(192, 255, 255, 255);
    rwhv->SetVisuallyDeemphasized(&whitish, animate);
  }
}

void BrowserWindowQt::CancelInstantFade() {
  DCHECK(contents_container_->GetTabContents());
  RenderWidgetHostView* rwhv =
      contents_container_->GetTabContents()->GetRenderWidgetHostView();
  if (rwhv)
    rwhv->SetVisuallyDeemphasized(NULL, false);
}

void BrowserWindowQt::InhibitScreenSaver(bool inhibit) {
#if defined(BUILD_QML_PLUGIN)
  QObject* window = g_browser_object->getDeclarativeView();
  QMetaObject::invokeMethod(window, "setInhibitScreenSaver", Qt::DirectConnection,
      QGenericReturnArgument(), Q_ARG(bool, inhibit)); 
#else
  extern LauncherWindow* g_main_window;
  DLOG(INFO) << "Inhibit screen saver " << inhibit;

  g_main_window->setInhibitScreenSaver(inhibit);
#endif
}

#include "moc_browser_window_qt.cc"
