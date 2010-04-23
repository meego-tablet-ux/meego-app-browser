// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/gtk/browser_toolbar_gtk.h"

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <X11/XF86keysym.h>

#include "app/gtk_dnd_util.h"
#include "app/l10n_util.h"
#include "app/menus/accelerator_gtk.h"
#include "app/resource_bundle.h"
#include "base/base_paths.h"
#include "base/i18n/rtl.h"
#include "base/keyboard_codes_posix.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/singleton.h"
#include "chrome/app/chrome_dll_resource.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/browser_theme_provider.h"
#include "chrome/browser/encoding_menu_controller.h"
#include "chrome/browser/gtk/accelerators_gtk.h"
#include "chrome/browser/gtk/back_forward_button_gtk.h"
#include "chrome/browser/gtk/browser_actions_toolbar_gtk.h"
#include "chrome/browser/gtk/browser_window_gtk.h"
#include "chrome/browser/gtk/cairo_cached_surface.h"
#include "chrome/browser/gtk/custom_button.h"
#include "chrome/browser/gtk/go_button_gtk.h"
#include "chrome/browser/gtk/gtk_chrome_button.h"
#include "chrome/browser/gtk/gtk_theme_provider.h"
#include "chrome/browser/gtk/gtk_util.h"
#include "chrome/browser/gtk/location_bar_view_gtk.h"
#include "chrome/browser/gtk/standard_menus.h"
#include "chrome/browser/gtk/tabs/tab_strip_gtk.h"
#include "chrome/browser/gtk/view_id_util.h"
#include "chrome/browser/net/url_fixer_upper.h"
#include "chrome/browser/pref_service.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/sync/sync_ui_util.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/common/notification_details.h"
#include "chrome/common/notification_service.h"
#include "chrome/common/notification_type.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "gfx/gtk_util.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"

namespace {

// Height of the toolbar in pixels (not counting padding).
const int kToolbarHeight = 29;

// Padding within the toolbar above the buttons and location bar.
const int kTopPadding = 4;

// Height of the toolbar in pixels when we only show the location bar.
const int kToolbarHeightLocationBarOnly = kToolbarHeight - 2;

// Interior spacing between toolbar widgets.
const int kToolbarWidgetSpacing = 2;

}  // namespace

// BrowserToolbarGtk, public ---------------------------------------------------

BrowserToolbarGtk::BrowserToolbarGtk(Browser* browser, BrowserWindowGtk* window)
    : toolbar_(NULL),
      location_bar_(new LocationBarViewGtk(browser)),
      model_(browser->toolbar_model()),
      page_menu_model_(this, browser),
      app_menu_model_(this, browser),
      browser_(browser),
      window_(window),
      profile_(NULL),
      sync_service_(NULL),
      menu_bar_helper_(this) {
  browser_->command_updater()->AddCommandObserver(IDC_BACK, this);
  browser_->command_updater()->AddCommandObserver(IDC_FORWARD, this);
  browser_->command_updater()->AddCommandObserver(IDC_RELOAD, this);
  browser_->command_updater()->AddCommandObserver(IDC_HOME, this);
  browser_->command_updater()->AddCommandObserver(IDC_BOOKMARK_PAGE, this);

  registrar_.Add(this,
                 NotificationType::BROWSER_THEME_CHANGED,
                 NotificationService::AllSources());
}

BrowserToolbarGtk::~BrowserToolbarGtk() {
  if (sync_service_)
    sync_service_->RemoveObserver(this);

  browser_->command_updater()->RemoveCommandObserver(IDC_BACK, this);
  browser_->command_updater()->RemoveCommandObserver(IDC_FORWARD, this);
  browser_->command_updater()->RemoveCommandObserver(IDC_RELOAD, this);
  browser_->command_updater()->RemoveCommandObserver(IDC_HOME, this);
  browser_->command_updater()->RemoveCommandObserver(IDC_BOOKMARK_PAGE, this);

  offscreen_entry_.Destroy();

  page_menu_.reset();
  app_menu_.reset();
  page_menu_button_.Destroy();
  app_menu_button_.Destroy();
}

void BrowserToolbarGtk::Init(Profile* profile,
                             GtkWindow* top_level_window) {
  // Make sure to tell the location bar the profile before calling its Init.
  SetProfile(profile);

  theme_provider_ = GtkThemeProvider::GetFrom(profile);
  offscreen_entry_.Own(gtk_entry_new());

  show_home_button_.Init(prefs::kShowHomeButton, profile->GetPrefs(), this);

  event_box_ = gtk_event_box_new();
  // Make the event box transparent so themes can use transparent toolbar
  // backgrounds.
  if (!theme_provider_->UseGtkTheme())
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(event_box_), FALSE);

  toolbar_ = gtk_hbox_new(FALSE, 0);
  alignment_ = gtk_alignment_new(0.0, 0.0, 1.0, 1.0);
  UpdateForBookmarkBarVisibility(false);
  g_signal_connect(alignment_, "expose-event",
                   G_CALLBACK(&OnAlignmentExposeThunk), this);
  gtk_container_add(GTK_CONTAINER(event_box_), alignment_);
  gtk_container_add(GTK_CONTAINER(alignment_), toolbar_);
  // Force the height of the toolbar so we get the right amount of padding
  // above and below the location bar. -1 for width means "let GTK do its
  // normal sizing".
  gtk_widget_set_size_request(toolbar_, -1, ShouldOnlyShowLocation() ?
      kToolbarHeightLocationBarOnly : kToolbarHeight);

  // Group back and forward into an hbox so there's no spacing between them.
  GtkWidget* back_forward_hbox_ = gtk_hbox_new(FALSE, 0);

  back_.reset(new BackForwardButtonGtk(browser_, false));
  gtk_box_pack_start(GTK_BOX(back_forward_hbox_), back_->widget(), FALSE,
                     FALSE, 0);
  g_signal_connect(back_->widget(), "clicked",
                   G_CALLBACK(OnButtonClickThunk), this);

  forward_.reset(new BackForwardButtonGtk(browser_, true));
  gtk_box_pack_start(GTK_BOX(back_forward_hbox_), forward_->widget(), FALSE,
                     FALSE, 0);
  g_signal_connect(forward_->widget(), "clicked",
                   G_CALLBACK(OnButtonClickThunk), this);
  gtk_box_pack_start(GTK_BOX(toolbar_), back_forward_hbox_, FALSE, FALSE,
                     kToolbarWidgetSpacing);

  home_.reset(BuildToolbarButton(IDR_HOME, IDR_HOME_P, IDR_HOME_H, 0,
                                 IDR_BUTTON_MASK,
                                 l10n_util::GetStringUTF8(IDS_TOOLTIP_HOME),
                                 GTK_STOCK_HOME));
  gtk_util::SetButtonTriggersNavigation(home_->widget());
  SetUpDragForHomeButton();


  reload_.reset(BuildToolbarButton(IDR_RELOAD, IDR_RELOAD_P, IDR_RELOAD_H, 0,
                                   IDR_RELOAD_MASK,
                                   l10n_util::GetStringUTF8(IDS_TOOLTIP_RELOAD),
                                   GTK_STOCK_REFRESH));

  location_hbox_ = gtk_hbox_new(FALSE, 0);
  location_bar_->Init(ShouldOnlyShowLocation());
  gtk_box_pack_start(GTK_BOX(location_hbox_), location_bar_->widget(), TRUE,
                     TRUE, 0);

  g_signal_connect(location_hbox_, "expose-event",
                   G_CALLBACK(OnLocationHboxExposeThunk), this);
  gtk_box_pack_start(GTK_BOX(toolbar_), location_hbox_, TRUE, TRUE,
      kToolbarWidgetSpacing + (ShouldOnlyShowLocation() ? 1 : 0));

  go_.reset(new GoButtonGtk(location_bar_.get(), browser_));
  gtk_box_pack_start(GTK_BOX(toolbar_), go_->widget(), FALSE, FALSE, 0);

  if (!ShouldOnlyShowLocation()) {
    actions_toolbar_.reset(new BrowserActionsToolbarGtk(browser_));
    gtk_box_pack_start(GTK_BOX(toolbar_), actions_toolbar_->widget(),
                       FALSE, FALSE, 0);
  }

  // Group the menu buttons together in an hbox.
  GtkWidget* menus_hbox_ = gtk_hbox_new(FALSE, 0);
  GtkWidget* page_menu = BuildToolbarMenuButton(
      l10n_util::GetStringUTF8(IDS_PAGEMENU_TOOLTIP),
      &page_menu_button_);
  menu_bar_helper_.Add(page_menu_button_.get());
  page_menu_image_ = gtk_image_new_from_pixbuf(
      theme_provider_->GetRTLEnabledPixbufNamed(IDR_MENU_PAGE));
  gtk_container_add(GTK_CONTAINER(page_menu), page_menu_image_);

  page_menu_.reset(new MenuGtk(this, &page_menu_model_));
  gtk_box_pack_start(GTK_BOX(menus_hbox_), page_menu, FALSE, FALSE, 0);

  GtkWidget* chrome_menu = BuildToolbarMenuButton(
      l10n_util::GetStringFUTF8(IDS_APPMENU_TOOLTIP,
          WideToUTF16(l10n_util::GetString(IDS_PRODUCT_NAME))),
      &app_menu_button_);
  menu_bar_helper_.Add(app_menu_button_.get());
  app_menu_image_ = gtk_image_new_from_pixbuf(
      theme_provider_->GetRTLEnabledPixbufNamed(IDR_MENU_CHROME));
  gtk_container_add(GTK_CONTAINER(chrome_menu), app_menu_image_);

  app_menu_.reset(new MenuGtk(this, &app_menu_model_));
  gtk_box_pack_start(GTK_BOX(menus_hbox_), chrome_menu, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(toolbar_), menus_hbox_, FALSE, FALSE,
                     kToolbarWidgetSpacing);

  if (ShouldOnlyShowLocation()) {
    gtk_widget_show(event_box_);
    gtk_widget_show(alignment_);
    gtk_widget_show(toolbar_);
    gtk_widget_show_all(location_hbox_);
    gtk_widget_hide(reload_->widget());
    gtk_widget_hide(go_->widget());
  } else {
    gtk_widget_show_all(event_box_);

    if (show_home_button_.GetValue())
      gtk_widget_show(home_->widget());
    else
      gtk_widget_hide(home_->widget());

    if (actions_toolbar_->button_count() == 0)
      gtk_widget_hide(actions_toolbar_->widget());
  }

  // Because the above does a recursive show all on all widgets we need to
  // update the icon visibility to hide them.
  location_bar_->UpdateContentSettingsIcons();

  SetViewIDs();
  theme_provider_->InitThemesFor(this);
}

void BrowserToolbarGtk::SetViewIDs() {
  ViewIDUtil::SetID(widget(), VIEW_ID_TOOLBAR);
  ViewIDUtil::SetID(back_->widget(), VIEW_ID_BACK_BUTTON);
  ViewIDUtil::SetID(forward_->widget(), VIEW_ID_FORWARD_BUTTON);
  ViewIDUtil::SetID(reload_->widget(), VIEW_ID_RELOAD_BUTTON);
  ViewIDUtil::SetID(home_->widget(), VIEW_ID_HOME_BUTTON);
  ViewIDUtil::SetID(location_bar_->widget(), VIEW_ID_LOCATION_BAR);
  ViewIDUtil::SetID(go_->widget(), VIEW_ID_GO_BUTTON);
  ViewIDUtil::SetID(page_menu_button_.get(), VIEW_ID_PAGE_MENU);
  ViewIDUtil::SetID(app_menu_button_.get(), VIEW_ID_APP_MENU);
}

void BrowserToolbarGtk::Show() {
  gtk_widget_show(toolbar_);
}

void BrowserToolbarGtk::Hide() {
  gtk_widget_hide(toolbar_);
}

LocationBar* BrowserToolbarGtk::GetLocationBar() const {
  return location_bar_.get();
}

void BrowserToolbarGtk::UpdateForBookmarkBarVisibility(
    bool show_bottom_padding) {
  gtk_alignment_set_padding(GTK_ALIGNMENT(alignment_),
      ShouldOnlyShowLocation() ? 0 : kTopPadding,
      !show_bottom_padding || ShouldOnlyShowLocation() ? 0 : kTopPadding,
      0, 0);
}

void BrowserToolbarGtk::ShowPageMenu() {
  PopupForButton(page_menu_button_.get());
}

void BrowserToolbarGtk::ShowAppMenu() {
  PopupForButton(app_menu_button_.get());
}

// CommandUpdater::CommandObserver ---------------------------------------------

void BrowserToolbarGtk::EnabledStateChangedForCommand(int id, bool enabled) {
  GtkWidget* widget = NULL;
  switch (id) {
    case IDC_BACK:
      widget = back_->widget();
      break;
    case IDC_FORWARD:
      widget = forward_->widget();
      break;
    case IDC_RELOAD:
      widget = reload_->widget();
      break;
    case IDC_GO:
      widget = go_->widget();
      break;
    case IDC_HOME:
      if (home_.get())
        widget = home_->widget();
      break;
  }
  if (widget) {
    if (!enabled && GTK_WIDGET_STATE(widget) == GTK_STATE_PRELIGHT) {
      // If we're disabling a widget, GTK will helpfully restore it to its
      // previous state when we re-enable it, even if that previous state
      // is the prelight.  This looks bad.  See the bug for a simple repro.
      // http://code.google.com/p/chromium/issues/detail?id=13729
      gtk_widget_set_state(widget, GTK_STATE_NORMAL);
    }
    gtk_widget_set_sensitive(widget, enabled);
  }
}

// MenuGtk::Delegate -----------------------------------------------------------

void BrowserToolbarGtk::StoppedShowing() {
  // Without these calls, the hover state can get stuck since the leave-notify
  // event is not sent when clicking a button brings up the menu.
  gtk_chrome_button_set_hover_state(
      GTK_CHROME_BUTTON(page_menu_button_.get()), 0.0);
  gtk_chrome_button_set_hover_state(
      GTK_CHROME_BUTTON(app_menu_button_.get()), 0.0);

  gtk_chrome_button_unset_paint_state(
      GTK_CHROME_BUTTON(page_menu_button_.get()));
  gtk_chrome_button_unset_paint_state(
      GTK_CHROME_BUTTON(app_menu_button_.get()));
}

// menus::SimpleMenuModel::Delegate

bool BrowserToolbarGtk::IsCommandIdEnabled(int id) const {
  return browser_->command_updater()->IsCommandEnabled(id);
}

bool BrowserToolbarGtk::IsCommandIdChecked(int id) const {
  if (!profile_)
    return false;

  EncodingMenuController controller;
  if (id == IDC_SHOW_BOOKMARK_BAR) {
    return profile_->GetPrefs()->GetBoolean(prefs::kShowBookmarkBar);
  } else if (controller.DoesCommandBelongToEncodingMenu(id)) {
    TabContents* tab_contents = browser_->GetSelectedTabContents();
    if (tab_contents) {
      return controller.IsItemChecked(profile_, tab_contents->encoding(),
                                      id);
    }
  }

  return false;
}

void BrowserToolbarGtk::ExecuteCommand(int id) {
  browser_->ExecuteCommand(id);
}

bool BrowserToolbarGtk::GetAcceleratorForCommandId(
    int id,
    menus::Accelerator* accelerator) {
  const menus::AcceleratorGtk* accelerator_gtk =
      Singleton<AcceleratorsGtk>()->GetPrimaryAcceleratorForCommand(id);
  if (accelerator_gtk)
    *accelerator = *accelerator_gtk;
  return !!accelerator_gtk;
}

// NotificationObserver --------------------------------------------------------

void BrowserToolbarGtk::Observe(NotificationType type,
                                const NotificationSource& source,
                                const NotificationDetails& details) {
  if (type == NotificationType::PREF_CHANGED) {
    std::wstring* pref_name = Details<std::wstring>(details).ptr();
    if (*pref_name == prefs::kShowHomeButton) {
      if (show_home_button_.GetValue() && !ShouldOnlyShowLocation()) {
        gtk_widget_show(home_->widget());
      } else {
        gtk_widget_hide(home_->widget());
      }
    }
  } else if (type == NotificationType::BROWSER_THEME_CHANGED) {
    // Update the spacing around the menu buttons
    int border = theme_provider_->UseGtkTheme() ? 0 : 2;
    gtk_container_set_border_width(
        GTK_CONTAINER(page_menu_button_.get()), border);
    gtk_container_set_border_width(
        GTK_CONTAINER(app_menu_button_.get()), border);

    // Update the menu button images.
    gtk_image_set_from_pixbuf(GTK_IMAGE(page_menu_image_),
        theme_provider_->GetRTLEnabledPixbufNamed(IDR_MENU_PAGE));
    gtk_image_set_from_pixbuf(GTK_IMAGE(app_menu_image_),
        theme_provider_->GetRTLEnabledPixbufNamed(IDR_MENU_CHROME));

    // Update the spacing between the reload button and the location bar.
    gtk_box_set_child_packing(
        GTK_BOX(toolbar_), reload_->widget(),
        FALSE, FALSE,
        theme_provider_->UseGtkTheme() ? kToolbarWidgetSpacing : 0,
        GTK_PACK_START);
    gtk_box_set_child_packing(
        GTK_BOX(toolbar_), location_hbox_,
        TRUE, TRUE,
        (theme_provider_->UseGtkTheme() ? kToolbarWidgetSpacing : 0) +
        (ShouldOnlyShowLocation() ? 1 : 0),
        GTK_PACK_START);

    // When using the GTK+ theme, we need to have the event box be visible so
    // buttons don't get a halo color from the background.  When using Chromium
    // themes, we want to let the background show through the toolbar.
    gtk_event_box_set_visible_window(GTK_EVENT_BOX(event_box_),
                                     theme_provider_->UseGtkTheme());
  } else {
    NOTREACHED();
  }
}

// BrowserToolbarGtk, public ---------------------------------------------------

void BrowserToolbarGtk::SetProfile(Profile* profile) {
  if (profile == profile_)
    return;

  profile_ = profile;
  location_bar_->SetProfile(profile);

  if (profile_->GetProfileSyncService()) {
    // Obtain a pointer to the profile sync service and add our instance as an
    // observer.
    sync_service_ = profile_->GetProfileSyncService();
    sync_service_->AddObserver(this);
  }
}

void BrowserToolbarGtk::UpdateTabContents(TabContents* contents,
                                          bool should_restore_state) {
  location_bar_->Update(should_restore_state ? contents : NULL);

  if (actions_toolbar_.get())
    actions_toolbar_->Update();
}

// BrowserToolbarGtk, private --------------------------------------------------

CustomDrawButton* BrowserToolbarGtk::BuildToolbarButton(
    int normal_id, int active_id, int highlight_id, int depressed_id,
    int background_id, const std::string& localized_tooltip,
    const char* stock_id) {
  CustomDrawButton* button = new CustomDrawButton(
      GtkThemeProvider::GetFrom(profile_),
      normal_id, active_id, highlight_id, depressed_id, background_id, stock_id,
      GTK_ICON_SIZE_SMALL_TOOLBAR);

  gtk_widget_set_tooltip_text(button->widget(),
                              localized_tooltip.c_str());
  g_signal_connect(button->widget(), "clicked",
                   G_CALLBACK(OnButtonClickThunk), this);

  gtk_box_pack_start(GTK_BOX(toolbar_), button->widget(), FALSE, FALSE,
                     kToolbarWidgetSpacing);
  return button;
}

GtkWidget* BrowserToolbarGtk::BuildToolbarMenuButton(
    const std::string& localized_tooltip,
    OwnedWidgetGtk* owner) {
  GtkWidget* button = theme_provider_->BuildChromeButton();
  owner->Own(button);

  gtk_widget_set_tooltip_text(button, localized_tooltip.c_str());
  g_signal_connect(button, "button-press-event",
                   G_CALLBACK(OnMenuButtonPressEventThunk), this);
  GTK_WIDGET_UNSET_FLAGS(button, GTK_CAN_FOCUS);

  return button;
}

void BrowserToolbarGtk::SetUpDragForHomeButton() {
  gtk_drag_dest_set(home_->widget(), GTK_DEST_DEFAULT_ALL,
                    NULL, 0, GDK_ACTION_COPY);
  static const int targets[] = { gtk_dnd_util::TEXT_PLAIN,
                                 gtk_dnd_util::TEXT_URI_LIST, -1 };
  gtk_dnd_util::SetDestTargetList(home_->widget(), targets);

  g_signal_connect(home_->widget(), "drag-data-received",
                   G_CALLBACK(OnDragDataReceivedThunk), this);
}

void BrowserToolbarGtk::ChangeActiveMenu(GtkWidget* active_menu,
    guint timestamp) {
  MenuGtk* old_menu;
  MenuGtk* new_menu;
  GtkWidget* relevant_button;
  if (active_menu == app_menu_->widget()) {
    old_menu = app_menu_.get();
    new_menu = page_menu_.get();
    relevant_button = page_menu_button_.get();
  } else {
    old_menu = page_menu_.get();
    new_menu = app_menu_.get();
    relevant_button = app_menu_button_.get();
  }

  old_menu->Cancel();
  gtk_chrome_button_set_paint_state(GTK_CHROME_BUTTON(relevant_button),
                                    GTK_STATE_ACTIVE);
  new_menu->Popup(relevant_button, 0, timestamp);
}

gboolean BrowserToolbarGtk::OnAlignmentExpose(GtkWidget* widget,
                                              GdkEventExpose* e) {
  // We don't need to render the toolbar image in GTK mode.
  if (theme_provider_->UseGtkTheme())
    return FALSE;

  cairo_t* cr = gdk_cairo_create(GDK_DRAWABLE(widget->window));
  gdk_cairo_rectangle(cr, &e->area);
  cairo_clip(cr);

  gfx::Point tabstrip_origin =
      window_->tabstrip()->GetTabStripOriginForWidget(widget);
  gtk_util::DrawThemedToolbarBackground(widget, cr, e, tabstrip_origin,
                                        theme_provider_);

  cairo_destroy(cr);

  return FALSE;  // Allow subwidgets to paint.
}

gboolean BrowserToolbarGtk::OnLocationHboxExpose(GtkWidget* location_hbox,
                                                 GdkEventExpose* e) {
  if (theme_provider_->UseGtkTheme()) {
    gtk_util::DrawTextEntryBackground(offscreen_entry_.get(),
                                      location_hbox, &e->area,
                                      &location_hbox->allocation);
  }

  return FALSE;
}

void BrowserToolbarGtk::OnButtonClick(GtkWidget* button) {
  if ((button == back_->widget()) ||
      (button == forward_->widget())) {
    location_bar_->Revert();
    return;
  }

  int tag = -1;
  if (button == reload_->widget()) {
    GdkModifierType modifier_state;
    if (gtk_get_current_event_state(&modifier_state) &&
        modifier_state & GDK_SHIFT_MASK) {
      tag = IDC_RELOAD_IGNORING_CACHE;
    } else {
      tag = IDC_RELOAD;
    }
    location_bar_->Revert();
  } else if (home_.get() && button == home_->widget()) {
    tag = IDC_HOME;
  }

  DCHECK_NE(tag, -1) << "Unexpected button click callback";
  browser_->ExecuteCommandWithDisposition(tag,
      gtk_util::DispositionForCurrentButtonPressEvent());
}

gboolean BrowserToolbarGtk::OnMenuButtonPressEvent(GtkWidget* button,
                                                   GdkEventButton* event) {
  if (event->button != 1)
    return FALSE;

  gtk_chrome_button_set_paint_state(GTK_CHROME_BUTTON(button),
                                    GTK_STATE_ACTIVE);
  MenuGtk* menu = button == page_menu_button_.get() ?
                  page_menu_.get() : app_menu_.get();
  menu->Popup(button, reinterpret_cast<GdkEvent*>(event));
  menu_bar_helper_.MenuStartedShowing(button, menu->widget());

  return TRUE;
}

void BrowserToolbarGtk::OnDragDataReceived(GtkWidget* widget,
    GdkDragContext* drag_context, gint x, gint y,
    GtkSelectionData* data, guint info, guint time) {
  if (info != gtk_dnd_util::TEXT_PLAIN) {
    NOTIMPLEMENTED() << "Only support plain text drops for now, sorry!";
    return;
  }

  GURL url(reinterpret_cast<char*>(data->data));
  if (!url.is_valid())
    return;

  bool url_is_newtab = url.spec() == chrome::kChromeUINewTabURL;
  profile_->GetPrefs()->SetBoolean(prefs::kHomePageIsNewTabPage,
                                   url_is_newtab);
  if (!url_is_newtab) {
    profile_->GetPrefs()->SetString(prefs::kHomePage,
                                    UTF8ToWide(url.spec()));
  }
}

void BrowserToolbarGtk::OnStateChanged() {
  DCHECK(sync_service_);

  std::string menu_label = UTF16ToUTF8(
      sync_ui_util::GetSyncMenuLabel(sync_service_));

  gtk_container_foreach(GTK_CONTAINER(app_menu_->widget()), &SetSyncMenuLabel,
                        &menu_label);
}

// static
void BrowserToolbarGtk::SetSyncMenuLabel(GtkWidget* widget, gpointer userdata) {
  const MenuCreateMaterial* data =
      reinterpret_cast<const MenuCreateMaterial*>(
          g_object_get_data(G_OBJECT(widget), "menu-data"));
  if (data) {
    if (data->id == IDC_SYNC_BOOKMARKS) {
      std::string label = gtk_util::ConvertAcceleratorsFromWindowsStyle(
          *reinterpret_cast<const std::string*>(userdata));
      GtkWidget *menu_label = gtk_bin_get_child(GTK_BIN(widget));
      gtk_label_set_label(GTK_LABEL(menu_label), label.c_str());
    }
  }
}

bool BrowserToolbarGtk::ShouldOnlyShowLocation() const {
  // If we're a popup window, only show the location bar (omnibox).
  return browser_->type() != Browser::TYPE_NORMAL;
}

void BrowserToolbarGtk::PopupForButton(GtkWidget* button) {
  page_menu_->Cancel();
  app_menu_->Cancel();

  gtk_chrome_button_set_paint_state(GTK_CHROME_BUTTON(button),
                                    GTK_STATE_ACTIVE);
  MenuGtk* menu = button == page_menu_button_.get() ?
                  page_menu_.get() : app_menu_.get();
  menu->PopupAsFromKeyEvent(button);
  menu_bar_helper_.MenuStartedShowing(button, menu->widget());
}

void BrowserToolbarGtk::PopupForButtonNextTo(GtkWidget* button,
                                             GtkMenuDirectionType dir) {
  GtkWidget* other_button = button == page_menu_button_.get() ?
      app_menu_button_.get() : page_menu_button_.get();
  PopupForButton(other_button);
}
