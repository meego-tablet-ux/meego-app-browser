// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_RENDERER_WEBIDBFACTORY_IMPL_H_
#define CHROME_RENDERER_RENDERER_WEBIDBFACTORY_IMPL_H_
#pragma once

#include "third_party/WebKit/WebKit/chromium/public/WebIDBCallbacks.h"
#include "third_party/WebKit/WebKit/chromium/public/WebIDBFactory.h"
#include "third_party/WebKit/WebKit/chromium/public/WebVector.h"

namespace WebKit {
class WebDOMStringList;
class WebFrame;
class WebIDBDatabase;
class WebSecurityOrigin;
class WebString;
}

class RendererWebIDBFactoryImpl : public WebKit::WebIDBFactory {
 public:
  RendererWebIDBFactoryImpl();
  virtual ~RendererWebIDBFactoryImpl();

  // See WebIDBFactory.h for documentation on these functions.
  virtual void open(
      const WebKit::WebString& name,
      const WebKit::WebString& description,
      WebKit::WebIDBCallbacks* callbacks,
      const WebKit::WebSecurityOrigin& origin,
      WebKit::WebFrame* web_frame,
      const WebKit::WebString& dataDir,
      uint64 maximum_size);
};

#endif  // CHROME_RENDERER_RENDERER_WEBIDBFACTORY_IMPL_H_
