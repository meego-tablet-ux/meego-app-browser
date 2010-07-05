// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SERVER_HTTP_LISTEN_SOCKET_H_
#define NET_SERVER_HTTP_LISTEN_SOCKET_H_

#include "net/base/listen_socket.h"

class HttpServerRequestInfo;

// Implements a simple HTTP listen socket on top of the raw socket interface.
class HttpListenSocket : public ListenSocket,
                         public ListenSocket::ListenSocketDelegate {
 public:
  class Delegate {
   public:
    virtual void OnHttpRequest(HttpListenSocket* socket,
                               HttpServerRequestInfo* info) = 0;

    virtual void OnWebSocketRequest(HttpListenSocket* socket,
                                    HttpServerRequestInfo* info) = 0;

    virtual void OnWebSocketMessage(HttpListenSocket* socket,
                                    const std::string& data) = 0;

    virtual void OnClose(HttpListenSocket* socket) = 0;
   protected:
    virtual ~Delegate() {}
  };

  static HttpListenSocket* Listen(const std::string& ip,
                                  int port,
                                  HttpListenSocket::Delegate* delegate);

  void AcceptWebSocket(HttpServerRequestInfo* request);

  void SendOverWebSocket(const std::string& data);

  void Listen() { ListenSocket::Listen(); }
  virtual void Accept();

  // ListenSocketDelegate
  virtual void DidAccept(ListenSocket* server, ListenSocket* connection);
  virtual void DidRead(ListenSocket* connection, const char* data, int len);
  virtual void DidClose(ListenSocket* sock);

 private:
  static const int kReadBufSize = 16 * 1024;
  HttpListenSocket(SOCKET s, HttpListenSocket::Delegate* del);
  virtual ~HttpListenSocket();

  // Expects the raw data to be stored in recv_data_. If parsing is successful,
  // will remove the data parsed from recv_data_, leaving only the unused
  // recv data.
  HttpServerRequestInfo* ParseHeaders();

  HttpListenSocket::Delegate* delegate_;
  bool is_web_socket_;
  std::string recv_data_;

  DISALLOW_COPY_AND_ASSIGN(HttpListenSocket);
};

#endif // NET_SERVER_HTTP_LISTEN_SOCKET_H_
