// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GTK_OPTIONS_URL_PICKER_DIALOG_GTK_H_
#define CHROME_BROWSER_GTK_OPTIONS_URL_PICKER_DIALOG_GTK_H_

#include "base/basictypes.h"
#include "base/task.h"
#include "chrome/browser/history/history.h"
#include "chrome/common/gtk_tree.h"

class GURL;
class Profile;
class PossibleURLModel;

class UrlPickerDialogGtk : public gtk_tree::ModelAdapter::Delegate {
 public:
  typedef Callback1<const GURL&>::Type UrlPickerCallback;

  UrlPickerDialogGtk(UrlPickerCallback* callback,
                     Profile* profile,
                     GtkWindow* parent);

  ~UrlPickerDialogGtk();

  // gtk_tree::ModelAdapter::Delegate implementation.
  virtual void SetColumnValues(int row, GtkTreeIter* iter);

 private:
  // Call the callback based on url entry.
  void AddURL();

  // Set sensitivity of buttons based on url entry state.
  void EnableControls();

  // Return the entry-formatted url for path in the sorted model.
  std::string GetURLForPath(GtkTreePath* path) const;

  // GTK sorting callbacks.
  static gint CompareTitle(GtkTreeModel* model, GtkTreeIter* a, GtkTreeIter* b,
                           gpointer window);
  static gint CompareURL(GtkTreeModel* model, GtkTreeIter* a, GtkTreeIter* b,
                         gpointer window);

  // Callback for URL entry changes.
  static void OnUrlEntryChanged(GtkEditable* editable,
                                UrlPickerDialogGtk* window);

  // Callback for user selecting rows in recent history list.
  static void OnHistorySelectionChanged(GtkTreeSelection* selection,
                                        UrlPickerDialogGtk* window);

  // Callback for user activating a row in recent history list.
  static void OnHistoryRowActivated(GtkTreeView* tree_view,
                                    GtkTreePath* path,
                                    GtkTreeViewColumn* column,
                                    UrlPickerDialogGtk* window);

  // Callback for dialog buttons.
  static void OnResponse(GtkDialog* dialog, int response_id,
                         UrlPickerDialogGtk* window);

  // Callback for window destruction.
  static void OnWindowDestroy(GtkWidget* widget, UrlPickerDialogGtk* window);

  // The dialog window.
  GtkWidget* dialog_;

  // The text entry for manually adding an URL.
  GtkWidget* url_entry_;

  // The add button (we need a reference to it so we can de-activate it when the
  // |url_entry_| is empty.)
  GtkWidget* add_button_;

  // The recent history list.
  GtkWidget* history_tree_;
  GtkListStore* history_list_store_;
  GtkTreeModel* history_list_sort_;
  GtkTreeSelection* history_selection_;

  // Profile.
  Profile* profile_;

  // The table model.
  scoped_ptr<PossibleURLModel> url_table_model_;
  scoped_ptr<gtk_tree::ModelAdapter> url_table_adapter_;

  // Called if the user selects an url.
  UrlPickerCallback* callback_;

  DISALLOW_COPY_AND_ASSIGN(UrlPickerDialogGtk);
};

#endif  // CHROME_BROWSER_GTK_OPTIONS_URL_PICKER_DIALOG_GTK_H_
