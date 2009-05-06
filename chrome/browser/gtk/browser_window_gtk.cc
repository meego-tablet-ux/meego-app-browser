// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/gtk/browser_window_gtk.h"

#include <gdk/gdkkeysyms.h>

#include "app/resource_bundle.h"
#include "base/base_paths_linux.h"
#include "base/command_line.h"
#include "base/gfx/gtk_util.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "base/string_util.h"
#include "base/time.h"
#include "chrome/app/chrome_dll_resource.h"
#include "chrome/browser/bookmarks/bookmark_utils.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/gtk/about_chrome_dialog.h"
#include "chrome/browser/gtk/bookmark_bar_gtk.h"
#include "chrome/browser/gtk/browser_toolbar_gtk.h"
#include "chrome/browser/gtk/go_button_gtk.h"
#include "chrome/browser/gtk/infobar_container_gtk.h"
#include "chrome/browser/gtk/find_bar_gtk.h"
#include "chrome/browser/gtk/status_bubble_gtk.h"
#include "chrome/browser/gtk/tab_contents_container_gtk.h"
#include "chrome/browser/gtk/tabs/tab_strip_gtk.h"
#include "chrome/browser/gtk/toolbar_star_toggle_gtk.h"
#include "chrome/browser/location_bar.h"
#include "chrome/browser/renderer_host/render_widget_host_view_gtk.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/browser/tab_contents/tab_contents_view.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/notification_service.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/pref_service.h"
#include "chrome/views/controls/button/text_button.h"
#include "grit/theme_resources.h"

namespace {

// The number of milliseconds between loading animation frames.
const int kLoadingAnimationFrameTimeMs = 30;

const GdkColor kBorderColor = GDK_COLOR_RGB(0xbe, 0xc8, 0xd4);

class DummyButtonListener : public views::ButtonListener {
 public:
  virtual void ButtonPressed(views::Button* sender) {
    DLOG(ERROR) << "Button Pressed!";
  }
};

gboolean MainWindowConfigured(GtkWindow* window, GdkEventConfigure* event,
                              BrowserWindowGtk* browser_win) {
  gfx::Rect bounds = gfx::Rect(event->x, event->y, event->width, event->height);
  browser_win->OnBoundsChanged(bounds);
  return FALSE;
}

gboolean MainWindowStateChanged(GtkWindow* window, GdkEventWindowState* event,
                                BrowserWindowGtk* browser_win) {
  browser_win->OnStateChanged(event->new_window_state);
  return FALSE;
}

// Callback for the delete event.  This event is fired when the user tries to
// close the window (e.g., clicking on the X in the window manager title bar).
gboolean MainWindowDeleteEvent(GtkWidget* widget, GdkEvent* event,
                               BrowserWindowGtk* window) {
  window->Close();

  // Return true to prevent the gtk window from being destroyed.  Close will
  // destroy it for us.
  return TRUE;
}

void MainWindowDestroy(GtkWidget* widget, BrowserWindowGtk* window) {
  // BUG 8712. When we gtk_widget_destroy() in Close(), this will emit the
  // signal right away, and we will be here (while Close() is still in the
  // call stack).  In order to not reenter Close(), and to also follow the
  // expectations of BrowserList, we should run the BrowserWindowGtk destructor
  // not now, but after the run loop goes back to process messages.  Otherwise
  // we will remove ourself from BrowserList while it's being iterated.
  // Additionally, now that we know the window is gone, we need to make sure to
  // set window_ to NULL, otherwise we will try to close the window again when
  // we call Close() in the destructor.
  MessageLoop::current()->DeleteSoon(FROM_HERE, window);
}

// Using gtk_window_get_position/size creates a race condition, so only use
// this to get the initial bounds.  After window creation, we pick up the
// normal bounds by connecting to the configure-event signal.
gfx::Rect GetInitialWindowBounds(GtkWindow* window) {
  gint x, y, width, height;
  gtk_window_get_position(window, &x, &y);
  gtk_window_get_size(window, &width, &height);
  return gfx::Rect(x, y, width, height);
}

const struct AcceleratorMapping {
  guint keyval;
  int command_id;
  GdkModifierType modifier_type;
} kAcceleratorMap[] = {
  // Focus.
  { GDK_k, IDC_FOCUS_SEARCH, GDK_CONTROL_MASK },
  { GDK_l, IDC_FOCUS_LOCATION, GDK_CONTROL_MASK },
  { GDK_d, IDC_FOCUS_LOCATION, GDK_MOD1_MASK },
  { GDK_F6, IDC_FOCUS_LOCATION, GdkModifierType(0) },

  // Tab/window controls.
  { GDK_Page_Down, IDC_SELECT_NEXT_TAB, GDK_CONTROL_MASK },
  { GDK_Page_Up, IDC_SELECT_PREVIOUS_TAB, GDK_CONTROL_MASK },
  { GDK_w, IDC_CLOSE_TAB, GDK_CONTROL_MASK },
  { GDK_t, IDC_RESTORE_TAB,
    GdkModifierType(GDK_CONTROL_MASK | GDK_SHIFT_MASK) },
  { GDK_1, IDC_SELECT_TAB_0, GDK_CONTROL_MASK },
  { GDK_2, IDC_SELECT_TAB_1, GDK_CONTROL_MASK },
  { GDK_3, IDC_SELECT_TAB_2, GDK_CONTROL_MASK },
  { GDK_4, IDC_SELECT_TAB_3, GDK_CONTROL_MASK },
  { GDK_5, IDC_SELECT_TAB_4, GDK_CONTROL_MASK },
  { GDK_6, IDC_SELECT_TAB_5, GDK_CONTROL_MASK },
  { GDK_7, IDC_SELECT_TAB_6, GDK_CONTROL_MASK },
  { GDK_8, IDC_SELECT_TAB_7, GDK_CONTROL_MASK },
  { GDK_9, IDC_SELECT_LAST_TAB, GDK_CONTROL_MASK },
  { GDK_w, IDC_CLOSE_TAB, GDK_CONTROL_MASK },
  { GDK_F4, IDC_CLOSE_TAB, GDK_CONTROL_MASK },
  { GDK_F4, IDC_CLOSE_WINDOW, GDK_MOD1_MASK },

  // Zoom level.
  { GDK_plus, IDC_ZOOM_PLUS,
    GdkModifierType(GDK_CONTROL_MASK | GDK_SHIFT_MASK) },
  { GDK_equal, IDC_ZOOM_PLUS, GDK_CONTROL_MASK },
  { GDK_0, IDC_ZOOM_NORMAL, GDK_CONTROL_MASK },
  { GDK_minus, IDC_ZOOM_MINUS, GDK_CONTROL_MASK },
  { GDK_underscore, IDC_ZOOM_MINUS,
    GdkModifierType(GDK_CONTROL_MASK | GDK_SHIFT_MASK) },

  // Find in page.
  { GDK_g, IDC_FIND_NEXT, GDK_CONTROL_MASK },
  { GDK_F3, IDC_FIND_NEXT, GdkModifierType(0) },
  { GDK_g, IDC_FIND_PREVIOUS,
    GdkModifierType(GDK_CONTROL_MASK | GDK_SHIFT_MASK) },
  { GDK_F3, IDC_FIND_PREVIOUS, GDK_SHIFT_MASK },

  // Navigation.
  { GDK_Home, IDC_HOME, GDK_MOD1_MASK },
  { GDK_Escape, IDC_STOP, GdkModifierType(0) },

  // Miscellany.
  { GDK_d, IDC_STAR, GDK_CONTROL_MASK },
  { GDK_o, IDC_OPEN_FILE, GDK_CONTROL_MASK },
  { GDK_F11, IDC_FULLSCREEN, GdkModifierType(0) },
  { GDK_u, IDC_VIEW_SOURCE, GDK_CONTROL_MASK },
  { GDK_p, IDC_PRINT, GDK_CONTROL_MASK },
  { GDK_Escape, IDC_TASK_MANAGER, GDK_SHIFT_MASK },
};

int GetCommandId(guint accel_key, GdkModifierType modifier) {
  // Bug 9806: If capslock is on, we will get a capital letter as accel_key.
  accel_key = gdk_keyval_to_lower(accel_key);
  // Filter modifier to only include accelerator modifiers.
  modifier = static_cast<GdkModifierType>(
      modifier & gtk_accelerator_get_default_mod_mask());
  for (size_t i = 0; i < arraysize(kAcceleratorMap); ++i) {
    if (kAcceleratorMap[i].keyval == accel_key &&
        kAcceleratorMap[i].modifier_type == modifier)
      return kAcceleratorMap[i].command_id;
  }
  NOTREACHED();
  return 0;
}

// An event handler for key press events.  We need to special case key
// combinations that are not valid gtk accelerators.  This function returns
// TRUE if it can handle the key press.
gboolean HandleCustomAccelerator(guint keyval, GdkModifierType modifier,
                                 Browser* browser) {
  // Filter modifier to only include accelerator modifiers.
  modifier = static_cast<GdkModifierType>(
      modifier & gtk_accelerator_get_default_mod_mask());
  switch (keyval) {
    // Gtk doesn't allow GDK_Tab or GDK_ISO_Left_Tab to be an accelerator (see
    // gtk_accelerator_valid), so we need to handle these accelerators
    // manually.
    case GDK_Tab:
      if (GDK_CONTROL_MASK == modifier) {
        browser->ExecuteCommand(IDC_SELECT_NEXT_TAB);
        return TRUE;
      }
      break;

    case GDK_ISO_Left_Tab:
      if ((GDK_CONTROL_MASK | GDK_SHIFT_MASK) == modifier) {
        browser->ExecuteCommand(IDC_SELECT_PREVIOUS_TAB);
        return TRUE;
      }
      break;

    default:
      break;
  }
  return FALSE;
}

// Usually accelerators are checked before propagating the key event, but if the
// focus is on the render area we want to reverse the order of things to allow
// webkit to handle key events like ctrl-l.
gboolean OnKeyPress(GtkWindow* window, GdkEventKey* event, Browser* browser) {
  TabContents* current_tab_contents =
      browser->tabstrip_model()->GetSelectedTabContents();
  // If there is no current tab contents or it is not focused then let the
  // default GtkWindow key handler run.
  if (!current_tab_contents ||
      !gtk_widget_is_focus(current_tab_contents->GetContentNativeView())) {
    return HandleCustomAccelerator(event->keyval,
        static_cast<GdkModifierType>(event->state), browser);
  }

  // If the content area is focused, let it handle the key event.
  gboolean result = gtk_window_propagate_key_event(window, event);
  DCHECK(result);
  return TRUE;
}

gboolean OnButtonPressEvent(GtkWidget* widget, GdkEventButton* event,
                            Browser* browser) {
  // TODO(jhawkins): Investigate the possibility of the button numbers being
  // different for other mice.
  if (event->button == 8) {
    browser->GoBack(CURRENT_TAB);
    return TRUE;
  } else if (event->button == 9) {
    browser->GoForward(CURRENT_TAB);
    return TRUE;
  }
  return FALSE;
}

}  // namespace

// TODO(estade): Break up this constructor into helper functions to improve
// readability.
BrowserWindowGtk::BrowserWindowGtk(Browser* browser)
    :  browser_(browser),
       // TODO(port): make this a pref.
       custom_frame_(false),
       full_screen_(false),
       method_factory_(this) {
  window_ = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
  SetWindowIcon();
  SetGeometryHints();
  g_object_set_data(G_OBJECT(window_), "browser_window_gtk", this);
  g_signal_connect(window_, "delete-event",
                   G_CALLBACK(MainWindowDeleteEvent), this);
  g_signal_connect(window_, "destroy",
                   G_CALLBACK(MainWindowDestroy), this);
  g_signal_connect(window_, "configure-event",
                   G_CALLBACK(MainWindowConfigured), this);
  g_signal_connect(window_, "window-state-event",
                   G_CALLBACK(MainWindowStateChanged), this);
  g_signal_connect(window_, "key-press-event",
                   G_CALLBACK(OnKeyPress), browser_.get());
  g_signal_connect(window_, "button-press-event",
                   G_CALLBACK(OnButtonPressEvent), browser_.get());
  ConnectAccelerators();
  bounds_ = GetInitialWindowBounds(window_);

  // This vbox encompasses all of the widgets within the browser, including
  // the tabstrip and the content vbox.
  window_vbox_ = gtk_vbox_new(FALSE, 0);

  tabstrip_.reset(new TabStripGtk(browser_->tabstrip_model()));
  tabstrip_->Init(bounds_.width());
  tabstrip_->AddTabStripToBox(window_vbox_);

  // This vbox surrounds the "content": toolbar+page.
  content_vbox_ = gtk_vbox_new(FALSE, 0);
  gtk_widget_set_app_paintable(content_vbox_, TRUE);
  gtk_widget_set_double_buffered(content_vbox_, FALSE);
  g_signal_connect(G_OBJECT(content_vbox_), "expose-event",
                   G_CALLBACK(&OnContentAreaExpose), this);

  // Temporary hack hidden behind a command line option to add one of the
  // experimental ViewsGtk objects to the Gtk hierarchy.
  const CommandLine& parsed_command_line = *CommandLine::ForCurrentProcess();
  if (parsed_command_line.HasSwitch(switches::kViewsGtk)) {
    experimental_widget_.reset(new views::WidgetGtk());
    experimental_widget_->Init(gfx::Rect(), false);
    experimental_widget_->SetContentsView(
        new views::TextButton(new DummyButtonListener, L"Button"));

    gtk_box_pack_start(GTK_BOX(content_vbox_),
                       experimental_widget_->GetNativeView(),
                       false, false, 2);
  }

  toolbar_.reset(new BrowserToolbarGtk(browser_.get()));
  toolbar_->Init(browser_->profile(), window_);
  toolbar_->AddToolbarToBox(content_vbox_);

  bookmark_bar_.reset(new BookmarkBarGtk(browser_->profile(), browser_.get()));
  bookmark_bar_->AddBookmarkbarToBox(content_vbox_);

  // This vbox surrounds the render area: find bar, info bars and render view.
  // The reason is that this area as a whole needs to be grouped in its own
  // GdkWindow hierarchy so that animations originating inside it (infobar,
  // download shelf, find bar) are all clipped to that area. This is why
  // |render_area_vbox_| is packed in |event_box|.
  render_area_vbox_ = gtk_vbox_new(FALSE, 0);
  infobar_container_.reset(new InfoBarContainerGtk(this));
  gtk_box_pack_start(GTK_BOX(render_area_vbox_),
                     infobar_container_->widget(),
                     FALSE, FALSE, 0);

  contents_container_.reset(new TabContentsContainerGtk());
  contents_container_->AddContainerToBox(render_area_vbox_);

  // Note that calling this the first time is necessary to get the
  // proper control layout.
  // TODO(port): make this a pref.
  SetCustomFrame(false);

  status_bubble_.reset(new StatusBubbleGtk(window_));

  GtkWidget* event_box = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(event_box), render_area_vbox_);
  gtk_container_add(GTK_CONTAINER(content_vbox_), event_box);
  gtk_container_add(GTK_CONTAINER(window_vbox_), content_vbox_);
  gtk_container_add(GTK_CONTAINER(window_), window_vbox_);
  gtk_widget_show_all(window_vbox_);
  browser_->tabstrip_model()->AddObserver(this);

  NotificationService* ns = NotificationService::current();
  ns->AddObserver(this, NotificationType::BOOKMARK_BAR_VISIBILITY_PREF_CHANGED,
                  NotificationService::AllSources());
}

BrowserWindowGtk::~BrowserWindowGtk() {
  NotificationService* ns = NotificationService::current();
  ns->RemoveObserver(this,
                     NotificationType::BOOKMARK_BAR_VISIBILITY_PREF_CHANGED,
                     NotificationService::AllSources());

  browser_->tabstrip_model()->RemoveObserver(this);
}

void BrowserWindowGtk::HandleAccelerator(guint keyval,
                                         GdkModifierType modifier) {
  if (!HandleCustomAccelerator(keyval, modifier, browser_.get())) {
    // Pass the accelerator on to GTK.
    gtk_accel_groups_activate(G_OBJECT(window_), keyval, modifier);
  }
}

gboolean BrowserWindowGtk::OnContentAreaExpose(GtkWidget* widget,
                                               GdkEventExpose* e,
                                               BrowserWindowGtk* window) {
  if (window->custom_frame_) {
    NOTIMPLEMENTED() << " needs custom drawing for the custom frame.";
    return FALSE;
  }

  return FALSE;  // Allow subwidgets to paint.
}

void BrowserWindowGtk::Show() {
  gtk_widget_show(GTK_WIDGET(window_));
}

void BrowserWindowGtk::SetBounds(const gfx::Rect& bounds) {
  gint x = static_cast<gint>(bounds.x());
  gint y = static_cast<gint>(bounds.y());
  gint width = static_cast<gint>(bounds.width());
  gint height = static_cast<gint>(bounds.height());

  gtk_window_move(window_, x, y);
  gtk_window_resize(window_, width, height);
}

void BrowserWindowGtk::Close() {
  if (!CanClose())
    return;

  // TODO(tc): We should store the window position, perhaps using
  // gtk_window_set_role.
  // SaveWindowPosition();

  GtkWidget* window = GTK_WIDGET(window_);
  // To help catch bugs in any event handlers that might get fired during the
  // destruction, set window_ to NULL before any handlers will run.
  window_ = NULL;
  gtk_widget_destroy(window);
}

void BrowserWindowGtk::Activate() {
  gtk_window_present(window_);
}

bool BrowserWindowGtk::IsActive() const {
  NOTIMPLEMENTED();
  return true;
}

void BrowserWindowGtk::FlashFrame() {
  // May not be respected by all window managers.
  gtk_window_set_urgency_hint(window_, TRUE);
}

gfx::NativeWindow BrowserWindowGtk::GetNativeHandle() {
  return window_;
}

BrowserWindowTesting* BrowserWindowGtk::GetBrowserWindowTesting() {
  NOTIMPLEMENTED();
  return NULL;
}

StatusBubble* BrowserWindowGtk::GetStatusBubble() {
  return status_bubble_.get();
}

void BrowserWindowGtk::SelectedTabToolbarSizeChanged(bool is_animating) {
  NOTIMPLEMENTED();
}

void BrowserWindowGtk::UpdateTitleBar() {
  std::wstring title = browser_->GetCurrentPageTitle();
  gtk_window_set_title(window_, WideToUTF8(title).c_str());
  if (ShouldShowWindowIcon()) {
    // If we're showing a title bar, we should update the app icon.
    NOTIMPLEMENTED();
  }
}

void BrowserWindowGtk::UpdateLoadingAnimations(bool should_animate) {
  if (should_animate) {
    if (!loading_animation_timer_.IsRunning()) {
      // Loads are happening, and the timer isn't running, so start it.
      loading_animation_timer_.Start(
          base::TimeDelta::FromMilliseconds(kLoadingAnimationFrameTimeMs), this,
          &BrowserWindowGtk::LoadingAnimationCallback);
    }
  } else {
    if (loading_animation_timer_.IsRunning()) {
      loading_animation_timer_.Stop();
      // Loads are now complete, update the state if a task was scheduled.
      LoadingAnimationCallback();
    }
  }
}

void BrowserWindowGtk::LoadingAnimationCallback() {
  if (browser_->type() == Browser::TYPE_NORMAL) {
    // Loading animations are shown in the tab for tabbed windows.  We check the
    // browser type instead of calling IsTabStripVisible() because the latter
    // will return false for fullscreen windows, but we still need to update
    // their animations (so that when they come out of fullscreen mode they'll
    // be correct).
    tabstrip_->UpdateLoadingAnimations();
  } else if (ShouldShowWindowIcon()) {
    // ... or in the window icon area for popups and app windows.
    // http://code.google.com/p/chromium/issues/detail?id=9380
    NOTIMPLEMENTED();
  }
}

void BrowserWindowGtk::SetStarredState(bool is_starred) {
  toolbar_->star()->SetStarred(is_starred);
}

gfx::Rect BrowserWindowGtk::GetNormalBounds() const {
  return bounds_;
}

bool BrowserWindowGtk::IsMaximized() const {
  return (state_ & GDK_WINDOW_STATE_MAXIMIZED);
}

void BrowserWindowGtk::SetFullscreen(bool fullscreen) {
  if (fullscreen) {
    full_screen_ = true;
    tabstrip_->Hide();
    toolbar_->Hide();
    gtk_window_fullscreen(window_);
  } else {
    full_screen_ = false;
    gtk_window_unfullscreen(window_);
    toolbar_->Show();
    tabstrip_->Show();
  }
}

bool BrowserWindowGtk::IsFullscreen() const {
  return full_screen_;
}

LocationBar* BrowserWindowGtk::GetLocationBar() const {
  return toolbar_->GetLocationBar();
}

void BrowserWindowGtk::SetFocusToLocationBar() {
  GetLocationBar()->FocusLocation();
}

void BrowserWindowGtk::UpdateStopGoState(bool is_loading, bool force) {
  toolbar_->GetGoButton()->ChangeMode(
      is_loading ? GoButtonGtk::MODE_STOP : GoButtonGtk::MODE_GO, force);
}

void BrowserWindowGtk::UpdateToolbar(TabContents* contents,
                                     bool should_restore_state) {
  toolbar_->UpdateTabContents(contents, should_restore_state);
}

void BrowserWindowGtk::FocusToolbar() {
  NOTIMPLEMENTED();
}

bool BrowserWindowGtk::IsBookmarkBarVisible() const {
  return browser_->SupportsWindowFeature(Browser::FEATURE_BOOKMARKBAR) &&
      bookmark_bar_.get();
}

gfx::Rect BrowserWindowGtk::GetRootWindowResizerRect() const {
  return gfx::Rect();
}

void BrowserWindowGtk::ToggleBookmarkBar() {
  bookmark_utils::ToggleWhenVisible(browser_->profile());
}

void BrowserWindowGtk::ShowAboutChromeDialog() {
  ShowAboutDialogForProfile(window_, browser_->profile());
}

void BrowserWindowGtk::ShowBookmarkManager() {
  NOTIMPLEMENTED();
}

void BrowserWindowGtk::ShowBookmarkBubble(const GURL& url,
                                          bool already_bookmarked) {
  toolbar_->star()->ShowStarBubble(url, !already_bookmarked);
}

void BrowserWindowGtk::ShowReportBugDialog() {
  NOTIMPLEMENTED();
}

void BrowserWindowGtk::ShowClearBrowsingDataDialog() {
  NOTIMPLEMENTED();
}

void BrowserWindowGtk::ShowImportDialog() {
  NOTIMPLEMENTED();
}

void BrowserWindowGtk::ShowSearchEnginesDialog() {
  NOTIMPLEMENTED();
}

void BrowserWindowGtk::ShowPasswordManager() {
  NOTIMPLEMENTED();
}

void BrowserWindowGtk::ShowSelectProfileDialog() {
  NOTIMPLEMENTED();
}

void BrowserWindowGtk::ShowNewProfileDialog() {
  NOTIMPLEMENTED();
}

void BrowserWindowGtk::ShowHTMLDialog(HtmlDialogUIDelegate* delegate,
                                      void* parent_window) {
  NOTIMPLEMENTED();
}

void BrowserWindowGtk::ConfirmBrowserCloseWithPendingDownloads() {
  NOTIMPLEMENTED();
  browser_->InProgressDownloadResponse(false);
}

void BrowserWindowGtk::Observe(NotificationType type,
                               const NotificationSource& source,
                               const NotificationDetails& details) {
  if (type == NotificationType::BOOKMARK_BAR_VISIBILITY_PREF_CHANGED) {
    MaybeShowBookmarkBar(browser_->GetSelectedTabContents());
  } else {
    NOTREACHED() << "Got a notification we didn't register for!";
  }
}

void BrowserWindowGtk::TabDetachedAt(TabContents* contents, int index) {
  // We use index here rather than comparing |contents| because by this time
  // the model has already removed |contents| from its list, so
  // browser_->GetSelectedTabContents() will return NULL or something else.
  if (index == browser_->tabstrip_model()->selected_index()) {
    infobar_container_->ChangeTabContents(NULL);
    contents_container_->SetTabContents(NULL);
  }
}

// TODO(estade): this function should probably be unforked from the BrowserView
// function of the same name by having a shared partial BrowserWindow
// implementation.
void BrowserWindowGtk::TabSelectedAt(TabContents* old_contents,
                                     TabContents* new_contents,
                                     int index,
                                     bool user_gesture) {
  DCHECK(old_contents != new_contents);

  // Update various elements that are interested in knowing the current
  // TabContents.
  infobar_container_->ChangeTabContents(new_contents);
  contents_container_->SetTabContents(new_contents);

  new_contents->DidBecomeSelected();
  // TODO(estade): after we manage browser activation, add a check to make sure
  // we are the active browser before calling RestoreFocus().
  if (!browser_->tabstrip_model()->closing_all())
    new_contents->view()->RestoreFocus();

  // Update all the UI bits.
  UpdateTitleBar();
  toolbar_->SetProfile(new_contents->profile());
  UpdateToolbar(new_contents, true);
  UpdateUIForContents(new_contents);
}

void BrowserWindowGtk::TabStripEmpty() {
  UpdateUIForContents(NULL);
}

void BrowserWindowGtk::MaybeShowBookmarkBar(TabContents* contents) {
  bool show_bar = false;

  if (browser_->SupportsWindowFeature(Browser::FEATURE_BOOKMARKBAR)
      && contents) {
    bookmark_bar_->SetProfile(contents->profile());
    bookmark_bar_->SetPageNavigator(contents);
    show_bar = true;
  }

  if (show_bar) {
    PrefService* prefs = contents->profile()->GetPrefs();
    show_bar = prefs->GetBoolean(prefs::kShowBookmarkBar);
  }

  if (show_bar) {
    bookmark_bar_->Show();
  } else {
    bookmark_bar_->Hide();
  }
}

void BrowserWindowGtk::UpdateUIForContents(TabContents* contents) {
  MaybeShowBookmarkBar(contents);
}

void BrowserWindowGtk::DestroyBrowser() {
  browser_.reset();
}

void BrowserWindowGtk::OnBoundsChanged(const gfx::Rect& bounds) {
  bounds_ = bounds;
}

void BrowserWindowGtk::OnStateChanged(GdkWindowState state) {
  state_ = state;
}

bool BrowserWindowGtk::CanClose() const {
  // TODO(tc): We don't have tab dragging yet.
  // You cannot close a frame for which there is an active originating drag
  // session.
  // if (tabstrip_->IsDragSessionActive())
  //   return false;

  // Give beforeunload handlers the chance to cancel the close before we hide
  // the window below.
  if (!browser_->ShouldCloseWindow())
    return false;

  if (!browser_->tabstrip_model()->empty()) {
    // Tab strip isn't empty.  Hide the window (so it appears to have closed
    // immediately) and close all the tabs, allowing the renderers to shut
    // down. When the tab strip is empty we'll be called back again.
    gtk_widget_hide(GTK_WIDGET(window_));
    browser_->OnWindowClosing();
    return false;
  }

  // Empty TabStripModel, it's now safe to allow the Window to be closed.
  NotificationService::current()->Notify(
      NotificationType::WINDOW_CLOSED,
      Source<GtkWindow>(window_),
      NotificationService::NoDetails());
  return true;
}

bool BrowserWindowGtk::ShouldShowWindowIcon() const {
  return browser_->SupportsWindowFeature(Browser::FEATURE_TITLEBAR);
}

void BrowserWindowGtk::AddFindBar(FindBarGtk* findbar) {
  gtk_box_pack_start(GTK_BOX(render_area_vbox_), findbar->widget(),
                     FALSE, FALSE, 0);
  gtk_box_reorder_child(GTK_BOX(render_area_vbox_), findbar->widget(), 0);
}

void BrowserWindowGtk::SetGeometryHints() {
  gtk_window_set_default_size(window_, 640, 480);

  // Allow the user to resize us arbitrarily small.
  GdkGeometry geometry;
  geometry.min_width = 1;
  geometry.min_height = 1;
  gtk_window_set_geometry_hints(window_, NULL, &geometry, GDK_HINT_MIN_SIZE);
}

void BrowserWindowGtk::SetWindowIcon() {
  ResourceBundle& rb = ResourceBundle::GetSharedInstance();
  GList* icon_list = NULL;
  icon_list = g_list_append(icon_list, rb.GetPixbufNamed(IDR_PRODUCT_ICON_32));
  icon_list = g_list_append(icon_list, rb.GetPixbufNamed(IDR_PRODUCT_LOGO_16));
  gtk_window_set_icon_list(window_, icon_list);
  g_list_free(icon_list);
}

void BrowserWindowGtk::ConnectAccelerators() {
  GtkAccelGroup* accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(window_, accel_group);
  // Drop the initial ref on |accel_group| so |window_| will own it.
  g_object_unref(accel_group);

  for (size_t i = 0; i < arraysize(kAcceleratorMap); ++i) {
    gtk_accel_group_connect(
        accel_group,
        kAcceleratorMap[i].keyval,
        kAcceleratorMap[i].modifier_type,
        GtkAccelFlags(0),
        g_cclosure_new(G_CALLBACK(OnGtkAccelerator), this, NULL));
  }
}

void BrowserWindowGtk::SetCustomFrame(bool custom_frame) {
  custom_frame_ = custom_frame;
  if (custom_frame_) {
    gtk_container_set_border_width(GTK_CONTAINER(window_vbox_), 2);
    // TODO(port): all the crazy blue title bar, etc.
    NOTIMPLEMENTED();
  } else {
    gtk_container_set_border_width(GTK_CONTAINER(window_vbox_), 0);
  }
}

// static
gboolean BrowserWindowGtk::OnGtkAccelerator(GtkAccelGroup* accel_group,
                                            GObject* acceleratable,
                                            guint keyval,
                                            GdkModifierType modifier,
                                            BrowserWindowGtk* browser_window) {
  int command_id = GetCommandId(keyval, modifier);
  // We have to delay certain commands that may try to destroy widgets to which
  // GTK is currently holding a reference. (For now the only such command is
  // tab closing.) GTK will hold a reference on the RWHV widget when the
  // event came through on that widget but GTK focus was elsewhere.
  if (IDC_CLOSE_TAB == command_id) {
    MessageLoop::current()->PostTask(FROM_HERE,
        browser_window->method_factory_.NewRunnableMethod(
            &BrowserWindowGtk::ExecuteBrowserCommand,
            command_id));
  } else {
    browser_window->ExecuteBrowserCommand(command_id);
  }

  return TRUE;
}

void BrowserWindowGtk::ExecuteBrowserCommand(int id) {
  browser_->ExecuteCommand(id);
}
