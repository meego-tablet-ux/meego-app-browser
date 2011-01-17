// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_TOOLS_TEST_SHELL_TEST_SHELL_WEBBLOBREGISTRY_IMPL_H_
#define WEBKIT_TOOLS_TEST_SHELL_TEST_SHELL_WEBBLOBREGISTRY_IMPL_H_

#include "base/ref_counted.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebBlobRegistry.h"

class GURL;

namespace webkit_blob {
class BlobData;
class BlobStorageController;
}

class TestShellWebBlobRegistryImpl
    : public WebKit::WebBlobRegistry,
      public base::RefCountedThreadSafe<TestShellWebBlobRegistryImpl> {
 public:
  static void InitializeOnIOThread(
      webkit_blob::BlobStorageController* blob_storage_controller);
  static void Cleanup();

  TestShellWebBlobRegistryImpl();

  // See WebBlobRegistry.h for documentation on these functions.
  virtual void registerBlobURL(const WebKit::WebURL& url,
                               WebKit::WebBlobData& data);
  virtual void registerBlobURL(const WebKit::WebURL& url,
                               const WebKit::WebURL& src_url);
  virtual void unregisterBlobURL(const WebKit::WebURL& url);

  // Run on I/O thread.
  void DoRegisterBlobUrl(const GURL& url, webkit_blob::BlobData* blob_data);
  void DoRegisterBlobUrlFrom(const GURL& url, const GURL& src_url);
  void DoUnregisterBlobUrl(const GURL& url);

 protected:
  friend class base::RefCountedThreadSafe<TestShellWebBlobRegistryImpl>;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestShellWebBlobRegistryImpl);
};

#endif  // WEBKIT_TOOLS_TEST_SHELL_TEST_SHELL_WEBBLOBREGISTRY_IMPL_H_
