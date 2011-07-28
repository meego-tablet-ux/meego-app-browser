// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_QT_H_
#define CHROME_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_QT_H_

#include <QMap>
#include <vector>
#include <string>

#include "base/scoped_ptr.h"
#include "base/time.h"
#include "content/browser/renderer_host/render_widget_host_view.h"
#include "ui/gfx/native_widget_types.h"
#include "webkit/glue/webcursor.h"
#include "webkit/glue/webmenuitem.h"
#include "ui/gfx/rect.h"
#include "webkit/plugins/npapi/qt_plugin_container_manager_host_delegate.h"

namespace webkit {
namespace npapi {
  class QtPluginContainerManager;
}
}

static const int kMaxWindowWidth = 4000;
static const int kMaxWindowHeight = 4000;
static const char* kRenderWidgetHostViewKey = "__RENDER_WIDGET_HOST_VIEW__";

class RenderWidgetHost;
struct NativeWebKeyboardEvent;

class VideoRendererWidget;
class PluginRendererWidget;

// -----------------------------------------------------------------------------
// See comments in render_widget_host_view.h about this class and its members.
// -----------------------------------------------------------------------------
class RenderWidgetHostViewQt : public RenderWidgetHostView,
                               public webkit::npapi::QtPluginContainerManagerHostDelegate {
 public:

  explicit RenderWidgetHostViewQt(RenderWidgetHost* widget);
  ~RenderWidgetHostViewQt();

  // Initialize this object for use as a drawing area.
  void InitAsChild();

  // RenderWidgetHostView implementation.
  virtual void InitAsPopup(RenderWidgetHostView* parent_host_view,
                           const gfx::Rect& pos);
  virtual void InitAsFullscreen() {DNOTIMPLEMENTED();}
  virtual RenderWidgetHost* GetRenderWidgetHost() const {return host_; }
  virtual void DidBecomeSelected();
  virtual void WasHidden();
  virtual void SetSize(const gfx::Size& size);
  virtual void SetPreferredSize(const gfx::Size& size);
  virtual gfx::NativeView GetNativeView();
  virtual void MovePluginWindows(
  const std::vector<webkit::npapi::WebPluginGeometry>& moves);
  virtual void Focus();
  virtual void Blur();
  virtual bool HasFocus();
  virtual void Show();
  virtual void Hide();
  virtual bool IsShowing();
  virtual gfx::Rect GetViewBounds() const;
  virtual void SetScrollPosition(const gfx::Point& pos);
  virtual void UpdateCursor(const WebCursor& cursor);
  virtual void SetIsLoading(bool is_loading);
  virtual void DidUpdateBackingStore(
      const gfx::Rect& scroll_rect, int scroll_dx, int scroll_dy,
      const std::vector<gfx::Rect>& copy_rects);
  virtual void RenderViewGone(base::TerminationStatus status,
                              int error_code);
  virtual void Destroy();
  virtual void WillDestroyRenderWidget(RenderWidgetHost* rwh) {};
  virtual void SetTooltipText(const std::wstring& tooltip_text);
  virtual void SelectionChanged(const std::string& text);
  virtual void ShowingContextMenu(bool showing);
  virtual BackingStore* AllocBackingStore(const gfx::Size& size);
  virtual void SetBackground(const SkBitmap& background);
  virtual void CreatePluginContainer(gfx::PluginWindowHandle id);
  virtual void DestroyPluginContainer(gfx::PluginWindowHandle id);
  virtual void SetVisuallyDeemphasized(const SkColor* color, bool animate);
  virtual bool ContainsNativeView(gfx::NativeView native_view) const;
  virtual void AcceleratedCompositingActivated(bool activated) {DNOTIMPLEMENTED();};
  virtual gfx::PluginWindowHandle GetCompositingSurface() {DNOTIMPLEMENTED();};
  virtual void ReleaseCompositingSurface(gfx::PluginWindowHandle surface) {DNOTIMPLEMENTED();};
  virtual void ImeUpdateTextInputState(WebKit::WebTextInputType, const gfx::Rect&);
  virtual void ImeCancelComposition();

#if defined(TOOLKIT_MEEGOTOUCH)
  virtual void SetBounds(const gfx::Rect& rect);
  virtual void SetScaleFactor(double factor);

  // this one is derived from QtPluginContainerManagerHostDelegate
  virtual void OnCloseFSPluginWindow(gfx::PluginWindowHandle id);

  virtual void UpdateWebKitNodeInfo(unsigned int node_info);
  virtual void UpdateSelectionRange(gfx::Point start, gfx::Point end, int height, bool set);

  ////////////////////////////////////////////////////////////
  // for tiled backing store
  virtual void UpdateContentsSize(const gfx::Size& size);
  virtual gfx::Size GetContentsSize();
  virtual gfx::Rect GetVisibleRect();

  virtual void DidBackingStoreScale();
  virtual void DidBackingStorePaint(const gfx::Rect& rect);

  virtual void PaintTileAck(unsigned int seq, unsigned int tag, const gfx::Rect& rect, const gfx::Rect& pixmap_rect);

  virtual void ScenePosChanged();

  gfx::Size CalPluginWindowSize();
  gfx::Size CalFSWinSize();

  virtual gfx::Size GetFSPluginWindowSize();

  void SetPluginWindowSize();
 ////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////

  //// for HW accelerated HTML5 video
  virtual void CreateVideoWidget(unsigned int id, const gfx::Size& size);
  virtual void UpdateVideoWidget(unsigned int id, unsigned int pixmap, const gfx::Rect& rect);
  virtual void EnableVideoWidget(unsigned int id, bool enabled);
  virtual void DestroyVideoWidgetPixmap(unsigned int id, unsigned int pixmap);
  virtual void DestroyVideoWidget(unsigned int id);

  // For compose flash embeded window
  virtual void ComposeEmbededFlashWindow(const gfx::Rect& rect);
  virtual void ReShowEmbededFlashWindow();

#if defined(PLUGIN_DIRECT_RENDERING)
  virtual void UpdatePluginWidget(unsigned int id,
                                  unsigned int pixmap,
                                  const gfx::Rect& rect,
                                  unsigned int seq);
  virtual void DestroyPluginWidget(unsigned int id);
#endif
  void DidPaintPluginWidget(unsigned int id, unsigned int ack);

#endif

  gfx::NativeView native_view() const {return view_;}

  void Paint(const gfx::Rect&);

  // Called by GtkIMContextWrapper to forward a keyboard event to renderer.
  // Before calling RenderWidgetHost::ForwardKeyboardEvent(), this method
  // calls GtkKeyBindingsHandler::Match() against the event and send matched
  // edit commands to renderer by calling
  // RenderWidgetHost::ForwardEditCommandsForNextKeyEvent().
  void ForwardKeyboardEvent(const NativeWebKeyboardEvent& event);

 private:
  friend class RWHVQtWidget;
  friend class RWHVQtWidgetView;

  // Returns whether the widget needs an input grab (GTK+ and X) to work
  // properly.
  bool NeedsInputGrab();

  // Returns whether this render view is a popup (<select> dropdown or
  // autocomplete window).
  bool IsPopup();

  // Update the display cursor for the render view.
  void ShowCurrentCursor();

  // The model object.
  RenderWidgetHost* host_;

  // The native UI widget.
  QGraphicsWidget* view_;
  QGraphicsWidget* parent_;

  // This is true when we are currently painting and thus should handle extra
  // paint requests by expanding the invalid rect rather than actually
  // painting.
  bool about_to_validate_and_paint_;

  // This is the rectangle which we'll paint.
  gfx::Rect invalid_rect_;

  // Whether or not this widget is hidden.
  bool is_hidden_;

  // Whether we are currently loading.
  bool is_loading_;
  // The cursor for the page. This is passed up from the renderer.
  WebCursor current_cursor_;

  // Whether we are showing a context menu.
  bool is_showing_context_menu_;

  // The time at which this view started displaying white pixels as a result of
  // not having anything to paint (empty backing store from renderer). This
  // value returns true for is_null() if we are not recording whiteout times.
  base::TimeTicks whiteout_start_time_;

  // The time it took after this view was selected for it to be fully painted.
  base::TimeTicks tab_switch_paint_time_;

  // If true, fade the render widget when painting it.
  bool visually_deemphasized_;

  // Variables used only for popups --------------------------------------------
  // Our parent widget.
  RenderWidgetHostView* parent_host_view_;
  // We ignore the first mouse release on popups.  This allows the popup to
  // stay open.
  bool is_popup_first_mouse_release_;

  // Whether or not this widget was focused before shadowed by another widget.
  // Used in OnGrabNotify() handler to track the focused state correctly.
  bool was_focused_before_grab_;

  // True if we are responsible for creating an X grab. This will only be used
  // for <select> dropdowns. It should be true for most such cases, but false
  // for extension popups.
  bool do_x_grab_;

  // The size that we want the renderer to be.  We keep this in a separate
  // variable because resizing in GTK+ is async.
  gfx::Size requested_size_;

  gfx::Size contents_size_;

  webkit::npapi::QtPluginContainerManager* plugin_container_manager_;

  gfx::Point scene_pos_;

  unsigned int webkit_node_info_;

  QMap<unsigned int, VideoRendererWidget* > video_widgets_map_;

#if defined(PLUGIN_DIRECT_RENDERING)
  QMap<unsigned int, PluginRendererWidget*> plugin_widgets_map_;
#endif
};

#endif  // CHROME_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_VIEW_QT_H_
