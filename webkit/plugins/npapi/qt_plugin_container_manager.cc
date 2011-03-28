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

#include <QLabel>

#include "base/logging.h"
#include "webkit/plugins/npapi/webplugin.h"

namespace webkit {
namespace npapi {

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
  QX11EmbedContainer *container = new QX11EmbedContainer(host_widget_);
  container->embedClient(id);
  //QLabel *container = new QLabel("just a test");
  
  container->show();
  
  plugin_window_to_widget_map_.insert(std::make_pair(id, container));

  return NULL;
}

void QtPluginContainerManager::DestroyPluginContainer(
    gfx::PluginWindowHandle id) {
  DCHECK(host_widget_);
  QWidget* widget = MapIDToWidget(id);
  //if (widget)
  //  gtk_widget_destroy(widget);

  plugin_window_to_widget_map_.erase(id);
  
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
    const WebPluginGeometry& move, gfx::Point& view_offset) {
  DCHECK(host_widget_);
  QWidget *widget = MapIDToWidget(move.window);
  if (!widget)
    return;

  if (!move.visible) {
    widget->hide();
    return;
  }

  widget->show();

  if (!move.rects_valid)
    return;

  /*
  GdkRectangle clip_rect = move.clip_rect.ToGdkRectangle();
  GdkRegion* clip_region = gdk_region_rectangle(&clip_rect);
  gfx::SubtractRectanglesFromRegion(clip_region, move.cutout_rects);
  gdk_window_shape_combine_region(widget->window, clip_region, 0, 0);
  gdk_region_destroy(clip_region);
  */
  
  // Update the window position.  Resizing is handled by WebPluginDelegate.
  // TODO(deanm): Verify that we only need to move and not resize.
  // TODO(evanm): we should cache the last shape and position and skip all
  // of this business in the common case where nothing has changed.
  int current_x, current_y;

  // Until the above TODO is resolved, we can grab the last position
  // off of the GtkFixed with a bit of hackery.
  /*
  GValue value = {0};
  g_value_init(&value, G_TYPE_INT);
  gtk_container_child_get_property(GTK_CONTAINER(host_widget_), widget,
                                   "x", &value);
  current_x = g_value_get_int(&value);
  gtk_container_child_get_property(GTK_CONTAINER(host_widget_), widget,
                                   "y", &value);
  current_y = g_value_get_int(&value);
  g_value_unset(&value);

  if (move.window_rect.x() != current_x ||
      move.window_rect.y() != current_y) {
    // Calling gtk_fixed_move unnecessarily is a no-no, as it causes the
    // parent window to repaint!
    gtk_fixed_move(GTK_FIXED(host_widget_),
                   widget,
                   move.window_rect.x(),
                   move.window_rect.y());
  }

  gtk_plugin_container_set_size(widget,
                                move.window_rect.width(),
                                move.window_rect.height());
  */
  widget->setGeometry(move.window_rect.x() + view_offset.x(), move.window_rect.y() + view_offset.y(),
                      move.window_rect.width(), move.window_rect.height());
  
  DNOTIMPLEMENTED() << " " << move.window << " " << move.window_rect.x() << "+" << move.window_rect.y() << "+"
                   << move.window_rect.width() << "x" << move.window_rect.height();
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
