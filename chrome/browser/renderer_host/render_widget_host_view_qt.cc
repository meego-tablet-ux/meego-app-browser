// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//#include "chrome/browser/renderer_host/render_widget_host_view_gtk.h"

// If this gets included after the gtk headers, then a bunch of compiler
// errors happen because of a "#define Status int" in Xlib.h, which interacts
// badly with URLRequestStatus::Status.
#include "chrome/common/render_messages.h"
#include "base/meegotouch_config.h"

#include <QtGui/QApplication>
#include <QtGui/QCursor>
#include <QtGui/QInputContext>
#include <QtGui/QDesktopWidget>
#include <QtGui/QX11Info>
#include <QGraphicsLinearLayout>
#include <QGraphicsView>
#include <QScrollBar>
#include <QtCore/QEvent>
#include <QtCore/QVariant>

#include <algorithm>
#include <string>

#include "ui/base/l10n/l10n_util.h"
#include "ui/base/x/x11_util.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/string_util.h"
#include "base/task.h"
#include "base/time.h"
#include "base/utf_string_conversions.h"
#include "content/browser/renderer_host/backing_store_x.h"
#include "content/browser/renderer_host/render_widget_host.h"
#include "content/browser/renderer_host/rwhv_qt_widget.h"
#include "chrome/common/chrome_switches.h"
#include "content/common/native_web_keyboard_event.h"
#include "content/common/view_messages.h"
#include "content/browser/renderer_host/render_widget_host_view.h"
#include "chrome/browser/renderer_host/render_widget_host_view_qt.h"
#include "chrome/browser/ui/meegotouch/qt_util.h"
#include "webkit/plugins/npapi/webplugin.h"
#include "webkit/plugins/npapi/qt_plugin_container_manager.h"

// static
RenderWidgetHostView* RenderWidgetHostView::CreateViewForWidget(
    RenderWidgetHost* widget) {
  return new RenderWidgetHostViewQt(widget);
}

RenderWidgetHostViewQt::RenderWidgetHostViewQt(RenderWidgetHost* widget_host)
    : host_(widget_host),
      about_to_validate_and_paint_(false),
      is_hidden_(false),
      is_loading_(false),
      is_showing_context_menu_(false),
      visually_deemphasized_(false),
      parent_host_view_(NULL),
      parent_(NULL),
      is_popup_first_mouse_release_(true),
      was_focused_before_grab_(false),
      do_x_grab_(false),
      webkit_node_info_(0),
      view_(NULL) {
  host_->set_view(this);
  plugin_container_manager_ = new webkit::npapi::QtPluginContainerManager(this);
}

RenderWidgetHostViewQt::~RenderWidgetHostViewQt() {
    delete plugin_container_manager_;
}

void RenderWidgetHostViewQt::InitAsChild() {
  scene_pos_ = gfx::Point(0, 0);
  view_ = new RWHVQtWidget(this);
  view_->show();
}
void RenderWidgetHostViewQt::InitAsPopup(
    RenderWidgetHostView* parent_host_view, const gfx::Rect& pos) {
  parent_host_view_ = parent_host_view;

  if (parent_host_view_ == NULL) {
    DNOTIMPLEMENTED();
    return;
  }

  double scale = 1.0;

  // The scale factor of popup widget should be the same 
  // as the parent's. 
  RenderWidgetHost* host = parent_host_view_->GetRenderWidgetHost();
  if(host) {
    scale = flatScaleByStep(host->GetScaleFactor());
    host_->SetScaleFactor(scale);
  }

  // Set contents size and preferred size for popup widget
  contents_size_ = gfx::Size(pos.width(), pos.height());
  host_->SetPreferredSize(gfx::Size(pos.width(), pos.height()));

  parent_ = parent_host_view_->GetNativeView();

  requested_size_ = gfx::Size(std::min(pos.width(), kMaxWindowWidth),
                              std::min(pos.height(), kMaxWindowHeight));

  // Initialize the popup widget for showing popup menu item
  RWHVQtWidget* widget = new RWHVQtWidget(this);

  widget->setParentItem(parent_->parentItem());
  widget->SetScaleFactor(scale);

  QRect geometry(pos.x()*scale, pos.y()*scale, 
                 requested_size_.width()*scale, 
                 requested_size_.height()*scale);

  widget->setGeometry(geometry);
  widget->show();
  view_ = widget;
  
  host_->WasResized();
}

void RenderWidgetHostViewQt::DidBecomeSelected() {
  if (!is_hidden_)
    return;

  plugin_container_manager_->Show();

  is_hidden_ = false;

  if (tab_switch_paint_time_.is_null())
    tab_switch_paint_time_ = base::TimeTicks::Now();
  host_->WasRestored();

  RWHVQtWidget* widget = reinterpret_cast<RWHVQtWidget*>(view_);
  if(widget) widget->DidBecomeSelected();
}

void RenderWidgetHostViewQt::WasHidden() {
  if (is_hidden_)
    return;

  plugin_container_manager_->Hide();

  // If we receive any more paint messages while we are hidden, we want to
  // ignore them so we don't re-allocate the backing store.  We will paint
  // everything again when we become selected again.
  is_hidden_ = true;

  RWHVQtWidget* widget = reinterpret_cast<RWHVQtWidget*>(view_);
  if(widget) widget->WasHidden();

  // If we have a renderer, then inform it that we are being hidden so it can
  // reduce its resource utilization.
  GetRenderWidgetHost()->WasHidden();
}

void RenderWidgetHostViewQt::SetSize(const gfx::Size& size) {
  // in tiled backing store, the view size is determined by
  // content size * scale
#if !defined(TILED_BACKING_STORE)
  // This is called when webkit has sent us a Move message.
  int width = std::min(size.width(), kMaxWindowWidth);
  int height = std::min(size.height(), kMaxWindowHeight);
  if (IsPopup()) {
    // We're a popup, honor the size request.
    //MEEGO: TODO
    DNOTIMPLEMENTED();
  }

  DLOG(INFO) << "-------" << __PRETTY_FUNCTION__
      << ",w: " << width
      << ",h: " << height
      << std::endl;  

  // Update the size of the RWH.
  if (requested_size_.width() != width ||
      requested_size_.height() != height) {
    requested_size_ = gfx::Size(width, height);
    host_->WasResized();
  }
  view_->setGeometry(0, 0, width, height);
#endif
}

void RenderWidgetHostViewQt::SetPreferredSize(const gfx::Size& size)
{
  if(host_)
    host_->SetPreferredSize(size);
}

gfx::NativeView RenderWidgetHostViewQt::GetNativeView() {
  return view_;
}

void RenderWidgetHostViewQt::MovePluginWindows(
    const std::vector<webkit::npapi::WebPluginGeometry>& moves) {
#if defined(MEEGO_ENABLE_WINDOWED_PLUGIN)
  if (!view_)
    return;

#if !defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
  QPointF offset = view_->scenePos();
  scene_pos_ = gfx::Point(int(offset.x()), int(offset.y()));
#endif

  for (size_t i = 0; i < moves.size(); ++i) {
    plugin_container_manager_->MovePluginContainer(moves[i], scene_pos_);
  }
#endif
}

gfx::Size RenderWidgetHostViewQt::CalFSWinSize() {

  QSize resolution;

  if (view_ && view_->scene()) {
    QWidget *root_win = view_->scene()->views().at(0);
    resolution = root_win->size();
  } else {
    resolution = qApp->desktop()->size();
  }

  return gfx::Size(resolution.width(), resolution.height());
}


gfx::Size RenderWidgetHostViewQt::CalPluginWindowSize() {

  gfx::Size pw_size(0, 0);
#if defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
  int reserved_width = 0;
  int reserved_height = plugin_container_manager_->FSPluginCloseBarHeight();

  gfx::Size resolution;

  resolution = CalFSWinSize();

  pw_size = gfx::Size(resolution.width() - reserved_width, resolution.height() - reserved_height);
  return pw_size;
#endif
}

gfx::Size RenderWidgetHostViewQt::GetFSPluginWindowSize() {
  return CalPluginWindowSize();
}

void RenderWidgetHostViewQt::SetPluginWindowSize() {
#if defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
  if (!host_)
      return;

  gfx::Size pw_size = CalPluginWindowSize();
  host_->SetFSPluginWinSize(pw_size);
#endif
}

void RenderWidgetHostViewQt::OnCloseFSPluginWindow(gfx::PluginWindowHandle id) {
  if (!host_)
    return;
  host_->ResetPlugin(id);
}

void RenderWidgetHostViewQt::ScenePosChanged() {

#if defined(MEEGO_FORCE_FULLSCREEN_PLUGIN)
  //ignore scene pos change since we don't use it
  //when we force windowed plugin into fullscreen
  return;
#endif

#if defined(MEEGO_ENABLE_WINDOWED_PLUGIN)
  if (!view_)
      return;

// When hidden, ScenePos should not been changed. If it did, it must caused
// by tab switching. under this case, we should not relocate plugin window.
  if (is_hidden_)
    return;

  QPointF offset = view_->scenePos();
  if ( (scene_pos_.x() == int(offset.x())) && (scene_pos_.y() == int(offset.y())) )
      return;

  scene_pos_ = gfx::Point(int(offset.x()), int(offset.y()));
  plugin_container_manager_->RelocatePluginContainers(scene_pos_);

#endif
}

void RenderWidgetHostViewQt::Focus() {
  if(view_)
    view_->setFocus();
}

void RenderWidgetHostViewQt::Blur() {
  if (host_)
    host_->Blur();
}

bool RenderWidgetHostViewQt::HasFocus() {
  if(view_)
    return view_->hasFocus();
}

void RenderWidgetHostViewQt::Show() {
  if(view_)
    view_->show();

  plugin_container_manager_->Show();
}

void RenderWidgetHostViewQt::Hide() {
  plugin_container_manager_->Hide();

  if(view_)
    view_->hide();
}

bool RenderWidgetHostViewQt::IsShowing() {
  if(view_)
    return view_->isVisible();
}

gfx::Rect RenderWidgetHostViewQt::GetViewBounds() const {
  QRectF rect;
  if(view_)
    QRectF rect = view_->boundingRect();
    
  return gfx::Rect(int(rect.x()), int(rect.y()),
                   requested_size_.width(),
                   requested_size_.height());
}

void RenderWidgetHostViewQt::SetScrollPosition(const gfx::Point& pos)
{
  if(view_)
  {
    reinterpret_cast<RWHVQtWidget*>(view_)->SetScrollPosition(pos);
  }
}

void RenderWidgetHostViewQt::UpdateCursor(const WebCursor& cursor) {
  ///\todo Fixme
}

void RenderWidgetHostViewQt::SetIsLoading(bool is_loading) {
  is_loading_ = is_loading;
}

void RenderWidgetHostViewQt::ImeUpdateTextInputState(WebKit::WebTextInputType type,
    const gfx::Rect& caret_rect) {
  if(view_)
    reinterpret_cast<RWHVQtWidget*>(view_)->imeUpdateTextInputState(type, caret_rect);
}


void RenderWidgetHostViewQt::ImeCancelComposition() {
  if(view_)
    reinterpret_cast<RWHVQtWidget*>(view_)->imeCancelComposition();
}

void RenderWidgetHostViewQt::DidUpdateBackingStore(
    const gfx::Rect& scroll_rect, int scroll_dx, int scroll_dy,
    const std::vector<gfx::Rect>& copy_rects) {
  if (is_hidden_)
    return;

  // Let tiled backing store to schedule update
#if !defined(TILED_BACKING_STORE)
  // TODO(darin): Implement the equivalent of Win32's ScrollWindowEX.  Can that
  // be done using XCopyArea?  Perhaps similar to
  // BackingStore::ScrollBackingStore?
  if (about_to_validate_and_paint_)
    invalid_rect_ = invalid_rect_.Union(scroll_rect);
  else
    Paint(scroll_rect);

  for (size_t i = 0; i < copy_rects.size(); ++i) {
    // Avoid double painting.  NOTE: This is only relevant given the call to
    // Paint(scroll_rect) above.
    gfx::Rect rect = copy_rects[i].Subtract(scroll_rect);
    if (rect.IsEmpty())
      continue;

    if (about_to_validate_and_paint_)
      invalid_rect_ = invalid_rect_.Union(rect);
    else
      Paint(rect);
  }
#endif
}

void RenderWidgetHostViewQt::RenderViewGone(base::TerminationStatus status,
                                             int error_code) {
  Destroy();
#if defined(MEEGO_ENABLE_WINDOWED_PLUGIN)
  //TODO: plugin
  plugin_container_manager_->set_host_widget(NULL);
#endif
}

void RenderWidgetHostViewQt::UpdateWebKitNodeInfo(unsigned int node_info)
{
  webkit_node_info_ = node_info;
}

void RenderWidgetHostViewQt::UpdateSelectionRange(gfx::Point start,
    gfx::Point end, int height, bool set) {
  if(view_)
      reinterpret_cast<RWHVQtWidget*>(view_)->UpdateSelectionRange(start, end, height, set);
}

void RenderWidgetHostViewQt::Destroy() {  
  if(view_) {
    view_->setParentItem(NULL);
    delete view_;
    view_ = NULL;
  }
  // The RenderWidgetHost's destruction led here, so don't call it.
  host_ = NULL;

  MessageLoop::current()->DeleteSoon(FROM_HERE, this);
}

void RenderWidgetHostViewQt::SetTooltipText(const std::wstring& tooltip_text) {

  const int kMaxTooltipLength = 8 << 10;

  const string16& clamped_tooltip =
      l10n_util::TruncateString(WideToUTF16Hack(tooltip_text),
                                kMaxTooltipLength);

  ///\todo
  DNOTIMPLEMENTED();
}

void RenderWidgetHostViewQt::SelectionChanged(const std::string& text) {
  //GtkClipboard* x_clipboard = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
  //gtk_clipboard_set_text(x_clipboard, text.c_str(), text.length());
  ///\todo DNOTIMPLEMENTED();
}

void RenderWidgetHostViewQt::ShowingContextMenu(bool showing) {
  is_showing_context_menu_ = showing;
}

bool RenderWidgetHostViewQt::NeedsInputGrab() {
  return popup_type_ == WebKit::WebPopupTypeSelect;
}

bool RenderWidgetHostViewQt::IsPopup() {
  return popup_type_ != WebKit::WebPopupTypeNone;
}

BackingStore* RenderWidgetHostViewQt::AllocBackingStore(
    const gfx::Size& size) {
  DLOG(INFO) << "AllocBackingStore size " << size.width() << " " << size.height();

  BackingStoreX* backing_store = new BackingStoreX(host_, size,
                           QApplication::desktop()->x11Info().visual(),
                           QApplication::desktop()->x11Info().depth());
#if defined(TILED_BACKING_STORE)
  if(backing_store && IsPopup()) {
    backing_store->SetContentsScale(host_->GetScaleFactor());
  }
#endif
  return backing_store;
}

void RenderWidgetHostViewQt::SetBackground(const SkBitmap& background) {
  RenderWidgetHostView::SetBackground(background);
  host_->Send(new ViewMsg_SetBackground(host_->routing_id(), background));
}

void RenderWidgetHostViewQt::Paint(const gfx::Rect& damage_rect) {
  DCHECK(!about_to_validate_and_paint_);

  invalid_rect_ = damage_rect;
  about_to_validate_and_paint_ = true;

  // Is there really any case that this get called before view_ is created?
  if (view_)
    view_->update(invalid_rect_.x(), invalid_rect_.y(),
        invalid_rect_.width(), invalid_rect_.height());
}

void RenderWidgetHostViewQt::ShowCurrentCursor() {
  ///\todo DNOTIMPLEMENTED();
}

void RenderWidgetHostViewQt::CreatePluginContainer(
    gfx::PluginWindowHandle id) {
#if defined(MEEGO_ENABLE_WINDOWED_PLUGIN)
  if(!view_)
    return;
  if(view_->scene())
    plugin_container_manager_->set_host_widget(view_->scene()->views().at(0));
  LOG(ERROR) << "view scene " << view_->scene();

  plugin_container_manager_->SetFSWindowSize(CalFSWinSize());
  plugin_container_manager_->CreatePluginContainer(id);
#endif
}

void RenderWidgetHostViewQt::DestroyPluginContainer(
    gfx::PluginWindowHandle id) {
#if defined(MEEGO_ENABLE_WINDOWED_PLUGIN)
  plugin_container_manager_->DestroyPluginContainer(id);
#endif
}

void RenderWidgetHostViewQt::SetVisuallyDeemphasized(
    const SkColor* color, bool animate) {
  DNOTIMPLEMENTED();
}

bool RenderWidgetHostViewQt::ContainsNativeView(
    gfx::NativeView native_view) const {
  // TODO(port)
  NOTREACHED() <<
    "RenderWidgetHostViewQt::ContainsNativeView not implemented.";
  return false;
}

void RenderWidgetHostViewQt::ForwardKeyboardEvent(
    const NativeWebKeyboardEvent& event) {
  if (!host_)
    return;

  host_->ForwardKeyboardEvent(event);
}

// static
RenderWidgetHostView*
    RenderWidgetHostView::GetRenderWidgetHostViewFromNativeView(
        gfx::NativeView widget) {
  return reinterpret_cast<RWHVQtWidget*>(widget)->hostView();
}

// Called on receiving ViewHostMsg_RequestMove IPC from
// render process. 
void RenderWidgetHostViewQt::SetBounds(const gfx::Rect& pos)
{
  if (!IsPopup()) return;
  
  // If the number of the suggestion row changes, the bound 
  // of the widgetshowing popup menu should be also changed.
  // Render process will send out RequestMove IPC to browser
  // process
  requested_size_ = gfx::Size(std::min(pos.width(), kMaxWindowWidth),
                     std::min(pos.height(), kMaxWindowHeight));

  // Adjust RWHVQtWidget size accordint to the given bounds
  RWHVQtWidget* rwhv = static_cast<RWHVQtWidget*>(view_);
  // Do we need to a DCHECK?
  if (!rwhv) return;

  qreal scale = rwhv->scale();
  QRect geometry(pos.x()*scale, pos.y()*scale, 
                 requested_size_.width()*scale, 
                 requested_size_.height()*scale);
  rwhv->setGeometry(geometry);
  rwhv->show();
  
  host_->WasResized();
  return;
}

void RenderWidgetHostViewQt::UpdateContentsSize(const gfx::Size& size)
{
  if (contents_size_ != size) {
    // a flag whether width is changed
    bool width_changed = (contents_size_.width() != size.width());
    contents_size_ = size;
    if (view_)
      reinterpret_cast<RWHVQtWidget*>(view_)->AdjustSize();
    BackingStoreX* backing_store = static_cast<BackingStoreX*>(
      host_->GetBackingStore(false));
#if defined(TILED_BACKING_STORE)
    if (backing_store)
    {
      // if width is not changed, we have the chance to reuse some
      // tiles to avoid checker painting many times
      // here assumes height changed won't adjust relayout and won't be
      // repaint for existing tiles. This can save rendering time and improve
      // performance. If we find height changes the existing tiles, we'll
      // add a repaint later
      backing_store->AdjustTiles(width_changed);
      DLOG(INFO) << __PRETTY_FUNCTION__ << "adjust tiles: " << width_changed;
    }
#endif
  }
}

void RenderWidgetHostViewQt::PaintTileAck(unsigned int seq, unsigned int tag, const gfx::Rect& rect, const gfx::Rect& pixmap_rect)
 {
  QRect qrect(rect.x(), rect.y(), rect.width(), rect.height());
  QRect qpixmap_rect(pixmap_rect.x(), pixmap_rect.y(), pixmap_rect.width(), pixmap_rect.height());
  BackingStoreX* backing_store = static_cast<BackingStoreX*>(
      host_->GetBackingStore(false));
#if defined(TILED_BACKING_STORE)
  if (backing_store)
  {
    backing_store->PaintTilesAck(seq, tag, qrect, qpixmap_rect);
  }
#endif
}

gfx::Size RenderWidgetHostViewQt::GetContentsSize()
{
  return contents_size_;
}

gfx::Rect RenderWidgetHostViewQt::GetVisibleRect()
{
  QRect qrect;
  if (view_)
    qrect = reinterpret_cast<RWHVQtWidget*>(view_)->GetVisibleRect();
 
  gfx::Rect rect(qrect.x(), qrect.y(), qrect.width(), qrect.height());
  DLOG(INFO) << "RenderWidgetHostViewQt::GetVisibleRect"
             << " " << rect.x()
             << " " << rect.y()
             << " " << rect.width()
             << " " << rect.height();
   
  return rect;
}

void RenderWidgetHostViewQt::DidBackingStoreScale()
{
  if (view_)
  {
    view_->setScale(1.0);
    reinterpret_cast<RWHVQtWidget*>(view_)->DidBackingStoreScale();
  }
}

void RenderWidgetHostViewQt::DidBackingStorePaint(const gfx::Rect& rect)
{
  if (view_)
  {
    view_->update(rect.x(), rect.y(), rect.width(), rect.height());
  }
}
