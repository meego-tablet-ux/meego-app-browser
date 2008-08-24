// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_NETWORK_SESSION_H_
#define NET_HTTP_HTTP_NETWORK_SESSION_H_

#include "base/ref_counted.h"
#include "net/base/auth_cache.h"
#include "net/base/client_socket_pool.h"
#include "net/proxy/proxy_service.h"

namespace net {

// This class holds session objects used by HttpNetworkTransaction objects.
class HttpNetworkSession : public base::RefCounted<HttpNetworkSession> {
 public:
  // Allow up to 6 connections per host.
  enum {
    MAX_SOCKETS_PER_GROUP = 6
  };
  
  HttpNetworkSession(ProxyResolver* proxy_resolver)
      : connection_pool_(new ClientSocketPool(MAX_SOCKETS_PER_GROUP)),
        proxy_resolver_(proxy_resolver),
        proxy_service_(proxy_resolver) {
  }

  AuthCache* auth_cache() { return &auth_cache_; }
  ClientSocketPool* connection_pool() { return connection_pool_; }
  ProxyService* proxy_service() { return &proxy_service_; }

 private:
  AuthCache auth_cache_;
  scoped_refptr<ClientSocketPool> connection_pool_;
  scoped_ptr<ProxyResolver> proxy_resolver_;
  ProxyService proxy_service_;
};

}  // namespace net

#endif  // NET_HTTP_HTTP_NETWORK_SESSION_H_

