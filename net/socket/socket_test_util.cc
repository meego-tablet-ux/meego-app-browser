// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/socket_test_util.h"

#include <algorithm>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/message_loop.h"
#include "net/base/ssl_info.h"
#include "net/socket/socket.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

MockClientSocket::MockClientSocket()
    : ALLOW_THIS_IN_INITIALIZER_LIST(method_factory_(this)),
      connected_(false) {
}

void MockClientSocket::GetSSLInfo(net::SSLInfo* ssl_info) {
  NOTREACHED();
}

void MockClientSocket::GetSSLCertRequestInfo(
    net::SSLCertRequestInfo* cert_request_info) {
  NOTREACHED();
}

void MockClientSocket::Disconnect() {
  connected_ = false;
}

bool MockClientSocket::IsConnected() const {
  return connected_;
}

bool MockClientSocket::IsConnectedAndIdle() const {
  return connected_;
}

#if defined(OS_LINUX)
int MockClientSocket::GetPeerName(struct sockaddr *name, socklen_t *namelen) {
  memset(reinterpret_cast<char *>(name), 0, *namelen);
  return net::OK;
}
#endif  // defined(OS_LINUX)

void MockClientSocket::RunCallbackAsync(net::CompletionCallback* callback,
                                        int result) {
  MessageLoop::current()->PostTask(FROM_HERE,
      method_factory_.NewRunnableMethod(
          &MockClientSocket::RunCallback, callback, result));
}

void MockClientSocket::RunCallback(net::CompletionCallback* callback,
                                   int result) {
  if (callback)
    callback->Run(result);
}

MockTCPClientSocket::MockTCPClientSocket(const net::AddressList& addresses,
                                         net::SocketDataProvider* data)
    : addresses_(addresses),
      data_(data),
      read_offset_(0),
      read_data_(true, net::ERR_UNEXPECTED),
      need_read_data_(true) {
  DCHECK(data_);
  data_->Reset();
}

int MockTCPClientSocket::Connect(net::CompletionCallback* callback,
                                 LoadLog* load_log) {
  if (connected_)
    return net::OK;
  connected_ = true;
  if (data_->connect_data().async) {
    RunCallbackAsync(callback, data_->connect_data().result);
    return net::ERR_IO_PENDING;
  }
  return data_->connect_data().result;
}

int MockTCPClientSocket::Read(net::IOBuffer* buf, int buf_len,
                              net::CompletionCallback* callback) {
  if (!IsConnected())
    return net::ERR_UNEXPECTED;

  if (need_read_data_) {
    read_data_ = data_->GetNextRead();
    need_read_data_ = false;
  }
  int result = read_data_.result;
  if (read_data_.data) {
    if (read_data_.data_len - read_offset_ > 0) {
      result = std::min(buf_len, read_data_.data_len - read_offset_);
      memcpy(buf->data(), read_data_.data + read_offset_, result);
      read_offset_ += result;
      if (read_offset_ == read_data_.data_len) {
        need_read_data_ = true;
        read_offset_ = 0;
      }
    } else {
      result = 0;  // EOF
    }
  }
  if (read_data_.async) {
    RunCallbackAsync(callback, result);
    return net::ERR_IO_PENDING;
  }
  return result;
}

int MockTCPClientSocket::Write(net::IOBuffer* buf, int buf_len,
                               net::CompletionCallback* callback) {
  DCHECK(buf);
  DCHECK_GT(buf_len, 0);

  if (!IsConnected())
    return net::ERR_UNEXPECTED;

  std::string data(buf->data(), buf_len);
  net::MockWriteResult write_result = data_->OnWrite(data);

  if (write_result.async) {
    RunCallbackAsync(callback, write_result.result);
    return net::ERR_IO_PENDING;
  }
  return write_result.result;
}

class MockSSLClientSocket::ConnectCallback :
    public net::CompletionCallbackImpl<MockSSLClientSocket::ConnectCallback> {
 public:
  ConnectCallback(MockSSLClientSocket *ssl_client_socket,
                  net::CompletionCallback* user_callback,
                  int rv)
      : ALLOW_THIS_IN_INITIALIZER_LIST(
          net::CompletionCallbackImpl<MockSSLClientSocket::ConnectCallback>(
                this, &ConnectCallback::Wrapper)),
        ssl_client_socket_(ssl_client_socket),
        user_callback_(user_callback),
        rv_(rv) {
  }

 private:
  void Wrapper(int rv) {
    if (rv_ == net::OK)
      ssl_client_socket_->connected_ = true;
    user_callback_->Run(rv_);
    delete this;
  }

  MockSSLClientSocket* ssl_client_socket_;
  net::CompletionCallback* user_callback_;
  int rv_;
};

MockSSLClientSocket::MockSSLClientSocket(
    net::ClientSocket* transport_socket,
    const std::string& hostname,
    const net::SSLConfig& ssl_config,
    net::SSLSocketDataProvider* data)
    : transport_(transport_socket),
      data_(data) {
  DCHECK(data_);
}

MockSSLClientSocket::~MockSSLClientSocket() {
  Disconnect();
}

void MockSSLClientSocket::GetSSLInfo(net::SSLInfo* ssl_info) {
  ssl_info->Reset();
}

int MockSSLClientSocket::Connect(net::CompletionCallback* callback,
                                 LoadLog* load_log) {
  ConnectCallback* connect_callback = new ConnectCallback(
      this, callback, data_->connect.result);
  int rv = transport_->Connect(connect_callback, load_log);
  if (rv == net::OK) {
    delete connect_callback;
    if (data_->connect.async) {
      RunCallbackAsync(callback, data_->connect.result);
      return net::ERR_IO_PENDING;
    }
    if (data_->connect.result == net::OK)
      connected_ = true;
    return data_->connect.result;
  }
  return rv;
}

void MockSSLClientSocket::Disconnect() {
  MockClientSocket::Disconnect();
  if (transport_ != NULL)
    transport_->Disconnect();
}

int MockSSLClientSocket::Read(net::IOBuffer* buf, int buf_len,
                              net::CompletionCallback* callback) {
  return transport_->Read(buf, buf_len, callback);
}

int MockSSLClientSocket::Write(net::IOBuffer* buf, int buf_len,
                               net::CompletionCallback* callback) {
  return transport_->Write(buf, buf_len, callback);
}

MockRead StaticSocketDataProvider::GetNextRead() {
  return reads_[read_index_++];
}

MockWriteResult StaticSocketDataProvider::OnWrite(const std::string& data) {
  if (!writes_) {
    // Not using mock writes; succeed synchronously.
    return MockWriteResult(false, data.length());
  }

  // Check that what we are writing matches the expectation.
  // Then give the mocked return value.
  net::MockWrite* w = &writes_[write_index_++];
  int result = w->result;
  if (w->data) {
    std::string expected_data(w->data, w->data_len);
    EXPECT_EQ(expected_data, data);
    if (expected_data != data)
      return MockWriteResult(false, net::ERR_UNEXPECTED);
    if (result == net::OK)
      result = w->data_len;
  }
  return MockWriteResult(w->async, result);
}

void StaticSocketDataProvider::Reset() {
  read_index_ = 0;
  write_index_ = 0;
}

DynamicSocketDataProvider::DynamicSocketDataProvider()
    : short_read_limit_(0),
      allow_unconsumed_reads_(false) {
}

MockRead DynamicSocketDataProvider::GetNextRead() {
  if (reads_.empty())
    return MockRead(true, ERR_UNEXPECTED);
  MockRead result = reads_.front();
  if (short_read_limit_ == 0 || result.data_len <= short_read_limit_) {
    reads_.pop_front();
  } else {
    result.data_len = short_read_limit_;
    reads_.front().data += result.data_len;
    reads_.front().data_len -= result.data_len;
  }
  return result;
}

void DynamicSocketDataProvider::Reset() {
  reads_.clear();
}

void DynamicSocketDataProvider::SimulateRead(const char* data) {
  if (!allow_unconsumed_reads_) {
    EXPECT_TRUE(reads_.empty()) << "Unconsumed read: " << reads_.front().data;
  }
  reads_.push_back(MockRead(data));
}

void MockClientSocketFactory::AddSocketDataProvider(
    SocketDataProvider* data) {
  mock_data_.Add(data);
}

void MockClientSocketFactory::AddSSLSocketDataProvider(
    SSLSocketDataProvider* data) {
  mock_ssl_data_.Add(data);
}

void MockClientSocketFactory::ResetNextMockIndexes() {
  mock_data_.ResetNextIndex();
  mock_ssl_data_.ResetNextIndex();
}

MockTCPClientSocket* MockClientSocketFactory::GetMockTCPClientSocket(
    int index) const {
  return tcp_client_sockets_[index];
}

MockSSLClientSocket* MockClientSocketFactory::GetMockSSLClientSocket(
    int index) const {
  return ssl_client_sockets_[index];
}

ClientSocket* MockClientSocketFactory::CreateTCPClientSocket(
    const AddressList& addresses) {
  MockTCPClientSocket* socket =
      new MockTCPClientSocket(addresses, mock_data_.GetNext());
  tcp_client_sockets_.push_back(socket);
  return socket;
}

SSLClientSocket* MockClientSocketFactory::CreateSSLClientSocket(
    ClientSocket* transport_socket,
    const std::string& hostname,
    const SSLConfig& ssl_config) {
  MockSSLClientSocket* socket =
      new MockSSLClientSocket(transport_socket, hostname, ssl_config,
                              mock_ssl_data_.GetNext());
  ssl_client_sockets_.push_back(socket);
  return socket;
}

int TestSocketRequest::WaitForResult() {
  return callback_.WaitForResult();
}

void TestSocketRequest::RunWithParams(const Tuple1<int>& params) {
  callback_.RunWithParams(params);
  (*completion_count_)++;
  request_order_->push_back(this);
}

// static
const int ClientSocketPoolTest::kIndexOutOfBounds = -1;

// static
const int ClientSocketPoolTest::kRequestNotFound = -2;

void ClientSocketPoolTest::SetUp() {
  completion_count_ = 0;
}

void ClientSocketPoolTest::TearDown() {
  // The tests often call Reset() on handles at the end which may post
  // DoReleaseSocket() tasks.
  // Pending tasks created by client_socket_pool_base_unittest.cc are
  // posted two milliseconds into the future and thus won't become
  // scheduled until that time.
  // We wait a few milliseconds to make sure that all such future tasks
  // are ready to run, before calling RunAllPending(). This will work
  // correctly even if Sleep() finishes late (and it should never finish
  // early), as all we have to ensure is that actual wall-time has progressed
  // past the scheduled starting time of the pending task.
  PlatformThread::Sleep(10);
  MessageLoop::current()->RunAllPending();
}

int ClientSocketPoolTest::GetOrderOfRequest(size_t index) {
  index--;
  if (index >= requests_.size())
    return kIndexOutOfBounds;

  for (size_t i = 0; i < request_order_.size(); i++)
    if (requests_[index] == request_order_[i])
      return i + 1;

  return kRequestNotFound;
}

bool ClientSocketPoolTest::ReleaseOneConnection(KeepAlive keep_alive) {
  ScopedVector<TestSocketRequest>::iterator i;
  for (i = requests_.begin(); i != requests_.end(); ++i) {
    if ((*i)->handle()->is_initialized()) {
      if (keep_alive == NO_KEEP_ALIVE)
        (*i)->handle()->socket()->Disconnect();
      (*i)->handle()->Reset();
      MessageLoop::current()->RunAllPending();
      return true;
    }
  }
  return false;
}

void ClientSocketPoolTest::ReleaseAllConnections(KeepAlive keep_alive) {
  bool released_one;
  do {
    released_one = ReleaseOneConnection(keep_alive);
  } while (released_one);
}

}  // namespace net
