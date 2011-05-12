#include "BrowserService.h"
#include "BrowserService-marshaller.h"
#include "MeeGoPluginAPI.h"

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(BrowserService, browser_service, G_TYPE_OBJECT);

static void browser_service_init(BrowserService* bs)
{
	//g_debug("browser_service_init");
}

static void browser_service_class_init(BrowserServiceClass* klass)
{
	//g_debug("browser_service_class_init");

	signals[URL_VISITED_SIGNAL] =
		g_signal_new("url_visited", 
				G_OBJECT_CLASS_TYPE(klass),
				(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
				0, NULL, NULL,
				browser_service_marshal_VOID__INT64_STRING_STRING_STRING,
				G_TYPE_NONE, 4, G_TYPE_INT64, 
				G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	signals[URL_REMOVED_SIGNAL] =
		g_signal_new("url_removed", 
				G_OBJECT_CLASS_TYPE(klass),
				(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
				0, NULL, NULL,
				g_cclosure_marshal_VOID__STRING,
				G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[BOOKMARK_UPDATED_SIGNAL] =
		g_signal_new("bookmark_updated", 
				G_OBJECT_CLASS_TYPE(klass),
				(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
				0, NULL, NULL,
				browser_service_marshal_VOID__INT64_STRING_STRING_STRING,
				G_TYPE_NONE, 4, G_TYPE_INT64, 
				G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	signals[BOOKMARK_REMOVED_SIGNAL] =
		g_signal_new("bookmark_removed",
				G_OBJECT_CLASS_TYPE(klass),
				(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
				0, NULL, NULL,
				browser_service_marshal_VOID__INT64,
				G_TYPE_NONE, 1, G_TYPE_INT64);

	signals[FAVICON_UPDATED_SIGNAL] =
		g_signal_new("favicon_updated", 
				G_OBJECT_CLASS_TYPE(klass),
				(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
				0, NULL, NULL,
				g_cclosure_marshal_VOID__STRING,
				G_TYPE_NONE, 1, G_TYPE_STRING);
	signals[THUMBNAIL_UPDATED_SIGNAL] =
		g_signal_new("thumbnail_updated",
				G_OBJECT_CLASS_TYPE(klass),
				(GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
				0, NULL, NULL,
				g_cclosure_marshal_VOID__STRING,
				G_TYPE_NONE, 1, G_TYPE_STRING);
  
  signals[TAB_INFO_UPDATED_SIGNAL] =
    g_signal_new("tab_info_updated",
        G_OBJECT_CLASS_TYPE(klass),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
        0, NULL, NULL,
        g_cclosure_marshal_VOID__INT,
        G_TYPE_NONE, 1, G_TYPE_INT);

  signals[TAB_LIST_UPDATED_SIGNAL] =
    g_signal_new("tab_list_updated",
        G_OBJECT_CLASS_TYPE(klass),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
        0, NULL, NULL,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

  signals[BROWSER_LAUNCHED_SIGNAL] =
    g_signal_new("browser_launched",
        G_OBJECT_CLASS_TYPE(klass),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
        0, NULL, NULL,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);

  signals[BROWSER_CLOSED_SIGNAL] =
    g_signal_new("browser_closed",
        G_OBJECT_CLASS_TYPE(klass),
        (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED),
        0, NULL, NULL,
        g_cclosure_marshal_VOID__VOID,
        G_TYPE_NONE, 0);
}

BrowserService* browser_service_new(gpointer data)
{
	BrowserService* obj = (BrowserService*)g_object_new(BROWSER_SERVICE_TYPE, NULL);
	obj->provider = data;

	return obj;
}

void browser_service_destroy(BrowserService* bs)
{
	if(bs) g_object_unref(bs);
}

//
// Browser service DBUS API implementation
//
gboolean browser_service_view_item(BrowserService* self, const char* url)
{

  DBG("browser_service_view_item: %s", url);

  MeeGoPluginAPI* plugin = static_cast<MeeGoPluginAPI*>(self->provider);

  if(plugin) plugin->openWebPage(url);

  return TRUE;
}


gboolean browser_service_remove_bookmark(BrowserService* self, const char* id)
{
  DBG("browser_service_remove_bookmark");

  MeeGoPluginAPI* plugin = static_cast<MeeGoPluginAPI*>(self->provider);

        if(plugin) plugin->removeBookmarkByExtension(id);

        return TRUE;
}

gboolean browser_service_remove_url(BrowserService* self, const char* url)
{
  DBG("browser_service_remove_url");
  MeeGoPluginAPI* plugin = static_cast<MeeGoPluginAPI*>(self->provider);
  if(plugin) plugin->removeUrlByExtension(url);

  return TRUE;
}

gboolean browser_service_update_current_tab(BrowserService * self, GError **error)
{
    DBG("browser_service_close_tab");
    MeeGoPluginAPI* plugin = static_cast<MeeGoPluginAPI*>(self->provider);
    if(plugin)
        plugin->updateCurrentTab();
    return TRUE;
}

gboolean browser_service_show_browser(BrowserService * self, const char * mode, const char * target)
{
    DBG("browser_service_close_tab");
    MeeGoPluginAPI* plugin = static_cast<MeeGoPluginAPI*>(self->provider);
    if(plugin)
        plugin->showBrowser(mode, target);
    return TRUE;
}

gboolean browser_service_close_tab(BrowserService * self, int index)
{
    DBG("browser_service_close_tab");
    MeeGoPluginAPI* plugin = static_cast<MeeGoPluginAPI*>(self->provider);
    if(plugin)
        plugin->closeTab(index);
    return TRUE;
}

gboolean browser_service_get_current_tab_index(BrowserService *self, int * index)
{
    DBG("browser_service_close_tab");
    MeeGoPluginAPI* plugin = static_cast<MeeGoPluginAPI*>(self->provider);
    if(plugin)
        *index = plugin->getCurrentTabIndex();
    return TRUE;
}


