// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_PLUGINS_NPAPI_PLUGINS_TEST_PLUGIN_GET_JAVASCRIPT_URL2_H_
#define WEBKIT_PLUGINS_NPAPI_PLUGINS_TEST_PLUGIN_GET_JAVASCRIPT_URL2_H_

#include "webkit/plugins/npapi/test/plugin_test.h"

namespace NPAPIClient {

// This class tests NPP_GetURLNotify for a javascript URL with _top
// as the target frame.
class ExecuteGetJavascriptUrl2Test : public PluginTest {
 public:
  // Constructor.
  ExecuteGetJavascriptUrl2Test(NPP id, NPNetscapeFuncs *host_functions);

  //
  // NPAPI functions
  //
  virtual NPError SetWindow(NPWindow* pNPWindow);
  virtual NPError NewStream(NPMIMEType type, NPStream* stream,
                            NPBool seekable, uint16* stype);
  virtual int32   WriteReady(NPStream *stream);
  virtual int32   Write(NPStream *stream, int32 offset, int32 len,
                        void *buffer);
  virtual NPError DestroyStream(NPStream *stream, NPError reason);
  virtual void    URLNotify(const char* url, NPReason reason, void* data);

 private:
  bool test_started_;
  std::string self_url_;
};

}  // namespace NPAPIClient

#endif  // WEBKIT_PLUGINS_NPAPI_PLUGINS_TEST_PLUGIN_GET_JAVASCRIPT_URL2_H_
