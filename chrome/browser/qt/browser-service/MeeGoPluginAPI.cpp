/*
 * Copyright (c) 2010, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>

#include <dbus/dbus-glib.h>

#include "MeeGoPluginAPI.h"

#define NETPANEL_DIRNAME "internet-panel"

#define DEFAULT_MODE (S_IRUSR | S_IWUSR |S_IXUSR) | (S_IRGRP | S_IXGRP) | (S_IROTH | S_IXOTH)

#define CREATE_URL_TABLE_STMT "CREATE TABLE IF NOT EXISTS urls \
	(id INTEGER PRIMARY KEY, url LONGVARCHAR, title LONGVARCHAR, \
	 visit_count INTEGER DEFAULT 0 NOT NULL, \
	 typed_count INTEGER DEFAULT 0 NOT NULL, \
	 last_visit_time INTEGER NOT NULL, \
	 favicon_url LONGVARCHAR)"

#define CREATE_BOOKMARK_TABLE_STMT "CREATE TABLE IF NOT EXISTS bookmarks \
	(id INTEGER PRIMARY KEY, url LONGVARCHAR, title LONGVARCHAR, \
	 dateAdded INTEGER DEFAULT 0, favicon_url LONGVARCHAR)"

#define CREATE_THUMBNAIL_TABLE_STMT "CREATE TABLE IF NOT EXISTS thumbnails \
	(id INTEGER PRIMARY KEY, url LONGCHARVAR UNIQUE, last_update_time INTEGER)"

#define CREATE_TABS_TABLE_STMT "CREATE TABLE IF NOT EXISTS current_tabs \
        (id INTEGER PRIMARY KEY, tab_id INTEGER, win_id INTEGER, url LONGVARCHAR,\
         title LONGVARCHAR, favicon_url LONGVARCHAR)"

#define CREATE_ICON_TABLE_STMT "CREATE TABLE IF NOT EXISTS favicons \
	(id INTEGER PRIMARY KEY, url LONGVARCHAR UNIQUE, last_update_time INTEGER)"

#include "BrowserService-glue.h"
#include "BrowserService-marshaller.h"

#include "BrowserServiceWrapper.h"

//
// MeeGoPluginAPI Implementation
//
gboolean MeeGoPluginAPI::removeUrlByExtension(const char* url)
{
  DBG("removeUrl by extension: %s", url);
  wrapper_->RemoveUrl(url);
  return TRUE;
}

gboolean MeeGoPluginAPI::removeBookmarkByExtension(const char* id)
{
  DBG("removeBookmark by extension: %s", id);  
  wrapper_->RemoveBookmark(id);
  
  return TRUE;
}

void MeeGoPluginAPI::openWebPage(const char* url)
{
  DBG("open web page: %s", url);
  wrapper_->SelectTabByUrl(url);
}

void MeeGoPluginAPI::init_browser_service()
{
  DBusGConnection* bus = NULL;
  DBusGProxy*      proxy = NULL;
  guint request_name_result;

  GError* error = NULL;
  g_type_init();

  dbus_g_object_type_install_info(BROWSER_SERVICE_TYPE, &dbus_glib_browser_service_object_info);

  bus = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
  if(!bus) 
  {
    g_warning("Can't connect to session bus: %s", error->message);
    return;
  }

  proxy = dbus_g_proxy_new_for_name(bus, "org.freedesktop.DBus",
                                    "/org/freedesktop/DBus", "org.freedesktop.DBus");

  if(!dbus_g_proxy_call(proxy, "RequestName", &error,
                        G_TYPE_STRING, "com.meego.browser.BrowserService",
                        G_TYPE_UINT, 0,
                        G_TYPE_INVALID,
                        G_TYPE_UINT, &request_name_result,
                        G_TYPE_INVALID))
  {
    g_warning("failed to get com.meego.browser.BrowserService: %s", error->message);
    return;
  }

  m_browserService = browser_service_new(this);
	
  dbus_g_object_register_marshaller(browser_service_marshal_VOID__INT64_STRING_STRING_STRING,
                                    G_TYPE_NONE, G_TYPE_INT64, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING); 
  dbus_g_object_register_marshaller(browser_service_marshal_VOID__STRING_UINT,
                                    G_TYPE_NONE, G_TYPE_UINT, G_TYPE_STRING); 
  dbus_g_object_register_marshaller(browser_service_marshal_VOID__INT64, G_TYPE_NONE, G_TYPE_INT64);
  dbus_g_object_register_marshaller(g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, G_TYPE_STRING);

  dbus_g_connection_register_g_object(bus, "/com/meego/browser/BrowserService", G_OBJECT(m_browserService));

  return;
}

static void UpdateAlreadyOpenedTab(BrowserServiceWrapper* wrapper)
{
  if(wrapper)
    wrapper->AddOpenedTab();
}

MeeGoPluginAPI::MeeGoPluginAPI(BrowserServiceWrapper* wrapper):
    wrapper_(wrapper),
    m_panel_db(NULL),
    m_browser_closing(false)
{
  init_db();
  init_browser_service();
  g_signal_emit_by_name(m_browserService, "browser_launched");

  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE, NewRunnableFunction(
        UpdateAlreadyOpenedTab, wrapper_));
}

MeeGoPluginAPI::~MeeGoPluginAPI()
{
  DBG("~MeeGoPluginAPI");

  browser_service_destroy(m_browserService);

  sqlite3_close(m_panel_db);
  g_free(m_db_dirname);
  m_panel_db = NULL;
}

void MeeGoPluginAPI::init_db()
{
  int rc;
  gchar* db_path = NULL;

  DBG("init panel database()");

  db_path = g_build_filename(g_get_home_dir(),
                             ".config", 
                             NETPANEL_DIRNAME,
                             "chromium.db",
                             NULL);
  m_db_dirname = g_path_get_dirname(db_path);
  if(!g_file_test(m_db_dirname, G_FILE_TEST_EXISTS))
  {
    // make the directory with 755
    g_mkdir_with_parents(m_db_dirname, DEFAULT_MODE);
  }

  // mkdir thumbnails
  char* thumbnail_dirname = g_build_filename(m_db_dirname, "thumbnails", NULL);
  if(!g_file_test(thumbnail_dirname, G_FILE_TEST_EXISTS))
  {
    g_mkdir(thumbnail_dirname, DEFAULT_MODE); 
  }
  g_free(thumbnail_dirname);

  //mkdir favicons
  char* favicon_dirname    = g_build_filename(m_db_dirname, "favicons", NULL);
  if(!g_file_test(favicon_dirname, G_FILE_TEST_EXISTS))
  {
    g_mkdir(favicon_dirname, DEFAULT_MODE);
  }
  g_free(favicon_dirname);

  rc = sqlite3_open(db_path, &m_panel_db);
  if(rc == SQLITE_OK)
  {
    sqlite3_exec(m_panel_db, CREATE_URL_TABLE_STMT, NULL, NULL, NULL);
    sqlite3_exec(m_panel_db, "CREATE INDEX url ON urls(url)", NULL, NULL, NULL);

    sqlite3_exec(m_panel_db, CREATE_BOOKMARK_TABLE_STMT, NULL, NULL, NULL);
    sqlite3_exec(m_panel_db, "CREATE INDEX url ON bookmarks(url)", NULL, NULL, NULL);

    sqlite3_exec(m_panel_db, CREATE_ICON_TABLE_STMT, NULL, NULL, NULL);
    sqlite3_exec(m_panel_db, "CREATE INDEX url ON favicons(url)", NULL, NULL, NULL);

    sqlite3_exec(m_panel_db, CREATE_TABS_TABLE_STMT, NULL, NULL, NULL);
    sqlite3_exec(m_panel_db, "DELETE FROM current_tabs", NULL, NULL, NULL);
    sqlite3_busy_timeout(m_panel_db, 100);
  }
  else 
  {
    DBG("failed to open %s: %s", db_path, sqlite3_errmsg(m_panel_db));
    sqlite3_close(m_panel_db);
    m_panel_db = NULL;
  }
  g_free(db_path);
}

// Insert a URL item into urls table
void MeeGoPluginAPI::addURLItem(int64 id, std::string url, std::string title, std::string favicon_url, 
                                int visit_count, int typed_count, long long last_visit_time)
{
  if(!m_panel_db) return;
	// Ignore chrome system page and empty url page (new tab), or empty title
	if(url.empty() || title.empty() || url.compare(0, 9,"chrome://") == 0) return;

	const char* insert_sql = 
			"INSERT OR REPLACE INTO urls(id, url, title, visit_count, typed_count, last_visit_time, favicon_url) "
			"VALUES(?, ?, ?, ?, ?, ?, ?)";
	
	sqlite3_stmt* insert_stmt = NULL;
	if(sqlite3_prepare_v2(m_panel_db, insert_sql, -1, &insert_stmt, NULL) != SQLITE_OK)
	{
			g_warning("failed to prepare: %s", insert_sql);
			return;
	}

	do {
			if(sqlite3_bind_int64(insert_stmt, 1, id) != SQLITE_OK) break;
			if(sqlite3_bind_text(insert_stmt, 2, url.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) break;
			if(sqlite3_bind_text(insert_stmt, 3, title.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) break;
			if(sqlite3_bind_int(insert_stmt, 4, visit_count) != SQLITE_OK) break;
			if(sqlite3_bind_int(insert_stmt, 5, typed_count) != SQLITE_OK) break;
			if(sqlite3_bind_int64(insert_stmt, 6, last_visit_time) != SQLITE_OK) break;
			if(sqlite3_bind_text(insert_stmt, 7, favicon_url.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) break;

			if(sqlite3_step(insert_stmt) != SQLITE_DONE) break;
  		g_signal_emit_by_name(m_browserService, "url_visited", id, url.c_str(), title.c_str(), favicon_url.c_str());
			
	} while(0);

	if(insert_stmt) sqlite3_finalize(insert_stmt);

  return;
}

void MeeGoPluginAPI::clearAllURLs()
{
  char* errmsg = NULL;
  if(!m_panel_db) return;
	
  DBG("MeeGoPluginAPI::clearAllURLs");
  // Remove thumbnails
  char* thumbnail_dirname = g_build_filename(m_db_dirname, "thumbnails", NULL);

  sqlite3_stmt* stmt = NULL;
  const char* url = NULL;
  char* filename = NULL;
  sqlite3_prepare_v2(m_panel_db, "SELECT url from urls", -1, &stmt, NULL);
  while(sqlite3_step(stmt) == SQLITE_ROW)
  {
    url = (const char*)sqlite3_column_text(stmt, 0);
    char* csum = g_compute_checksum_for_string(G_CHECKSUM_MD5, url, -1);
    char* filename = g_strconcat(csum, ".jpg", NULL);
    char* fullname = g_build_filename(thumbnail_dirname, filename, NULL);
    g_remove(fullname);
    g_free(csum);
    g_free(filename);
    g_free(fullname);
  }
  sqlite3_finalize(stmt);
  g_free(thumbnail_dirname);

  const char* sql = "DELETE FROM urls";

  if(sqlite3_exec(m_panel_db, sql, NULL, NULL, &errmsg) != SQLITE_OK)
  {
    g_warning("failed to exec %s: %s", sql, errmsg);
    g_free(errmsg);
  }
}

// Insert a bookmark item into bookmarks table
void MeeGoPluginAPI::addBookmarkItem(int64 id, std::string url, 
                                     std::string title, std::string faviconUrl, 
                                     long long dateAdded)
{
  if(!m_panel_db) return;

  //long long id = g_ascii_strtoll(sid.c_str(), NULL, 10);

	const char* insert_sql = 
      "INSERT OR REPLACE INTO bookmarks(id, url, title, dateAdded, favicon_url) "
      "VALUES(?, ?, ?, ?, ?)"; 

	sqlite3_stmt* stmt = NULL;

	if(sqlite3_prepare_v2(m_panel_db, insert_sql, -1, &stmt, NULL) != SQLITE_OK) 
	{
			g_warning("failed to prepare: %s", insert_sql);
			return;
	}

	do {
			if(sqlite3_bind_int64(stmt, 1, id) != SQLITE_OK) break;
			if(sqlite3_bind_text(stmt, 2, url.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) break;
			if(sqlite3_bind_text(stmt, 3, title.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) break;
			if(sqlite3_bind_int64(stmt, 4, dateAdded) != SQLITE_OK) break;
			if(sqlite3_bind_text(stmt, 5, faviconUrl.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) break;

			if(sqlite3_step(stmt)!=SQLITE_DONE) break;
			// emite dbus signal
			g_signal_emit_by_name(m_browserService, "bookmark_updated", 
							id, url.c_str(), title.c_str(), faviconUrl.c_str());

	} while(0);

	if(stmt) sqlite3_finalize(stmt);
}

void MeeGoPluginAPI::addFavIconItem(std::string url, long long last_update, const unsigned char *icon_data, size_t len)
{
  if(!m_panel_db) return;
	if (icon_data == NULL || len == 0) return;

	const char *select_sql, *update_sql, *insert_sql;

	sqlite3_stmt *select_stmt;
	sqlite3_stmt *update_stmt;
	sqlite3_stmt *insert_stmt;

	bool done = false;
	select_stmt = update_stmt = insert_stmt = NULL;
	do {
			// Check whether the favicon for the given url exists in database
			// If the favicon is already existing, execute update command to update the icon data and 
			// update time. Otherwise, execute insert comand to insert a new record
			select_sql = "SELECT id FROM favicons WHERE url=?";
			if(sqlite3_prepare_v2(m_panel_db, select_sql, -1, &select_stmt, NULL) != SQLITE_OK) break;
			if(sqlite3_bind_text(select_stmt, 1, url.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) break;

			bool exists = false;
			if(sqlite3_step(select_stmt) == SQLITE_ROW) { //Already exists
					exists = true;
			} 

			// Execute update command
			if(exists) {
					update_sql = "UPDATE favicons SET last_update_time=? WHERE url=?";
					if(sqlite3_prepare_v2(m_panel_db, update_sql, -1, &update_stmt, NULL) != SQLITE_OK) break;
					if(sqlite3_bind_int64(update_stmt, 1, last_update) != SQLITE_OK) break;
					if(sqlite3_bind_text(update_stmt, 2, url.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) break;
					if(sqlite3_step(update_stmt) != SQLITE_DONE) break;
			} else {
					insert_sql = "INSERT INTO favicons(url, last_update_time) VALUES(?, ?)";
					if(sqlite3_prepare_v2(m_panel_db, insert_sql, -1, &insert_stmt, NULL) != SQLITE_OK) break;
					if(sqlite3_bind_int64(insert_stmt, 2, last_update) != SQLITE_OK) break;
					if(sqlite3_bind_text(insert_stmt, 1, url.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) break;
					if(sqlite3_step(insert_stmt) != SQLITE_DONE) break;
			}
			// Successful to add a new record
			done = true;

	} while(0);

	// Finalize prepared statement
	if(select_stmt) sqlite3_finalize(select_stmt);
	if(update_stmt) sqlite3_finalize(update_stmt);
	if(insert_stmt) sqlite3_finalize(insert_stmt);

	// Save the icon data into local file system
	if(done) {
			// Build icon filename from url
			char* csum = g_compute_checksum_for_string (G_CHECKSUM_MD5, url.c_str(), -1);
			char* filename = g_strconcat(csum, ".png", NULL);

			// Save the icon data into local file
			GError* error = NULL;
			char* icon_filename = NULL;
			icon_filename = g_build_filename(m_db_dirname, "favicons", filename, NULL);
			if(g_file_set_contents(icon_filename, reinterpret_cast<const gchar*>(icon_data), len, &error))
			{
					g_signal_emit_by_name(m_browserService, "favicon_updated",url.c_str());
			}

			if(error) {
					g_warning("failed to g_file_set_contents: %s", error->message);
					g_error_free(error);
			}
	}

	return;
}

void MeeGoPluginAPI::updateURLInfo(std::string url, std::string title, std::string favicon_url)
{
  if(!m_panel_db) return;

	const char* sql = "UPDATE urls SET title=?, favicon_url=? WHERE url=?";

	sqlite3_stmt* stmt = NULL;

	if(sqlite3_prepare_v2(m_panel_db, sql, -1, &stmt, NULL) != SQLITE_OK)
	{
			g_warning("failed to prepare %s", sql);
			return;
	}

	do {
			if(sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) break;
			if(sqlite3_bind_text(stmt, 2, favicon_url.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) break;
			if(sqlite3_bind_text(stmt, 3, url.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) break;
			if(sqlite3_step(stmt) != SQLITE_DONE) break;
	} while(0);

	if(stmt) sqlite3_finalize(stmt);

  return;
}
 
#define DATA_URL_PREFIX_LENGTH 22
// save thumbnail to a local file
void MeeGoPluginAPI::addThumbnailItem(int tab_id, std::string url, long long last_update, const unsigned char* blob, size_t len)
{
  if(!m_panel_db) return;

  // Build filename for thumbnail
  char* csum = g_compute_checksum_for_string (G_CHECKSUM_MD5, url.c_str(), -1);
  char* filename = g_strconcat(csum, ".jpg", NULL);

  GError* error = NULL;
  char* thumbnail_filename = g_build_filename(m_db_dirname, "thumbnails", filename, NULL);

  // Save thumbnail into a file
  FILE* fp = NULL;
  gsize nwritten;

  if(fp = fopen(thumbnail_filename, "wb"))
  {
    nwritten = fwrite(blob, 1, len, fp);
    if(nwritten < len)
    {
      g_warning("failed to write %s", thumbnail_filename);
    }
    fflush(fp);
  }
  else
  {
    g_warning("failed to open %s", thumbnail_filename);
  }

  g_free(thumbnail_filename);

  //cleanup
  g_free(csum);
  g_free(filename);
  if(fp) fclose(fp);

  g_signal_emit_by_name(m_browserService, "thumbnail_updated", url.c_str());
  g_signal_emit_by_name(m_browserService, "tab_info_updated", tab_id);

  return;
}

// Insert a tab item FB:varianto current_tabs table
void MeeGoPluginAPI::addTabItem(int tab_id, int win_id, std::string url, 
                                std::string title, std::string faviconUrl)
{
  bool success = false;
	if(!m_panel_db) return;

  char* errmsg = NULL;
  if(sqlite3_exec(m_panel_db, "BEGIN IMMEDIATE TRANSACTION", 0, 0, NULL) != SQLITE_OK)
  {
    g_warning("failed to start a sqlite3 transaction: %s", errmsg);
    g_free(errmsg);
    return;
  }
  
  // Update tab_id field according to the tab insertion mechanism used in chromium
  // New tab will inserted adjacent to the tab that opened the new one
  char* update_sql = g_strdup_printf("UPDATE current_tabs SET tab_id=tab_id+1 WHERE tab_id>=%d", tab_id);
  sqlite3_stmt *select_stmt = NULL;
  if(sqlite3_exec(m_panel_db, update_sql, 0,0,&errmsg) == SQLITE_OK) 
  {
    const char* sql =
      "INSERT INTO current_tabs(tab_id, win_id, url, title, favicon_url) "
      "VALUES(?, ?, ?, ?, ?)";

    if(sqlite3_prepare_v2(m_panel_db, sql, -1, &select_stmt, NULL) == SQLITE_OK
        && sqlite3_bind_int(select_stmt, 1, tab_id) == SQLITE_OK 
        && sqlite3_bind_int(select_stmt, 2, win_id) == SQLITE_OK
        && sqlite3_bind_text(select_stmt,3, url.c_str(), -1, SQLITE_STATIC) == SQLITE_OK
        && sqlite3_bind_text(select_stmt, 4, title.c_str(), -1, SQLITE_STATIC) == SQLITE_OK
        && sqlite3_bind_text(select_stmt, 5, faviconUrl.c_str(), -1, SQLITE_STATIC) == SQLITE_OK
        && sqlite3_step(select_stmt) == SQLITE_DONE) {
        success = true;
      } else {
        success = false;
      }
  }

  if(success) {
      sqlite3_exec(m_panel_db, "COMMIT TRANSACTION", 0, 0, NULL);
      g_signal_emit_by_name(m_browserService, "tab_list_updated");
  } else {
    sqlite3_exec(m_panel_db, "ROLLBACK TRANSACTION", 0, 0, NULL);
  }

	if(select_stmt) sqlite3_finalize(select_stmt);
  if(errmsg) g_free(errmsg);
  g_free(update_sql);
 
  return;
}

void MeeGoPluginAPI::updateTabItem(int tab_id, int win_id, std::string url, std::string title, std::string faviconUrl)
{
  bool updated = false;
  if(!m_panel_db) return;

	const char* sql = "UPDATE current_tabs SET win_id=?, url=?, title=?, favicon_url=? WHERE tab_id=?";

	sqlite3_stmt* stmt = NULL;

	if(sqlite3_prepare_v2(m_panel_db, sql, -1, &stmt, NULL) != SQLITE_OK)
	{
			g_warning("failed to prepare %s", sql);
			return;
	}

	do {
			if(sqlite3_bind_int(stmt, 1, win_id) != SQLITE_OK) break;
			if(sqlite3_bind_text(stmt, 2, url.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) break;
			if(sqlite3_bind_text(stmt, 3, title.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) break;
      if(sqlite3_bind_text(stmt, 4, faviconUrl.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) break;
      if(sqlite3_bind_int(stmt, 5, tab_id) != SQLITE_OK) break;
			if(sqlite3_step(stmt) != SQLITE_DONE) break;
      updated = true;
	} while(0);

	if(stmt) sqlite3_finalize(stmt);

  if(updated)
    g_signal_emit_by_name(m_browserService, "tab_info_updated", tab_id);
}

void MeeGoPluginAPI::updateWindowId(int tab_id, int newwin_id)
{
  int rc;
	
  if(!m_panel_db) return;

  char* sql = g_strdup_printf("UPDATE current_tabs SET win_id=%d WHERE tab_id=%d", newwin_id, tab_id);
  char* errmsg = NULL;
  rc = sqlite3_exec(m_panel_db, sql, NULL, NULL, &errmsg);
  if(rc != SQLITE_OK)
  {
    g_warning("failed to exec '%s': %s", sql, errmsg);
    g_free(errmsg);
  }
  g_free(sql);
}

void MeeGoPluginAPI::clearAllTabItems()
{
  int rc;
  sqlite3_stmt* stmt;
	
  if(!m_panel_db) return;

  DBG("clearAllTabItems ...");

  char* errmsg = NULL;
  rc = sqlite3_exec(m_panel_db, "DELETE FROM current_tabs", NULL, NULL, &errmsg);
  if(rc != SQLITE_OK)
  {
    g_warning("failed to clear tabs: %s", errmsg);
    g_free(errmsg);
  }
  return;
}

// Remove a tab item
void MeeGoPluginAPI::removeTabItem(int tab_id)
{
  int rc;
  bool success = false;
  char* errmsg = NULL;
  if(!m_panel_db || m_browser_closing) return;

  DBG("removeTabItem: tab id = %d", tab_id);

  rc = sqlite3_exec(m_panel_db, "BEGIN IMMEDIATE TRANSACTION", 0, 0, &errmsg);
  if(rc != SQLITE_OK)
  {
    g_warning("failed to begin transaction: %s", errmsg);
    g_free(errmsg);
    errmsg = NULL;
    return;
  }

  char* delete_sql = g_strdup_printf("DELETE FROM current_tabs WHERE tab_id=%d", tab_id);
  char* update_sql = NULL;
  rc = sqlite3_exec(m_panel_db, delete_sql, NULL, NULL, &errmsg);
  if(rc != SQLITE_OK)
  {
    g_warning("Error: failed to exec : %s", errmsg);
    g_free(errmsg);
  } else {
    update_sql = g_strdup_printf("UPDATE current_tabs SET tab_id=tab_id-1 WHERE tab_id>%d", tab_id);
    if(sqlite3_exec(m_panel_db, update_sql, NULL, NULL, &errmsg) != SQLITE_OK) {
      g_warning("failed to exec %s: %s", update_sql, errmsg);
      g_free(errmsg);
      success = false;
    } else {
      success = true;
    }
  }
  g_free(delete_sql);
  if(update_sql) g_free(update_sql);

  if(success) {
    sqlite3_exec(m_panel_db, "COMMIT TRANSACTION", NULL, NULL, NULL);
    g_signal_emit_by_name(m_browserService, "tab_list_updated");
  } else {
    sqlite3_exec(m_panel_db, "ROLLBACK TRANACTION", NULL, NULL, NULL);
  }

  return;
}

// Remove a URL item
void MeeGoPluginAPI::removeURLItem(std::string url)
{
		if(!m_panel_db) return;

		DBG("removeURLItem: %s", url.c_str());

		// Delete the item from urls
		const char* delete_sql = "DELETE FROM urls WHERE id=?";
		sqlite3_stmt* stmt = NULL;

		if(sqlite3_prepare_v2(m_panel_db, delete_sql, -1, &stmt, NULL) != SQLITE_OK) {
				g_warning("failed to prepare: %s", delete_sql);
				return;
		}

		do {
				if(sqlite3_bind_text(stmt, 1, url.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) break;
				if(sqlite3_step(stmt) != SQLITE_DONE) break; 

				// Remove thumbnail file
				char* csum = g_compute_checksum_for_string (G_CHECKSUM_MD5, url.c_str(), -1);
				char* filename = g_strconcat(csum, ".jpg", NULL);
				char* thumbnail_filename = g_build_filename(m_db_dirname, "thumbnails", filename, NULL);

				g_remove(thumbnail_filename);

				g_free(csum);
				g_free(filename);
				g_free(thumbnail_filename);

				g_signal_emit_by_name(m_browserService, "url_removed", url.c_str());

		}while(0);

		if(stmt) sqlite3_finalize(stmt);
		return;
}

// Remove a bookmark item
void MeeGoPluginAPI::removeBookmarkItem(int64 id)
{
  int rc;
  char* errmsg = NULL;

  if(!m_panel_db) return;

  //long long id = g_ascii_strtoll(sid.c_str(), NULL, 10);
  
	const char* delete_sql = "DELETE FROM bookmarks WHERE id=?";

	sqlite3_stmt* stmt = NULL;
	if(sqlite3_prepare_v2(m_panel_db, delete_sql, -1, &stmt, NULL) != SQLITE_OK)
	{
			g_warning("failed to prepare: %s", delete_sql);
			return;
	}

	if(sqlite3_bind_int64(stmt, 1, id) == SQLITE_OK)
	{
		sqlite3_step(stmt);
  	g_signal_emit_by_name(m_browserService, "bookmark_removed", id);
	}

	if(stmt) sqlite3_finalize(stmt);
}

void MeeGoPluginAPI::updateCurrentTab()
{
    wrapper_->updateCurrentTab();
}

void MeeGoPluginAPI::showBrowser(const char *mode, const char *target)
{
    wrapper_->showBrowser(mode, target);
}

void MeeGoPluginAPI::closeTab(int index)
{
    wrapper_->closeTab(index);
}

int MeeGoPluginAPI::getCurrentTabIndex()
{
    return wrapper_->getCurrentTabIndex();
}

void MeeGoPluginAPI::emitBrowserCloseSignal()
{
    g_signal_emit_by_name(m_browserService, "browser_closed");  
    m_browser_closing = true;
}

