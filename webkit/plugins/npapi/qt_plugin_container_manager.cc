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

#include <QPushButton>

#include "base/logging.h"
#include "webkit/plugins/npapi/webplugin.h"

namespace webkit {
namespace npapi {

FSPluginWidgets::~FSPluginWidgets()
{
  NOTIMPLEMENTED();
  delete top_window;
  //close_btn should be the child of top_window, so not need to delete explicitly
}


QWidget* QtPluginContainerManager::CreatePluginContainer(
    gfx::PluginWindowHandle id) {
  /*
  DCHECK(host_widget_);
  GtkWidget *widget = gtk_plugin_container_new();
  plugin_window_to_widget_map_.insert(std::make_pair(id, widget));

  // The Realize callback is responsible for adding the plug into the socket.
  // The reason is 2-fold:
  // - the plug can't be added until the socket is realized, but this may not
  // happen until the socket is attached to a top-level window, which isn't the
  // case for background tabs.
  // - when dragging tabs, the socket gets unrealized, which breaks the XEMBED
  // connection. We need to make it again when the tab is reattached, and the
  // socket gets realized again.
  //
  // Note, the RealizeCallback relies on the plugin_window_to_widget_map_ to
  // have the mapping.
  g_signal_connect(widget, "realize",
                   G_CALLBACK(RealizeCallback), this);

  // Don't destroy the widget when the plug is removed.
  g_signal_connect(widget, "plug-removed",
                   G_CALLBACK(gtk_true), NULL);

  gtk_container_add(GTK_CONTAINER(host_widget_), widget);
  gtk_widget_show(widget);
  */
  DNOTIMPLEMENTED() << "PluginWindowHandle " << id;

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
//  PluginWindowToFSWidgetsMap::iterator iter = plugin_window_to_fswidgets_map_.find(id);
//  if (iter != plugin_window_to_fswidgets_map_.end()) {
//    FSPluginWidgets* fs_widgets = iter.second();
//    delete fs_widgets->top_window;
//    delete fs_widgets->close_btn;
//  }
  NOTIMPLEMENTED();
  plugin_window_to_fswidgets_map_.erase(id);
#endif

  DNOTIMPLEMENTED();
}

void QtPluginContainerManager::Show()
{
  for (PluginWindowToWidgetMap::const_iterator i =
          plugin_window_to_widget_map_.begin();
       i != plugin_window_to_widget_map_.end(); ++i) {
    i->second->show();
  }
}

void QtPluginContainerManager::Hide()
{
  for (PluginWindowToWidgetMap::const_iterator i =
           plugin_window_to_widget_map_.begin();
       i != plugin_window_to_widget_map_.end(); ++i) {
    i->second->hide();
  }

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

  widget->show();

  if (!move.rects_valid)
    return;

  int current_x, current_y;
  widget->setGeometry(move.window_rect.x() + view_offset.x(), move.window_rect.y() + view_offset.y(),
                      move.window_rect.width(), move.window_rect.height());
  DNOTIMPLEMENTED() << " " << move.window << " " << move.window_rect.x() << "+" << move.window_rect.y() << "+"
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
