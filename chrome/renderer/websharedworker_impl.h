// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_WEBSHAREDWORKER_IMPL_H_
#define CHROME_RENDERER_WEBSHAREDWORKER_IMPL_H_

#include "base/basictypes.h"
#include "chrome/renderer/webworker_base.h"
#include "googleurl/src/gurl.h"
#include "webkit/api/public/WebSharedWorker.h"

class ChildThread;

// Implementation of the WebSharedWorker APIs. This object is intended to only
// live long enough to allow the caller to send a "connect" event to the worker
// thread. Once the connect event has been sent, all future communication will
// happen via the WebMessagePortChannel, and the WebSharedWorker instance will
// be freed.
class WebSharedWorkerImpl : public WebKit::WebSharedWorker,
                            private WebWorkerBase {
 public:
  WebSharedWorkerImpl(const GURL& url,
                      const string16& name,
                      ChildThread* child_thread,
                      int route_id,
                      int render_view_route_id);

  // Implementations of WebSharedWorker APIs
  virtual bool isStarted();
  virtual void connect(WebKit::WebMessagePortChannel* channel);
  virtual void startWorkerContext(const WebKit::WebURL& script_url,
                                  const WebKit::WebString& user_agent,
                                  const WebKit::WebString& source_code);

  // IPC::Channel::Listener implementation.
  void OnMessageReceived(const IPC::Message& message);

 private:
  void OnWorkerCreated();

  // The name and URL that uniquely identify this worker.
  GURL url_;
  string16 name_;

  DISALLOW_COPY_AND_ASSIGN(WebSharedWorkerImpl);
};

#endif  // CHROME_RENDERER_WEBSHAREDWORKER_IMPL_H_
