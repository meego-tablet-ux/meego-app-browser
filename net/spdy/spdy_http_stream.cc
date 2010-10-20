// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/spdy_http_stream.h"

#include <algorithm>
#include <list>
#include <string>

#include "base/logging.h"
#include "base/message_loop.h"
#include "net/base/load_flags.h"
#include "net/base/net_util.h"
#include "net/http/http_request_info.h"
#include "net/http/http_response_info.h"
#include "net/http/http_util.h"
#include "net/spdy/spdy_http_utils.h"
#include "net/spdy/spdy_session.h"

namespace net {

SpdyHttpStream::SpdyHttpStream(SpdySession* spdy_session, bool direct)
    : ALLOW_THIS_IN_INITIALIZER_LIST(read_callback_factory_(this)),
      stream_(NULL),
      spdy_session_(spdy_session),
      response_info_(NULL),
      download_finished_(false),
      user_callback_(NULL),
      user_buffer_len_(0),
      buffered_read_callback_pending_(false),
      more_read_data_pending_(false),
      direct_(direct) { }

SpdyHttpStream::~SpdyHttpStream() {
  if (stream_)
    stream_->DetachDelegate();
}

int SpdyHttpStream::InitializeStream(const HttpRequestInfo* request_info,
                                     const BoundNetLog& stream_net_log,
                                     CompletionCallback* callback) {
  if (spdy_session_->IsClosed())
   return ERR_CONNECTION_CLOSED;

  request_info_ = request_info;
  if (request_info_->method == "GET") {
    int error = spdy_session_->GetPushStream(request_info_->url, &stream_,
                                             stream_net_log);
    if (error != OK)
      return error;
  }

  if (stream_.get())
    return OK;

  return spdy_session_->CreateStream(request_info_->url,
                                     request_info_->priority, &stream_,
                                     stream_net_log, callback);
}

const HttpResponseInfo* SpdyHttpStream::GetResponseInfo() const {
  return response_info_;
}

uint64 SpdyHttpStream::GetUploadProgress() const {
  if (!request_body_stream_.get())
    return 0;

  return request_body_stream_->position();
}

int SpdyHttpStream::ReadResponseHeaders(CompletionCallback* callback) {
  CHECK(callback);
  CHECK(!stream_->cancelled());

  if (stream_->closed())
    return stream_->response_status();

  // Check if we already have the response headers. If so, return synchronously.
  if(stream_->response_received()) {
    CHECK(stream_->is_idle());
    return OK;
  }

  // Still waiting for the response, return IO_PENDING.
  CHECK(!user_callback_);
  user_callback_ = callback;
  return ERR_IO_PENDING;
}

int SpdyHttpStream::ReadResponseBody(
    IOBuffer* buf, int buf_len, CompletionCallback* callback) {
  CHECK(stream_->is_idle());
  CHECK(buf);
  CHECK(buf_len);
  CHECK(callback);

  // If we have data buffered, complete the IO immediately.
  if (!response_body_.empty()) {
    int bytes_read = 0;
    while (!response_body_.empty() && buf_len > 0) {
      scoped_refptr<IOBufferWithSize> data = response_body_.front();
      const int bytes_to_copy = std::min(buf_len, data->size());
      memcpy(&(buf->data()[bytes_read]), data->data(), bytes_to_copy);
      buf_len -= bytes_to_copy;
      if (bytes_to_copy == data->size()) {
        response_body_.pop_front();
      } else {
        const int bytes_remaining = data->size() - bytes_to_copy;
        IOBufferWithSize* new_buffer = new IOBufferWithSize(bytes_remaining);
        memcpy(new_buffer->data(), &(data->data()[bytes_to_copy]),
               bytes_remaining);
        response_body_.pop_front();
        response_body_.push_front(new_buffer);
      }
      bytes_read += bytes_to_copy;
    }
    if (spdy_session_->flow_control())
      stream_->IncreaseRecvWindowSize(bytes_read);
    return bytes_read;
  } else if (stream_->closed()) {
    return stream_->response_status();
  }

  CHECK(!user_callback_);
  CHECK(!user_buffer_);
  CHECK_EQ(0, user_buffer_len_);

  user_callback_ = callback;
  user_buffer_ = buf;
  user_buffer_len_ = buf_len;
  return ERR_IO_PENDING;
}

void SpdyHttpStream::Close(bool not_reusable) {
  // Note: the not_reusable flag has no meaning for SPDY streams.

  Cancel();
}

int SpdyHttpStream::SendRequest(const std::string& /*headers_string*/,
                                UploadDataStream* request_body,
                                HttpResponseInfo* response,
                                CompletionCallback* callback) {
  base::Time request_time = base::Time::Now();
  CHECK(stream_.get());

  stream_->SetDelegate(this);

  HttpRequestHeaders request_headers;
  HttpUtil::BuildRequestHeaders(request_info_, request_body, NULL, false, false,
                                !direct_, &request_headers);
  linked_ptr<spdy::SpdyHeaderBlock> headers(new spdy::SpdyHeaderBlock);
  CreateSpdyHeadersFromHttpRequest(*request_info_, request_headers,
                                   headers.get(), direct_);
  stream_->set_spdy_headers(headers);

  stream_->SetRequestTime(request_time);
  // This should only get called in the case of a request occurring
  // during server push that has already begun but hasn't finished,
  // so we set the response's request time to be the actual one
  if (response_info_)
    response_info_->request_time = request_time;

  CHECK(!request_body_stream_.get());
  if (request_body) {
    if (request_body->size())
      request_body_stream_.reset(request_body);
    else
      delete request_body;
  }

  CHECK(callback);
  CHECK(!stream_->cancelled());
  CHECK(response);

  if (!stream_->pushed() && stream_->closed()) {
    if (stream_->response_status() == OK)
      return ERR_FAILED;
    else
      return stream_->response_status();
  }

  // SendRequest can be called in two cases.
  //
  // a) A client initiated request. In this case, |response_info_| should be
  //    NULL to start with.
  // b) A client request which matches a response that the server has already
  //    pushed.
  if (push_response_info_.get()) {
    *response = *(push_response_info_.get());
    push_response_info_.reset();
  }
  else
    DCHECK_EQ(static_cast<HttpResponseInfo*>(NULL), response_info_);

  response_info_ = response;

  bool has_upload_data = request_body_stream_.get() != NULL;
  int result = stream_->SendRequest(has_upload_data);
  if (result == ERR_IO_PENDING) {
    CHECK(!user_callback_);
    user_callback_ = callback;
  }
  return result;
}

void SpdyHttpStream::Cancel() {
  if (spdy_session_)
    spdy_session_->CancelPendingCreateStreams(&stream_);
  user_callback_ = NULL;
  if (stream_)
    stream_->Cancel();
}

bool SpdyHttpStream::OnSendHeadersComplete(int status) {
  if (user_callback_)
    DoCallback(status);
  return request_body_stream_.get() == NULL;
}

int SpdyHttpStream::OnSendBody() {
  CHECK(request_body_stream_.get());
  int buf_len = static_cast<int>(request_body_stream_->buf_len());
  if (!buf_len)
    return OK;
  return stream_->WriteStreamData(request_body_stream_->buf(), buf_len,
                                  spdy::DATA_FLAG_FIN);
}

bool SpdyHttpStream::OnSendBodyComplete(int status) {
  CHECK(request_body_stream_.get());
  request_body_stream_->DidConsume(status);
  return request_body_stream_->eof();
}

int SpdyHttpStream::OnResponseReceived(const spdy::SpdyHeaderBlock& response,
                                       base::Time response_time,
                                       int status) {
  if (!response_info_) {
    DCHECK(stream_->pushed());
    push_response_info_.reset(new HttpResponseInfo);
    response_info_ = push_response_info_.get();
  }

  // TODO(mbelshe): This is the time of all headers received, not just time
  // to first byte.
  DCHECK(response_info_->response_time.is_null());
  response_info_->response_time = base::Time::Now();

  if (!SpdyHeadersToHttpResponse(response, response_info_)) {
    status = ERR_INVALID_RESPONSE;
  } else {
    stream_->GetSSLInfo(&response_info_->ssl_info,
                        &response_info_->was_npn_negotiated);
    response_info_->request_time = stream_->GetRequestTime();
    response_info_->vary_data.Init(*request_info_, *response_info_->headers);
    // TODO(ahendrickson): This is recorded after the entire SYN_STREAM control
    // frame has been received and processed.  Move to framer?
    response_info_->response_time = response_time;
  }

  if (user_callback_)
    DoCallback(status);
  return status;
}

void SpdyHttpStream::OnDataReceived(const char* data, int length) {
  // Note that data may be received for a SpdyStream prior to the user calling
  // ReadResponseBody(), therefore user_buffer_ may be NULL.  This may often
  // happen for server initiated streams.
  DCHECK(!stream_->closed() || stream_->pushed());
  if (length > 0) {
    // Save the received data.
    IOBufferWithSize* io_buffer = new IOBufferWithSize(length);
    memcpy(io_buffer->data(), data, length);
    response_body_.push_back(io_buffer);

    if (user_buffer_) {
      // Handing small chunks of data to the caller creates measurable overhead.
      // We buffer data in short time-spans and send a single read notification.
      ScheduleBufferedReadCallback();
    }
  }
}

void SpdyHttpStream::OnDataSent(int length) {
  // For HTTP streams, no data is sent from the client while in the OPEN state,
  // so it is never called.
  NOTREACHED();
}

void SpdyHttpStream::OnClose(int status) {
  bool invoked_callback = false;
  if (status == net::OK) {
    // We need to complete any pending buffered read now.
    invoked_callback = DoBufferedReadCallback();
  }
  if (!invoked_callback && user_callback_)
    DoCallback(status);
}

void SpdyHttpStream::ScheduleBufferedReadCallback() {
  // If there is already a scheduled DoBufferedReadCallback, don't issue
  // another one.  Mark that we have received more data and return.
  if (buffered_read_callback_pending_) {
    more_read_data_pending_ = true;
    return;
  }

  more_read_data_pending_ = false;
  buffered_read_callback_pending_ = true;
  const int kBufferTimeMs = 1;
  MessageLoop::current()->PostDelayedTask(FROM_HERE, read_callback_factory_.
      NewRunnableMethod(&SpdyHttpStream::DoBufferedReadCallback),
      kBufferTimeMs);
}

// Checks to see if we should wait for more buffered data before notifying
// the caller.  Returns true if we should wait, false otherwise.
bool SpdyHttpStream::ShouldWaitForMoreBufferedData() const {
  // If the response is complete, there is no point in waiting.
  if (stream_->closed())
    return false;

  int bytes_buffered = 0;
  std::list<scoped_refptr<IOBufferWithSize> >::const_iterator it;
  for (it = response_body_.begin();
       it != response_body_.end() && bytes_buffered < user_buffer_len_;
       ++it)
    bytes_buffered += (*it)->size();

  return bytes_buffered < user_buffer_len_;
}

bool SpdyHttpStream::DoBufferedReadCallback() {
  read_callback_factory_.RevokeAll();
  buffered_read_callback_pending_ = false;

  // If the transaction is cancelled or errored out, we don't need to complete
  // the read.
  if (!stream_ || stream_->response_status() != OK || stream_->cancelled())
    return false;

  // When more_read_data_pending_ is true, it means that more data has
  // arrived since we started waiting.  Wait a little longer and continue
  // to buffer.
  if (more_read_data_pending_ && ShouldWaitForMoreBufferedData()) {
    ScheduleBufferedReadCallback();
    return false;
  }

  int rv = 0;
  if (user_buffer_) {
    rv = ReadResponseBody(user_buffer_, user_buffer_len_, user_callback_);
    CHECK_NE(rv, ERR_IO_PENDING);
    user_buffer_ = NULL;
    user_buffer_len_ = 0;
    DoCallback(rv);
    return true;
  }
  return false;
}

void SpdyHttpStream::DoCallback(int rv) {
  CHECK_NE(rv, ERR_IO_PENDING);
  CHECK(user_callback_);

  // Since Run may result in being called back, clear user_callback_ in advance.
  CompletionCallback* c = user_callback_;
  user_callback_ = NULL;
  c->Run(rv);
}

void SpdyHttpStream::GetSSLInfo(SSLInfo* ssl_info) {
  DCHECK(stream_);
  bool using_npn;
  stream_->GetSSLInfo(ssl_info, &using_npn);
}

void SpdyHttpStream::GetSSLCertRequestInfo(
    SSLCertRequestInfo* cert_request_info) {
  DCHECK(stream_);
  stream_->GetSSLCertRequestInfo(cert_request_info);
}

}  // namespace net
