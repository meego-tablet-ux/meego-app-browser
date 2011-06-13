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
#endif

#include "base/logging.h"
#include "webkit/plugins/npapi/webplugin.h"

namespace webkit {
namespace npapi {

#if defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)

FSPluginWidgets::~FSPluginWidgets()
{
  NOTIMPLEMENTED();
  delete top_window;
  //close_btn should be the child of top_window, so not need to delete explicitly
}

gfx::PluginWindowHandle QtPluginContainerManager::MapCloseBtnToID(QPushButton* button)
{
  for (PluginWindowToFSWidgetsMap::const_iterator i = plugin_window_to_fswidgets_map_.begin();
       i != plugin_window_to_fswidgets_map_.end(); ++i) {
    FSPluginWidgets* fs_widgets = i->second;
    if (fs_widgets->close_btn == button)
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


QtPluginContainerManager::QtPluginContainerManager(QtPluginContainerManagerHostDelegate *host)
    : QObject(), host_widget_(NULL), host_delegate_(host) {
  fs_win_size_.SetSize(0, 0);
  is_hidden_ = true;
}

QWidget* QtPluginContainerManager::CreatePluginContainer(
    gfx::PluginWindowHandle id) {
  DLOG(INFO) << "PluginWindowHandle " << id;

  DCHECK(host_widget_);

  QWidget *window = NULL;
#if defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
  QWidget *fs_window = new QWidget(host_widget_);
  QPushButton *button = new QPushButton("Close", fs_window);

  FSPluginWidgets *fs_widgets = new FSPluginWidgets();
  fs_widgets->top_window = fs_window;
  fs_widgets->close_btn = button;
  plugin_window_to_fswidgets_map_.insert(std::make_pair(id, fs_widgets));

  fs_window->setGeometry(0, 0, fs_win_size_.width(), fs_win_size_.height());
  button->setGeometry(0, fs_win_size_.height() - FSPluginCloseBarHeight(), fs_win_size_.width(), FSPluginCloseBarHeight());

  connect(button, SIGNAL(clicked()), this, SLOT(CloseFSPluginWindow()));

  window = fs_window;
  window->show();

  QX11EmbedContainer *container = new QX11EmbedContainer(fs_window);
#else
  QX11EmbedContainer *container = new QX11EmbedContainer(host_widget_);
  window = container;
#endif

  container->embedClient(id);
  container->show();
  window->show();
  
  plugin_window_to_widget_map_.insert(std::make_pair(id, container));

  WebPluginGeometry *geo = new struct WebPluginGeometry();
  plugin_window_to_geometry_map_.insert(std::make_pair(id, geo));

  return NULL;
}

void QtPluginContainerManager::DestroyPluginContainer(
    gfx::PluginWindowHandle id) {
  DCHECK(host_widget_);
  QWidget* widget = MapIDToWidget(id);
  //if (widget)
  //  gtk_widget_destroy(widget);

  plugin_window_to_widget_map_.erase(id);
  plugin_window_to_geometry_map_.erase(id);

#if defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
// hmm, the erase(id) operation don't call FSPluginWidgets's destructor, strange
  PluginWindowToFSWidgetsMap::const_iterator iter = plugin_window_to_fswidgets_map_.find(id);
  if (iter != plugin_window_to_fswidgets_map_.end()) {
    FSPluginWidgets* fs_widgets = iter->second;
    delete fs_widgets->top_window;
    fs_widgets->top_window = NULL; // in case Destructor some how been called...
  }
  plugin_window_to_fswidgets_map_.erase(id);
#endif
}

void QtPluginContainerManager::Show()
{
  for (PluginWindowToWidgetMap::const_iterator i =
          plugin_window_to_widget_map_.begin();
       i != plugin_window_to_widget_map_.end(); ++i) {
    i->second->show();
  }

  is_hidden_ = false;
}

void QtPluginContainerManager::Hide()
{
  for (PluginWindowToWidgetMap::const_iterator i =
           plugin_window_to_widget_map_.begin();
       i != plugin_window_to_widget_map_.end(); ++i) {
    i->second->hide();
  }

  is_hidden_ = true;
}

void QtPluginContainerManager::MovePluginContainer(
    QWidget *widget, const WebPluginGeometry& move, gfx::Point& view_offset) {
  DCHECK(host_widget_);
  if (!widget)
    return;

  if (!move.visible) {
    widget->hide();
    return;
  }

  if (!move.rects_valid)
    return;

  int current_x, current_y;
  widget->setGeometry(move.window_rect.x() + view_offset.x(), move.window_rect.y() + view_offset.y(),
                      move.window_rect.width(), move.window_rect.height());

  if (!is_hidden_)
    widget->show();

  DLOG(INFO) << " " << move.window << " " << move.window_rect.x() << "+" << move.window_rect.y() << "+"
                   << move.window_rect.width() << "x" << move.window_rect.height()
                   << " - offset = " << view_offset.x() << "-" << view_offset.y();
}

void QtPluginContainerManager::MovePluginContainer(
    const WebPluginGeometry& move, gfx::Point& view_offset) {
  QWidget *widget = MapIDToWidget(move.window);
  if (!widget)
    return;

  if (move.rects_valid) {
    WebPluginGeometry *saved_geo = MapIDToGeometry(move.window);
    *saved_geo = move;
    MovePluginContainer(widget, move, view_offset);
  }
}

void QtPluginContainerManager::RelocatePluginContainers(gfx::Point& offset)
{
  PluginWindowToGeometryMap::const_iterator i = plugin_window_to_geometry_map_.begin();

  for (; i != plugin_window_to_geometry_map_.end(); ++i) {
    MovePluginContainer(MapIDToWidget(i->first), *(i->second), offset);
  }
}

QWidget* QtPluginContainerManager::MapIDToWidget(
    gfx::PluginWindowHandle id) {
  PluginWindowToWidgetMap::const_iterator i =
      plugin_window_to_widget_map_.find(id);
  if (i != plugin_window_to_widget_map_.end())
    return i->second;

  LOG(ERROR) << "Request for widget host for unknown window id " << id;
  return NULL;
}

gfx::PluginWindowHandle QtPluginContainerManager::MapWidgetToID(
     QWidget* widget) {
  for (PluginWindowToWidgetMap::const_iterator i =
          plugin_window_to_widget_map_.begin();
       i != plugin_window_to_widget_map_.end(); ++i) {
    if (i->second == widget)
      return i->first;
  }

  LOG(ERROR) << "Request for id for unknown widget";
  return 0;
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


// static
/*
void QtPluginContainerManager::RealizeCallback(GtkWidget* widget,
                                                void* user_data) {
  QtPluginContainerManager* plugin_container_manager =
      static_cast<QtPluginContainerManager*>(user_data);

  gfx::PluginWindowHandle id = plugin_container_manager->MapWidgetToID(widget);
  if (id)
    gtk_socket_add_id(GTK_SOCKET(widget), id);
}
*/

}  // namespace npapi
}  // namespace webkit

#include "webkit/plugins/npapi/moc_qt_plugin_container_manager.cc"
