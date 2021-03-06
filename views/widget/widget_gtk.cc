// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "views/widget/widget_gtk.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/extensions/shape.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include <set>
#include <vector>

#include "base/auto_reset.h"
#include "base/compiler_specific.h"
#include "base/message_loop.h"
#include "base/utf_string_conversions.h"
#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "ui/base/dragdrop/os_exchange_data_provider_gtk.h"
#include "ui/base/gtk/gtk_windowing.h"
#include "ui/base/x/x11_util.h"
#include "ui/gfx/canvas_skia_paint.h"
#include "ui/gfx/path.h"
#include "views/views_delegate.h"
#include "views/controls/textfield/native_textfield_views.h"
#include "views/focus/view_storage.h"
#include "views/widget/drop_target_gtk.h"
#include "views/widget/gtk_views_fixed.h"
#include "views/widget/gtk_views_window.h"
#include "views/widget/root_view.h"
#include "views/widget/tooltip_manager_gtk.h"
#include "views/widget/widget_delegate.h"
#include "views/window/window_gtk.h"

#if defined(TOUCH_UI)
#if defined(HAVE_XINPUT2)
#include <gdk/gdkx.h>

#include "ui/gfx/gtk_util.h"
#include "views/touchui/touch_factory.h"
#endif
#endif

#if defined(TOUCH_UI) && defined(HAVE_IBUS)
#include "views/ime/input_method_ibus.h"
#else
#include "views/ime/input_method_gtk.h"
#endif

using ui::OSExchangeData;
using ui::OSExchangeDataProviderGtk;
using ui::ActiveWindowWatcherX;

namespace views {

namespace {

// Links the GtkWidget to its NativeWidget.
const char* const kNativeWidgetKey = "__VIEWS_NATIVE_WIDGET__";

// A g_object data key to associate a CompositePainter object to a GtkWidget.
const char* kCompositePainterKey = "__VIEWS_COMPOSITE_PAINTER__";

// A g_object data key to associate the flag whether or not the widget
// is composited to a GtkWidget. gtk_widget_is_composited simply tells
// if x11 supports composition and cannot be used to tell if given widget
// is composited.
const char* kCompositeEnabledKey = "__VIEWS_COMPOSITE_ENABLED__";

// A g_object data key to associate the expose handler id that is
// used to remove FREEZE_UPDATE property on the window.
const char* kExposeHandlerIdKey = "__VIEWS_EXPOSE_HANDLER_ID__";

// CompositePainter draws a composited child widgets image into its
// drawing area. This object is created at most once for a widget and kept
// until the widget is destroyed.
class CompositePainter {
 public:
  explicit CompositePainter(GtkWidget* parent)
      : parent_object_(G_OBJECT(parent)) {
    handler_id_ = g_signal_connect_after(
        parent_object_, "expose_event", G_CALLBACK(OnCompositePaint), NULL);
  }

  static void AddCompositePainter(GtkWidget* widget) {
    CompositePainter* painter = static_cast<CompositePainter*>(
        g_object_get_data(G_OBJECT(widget), kCompositePainterKey));
    if (!painter) {
      g_object_set_data(G_OBJECT(widget), kCompositePainterKey,
                        new CompositePainter(widget));
      g_signal_connect(widget, "destroy",
                       G_CALLBACK(&DestroyPainter), NULL);
    }
  }

  // Set the composition flag.
  static void SetComposited(GtkWidget* widget) {
    g_object_set_data(G_OBJECT(widget), kCompositeEnabledKey,
                      const_cast<char*>(""));
  }

  // Returns true if the |widget| is composited and ready to be drawn.
  static bool IsComposited(GtkWidget* widget) {
    return g_object_get_data(G_OBJECT(widget), kCompositeEnabledKey) != NULL;
  }

 private:
  virtual ~CompositePainter() {}

  // Composes a image from one child.
  static void CompositeChildWidget(GtkWidget* child, gpointer data) {
    GdkEventExpose* event = static_cast<GdkEventExpose*>(data);
    GtkWidget* parent = gtk_widget_get_parent(child);
    DCHECK(parent);
    if (IsComposited(child)) {
      cairo_t* cr = gdk_cairo_create(parent->window);
      gdk_cairo_set_source_pixmap(cr, child->window,
                                  child->allocation.x,
                                  child->allocation.y);
      GdkRegion* region = gdk_region_rectangle(&child->allocation);
      gdk_region_intersect(region, event->region);
      gdk_cairo_region(cr, region);
      cairo_clip(cr);
      cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
      cairo_paint(cr);
      cairo_destroy(cr);
    }
  }

  // Expose-event handler that compose & draws children's image into
  // the |parent|'s drawing area.
  static gboolean OnCompositePaint(GtkWidget* parent, GdkEventExpose* event) {
    gtk_container_foreach(GTK_CONTAINER(parent),
                          CompositeChildWidget,
                          event);
    return false;
  }

  static void DestroyPainter(GtkWidget* object) {
    CompositePainter* painter = reinterpret_cast<CompositePainter*>(
        g_object_get_data(G_OBJECT(object), kCompositePainterKey));
    DCHECK(painter);
    delete painter;
  }

  GObject* parent_object_;
  gulong handler_id_;

  DISALLOW_COPY_AND_ASSIGN(CompositePainter);
};

void EnumerateChildWidgetsForNativeWidgets(GtkWidget* child_widget,
                                           gpointer param) {
  // Walk child widgets, if necessary.
  if (GTK_IS_CONTAINER(child_widget)) {
    gtk_container_foreach(GTK_CONTAINER(child_widget),
                          EnumerateChildWidgetsForNativeWidgets,
                          param);
  }

  NativeWidget* native_widget =
      NativeWidget::GetNativeWidgetForNativeView(child_widget);
  if (native_widget) {
    NativeWidget::NativeWidgets* widgets =
        reinterpret_cast<NativeWidget::NativeWidgets*>(param);
    widgets->insert(native_widget);
  }
}

void RemoveExposeHandlerIfExists(GtkWidget* widget) {
  gulong id = reinterpret_cast<gulong>(g_object_get_data(G_OBJECT(widget),
                                                         kExposeHandlerIdKey));
  if (id) {
    g_signal_handler_disconnect(G_OBJECT(widget), id);
    g_object_set_data(G_OBJECT(widget), kExposeHandlerIdKey, 0);
  }
}

}  // namespace

// During drag and drop GTK sends a drag-leave during a drop. This means we
// have no way to tell the difference between a normal drag leave and a drop.
// To work around that we listen for DROP_START, then ignore the subsequent
// drag-leave that GTK generates.
class WidgetGtk::DropObserver : public MessageLoopForUI::Observer {
 public:
  DropObserver() {}

  static DropObserver* GetInstance() {
    return Singleton<DropObserver>::get();
  }

  virtual void WillProcessEvent(GdkEvent* event) {
    if (event->type == GDK_DROP_START) {
      WidgetGtk* widget = GetWidgetGtkForEvent(event);
      if (widget)
        widget->ignore_drag_leave_ = true;
    }
  }

  virtual void DidProcessEvent(GdkEvent* event) {
  }

 private:
  WidgetGtk* GetWidgetGtkForEvent(GdkEvent* event) {
    GtkWidget* gtk_widget = gtk_get_event_widget(event);
    if (!gtk_widget)
      return NULL;

    return static_cast<WidgetGtk*>(
        NativeWidget::GetNativeWidgetForNativeView(gtk_widget));
  }

  DISALLOW_COPY_AND_ASSIGN(DropObserver);
};

// Returns the position of a widget on screen.
static void GetWidgetPositionOnScreen(GtkWidget* widget, int* x, int *y) {
  // First get the root window.
  GtkWidget* root = widget;
  while (root && !GTK_IS_WINDOW(root)) {
    root = gtk_widget_get_parent(root);
  }
  if (!root) {
    // If root is null we're not parented. Return 0x0 and assume the caller will
    // query again when we're parented.
    *x = *y = 0;
    return;
  }
  // Translate the coordinate from widget to root window.
  gtk_widget_translate_coordinates(widget, root, 0, 0, x, y);
  // Then adjust the position with the position of the root window.
  int window_x, window_y;
  gtk_window_get_position(GTK_WINDOW(root), &window_x, &window_y);
  *x += window_x;
  *y += window_y;
}

// "expose-event" handler of drag icon widget that renders drag image pixbuf.
static gboolean DragIconWidgetPaint(GtkWidget* widget,
                                    GdkEventExpose* event,
                                    gpointer data) {
  GdkPixbuf* pixbuf = reinterpret_cast<GdkPixbuf*>(data);

  cairo_t* cr = gdk_cairo_create(widget->window);

  gdk_cairo_region(cr, event->region);
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  gdk_cairo_set_source_pixbuf(cr, pixbuf, 0.0, 0.0);
  cairo_paint(cr);

  cairo_destroy(cr);
  return true;
}

// Creates a drag icon widget that draws drag_image.
static GtkWidget* CreateDragIconWidget(GdkPixbuf* drag_image) {
  GdkColormap* rgba_colormap =
      gdk_screen_get_rgba_colormap(gdk_screen_get_default());
  if (!rgba_colormap)
    return NULL;

  GtkWidget* drag_widget = gtk_window_new(GTK_WINDOW_POPUP);

  gtk_widget_set_colormap(drag_widget, rgba_colormap);
  gtk_widget_set_app_paintable(drag_widget, true);
  gtk_widget_set_size_request(drag_widget,
                              gdk_pixbuf_get_width(drag_image),
                              gdk_pixbuf_get_height(drag_image));

  g_signal_connect(G_OBJECT(drag_widget), "expose-event",
                   G_CALLBACK(&DragIconWidgetPaint), drag_image);
  return drag_widget;
}

// static
GtkWidget* WidgetGtk::null_parent_ = NULL;
bool WidgetGtk::debug_paint_enabled_ = false;

////////////////////////////////////////////////////////////////////////////////
// WidgetGtk, public:

WidgetGtk::WidgetGtk(Type type)
    : is_window_(false),
      ALLOW_THIS_IN_INITIALIZER_LIST(delegate_(this)),
      type_(type),
      widget_(NULL),
      window_contents_(NULL),
      ALLOW_THIS_IN_INITIALIZER_LIST(close_widget_factory_(this)),
      delete_on_destroy_(true),
      transparent_(false),
      ignore_events_(false),
      ignore_drag_leave_(false),
      opacity_(255),
      drag_data_(NULL),
      is_active_(false),
      transient_to_parent_(false),
      got_initial_focus_in_(false),
      has_focus_(false),
      always_on_top_(false),
      is_double_buffered_(false),
      should_handle_menu_key_release_(false),
      dragged_view_(NULL),
      painted_(false) {
  set_native_widget(this);
  static bool installed_message_loop_observer = false;
  if (!installed_message_loop_observer) {
    installed_message_loop_observer = true;
    MessageLoopForUI* loop = MessageLoopForUI::current();
    if (loop)
      loop->AddObserver(DropObserver::GetInstance());
  }
}

WidgetGtk::~WidgetGtk() {
  // We need to delete the input method before calling DestroyRootView(),
  // because it'll set focus_manager_ to NULL.
  input_method_.reset();
  DestroyRootView();
  DCHECK(delete_on_destroy_ || widget_ == NULL);
  if (type_ != TYPE_CHILD)
    ActiveWindowWatcherX::RemoveObserver(this);
}

GtkWindow* WidgetGtk::GetTransientParent() const {
  return (type_ != TYPE_CHILD && widget_) ?
      gtk_window_get_transient_for(GTK_WINDOW(widget_)) : NULL;
}

bool WidgetGtk::MakeTransparent() {
  // Transparency can only be enabled only if we haven't realized the widget.
  DCHECK(!widget_);

  if (!gdk_screen_is_composited(gdk_screen_get_default())) {
    // Transparency is only supported for compositing window managers.
    // NOTE: there's a race during ChromeOS startup such that X might think
    // compositing isn't supported. We ignore it if the wm says compositing
    // isn't supported.
    DLOG(WARNING) << "compositing not supported; allowing anyway";
  }

  if (!gdk_screen_get_rgba_colormap(gdk_screen_get_default())) {
    // We need rgba to make the window transparent.
    return false;
  }

  transparent_ = true;
  return true;
}

void WidgetGtk::EnableDoubleBuffer(bool enabled) {
  is_double_buffered_ = enabled;
  if (window_contents_) {
    if (is_double_buffered_)
      GTK_WIDGET_SET_FLAGS(window_contents_, GTK_DOUBLE_BUFFERED);
    else
      GTK_WIDGET_UNSET_FLAGS(window_contents_, GTK_DOUBLE_BUFFERED);
  }
}

bool WidgetGtk::MakeIgnoreEvents() {
  // Transparency can only be enabled for windows/popups and only if we haven't
  // realized the widget.
  DCHECK(!widget_ && type_ != TYPE_CHILD);

  ignore_events_ = true;
  return true;
}

void WidgetGtk::AddChild(GtkWidget* child) {
  gtk_container_add(GTK_CONTAINER(window_contents_), child);
}

void WidgetGtk::RemoveChild(GtkWidget* child) {
  // We can be called after the contents widget has been destroyed, e.g. any
  // NativeViewHost not removed from the view hierarchy before the window is
  // closed.
  if (GTK_IS_CONTAINER(window_contents_)) {
    gtk_container_remove(GTK_CONTAINER(window_contents_), child);
    gtk_views_fixed_set_widget_size(child, 0, 0);
  }
}

void WidgetGtk::ReparentChild(GtkWidget* child) {
  gtk_widget_reparent(child, window_contents_);
}

void WidgetGtk::PositionChild(GtkWidget* child, int x, int y, int w, int h) {
  gtk_views_fixed_set_widget_size(child, w, h);
  gtk_fixed_move(GTK_FIXED(window_contents_), child, x, y);
}

void WidgetGtk::DoDrag(const OSExchangeData& data, int operation) {
  const OSExchangeDataProviderGtk& data_provider =
      static_cast<const OSExchangeDataProviderGtk&>(data.provider());
  GtkTargetList* targets = data_provider.GetTargetList();
  GdkEvent* current_event = gtk_get_current_event();
  const OSExchangeDataProviderGtk& provider(
      static_cast<const OSExchangeDataProviderGtk&>(data.provider()));

  GdkDragContext* context = gtk_drag_begin(
      window_contents_,
      targets,
      static_cast<GdkDragAction>(
          ui::DragDropTypes::DragOperationToGdkDragAction(operation)),
      1,
      current_event);

  GtkWidget* drag_icon_widget = NULL;

  // Set the drag image if one was supplied.
  if (provider.drag_image()) {
    drag_icon_widget = CreateDragIconWidget(provider.drag_image());
    if (drag_icon_widget) {
      // Use a widget as the drag icon when compositing is enabled for proper
      // transparency handling.
      g_object_ref(provider.drag_image());
      gtk_drag_set_icon_widget(context,
                               drag_icon_widget,
                               provider.cursor_offset().x(),
                               provider.cursor_offset().y());
    } else {
      gtk_drag_set_icon_pixbuf(context,
                               provider.drag_image(),
                               provider.cursor_offset().x(),
                               provider.cursor_offset().y());
    }
  }

  if (current_event)
    gdk_event_free(current_event);
  gtk_target_list_unref(targets);

  drag_data_ = &data_provider;

  // Block the caller until drag is done by running a nested message loop.
  MessageLoopForUI::current()->Run(NULL);

  drag_data_ = NULL;

  if (drag_icon_widget) {
    gtk_widget_destroy(drag_icon_widget);
    g_object_unref(provider.drag_image());
  }
}

void WidgetGtk::IsActiveChanged() {
  if (widget_delegate())
    widget_delegate()->OnWidgetActivated(IsActive());
}

void WidgetGtk::ResetDropTarget() {
  ignore_drag_leave_ = false;
  drop_target_.reset(NULL);
}

void WidgetGtk::GetRequestedSize(gfx::Size* out) const {
  int width, height;
  if (GTK_IS_VIEWS_FIXED(widget_) &&
      gtk_views_fixed_get_widget_size(GetNativeView(), &width, &height)) {
    out->SetSize(width, height);
  } else {
    GtkRequisition requisition;
    gtk_widget_get_child_requisition(GetNativeView(), &requisition);
    out->SetSize(requisition.width, requisition.height);
  }
}

////////////////////////////////////////////////////////////////////////////////
// WidgetGtk, ActiveWindowWatcherX::Observer implementation:

void WidgetGtk::ActiveWindowChanged(GdkWindow* active_window) {
  if (!GetNativeView())
    return;

  bool was_active = IsActive();
  is_active_ = (active_window == GTK_WIDGET(GetNativeView())->window);
  if (!is_active_ && active_window && type_ != TYPE_CHILD) {
    // We're not active, but the force the window to be rendered as active if
    // a child window is transient to us.
    gpointer data = NULL;
    gdk_window_get_user_data(active_window, &data);
    GtkWidget* widget = reinterpret_cast<GtkWidget*>(data);
    is_active_ =
        (widget && GTK_IS_WINDOW(widget) &&
         gtk_window_get_transient_for(GTK_WINDOW(widget)) == GTK_WINDOW(
             widget_));
  }
  if (was_active != IsActive()) {
    IsActiveChanged();
    GetRootView()->SchedulePaint();
  }
}

////////////////////////////////////////////////////////////////////////////////
// WidgetGtk, Widget implementation:

void WidgetGtk::InitWithWidget(Widget* parent,
                               const gfx::Rect& bounds) {
  WidgetGtk* parent_gtk = static_cast<WidgetGtk*>(parent);
  GtkWidget* native_parent = NULL;
  if (parent != NULL) {
    if (type_ != TYPE_CHILD) {
      // window's parent has to be window.
      native_parent = parent_gtk->GetNativeView();
    } else {
      native_parent = parent_gtk->window_contents();
    }
  }
  Init(native_parent, bounds);
}

void WidgetGtk::Init(GtkWidget* parent,
                     const gfx::Rect& bounds) {
  Widget::Init(parent, bounds);
  if (type_ != TYPE_CHILD)
    ActiveWindowWatcherX::AddObserver(this);

  // Make container here.
  CreateGtkWidget(parent, bounds);
  delegate_->OnNativeWidgetCreated();

  // Creates input method for toplevel widget after calling
  // delegate_->OnNativeWidgetCreated(), to make sure that focus manager is
  // already created at this point.
  // TODO(suzhe): Always enable input method when we start to use
  // RenderWidgetHostViewViews in normal ChromeOS.
#if defined(TOUCH_UI) && defined(HAVE_IBUS)
  if (type_ != TYPE_CHILD) {
    input_method_.reset(new InputMethodIBus(this));
#else
  if (type_ != TYPE_CHILD && NativeTextfieldViews::IsTextfieldViewsEnabled()) {
    input_method_.reset(new InputMethodGtk(this));
#endif
    input_method_->Init(GetWidget());
  }

  if (opacity_ != 255)
    SetOpacity(opacity_);

  // Make sure we receive our motion events.

  // In general we register most events on the parent of all widgets. At a
  // minimum we need painting to happen on the parent (otherwise painting
  // doesn't work at all), and similarly we need mouse release events on the
  // parent as windows don't get mouse releases.
  gtk_widget_add_events(window_contents_,
                        GDK_ENTER_NOTIFY_MASK |
                        GDK_LEAVE_NOTIFY_MASK |
                        GDK_BUTTON_PRESS_MASK |
                        GDK_BUTTON_RELEASE_MASK |
                        GDK_POINTER_MOTION_MASK |
                        GDK_KEY_PRESS_MASK |
                        GDK_KEY_RELEASE_MASK);

  g_signal_connect_after(G_OBJECT(window_contents_), "size_request",
                         G_CALLBACK(&OnSizeRequestThunk), this);
  g_signal_connect_after(G_OBJECT(window_contents_), "size_allocate",
                         G_CALLBACK(&OnSizeAllocateThunk), this);
  gtk_widget_set_app_paintable(window_contents_, true);
  g_signal_connect(window_contents_, "expose_event",
                   G_CALLBACK(&OnPaintThunk), this);
  g_signal_connect(window_contents_, "enter_notify_event",
                   G_CALLBACK(&OnEnterNotifyThunk), this);
  g_signal_connect(window_contents_, "leave_notify_event",
                   G_CALLBACK(&OnLeaveNotifyThunk), this);
  g_signal_connect(window_contents_, "motion_notify_event",
                   G_CALLBACK(&OnMotionNotifyThunk), this);
  g_signal_connect(window_contents_, "button_press_event",
                   G_CALLBACK(&OnButtonPressThunk), this);
  g_signal_connect(window_contents_, "button_release_event",
                   G_CALLBACK(&OnButtonReleaseThunk), this);
  g_signal_connect(window_contents_, "grab_broken_event",
                   G_CALLBACK(&OnGrabBrokeEventThunk), this);
  g_signal_connect(window_contents_, "grab_notify",
                   G_CALLBACK(&OnGrabNotifyThunk), this);
  g_signal_connect(window_contents_, "scroll_event",
                   G_CALLBACK(&OnScrollThunk), this);
  g_signal_connect(window_contents_, "visibility_notify_event",
                   G_CALLBACK(&OnVisibilityNotifyThunk), this);

  // In order to receive notification when the window is no longer the front
  // window, we need to install these on the widget.
  // NOTE: this doesn't work with focus follows mouse.
  g_signal_connect(widget_, "focus_in_event",
                   G_CALLBACK(&OnFocusInThunk), this);
  g_signal_connect(widget_, "focus_out_event",
                   G_CALLBACK(&OnFocusOutThunk), this);
  g_signal_connect(widget_, "destroy",
                   G_CALLBACK(&OnDestroyThunk), this);
  g_signal_connect(widget_, "show",
                   G_CALLBACK(&OnShowThunk), this);
  g_signal_connect(widget_, "map",
                   G_CALLBACK(&OnMapThunk), this);
  g_signal_connect(widget_, "hide",
                   G_CALLBACK(&OnHideThunk), this);

  // Views/FocusManager (re)sets the focus to the root window,
  // so we need to connect signal handlers to the gtk window.
  // See views::Views::Focus and views::FocusManager::ClearNativeFocus
  // for more details.
  g_signal_connect(widget_, "key_press_event",
                   G_CALLBACK(&OnKeyEventThunk), this);
  g_signal_connect(widget_, "key_release_event",
                   G_CALLBACK(&OnKeyEventThunk), this);

  // Drag and drop.
  gtk_drag_dest_set(window_contents_, static_cast<GtkDestDefaults>(0),
                    NULL, 0, GDK_ACTION_COPY);
  g_signal_connect(window_contents_, "drag_motion",
                   G_CALLBACK(&OnDragMotionThunk), this);
  g_signal_connect(window_contents_, "drag_data_received",
                   G_CALLBACK(&OnDragDataReceivedThunk), this);
  g_signal_connect(window_contents_, "drag_drop",
                   G_CALLBACK(&OnDragDropThunk), this);
  g_signal_connect(window_contents_, "drag_leave",
                   G_CALLBACK(&OnDragLeaveThunk), this);
  g_signal_connect(window_contents_, "drag_data_get",
                   G_CALLBACK(&OnDragDataGetThunk), this);
  g_signal_connect(window_contents_, "drag_end",
                   G_CALLBACK(&OnDragEndThunk), this);
  g_signal_connect(window_contents_, "drag_failed",
                   G_CALLBACK(&OnDragFailedThunk), this);

  tooltip_manager_.reset(new TooltipManagerGtk(this));

  // Register for tooltips.
  g_object_set(G_OBJECT(window_contents_), "has-tooltip", TRUE, NULL);
  g_signal_connect(window_contents_, "query_tooltip",
                   G_CALLBACK(&OnQueryTooltipThunk), this);

  if (type_ == TYPE_CHILD) {
    if (parent) {
      SetBounds(bounds);
    }
  } else {
    if (bounds.width() > 0 && bounds.height() > 0)
      gtk_window_resize(GTK_WINDOW(widget_), bounds.width(), bounds.height());
    gtk_window_move(GTK_WINDOW(widget_), bounds.x(), bounds.y());
  }
}

gfx::NativeView WidgetGtk::GetNativeView() const {
  return widget_;
}

bool WidgetGtk::GetAccelerator(int cmd_id, ui::Accelerator* accelerator) {
  NOTIMPLEMENTED();
  return false;
}

Window* WidgetGtk::GetWindow() {
  return GetWindowImpl(widget_);
}

const Window* WidgetGtk::GetWindow() const {
  return GetWindowImpl(widget_);
}

void WidgetGtk::ViewHierarchyChanged(bool is_add, View* parent, View* child) {
  Widget::ViewHierarchyChanged(is_add, parent, child);
  if (drop_target_.get())
    drop_target_->ResetTargetViewIfEquals(child);
}

void WidgetGtk::NotifyAccessibilityEvent(
    View* view,
    ui::AccessibilityTypes::Event event_type,
    bool send_native_event) {
  // Send the notification to the delegate.
  if (ViewsDelegate::views_delegate)
    ViewsDelegate::views_delegate->NotifyAccessibilityEvent(view, event_type);

  // In the future if we add native GTK accessibility support, the
  // notification should be sent here.
}

void WidgetGtk::ClearNativeFocus() {
  DCHECK(type_ != TYPE_CHILD);
  if (!GetNativeView()) {
    NOTREACHED();
    return;
  }
  gtk_window_set_focus(GTK_WINDOW(GetNativeView()), NULL);
}

bool WidgetGtk::HandleKeyboardEvent(const KeyEvent& key) {
  if (!GetFocusManager())
    return false;

  const int key_code = key.key_code();
  bool handled = false;

  // Always reset |should_handle_menu_key_release_| unless we are handling a
  // VKEY_MENU key release event. It ensures that VKEY_MENU accelerator can only
  // be activated when handling a VKEY_MENU key release event which is preceded
  // by an un-handled VKEY_MENU key press event.
  if (key_code != ui::VKEY_MENU || key.type() != ui::ET_KEY_RELEASED)
    should_handle_menu_key_release_ = false;

  if (key.type() == ui::ET_KEY_PRESSED) {
    // VKEY_MENU is triggered by key release event.
    // FocusManager::OnKeyEvent() returns false when the key has been consumed.
    if (key_code != ui::VKEY_MENU)
      handled = !GetFocusManager()->OnKeyEvent(key);
    else
      should_handle_menu_key_release_ = true;
  } else if (key_code == ui::VKEY_MENU && should_handle_menu_key_release_ &&
             (key.flags() & ~ui::EF_ALT_DOWN) == 0) {
    // Trigger VKEY_MENU when only this key is pressed and released, and both
    // press and release events are not handled by others.
    Accelerator accelerator(ui::VKEY_MENU, false, false, false);
    handled = GetFocusManager()->ProcessAccelerator(accelerator);
  }

  return handled;
}

// static
void WidgetGtk::EnableDebugPaint() {
  debug_paint_enabled_ = true;
}

// static
void WidgetGtk::UpdateFreezeUpdatesProperty(GtkWindow* window, bool enable) {
  if (!GTK_WIDGET_REALIZED(GTK_WIDGET(window)))
    gtk_widget_realize(GTK_WIDGET(window));
  GdkWindow* gdk_window = GTK_WIDGET(window)->window;

  static GdkAtom freeze_atom_ =
      gdk_atom_intern("_CHROME_FREEZE_UPDATES", FALSE);
  if (enable) {
    VLOG(1) << "setting FREEZE UPDATES property. xid=" <<
        GDK_WINDOW_XID(gdk_window);
    int32 val = 1;
    gdk_property_change(gdk_window,
                        freeze_atom_,
                        freeze_atom_,
                        32,
                        GDK_PROP_MODE_REPLACE,
                        reinterpret_cast<const guchar*>(&val),
                        1);
  } else {
    VLOG(1) << "deleting FREEZE UPDATES property. xid=" <<
        GDK_WINDOW_XID(gdk_window);
    gdk_property_delete(gdk_window, freeze_atom_);
  }
}

// static
void WidgetGtk::RegisterChildExposeHandler(GtkWidget* child) {
  RemoveExposeHandlerIfExists(child);
  gulong id = g_signal_connect_after(child, "expose-event",
                                     G_CALLBACK(&ChildExposeHandler), NULL);
  g_object_set_data(G_OBJECT(child), kExposeHandlerIdKey,
                    reinterpret_cast<void*>(id));
}

////////////////////////////////////////////////////////////////////////////////
// WidgetGtk, NativeWidget implementation:

void WidgetGtk::SetCreateParams(const CreateParams& params) {
  DCHECK(!GetNativeView());

  // Set non-style attributes.
  set_delete_on_destroy(params.delete_on_destroy);

  if (params.transparent)
    MakeTransparent();
  if (!params.accept_events)
    MakeIgnoreEvents();

  if (params.type == CreateParams::TYPE_MENU) {
    GdkEvent* event = gtk_get_current_event();
    if (event) {
      is_mouse_button_pressed_ = event->type == GDK_BUTTON_PRESS ||
        event->type == GDK_2BUTTON_PRESS ||
        event->type == GDK_3BUTTON_PRESS;
      gdk_event_free(event);
    }
  }
}

Widget* WidgetGtk::GetWidget() {
  return this;
}

void WidgetGtk::SetNativeWindowProperty(const char* name, void* value) {
  g_object_set_data(G_OBJECT(widget_), name, value);
}

void* WidgetGtk::GetNativeWindowProperty(const char* name) {
  return g_object_get_data(G_OBJECT(widget_), name);
}

TooltipManager* WidgetGtk::GetTooltipManager() const {
  return tooltip_manager_.get();
}

bool WidgetGtk::IsScreenReaderActive() const {
  return false;
}

void WidgetGtk::SetMouseCapture() {
  DCHECK(!HasMouseCapture());
  gtk_grab_add(window_contents_);
}

void WidgetGtk::ReleaseMouseCapture() {
  if (HasMouseCapture())
    gtk_grab_remove(window_contents_);
}

bool WidgetGtk::HasMouseCapture() const {
  // TODO(beng): Should be able to use gtk_widget_has_grab() here but the
  //             trybots don't have Gtk 2.18.
  return GTK_WIDGET_HAS_GRAB(window_contents_);
}

InputMethod* WidgetGtk::GetInputMethodNative() {
  return input_method_.get();
}

void WidgetGtk::ReplaceInputMethod(InputMethod* input_method) {
  input_method_.reset(input_method);
  if (input_method) {
    input_method->set_delegate(this);
    input_method->Init(GetWidget());
  }
}

gfx::Rect WidgetGtk::GetWindowScreenBounds() const {
  // Client == Window bounds on Gtk.
  return GetClientAreaScreenBounds();
}

gfx::Rect WidgetGtk::GetClientAreaScreenBounds() const {
  // Due to timing we can get a request for bounds after Close().
  // TODO(beng): Figure out if this is bogus.
  if (!widget_)
    return gfx::Rect(size_);

  int x = 0, y = 0, w = 0, h = 0;
  if (GTK_IS_WINDOW(widget_)) {
    gtk_window_get_position(GTK_WINDOW(widget_), &x, &y);
    // NOTE: this doesn't include frame decorations, but it should be good
    // enough for our uses.
    gtk_window_get_size(GTK_WINDOW(widget_), &w, &h);
  } else {
    GetWidgetPositionOnScreen(widget_, &x, &y);
    w = widget_->allocation.width;
    h = widget_->allocation.height;
  }
  return gfx::Rect(x, y, w, h);
}

void WidgetGtk::SetBounds(const gfx::Rect& bounds) {
  if (type_ == TYPE_CHILD) {
    GtkWidget* parent = gtk_widget_get_parent(widget_);
    if (GTK_IS_VIEWS_FIXED(parent)) {
      WidgetGtk* parent_widget = static_cast<WidgetGtk*>(
          NativeWidget::GetNativeWidgetForNativeView(parent));
      parent_widget->PositionChild(widget_, bounds.x(), bounds.y(),
                                   bounds.width(), bounds.height());
    } else {
      DCHECK(GTK_IS_FIXED(parent))
          << "Parent of WidgetGtk has to be Fixed or ViewsFixed";
      // Just request the size if the parent is not WidgetGtk but plain
      // GtkFixed. WidgetGtk does not know the minimum size so we assume
      // the caller of the SetBounds knows exactly how big it wants to be.
      gtk_widget_set_size_request(widget_, bounds.width(), bounds.height());
      if (parent != null_parent_)
        gtk_fixed_move(GTK_FIXED(parent), widget_, bounds.x(), bounds.y());
    }
  } else {
    if (GTK_WIDGET_MAPPED(widget_)) {
      // If the widget is mapped (on screen), we can move and resize with one
      // call, which avoids two separate window manager steps.
      gdk_window_move_resize(widget_->window, bounds.x(), bounds.y(),
                             bounds.width(), bounds.height());
    }

    // Always call gtk_window_move and gtk_window_resize so that GtkWindow's
    // geometry info is up-to-date.
    GtkWindow* gtk_window = GTK_WINDOW(widget_);
    // TODO: this may need to set an initial size if not showing.
    // TODO: need to constrain based on screen size.
    if (!bounds.IsEmpty()) {
      gtk_window_resize(gtk_window, bounds.width(), bounds.height());
    }
    gtk_window_move(gtk_window, bounds.x(), bounds.y());
  }
}

void WidgetGtk::SetSize(const gfx::Size& size) {
  if (type_ == TYPE_CHILD) {
    GtkWidget* parent = gtk_widget_get_parent(widget_);
    if (GTK_IS_VIEWS_FIXED(parent)) {
      gtk_views_fixed_set_widget_size(widget_, size.width(), size.height());
    } else {
      DCHECK(GTK_IS_FIXED(parent))
          << "Parent of WidgetGtk has to be Fixed or ViewsFixed";
      gtk_widget_set_size_request(widget_, size.width(), size.height());
    }
  } else {
    if (GTK_WIDGET_MAPPED(widget_))
      gdk_window_resize(widget_->window, size.width(), size.height());
    GtkWindow* gtk_window = GTK_WINDOW(widget_);
    if (!size.IsEmpty())
      gtk_window_resize(gtk_window, size.width(), size.height());
  }
}

void WidgetGtk::MoveAbove(gfx::NativeView native_view) {
  ui::StackPopupWindow(GetNativeView(), native_view);
}

void WidgetGtk::SetShape(gfx::NativeRegion region) {
  DCHECK(widget_);
  DCHECK(widget_->window);
  gdk_window_shape_combine_region(widget_->window, region, 0, 0);
  gdk_region_destroy(region);
}

void WidgetGtk::Close() {
  if (!widget_)
    return;  // No need to do anything.

  // Hide first.
  Hide();
  if (close_widget_factory_.empty()) {
    // And we delay the close just in case we're on the stack.
    MessageLoop::current()->PostTask(FROM_HERE,
        close_widget_factory_.NewRunnableMethod(
            &WidgetGtk::CloseNow));
  }
}

void WidgetGtk::CloseNow() {
  if (widget_) {
    input_method_.reset();
    gtk_widget_destroy(widget_);  // Triggers OnDestroy().
  }
}

void WidgetGtk::Show() {
  if (widget_) {
    gtk_widget_show(widget_);
    if (widget_->window)
      gdk_window_raise(widget_->window);
  }
}

void WidgetGtk::Hide() {
  if (widget_) {
    gtk_widget_hide(widget_);
    if (widget_->window)
      gdk_window_lower(widget_->window);
  }
}

void WidgetGtk::SetOpacity(unsigned char opacity) {
  opacity_ = opacity;
  if (widget_) {
    // We can only set the opacity when the widget has been realized.
    gdk_window_set_opacity(widget_->window, static_cast<gdouble>(opacity) /
      static_cast<gdouble>(255));
  }
}

void WidgetGtk::SetAlwaysOnTop(bool on_top) {
  DCHECK(type_ != TYPE_CHILD);
  always_on_top_ = on_top;
  if (widget_)
    gtk_window_set_keep_above(GTK_WINDOW(widget_), on_top);
}

bool WidgetGtk::IsVisible() const {
  return GTK_WIDGET_VISIBLE(widget_);
}

bool WidgetGtk::IsActive() const {
  DCHECK(type_ != TYPE_CHILD);
  return is_active_;
}

bool WidgetGtk::IsAccessibleWidget() const {
  return false;
}

bool WidgetGtk::ContainsNativeView(gfx::NativeView native_view) const {
  // TODO(port)  See implementation in WidgetWin::ContainsNativeView.
  NOTREACHED() << "WidgetGtk::ContainsNativeView is not implemented.";
  return false;
}

void WidgetGtk::RunShellDrag(View* view,
                             const ui::OSExchangeData& data,
                             int operation) {
  DoDrag(data, operation);
}

void WidgetGtk::SchedulePaintInRect(const gfx::Rect& rect) {
  // No need to schedule paint if
  // 1) widget_ is NULL. This may happen because this instance may
  // be deleted after the gtk widget has been destroyed (See OnDestroy()).
  // 2) widget_ is not drawable (mapped and visible)
  // 3) If it's never painted before. The first expose event will
  // paint the area that has to be painted.
  if (widget_ && GTK_WIDGET_DRAWABLE(widget_) && painted_) {
    gtk_widget_queue_draw_area(widget_, rect.x(), rect.y(), rect.width(),
                               rect.height());
  }
}

void WidgetGtk::SetCursor(gfx::NativeCursor cursor) {
#if defined(TOUCH_UI) && defined(HAVE_XINPUT2)
  if (!TouchFactory::GetInstance()->is_cursor_visible() &&
      !RootView::GetKeepMouseCursor())
    cursor = gfx::GetCursor(GDK_BLANK_CURSOR);
#endif
  // |window_contents_| is placed on top of |widget_|. So the cursor needs to be
  // set on |window_contents_| instead of |widget_|.
  if (window_contents_)
    gdk_window_set_cursor(window_contents_->window, cursor);
}

////////////////////////////////////////////////////////////////////////////////
// WidgetGtk, protected:

void WidgetGtk::OnSizeRequest(GtkWidget* widget, GtkRequisition* requisition) {
  // Do only return the preferred size for child windows. GtkWindow interprets
  // the requisition as a minimum size for top level windows, returning a
  // preferred size for these would prevents us from setting smaller window
  // sizes.
  if (type_ == TYPE_CHILD) {
    gfx::Size size(GetRootView()->GetPreferredSize());
    requisition->width = size.width();
    requisition->height = size.height();
  }
}

void WidgetGtk::OnSizeAllocate(GtkWidget* widget, GtkAllocation* allocation) {
  // See comment next to size_ as to why we do this. Also note, it's tempting
  // to put this in the static method so subclasses don't need to worry about
  // it, but if a subclasses needs to set a shape then they need to always
  // reset the shape in this method regardless of whether the size changed.
  gfx::Size new_size(allocation->width, allocation->height);
  if (new_size == size_)
    return;
  size_ = new_size;
  delegate_->OnSizeChanged(size_);
}

gboolean WidgetGtk::OnPaint(GtkWidget* widget, GdkEventExpose* event) {
  if (transparent_ && type_ == TYPE_CHILD) {
    // Clear the background before drawing any view and native components.
    DrawTransparentBackground(widget, event);
    if (!CompositePainter::IsComposited(widget_) &&
        gdk_screen_is_composited(gdk_screen_get_default())) {
      // Let the parent draw the content only after something is drawn on
      // the widget.
      CompositePainter::SetComposited(widget_);
    }
  }

  if (debug_paint_enabled_) {
    // Using cairo directly because using skia didn't have immediate effect.
    cairo_t* cr = gdk_cairo_create(event->window);
    gdk_cairo_region(cr, event->region);
    cairo_set_source_rgb(cr, 1, 0, 0);  // red
    cairo_rectangle(cr,
                    event->area.x, event->area.y,
                    event->area.width, event->area.height);
    cairo_fill(cr);
    cairo_destroy(cr);
    // Make sure that users see the red flash.
    XSync(ui::GetXDisplay(), false /* don't discard events */);
  }

  gfx::CanvasSkiaPaint canvas(event);
  if (!canvas.is_empty()) {
    canvas.set_composite_alpha(is_transparent());
    delegate_->OnNativeWidgetPaint(&canvas);
  }

  if (!painted_) {
    painted_ = true;
    if (type_ != TYPE_CHILD)
      UpdateFreezeUpdatesProperty(GTK_WINDOW(widget_), false /* remove */);
  }
  return false;  // False indicates other widgets should get the event as well.
}

void WidgetGtk::OnDragDataGet(GtkWidget* widget,
                              GdkDragContext* context,
                              GtkSelectionData* data,
                              guint info,
                              guint time) {
  if (!drag_data_) {
    NOTREACHED();
    return;
  }
  drag_data_->WriteFormatToSelection(info, data);
}

void WidgetGtk::OnDragDataReceived(GtkWidget* widget,
                                   GdkDragContext* context,
                                   gint x,
                                   gint y,
                                   GtkSelectionData* data,
                                   guint info,
                                   guint time) {
  if (drop_target_.get())
    drop_target_->OnDragDataReceived(context, x, y, data, info, time);
}

gboolean WidgetGtk::OnDragDrop(GtkWidget* widget,
                               GdkDragContext* context,
                               gint x,
                               gint y,
                               guint time) {
  if (drop_target_.get()) {
    return drop_target_->OnDragDrop(context, x, y, time);
  }
  return FALSE;
}

void WidgetGtk::OnDragEnd(GtkWidget* widget, GdkDragContext* context) {
  if (!drag_data_) {
    // This indicates we didn't start a drag operation, and should never
    // happen.
    NOTREACHED();
    return;
  }
  // Quit the nested message loop we spawned in DoDrag.
  MessageLoop::current()->Quit();
}

gboolean WidgetGtk::OnDragFailed(GtkWidget* widget,
                                 GdkDragContext* context,
                                 GtkDragResult result) {
  return FALSE;
}

void WidgetGtk::OnDragLeave(GtkWidget* widget,
                            GdkDragContext* context,
                            guint time) {
  if (ignore_drag_leave_) {
    ignore_drag_leave_ = false;
    return;
  }
  if (drop_target_.get()) {
    drop_target_->OnDragLeave(context, time);
    drop_target_.reset(NULL);
  }
}

gboolean WidgetGtk::OnDragMotion(GtkWidget* widget,
                                 GdkDragContext* context,
                                 gint x,
                                 gint y,
                                 guint time) {
  if (!drop_target_.get())
    drop_target_.reset(new DropTargetGtk(GetRootView(), context));
  return drop_target_->OnDragMotion(context, x, y, time);
}

gboolean WidgetGtk::OnEnterNotify(GtkWidget* widget, GdkEventCrossing* event) {
  if (HasMouseCapture() && event->mode == GDK_CROSSING_GRAB) {
    // Doing a grab results an async enter event, regardless of where the mouse
    // is. We don't want to generate a mouse move in this case.
    return false;
  }

  if (!last_mouse_event_was_move_ && !is_mouse_button_pressed_) {
    // When a mouse button is pressed gtk generates a leave, enter, press.
    // RootView expects to get a mouse move before a press, otherwise enter is
    // not set. So we generate a move here.
    GdkEventMotion motion = { GDK_MOTION_NOTIFY, event->window,
        event->send_event, event->time, event->x, event->y, NULL, event->state,
        0, NULL, event->x_root, event->y_root };

    // If this event is the result of pressing a button then one of the button
    // modifiers is set. Unset it as we're compensating for the leave generated
    // when you press a button.
    motion.state &= ~(GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK);

    MouseEvent mouse_event(TransformEvent(&motion));
    delegate_->OnMouseEvent(mouse_event);
  }

  return false;
}

gboolean WidgetGtk::OnLeaveNotify(GtkWidget* widget, GdkEventCrossing* event) {
  last_mouse_event_was_move_ = false;
  if (!HasMouseCapture() && !is_mouse_button_pressed_) {
    MouseEvent mouse_event(TransformEvent(event));
    delegate_->OnMouseEvent(mouse_event);
  }
  return false;
}

gboolean WidgetGtk::OnMotionNotify(GtkWidget* widget, GdkEventMotion* event) {
  MouseEvent mouse_event(TransformEvent(event));
  delegate_->OnMouseEvent(mouse_event);
  return true;
}

gboolean WidgetGtk::OnButtonPress(GtkWidget* widget, GdkEventButton* event) {
  if (event->type == GDK_2BUTTON_PRESS || event->type == GDK_3BUTTON_PRESS) {
    // The sequence for double clicks is press, release, press, 2press, release.
    // This means that at the time we get the second 'press' we don't know
    // whether it corresponds to a double click or not. For now we're completely
    // ignoring the 2press/3press events as they are duplicate. To make this
    // work right we need to write our own code that detects if the press is a
    // double/triple. For now we're completely punting, which means we always
    // get single clicks.
    // TODO: fix this.
    return true;
  }

  MouseEvent mouse_event(TransformEvent(event));
  // Returns true to consume the event when widget is not transparent.
  return delegate_->OnMouseEvent(mouse_event) || !transparent_;
}

gboolean WidgetGtk::OnButtonRelease(GtkWidget* widget, GdkEventButton* event) {
  // GTK generates a mouse release at the end of dnd. We need to ignore it.
  if (!drag_data_) {
    MouseEvent mouse_event(TransformEvent(event));
    delegate_->OnMouseEvent(mouse_event);
  }
  return true;
}

gboolean WidgetGtk::OnScroll(GtkWidget* widget, GdkEventScroll* event) {
  MouseEvent mouse_event(TransformEvent(event));
  return delegate_->OnMouseEvent(mouse_event);
}

gboolean WidgetGtk::OnFocusIn(GtkWidget* widget, GdkEventFocus* event) {
  if (has_focus_)
    return false;  // This is the second focus-in event in a row, ignore it.
  has_focus_ = true;

  should_handle_menu_key_release_ = false;

  if (type_ == TYPE_CHILD)
    return false;

  // Only top-level Widget should have an InputMethod instance.
  if (input_method_.get())
    input_method_->OnFocus();

  // See description of got_initial_focus_in_ for details on this.
  if (!got_initial_focus_in_) {
    got_initial_focus_in_ = true;
    SetInitialFocus();
  }
  return false;
}

gboolean WidgetGtk::OnFocusOut(GtkWidget* widget, GdkEventFocus* event) {
  if (!has_focus_)
    return false;  // This is the second focus-out event in a row, ignore it.
  has_focus_ = false;

  if (type_ == TYPE_CHILD)
    return false;

  // Only top-level Widget should have an InputMethod instance.
  if (input_method_.get())
    input_method_->OnBlur();
  return false;
}

gboolean WidgetGtk::OnKeyEvent(GtkWidget* widget, GdkEventKey* event) {
  KeyEvent key(reinterpret_cast<NativeEvent>(event));
  if (input_method_.get())
    input_method_->DispatchKeyEvent(key);
  else
    DispatchKeyEventPostIME(key);

  // Returns true to prevent GtkWindow's default key event handler.
  return true;
}

gboolean WidgetGtk::OnQueryTooltip(GtkWidget* widget,
                                   gint x,
                                   gint y,
                                   gboolean keyboard_mode,
                                   GtkTooltip* tooltip) {
  return tooltip_manager_->ShowTooltip(x, y, keyboard_mode, tooltip);
}

gboolean WidgetGtk::OnVisibilityNotify(GtkWidget* widget,
                                       GdkEventVisibility* event) {
  return false;
}

gboolean WidgetGtk::OnGrabBrokeEvent(GtkWidget* widget, GdkEvent* event) {
  HandleXGrabBroke();
  return false;  // To let other widgets get the event.
}

void WidgetGtk::OnGrabNotify(GtkWidget* widget, gboolean was_grabbed) {
  if (!window_contents_)
    return;  // Grab broke after window destroyed, don't try processing it.
  gtk_grab_remove(window_contents_);
  HandleGtkGrabBroke();
}

void WidgetGtk::OnDestroy(GtkWidget* object) {
  // Note that this handler is hooked to GtkObject::destroy.
  // NULL out pointers here since we might still be in an observerer list
  // until delstion happens.
  widget_ = window_contents_ = NULL;
  if (delete_on_destroy_) {
    // Delays the deletion of this WidgetGtk as we want its children to have
    // access to it when destroyed.
    MessageLoop::current()->DeleteSoon(FROM_HERE, this);
  }
}

void WidgetGtk::OnShow(GtkWidget* widget) {
}

void WidgetGtk::OnMap(GtkWidget* widget) {
#if defined(TOUCH_UI)
  // Force an expose event to trigger OnPaint for touch. This is
  // a workaround for a bug that X Expose event does not trigger
  // Gdk's expose signal. This happens when you try to open views menu
  // while a virtual keyboard gets kicked in or out. This seems to be
  // a bug in message_pump_glib_x.cc as we do get X Expose event but
  // it doesn't trigger gtk's expose signal. We're not going to fix this
  // as we're removing gtk and migrating to new compositor.
  gdk_window_process_updates(widget_->window, true);
#endif
}

void WidgetGtk::OnHide(GtkWidget* widget) {
}

void WidgetGtk::HandleXGrabBroke() {
}

void WidgetGtk::HandleGtkGrabBroke() {
  delegate_->OnMouseCaptureLost();
}

////////////////////////////////////////////////////////////////////////////////
// WidgetGtk, private:

RootView* WidgetGtk::CreateRootView() {
  return new RootView(this);
}

gfx::AcceleratedWidget WidgetGtk::GetAcceleratedWidget() {
  DCHECK(window_contents_ && window_contents_->window);
  return GDK_WINDOW_XID(window_contents_->window);
}

void WidgetGtk::DispatchKeyEventPostIME(const KeyEvent& key) {
  // Always reset |should_handle_menu_key_release_| unless we are handling a
  // VKEY_MENU key release event. It ensures that VKEY_MENU accelerator can only
  // be activated when handling a VKEY_MENU key release event which is preceded
  // by an unhandled VKEY_MENU key press event. See also HandleKeyboardEvent().
  if (key.key_code() != ui::VKEY_MENU || key.type() != ui::ET_KEY_RELEASED)
    should_handle_menu_key_release_ = false;

  bool handled = false;

  // Dispatch the key event to View hierarchy first.
  handled = GetRootView()->ProcessKeyEvent(key);

  if (key.key_code() == ui::VKEY_PROCESSKEY || handled)
    return;

  // Dispatch the key event to native GtkWidget hierarchy.
  // To prevent GtkWindow from handling the key event as a keybinding, we need
  // to bypass GtkWindow's default key event handler and dispatch the event
  // here.
  GdkEventKey* event = reinterpret_cast<GdkEventKey*>(key.native_event());
  if (!handled && event && GTK_IS_WINDOW(widget_))
    handled = gtk_window_propagate_key_event(GTK_WINDOW(widget_), event);

  // On Linux, in order to handle VKEY_MENU (Alt) accelerator key correctly and
  // avoid issues like: http://crbug.com/40966 and http://crbug.com/49701, we
  // should only send the key event to the focus manager if it's not handled by
  // any View or native GtkWidget.
  // The flow is different when the focus is in a RenderWidgetHostViewGtk, which
  // always consumes the key event and send it back to us later by calling
  // HandleKeyboardEvent() directly, if it's not handled by webkit.
  if (!handled)
    handled = HandleKeyboardEvent(key);

  // Dispatch the key event for bindings processing.
  if (!handled && event && GTK_IS_WINDOW(widget_))
    gtk_bindings_activate_event(GTK_OBJECT(widget_), event);
}

gboolean WidgetGtk::OnWindowPaint(GtkWidget* widget, GdkEventExpose* event) {
  // Clear the background to be totally transparent. We don't need to
  // paint the root view here as that is done by OnPaint.
  DCHECK(transparent_);
  DrawTransparentBackground(widget, event);
  // The Keyboard layout view has a renderer that covers the entire
  // window, which prevents OnPaint from being called on window_contents_,
  // so we need to remove the FREEZE_UPDATES property here.
  if (!painted_) {
    painted_ = true;
    UpdateFreezeUpdatesProperty(GTK_WINDOW(widget_), false /* remove */);
  }
  return false;
}

void WidgetGtk::OnChildExpose(GtkWidget* child) {
  DCHECK(type_ != TYPE_CHILD);
  if (!painted_) {
    painted_ = true;
    UpdateFreezeUpdatesProperty(GTK_WINDOW(widget_), false /* remove */);
  }
  RemoveExposeHandlerIfExists(child);
}

// static
gboolean WidgetGtk::ChildExposeHandler(GtkWidget* widget,
                                       GdkEventExpose* event) {
  GtkWidget* toplevel = gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW);
  CHECK(toplevel);
  NativeWidget* native_widget =
      NativeWidget::GetNativeWidgetForNativeView(toplevel);
  CHECK(native_widget);
  WidgetGtk* widget_gtk = static_cast<WidgetGtk*>(native_widget);
  widget_gtk->OnChildExpose(widget);
  return false;
}

// static
Window* WidgetGtk::GetWindowImpl(GtkWidget* widget) {
  GtkWidget* parent = widget;
  while (parent) {
    WidgetGtk* widget_gtk = static_cast<WidgetGtk*>(
        NativeWidget::GetNativeWidgetForNativeView(parent));
    if (widget_gtk && widget_gtk->is_window_)
      return static_cast<WindowGtk*>(widget_gtk);
    parent = gtk_widget_get_parent(parent);
  }
  return NULL;
}

void WidgetGtk::CreateGtkWidget(GtkWidget* parent, const gfx::Rect& bounds) {
  // We turn off double buffering for two reasons:
  // 1. We draw to a canvas then composite to the screen, which means we're
  //    doing our own double buffering already.
  // 2. GTKs double buffering clips to the dirty region. RootView occasionally
  //    needs to expand the paint region (see RootView::OnPaint). This means
  //    that if we use GTK's double buffering and we tried to expand the dirty
  //    region, it wouldn't get painted.
  if (type_ == TYPE_CHILD) {
    window_contents_ = widget_ = gtk_views_fixed_new();
    gtk_widget_set_name(widget_, "views-gtkwidget-child-fixed");
    if (!is_double_buffered_)
      GTK_WIDGET_UNSET_FLAGS(widget_, GTK_DOUBLE_BUFFERED);
    gtk_fixed_set_has_window(GTK_FIXED(widget_), true);
    if (!parent && !null_parent_) {
      GtkWidget* popup = gtk_window_new(GTK_WINDOW_POPUP);
      null_parent_ = gtk_fixed_new();
      gtk_widget_set_name(widget_, "views-gtkwidget-null-parent");
      gtk_container_add(GTK_CONTAINER(popup), null_parent_);
      gtk_widget_realize(null_parent_);
    }
    if (transparent_) {
      // transparency has to be configured before widget is realized.
      DCHECK(parent) << "Transparent widget must have parent when initialized";
      ConfigureWidgetForTransparentBackground(parent);
    }
    gtk_container_add(GTK_CONTAINER(parent ? parent : null_parent_), widget_);
    gtk_widget_realize(widget_);
    if (transparent_) {
      // The widget has to be realized to set composited flag.
      // I tried "realize" signal to set this flag, but it did not work
      // when the top level is popup.
      DCHECK(GTK_WIDGET_REALIZED(widget_));
      gdk_window_set_composited(widget_->window, true);
    }
    if (parent && !bounds.size().IsEmpty()) {
      // Make sure that an widget is given it's initial size before
      // we're done initializing, to take care of some potential
      // corner cases when programmatically arranging hierarchies as
      // seen in
      // http://code.google.com/p/chromium-os/issues/detail?id=5987

      // This can't be done without a parent present, or stale data
      // might show up on the screen as seen in
      // http://code.google.com/p/chromium/issues/detail?id=53870
      GtkAllocation alloc = { 0, 0, bounds.width(), bounds.height() };
      gtk_widget_size_allocate(widget_, &alloc);
    }
  } else {
    // Use our own window class to override GtkWindow's move_focus method.
    widget_ = gtk_views_window_new(
        (type_ == TYPE_WINDOW || type_ == TYPE_DECORATED_WINDOW) ?
        GTK_WINDOW_TOPLEVEL : GTK_WINDOW_POPUP);
    gtk_widget_set_name(widget_, "views-gtkwidget-window");
    if (transient_to_parent_)
      gtk_window_set_transient_for(GTK_WINDOW(widget_), GTK_WINDOW(parent));
    GTK_WIDGET_UNSET_FLAGS(widget_, GTK_DOUBLE_BUFFERED);

    // Gtk determines the size for windows based on the requested size of the
    // child. For WidgetGtk the child is a fixed. If the fixed ends up with a
    // child widget it's possible the child widget will drive the requested size
    // of the widget, which we don't want. We explicitly set a value of 1x1 here
    // so that gtk doesn't attempt to resize the window if we end up with a
    // situation where the requested size of a child of the fixed is greater
    // than the size of the window. By setting the size in this manner we're
    // also allowing users of WidgetGtk to change the requested size at any
    // time.
    gtk_widget_set_size_request(widget_, 1, 1);

    if (!bounds.size().IsEmpty()) {
      // When we realize the window, the window manager is given a size. If we
      // don't specify a size before then GTK defaults to 200x200. Specify
      // a size now so that the window manager sees the requested size.
      GtkAllocation alloc = { 0, 0, bounds.width(), bounds.height() };
      gtk_widget_size_allocate(widget_, &alloc);
    }
    if (type_ != TYPE_DECORATED_WINDOW) {
      gtk_window_set_decorated(GTK_WINDOW(widget_), false);
      // We'll take care of positioning our window.
      gtk_window_set_position(GTK_WINDOW(widget_), GTK_WIN_POS_NONE);
    }

    window_contents_ = gtk_views_fixed_new();
    gtk_widget_set_name(window_contents_, "views-gtkwidget-window-fixed");
    if (!is_double_buffered_)
      GTK_WIDGET_UNSET_FLAGS(window_contents_, GTK_DOUBLE_BUFFERED);
    gtk_fixed_set_has_window(GTK_FIXED(window_contents_), true);
    gtk_container_add(GTK_CONTAINER(widget_), window_contents_);
    gtk_widget_show(window_contents_);
    g_object_set_data(G_OBJECT(window_contents_), kNativeWidgetKey,
                       static_cast<Widget*>(this));
    if (transparent_)
      ConfigureWidgetForTransparentBackground(NULL);

    if (ignore_events_)
      ConfigureWidgetForIgnoreEvents();

    SetAlwaysOnTop(always_on_top_);
    // UpdateFreezeUpdatesProperty will realize the widget and handlers like
    // size-allocate will function properly.
    UpdateFreezeUpdatesProperty(GTK_WINDOW(widget_), true /* add */);
  }
  SetNativeWindowProperty(kNativeWidgetKey, this);
}

void WidgetGtk::ConfigureWidgetForTransparentBackground(GtkWidget* parent) {
  DCHECK(widget_ && window_contents_);

  GdkColormap* rgba_colormap =
      gdk_screen_get_rgba_colormap(gtk_widget_get_screen(widget_));
  if (!rgba_colormap) {
    transparent_ = false;
    return;
  }
  // To make the background transparent we need to install the RGBA colormap
  // on both the window and fixed. In addition we need to make sure no
  // decorations are drawn. The last bit is to make sure the widget doesn't
  // attempt to draw a pixmap in it's background.
  if (type_ != TYPE_CHILD) {
    DCHECK(parent == NULL);
    gtk_widget_set_colormap(widget_, rgba_colormap);
    gtk_widget_set_app_paintable(widget_, true);
    g_signal_connect(widget_, "expose_event",
                     G_CALLBACK(&OnWindowPaintThunk), this);
    gtk_widget_realize(widget_);
    gdk_window_set_decorations(widget_->window,
                               static_cast<GdkWMDecoration>(0));
  } else {
    DCHECK(parent);
    CompositePainter::AddCompositePainter(parent);
  }
  DCHECK(!GTK_WIDGET_REALIZED(window_contents_));
  gtk_widget_set_colormap(window_contents_, rgba_colormap);
}

void WidgetGtk::ConfigureWidgetForIgnoreEvents() {
  gtk_widget_realize(widget_);
  GdkWindow* gdk_window = widget_->window;
  Display* display = GDK_WINDOW_XDISPLAY(gdk_window);
  XID win = GDK_WINDOW_XID(gdk_window);

  // This sets the clickable area to be empty, allowing all events to be
  // passed to any windows behind this one.
  XShapeCombineRectangles(
      display,
      win,
      ShapeInput,
      0,  // x offset
      0,  // y offset
      NULL,  // rectangles
      0,  // num rectangles
      ShapeSet,
      0);
}

void WidgetGtk::DrawTransparentBackground(GtkWidget* widget,
                                          GdkEventExpose* event) {
  cairo_t* cr = gdk_cairo_create(widget->window);
  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  gdk_cairo_region(cr, event->region);
  cairo_fill(cr);
  cairo_destroy(cr);
}

////////////////////////////////////////////////////////////////////////////////
// Widget, public:

// static
Widget* Widget::CreateWidget(const CreateParams& params) {
  // TODO(beng): coalesce with CreateParams::Type.
  WidgetGtk::Type widget_gtk_type = WidgetGtk::TYPE_DECORATED_WINDOW;
  switch (params.type) {
    case CreateParams::TYPE_CONTROL:
      widget_gtk_type = WidgetGtk::TYPE_CHILD;
      break;
    case CreateParams::TYPE_MENU:
      widget_gtk_type = WidgetGtk::TYPE_POPUP;
      break;
    case CreateParams::TYPE_POPUP:
      widget_gtk_type = WidgetGtk::TYPE_POPUP;
      break;
    case CreateParams::TYPE_WINDOW:
      widget_gtk_type = WidgetGtk::TYPE_DECORATED_WINDOW;
      break;
    default:
      NOTREACHED();
      break;
  }

  WidgetGtk* widget = new WidgetGtk(widget_gtk_type);
  widget->SetCreateParams(params);
  return widget;
}

// static
void Widget::NotifyLocaleChanged() {
  GList *window_list = gtk_window_list_toplevels();
  for (GList* element = window_list; element; element = g_list_next(element)) {
    NativeWidget* native_widget =
        NativeWidget::GetNativeWidgetForNativeWindow(GTK_WINDOW(element->data));
    if (native_widget)
      native_widget->GetWidget()->LocaleChanged();
  }
  g_list_free(window_list);
}

// static
bool Widget::ConvertRect(const Widget* source,
                         const Widget* target,
                         gfx::Rect* rect) {
  DCHECK(source);
  DCHECK(target);
  DCHECK(rect);

  GtkWidget* source_widget = source->GetNativeView();
  GtkWidget* target_widget = target->GetNativeView();
  if (source_widget == target_widget)
    return true;

  if (!source_widget || !target_widget)
    return false;

  GdkRectangle gdk_rect = rect->ToGdkRectangle();
  if (gtk_widget_translate_coordinates(source_widget, target_widget,
                                       gdk_rect.x, gdk_rect.y,
                                       &gdk_rect.x, &gdk_rect.y)) {
    *rect = gdk_rect;
    return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// NativeWidget, public:

// static
NativeWidget* NativeWidget::GetNativeWidgetForNativeView(
    gfx::NativeView native_view) {
  if (!native_view)
    return NULL;
  return reinterpret_cast<WidgetGtk*>(
      g_object_get_data(G_OBJECT(native_view), kNativeWidgetKey));
}

// static
NativeWidget* NativeWidget::GetNativeWidgetForNativeWindow(
    gfx::NativeWindow native_window) {
  if (!native_window)
    return NULL;
  return reinterpret_cast<WidgetGtk*>(
      g_object_get_data(G_OBJECT(native_window), kNativeWidgetKey));
}

// static
NativeWidget* NativeWidget::GetTopLevelNativeWidget(
    gfx::NativeView native_view) {
  if (!native_view)
    return NULL;

  NativeWidget* widget = NULL;

  GtkWidget* parent_gtkwidget = native_view;
  NativeWidget* parent_widget;
  do {
    parent_widget = GetNativeWidgetForNativeView(parent_gtkwidget);
    if (parent_widget)
      widget = parent_widget;
    parent_gtkwidget = gtk_widget_get_parent(parent_gtkwidget);
  } while (parent_gtkwidget);

  return widget;
}

// static
void NativeWidget::GetAllNativeWidgets(gfx::NativeView native_view,
                                       NativeWidgets* children) {
  if (!native_view)
    return;

  NativeWidget* native_widget = GetNativeWidgetForNativeView(native_view);
  if (native_widget)
    children->insert(native_widget);
  gtk_container_foreach(GTK_CONTAINER(native_view),
                        EnumerateChildWidgetsForNativeWidgets,
                        reinterpret_cast<gpointer>(children));
}

}  // namespace views
