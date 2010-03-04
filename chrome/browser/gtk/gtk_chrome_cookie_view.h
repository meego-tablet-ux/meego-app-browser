// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GTK_GTK_CHROME_COOKIE_VIEW_H_
#define CHROME_BROWSER_GTK_GTK_CHROME_COOKIE_VIEW_H_

#include <gtk/gtk.h>

#include "chrome/browser/browsing_data_appcache_helper.h"
#include "chrome/browser/browsing_data_database_helper.h"
#include "chrome/browser/browsing_data_local_storage_helper.h"
#include "net/base/cookie_monster.h"

G_BEGIN_DECLS

#define GTK_TYPE_CHROME_COOKIE_VIEW gtk_chrome_cookie_view_get_type()

#define GTK_CHROME_COOKIE_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), \
  GTK_TYPE_CHROME_COOKIE_VIEW, GtkChromeCookieView))

#define GTK_CHROME_COOKIE_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), \
  GTK_TYPE_CHROME_COOKIE_VIEW, GtkChromeCookieViewClass))

#define GTK_IS_CHROME_COOKIE_VIEW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
  GTK_TYPE_CHROME_COOKIE_VIEW))

#define GTK_IS_CHROME_COOKIE_VIEW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), \
  GTK_TYPE_CHROME_COOKIE_VIEW))

#define GTK_CHROME_COOKIE_VIEW_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj), \
  GTK_TYPE_CHROME_COOKIE_VIEW, GtkChromeCookieViewClass))

typedef struct {
  GtkFrame parent;

  // All public for testing since I don't think there's a "friend" mechanism in
  // gobject.

  GtkWidget* table_box_;

  // A label we keep around so we can access its GtkStyle* once it is realized.
  GtkWidget* first_label_;

  // The cookie details widgets.
  GtkWidget* cookie_details_table_;
  GtkWidget* cookie_name_entry_;
  GtkWidget* cookie_content_entry_;
  GtkWidget* cookie_domain_entry_;
  GtkWidget* cookie_path_entry_;
  GtkWidget* cookie_send_for_entry_;
  GtkWidget* cookie_created_entry_;
  GtkWidget* cookie_expires_entry_;

  // The database details widgets.
  GtkWidget* database_details_table_;
  GtkWidget* database_name_entry_;
  GtkWidget* database_description_entry_;
  GtkWidget* database_size_entry_;
  GtkWidget* database_last_modified_entry_;

  // The local storage details widgets.
  GtkWidget* local_storage_details_table_;
  GtkWidget* local_storage_origin_entry_;
  GtkWidget* local_storage_size_entry_;
  GtkWidget* local_storage_last_modified_entry_;

  // The appcache details widgets.
  GtkWidget* appcache_details_table_;
  GtkWidget* appcache_manifest_entry_;
  GtkWidget* appcache_size_entry_;
  GtkWidget* appcache_created_entry_;
  GtkWidget* appcache_last_accessed_entry_;
} GtkChromeCookieView;

typedef struct {
  GtkFrameClass parent_class;
} GtkChromeCookieViewClass;

GType gtk_chrome_cookie_view_get_type();

// Builds a new cookie view.
GtkChromeCookieView* gtk_chrome_cookie_view_new();

// Clears the cookie view.
void gtk_chrome_cookie_view_clear(GtkChromeCookieView* widget);

// NOTE: The G_END_DECLS ends here instead of at the end of the document
// because we want to define some methods on GtkChromeCookieView that take C++
// objects.
G_END_DECLS
// NOTE: ^^^^^^^^^^^^^^^^^^^^^^^

// Switches the display to showing the passed in cookie.
void gtk_chrome_cookie_view_display_cookie(
    GtkChromeCookieView* widget,
    const std::string& domain,
    const net::CookieMonster::CanonicalCookie& cookie);

// Switches the display to showing the passed in database.
void gtk_chrome_cookie_view_display_database(
    GtkChromeCookieView* widget,
    const BrowsingDataDatabaseHelper::DatabaseInfo& database_info);

// Switches the display to showing the passed in local storage data.
void gtk_chrome_cookie_view_display_local_storage(
    GtkChromeCookieView* widget,
    const BrowsingDataLocalStorageHelper::LocalStorageInfo&
    local_storage_info);

// Switches the display to showing the passed in app cache.
void gtk_chrome_cookie_view_display_app_cache(
    GtkChromeCookieView* widget,
    const BrowsingDataAppCacheHelper::AppCacheInfo& info);

#endif  // CHROME_BROWSER_GTK_GTK_CHROME_COOKIE_VIEW_H_
