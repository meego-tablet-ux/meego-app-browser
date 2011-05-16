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
#include "ui/base/theme_provider.h"
#include "base/base_paths.h"
#include "base/command_line.h"
//#include "base/keyboard_codes.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "base/scoped_ptr.h"
#include "base/singleton.h"
#include "base/string_util.h"
#include "base/time.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/app_modal_dialogs/app_modal_dialog_queue.h"
#include "chrome/browser/autocomplete/autocomplete_edit_view.h"
#include "chrome/browser/bookmarks/bookmark_utils.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/themes/browser_theme_provider.h"
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
#include "chrome/browser/ui/window_sizer.h"
#include "chrome/browser/net/url_fixer_upper.h"
#include "chrome/common/chrome_switches.h"
#include "content/common/notification_service.h"
#include "chrome/common/pref_names.h"
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
#include <launcherwindow.h>
#include <launcherapp.h>

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

        ((LauncherApp*)qApp)->setOrientation(qmlOrient);

        // Need to tell the MInputContext plugin to rotate the VKB too
        QMetaObject::invokeMethod(qApp->inputContext(),
                                  "notifyOrientationChange",
                                  Q_ARG(M::OrientationAngle, qtOrient));
        return false;
    }
};

class BrowserWindowQtImpl : public QObject
{
  Q_OBJECT;
 public:
  BrowserWindowQtImpl(BrowserWindowQt* window):
      QObject(),
      window_(window)
  {
  }

 public Q_SLOTS:
  void onCalled(const QStringList& parameters)
  {
    for (int i = 0 ; i < parameters.size(); i++)
    {
      DLOG(INFO) << "BrowserWindowQtImpl::onCalled " << parameters[i].toStdString();
      window_->browser_->OpenURL(URLFixerUpper::FixupURL(parameters[i].toStdString(), std::string()),
                                 GURL(), NEW_FOREGROUND_TAB, PageTransition::LINK);
    }
  }


 protected:
  bool eventFilter(QObject *obj, QEvent *event)
  {
    if (event->type() == QEvent::Close) {
      window_->browser_->ExecuteCommandWithDisposition(IDC_CLOSE_WINDOW, CURRENT_TAB);
    }
    return QObject::eventFilter(obj, event);
  }

 private:
  BrowserWindowQt* window_;
};

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

  delete impl_;
  browser_->tabstrip_model()->RemoveObserver(this);
}

QDeclarativeView* BrowserWindowQt::DeclarativeView()
{
  return window_->getDeclarativeView();
}

void BrowserWindowQt::InitWidget()
{
  extern LauncherWindow* g_main_window;

  window_ = g_main_window;
  window_->installEventFilter(impl_);
  bool result = impl_->connect(window_, SIGNAL(call(const QStringList&)),
                               impl_, SLOT(onCalled(const QStringList&)));

  QDeclarativeContext *context = window_->getDeclarativeView()->rootContext();

  // Set modal as NULL to avoid QML warnings
  bool fullscreen = false;
  context->setContextProperty("is_fullscreen", fullscreen);

  LauncherApp *app = static_cast<LauncherApp *>(qApp);
  QString mainQml = app->applicationName() + "/main.qml";
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

  // Expose the DPI to QML
  context->setContextProperty("dpiX", app->desktop()->logicalDpiX());
  context->setContextProperty("dpiY", app->desktop()->logicalDpiY());
  
  contents_container_.reset(new TabContentsContainerQt(this));
  toolbar_.reset(new BrowserToolbarQt(browser_.get(), this));
  menu_.reset(new MenuQt(this));
  dialog_.reset(new DialogQt(this));
  select_file_dialog_.reset(new SelectFileDialogQtImpl(this));
  fullscreen_exit_bubble_.reset(new FullscreenExitBubbleQt(this, false));
  bookmark_bar_.reset(new BookmarkBarQt(this, browser_->profile(), browser_.get()));
  bookmark_others_.reset(new BookmarkOthersQt(this, browser_->profile(), browser_.get()));
  infobar_container_.reset(new InfoBarContainerQt(browser_->profile(), this));
  find_bar_.reset(new FindBarQt(browser_.get(), this));
  new_tab_.reset(new NewTabUIQt(browser_.get(), this));
  bookmark_bubble_.reset(new BookmarkBubbleQt(this, browser_.get(), browser_->profile()));
  web_popuplist_.reset(new PopupListQt(this));
  
  DownloadManager* dlm = browser_->profile()->GetDownloadManager();
  download_handler_.reset(new DownloadsQtHandler(this, browser_.get(), dlm));

  // set the srouce at the end to make sure every model is ready now
  window_->getDeclarativeView()->setSource(QUrl(sharePath + mainQml));

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
    static OrientationSensorFilter *filter = new OrientationSensorFilter;
    sensor->addFilter(filter);
    sensor->start();
  }

  //Init TopSitesCache
  browser_->profile()->GetTopSites();

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

void BrowserWindowQt::ShowContextMenu(ui::MenuModel* model, gfx::Point p)
{
  menu_->SetModel(model);
  menu_->PopupAt(p);
}

void BrowserWindowQt::ShowDialog(DialogQtModel* model, DialogQtResultListener* listener)
{
  dialog_->SetModelAndListener(model, listener);
  dialog_->Popup();
}

void BrowserWindowQt::Show()
{
  BrowserList::SetLastActive(browser_.get());
  window_->show();
  window_->raise();
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
  window_->goHome();
}

void BrowserWindowQt::TabDetachedAt(TabContentsWrapper* contents, int index) {
  // We use index here rather than comparing |contents| because by this time
  // the model has already removed |contents| from its list, so
  // browser_->GetSelectedTabContents() will return NULL or something else.
  if (index == browser_->tabstrip_model()->selected_index())
      infobar_container_->ChangeTabContents(NULL);
  //  contents_container_->DetachTabContents(contents);
  //  UpdateDevToolsForContents(NULL);
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

  contents_container_->SetTabContents(new_contents->tab_contents());
    
  UpdateToolbar(new_contents, true);
  contents_container_->SetTabContents(new_contents->tab_contents());
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
    contents->FadeForInstant(true);
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

FindBarQt* BrowserWindowQt::GetFindBar()
{
  return find_bar_.get();
}

SelectFileDialogQtImpl* BrowserWindowQt::GetSelectFileDialog()
{
  return select_file_dialog_.get();
}

PopupListQt* BrowserWindowQt::GetWebPopupList()
{
    return web_popuplist_.get();
}

#include "moc_browser_window_qt.cc"
