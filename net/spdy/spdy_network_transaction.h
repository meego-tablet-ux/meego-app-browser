// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SPDY_NETWORK_TRANSACTION_H_
#define NET_SPDY_NETWORK_TRANSACTION_H_

#include <string>
#include <deque>

#include "base/basictypes.h"
#include "base/ref_counted.h"
#include "base/scoped_ptr.h"
#include "base/time.h"
#include "net/base/completion_callback.h"
#include "net/base/load_states.h"
#include "net/http/http_response_info.h"
#include "net/http/http_transaction.h"
#include "net/spdy/spdy_session.h"

namespace net {

class SpdySession;
class SpdyStream;
class HttpNetworkSession;
class HttpResponseInfo;
class IOBuffer;
class UploadDataStream;

// A SpdyNetworkTransaction can be used to fetch HTTP conent.
// The SpdyDelegate is the consumer of events from the SpdySession.
class SpdyNetworkTransaction : public HttpTransaction {
 public:
  explicit SpdyNetworkTransaction(HttpNetworkSession* session);
  virtual ~SpdyNetworkTransaction();

  // HttpTransaction methods:
  virtual int Start(const HttpRequestInfo* request_info,
                    CompletionCallback* callback,
                    LoadLog* load_log);
  virtual int RestartIgnoringLastError(CompletionCallback* callback);
  virtual int RestartWithCertificate(X509Certificate* client_cert,
                                     CompletionCallback* callback);
  virtual int RestartWithAuth(const std::wstring& username,
                              const std::wstring& password,
                              CompletionCallback* callback);
  virtual bool IsReadyToRestartForAuth() { return false; }
  virtual int Read(IOBuffer* buf, int buf_len, CompletionCallback* callback);
  virtual const HttpResponseInfo* GetResponseInfo() const;
  virtual LoadState GetLoadState() const;
  virtual uint64 GetUploadProgress() const;

 protected:
  friend class SpdyNetworkTransactionTest;

  // Provide access to the session for testing.
  SpdySession* GetSpdySession() { return spdy_.get(); }

 private:
  enum State {
    STATE_INIT_CONNECTION,
    STATE_INIT_CONNECTION_COMPLETE,
    STATE_SEND_REQUEST,
    STATE_SEND_REQUEST_COMPLETE,
    STATE_READ_HEADERS,
    STATE_READ_HEADERS_COMPLETE,
    STATE_READ_BODY,
    STATE_READ_BODY_COMPLETE,
    STATE_NONE
  };

  void DoCallback(int result);
  void OnIOComplete(int result);

  // Runs the state transition loop.
  int DoLoop(int result);

  // Each of these methods corresponds to a State value.  Those with an input
  // argument receive the result from the previous state.  If a method returns
  // ERR_IO_PENDING, then the result from OnIOComplete will be passed to the
  // next state method as the result arg.
  int DoInitConnection();
  int DoInitConnectionComplete(int result);
  int DoSendRequest();
  int DoSendRequestComplete(int result);
  int DoReadHeaders();
  int DoReadHeadersComplete(int result);
  int DoReadBody();
  int DoReadBodyComplete(int result);

  scoped_refptr<LoadLog> load_log_;

  scoped_refptr<SpdySession> spdy_;

  CompletionCallbackImpl<SpdyNetworkTransaction> io_callback_;
  CompletionCallback* user_callback_;

  // Used to pass onto the SpdyStream
  scoped_refptr<IOBuffer> user_buffer_;
  int user_buffer_len_;

  scoped_refptr<HttpNetworkSession> session_;

  const HttpRequestInfo* request_;
  HttpResponseInfo response_;

  // The time the Start method was called.
  base::TimeTicks start_time_;

  // The next state in the state machine.
  State next_state_;

  scoped_refptr<SpdyStream> stream_;

  DISALLOW_COPY_AND_ASSIGN(SpdyNetworkTransaction);
};

}  // namespace net

#endif  // NET_SPDY_NETWORK_TRANSACTION_H_
