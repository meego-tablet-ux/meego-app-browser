// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_WEBSHAREDWORKER_PROXY_H_
#define CHROME_RENDERER_WEBSHAREDWORKER_PROXY_H_

#include "base/basictypes.h"
#include "chrome/renderer/webworker_base.h"
#include "googleurl/src/gurl.h"
#include "third_party/WebKit/WebKit/chromium/public/WebSharedWorker.h"

class ChildThread;

// Implementation of the WebSharedWorker APIs. This object is intended to only
// live long enough to allow the caller to send a "connect" event to the worker
// thread. Once the connect event has been sent, all future communication will
// happen via the WebMessagePortChannel, and the WebSharedWorker instance will
// be freed.
class WebSharedWorkerProxy : public WebKit::WebSharedWorker,
                             private WebWorkerBase {
 public:
  // If the worker not loaded yet, route_id == MSG_ROUTING_NONE
  WebSharedWorkerProxy(ChildThread* child_thread,
                       unsigned long long document_id,
                       int route_id,
                       int render_view_route_id);

  // Implementations of WebSharedWorker APIs
  virtual bool isStarted();
  virtual void connect(WebKit::WebMessagePortChannel* channel,
                       ConnectListener* listener);
  virtual void startWorkerContext(const WebKit::WebURL& script_url,
                                  const WebKit::WebString& name,
                                  const WebKit::WebString& user_agent,
                                  const WebKit::WebString& source_code);
  virtual void terminateWorkerContext();
  virtual void clientDestroyed();

  // IPC::Channel::Listener implementation.
  void OnMessageReceived(const IPC::Message& message);

 private:
  void OnWorkerCreated();

  ConnectListener* connect_listener_;

  DISALLOW_COPY_AND_ASSIGN(WebSharedWorkerProxy);
};

#endif  // CHROME_RENDERER_WEBSHAREDWORKER_PROXY_H_
