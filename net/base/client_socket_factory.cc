// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/client_socket_factory.h"

#include "base/singleton.h"
#include "build/build_config.h"
#if defined(OS_WIN)
#include "net/base/ssl_client_socket_win.h"
#endif
#include "net/base/tcp_client_socket.h"

namespace net {

class DefaultClientSocketFactory : public ClientSocketFactory {
 public:
  virtual ClientSocket* CreateTCPClientSocket(
      const AddressList& addresses) {
    return new TCPClientSocket(addresses);
  }

  virtual SSLClientSocket* CreateSSLClientSocket(
      ClientSocket* transport_socket,
      const std::string& hostname,
      const SSLConfig& ssl_config) {
#if defined(OS_WIN)
    return new SSLClientSocketWin(transport_socket, hostname, ssl_config);
#else
    // TODO(pinkerton): turn on when we port SSL socket from win32
    NOTIMPLEMENTED();
    return NULL;
#endif
  }
};

// static
ClientSocketFactory* ClientSocketFactory::GetDefaultFactory() {
  return Singleton<DefaultClientSocketFactory>::get();
}

}  // namespace net

