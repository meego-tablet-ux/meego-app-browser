// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_GLUE_PLUGINS_QT_PLUGIN_CONTAINER_MANAGER_H_
#define WEBKIT_GLUE_PLUGINS_QT_PLUGIN_CONTAINER_MANAGER_H_

#include <map>
#include "base/meegotouch_config.h"

#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/point.h"
#include "ui/gfx/size.h"

#include <QObject>

#include "webkit/plugins/npapi/qt_plugin_container_manager_host_delegate.h"

class QWidget;

#if defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
class QPushButton;
#endif

static const int FullScreenPluginCloseBarHeight = 40;

namespace webkit {
namespace npapi {

struct WebPluginGeometry;

#if defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
struct FSPluginWidgets{
  FSPluginWidgets() : top_window(NULL), close_btn(NULL) {}
  ~FSPluginWidgets();
  QWidget *top_window;
  QPushButton *close_btn;
};
#endif

class QtPluginContainerManager : public QObject {

  Q_OBJECT

 public:
  QtPluginContainerManager(QtPluginContainerManagerHostDelegate *host);

  // Sets the widget that will host the plugin containers. Must be a GtkFixed.
  void set_host_widget(QWidget *widget) { host_widget_ = widget; }

  // Creates a new plugin container, for a given plugin XID.
  QWidget* CreatePluginContainer(gfx::PluginWindowHandle id);

  // Destroys a plugin container, given the plugin XID.
  void DestroyPluginContainer(gfx::PluginWindowHandle id);

  // Takes an update from WebKit about a plugin's position and side and moves
  // the plugin accordingly.
  void MovePluginContainer(const WebPluginGeometry& move, gfx::Point& view_offset);

  // When the web page been scrolled in a flickable container. the windowed plugin
  // need to update it's position accordingly.
  void RelocatePluginContainers(gfx::Point& offset);

  int FSPluginCloseBarHeight() { return FullScreenPluginCloseBarHeight; }
  int SetFSWindowSize(gfx::Size new_size) { fs_win_size_ = new_size; }

#if defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
  gfx::PluginWindowHandle MapCloseBtnToID(QPushButton* button);
#endif

  void Hide();
  void Show();

  //This slot should have been surrounded by #if defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
  //But it seems that moc have trouble to generate the metadata in the MACRO. So leave
  //the define here and surround the implementation with MACROS instead.
 public Q_SLOTS:
  void CloseFSPluginWindow();

 private:
  // Compare to the public version, this internal one do not save the move info
  void MovePluginContainer(QWidget* widget, const WebPluginGeometry& move, gfx::Point& view_offset);

  // Maps a plugin XID to the corresponding container widget.
  QWidget* MapIDToWidget(gfx::PluginWindowHandle id);

  // Maps a plugin XID to the corresponding container widget's geometry.
  WebPluginGeometry* MapIDToGeometry(gfx::PluginWindowHandle id);

  // Maps a container widget to the corresponding plugin XID.
  gfx::PluginWindowHandle MapWidgetToID(QWidget* widget);

  // Callback for when the plugin container gets realized, at which point it
  // plugs the plugin XID.
  //static void RealizeCallback(QGraphicsWidget *widget, void *user_data);

  // Parent of the plugin containers.
  QWidget* host_widget_;

  // A map that associates plugin containers to the plugin XID.
  typedef std::map<gfx::PluginWindowHandle, QWidget*> PluginWindowToWidgetMap;
  PluginWindowToWidgetMap plugin_window_to_widget_map_;

  // A map that store the plugin gemeotry for relocate usage.
  typedef std::map<gfx::PluginWindowHandle, WebPluginGeometry*> PluginWindowToGeometryMap;
  PluginWindowToGeometryMap plugin_window_to_geometry_map_;

#if defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
  // A map that store the fullscreen plugin related widgets.
  typedef std::map<gfx::PluginWindowHandle, FSPluginWidgets*> PluginWindowToFSWidgetsMap;
  PluginWindowToFSWidgetsMap plugin_window_to_fswidgets_map_;
#endif

  webkit::npapi::QtPluginContainerManagerHostDelegate *host_delegate_;

  gfx::Size fs_win_size_;
  bool is_hidden_;
};

}  // namespace npapi
}  // namespace webkit
#endif  // WEBKIT_GLUE_PLUGINS_QT_PLUGIN_CONTAINER_MANAGER_H_
