// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is the GTK implementation of the First Run bubble, the dialog box
// presented on first run of Chromium. There can only ever be a single
// bubble open, so the class presents only static methods.

#ifndef CHROME_BROWSER_GTK_FIRST_RUN_BUBBLE_GTK_H_
#define CHROME_BROWSER_GTK_FIRST_RUN_BUBBLE_GTK_H_

#include <gtk/gtk.h>

#include "base/basictypes.h"
#include "chrome/browser/gtk/info_bubble_gtk.h"
#include "chrome/browser/profile.h"

class FirstRunBubble : public InfoBubbleGtkDelegate {
 public:
  // Shows the first run bubble, pointing at |rect|.
  static void Show(Profile* profile,
                   GtkWindow* parent,
                   const gfx::Rect& rect,
                   bool use_OEM_bubble);

  // Implements the InfoBubbleGtkDelegate.  We are notified when the bubble
  // is about to be closed.
  virtual void InfoBubbleClosing(InfoBubbleGtk* info_bubble,
                                 bool closed_by_escape);
  virtual bool CloseOnEscape() { return true; }

 private:
  FirstRunBubble(Profile* profile,
                 GtkWindow* parent,
                 const gfx::Rect& rect);
  ~FirstRunBubble() { }

  static void HandleChangeButtonThunk(GtkWidget* widget, gpointer user_data) {
    reinterpret_cast<FirstRunBubble*>(user_data)->HandleChangeButton();
  }
  void HandleChangeButton();

  static void HandleDestroyThunk(GtkWidget* widget, gpointer userdata) {
    reinterpret_cast<FirstRunBubble*>(userdata)->HandleDestroy();
  }
  void HandleDestroy();

  static void HandleKeepButtonThunk(GtkWidget* widget, gpointer user_data) {
    reinterpret_cast<FirstRunBubble*>(user_data)->HandleKeepButton();
  }
  void HandleKeepButton();

  // Our current profile.
  Profile* profile_;

  // The toplevel window our dialogs should be transient for.
  GtkWindow* parent_;

  // We let the InfoBubble own our content, and then we delete ourself
  // when the widget is destroyed (when the InfoBubble is destroyed).
  GtkWidget* content_;

  InfoBubbleGtk* bubble_;

  DISALLOW_COPY_AND_ASSIGN(FirstRunBubble);
};

#endif  // CHROME_BROWSER_GTK_FIRST_RUN_BUBBLE_GTK_H_
