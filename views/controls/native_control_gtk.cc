// Copyright (c) 2009 The Chromium Authors. All rights reserved. Use of this
// source code is governed by a BSD-style license that can be found in the
// LICENSE file.

#include "views/controls/native_control_gtk.h"

#include <gtk/gtk.h>

#include "base/logging.h"
#include "views/focus/focus_manager.h"

namespace views {

NativeControlGtk::NativeControlGtk() {
}

NativeControlGtk::~NativeControlGtk() {
  if (native_view())
    gtk_widget_destroy(native_view());
}

////////////////////////////////////////////////////////////////////////////////
// NativeControlGtk, View overrides:

void NativeControlGtk::SetEnabled(bool enabled) {
  NOTIMPLEMENTED();
  if (IsEnabled() != enabled) {
    View::SetEnabled(enabled);
    if (native_view())
      gtk_widget_set_sensitive(native_view(), IsEnabled());
  }
}

void NativeControlGtk::ViewHierarchyChanged(bool is_add, View* parent,
                                            View* child) {
  // Call the base class to hide the view if we're being removed.
  NativeViewHost::ViewHierarchyChanged(is_add, parent, child);

  // Create the widget when we're added to a valid Widget. Many controls need a
  // parent widget to function properly.
  if (is_add && GetWidget() && !native_view())
    CreateNativeControl();
}

void NativeControlGtk::VisibilityChanged(View* starting_from, bool is_visible) {
  if (!is_visible) {
    // We destroy the child widget when we become invisible because of the
    // performance cost of maintaining widgets that aren't currently needed.
    GtkWidget* widget = native_view();
    Detach();
    gtk_widget_destroy(widget);
  } else if (!native_view()) {
    CreateNativeControl();
  }
}

void NativeControlGtk::Focus() {
  DCHECK(native_view());
  gtk_widget_grab_focus(native_view());
}

void NativeControlGtk::NativeControlCreated(GtkWidget* native_control) {
  Attach(native_control);

  // Update the newly created GtkWdigetwith any resident enabled state.
  gtk_widget_set_sensitive(native_view(), IsEnabled());

  // Listen for focus change event to update the FocusManager focused view.
  g_signal_connect(G_OBJECT(native_control), "focus-in-event",
                   G_CALLBACK(CallFocusIn), this);
}

// static
void NativeControlGtk::CallFocusIn(GtkWidget* widget,
                                   GdkEventFocus* event,
                                   NativeControlGtk* control) {
  FocusManager* focus_manager =
      FocusManager::GetFocusManagerForNativeView(widget);
  if (!focus_manager) {
    NOTREACHED();
    return;
  }
  focus_manager->SetFocusedView(control->focus_view());
}

}  // namespace views
