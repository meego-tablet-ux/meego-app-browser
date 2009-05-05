// Copyright (c) 2009 The Chromium Authors. All rights reserved.  Use of this
// source code is governed by a BSD-style license that can be found in the
// LICENSE file.

#include "chrome/worker/worker_webkitclient_impl.h"

#include "base/logging.h"
#include "chrome/renderer/webworker_proxy.h"
#include "chrome/worker/worker_thread.h"
#include "third_party/WebKit/WebKit/chromium/public/WebString.h"
#include "third_party/WebKit/WebKit/chromium/public/WebURL.h"

WebKit::WebClipboard* WorkerWebKitClientImpl::clipboard() {
  NOTREACHED();
  return NULL;
}

WebKit::WebMimeRegistry* WorkerWebKitClientImpl::mimeRegistry() {
  NOTREACHED();
  return NULL;
}

WebKit::WebSandboxSupport* WorkerWebKitClientImpl::sandboxSupport() {
  NOTREACHED();
  return NULL;
}

unsigned long long WorkerWebKitClientImpl::visitedLinkHash(
    const char* canonical_url,
    size_t length) {
  NOTREACHED();
  return 0;
}

bool WorkerWebKitClientImpl::isLinkVisited(unsigned long long link_hash) {
  NOTREACHED();
  return false;
}

void WorkerWebKitClientImpl::setCookies(const WebKit::WebURL& url,
                                        const WebKit::WebURL& policy_url,
                                        const WebKit::WebString& value) {
  NOTREACHED();
}

WebKit::WebString WorkerWebKitClientImpl::cookies(
    const WebKit::WebURL& url, const WebKit::WebURL& policy_url) {
  NOTREACHED();
  return WebKit::WebString();
}

void WorkerWebKitClientImpl::prefetchHostName(const WebKit::WebString&) {
  NOTREACHED();
}

WebKit::WebString WorkerWebKitClientImpl::defaultLocale() {
  NOTREACHED();
  return WebKit::WebString();
}

WebKit::WebWorker* WorkerWebKitClientImpl::createWorker(
    WebKit::WebWorkerClient* client) {
  return new WebWorkerProxy(client, WorkerThread::current(), 0);
}
