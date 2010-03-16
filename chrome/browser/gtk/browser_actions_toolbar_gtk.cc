// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/gtk/browser_actions_toolbar_gtk.h"

#include <vector>

#include "app/gfx/canvas_paint.h"
#include "app/gfx/gtk_util.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/extensions/extension_action_context_menu_model.h"
#include "chrome/browser/extensions/extension_browser_event_router.h"
#include "chrome/browser/extensions/extensions_service.h"
#include "chrome/browser/extensions/image_loading_tracker.h"
#include "chrome/browser/gtk/cairo_cached_surface.h"
#include "chrome/browser/gtk/extension_popup_gtk.h"
#include "chrome/browser/gtk/gtk_chrome_button.h"
#include "chrome/browser/gtk/gtk_chrome_shrinkable_hbox.h"
#include "chrome/browser/gtk/gtk_theme_provider.h"
#include "chrome/browser/gtk/gtk_util.h"
#include "chrome/browser/gtk/menu_gtk.h"
#include "chrome/browser/gtk/view_id_util.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/extensions/extension_action.h"
#include "chrome/common/notification_details.h"
#include "chrome/common/notification_service.h"
#include "chrome/common/notification_source.h"
#include "chrome/common/notification_type.h"
#include "grit/app_resources.h"

namespace {

// The size of each button on the toolbar.
const int kButtonSize = 29;

// The padding between browser action buttons. Visually, the actual number of
// "empty" (non-drawing) pixels is this value + 2 when adjacent browser icons
// use their maximum allowed size.
const int kButtonPadding = 3;

const char* kDragTarget = "application/x-chrome-browseraction";

GtkTargetEntry GetDragTargetEntry() {
  static std::string drag_target_string(kDragTarget);
  GtkTargetEntry drag_target;
  drag_target.target = const_cast<char*>(drag_target_string.c_str());
  drag_target.flags = GTK_TARGET_SAME_APP;
  drag_target.info = 0;
  return drag_target;
}

// The minimum width in pixels of the button hbox if |icon_count| icons are
// showing.
gint WidthForIconCount(gint icon_count) {
  return std::max((kButtonSize + kButtonPadding) * icon_count - kButtonPadding,
                  0);
}

}  // namespace

class BrowserActionButton : public NotificationObserver,
                            public ImageLoadingTracker::Observer {
 public:
  BrowserActionButton(BrowserActionsToolbarGtk* toolbar,
                      Extension* extension)
      : toolbar_(toolbar),
        extension_(extension),
        tracker_(NULL),
        tab_specific_icon_(NULL),
        default_icon_(NULL) {
    button_.Own(
        GtkThemeProvider::GetFrom(toolbar->profile_)->BuildChromeButton());

    DCHECK(extension_->browser_action());

    gtk_widget_set_size_request(button_.get(), kButtonSize, kButtonSize);

    UpdateState();

    // The Browser Action API does not allow the default icon path to be
    // changed at runtime, so we can load this now and cache it.
    std::string path = extension_->browser_action()->default_icon_path();
    if (!path.empty()) {
      tracker_ = new ImageLoadingTracker(this, 1);
      tracker_->PostLoadImageTask(extension_->GetResource(path),
          gfx::Size(Extension::kBrowserActionIconMaxSize,
                    Extension::kBrowserActionIconMaxSize));
    }

    g_signal_connect(button_.get(), "button-press-event",
                     G_CALLBACK(OnButtonPress), this);
    g_signal_connect(button_.get(), "clicked",
                     G_CALLBACK(OnClicked), this);
    g_signal_connect_after(button_.get(), "expose-event",
                           G_CALLBACK(OnExposeEvent), this);
    g_signal_connect(button_.get(), "drag-begin",
                     G_CALLBACK(&OnDragBegin), this);

    registrar_.Add(this, NotificationType::EXTENSION_BROWSER_ACTION_UPDATED,
                   Source<ExtensionAction>(extension->browser_action()));
  }

  ~BrowserActionButton() {
    if (tab_specific_icon_)
      g_object_unref(tab_specific_icon_);

    if (default_icon_)
      g_object_unref(default_icon_);

    button_.Destroy();

    if (tracker_)
      tracker_->StopTrackingImageLoad();
  }

  GtkWidget* widget() { return button_.get(); }

  Extension* extension() { return extension_; }

  // NotificationObserver implementation.
  void Observe(NotificationType type,
               const NotificationSource& source,
               const NotificationDetails& details) {
    if (type == NotificationType::EXTENSION_BROWSER_ACTION_UPDATED)
      UpdateState();
    else
      NOTREACHED();
  }

  // ImageLoadingTracker::Observer implementation.
  void OnImageLoaded(SkBitmap* image, size_t index) {
    if (image) {
      default_skbitmap_ = *image;
      default_icon_ = gfx::GdkPixbufFromSkBitmap(image);
    }
    tracker_ = NULL;  // The tracker object will delete itself when we return.
    UpdateState();
  }

  // Updates the button based on the latest state from the associated
  // browser action.
  void UpdateState() {
    int tab_id = toolbar_->GetCurrentTabId();
    if (tab_id < 0)
      return;

    std::string tooltip = extension_->browser_action()->GetTitle(tab_id);
    if (tooltip.empty())
      gtk_widget_set_has_tooltip(button_.get(), FALSE);
    else
      gtk_widget_set_tooltip_text(button_.get(), tooltip.c_str());

    SkBitmap image = extension_->browser_action()->GetIcon(tab_id);
    if (!image.isNull()) {
      GdkPixbuf* previous_gdk_icon = tab_specific_icon_;
      tab_specific_icon_ = gfx::GdkPixbufFromSkBitmap(&image);
      SetImage(tab_specific_icon_);
      if (previous_gdk_icon)
        g_object_unref(previous_gdk_icon);
    } else if (default_icon_) {
      SetImage(default_icon_);
    }
    gtk_widget_queue_draw(button_.get());
  }

  SkBitmap GetIcon() {
    const SkBitmap& image = extension_->browser_action()->GetIcon(
        toolbar_->GetCurrentTabId());
    if (!image.isNull()) {
      return image;
    } else {
      return default_skbitmap_;
    }
  }

 private:
  void SetImage(GdkPixbuf* image) {
    gtk_button_set_image(GTK_BUTTON(button_.get()),
        gtk_image_new_from_pixbuf(image));
  }

  static gboolean OnButtonPress(GtkWidget* widget,
                                GdkEvent* event,
                                BrowserActionButton* action) {
    if (event->button.button != 3)
      return FALSE;

    if (!action->context_menu_model_.get()) {
      action->context_menu_model_.reset(
          new ExtensionActionContextMenuModel(action->extension_));
    }

    action->context_menu_.reset(
        new MenuGtk(NULL, action->context_menu_model_.get()));

    action->context_menu_->Popup(widget, event);
    return TRUE;
  }

  static void OnClicked(GtkWidget* widget, BrowserActionButton* action) {
    ExtensionAction* browser_action = action->extension_->browser_action();

    int tab_id = action->toolbar_->GetCurrentTabId();
    if (tab_id < 0) {
      NOTREACHED() << "No current tab.";
      return;
    }

    if (browser_action->HasPopup(tab_id)) {
      ExtensionPopupGtk::Show(
          browser_action->GetPopupUrl(tab_id),
          action->toolbar_->browser(),
          gtk_util::GetWidgetRectRelativeToToplevel(widget));
    } else {
      ExtensionBrowserEventRouter::GetInstance()->BrowserActionExecuted(
          action->toolbar_->browser()->profile(), action->extension_->id(),
          action->toolbar_->browser());
    }
  }

  static gboolean OnExposeEvent(GtkWidget* widget,
                                GdkEventExpose* event,
                                BrowserActionButton* button) {
    int tab_id = button->toolbar_->GetCurrentTabId();
    if (tab_id < 0)
      return FALSE;

    ExtensionAction* action = button->extension_->browser_action();
    if (action->GetBadgeText(tab_id).empty())
      return FALSE;

    gfx::CanvasPaint canvas(event, false);
    gfx::Rect bounding_rect(widget->allocation);
    action->PaintBadge(&canvas, bounding_rect, tab_id);
    return FALSE;
  }

  static void OnDragBegin(GtkWidget* widget,
                          GdkDragContext* drag_context,
                          BrowserActionButton* button) {
    // Simply pass along the notification to the toolbar. The point of this
    // function is to tell the toolbar which BrowserActionButton initiated the
    // drag.
    button->toolbar_->DragStarted(button, drag_context);
  }

  // The toolbar containing this button.
  BrowserActionsToolbarGtk* toolbar_;

  // The extension that contains this browser action.
  Extension* extension_;

  // The gtk widget for this browser action.
  OwnedWidgetGtk button_;

  // Loads the button's icons for us on the file thread.
  ImageLoadingTracker* tracker_;

  // If we are displaying a tab-specific icon, it will be here.
  GdkPixbuf* tab_specific_icon_;

  // If the browser action has a default icon, it will be here.
  GdkPixbuf* default_icon_;

  // Same as |default_icon_|, but stored as SkBitmap.
  SkBitmap default_skbitmap_;

  NotificationRegistrar registrar_;

  // The context menu view and model for this extension action.
  scoped_ptr<MenuGtk> context_menu_;
  scoped_ptr<ExtensionActionContextMenuModel> context_menu_model_;

  friend class BrowserActionsToolbarGtk;
};

BrowserActionsToolbarGtk::BrowserActionsToolbarGtk(Browser* browser)
    : browser_(browser),
      profile_(browser->profile()),
      theme_provider_(GtkThemeProvider::GetFrom(browser->profile())),
      model_(NULL),
      hbox_(gtk_hbox_new(FALSE, 0)),
      button_hbox_(gtk_chrome_shrinkable_hbox_new(TRUE, FALSE, kButtonPadding)),
      overflow_button_(browser->profile()),
      drag_button_(NULL),
      drop_index_(-1),
      resize_animation_(this),
      desired_width_(0),
      start_width_(0),
      method_factory_(this) {
  ExtensionsService* extension_service = profile_->GetExtensionsService();
  // The |extension_service| can be NULL in Incognito.
  if (!extension_service)
    return;

  GtkWidget* gripper = gtk_button_new();
  GTK_WIDGET_UNSET_FLAGS(gripper, GTK_CAN_FOCUS);
  gtk_widget_add_events(gripper, GDK_POINTER_MOTION_MASK);
  g_signal_connect(gripper, "motion-notify-event",
                   G_CALLBACK(OnGripperMotionNotifyThunk), this);
  g_signal_connect(gripper, "expose-event",
                   G_CALLBACK(OnGripperExposeThunk), this);
  g_signal_connect(gripper, "enter-notify-event",
                   G_CALLBACK(OnGripperEnterNotifyThunk), this);
  g_signal_connect(gripper, "leave-notify-event",
                   G_CALLBACK(OnGripperLeaveNotifyThunk), this);
  g_signal_connect(gripper, "button-release-event",
                   G_CALLBACK(OnGripperButtonReleaseThunk), this);
  g_signal_connect(gripper, "button-press-event",
                   G_CALLBACK(OnGripperButtonPressThunk), this);
  g_signal_connect(overflow_button_.widget(), "button-press-event",
                   G_CALLBACK(OnOverflowButtonPressThunk), this);

  gtk_box_pack_start(GTK_BOX(hbox_.get()), gripper, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox_.get()), button_hbox_, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox_.get()), overflow_button_.widget(),
                     FALSE, FALSE, 0);

  model_ = extension_service->toolbar_model();
  model_->AddObserver(this);
  SetupDrags();
  CreateAllButtons();

  // We want to connect to "set-focus" on the toplevel window; we have to wait
  // until we are added to a toplevel window to do so.
  g_signal_connect(widget(), "hierarchy-changed",
                   G_CALLBACK(OnHierarchyChangedThunk), this);

  int showing_actions = model_->GetVisibleIconCount();
  if (showing_actions >= 0)
    SetButtonHBoxWidth(WidthForIconCount(showing_actions));

  ViewIDUtil::SetID(button_hbox_, VIEW_ID_BROWSER_ACTION_TOOLBAR);
}

BrowserActionsToolbarGtk::~BrowserActionsToolbarGtk() {
  GtkWidget* toplevel = gtk_widget_get_toplevel(widget());
  if (toplevel) {
    g_signal_handlers_disconnect_by_func(
        toplevel, reinterpret_cast<gpointer>(OnSetFocusThunk), this);
  }

  if (model_)
    model_->RemoveObserver(this);
  hbox_.Destroy();
}

int BrowserActionsToolbarGtk::GetCurrentTabId() {
  TabContents* selected_tab = browser_->GetSelectedTabContents();
  if (!selected_tab)
    return -1;

  return selected_tab->controller().session_id().id();
}

void BrowserActionsToolbarGtk::Update() {
  for (ExtensionButtonMap::iterator iter = extension_button_map_.begin();
       iter != extension_button_map_.end(); ++iter) {
    iter->second->UpdateState();
  }
}

void BrowserActionsToolbarGtk::SetupDrags() {
  GtkTargetEntry drag_target = GetDragTargetEntry();
  gtk_drag_dest_set(button_hbox_, GTK_DEST_DEFAULT_DROP, &drag_target, 1,
                    GDK_ACTION_MOVE);

  g_signal_connect(button_hbox_, "drag-motion",
                   G_CALLBACK(OnDragMotionThunk), this);
}

void BrowserActionsToolbarGtk::CreateAllButtons() {
  extension_button_map_.clear();

  int i = 0;
  for (ExtensionList::iterator iter = model_->begin();
       iter != model_->end(); ++iter) {
    CreateButtonForExtension(*iter, i++);
  }
}

void BrowserActionsToolbarGtk::CreateButtonForExtension(Extension* extension,
                                                        int index) {
  if (!ShouldDisplayBrowserAction(extension))
    return;

  if (profile_->IsOffTheRecord())
    index = model_->OriginalIndexToIncognito(index);

  RemoveButtonForExtension(extension);
  linked_ptr<BrowserActionButton> button(
      new BrowserActionButton(this, extension));
  gtk_chrome_shrinkable_hbox_pack_start(
      GTK_CHROME_SHRINKABLE_HBOX(button_hbox_), button->widget(), 0);
  gtk_box_reorder_child(GTK_BOX(button_hbox_), button->widget(), index);
  gtk_widget_show(button->widget());
  extension_button_map_[extension->id()] = button;

  GtkTargetEntry drag_target = GetDragTargetEntry();
  gtk_drag_source_set(button->widget(), GDK_BUTTON1_MASK, &drag_target, 1,
                      GDK_ACTION_MOVE);
  // We ignore whether the drag was a "success" or "failure" in Gtk's opinion.
  g_signal_connect(button->widget(), "drag-end",
                   G_CALLBACK(&OnDragEndThunk), this);
  g_signal_connect(button->widget(), "drag-failed",
                   G_CALLBACK(&OnDragFailedThunk), this);

  UpdateVisibility();
}

GtkWidget* BrowserActionsToolbarGtk::GetBrowserActionWidget(
    Extension* extension) {
  ExtensionButtonMap::iterator it = extension_button_map_.find(
      extension->id());
  if (it == extension_button_map_.end())
    return NULL;

  return it->second.get()->widget();
}

void BrowserActionsToolbarGtk::RemoveButtonForExtension(Extension* extension) {
  if (extension_button_map_.erase(extension->id()))
    UpdateVisibility();
}

void BrowserActionsToolbarGtk::UpdateVisibility() {
  if (button_count() == 0)
    gtk_widget_hide(widget());
  else
    gtk_widget_show(widget());
}

bool BrowserActionsToolbarGtk::ShouldDisplayBrowserAction(
    Extension* extension) {
  // Only display incognito-enabled extensions while in incognito mode.
  return (!profile_->IsOffTheRecord() ||
          profile_->GetExtensionsService()->IsIncognitoEnabled(extension));
}

void BrowserActionsToolbarGtk::HidePopup() {
  ExtensionPopupGtk* popup = ExtensionPopupGtk::get_current_extension_popup();
  if (popup)
    popup->DestroyPopup();
}

void BrowserActionsToolbarGtk::AnimateToShowNIcons(int count) {
  desired_width_ = WidthForIconCount(count);
  start_width_ = button_hbox_->allocation.width;
  resize_animation_.Reset();
  resize_animation_.Show();
}

void BrowserActionsToolbarGtk::ButtonAddedOrRemoved() {
  // TODO(estade): this is a little bit janky looking when the removed button
  // is not the farthest right button.
  if (!GTK_WIDGET_VISIBLE(overflow_button_.widget())) {
    AnimateToShowNIcons(button_count());
    model_->SetVisibleIconCount(button_count());
  }
}

void BrowserActionsToolbarGtk::BrowserActionAdded(Extension* extension,
                                                  int index) {
  CreateButtonForExtension(extension, index);
  ButtonAddedOrRemoved();
}

void BrowserActionsToolbarGtk::BrowserActionRemoved(Extension* extension) {
  if (drag_button_ != NULL) {
    // Break the current drag.
    gtk_grab_remove(button_hbox_);
  }

  RemoveButtonForExtension(extension);
  ButtonAddedOrRemoved();
}

void BrowserActionsToolbarGtk::BrowserActionMoved(Extension* extension,
                                                  int index) {
  // We initiated this move action, and have already moved the button.
  if (drag_button_ != NULL)
    return;

  BrowserActionButton* button = extension_button_map_[extension->id()].get();
  if (!button) {
    if (ShouldDisplayBrowserAction(extension))
      NOTREACHED();
    return;
  }

  if (profile_->IsOffTheRecord())
    index = model_->OriginalIndexToIncognito(index);

  gtk_box_reorder_child(GTK_BOX(button_hbox_), button->widget(), index);
}

void BrowserActionsToolbarGtk::AnimationProgressed(const Animation* animation) {
  int width = start_width_ + (desired_width_ - start_width_) *
      animation->GetCurrentValue();
  gtk_widget_set_size_request(button_hbox_, width, -1);

  if (width == desired_width_)
    resize_animation_.Reset();
}

void BrowserActionsToolbarGtk::AnimationEnded(const Animation* animation) {
  gtk_widget_set_size_request(button_hbox_, desired_width_, -1);
}

void BrowserActionsToolbarGtk::ExecuteCommandById(int command_id) {
  Extension* extension = model_->GetExtensionByIndex(command_id);
  ExtensionAction* browser_action = extension->browser_action();

  int tab_id = GetCurrentTabId();
  if (tab_id < 0) {
    NOTREACHED() << "No current tab.";
    return;
  }

  if (browser_action->HasPopup(tab_id)) {
    ExtensionPopupGtk::Show(
        browser_action->GetPopupUrl(tab_id), browser(),
        gtk_util::GetWidgetRectRelativeToToplevel(overflow_button_.widget()));
  } else {
    ExtensionBrowserEventRouter::GetInstance()->BrowserActionExecuted(
        browser()->profile(), extension->id(), browser());
  }
}

void BrowserActionsToolbarGtk::StoppedShowing() {
  gtk_chrome_button_unset_paint_state(
      GTK_CHROME_BUTTON(overflow_button_.widget()));
}

void BrowserActionsToolbarGtk::DragStarted(BrowserActionButton* button,
                                           GdkDragContext* drag_context) {
  // No representation of the widget following the cursor.
  GdkPixbuf* pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, 1, 1);
  gtk_drag_set_icon_pixbuf(drag_context, pixbuf, 0, 0);
  g_object_unref(pixbuf);

  DCHECK(!drag_button_);
  drag_button_ = button;
}

void BrowserActionsToolbarGtk::SetButtonHBoxWidth(int new_width) {
  gint max_width = WidthForIconCount(model_->size());
  new_width = std::min(max_width, new_width);
  new_width = std::max(new_width, 0);
  gtk_widget_set_size_request(button_hbox_, new_width, -1);

  int showing_icon_count =
      gtk_chrome_shrinkable_hbox_get_visible_child_count(
          GTK_CHROME_SHRINKABLE_HBOX(button_hbox_));

  model_->SetVisibleIconCount(showing_icon_count);
  if (model_->size() > static_cast<size_t>(showing_icon_count)) {
    if (!GTK_WIDGET_VISIBLE(overflow_button_.widget())) {
      // When the overflow chevron shows for the first time, take that
      // much space away from |button_hbox_| to make the drag look smoother.
      GtkRequisition req;
      gtk_widget_size_request(overflow_button_.widget(), &req);
      new_width -= req.width;
      new_width = std::max(new_width, 0);
      gtk_widget_set_size_request(button_hbox_, new_width, -1);

      gtk_widget_show(overflow_button_.widget());
    }
  } else {
    gtk_widget_hide(overflow_button_.widget());
  }
}

gboolean BrowserActionsToolbarGtk::OnDragMotion(GtkWidget* widget,
                                                GdkDragContext* drag_context,
                                                gint x, gint y, guint time) {
  // Only handle drags we initiated.
  if (!drag_button_)
    return FALSE;

  drop_index_ = x < kButtonSize ? 0 : x / (kButtonSize + kButtonPadding);

  // We will go ahead and reorder the child in order to provide visual feedback
  // to the user. We don't inform the model that it has moved until the drag
  // ends.
  gtk_box_reorder_child(GTK_BOX(button_hbox_), drag_button_->widget(),
                        drop_index_);

  gdk_drag_status(drag_context, GDK_ACTION_MOVE, time);
  return TRUE;
}

void BrowserActionsToolbarGtk::OnDragEnd(GtkWidget* button,
                                         GdkDragContext* drag_context) {
  if (drop_index_ != -1) {
    if (profile_->IsOffTheRecord())
      drop_index_ = model_->IncognitoIndexToOriginal(drop_index_);

    model_->MoveBrowserAction(drag_button_->extension(), drop_index_);
  }

  drag_button_ = NULL;
  drop_index_ = -1;
}

gboolean BrowserActionsToolbarGtk::OnDragFailed(GtkWidget* widget,
                                                GdkDragContext* drag_context,
                                                GtkDragResult result) {
  // We connect to this signal and return TRUE so that the default failure
  // animation (wherein the drag widget floats back to the start of the drag)
  // does not show, and the drag-end signal is emitted immediately instead of
  // several seconds later.
  return TRUE;
}

void BrowserActionsToolbarGtk::OnHierarchyChanged(GtkWidget* widget,
                                                  GtkWidget* previous_toplevel) {
  GtkWidget* toplevel = gtk_widget_get_toplevel(widget);
  if (!GTK_WIDGET_TOPLEVEL(toplevel))
    return;

  g_signal_connect(toplevel, "set-focus", G_CALLBACK(OnSetFocusThunk), this);
}

void BrowserActionsToolbarGtk::OnSetFocus(GtkWidget* widget,
                                          GtkWidget* focus_widget) {
  // The focus of the parent window has changed. Close the popup. Delay the hide
  // because it will destroy the RenderViewHost, which may still be on the
  // call stack.
  if (!ExtensionPopupGtk::get_current_extension_popup())
    return;
  MessageLoop::current()->PostTask(FROM_HERE,
      method_factory_.NewRunnableMethod(&BrowserActionsToolbarGtk::HidePopup));
}

gboolean BrowserActionsToolbarGtk::OnGripperMotionNotify(
    GtkWidget* widget, GdkEventMotion* event) {
  if (!(event->state & GDK_BUTTON1_MASK))
    return FALSE;

  gint new_width = button_hbox_->allocation.width -
                   (event->x - widget->allocation.width);
  SetButtonHBoxWidth(new_width);

  return FALSE;
}

gboolean BrowserActionsToolbarGtk::OnGripperExpose(GtkWidget* gripper,
                                                   GdkEventExpose* expose) {
  cairo_t* cr = gdk_cairo_create(GDK_DRAWABLE(expose->window));

  CairoCachedSurface* surface = theme_provider_->GetSurfaceNamed(
      IDR_RESIZE_GRIPPER, gripper);
  gfx::Point center = gfx::Rect(gripper->allocation).CenterPoint();
  center.Offset(-surface->Width() / 2, -surface->Height() / 2);
  surface->SetSource(cr, center.x(), center.y());
  gdk_cairo_rectangle(cr, &expose->area);
  cairo_fill(cr);

  cairo_destroy(cr);

  return TRUE;
}

// These three signal handlers (EnterNotify, LeaveNotify, and ButtonRelease)
// are used to give the gripper the resize cursor. Since it doesn't have its
// own window, we have to set the cursor whenever the pointer moves into the
// button or leaves the button, and be sure to leave it on when the user is
// dragging.
gboolean BrowserActionsToolbarGtk::OnGripperEnterNotify(
    GtkWidget* gripper, GdkEventCrossing* event) {
  gdk_window_set_cursor(gripper->window,
                        gtk_util::GetCursor(GDK_SB_H_DOUBLE_ARROW));
  return FALSE;
}

gboolean BrowserActionsToolbarGtk::OnGripperLeaveNotify(
    GtkWidget* gripper, GdkEventCrossing* event) {
  if (!(event->state & GDK_BUTTON1_MASK))
    gdk_window_set_cursor(gripper->window, NULL);
  return FALSE;
}

gboolean BrowserActionsToolbarGtk::OnGripperButtonRelease(
    GtkWidget* gripper, GdkEventButton* event) {
  gfx::Rect gripper_rect(0, 0,
                         gripper->allocation.width, gripper->allocation.height);
  gfx::Point release_point(event->x, event->y);
  if (!gripper_rect.Contains(release_point))
    gdk_window_set_cursor(gripper->window, NULL);

  // After the user resizes the toolbar, we want to smartly resize it to be
  // the perfect size to fit the buttons.
  int visible_icon_count =
      gtk_chrome_shrinkable_hbox_get_visible_child_count(
          GTK_CHROME_SHRINKABLE_HBOX(button_hbox_));
  AnimateToShowNIcons(visible_icon_count);

  return FALSE;
}

gboolean BrowserActionsToolbarGtk::OnGripperButtonPress(
    GtkWidget* gripper, GdkEventButton* event) {
  resize_animation_.Reset();

  return FALSE;
}

gboolean BrowserActionsToolbarGtk::OnOverflowButtonPress(
    GtkWidget* overflow, GdkEventButton* event) {
  overflow_menu_.reset(new MenuGtk(this));

  int visible_icon_count =
      gtk_chrome_shrinkable_hbox_get_visible_child_count(
          GTK_CHROME_SHRINKABLE_HBOX(button_hbox_));
  for (int i = visible_icon_count; i < static_cast<int>(model_->size()); ++i) {
    Extension* extension = model_->GetExtensionByIndex(i);
    BrowserActionButton* button = extension_button_map_[extension->id()].get();

    overflow_menu_->AppendMenuItemWithIcon(
        i,
        extension->name(),
        button->GetIcon());

    // TODO(estade): set the menu item's tooltip.
  }

  gtk_chrome_button_set_paint_state(GTK_CHROME_BUTTON(overflow),
                                    GTK_STATE_ACTIVE);
  overflow_menu_->PopupAsFromKeyEvent(overflow);

  return FALSE;
}
