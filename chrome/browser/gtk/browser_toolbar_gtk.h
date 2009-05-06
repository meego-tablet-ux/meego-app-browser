// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GTK_BROWSER_TOOLBAR_GTK_H_
#define CHROME_BROWSER_GTK_BROWSER_TOOLBAR_GTK_H_

#include <gtk/gtk.h>
#include <string>

#include "base/scoped_ptr.h"
#include "base/task.h"
#include "chrome/browser/autocomplete/autocomplete_popup_view.h"
#include "chrome/browser/command_updater.h"
#include "chrome/browser/gtk/menu_gtk.h"
#include "chrome/common/pref_member.h"

class BackForwardMenuModelGtk;
class Browser;
class CustomDrawButton;
class GoButtonGtk;
class LocationBar;
class LocationBarViewGtk;
class NineBox;
class Profile;
class TabContents;
class ToolbarModel;
class ToolbarStarToggleGtk;

// View class that displays the GTK version of the toolbar and routes gtk
// events back to the Browser.
class BrowserToolbarGtk : public CommandUpdater::CommandObserver,
                          public MenuGtk::Delegate,
                          public NotificationObserver,
                          public AutocompletePopupPositioner {
 public:
  explicit BrowserToolbarGtk(Browser* browser);
  virtual ~BrowserToolbarGtk();

  // Create the contents of the toolbar. |top_level_window| is the GtkWindow
  // to which we attach our accelerators.
  void Init(Profile* profile, GtkWindow* top_level_window);

  // Adds this GTK toolbar into a sizing box.
  void AddToolbarToBox(GtkWidget* box);

  void Show();
  void Hide();

  virtual LocationBar* GetLocationBar() const;

  GoButtonGtk* GetGoButton() { return go_.get(); }

  // Overridden from CommandUpdater::CommandObserver:
  virtual void EnabledStateChangedForCommand(int id, bool enabled);

  // Overridden from MenuGtk::Delegate:
  virtual bool IsCommandEnabled(int command_id) const;
  virtual bool IsItemChecked(int id) const;
  virtual void ExecuteCommand(int command_id);

  // NotificationObserver implementation.
  void Observe(NotificationType type,
               const NotificationSource& source,
               const NotificationDetails& details);

  void SetProfile(Profile* profile);

  // Message that we should react to a state change.
  void UpdateTabContents(TabContents* contents, bool should_restore_state);

  ToolbarStarToggleGtk* star() { return star_.get(); }

  // Implement AutocompletePopupPositioner, return the position of where the
  // Omnibox results popup should go (from the star to the go buttons).
  virtual gfx::Rect GetPopupBounds() const;

 private:
  // Builds a toolbar button with all the properties set.
  CustomDrawButton* BuildToolbarButton(int normal_id,
                                       int active_id,
                                       int highlight_id,
                                       int depressed_id,
                                       const std::string& localized_tooltip);

  ToolbarStarToggleGtk* BuildStarButton(const std::string& localized_tooltip);

  void BuildToolbarMenuButton(
      int icon_id,
      const std::string& localized_tooltip,
      OwnedWidgetGtk* owner);

  // Adds a keyboard accelerator which triggers a button (e.g., ctrl+r is now
  // equivalent to a reload click).
  void AddAcceleratorToButton(
      const scoped_ptr<CustomDrawButton>& button,
      unsigned int accelerator,
      unsigned int accelerator_mod);

  // Gtk callback for the "expose-event" signal.
  static gboolean OnContentAreaExpose(GtkWidget* widget, GdkEventExpose* e,
                                      BrowserToolbarGtk* toolbar);

  // Gtk callback for the "clicked" signal.
  static void OnButtonClick(GtkWidget* button, BrowserToolbarGtk* toolbar);

  // Gtk callback to intercept mouse clicks to the menu buttons.
  static gboolean OnMenuButtonPressEvent(GtkWidget* button,
                                         GdkEvent *event,
                                         BrowserToolbarGtk* toolbar);

  // Displays the page menu.
  void RunPageMenu(GdkEvent* button_press_event);

  // Displays the app menu.
  void RunAppMenu(GdkEvent* button_press_event);

  // Construct the Home button.
  CustomDrawButton* MakeHomeButton();

  // Initialize the background NineBox.
  void InitNineBox();

  // Ninebox for the toolbar background
  scoped_ptr<NineBox> background_ninebox_;

  // Gtk widgets. The toolbar is an hbox with each of the other pieces of the
  // toolbar placed side by side.
  GtkWidget* toolbar_;

  // The location bar view.
  scoped_ptr<LocationBarViewGtk> location_bar_;

  // A pointer to our window's accelerator list.
  GtkAccelGroup* accel_group_;

  // All the buttons in the toolbar.
  scoped_ptr<CustomDrawButton> back_, forward_;
  scoped_ptr<CustomDrawButton> reload_;
  scoped_ptr<CustomDrawButton> home_;  // May be NULL.
  scoped_ptr<ToolbarStarToggleGtk> star_;
  scoped_ptr<GoButtonGtk> go_;
  OwnedWidgetGtk page_menu_button_, app_menu_button_;

  // The model that contains the security level, text, icon to display...
  ToolbarModel* model_;

  scoped_ptr<MenuGtk> page_menu_;
  scoped_ptr<MenuGtk> app_menu_;

  Browser* browser_;
  Profile* profile_;

  // Controls whether or not a home button should be shown on the toolbar.
  BooleanPrefMember show_home_button_;

  // Back/Forward menus ------------------------------------------------------
  // When clicked, these buttons will navigate forward or backward. When
  // pressed and held, they show a dropdown menu of recent web sites.
  // TODO(port): to match windows, we need to immediately show the back/forward
  // menu when the user starts dragging the mouse.

  // Builds a toolbar button for the back or forward dropdown menus.
  CustomDrawButton* BuildBackForwardButton(
    int normal_id,
    int active_id,
    int highlight_id,
    int depressed_id,
    const std::string& localized_tooltip);

  // Starts a timer to show the dropdown menu.
  static gboolean OnBackForwardPressEvent(GtkWidget* button,
                                          GdkEventButton* event,
                                          BrowserToolbarGtk* toolbar);

  // Shows the dropdown menu when the timer fires. |button_type| refers to the
  // click that originated the button press event.
  void ShowBackForwardMenu(GtkWidget* button, gint button_type);

  // The back/forward menu gets reset every time it is shown.
  scoped_ptr<MenuGtk> back_forward_menu_;

  scoped_ptr<BackForwardMenuModelGtk> back_menu_model_;
  scoped_ptr<BackForwardMenuModelGtk> forward_menu_model_;

  ScopedRunnableMethodFactory<BrowserToolbarGtk> show_menu_factory_;
};

#endif  // CHROME_BROWSER_GTK_BROWSER_TOOLBAR_GTK_H_
