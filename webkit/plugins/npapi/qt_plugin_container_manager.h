// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_GLUE_PLUGINS_QT_PLUGIN_CONTAINER_MANAGER_H_
#define WEBKIT_GLUE_PLUGINS_QT_PLUGIN_CONTAINER_MANAGER_H_

#include <map>
#include "base/meegotouch_config.h"

#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/rect.h"

#include <QObject>
#include <QRect>
#include <QX11EmbedContainer>

#include "webkit/plugins/npapi/qt_plugin_container_manager_host_delegate.h"

#if defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
class QPushButton;
#endif

static const int FullScreenPluginCloseBarHeight = 40;

namespace webkit {
namespace npapi {

struct WebPluginGeometry;

struct WindowedPluginWidgets{
  WindowedPluginWidgets() : top_window(NULL), window(NULL) {}
  ~WindowedPluginWidgets();
  QWidget *top_window;
  QWidget *window;
#if defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
  QPushButton *close_btn;
#endif
};


class QtPluginContainer : public QX11EmbedContainer {

 public:
  QtPluginContainer(gfx::PluginWindowHandle id, QWidget *parent = 0)
            : id_(id), QX11EmbedContainer(parent) { embedded_ = false; }

 protected:
  void showEvent(QShowEvent *event);
  void hideEvent(QHideEvent *event);

 private:
  gfx::PluginWindowHandle id_;
  bool embedded_;

};


class QtPluginContainerManager : public QObject {

  Q_OBJECT

 public:
  QtPluginContainerManager(QtPluginContainerManagerHostDelegate *host);
  ~QtPluginContainerManager();

  // Sets the widget that will host the plugin containers.
  void set_host_widget(QWidget *widget) { host_widget_ = widget; }

  // the GraphicsWidget that host the plugin's representative in webkit.
  void set_native_view(QGraphicsWidget *view) { native_view_ = view; }

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

#if !defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
  void SetClipRect(QRect rect);
  void SetScaleFactor(double factor);
#endif

#if defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
  gfx::PluginWindowHandle MapCloseBtnToID(QPushButton* button);
#endif

  void Hide();
  void Show();
  void ComposeEmbededFlashWindow(const gfx::Rect& rect);
  void ReShowEmbededFlashWindow();

  //This slot should have been surrounded by #if defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
  //But it seems that moc have trouble to generate the metadata in the MACRO. So leave
  //the define here and surround the implementation with MACROS instead.
 public Q_SLOTS:
  void CloseFSPluginWindow();

 private:
  // Compare to the public version, this internal one do not save the move info
  void MovePluginContainer(WindowedPluginWidgets* widgets, const WebPluginGeometry& move, gfx::Point& view_offset);

#if !defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
  QWidget* GetTopClipWindow();
#endif

  // Maps a plugin XID to the corresponding container widgets structure.
  WindowedPluginWidgets* MapIDToWidgets(gfx::PluginWindowHandle id);

  // Maps a plugin XID to the corresponding container widget's geometry.
  WebPluginGeometry* MapIDToGeometry(gfx::PluginWindowHandle id);

  // Callback for when the plugin container gets realized, at which point it
  // plugs the plugin XID.
  //static void RealizeCallback(QGraphicsWidget *widget, void *user_data);

  // Parent of the plugin containers.
  QWidget* host_widget_;

  // Parent graphicsitem that contain the plugin's representative in webkit
  QGraphicsWidget* native_view_;

  // A map that store the plugin gemeotry for relocate usage.
  typedef std::map<gfx::PluginWindowHandle, WebPluginGeometry*> PluginWindowToGeometryMap;
  PluginWindowToGeometryMap plugin_window_to_geometry_map_;

  // A map that store the windowed plugin related widgets.
  typedef std::map<gfx::PluginWindowHandle, WindowedPluginWidgets*> PluginWindowToWidgetsMap;
  PluginWindowToWidgetsMap plugin_window_to_widgets_map_;

  webkit::npapi::QtPluginContainerManagerHostDelegate *host_delegate_;

  gfx::Size fs_win_size_;
  bool is_hidden_;

#if !defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
  // clip_window_rect_ is used to clip the windowed plugins when they move around
  QRect clip_window_rect_;
  QWidget* top_clip_window_;
  double scale_factor_;
#endif

};

}  // namespace npapi
}  // namespace webkit
#endif  // WEBKIT_GLUE_PLUGINS_QT_PLUGIN_CONTAINER_MANAGER_H_
