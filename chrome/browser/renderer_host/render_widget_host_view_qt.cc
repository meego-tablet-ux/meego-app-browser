// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//#include "chrome/browser/renderer_host/render_widget_host_view_gtk.h"

// If this gets included after the gtk headers, then a bunch of compiler
// errors happen because of a "#define Status int" in Xlib.h, which interacts
// badly with URLRequestStatus::Status.
#include "chrome/common/render_messages.h"

#include <QtGui/QApplication>
#include <QtGui/QCursor>
#include <QtGui/QInputContext>
#include <QtGui/QDesktopWidget>
#include <QtGui/QX11Info>
#include <QGraphicsLinearLayout>

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
#include "webkit/glue/webmenuitem.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"
#include "chrome/browser/ui/meegotouch/popup_list_qt.h"

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
      popup_(NULL),
      is_popup_first_mouse_release_(true),
      was_focused_before_grab_(false),
      do_x_grab_(false),
      webkit_node_info_(0),
      view_(NULL) {
  host_->set_view(this);
}

RenderWidgetHostViewQt::~RenderWidgetHostViewQt() {
}

void RenderWidgetHostViewQt::InitAsChild() {
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

  popup_ = new QGraphicsWidget(parent_host_view_->GetNativeView());
  QGraphicsLinearLayout *layout_ = new QGraphicsLinearLayout(Qt::Vertical);

  layout_->setContentsMargins(0, 0, 0, 0);
  layout_->setSpacing(0.0);
  popup_->setLayout(layout_);

  view_ = new RWHVQtWidget(this);
  layout_->addItem(view_);

  requested_size_ = gfx::Size(std::min(pos.width(), kMaxWindowWidth),
                              std::min(pos.height(), kMaxWindowHeight));
  QRect geometry(pos.x(), pos.y(), requested_size_.width(), requested_size_.height());
  popup_->setMinimumSize(requested_size_.width(), requested_size_.height());
  popup_->setMaximumSize(requested_size_.width(), requested_size_.height());
  popup_->setGeometry(geometry);

  host_->WasResized();
  popup_->show();

}

void RenderWidgetHostViewQt::DidBecomeSelected() {
  if (!is_hidden_)
    return;

  if (tab_switch_paint_time_.is_null())
    tab_switch_paint_time_ = base::TimeTicks::Now();
  is_hidden_ = false;
  host_->WasRestored();
}

void RenderWidgetHostViewQt::WasHidden() {
  if (is_hidden_)
    return;

  // If we receive any more paint messages while we are hidden, we want to
  // ignore them so we don't re-allocate the backing store.  We will paint
  // everything again when we become selected again.
  is_hidden_ = true;

  // If we have a renderer, then inform it that we are being hidden so it can
  // reduce its resource utilization.
  GetRenderWidgetHost()->WasHidden();
}

void RenderWidgetHostViewQt::SetSize(const gfx::Size& size) {

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
}

gfx::NativeView RenderWidgetHostViewQt::GetNativeView() {
  return view_;
}

void RenderWidgetHostViewQt::MovePluginWindows(
    const std::vector<webkit::npapi::WebPluginGeometry>& moves) {
#if defined(MEEGO_ENABLE_WINDOWED_PLUGIN)
  if (!view_)
    return;
  QPointF offset = view_->scenePos();
  gfx::Point view_offset(int(offset.x()), int(offset.y()));
  for (size_t i = 0; i < moves.size(); ++i) {
    plugin_container_manager_.MovePluginContainer(moves[i], view_offset);
  }
#endif
}

void RenderWidgetHostViewQt::Focus() {
  if(view_)
    view_->setFocus();
}

void RenderWidgetHostViewQt::Blur() {
  host_->Blur();
}

bool RenderWidgetHostViewQt::HasFocus() {
  if(view_)
    return view_->hasFocus();
}

void RenderWidgetHostViewQt::Show() {
  if(view_)
    view_->show();
}

void RenderWidgetHostViewQt::Hide() {
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
}

void RenderWidgetHostViewQt::RenderViewGone(base::TerminationStatus status,
                                             int error_code) {
  Destroy();
#if defined(MEEGO_ENABLE_WINDOWED_PLUGIN)
  //TODO: plugin
  plugin_container_manager_.set_host_widget(NULL);
#endif
}

void RenderWidgetHostViewQt::UpdateWebKitNodeInfo(bool is_embedded_object,
    bool is_content_editable) {
  webkit_node_info_ = NODE_INFO_NONE;

  if (is_embedded_object)
    webkit_node_info_ |= NODE_INFO_IS_EMBEDDED_OBJECT;

  if (is_content_editable)
    webkit_node_info_ |= NODE_INFO_IS_EDITABLE;
}

void RenderWidgetHostViewQt::UpdateSelectionRange(gfx::Point start,
    gfx::Point end, bool set) {
  if(view_)
    reinterpret_cast<RWHVQtWidget*>(view_)->UpdateSelectionRange(start, end, set);
}

void RenderWidgetHostViewQt::Destroy() {
  ///\todo Fixme, can we delete view here???
  if (IsPopup()) {
    if (popup_) {
      delete popup_;
      popup_ = NULL;
    }
  } else  if(view_) {
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

void RenderWidgetHostViewQt::ShowPopupWithItems(gfx::Rect bounds, 
                                            int item_height,
                                            double item_font_size,
                                            int selected_item,
                                            const std::vector<WebMenuItem>& items,
                                            bool right_aligned)
{
  std::vector<WebMenuItem>::const_iterator iter;
  
  for(iter = items.begin(); iter != items.end(); ++iter)  {
    DLOG(INFO) << ">> " << (*iter).label;
  }

  Browser* browser = BrowserList::GetLastActive();
  BrowserWindowQt* browser_window = (BrowserWindowQt*)browser->window();

  PopupListQt* popup_list = browser_window->GetWebPopupList();
  popup_list->PopulateMenuItemData(selected_item, items);
  popup_list->SetHeaderBounds(bounds);
  popup_list->setCurrentView(this);
  popup_list->show();
}

void RenderWidgetHostViewQt::selectPopupItem(int index)
{
  if (index >= 0) {
    host_->DidSelectPopupMenuItem(index);
  } else if (index == -1) {
    host_->DidCancelPopupMenu();
  } else {
    DLOG(ERROR) << "Invalid Index";
  }

}

bool RenderWidgetHostViewQt::NeedsInputGrab() {
  return popup_type_ == WebKit::WebPopupTypeSelect;
}

bool RenderWidgetHostViewQt::IsPopup() {
  return popup_type_ != WebKit::WebPopupTypeNone;
}

BackingStore* RenderWidgetHostViewQt::AllocBackingStore(
    const gfx::Size& size) {
  return new BackingStoreX(host_, size,
                           QApplication::desktop()->x11Info().visual(),
                           QApplication::desktop()->x11Info().depth());
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
    plugin_container_manager_.set_host_widget(view_->scene()->views().at(0));
  LOG(ERROR) << "view scene " << view_->scene();

  plugin_container_manager_.CreatePluginContainer(id);
#endif
}

void RenderWidgetHostViewQt::DestroyPluginContainer(
    gfx::PluginWindowHandle id) {
#if defined(MEEGO_ENABLE_WINDOWED_PLUGIN)
  plugin_container_manager_.DestroyPluginContainer(id);
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

#include "moc_render_widget_host_view_qt.cc"
