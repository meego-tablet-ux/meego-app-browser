// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/plugins/npapi/qt_plugin_container_manager.h"

#include <QtGui/QApplication>
#include <QtGui/QCursor>
#include <QtGui/QInputContext>
#include <QtGui/QGraphicsView>
#include <QtGui/QGraphicsWidget>
#include <QX11EmbedContainer>
#include <QGraphicsProxyWidget>

#if defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
#include <QPushButton>
#include <QPalette>
#include "ui/base/l10n/l10n_util.h"
#include "../chrome/grit/generated_resources.h"
#endif

#include "base/logging.h"
#include "webkit/plugins/npapi/webplugin.h"

namespace webkit {
namespace npapi {

#define USE_TOP_CLIP_WINDOW 0

void QtPluginContainer::showEvent(QShowEvent *event) {
  if (!embedded_) {
    embedClient(id_);
    embedded_ = true;
  }

  QX11EmbedContainer::showEvent(event);
}

void QtPluginContainer::hideEvent(QHideEvent *event) {
  QX11EmbedContainer::hideEvent(event);
}

WindowedPluginWidgets::~WindowedPluginWidgets()
{
  delete top_window;
}

#if defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
gfx::PluginWindowHandle QtPluginContainerManager::MapCloseBtnToID(QPushButton* button)
{
  for (PluginWindowToWidgetsMap::const_iterator i = plugin_window_to_widgets_map_.begin();
       i != plugin_window_to_widgets_map_.end(); ++i) {
    WindowedPluginWidgets* plugin_widgets = i->second;
    if (plugin_widgets->close_btn == button)
      return i->first;
  }

  return 0;
}
#endif

void QtPluginContainerManager::CloseFSPluginWindow()
{
#if defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
  QPushButton *button = qobject_cast<QPushButton *>(sender());
  gfx::PluginWindowHandle id = MapCloseBtnToID(button);

  if (host_delegate_)
    host_delegate_->OnCloseFSPluginWindow(id);
#endif
}

#if !defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
QWidget* QtPluginContainerManager::GetTopClipWindow()
{
#if USE_TOP_CLIP_WINDOW
  return NULL;
#else
  DCHECK(host_widget_);

  if (!top_clip_window_) {
    top_clip_window_ = new QWidget(host_widget_);
    top_clip_window_->setGeometry(clip_window_rect_);
    top_clip_window_->setAttribute(Qt::WA_NativeWindow, true);
    top_clip_window_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
  }

  return top_clip_window_;
#endif
}

void QtPluginContainerManager::SetClipRect(QRect rect)
{
  clip_window_rect_ = rect;

#if USE_TOP_CLIP_WINDOW
  if (top_clip_window_)
    top_clip_window_->setGeometry(clip_window_rect_);
#endif
}

void QtPluginContainerManager::SetScaleFactor(double factor)
{
  if (factor == scale_factor_)
    return;
  scale_factor_ = factor;

  // offset is actually not needed for the current code , so just send 0 to it.
  gfx::Point offset = gfx::Point(0, 0);
  RelocatePluginContainers(offset);
}

#endif

QtPluginContainerManager::QtPluginContainerManager(QtPluginContainerManagerHostDelegate *host)
    : QObject(), host_widget_(NULL), native_view_(NULL), host_delegate_(host) {
  fs_win_size_.SetSize(0, 0);
  is_hidden_ = false;
#if !defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
  clip_window_rect_.setRect(0, 0, 0, 0);
  top_clip_window_ = NULL;
  scale_factor_ = 1.0;
#endif
}

QtPluginContainerManager::~QtPluginContainerManager()
{
#if !defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
  if (top_clip_window_)
    delete top_clip_window_;
#endif
}

QWidget* QtPluginContainerManager::CreatePluginContainer(
    gfx::PluginWindowHandle id) {
  DLOG(INFO) << "PluginWindowHandle " << id;

  DCHECK(host_widget_);

  WindowedPluginWidgets *plugin_widgets = new WindowedPluginWidgets();

  QWidget *top_window = NULL;

#if !defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)

#if USE_TOP_CLIP_WINDOW
  QWidget *clip_window = GetTopClipWindow();
  if (clip_window) {
    clip_window->show();
    top_window = new QWidget(clip_window);
  } else
    top_window = new QWidget(host_widget_);

#else
  top_window = new QWidget(host_widget_);
#endif //USE_TOP_CLIP_WINDOW

#else
  top_window = new QWidget(host_widget_);
#endif

  top_window->setAttribute(Qt::WA_NativeWindow, true);
  plugin_widgets->top_window = top_window;

#if defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
  top_window->setGeometry(0, 0, fs_win_size_.width(), fs_win_size_.height());

  QPushButton *button = new QPushButton(QString::fromUtf8(l10n_util::GetStringUTF8(IDS_CLOSE).c_str()), top_window);

  plugin_widgets->close_btn = button;
  button->setGeometry(0, fs_win_size_.height() - FSPluginCloseBarHeight(), fs_win_size_.width(), FSPluginCloseBarHeight());
  connect(button, SIGNAL(clicked()), this, SLOT(CloseFSPluginWindow()));

  QPalette pal = button->palette( );
  pal.setColor( QPalette::Button, Qt::black );
  pal.setColor( QPalette::ButtonText, Qt::white );
  button->setPalette(pal);
  button->setFlat(true);
  button->setAutoFillBackground(true);
#endif

  QX11EmbedContainer *container = new QtPluginContainer(id, top_window);

  plugin_widgets->window = container;
  plugin_widgets->window->show();

  plugin_window_to_widgets_map_.insert(std::make_pair(id, plugin_widgets));
  
  WebPluginGeometry *geo = new struct WebPluginGeometry();
  plugin_window_to_geometry_map_.insert(std::make_pair(id, geo));

  return NULL;
}

void QtPluginContainerManager::DestroyPluginContainer(
    gfx::PluginWindowHandle id) {
  DCHECK(host_widget_);

  plugin_window_to_geometry_map_.erase(id);

// hmm, the erase(id) operation don't call WindowedPluginWidgets's destructor, strange
  PluginWindowToWidgetsMap::const_iterator iter = plugin_window_to_widgets_map_.find(id);
  if (iter != plugin_window_to_widgets_map_.end()) {
    WindowedPluginWidgets* plugin_widgets = iter->second;
    delete plugin_widgets->top_window;
    plugin_widgets->top_window = NULL; // in case Destructor some how been called...
  }
  plugin_window_to_widgets_map_.erase(id);

}

void QtPluginContainerManager::Show()
{
  for (PluginWindowToWidgetsMap::const_iterator i =
           plugin_window_to_widgets_map_.begin();
       i != plugin_window_to_widgets_map_.end(); ++i) {
    i->second->window->show();
    i->second->top_window->show();
  }

#if !defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
#if USE_TOP_CLIP_WINDOW
    if (top_clip_window_)
      top_clip_window_->show();
#endif
#endif

  is_hidden_ = false;
}

void QtPluginContainerManager::Hide()
{

#if !defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
#if USE_TOP_CLIP_WINDOW
    if (top_clip_window_)
      top_clip_window_->hide();
#endif
#endif

  for (PluginWindowToWidgetsMap::const_iterator i =
           plugin_window_to_widgets_map_.begin();
       i != plugin_window_to_widgets_map_.end(); ++i) {
    i->second->window->hide();
    i->second->top_window->hide();
  }

  is_hidden_ = true;
}

void QtPluginContainerManager::ComposeEmbededFlashWindow(const gfx::Rect& rect) {
  if (!is_hidden_) {
    // TODO: compose the covered flash rect with given rect
    DLOG(INFO) << "Compose Embeded flash window";
    Hide();
  }
}

void QtPluginContainerManager::ReShowEmbededFlashWindow() {
  if (is_hidden_) {
    DLOG(INFO) << "Reshow Embeded flash window";
    Show();
  }
}

void QtPluginContainerManager::MovePluginContainer(
    WindowedPluginWidgets *widgets, const WebPluginGeometry& move, gfx::Point& view_offset) {

// It seems that we do not need view_offset anymore. But we might change our implementation a lot recently,
// so reserve it for now.

  DCHECK(host_widget_);
  if (!widgets)
    return;

  if (!move.visible) {
    widgets->top_window->hide();
    return;
  }

  if (!move.rects_valid)
    return;

  int current_x, current_y;

#if defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
  widgets->window->setGeometry(move.window_rect.x(), move.window_rect.y(),
                      move.window_rect.width(), move.window_rect.height());
#else

#if USE_TOP_CLIP_WINDOW
  QRectF scene_geo = native_view_->mapRectToScene(move.window_rect.x() * scale_factor_, move.window_rect.y() * scale_factor_,
                    move.window_rect.width() * scale_factor_, move.window_rect.height() * scale_factor_);

  widgets->top_window->setGeometry(scene_geo.x() - clip_window_rect_.x(), scene_geo.y() - clip_window_rect_.y(),
                      scene_geo.width(), scene_geo.height());

  widgets->window->setGeometry(0, 0, scene_geo.width(), scene_geo.height());
#else
  QRectF scene_geo = native_view_->mapRectToScene(move.window_rect.x() * scale_factor_, move.window_rect.y() * scale_factor_,
                    move.window_rect.width() * scale_factor_, move.window_rect.height() * scale_factor_);

  QRect cliped_rect = scene_geo.toRect() & clip_window_rect_;

  widgets->top_window->setGeometry(cliped_rect.x(), cliped_rect.y(), cliped_rect.width(), cliped_rect.height());

  widgets->window->setGeometry(scene_geo.x() - cliped_rect.x(), scene_geo.y() - cliped_rect.y(), scene_geo.width(), scene_geo.height());
#endif //USE_TOP_CLIP_WINDOW

#endif

  if (!is_hidden_)
    widgets->top_window->show();
}

void QtPluginContainerManager::MovePluginContainer(
    const WebPluginGeometry& move, gfx::Point& view_offset) {

  WindowedPluginWidgets *widgets = MapIDToWidgets(move.window);
  if (!widgets)
    return;

  if (move.rects_valid) {
    WebPluginGeometry *saved_geo = MapIDToGeometry(move.window);
    *saved_geo = move;
    MovePluginContainer(widgets, move, view_offset);
  }
}

void QtPluginContainerManager::RelocatePluginContainers(gfx::Point& offset)
{
  PluginWindowToGeometryMap::const_iterator i = plugin_window_to_geometry_map_.begin();

  for (; i != plugin_window_to_geometry_map_.end(); ++i) {
    MovePluginContainer(MapIDToWidgets(i->first), *(i->second), offset);
  }
}

WindowedPluginWidgets* QtPluginContainerManager::MapIDToWidgets(
    gfx::PluginWindowHandle id) {
  PluginWindowToWidgetsMap::const_iterator i =
      plugin_window_to_widgets_map_.find(id);
  if (i != plugin_window_to_widgets_map_.end())
    return i->second;

  LOG(ERROR) << "Request for widget host for unknown window id " << id;
  return NULL;
}

WebPluginGeometry* QtPluginContainerManager::MapIDToGeometry(
    gfx::PluginWindowHandle id) {
  PluginWindowToGeometryMap::const_iterator i =
      plugin_window_to_geometry_map_.find(id);
  if (i != plugin_window_to_geometry_map_.end())
    return i->second;

  LOG(ERROR) << "Request for geometry for unknown window id " << id;
  return NULL;
}


}  // namespace npapi
}  // namespace webkit

#include "webkit/plugins/npapi/moc_qt_plugin_container_manager.cc"
