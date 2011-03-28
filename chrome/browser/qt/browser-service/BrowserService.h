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
#ifndef BROWSER_SERVICE_H
#define BROWSER_SERVICE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define BROWSER_SERVICE_TYPE 			(browser_service_get_type())

#define BROWSER_SERVICE(obj) 			(G_TYPE_CHECK_INSTANCE_CAST((obj), \
		                                 BROWSER_SERVICE_TYPE, \
		 							     BrowserService))

#define BROWSER_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), \
			                             BROWSER_SERVICE_TYPE, \
										 BrowserServiceClass))

#define IS_BROWSER_SERVICE_TYPE(obj)    (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
			                             BROWSER_SERVICE_TYPE))

#define IS_BROWSER_SERVICE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),\
			                              BROWSER_SERVICE_TYPE))

enum 
{
	URL_VISITED_SIGNAL = 0,
	URL_REMOVED_SIGNAL,
	BOOKMARK_UPDATED_SIGNAL,
	BOOKMARK_REMOVED_SIGNAL,
	FAVICON_UPDATED_SIGNAL,
	THUMBNAIL_UPDATED_SIGNAL,
	LAST_SIGNAL
};

typedef struct _BrowserService BrowserService;
typedef struct _BrowserServiceClass BrowserServiceClass;
typedef struct _BrowserServicePriv BrowserServicePriv;

struct _BrowserService
{
	GObject parent;
	gpointer provider;
};

struct _BrowserServiceClass
{
	GObjectClass parent;
};

GType browser_service_get_type() G_GNUC_CONST;

BrowserService* browser_service_new(gpointer provider);

void browser_service_destroy(BrowserService* self);

gboolean browser_service_view_item(BrowserService* self, const char* url);

gboolean browser_service_remove_bookmark(BrowserService* self, const char* id);

gboolean browser_service_remove_url(BrowserService* self, const char* url);

G_END_DECLS

#endif
