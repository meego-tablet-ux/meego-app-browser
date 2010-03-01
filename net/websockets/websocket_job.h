// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_WEBSOCKETS_WEBSOCKET_JOB_H_
#define NET_WEBSOCKETS_WEBSOCKET_JOB_H_

#include <string>
#include <vector>

#include "base/ref_counted.h"
#include "net/base/completion_callback.h"
#include "net/socket_stream/socket_stream_job.h"

class GURL;

namespace net {

// WebSocket protocol specific job on SocketStream.
// It captures WebSocket handshake message and handles cookie operations.
// Chome security policy doesn't allow renderer process (except dev tools)
// see HttpOnly cookies, so it injects cookie header in handshake request and
// strips set-cookie headers in handshake response.
// TODO(ukai): refactor to merge WebSocketThrottle functionality.
// TODO(ukai): refactor websocket.cc to use this.
class WebSocketJob : public SocketStreamJob, public SocketStream::Delegate {
 public:
  // This is state of WebSocket, not SocketStream.
  enum State {
    INITIALIZED = -1,
    CONNECTING = 0,
    OPEN = 1,
    CLOSED = 2,
  };
  static void EnsureInit();

  explicit WebSocketJob(SocketStream::Delegate* delegate);

  virtual void Connect();
  virtual bool SendData(const char* data, int len);
  virtual void Close();
  virtual void RestartWithAuth(
      const std::wstring& username,
      const std::wstring& password);
  virtual void DetachDelegate();

  // SocketStream::Delegate methods.
  virtual void OnConnected(
      SocketStream* socket, int max_pending_send_allowed);
  virtual void OnSentData(
      SocketStream* socket, int amount_sent);
  virtual void OnReceivedData(
      SocketStream* socket, const char* data, int len);
  virtual void OnClose(SocketStream* socket);
  virtual void OnAuthRequired(
      SocketStream* socket, AuthChallengeInfo* auth_info);
  virtual void OnError(
      const SocketStream* socket, int error);

 private:
  friend class WebSocketJobTest;
  virtual ~WebSocketJob();

  bool SendHandshakeRequest(const char* data, int len);
  void AddCookieHeaderAndSend();
  void OnCanGetCookiesCompleted(int policy);

  void OnSentHandshakeRequest(SocketStream* socket, int amount_sent);
  void OnReceivedHandshakeResponse(
      SocketStream* socket, const char* data, int len);
  void SaveCookiesAndNotifyHeaderComplete();
  void SaveNextCookie();
  void OnCanSetCookieCompleted(int policy);

  GURL GetURLForCookies() const;

  SocketStream::Delegate* delegate_;
  State state_;

  std::string original_handshake_request_;
  int original_handshake_request_header_length_;
  std::string handshake_request_;
  size_t handshake_request_sent_;

  std::string handshake_response_;
  int handshake_response_header_length_;
  std::vector<std::string> response_cookies_;
  size_t response_cookies_save_index_;

  CompletionCallbackImpl<WebSocketJob> can_get_cookies_callback_;
  CompletionCallbackImpl<WebSocketJob> can_set_cookie_callback_;

  DISALLOW_COPY_AND_ASSIGN(WebSocketJob);
};

}  // namespace

#endif  // NET_WEBSOCKETS_WEBSOCKET_JOB_H_
