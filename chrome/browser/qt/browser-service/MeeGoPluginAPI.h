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
#ifndef H_MeeGoPluginAPI
#define H_MeeGoPluginAPI

#include "base/basictypes.h"
#include "third_party/sqlite/sqlite3.h"
#include <string>

#include "BrowserService.h"

#ifdef EXTENSION_DEBUG
#include <stdarg.h>
#define DBG(...) g_log(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, __VA_ARGS__)
#else
#define DBG(...)
#endif

class BrowserServiceWrapper;

class MeeGoPluginAPI
{
public:
    MeeGoPluginAPI(BrowserServiceWrapper* wrapper);
    virtual ~MeeGoPluginAPI();

    // Insert a URL item into urls table
    void addURLItem(int64 id, std::string url, std::string title, std::string favicon_url, 
                    int visit_count, int typed_count, long long last_visit_time);

    // Clear URL table
    void clearAllURLs();

    void updateURLInfo(std::string url, std::string title, std::string favicon_url);
    
    // Remove a URL item
    void removeURLItem(std::string url);

    // Insert a bookmark item into bookmarks table
    void addBookmarkItem(int64 id, std::string url, std::string title, std::string faviconUrl, long long dataAdded);

    // Remove a bookmark item
    void removeBookmarkItem(int64 id);

    // Insert a favorite icon item into favicons table
    void addFavIconItem(std::string url, long long last_update, const unsigned char* data, size_t len );

    // Insert a thumbnail for a URL into thumbnails table
    void addThumbnailItem(int tab_id, std::string url, long long last_update, const unsigned char* data, size_t len);

    // Insert a tab item into current_tabs table
    void addTabItem(int tab_id, int win_id, std::string url, std::string title, std::string faviconUrl);

    // Clear all tabs
    void clearAllTabItems();

    // Update the window ID for tab if it is attached to a new window
    void updateWindowId(int tab_id, int win_id);

    // Update Tab item info.
    void updateTabItem(int tab_id, int win_id, std::string url, std::string title, std::string faviconUrl);

    //Remove a tab item
    void removeTabItem(int tab_id);

    // Open a web page to load the given url
    // If the url is already opened in a tab, then then tab will be focused
    // instead of creating a new tab to load the same url
    void openWebPage(const char* url);

    // Call extension API to remove a bookmark in browser
    gboolean removeBookmarkByExtension(const char* id);

    // Call extension API to remove a history URL in browser
    gboolean removeUrlByExtension(const char* url);

    // TabManager API
    void updateCurrentTab();
    void showBrowser(const char * mode, const char *target);
    void closeTab(int index);
    int getCurrentTabIndex();
    void emitBrowserCloseSignal();

private:
    void init_db();
    void init_browser_service();

    // Remove thumbnail and favicon from database for internet panel
    void removeThumbnailAndFavicon(const char* url);
    // Clean up obsolete history items in database
    void cleanupObsoleteHistoryItem(int count);

    BrowserService* m_browserService;
    sqlite3* m_panel_db;
    char*    m_db_dirname;
    char*    m_thumbnail_dirname;
    char*    m_favicon_dirname;
    bool     m_browser_closing;

    BrowserServiceWrapper* wrapper_;
};

#endif // H_MeeGoPluginAPI
