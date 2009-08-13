// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GTK_VIEW_ID_UTIL_H_
#define CHROME_BROWSER_GTK_VIEW_ID_UTIL_H_

#include "chrome/browser/view_ids.h"

typedef struct _GtkWidget GtkWidget;

class ViewIDUtil {
 public:
  // Use this delegate to override default view id searches.
  class Delegate {
   public:
    virtual GtkWidget* GetWidgetForViewID(ViewID id) = 0;
  };

  static void SetID(GtkWidget* widget, ViewID id);

  static GtkWidget* GetWidget(GtkWidget* root, ViewID id);

  static void SetDelegateForWidget(GtkWidget* widget, Delegate* delegate);
};

#endif  // CHROME_BROWSER_GTK_VIEW_ID_UTIL_H_
