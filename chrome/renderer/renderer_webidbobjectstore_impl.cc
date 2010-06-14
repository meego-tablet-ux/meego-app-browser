// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/renderer_webidbobjectstore_impl.h"

#include "chrome/common/render_messages.h"
#include "chrome/renderer/indexed_db_dispatcher.h"
#include "chrome/renderer/render_thread.h"
#include "chrome/renderer/renderer_webidbindex_impl.h"
#include "third_party/WebKit/WebKit/chromium/public/WebDOMStringList.h"
#include "third_party/WebKit/WebKit/chromium/public/WebString.h"

using WebKit::WebDOMStringList;
using WebKit::WebFrame;
using WebKit::WebIDBCallbacks;
using WebKit::WebIDBIndex;
using WebKit::WebString;

RendererWebIDBObjectStoreImpl::RendererWebIDBObjectStoreImpl(
    int32 idb_object_store_id)
    : idb_object_store_id_(idb_object_store_id) {
}

RendererWebIDBObjectStoreImpl::~RendererWebIDBObjectStoreImpl() {
  RenderThread::current()->Send(
      new ViewHostMsg_IDBObjectStoreDestroyed(idb_object_store_id_));
}

WebString RendererWebIDBObjectStoreImpl::name() const {
  string16 result;
  RenderThread::current()->Send(
      new ViewHostMsg_IDBObjectStoreName(idb_object_store_id_, &result));
  return result;
}

WebString RendererWebIDBObjectStoreImpl::keyPath() const {
  string16 result;
  RenderThread::current()->Send(
      new ViewHostMsg_IDBObjectStoreKeyPath(idb_object_store_id_, &result));
  return result;
}

WebDOMStringList RendererWebIDBObjectStoreImpl::indexNames() const {
  std::vector<string16> result;
  RenderThread::current()->Send(
      new ViewHostMsg_IDBObjectStoreIndexNames(idb_object_store_id_, &result));
  WebDOMStringList web_result;
  for (std::vector<string16>::const_iterator it = result.begin();
       it != result.end(); ++it) {
    web_result.append(*it);
  }
  return web_result;
}

void RendererWebIDBObjectStoreImpl::createIndex(
    const WebString& name, const WebString& key_path, bool unique,
    WebIDBCallbacks* callbacks) {
  IndexedDBDispatcher* dispatcher =
      RenderThread::current()->indexed_db_dispatcher();
  dispatcher->RequestIDBObjectStoreCreateIndex(
      name, key_path, unique, callbacks, idb_object_store_id_);

}

WebIDBIndex* RendererWebIDBObjectStoreImpl::index(const WebString& name) {
  bool success;
  int32 idb_index_id;
  RenderThread::current()->Send(
      new ViewHostMsg_IDBObjectStoreIndex(idb_object_store_id_, name,
                                          &success, &idb_index_id));
  if (!success)
      return NULL;
  return new RendererWebIDBIndexImpl(idb_index_id);
}

void RendererWebIDBObjectStoreImpl::removeIndex(const WebString& name,
                                                WebIDBCallbacks* callbacks) {
  IndexedDBDispatcher* dispatcher =
      RenderThread::current()->indexed_db_dispatcher();
  dispatcher->RequestIDBObjectStoreRemoveIndex(name, callbacks,
                                               idb_object_store_id_);
}
