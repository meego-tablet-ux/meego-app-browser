// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/gtk/options/content_exceptions_window_gtk.h"

#include <set>

#include "app/l10n_util.h"
#include "base/message_loop.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/gtk/gtk_util.h"
#include "chrome/browser/gtk/options/content_exception_editor.h"
#include "gfx/gtk_util.h"
#include "grit/generated_resources.h"
#include "grit/locale_settings.h"

namespace {

// Singletons for each possible exception window.
ContentExceptionsWindowGtk* instances[CONTENT_SETTINGS_NUM_TYPES] = { NULL };

}  // namespace

// static
void ContentExceptionsWindowGtk::ShowExceptionsWindow(
    GtkWindow* parent,
    HostContentSettingsMap* map,
    ContentSettingsType type) {
  DCHECK(map);
  DCHECK(type < CONTENT_SETTINGS_NUM_TYPES);
  // Geolocation exceptions are handled by GeolocationContentExceptionsWindow.
  DCHECK(type != CONTENT_SETTINGS_TYPE_GEOLOCATION);

  if (!instances[type]) {
    // Create the options window.
    instances[type] = new ContentExceptionsWindowGtk(parent, map, type);
  } else {
    gtk_util::PresentWindow(instances[type]->dialog_, 0);
  }
}

ContentExceptionsWindowGtk::~ContentExceptionsWindowGtk() {
}

ContentExceptionsWindowGtk::ContentExceptionsWindowGtk(
    GtkWindow* parent,
    HostContentSettingsMap* map,
    ContentSettingsType type) {
  // Build the model adapters that translate views and TableModels into
  // something GTK can use.
  list_store_ = gtk_list_store_new(COL_COUNT, G_TYPE_STRING, G_TYPE_STRING);
  treeview_ = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list_store_));
  g_object_unref(list_store_);

  // Set up the properties of the treeview
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview_), TRUE);
  g_signal_connect(treeview_, "row-activated",
                   G_CALLBACK(OnTreeViewRowActivateThunk), this);

  GtkTreeViewColumn* pattern_column = gtk_tree_view_column_new_with_attributes(
      l10n_util::GetStringUTF8(IDS_EXCEPTIONS_PATTERN_HEADER).c_str(),
      gtk_cell_renderer_text_new(),
      "text", COL_PATTERN,
      NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeview_), pattern_column);
  gtk_tree_view_column_set_sort_column_id(pattern_column, COL_PATTERN);

  GtkTreeViewColumn* action_column = gtk_tree_view_column_new_with_attributes(
      l10n_util::GetStringUTF8(IDS_EXCEPTIONS_ACTION_HEADER).c_str(),
      gtk_cell_renderer_text_new(),
      "text", COL_ACTION,
      NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeview_), action_column);
  gtk_tree_view_column_set_sort_column_id(action_column, COL_ACTION);

  treeview_selection_ = gtk_tree_view_get_selection(
      GTK_TREE_VIEW(treeview_));
  gtk_tree_selection_set_mode(treeview_selection_, GTK_SELECTION_MULTIPLE);
  g_signal_connect(treeview_selection_, "changed",
                   G_CALLBACK(OnTreeSelectionChangedThunk), this);

  // Bind |list_store_| to our C++ model.
  model_.reset(new ContentExceptionsTableModel(map, type));
  model_adapter_.reset(new gtk_tree::TableAdapter(this, list_store_,
                                                  model_.get()));
  // Force a reload of everything to copy data into |list_store_|.
  model_adapter_->OnModelChanged();

  dialog_ = gtk_dialog_new_with_buttons(
      GetWindowTitle().c_str(),
      parent,
      static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR),
      GTK_STOCK_CLOSE,
      GTK_RESPONSE_CLOSE,
      NULL);
  gtk_window_set_default_size(GTK_WINDOW(dialog_), 500, -1);
  // Allow browser windows to go in front of the options dialog in metacity.
  gtk_window_set_type_hint(GTK_WINDOW(dialog_), GDK_WINDOW_TYPE_HINT_NORMAL);
  gtk_box_set_spacing(GTK_BOX(GTK_DIALOG(dialog_)->vbox),
                      gtk_util::kContentAreaSpacing);

  GtkWidget* hbox = gtk_hbox_new(FALSE, gtk_util::kControlSpacing);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog_)->vbox), hbox);

  // Create a scrolled window to wrap the treeview widget.
  GtkWidget* scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolled),
                                      GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(scrolled), treeview_);
  gtk_box_pack_start(GTK_BOX(hbox), scrolled, TRUE, TRUE, 0);

  GtkWidget* button_box = gtk_vbox_new(FALSE, gtk_util::kControlSpacing);

  GtkWidget* add_button = gtk_util::BuildDialogButton(dialog_,
                                                      IDS_EXCEPTIONS_ADD_BUTTON,
                                                      GTK_STOCK_ADD);
  g_signal_connect(add_button, "clicked", G_CALLBACK(AddThunk), this);
  gtk_box_pack_start(GTK_BOX(button_box), add_button, FALSE, FALSE, 0);

  edit_button_ = gtk_util::BuildDialogButton(dialog_,
                                             IDS_EXCEPTIONS_EDIT_BUTTON,
                                             GTK_STOCK_EDIT);
  g_signal_connect(edit_button_, "clicked", G_CALLBACK(EditThunk), this);
  gtk_box_pack_start(GTK_BOX(button_box), edit_button_, FALSE, FALSE, 0);

  remove_button_ = gtk_util::BuildDialogButton(dialog_,
                                               IDS_EXCEPTIONS_REMOVE_BUTTON,
                                               GTK_STOCK_REMOVE);
  g_signal_connect(remove_button_, "clicked", G_CALLBACK(RemoveThunk), this);
  gtk_box_pack_start(GTK_BOX(button_box), remove_button_, FALSE, FALSE, 0);

  remove_all_button_ = gtk_util::BuildDialogButton(
      dialog_,
      IDS_EXCEPTIONS_REMOVEALL_BUTTON,
      GTK_STOCK_CLEAR);
  g_signal_connect(remove_all_button_, "clicked", G_CALLBACK(RemoveAllThunk),
                   this);
  gtk_box_pack_start(GTK_BOX(button_box), remove_all_button_, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(hbox), button_box, FALSE, FALSE, 0);

  UpdateButtonState();

  gtk_util::ShowDialogWithLocalizedSize(dialog_,
      IDS_CONTENT_EXCEPTION_DIALOG_WIDTH_CHARS,
      -1,
      true);

  g_signal_connect(dialog_, "response", G_CALLBACK(gtk_widget_destroy), NULL);
  g_signal_connect(dialog_, "destroy", G_CALLBACK(OnWindowDestroyThunk), this);
}

void ContentExceptionsWindowGtk::SetColumnValues(int row, GtkTreeIter* iter) {
  std::wstring pattern = model_->GetText(row, IDS_EXCEPTIONS_PATTERN_HEADER);
  gtk_list_store_set(list_store_, iter, COL_PATTERN,
                     WideToUTF8(pattern).c_str(), -1);

  std::wstring action = model_->GetText(row, IDS_EXCEPTIONS_ACTION_HEADER);
  gtk_list_store_set(list_store_, iter, COL_ACTION,
                     WideToUTF8(action).c_str(), -1);
}

void ContentExceptionsWindowGtk::AcceptExceptionEdit(
    const HostContentSettingsMap::Pattern& pattern,
    ContentSetting setting,
    int index,
    bool is_new) {
  if (!is_new)
    model_->RemoveException(index);

  model_->AddException(pattern, setting);

  int new_index = model_->IndexOfExceptionByPattern(pattern);
  DCHECK_NE(-1, new_index);

  gtk_tree::SelectAndFocusRowNum(new_index, GTK_TREE_VIEW(treeview_));

  UpdateButtonState();
}

void ContentExceptionsWindowGtk::UpdateButtonState() {
  int num_selected = gtk_tree_selection_count_selected_rows(
      treeview_selection_);
  int row_count = gtk_tree_model_iter_n_children(
      GTK_TREE_MODEL(list_store_), NULL);

  // TODO(erg): http://crbug.com/34177 , support editing of more than one entry
  // at a time.
  gtk_widget_set_sensitive(edit_button_, num_selected == 1);
  gtk_widget_set_sensitive(remove_button_, num_selected >= 1);
  gtk_widget_set_sensitive(remove_all_button_, row_count > 0);
}

void ContentExceptionsWindowGtk::Add(GtkWidget* widget) {
  new ContentExceptionEditor(GTK_WINDOW(dialog_),
                             this, model_.get(), -1,
                             HostContentSettingsMap::Pattern(),
                             CONTENT_SETTING_BLOCK);
}

void ContentExceptionsWindowGtk::Edit(GtkWidget* widget) {
  std::set<int> indices;
  gtk_tree::GetSelectedIndices(treeview_selection_, &indices);
  DCHECK_GT(indices.size(), 0u);
  int index = *indices.begin();
  const HostContentSettingsMap::PatternSettingPair& entry =
      model_->entry_at(index);
  new ContentExceptionEditor(GTK_WINDOW(dialog_), this, model_.get(), index,
                             entry.first, entry.second);
}

void ContentExceptionsWindowGtk::Remove(GtkWidget* widget) {
  std::set<int> selected_indices;
  gtk_tree::GetSelectedIndices(treeview_selection_, &selected_indices);

  int selected_row = 0;
  for (std::set<int>::reverse_iterator i = selected_indices.rbegin();
       i != selected_indices.rend(); ++i) {
    model_->RemoveException(*i);
    selected_row = *i;
  }

  int row_count = model_->RowCount();
  if (row_count <= 0)
    return;
  if (selected_row >= row_count)
    selected_row = row_count - 1;
  gtk_tree::SelectAndFocusRowNum(selected_row,
                                 GTK_TREE_VIEW(treeview_));

  UpdateButtonState();
}

void ContentExceptionsWindowGtk::RemoveAll(GtkWidget* widget) {
  model_->RemoveAll();
  UpdateButtonState();
}

std::string ContentExceptionsWindowGtk::GetWindowTitle() const {
  switch (model_->content_type()) {
    case CONTENT_SETTINGS_TYPE_COOKIES:
      return l10n_util::GetStringUTF8(IDS_COOKIE_EXCEPTION_TITLE);
    case CONTENT_SETTINGS_TYPE_IMAGES:
      return l10n_util::GetStringUTF8(IDS_IMAGES_EXCEPTION_TITLE);
    case CONTENT_SETTINGS_TYPE_JAVASCRIPT:
      return l10n_util::GetStringUTF8(IDS_JS_EXCEPTION_TITLE);
    case CONTENT_SETTINGS_TYPE_PLUGINS:
      return l10n_util::GetStringUTF8(IDS_PLUGINS_EXCEPTION_TITLE);
    case CONTENT_SETTINGS_TYPE_POPUPS:
      return l10n_util::GetStringUTF8(IDS_POPUP_EXCEPTION_TITLE);
    default:
      NOTREACHED();
  }
  return std::string();
}

void ContentExceptionsWindowGtk::OnTreeViewRowActivate(
    GtkWidget* sender,
    GtkTreePath* path,
    GtkTreeViewColumn* column) {
  Edit(sender);
}

void ContentExceptionsWindowGtk::OnWindowDestroy(GtkWidget* widget) {
  instances[model_->content_type()] = NULL;
  MessageLoop::current()->DeleteSoon(FROM_HERE, this);
}

void ContentExceptionsWindowGtk::OnTreeSelectionChanged(
    GtkWidget* selection) {
  UpdateButtonState();
}
