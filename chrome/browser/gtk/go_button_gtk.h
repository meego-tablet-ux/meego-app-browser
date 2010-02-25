// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GTK_GO_BUTTON_GTK_H_
#define CHROME_BROWSER_GTK_GO_BUTTON_GTK_H_

#include <gtk/gtk.h>

#include "base/basictypes.h"
#include "base/task.h"
#include "chrome/browser/gtk/custom_button.h"
#include "chrome/common/notification_observer.h"
#include "chrome/common/notification_registrar.h"
#include "chrome/common/owned_widget_gtk.h"

class Browser;
class GtkThemeProvider;
class LocationBarViewGtk;
class Task;

class GoButtonGtk : public NotificationObserver {
 public:
  enum Mode { MODE_GO = 0, MODE_STOP };

  GoButtonGtk(LocationBarViewGtk* location_bar, Browser* browser);
  ~GoButtonGtk();

  GtkWidget* widget() const { return widget_.get(); }

  // Ask for a specified button state.  If |force| is true this will be applied
  // immediately.
  void ChangeMode(Mode mode, bool force);

  // Provide NotificationObserver implementation.
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

 private:
  friend class GoButtonGtkPeer;

  // gtk signals
  static gboolean OnExpose(GtkWidget* widget,
                           GdkEventExpose* e,
                           GoButtonGtk* button);
  static gboolean OnLeave(GtkWidget* widget, GdkEventCrossing* event,
                          GoButtonGtk* button);
  static gboolean OnClicked(GtkButton* widget, GoButtonGtk* button);
  static gboolean OnQueryTooltipThunk(GtkWidget* widget,
                                      gint x, gint y, gboolean keyboard_mode,
                                      GtkTooltip* tooltip,
                                      GoButtonGtk* button) {
    return button->OnQueryTooltip(tooltip);
  }
  gboolean OnQueryTooltip(GtkTooltip* tooltip);

  void SetToggled();

  Task* CreateButtonTimerTask();
  void OnButtonTimer();
  void UpdateThemeButtons();

  // Used to listen for theme change notifications.
  NotificationRegistrar registrar_;

  LocationBarViewGtk* const location_bar_;

  // Keep a pointer to the Browser object to execute commands on it.
  Browser* const browser_;

  // Delay time to wait before allowing a mode change.  This is to prevent a
  // mode switch while the user is double clicking.
  int button_delay_;
  ScopedRunnableMethodFactory<GoButtonGtk> stop_timer_;

  // The mode we should be in.
  Mode intended_mode_;

  // The currently-visible mode - this may different from the intended mode.
  Mode visible_mode_;

  GtkThemeProvider* theme_provider_;

  CustomDrawButtonBase go_;
  CustomDrawButtonBase stop_;
  CustomDrawHoverController hover_controller_;

  OwnedWidgetGtk widget_;

  DISALLOW_COPY_AND_ASSIGN(GoButtonGtk);
};

#endif  // CHROME_BROWSER_GTK_GO_BUTTON_GTK_H_
