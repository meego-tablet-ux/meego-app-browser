// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_HOST_GTK_KEY_BINDINGS_HANDLER_H_
#define CHROME_BROWSER_RENDERER_HOST_GTK_KEY_BINDINGS_HANDLER_H_
#pragma once

#include <gtk/gtk.h>

#include <string>

#include "chrome/browser/gtk/owned_widget_gtk.h"
#include "chrome/common/edit_command.h"

struct NativeWebKeyboardEvent;

// This class is a convenience class for handling editor key bindings defined
// in gtk keyboard theme.
// In gtk, only GtkEntry and GtkTextView support customizing editor key bindings
// through keyboard theme. And in gtk keyboard theme definition file, each key
// binding must be bound to a specific class or object. So existing keyboard
// themes only define editor key bindings exactly for GtkEntry and GtkTextView.
// Then, the only way for us to intercept editor key bindings defined in
// keyboard theme, is to create a GtkEntry or GtkTextView object and call
// gtk_bindings_activate_event() against it for the key events. If a key event
// matches a predefined key binding, corresponding signal will be emitted.
// GtkTextView is used here because it supports more key bindings than GtkEntry,
// but in order to minimize the side effect of using a GtkTextView object, a new
// class derived from GtkTextView is used, which overrides all signals related
// to key bindings, to make sure GtkTextView won't receive them.
//
// See third_party/WebKit/WebCore/editing/EditorCommand.cpp for detailed
// definition of webkit edit commands.
// See webkit/glue/editor_client_impl.cc for key bindings predefined in our
// webkit glue.
class GtkKeyBindingsHandler {
 public:
  explicit GtkKeyBindingsHandler(GtkWidget* parent_widget);
  ~GtkKeyBindingsHandler();

  // Matches a key event against predefined gtk key bindings, false will be
  // returned if the key event doesn't correspond to a predefined key binding.
  // Edit commands matched with |wke| will be stored in |edit_commands|.
  bool Match(const NativeWebKeyboardEvent& wke, EditCommands* edit_commands);

 private:
  // Object structure of Handler class, which is derived from GtkTextView.
  struct Handler {
    GtkTextView parent_object;
    GtkKeyBindingsHandler *owner;
  };

  // Class structure of Handler class.
  struct HandlerClass {
    GtkTextViewClass parent_class;
  };

  // Creates a new instance of Handler class.
  GtkWidget* CreateNewHandler();

  // Adds an edit command to the key event.
  void EditCommandMatched(const std::string& name, const std::string& value);

  // Initializes Handler structure.
  static void HandlerInit(Handler *self);

  // Initializes HandlerClass structure.
  static void HandlerClassInit(HandlerClass *klass);

  // Registeres Handler class to GObject type system and return its type id.
  static GType HandlerGetType();

  // Gets the GtkKeyBindingsHandler object which owns the Handler object.
  static GtkKeyBindingsHandler* GetHandlerOwner(GtkTextView* text_view);

  // Handler of "backspace" signal.
  static void BackSpace(GtkTextView* text_view);

  // Handler of "copy-clipboard" signal.
  static void CopyClipboard(GtkTextView* text_view);

  // Handler of "cut-clipboard" signal.
  static void CutClipboard(GtkTextView* text_view);

  // Handler of "delete-from-cursor" signal.
  static void DeleteFromCursor(GtkTextView* text_view, GtkDeleteType type,
                               gint count);

  // Handler of "insert-at-cursor" signal.
  static void InsertAtCursor(GtkTextView* text_view, const gchar* str);

  // Handler of "move-cursor" signal.
  static void MoveCursor(GtkTextView* text_view, GtkMovementStep step,
                         gint count, gboolean extend_selection);

  // Handler of "move-viewport" signal.
  static void MoveViewport(GtkTextView* text_view, GtkScrollStep step,
                           gint count);

  // Handler of "paste-clipboard" signal.
  static void PasteClipboard(GtkTextView* text_view);

  // Handler of "select-all" signal.
  static void SelectAll(GtkTextView* text_view, gboolean select);

  // Handler of "set-anchor" signal.
  static void SetAnchor(GtkTextView* text_view);

  // Handler of "toggle-cursor-visible" signal.
  static void ToggleCursorVisible(GtkTextView* text_view);

  // Handler of "toggle-overwrite" signal.
  static void ToggleOverwrite(GtkTextView* text_view);

  // Handler of "show-help" signal.
  static gboolean ShowHelp(GtkWidget* widget, GtkWidgetHelpType arg1);

  // Handler of "move-focus" signal.
  static void MoveFocus(GtkWidget* widget, GtkDirectionType arg1);

  OwnedWidgetGtk handler_;

  // Buffer to store the match results.
  EditCommands edit_commands_;
};

#endif  // CHROME_BROWSER_RENDERER_HOST_GTK_KEY_BINDINGS_HANDLER_H_
