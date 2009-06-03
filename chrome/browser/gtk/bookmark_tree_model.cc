// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/gtk/bookmark_tree_model.h"

#include <gtk/gtk.h>

#include "app/resource_bundle.h"
#include "base/string_util.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "grit/app_resources.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"

namespace {

// Helper function for BuildTreeStoreFrom() which recursively inserts data from
// a BookmarkNode tree into a GtkTreeStore.
void RecursiveInsert(BookmarkNode* node, int selected_id,
                     GtkTreeStore* store, GtkTreeIter* selected_iter,
                     GtkTreeIter* parent) {
  GtkTreeIter iter;

  for (int i = 0; i < node->GetChildCount(); ++i) {
    BookmarkNode* child = node->GetChild(i);
    if (child->is_folder()) {
      gtk_tree_store_append(store, &iter, parent);
      // TODO(estade): we should show the folder open when it's expanded.
      gtk_tree_store_set(store, &iter,
                         bookmark_utils::FOLDER_ICON,
                         bookmark_utils::GetFolderIcon(),
                         bookmark_utils::FOLDER_NAME,
                         WideToUTF8(child->GetTitle()).c_str(),
                         bookmark_utils::ITEM_ID, child->id(),
                         -1);
      if (selected_id && child->id() == selected_id) {
        // Save the iterator. Since we're using a GtkTreeStore, we're
        // guaranteed that the iterator will remain valid as long as the above
        // appended item exists.
        *selected_iter = iter;
      }

      RecursiveInsert(child, selected_id, store, selected_iter, &iter);
    }
  }
}

// Helper function for CommitTreeStoreDifferencesBetween() which recursively
// merges changes back from a GtkTreeStore into a tree of BookmarkNodes. This
// function only works on non-root nodes; our caller handles that special case.
void RecursiveResolve(BookmarkModel* bb_model, BookmarkNode* bb_node,
                      GtkTreeModel* tree_model, GtkTreeIter* parent_iter,
                      GtkTreePath* selected_path,
                      BookmarkNode** selected_node) {
  GtkTreePath* current_path = gtk_tree_model_get_path(tree_model, parent_iter);
  if (gtk_tree_path_compare(current_path, selected_path) == 0)
    *selected_node = bb_node;
  gtk_tree_path_free(current_path);

  GtkTreeIter child_iter;
  if (gtk_tree_model_iter_children(tree_model, &child_iter, parent_iter)) {
    do {
      int id = bookmark_utils::GetIdFromTreeIter(tree_model, &child_iter);
      std::wstring title =
          bookmark_utils::GetTitleFromTreeIter(tree_model, &child_iter);
      BookmarkNode* child_bb_node = NULL;
      if (id == 0) {
        child_bb_node = bb_model->AddGroup(bb_node, bb_node->GetChildCount(),
                                           title);
      } else {
        // Existing node, reset the title (BBModel ignores changes if the title
        // is the same).
        for (int j = 0; j < bb_node->GetChildCount(); ++j) {
          BookmarkNode* node = bb_node->GetChild(j);
          if (node->is_folder() && node->id() == id) {
            child_bb_node = node;
            break;
          }
        }
        DCHECK(child_bb_node);
        bb_model->SetTitle(child_bb_node, title);
      }
      RecursiveResolve(bb_model, child_bb_node,
                       tree_model, &child_iter,
                       selected_path, selected_node);
    } while (gtk_tree_model_iter_next(tree_model, &child_iter));
  }
}

}  // namespace

namespace bookmark_utils {

GtkTreeStore* MakeFolderTreeStore() {
  return gtk_tree_store_new(FOLDER_STORE_NUM_COLUMNS, GDK_TYPE_PIXBUF,
                            G_TYPE_STRING, G_TYPE_INT);
}

void AddToTreeStore(BookmarkModel* model, int selected_id,
                    GtkTreeStore* store, GtkTreeIter* selected_iter) {
  RecursiveInsert(model->root_node(), selected_id, store, selected_iter, NULL);
}

BookmarkNode* CommitTreeStoreDifferencesBetween(
    BookmarkModel* bb_model, GtkTreeStore* tree_store, GtkTreeIter* selected) {
  BookmarkNode* node_to_return = NULL;
  GtkTreeModel* tree_model = GTK_TREE_MODEL(tree_store);

  GtkTreePath* selected_path = gtk_tree_model_get_path(tree_model, selected);

  GtkTreeIter tree_root;
  if (!gtk_tree_model_get_iter_first(tree_model, &tree_root))
    NOTREACHED() << "Impossible missing bookmarks case";

  // The top level of this tree is weird and needs to be special cased. The
  // BookmarksNode tree is rooted on a root node while the GtkTreeStore has a
  // set of top level nodes that are the root BookmarksNode's children. These
  // items in the top level are not editable and therefore don't need the extra
  // complexity of trying to modify their title.
  BookmarkNode* root_node = bb_model->root_node();
  do {
    DCHECK(GetIdFromTreeIter(tree_model, &tree_root) != 0)
        << "It should be impossible to add another toplevel node";

    int id = GetIdFromTreeIter(tree_model, &tree_root);
    BookmarkNode* child_node = NULL;
    for (int j = 0; j < root_node->GetChildCount(); ++j) {
      BookmarkNode* node = root_node->GetChild(j);
      if (node->is_folder() && node->id() == id) {
        child_node = node;
        break;
      }
    }
    DCHECK(child_node);

    GtkTreeIter child_iter = tree_root;
    RecursiveResolve(bb_model, child_node, tree_model, &child_iter,
                     selected_path, &node_to_return);
  } while (gtk_tree_model_iter_next(tree_model, &tree_root));

  gtk_tree_path_free(selected_path);
  return node_to_return;
}

int GetIdFromTreeIter(GtkTreeModel* model, GtkTreeIter* iter) {
  GValue value = { 0, };
  int ret_val = -1;
  gtk_tree_model_get_value(model, iter, ITEM_ID, &value);
  if (G_VALUE_HOLDS_INT(&value))
    ret_val = g_value_get_int(&value);
  else
    NOTREACHED() << "Impossible type mismatch";

  return ret_val;
}

std::wstring GetTitleFromTreeIter(GtkTreeModel* model, GtkTreeIter* iter) {
  GValue value = { 0, };
  std::wstring ret_val;
  gtk_tree_model_get_value(model, iter, FOLDER_NAME, &value);
  if (G_VALUE_HOLDS_STRING(&value)) {
    const gchar* utf8str = g_value_get_string(&value);
    ret_val = UTF8ToWide(utf8str);
    g_value_unset(&value);
  } else {
    NOTREACHED() << "Impossible type mismatch";
  }

  return ret_val;
}

GdkPixbuf* GetFolderIcon() {
  ResourceBundle& rb = ResourceBundle::GetSharedInstance();
  static GdkPixbuf* default_folder_icon = rb.GetPixbufNamed(
      IDR_BOOKMARK_BAR_FOLDER);
  return default_folder_icon;
}

GdkPixbuf* GetDefaultFavicon() {
  ResourceBundle& rb = ResourceBundle::GetSharedInstance();
  static GdkPixbuf* default_bookmark_icon = rb.GetPixbufNamed(
      IDR_DEFAULT_FAVICON);
  return default_bookmark_icon;
}

}  // namespace bookmark_utils
