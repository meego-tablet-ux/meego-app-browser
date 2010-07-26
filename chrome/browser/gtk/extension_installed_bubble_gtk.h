// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GTK_EXTENSION_INSTALLED_BUBBLE_GTK_H_
#define CHROME_BROWSER_GTK_EXTENSION_INSTALLED_BUBBLE_GTK_H_
#pragma once

#include "base/ref_counted.h"
#include "base/scoped_ptr.h"
#include "chrome/browser/gtk/custom_button.h"
#include "chrome/browser/gtk/info_bubble_gtk.h"
#include "chrome/common/notification_observer.h"
#include "chrome/common/notification_registrar.h"
#include "third_party/skia/include/core/SkBitmap.h"

class Browser;
class BrowserWindowGtk;
class Extension;
class SkBitmap;

// Provides feedback to the user upon successful installation of an
// extension. Depending on the type of extension, the InfoBubble will
// point to:
//    BROWSER_ACTION -> The browserAction icon in the toolbar.
//    PAGE_ACTION    -> A preview of the page action icon in the location
//                      bar which is shown while the InfoBubble is shown.
//    GENERIC        -> The wrench menu. This case includes page actions that
//                      don't specify a default icon.
//
// ExtensionInstallBubble manages its own lifetime.
class ExtensionInstalledBubbleGtk
    : public InfoBubbleGtkDelegate,
      public NotificationObserver,
      public base::RefCountedThreadSafe<ExtensionInstalledBubbleGtk> {
 public:
  // The behavior and content of this InfoBubble comes in three varieties.
  enum BubbleType {
    BROWSER_ACTION,
    PAGE_ACTION,
    GENERIC
  };

  // Creates the ExtensionInstalledBubble and schedules it to be shown once
  // the extension has loaded. |extension| is the installed extension. |browser|
  // is the browser window which will host the bubble. |icon| is the install
  // icon of the extension.
  static void Show(Extension *extension, Browser *browser, SkBitmap icon);

 private:
  friend class base::RefCountedThreadSafe<ExtensionInstalledBubbleGtk>;

  // Private ctor. Registers a listener for EXTENSION_LOADED.
  ExtensionInstalledBubbleGtk(Extension *extension, Browser *browser,
                              SkBitmap icon);

  ~ExtensionInstalledBubbleGtk() {}

  // Shows the bubble. Called internally via PostTask.
  void ShowInternal();

  // NotificationObserver
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

  // InfoBubbleDelegate
  virtual void InfoBubbleClosing(InfoBubbleGtk* info_bubble,
                                 bool closed_by_escape);

  // Calls Release() internally. Called internally via PostTask.
  void Close();

  static void OnButtonClick(GtkWidget* button,
                            ExtensionInstalledBubbleGtk* toolbar);

  Extension *extension_;
  Browser *browser_;
  SkBitmap icon_;
  NotificationRegistrar registrar_;
  BubbleType type_;

  // The 'x' that the user can press to hide the info bubble shelf.
  scoped_ptr<CustomDrawButton> close_button_;

  InfoBubbleGtk* info_bubble_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionInstalledBubbleGtk);
};

#endif  // CHROME_BROWSER_GTK_EXTENSION_INSTALLED_BUBBLE_GTK_H_
