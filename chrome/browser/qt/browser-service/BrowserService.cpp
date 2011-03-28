#include "BrowserService.h"
#include "BrowserService-marshaller.h"

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
