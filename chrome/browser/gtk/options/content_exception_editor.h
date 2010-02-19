// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GTK_OPTIONS_CONTENT_EXCEPTION_EDITOR_H_
#define CHROME_BROWSER_GTK_OPTIONS_CONTENT_EXCEPTION_EDITOR_H_

#include <gtk/gtk.h>

#include <string>

#include "chrome/browser/content_exceptions_table_model.h"
#include "chrome/common/content_settings.h"
#include "chrome/common/content_settings_types.h"

// An editor which lets the user create or edit an individual exception to the
// current content setting policy. (i.e. let www.google.com always show
// images). Modal to parent.
class ContentExceptionEditor {
 public:
  class Delegate {
   public:
    // Invoked when the user accepts the edit.
    virtual void AcceptExceptionEdit(const std::string& host,
                                     ContentSetting setting,
                                     int index,
                                     bool is_new) = 0;

   protected:
    virtual ~Delegate() {}
  };

  ContentExceptionEditor(GtkWindow* parent,
                         Delegate* delegate,
                         ContentExceptionsTableModel* model,
                         int index,
                         const std::string& host,
                         ContentSetting setting);

 private:
  // Returns true if we're adding a new item.
  bool is_new() const { return index_ == -1; }

  // Returns the number of items in the |action_combo_|.
  int GetItemCount();

  // Returns the string representation for an item in |action_combo_|.
  std::string GetTitleFor(int index);

  // Changes an index from |action_combo_| into a ContentSetting and vice versa.
  ContentSetting SettingForIndex(int index);
  int IndexForSetting(ContentSetting setting);

  // GTK callbacks
  static void OnEntryChanged(GtkEditable* entry,
                             ContentExceptionEditor* window);
  static void OnResponse(GtkWidget* sender,
                         int response_id,
                         ContentExceptionEditor* window);
  static void OnWindowDestroy(GtkWidget* widget,
                              ContentExceptionEditor* editor);

  Delegate* delegate_;
  ContentExceptionsTableModel* model_;
  bool show_ask_;

  // Index of the item being edited. If -1, indicates this is a new entry.
  const int index_;
  const std::string host_;
  const ContentSetting setting_;

  // UI widgets.
  GtkWidget* dialog_;
  GtkWidget* entry_;
  GtkWidget* action_combo_;

  DISALLOW_COPY_AND_ASSIGN(ContentExceptionEditor);
};

#endif  // CHROME_BROWSER_GTK_OPTIONS_CONTENT_EXCEPTION_EDITOR_H_
