// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/gtk/options/options_layout_gtk.h"

namespace {

// Style for option group titles
const char kGroupTitleMarkup[] =
    "<span weight='bold'>%s</span>";

// Space around the outsides of the page
const int kBorderSize = 12;

// Indent of the options within each group
const int kOptionsIndent = 12;

// Spacing between options of the same group
const int kOptionSpacing = 6;

// Spacing between groups
const int kGroupSpacing = 18;

}

OptionsLayoutBuilderGtk::OptionsLayoutBuilderGtk(int num_rows) {
  page_ = gtk_vbox_new(FALSE, kGroupSpacing);
  gtk_container_set_border_width(GTK_CONTAINER(page_), kBorderSize);
}

void OptionsLayoutBuilderGtk::AddOptionGroup(const std::string& title,
                                           GtkWidget* content) {
  GtkWidget* title_label = gtk_label_new(NULL);
  char* markup = g_markup_printf_escaped(kGroupTitleMarkup,
                                         title.c_str());
  gtk_label_set_markup(GTK_LABEL(title_label), markup);
  g_free(markup);

  GtkWidget* title_alignment = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
  gtk_container_add(GTK_CONTAINER(title_alignment), title_label);

  GtkWidget* content_alignment = gtk_alignment_new(0.0, 0.5, 1.0, 1.0);
  gtk_alignment_set_padding(GTK_ALIGNMENT(content_alignment), 0, 0,
                            kOptionsIndent, 0);
  gtk_container_add(GTK_CONTAINER(content_alignment), content);

  GtkWidget* group = gtk_vbox_new(FALSE, kOptionSpacing);
  gtk_container_add(GTK_CONTAINER(group), title_alignment);
  gtk_container_add(GTK_CONTAINER(group), content_alignment);

  gtk_box_pack_start(GTK_BOX(page_), group, FALSE, FALSE, 0);
}
