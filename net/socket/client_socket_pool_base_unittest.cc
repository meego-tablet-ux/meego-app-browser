// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/client_socket_pool_base.h"

#include "base/compiler_specific.h"
#include "base/message_loop.h"
#include "base/scoped_vector.h"
#include "net/base/net_errors.h"
#include "net/base/test_completion_callback.h"
#include "net/socket/client_socket.h"
#include "net/socket/client_socket_factory.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/socket_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace {

const int kDefaultMaxSockets = 4;

const int kDefaultMaxSocketsPerGroup = 2;

const int kDefaultPriority = 5;

class MockClientSocket : public ClientSocket {
 public:
  MockClientSocket() : connected_(false) {}

  // Socket methods:
  virtual int Read(
      IOBuffer* /* buf */, int /* len */, CompletionCallback* /* callback */) {
    return ERR_UNEXPECTED;
  }

  virtual int Write(
      IOBuffer* /* buf */, int /* len */, CompletionCallback* /* callback */) {
    return ERR_UNEXPECTED;
  }

  // ClientSocket methods:

  virtual int Connect(CompletionCallback* callback) {
    connected_ = true;
    return OK;
  }

  virtual void Disconnect() { connected_ = false; }
  virtual bool IsConnected() const { return connected_; }
  virtual bool IsConnectedAndIdle() const { return connected_; }

#if defined(OS_LINUX)
  virtual int GetPeerName(struct sockaddr* /* name */,
                          socklen_t* /* namelen */) {
    return 0;
  }
#endif

 private:
  bool connected_;

  DISALLOW_COPY_AND_ASSIGN(MockClientSocket);
};

class TestConnectJob;

class MockClientSocketFactory : public ClientSocketFactory {
 public:
  MockClientSocketFactory() : allocation_count_(0) {}

  virtual ClientSocket* CreateTCPClientSocket(const AddressList& addresses) {
    allocation_count_++;
    return NULL;
  }

  virtual SSLClientSocket* CreateSSLClientSocket(
      ClientSocket* transport_socket,
      const std::string& hostname,
      const SSLConfig& ssl_config) {
    NOTIMPLEMENTED();
    return NULL;
  }

  void WaitForSignal(TestConnectJob* job) { waiting_jobs_.push_back(job); }
  void SignalJobs();

  int allocation_count() const { return allocation_count_; }

 private:
  int allocation_count_;
  std::vector<TestConnectJob*> waiting_jobs_;
};

class TestConnectJob : public ConnectJob {
 public:
  enum JobType {
    kMockJob,
    kMockFailingJob,
    kMockPendingJob,
    kMockPendingFailingJob,
    kMockWaitingJob,
    kMockAdvancingLoadStateJob,
  };

  TestConnectJob(JobType job_type,
                 const std::string& group_name,
                 const ClientSocketPoolBase::Request& request,
                 ConnectJob::Delegate* delegate,
                 MockClientSocketFactory* client_socket_factory)
      : ConnectJob(group_name, request.handle, delegate),
        job_type_(job_type),
        client_socket_factory_(client_socket_factory),
        method_factory_(ALLOW_THIS_IN_INITIALIZER_LIST(this)) {}

  // ConnectJob methods:

  virtual int Connect() {
    AddressList ignored;
    client_socket_factory_->CreateTCPClientSocket(ignored);
    switch (job_type_) {
      case kMockJob:
        return DoConnect(true /* successful */, false /* sync */);
      case kMockFailingJob:
        return DoConnect(false /* error */, false /* sync */);
      case kMockPendingJob:
        set_load_state(LOAD_STATE_CONNECTING);
        MessageLoop::current()->PostTask(
            FROM_HERE,
            method_factory_.NewRunnableMethod(
               &TestConnectJob::DoConnect,
               true /* successful */,
               true /* async */));
        return ERR_IO_PENDING;
      case kMockPendingFailingJob:
        set_load_state(LOAD_STATE_CONNECTING);
        MessageLoop::current()->PostTask(
            FROM_HERE,
            method_factory_.NewRunnableMethod(
               &TestConnectJob::DoConnect,
               false /* error */,
               true  /* async */));
        return ERR_IO_PENDING;
      case kMockWaitingJob:
        client_socket_factory_->WaitForSignal(this);
        waiting_success_ = true;
        return ERR_IO_PENDING;
      case kMockAdvancingLoadStateJob:
        MessageLoop::current()->PostTask(
            FROM_HERE,
            method_factory_.NewRunnableMethod(
                &TestConnectJob::AdvanceLoadState, load_state()));
        return ERR_IO_PENDING;
      default:
        NOTREACHED();
        return ERR_FAILED;
    }
  }

  void Signal() {
    DoConnect(waiting_success_, true /* async */);
  }

 private:
  int DoConnect(bool succeed, bool was_async) {
    int result = ERR_CONNECTION_FAILED;
    if (succeed) {
      result = OK;
      set_socket(new MockClientSocket());
      socket()->Connect(NULL);
    }

    if (was_async)
      delegate()->OnConnectJobComplete(result, this);
    return result;
  }

  void AdvanceLoadState(LoadState state) {
    int tmp = state;
    tmp++;
    state = static_cast<LoadState>(tmp);
    set_load_state(state);
    // Post a delayed task so RunAllPending() won't run it.
    MessageLoop::current()->PostDelayedTask(
        FROM_HERE,
        method_factory_.NewRunnableMethod(&TestConnectJob::AdvanceLoadState,
                                          state),
        1 /* 1ms delay */);
  }

  bool waiting_success_;
  const JobType job_type_;
  MockClientSocketFactory* const client_socket_factory_;
  ScopedRunnableMethodFactory<TestConnectJob> method_factory_;

  DISALLOW_COPY_AND_ASSIGN(TestConnectJob);
};

class TestConnectJobFactory : public ClientSocketPoolBase::ConnectJobFactory {
 public:
  explicit TestConnectJobFactory(MockClientSocketFactory* client_socket_factory)
      : job_type_(TestConnectJob::kMockJob),
        client_socket_factory_(client_socket_factory) {}

  virtual ~TestConnectJobFactory() {}

  void set_job_type(TestConnectJob::JobType job_type) { job_type_ = job_type; }

  // ConnectJobFactory methods:

  virtual ConnectJob* NewConnectJob(
      const std::string& group_name,
      const ClientSocketPoolBase::Request& request,
      ConnectJob::Delegate* delegate) const {
    return new TestConnectJob(job_type_,
                              group_name,
                              request,
                              delegate,
                              client_socket_factory_);
  }

 private:
  TestConnectJob::JobType job_type_;
  MockClientSocketFactory* const client_socket_factory_;

  DISALLOW_COPY_AND_ASSIGN(TestConnectJobFactory);
};

class TestClientSocketPool : public ClientSocketPool {
 public:
  TestClientSocketPool(
      int max_sockets,
      int max_sockets_per_group,
      ClientSocketPoolBase::ConnectJobFactory* connect_job_factory)
      : base_(new ClientSocketPoolBase(
          max_sockets, max_sockets_per_group, connect_job_factory)) {}

  virtual int RequestSocket(
      const std::string& group_name,
      const HostResolver::RequestInfo& resolve_info,
      int priority,
      ClientSocketHandle* handle,
      CompletionCallback* callback) {
    return base_->RequestSocket(
        group_name, resolve_info, priority, handle, callback);
  }

  virtual void CancelRequest(
      const std::string& group_name,
      const ClientSocketHandle* handle) {
    base_->CancelRequest(group_name, handle);
  }

  virtual void ReleaseSocket(
      const std::string& group_name,
      ClientSocket* socket) {
    base_->ReleaseSocket(group_name, socket);
  }

  virtual void CloseIdleSockets() {
    base_->CloseIdleSockets();
  }

  virtual int IdleSocketCount() const { return base_->idle_socket_count(); }

  virtual int IdleSocketCountInGroup(const std::string& group_name) const {
    return base_->IdleSocketCountInGroup(group_name);
  }

  virtual LoadState GetLoadState(const std::string& group_name,
                                 const ClientSocketHandle* handle) const {
    return base_->GetLoadState(group_name, handle);
  }

  const ClientSocketPoolBase* base() const { return base_.get(); }

 private:
  const scoped_refptr<ClientSocketPoolBase> base_;

  DISALLOW_COPY_AND_ASSIGN(TestClientSocketPool);
};

void MockClientSocketFactory::SignalJobs() {
  for (std::vector<TestConnectJob*>::iterator it = waiting_jobs_.begin();
       it != waiting_jobs_.end(); ++it) {
    (*it)->Signal();
  }
  waiting_jobs_.clear();
}

class ClientSocketPoolBaseTest : public ClientSocketPoolTest {
 protected:
  ClientSocketPoolBaseTest()
      : connect_job_factory_(
            new TestConnectJobFactory(&client_socket_factory_)) {}

  void CreatePool(int max_sockets, int max_sockets_per_group) {
    DCHECK(!pool_.get());
    pool_ = new TestClientSocketPool(max_sockets,
                                     max_sockets_per_group,
                                     connect_job_factory_);
  }

  int StartRequest(const std::string& group_name, int priority) {
    return StartRequestUsingPool(pool_.get(), group_name, priority);
  }

  virtual void TearDown() {
    // Need to delete |pool_| before we turn late binding back off. We also need
    // to delete |requests_| because the pool is reference counted and requests
    // keep reference to it.
    // TODO(willchan): Remove this part when late binding becomes the default.
    pool_ = NULL;
    requests_.reset();

    ClientSocketPoolBase::EnableLateBindingOfSockets(false);

    ClientSocketPoolTest::TearDown();
  }

  MockClientSocketFactory client_socket_factory_;
  TestConnectJobFactory* const connect_job_factory_;
  scoped_refptr<TestClientSocketPool> pool_;
};

TEST_F(ClientSocketPoolBaseTest, BasicSynchronous) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  TestCompletionCallback callback;
  ClientSocketHandle handle(pool_.get());
  EXPECT_EQ(OK, handle.Init("a", ignored_request_info_, kDefaultPriority,
                            &callback));
  EXPECT_TRUE(handle.is_initialized());
  EXPECT_TRUE(handle.socket());
  handle.Reset();
}

TEST_F(ClientSocketPoolBaseTest, BasicAsynchronous) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingJob);
  TestSocketRequest req(pool_.get(), &request_order_, &completion_count_);
  int rv = req.handle()->Init("a", ignored_request_info_, 0, &req);
  EXPECT_EQ(ERR_IO_PENDING, rv);
  EXPECT_EQ(LOAD_STATE_CONNECTING, pool_->GetLoadState("a", req.handle()));
  EXPECT_EQ(OK, req.WaitForResult());
  EXPECT_TRUE(req.handle()->is_initialized());
  EXPECT_TRUE(req.handle()->socket());
  req.handle()->Reset();
}

TEST_F(ClientSocketPoolBaseTest, InitConnectionFailure) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  connect_job_factory_->set_job_type(TestConnectJob::kMockFailingJob);
  TestSocketRequest req(pool_.get(), &request_order_, &completion_count_);
  EXPECT_EQ(ERR_CONNECTION_FAILED,
            req.handle()->Init("a", ignored_request_info_,
                               kDefaultPriority, &req));
}

TEST_F(ClientSocketPoolBaseTest, InitConnectionAsynchronousFailure) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingFailingJob);
  TestSocketRequest req(pool_.get(), &request_order_, &completion_count_);
  EXPECT_EQ(ERR_IO_PENDING,
            req.handle()->Init("a", ignored_request_info_, kDefaultPriority,
                               &req));
  EXPECT_EQ(LOAD_STATE_CONNECTING, pool_->GetLoadState("a", req.handle()));
  EXPECT_EQ(ERR_CONNECTION_FAILED, req.WaitForResult());
}

TEST_F(ClientSocketPoolBaseTest, TotalLimit) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  EXPECT_EQ(OK, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(OK, StartRequest("b", kDefaultPriority));
  EXPECT_EQ(OK, StartRequest("c", kDefaultPriority));
  EXPECT_EQ(OK, StartRequest("d", kDefaultPriority));

  EXPECT_EQ(static_cast<int>(requests_.size()),
            client_socket_factory_.allocation_count());
  EXPECT_EQ(requests_.size() - kDefaultMaxSockets, completion_count_);

  EXPECT_EQ(ERR_IO_PENDING, StartRequest("e", kDefaultPriority));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("f", kDefaultPriority));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("g", kDefaultPriority));

  ReleaseAllConnections(KEEP_ALIVE);

  EXPECT_EQ(static_cast<int>(requests_.size()),
            client_socket_factory_.allocation_count());
  EXPECT_EQ(requests_.size() - kDefaultMaxSockets, completion_count_);

  EXPECT_EQ(1, GetOrderOfRequest(1));
  EXPECT_EQ(2, GetOrderOfRequest(2));
  EXPECT_EQ(3, GetOrderOfRequest(3));
  EXPECT_EQ(4, GetOrderOfRequest(4));
  EXPECT_EQ(5, GetOrderOfRequest(5));
  EXPECT_EQ(6, GetOrderOfRequest(6));
  EXPECT_EQ(7, GetOrderOfRequest(7));

  // Make sure we test order of all requests made.
  EXPECT_EQ(kIndexOutOfBounds, GetOrderOfRequest(8));
}

TEST_F(ClientSocketPoolBaseTest, TotalLimitReachedNewGroup) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  // Reach all limits: max total sockets, and max sockets per group.
  EXPECT_EQ(OK, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(OK, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(OK, StartRequest("b", kDefaultPriority));
  EXPECT_EQ(OK, StartRequest("b", kDefaultPriority));

  EXPECT_EQ(static_cast<int>(requests_.size()),
            client_socket_factory_.allocation_count());
  EXPECT_EQ(requests_.size() - kDefaultMaxSockets, completion_count_);

  // Now create a new group and verify that we don't starve it.
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("c", kDefaultPriority));

  ReleaseAllConnections(KEEP_ALIVE);

  EXPECT_EQ(static_cast<int>(requests_.size()),
            client_socket_factory_.allocation_count());
  EXPECT_EQ(requests_.size() - kDefaultMaxSockets, completion_count_);

  EXPECT_EQ(1, GetOrderOfRequest(1));
  EXPECT_EQ(2, GetOrderOfRequest(2));
  EXPECT_EQ(3, GetOrderOfRequest(3));
  EXPECT_EQ(4, GetOrderOfRequest(4));
  EXPECT_EQ(5, GetOrderOfRequest(5));

  // Make sure we test order of all requests made.
  EXPECT_EQ(kIndexOutOfBounds, GetOrderOfRequest(6));
}

TEST_F(ClientSocketPoolBaseTest, TotalLimitRespectsPriority) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  EXPECT_EQ(OK, StartRequest("b", 3));
  EXPECT_EQ(OK, StartRequest("a", 3));
  EXPECT_EQ(OK, StartRequest("b", 6));
  EXPECT_EQ(OK, StartRequest("a", 6));

  EXPECT_EQ(static_cast<int>(requests_.size()),
            client_socket_factory_.allocation_count());

  EXPECT_EQ(ERR_IO_PENDING, StartRequest("c", 4));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 5));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("b", 7));

  ReleaseAllConnections(KEEP_ALIVE);

  // We're re-using one socket for group "a", and one for "b".
  EXPECT_EQ(static_cast<int>(requests_.size()) - 2,
            client_socket_factory_.allocation_count());
  EXPECT_EQ(requests_.size() - kDefaultMaxSockets, completion_count_);

  // First 4 requests don't have to wait, and finish in order.
  EXPECT_EQ(1, GetOrderOfRequest(1));
  EXPECT_EQ(2, GetOrderOfRequest(2));
  EXPECT_EQ(3, GetOrderOfRequest(3));
  EXPECT_EQ(4, GetOrderOfRequest(4));

  // Request ("b", 7) has the highest priority, then ("a", 5),
  // and then ("c", 4).
  EXPECT_EQ(7, GetOrderOfRequest(5));
  EXPECT_EQ(6, GetOrderOfRequest(6));
  EXPECT_EQ(5, GetOrderOfRequest(7));

  // Make sure we test order of all requests made.
  EXPECT_EQ(kIndexOutOfBounds, GetOrderOfRequest(8));
}

TEST_F(ClientSocketPoolBaseTest, TotalLimitRespectsGroupLimit) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  EXPECT_EQ(OK, StartRequest("a", 3));
  EXPECT_EQ(OK, StartRequest("a", 6));
  EXPECT_EQ(OK, StartRequest("b", 3));
  EXPECT_EQ(OK, StartRequest("b", 6));

  EXPECT_EQ(static_cast<int>(requests_.size()),
            client_socket_factory_.allocation_count());

  EXPECT_EQ(ERR_IO_PENDING, StartRequest("c", 6));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 4));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("b", 7));

  ReleaseAllConnections(KEEP_ALIVE);

  // We're re-using one socket for group "a", and one for "b".
  EXPECT_EQ(static_cast<int>(requests_.size()) - 2,
            client_socket_factory_.allocation_count());
  EXPECT_EQ(requests_.size() - kDefaultMaxSockets, completion_count_);

  // First 4 requests don't have to wait, and finish in order.
  EXPECT_EQ(1, GetOrderOfRequest(1));
  EXPECT_EQ(2, GetOrderOfRequest(2));
  EXPECT_EQ(3, GetOrderOfRequest(3));
  EXPECT_EQ(4, GetOrderOfRequest(4));

  // Request ("b", 7) has the highest priority, but we can't make new socket for
  // group "b", because it has reached the per-group limit. Then we make
  // socket for ("c", 6), because it has higher priority than ("a", 4),
  // and we still can't make a socket for group "b".
  EXPECT_EQ(5, GetOrderOfRequest(5));
  EXPECT_EQ(6, GetOrderOfRequest(6));
  EXPECT_EQ(7, GetOrderOfRequest(7));

  // Make sure we test order of all requests made.
  EXPECT_EQ(kIndexOutOfBounds, GetOrderOfRequest(8));
}

// Make sure that we count connecting sockets against the total limit.
TEST_F(ClientSocketPoolBaseTest, TotalLimitCountsConnectingSockets) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  EXPECT_EQ(OK, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(OK, StartRequest("b", kDefaultPriority));
  EXPECT_EQ(OK, StartRequest("c", kDefaultPriority));

  // Create one asynchronous request.
  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingJob);
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("d", kDefaultPriority));

  // The next synchronous request should wait for its turn.
  connect_job_factory_->set_job_type(TestConnectJob::kMockJob);
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("e", kDefaultPriority));

  ReleaseAllConnections(KEEP_ALIVE);

  EXPECT_EQ(static_cast<int>(requests_.size()),
            client_socket_factory_.allocation_count());

  EXPECT_EQ(1, GetOrderOfRequest(1));
  EXPECT_EQ(2, GetOrderOfRequest(2));
  EXPECT_EQ(3, GetOrderOfRequest(3));
  EXPECT_EQ(4, GetOrderOfRequest(4));
  EXPECT_EQ(5, GetOrderOfRequest(5));

  // Make sure we test order of all requests made.
  EXPECT_EQ(kIndexOutOfBounds, GetOrderOfRequest(6));
}

// Inside ClientSocketPoolBase we have a may_have_stalled_group flag,
// which tells it to use more expensive, but accurate, group selection
// algorithm. Make sure it doesn't get stuck in the "on" state.
TEST_F(ClientSocketPoolBaseTest, MayHaveStalledGroupReset) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  EXPECT_FALSE(pool_->base()->may_have_stalled_group());

  // Reach group socket limit.
  EXPECT_EQ(OK, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(OK, StartRequest("a", kDefaultPriority));
  EXPECT_FALSE(pool_->base()->may_have_stalled_group());

  // Reach total limit, but don't request more sockets.
  EXPECT_EQ(OK, StartRequest("b", kDefaultPriority));
  EXPECT_EQ(OK, StartRequest("b", kDefaultPriority));
  EXPECT_FALSE(pool_->base()->may_have_stalled_group());

  // Request one more socket while we are at the maximum sockets limit.
  // This should flip the may_have_stalled_group flag.
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("c", kDefaultPriority));
  EXPECT_TRUE(pool_->base()->may_have_stalled_group());

  // After releasing first connection for "a", we're still at the
  // maximum sockets limit, but every group's pending queue is empty,
  // so we reset the flag.
  EXPECT_TRUE(ReleaseOneConnection(KEEP_ALIVE));
  EXPECT_FALSE(pool_->base()->may_have_stalled_group());

  // Requesting additional socket while at the total limit should
  // flip the flag back to "on".
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("c", kDefaultPriority));
  EXPECT_TRUE(pool_->base()->may_have_stalled_group());

  // We'll request one more socket to verify that we don't reset the flag
  // too eagerly.
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("d", kDefaultPriority));
  EXPECT_TRUE(pool_->base()->may_have_stalled_group());

  // We're at the maximum socket limit, and still have one request pending
  // for "d". Flag should be "on".
  EXPECT_TRUE(ReleaseOneConnection(KEEP_ALIVE));
  EXPECT_TRUE(pool_->base()->may_have_stalled_group());

  // Now every group's pending queue should be empty again.
  EXPECT_TRUE(ReleaseOneConnection(KEEP_ALIVE));
  EXPECT_FALSE(pool_->base()->may_have_stalled_group());

  ReleaseAllConnections(KEEP_ALIVE);
  EXPECT_FALSE(pool_->base()->may_have_stalled_group());
}

TEST_F(ClientSocketPoolBaseTest, PendingRequests) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  EXPECT_EQ(OK, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(OK, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 1));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 3));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 4));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 2));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 1));

  ReleaseAllConnections(KEEP_ALIVE);

  EXPECT_EQ(kDefaultMaxSocketsPerGroup,
            client_socket_factory_.allocation_count());
  EXPECT_EQ(requests_.size() - kDefaultMaxSocketsPerGroup, completion_count_);

  EXPECT_EQ(1, GetOrderOfRequest(1));
  EXPECT_EQ(2, GetOrderOfRequest(2));
  EXPECT_EQ(6, GetOrderOfRequest(3));
  EXPECT_EQ(4, GetOrderOfRequest(4));
  EXPECT_EQ(3, GetOrderOfRequest(5));
  EXPECT_EQ(5, GetOrderOfRequest(6));
  EXPECT_EQ(7, GetOrderOfRequest(7));

  // Make sure we test order of all requests made.
  EXPECT_EQ(kIndexOutOfBounds, GetOrderOfRequest(8));
}

TEST_F(ClientSocketPoolBaseTest, PendingRequests_NoKeepAlive) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  EXPECT_EQ(OK, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(OK, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 1));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 3));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 4));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 2));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 1));

  ReleaseAllConnections(NO_KEEP_ALIVE);

  for (size_t i = kDefaultMaxSocketsPerGroup; i < requests_.size(); ++i)
    EXPECT_EQ(OK, requests_[i]->WaitForResult());

  EXPECT_EQ(static_cast<int>(requests_.size()),
            client_socket_factory_.allocation_count());
  EXPECT_EQ(requests_.size() - kDefaultMaxSocketsPerGroup, completion_count_);
}

// This test will start up a RequestSocket() and then immediately Cancel() it.
// The pending connect job will be cancelled and should not call back into
// ClientSocketPoolBase.
TEST_F(ClientSocketPoolBaseTest, CancelRequestClearGroup) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingJob);
  TestSocketRequest req(pool_.get(), &request_order_, &completion_count_);
  EXPECT_EQ(ERR_IO_PENDING,
            req.handle()->Init("a", ignored_request_info_,
                               kDefaultPriority, &req));
  req.handle()->Reset();
}

TEST_F(ClientSocketPoolBaseTest, TwoRequestsCancelOne) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingJob);
  TestSocketRequest req(pool_.get(), &request_order_, &completion_count_);
  TestSocketRequest req2(pool_.get(), &request_order_, &completion_count_);

  EXPECT_EQ(ERR_IO_PENDING,
            req.handle()->Init("a", ignored_request_info_,
                               kDefaultPriority, &req));
  EXPECT_EQ(ERR_IO_PENDING,
            req2.handle()->Init("a", ignored_request_info_,
                                kDefaultPriority, &req2));

  req.handle()->Reset();

  EXPECT_EQ(OK, req2.WaitForResult());
  req2.handle()->Reset();
}

TEST_F(ClientSocketPoolBaseTest, ConnectCancelConnect) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingJob);
  ClientSocketHandle handle(pool_.get());
  TestCompletionCallback callback;
  TestSocketRequest req(pool_.get(), &request_order_, &completion_count_);

  EXPECT_EQ(ERR_IO_PENDING,
            handle.Init("a", ignored_request_info_,
                        kDefaultPriority, &callback));

  handle.Reset();

  TestCompletionCallback callback2;
  EXPECT_EQ(ERR_IO_PENDING,
            handle.Init("a", ignored_request_info_,
                        kDefaultPriority, &callback2));

  EXPECT_EQ(OK, callback2.WaitForResult());
  EXPECT_FALSE(callback.have_result());

  handle.Reset();
}

TEST_F(ClientSocketPoolBaseTest, CancelRequest) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  EXPECT_EQ(OK, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(OK, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 1));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 3));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 4));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 2));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 1));

  // Cancel a request.
  size_t index_to_cancel = kDefaultMaxSocketsPerGroup + 2;
  EXPECT_FALSE(requests_[index_to_cancel]->handle()->is_initialized());
  requests_[index_to_cancel]->handle()->Reset();

  ReleaseAllConnections(KEEP_ALIVE);

  EXPECT_EQ(kDefaultMaxSocketsPerGroup,
            client_socket_factory_.allocation_count());
  EXPECT_EQ(requests_.size() - kDefaultMaxSocketsPerGroup - 1,
            completion_count_);

  EXPECT_EQ(1, GetOrderOfRequest(1));
  EXPECT_EQ(2, GetOrderOfRequest(2));
  EXPECT_EQ(5, GetOrderOfRequest(3));
  EXPECT_EQ(3, GetOrderOfRequest(4));
  EXPECT_EQ(kRequestNotFound, GetOrderOfRequest(5));  // Canceled request.
  EXPECT_EQ(4, GetOrderOfRequest(6));
  EXPECT_EQ(6, GetOrderOfRequest(7));

  // Make sure we test order of all requests made.
  EXPECT_EQ(kIndexOutOfBounds, GetOrderOfRequest(8));
}

class RequestSocketCallback : public CallbackRunner< Tuple1<int> > {
 public:
  RequestSocketCallback(ClientSocketHandle* handle,
                        TestConnectJobFactory* test_connect_job_factory,
                        TestConnectJob::JobType next_job_type)
      : handle_(handle),
        within_callback_(false),
        test_connect_job_factory_(test_connect_job_factory),
        next_job_type_(next_job_type) {}

  virtual void RunWithParams(const Tuple1<int>& params) {
    callback_.RunWithParams(params);
    ASSERT_EQ(OK, params.a);

    if (!within_callback_) {
      test_connect_job_factory_->set_job_type(next_job_type_);
      handle_->Reset();
      within_callback_ = true;
      int rv = handle_->Init(
          "a", HostResolver::RequestInfo("www.google.com", 80),
          kDefaultPriority, this);
      switch (next_job_type_) {
        case TestConnectJob::kMockJob:
          EXPECT_EQ(OK, rv);
          break;
        case TestConnectJob::kMockPendingJob:
          EXPECT_EQ(ERR_IO_PENDING, rv);
          break;
        default:
          FAIL() << "Unexpected job type: " << next_job_type_;
          break;
      }
    }
  }

  int WaitForResult() {
    return callback_.WaitForResult();
  }

 private:
  ClientSocketHandle* const handle_;
  bool within_callback_;
  TestConnectJobFactory* const test_connect_job_factory_;
  TestConnectJob::JobType next_job_type_;
  TestCompletionCallback callback_;
};

TEST_F(ClientSocketPoolBaseTest, RequestPendingJobTwice) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingJob);
  ClientSocketHandle handle(pool_.get());
  RequestSocketCallback callback(
      &handle, connect_job_factory_, TestConnectJob::kMockPendingJob);
  int rv = handle.Init(
      "a", ignored_request_info_, kDefaultPriority, &callback);
  ASSERT_EQ(ERR_IO_PENDING, rv);

  EXPECT_EQ(OK, callback.WaitForResult());
  handle.Reset();
}

TEST_F(ClientSocketPoolBaseTest, RequestPendingJobThenSynchronous) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingJob);
  ClientSocketHandle handle(pool_.get());
  RequestSocketCallback callback(
      &handle, connect_job_factory_, TestConnectJob::kMockJob);
  int rv = handle.Init(
      "a", ignored_request_info_, kDefaultPriority, &callback);
  ASSERT_EQ(ERR_IO_PENDING, rv);

  EXPECT_EQ(OK, callback.WaitForResult());
  handle.Reset();
}

// Make sure that pending requests get serviced after active requests get
// cancelled.
TEST_F(ClientSocketPoolBaseTest, CancelActiveRequestWithPendingRequests) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingJob);

  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", kDefaultPriority));

  // Now, kDefaultMaxSocketsPerGroup requests should be active.
  // Let's cancel them.
  for (int i = 0; i < kDefaultMaxSocketsPerGroup; ++i) {
    ASSERT_FALSE(requests_[i]->handle()->is_initialized());
    requests_[i]->handle()->Reset();
  }

  // Let's wait for the rest to complete now.
  for (size_t i = kDefaultMaxSocketsPerGroup; i < requests_.size(); ++i) {
    EXPECT_EQ(OK, requests_[i]->WaitForResult());
    requests_[i]->handle()->Reset();
  }

  EXPECT_EQ(requests_.size() - kDefaultMaxSocketsPerGroup, completion_count_);
}

// Make sure that pending requests get serviced after active requests fail.
TEST_F(ClientSocketPoolBaseTest, FailingActiveRequestWithPendingRequests) {
  const size_t kMaxSockets = 5;
  CreatePool(kMaxSockets, kDefaultMaxSocketsPerGroup);

  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingFailingJob);

  const size_t kNumberOfRequests = 2 * kDefaultMaxSocketsPerGroup + 1;
  ASSERT_LE(kNumberOfRequests, kMaxSockets);  // Otherwise the test will hang.

  // Queue up all the requests
  for (size_t i = 0; i < kNumberOfRequests; ++i)
    EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", kDefaultPriority));

  for (size_t i = 0; i < kNumberOfRequests; ++i)
    EXPECT_EQ(ERR_CONNECTION_FAILED, requests_[i]->WaitForResult());
}

TEST_F(ClientSocketPoolBaseTest, CancelActiveRequestThenRequestSocket) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingJob);

  TestSocketRequest req(pool_.get(), &request_order_, &completion_count_);
  int rv = req.handle()->Init(
      "a", ignored_request_info_, kDefaultPriority, &req);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // Cancel the active request.
  req.handle()->Reset();

  rv = req.handle()->Init("a", ignored_request_info_, kDefaultPriority, &req);
  EXPECT_EQ(ERR_IO_PENDING, rv);
  EXPECT_EQ(OK, req.WaitForResult());

  EXPECT_FALSE(req.handle()->is_reused());
  EXPECT_EQ(1U, completion_count_);
  EXPECT_EQ(2, client_socket_factory_.allocation_count());
}

// A pending asynchronous job completes, which will free up a socket slot.  The
// next job finishes synchronously.  The callback for the asynchronous job
// should be first though.
TEST_F(ClientSocketPoolBaseTest, PendingJobCompletionOrder) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  // First two jobs are async.
  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingFailingJob);

  // Start job 1 (async error).
  TestSocketRequest req1(pool_.get(), &request_order_, &completion_count_);
  int rv = req1.handle()->Init("a", ignored_request_info_,
                               kDefaultPriority, &req1);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // Start job 2 (async error).
  TestSocketRequest req2(pool_.get(), &request_order_, &completion_count_);
  rv = req2.handle()->Init("a", ignored_request_info_, kDefaultPriority, &req2);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // The pending job is sync.
  connect_job_factory_->set_job_type(TestConnectJob::kMockJob);

  // Request 3 does not have a ConnectJob yet.  It's just pending.
  TestSocketRequest req3(pool_.get(), &request_order_, &completion_count_);
  rv = req3.handle()->Init("a", ignored_request_info_, kDefaultPriority, &req3);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  EXPECT_EQ(ERR_CONNECTION_FAILED, req1.WaitForResult());
  EXPECT_EQ(ERR_CONNECTION_FAILED, req2.WaitForResult());
  EXPECT_EQ(OK, req3.WaitForResult());

  ASSERT_EQ(3U, request_order_.size());

  // After job 1 finishes unsuccessfully, it will try to process the pending
  // requests queue, so it starts up job 3 for request 3.  This job
  // synchronously succeeds, so the request order is 1, 3, 2.
  EXPECT_EQ(&req1, request_order_[0]);
  EXPECT_EQ(&req2, request_order_[2]);
  EXPECT_EQ(&req3, request_order_[1]);
}

// When a ConnectJob is coupled to a request, even if a free socket becomes
// available, the request will be serviced by the ConnectJob.
TEST_F(ClientSocketPoolBaseTest, ReleaseSockets) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);
  ClientSocketPoolBase::EnableLateBindingOfSockets(false);

  // Start job 1 (async OK)
  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingJob);

  TestSocketRequest req1(pool_.get(), &request_order_, &completion_count_);
  int rv = req1.handle()->Init("a", ignored_request_info_, kDefaultPriority,
                               &req1);
  EXPECT_EQ(ERR_IO_PENDING, rv);
  EXPECT_EQ(OK, req1.WaitForResult());

  // Job 1 finished OK.  Start job 2 (also async OK).  Release socket 1.
  connect_job_factory_->set_job_type(TestConnectJob::kMockWaitingJob);

  TestSocketRequest req2(pool_.get(), &request_order_, &completion_count_);
  rv = req2.handle()->Init("a", ignored_request_info_, kDefaultPriority, &req2);
  EXPECT_EQ(ERR_IO_PENDING, rv);
  req1.handle()->Reset();
  MessageLoop::current()->RunAllPending();  // Run the DoReleaseSocket()

  // Job 2 is pending. Start request 3 (which has no associated job since it
  // will use the idle socket).

  TestSocketRequest req3(pool_.get(), &request_order_, &completion_count_);
  rv = req3.handle()->Init("a", ignored_request_info_, kDefaultPriority, &req3);
  EXPECT_EQ(OK, rv);

  EXPECT_FALSE(req2.handle()->socket());
  client_socket_factory_.SignalJobs();
  EXPECT_EQ(OK, req2.WaitForResult());

  ASSERT_EQ(2U, request_order_.size());
  EXPECT_EQ(&req1, request_order_[0]);
  EXPECT_EQ(&req2, request_order_[1]);
  EXPECT_EQ(0, pool_->IdleSocketCountInGroup("a"));
}

class ClientSocketPoolBaseTest_LateBinding : public ClientSocketPoolBaseTest {
 protected:
  virtual void SetUp() {
    ClientSocketPoolBaseTest::SetUp();
    ClientSocketPoolBase::EnableLateBindingOfSockets(true);
  }
};

TEST_F(ClientSocketPoolBaseTest_LateBinding, BasicSynchronous) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  TestCompletionCallback callback;
  ClientSocketHandle handle(pool_.get());
  EXPECT_EQ(OK, handle.Init("a", ignored_request_info_, kDefaultPriority,
                            &callback));
  EXPECT_TRUE(handle.is_initialized());
  EXPECT_TRUE(handle.socket());
  handle.Reset();
}

TEST_F(ClientSocketPoolBaseTest_LateBinding, BasicAsynchronous) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingJob);
  TestSocketRequest req(pool_.get(), &request_order_, &completion_count_);
  int rv = req.handle()->Init("a", ignored_request_info_, 0, &req);
  EXPECT_EQ(ERR_IO_PENDING, rv);
  EXPECT_EQ(LOAD_STATE_CONNECTING, pool_->GetLoadState("a", req.handle()));
  EXPECT_EQ(OK, req.WaitForResult());
  EXPECT_TRUE(req.handle()->is_initialized());
  EXPECT_TRUE(req.handle()->socket());
  req.handle()->Reset();
}

TEST_F(ClientSocketPoolBaseTest_LateBinding, InitConnectionFailure) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  connect_job_factory_->set_job_type(TestConnectJob::kMockFailingJob);
  TestSocketRequest req(pool_.get(), &request_order_, &completion_count_);
  EXPECT_EQ(ERR_CONNECTION_FAILED,
            req.handle()->Init("a", ignored_request_info_,
                               kDefaultPriority, &req));
}

TEST_F(ClientSocketPoolBaseTest_LateBinding,
       InitConnectionAsynchronousFailure) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingFailingJob);
  TestSocketRequest req(pool_.get(), &request_order_, &completion_count_);
  EXPECT_EQ(ERR_IO_PENDING,
            req.handle()->Init("a", ignored_request_info_, kDefaultPriority,
                               &req));
  EXPECT_EQ(LOAD_STATE_CONNECTING, pool_->GetLoadState("a", req.handle()));
  EXPECT_EQ(ERR_CONNECTION_FAILED, req.WaitForResult());
}

TEST_F(ClientSocketPoolBaseTest_LateBinding, PendingRequests) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  EXPECT_EQ(OK, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(OK, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 1));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 3));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 4));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 2));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 1));

  ReleaseAllConnections(KEEP_ALIVE);

  EXPECT_EQ(kDefaultMaxSocketsPerGroup,
            client_socket_factory_.allocation_count());
  EXPECT_EQ(requests_.size() - kDefaultMaxSocketsPerGroup, completion_count_);

  EXPECT_EQ(1, GetOrderOfRequest(1));
  EXPECT_EQ(2, GetOrderOfRequest(2));
  EXPECT_EQ(6, GetOrderOfRequest(3));
  EXPECT_EQ(4, GetOrderOfRequest(4));
  EXPECT_EQ(3, GetOrderOfRequest(5));
  EXPECT_EQ(5, GetOrderOfRequest(6));
  EXPECT_EQ(7, GetOrderOfRequest(7));

  // Make sure we test order of all requests made.
  EXPECT_EQ(kIndexOutOfBounds, GetOrderOfRequest(8));
}

TEST_F(ClientSocketPoolBaseTest_LateBinding, PendingRequests_NoKeepAlive) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  EXPECT_EQ(OK, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(OK, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 1));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 3));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 4));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 2));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 1));

  ReleaseAllConnections(NO_KEEP_ALIVE);

  for (size_t i = kDefaultMaxSocketsPerGroup; i < requests_.size(); ++i)
    EXPECT_EQ(OK, requests_[i]->WaitForResult());

  EXPECT_EQ(static_cast<int>(requests_.size()),
            client_socket_factory_.allocation_count());
  EXPECT_EQ(requests_.size() - kDefaultMaxSocketsPerGroup, completion_count_);
}

// This test will start up a RequestSocket() and then immediately Cancel() it.
// The pending connect job will be cancelled and should not call back into
// ClientSocketPoolBase.
TEST_F(ClientSocketPoolBaseTest_LateBinding, CancelRequestClearGroup) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingJob);
  TestSocketRequest req(pool_.get(), &request_order_, &completion_count_);
  EXPECT_EQ(ERR_IO_PENDING,
            req.handle()->Init("a", ignored_request_info_,
                               kDefaultPriority, &req));
  req.handle()->Reset();
}

TEST_F(ClientSocketPoolBaseTest_LateBinding, TwoRequestsCancelOne) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingJob);
  TestSocketRequest req(pool_.get(), &request_order_, &completion_count_);
  TestSocketRequest req2(pool_.get(), &request_order_, &completion_count_);

  EXPECT_EQ(ERR_IO_PENDING,
            req.handle()->Init("a", ignored_request_info_,
                               kDefaultPriority, &req));
  EXPECT_EQ(ERR_IO_PENDING,
            req2.handle()->Init("a", ignored_request_info_,
                                kDefaultPriority, &req2));

  req.handle()->Reset();

  EXPECT_EQ(OK, req2.WaitForResult());
  req2.handle()->Reset();
}

TEST_F(ClientSocketPoolBaseTest_LateBinding, ConnectCancelConnect) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingJob);
  ClientSocketHandle handle(pool_.get());
  TestCompletionCallback callback;
  TestSocketRequest req(pool_.get(), &request_order_, &completion_count_);

  EXPECT_EQ(ERR_IO_PENDING,
            handle.Init("a", ignored_request_info_,
                        kDefaultPriority, &callback));

  handle.Reset();

  TestCompletionCallback callback2;
  EXPECT_EQ(ERR_IO_PENDING,
            handle.Init("a", ignored_request_info_,
                        kDefaultPriority, &callback2));

  EXPECT_EQ(OK, callback2.WaitForResult());
  EXPECT_FALSE(callback.have_result());

  handle.Reset();
}

TEST_F(ClientSocketPoolBaseTest_LateBinding, CancelRequest) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  EXPECT_EQ(OK, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(OK, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 1));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 3));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 4));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 2));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", 1));

  // Cancel a request.
  size_t index_to_cancel = kDefaultMaxSocketsPerGroup + 2;
  EXPECT_FALSE(requests_[index_to_cancel]->handle()->is_initialized());
  requests_[index_to_cancel]->handle()->Reset();

  ReleaseAllConnections(KEEP_ALIVE);

  EXPECT_EQ(kDefaultMaxSocketsPerGroup,
            client_socket_factory_.allocation_count());
  EXPECT_EQ(requests_.size() - kDefaultMaxSocketsPerGroup - 1,
            completion_count_);

  EXPECT_EQ(1, GetOrderOfRequest(1));
  EXPECT_EQ(2, GetOrderOfRequest(2));
  EXPECT_EQ(5, GetOrderOfRequest(3));
  EXPECT_EQ(3, GetOrderOfRequest(4));
  EXPECT_EQ(kRequestNotFound, GetOrderOfRequest(5));  // Canceled request.
  EXPECT_EQ(4, GetOrderOfRequest(6));
  EXPECT_EQ(6, GetOrderOfRequest(7));

  // Make sure we test order of all requests made.
  EXPECT_EQ(kIndexOutOfBounds, GetOrderOfRequest(8));
}

TEST_F(ClientSocketPoolBaseTest_LateBinding, RequestPendingJobTwice) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingJob);
  ClientSocketHandle handle(pool_.get());
  RequestSocketCallback callback(
      &handle, connect_job_factory_, TestConnectJob::kMockPendingJob);
  int rv = handle.Init(
      "a", ignored_request_info_, kDefaultPriority, &callback);
  ASSERT_EQ(ERR_IO_PENDING, rv);

  EXPECT_EQ(OK, callback.WaitForResult());
  handle.Reset();
}

TEST_F(ClientSocketPoolBaseTest_LateBinding, RequestPendingJobThenSynchronous) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingJob);
  ClientSocketHandle handle(pool_.get());
  RequestSocketCallback callback(
      &handle, connect_job_factory_, TestConnectJob::kMockJob);
  int rv = handle.Init(
      "a", ignored_request_info_, kDefaultPriority, &callback);
  ASSERT_EQ(ERR_IO_PENDING, rv);

  EXPECT_EQ(OK, callback.WaitForResult());
  handle.Reset();
}

// Make sure that pending requests get serviced after active requests get
// cancelled.
TEST_F(ClientSocketPoolBaseTest_LateBinding,
       CancelActiveRequestWithPendingRequests) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingJob);

  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", kDefaultPriority));
  EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", kDefaultPriority));

  // Now, kDefaultMaxSocketsPerGroup requests should be active.
  // Let's cancel them.
  for (int i = 0; i < kDefaultMaxSocketsPerGroup; ++i) {
    ASSERT_FALSE(requests_[i]->handle()->is_initialized());
    requests_[i]->handle()->Reset();
  }

  // Let's wait for the rest to complete now.
  for (size_t i = kDefaultMaxSocketsPerGroup; i < requests_.size(); ++i) {
    EXPECT_EQ(OK, requests_[i]->WaitForResult());
    requests_[i]->handle()->Reset();
  }

  EXPECT_EQ(requests_.size() - kDefaultMaxSocketsPerGroup, completion_count_);
}

// Make sure that pending requests get serviced after active requests fail.
TEST_F(ClientSocketPoolBaseTest_LateBinding,
       FailingActiveRequestWithPendingRequests) {
  const int kMaxSockets = 5;
  CreatePool(kMaxSockets, kDefaultMaxSocketsPerGroup);

  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingFailingJob);

  const int kNumberOfRequests = 2 * kDefaultMaxSocketsPerGroup + 1;
  ASSERT_LE(kNumberOfRequests, kMaxSockets);  // Otherwise the test hangs.

  // Queue up all the requests
  for (int i = 0; i < kNumberOfRequests; ++i)
    EXPECT_EQ(ERR_IO_PENDING, StartRequest("a", kDefaultPriority));

  for (int i = 0; i < kNumberOfRequests; ++i)
    EXPECT_EQ(ERR_CONNECTION_FAILED, requests_[i]->WaitForResult());
}

TEST_F(ClientSocketPoolBaseTest_LateBinding,
       CancelActiveRequestThenRequestSocket) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingJob);

  TestSocketRequest req(pool_.get(), &request_order_, &completion_count_);
  int rv = req.handle()->Init(
      "a", ignored_request_info_, kDefaultPriority, &req);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // Cancel the active request.
  req.handle()->Reset();

  rv = req.handle()->Init("a", ignored_request_info_, kDefaultPriority, &req);
  EXPECT_EQ(ERR_IO_PENDING, rv);
  EXPECT_EQ(OK, req.WaitForResult());

  EXPECT_FALSE(req.handle()->is_reused());
  EXPECT_EQ(1U, completion_count_);
  EXPECT_EQ(2, client_socket_factory_.allocation_count());
}

// When requests and ConnectJobs are not coupled, the request will get serviced
// by whatever comes first.
TEST_F(ClientSocketPoolBaseTest_LateBinding, ReleaseSockets) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);

  // Start job 1 (async OK)
  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingJob);

  TestSocketRequest req1(pool_.get(), &request_order_, &completion_count_);
  int rv = req1.handle()->Init("a", ignored_request_info_, kDefaultPriority,
                               &req1);
  EXPECT_EQ(ERR_IO_PENDING, rv);
  EXPECT_EQ(OK, req1.WaitForResult());

  // Job 1 finished OK.  Start job 2 (also async OK).  Request 3 is pending
  // without a job.
  connect_job_factory_->set_job_type(TestConnectJob::kMockWaitingJob);

  TestSocketRequest req2(pool_.get(), &request_order_, &completion_count_);
  rv = req2.handle()->Init("a", ignored_request_info_, kDefaultPriority, &req2);
  EXPECT_EQ(ERR_IO_PENDING, rv);
  TestSocketRequest req3(pool_.get(), &request_order_, &completion_count_);
  rv = req3.handle()->Init("a", ignored_request_info_, kDefaultPriority, &req3);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // Both Requests 2 and 3 are pending.  We release socket 1 which should
  // service request 2.  Request 3 should still be waiting.
  req1.handle()->Reset();
  MessageLoop::current()->RunAllPending();  // Run the DoReleaseSocket()
  ASSERT_TRUE(req2.handle()->socket());
  EXPECT_EQ(OK, req2.WaitForResult());
  EXPECT_FALSE(req3.handle()->socket());

  // Signal job 2, which should service request 3.

  client_socket_factory_.SignalJobs();
  EXPECT_EQ(OK, req3.WaitForResult());

  ASSERT_EQ(3U, request_order_.size());
  EXPECT_EQ(&req1, request_order_[0]);
  EXPECT_EQ(&req2, request_order_[1]);
  EXPECT_EQ(&req3, request_order_[2]);
  EXPECT_EQ(0, pool_->IdleSocketCountInGroup("a"));
}

// The requests are not coupled to the jobs.  So, the requests should finish in
// their priority / insertion order.
TEST_F(ClientSocketPoolBaseTest_LateBinding, PendingJobCompletionOrder) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);
  // First two jobs are async.
  connect_job_factory_->set_job_type(TestConnectJob::kMockPendingFailingJob);

  TestSocketRequest req1(pool_.get(), &request_order_, &completion_count_);
  int rv = req1.handle()->Init("a", ignored_request_info_, kDefaultPriority, &req1);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  TestSocketRequest req2(pool_.get(), &request_order_, &completion_count_);
  rv = req2.handle()->Init("a", ignored_request_info_, kDefaultPriority, &req2);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // The pending job is sync.
  connect_job_factory_->set_job_type(TestConnectJob::kMockJob);

  TestSocketRequest req3(pool_.get(), &request_order_, &completion_count_);
  rv = req3.handle()->Init("a", ignored_request_info_, kDefaultPriority, &req3);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  EXPECT_EQ(ERR_CONNECTION_FAILED, req1.WaitForResult());
  EXPECT_EQ(OK, req2.WaitForResult());
  EXPECT_EQ(ERR_CONNECTION_FAILED, req3.WaitForResult());

  ASSERT_EQ(3U, request_order_.size());
  EXPECT_EQ(&req1, request_order_[0]);
  EXPECT_EQ(&req2, request_order_[1]);
  EXPECT_EQ(&req3, request_order_[2]);
}

TEST_F(ClientSocketPoolBaseTest_LateBinding, DISABLED_LoadState) {
  CreatePool(kDefaultMaxSockets, kDefaultMaxSocketsPerGroup);
  connect_job_factory_->set_job_type(
      TestConnectJob::kMockAdvancingLoadStateJob);

  TestSocketRequest req1(pool_.get(), &request_order_, &completion_count_);
  int rv = req1.handle()->Init("a", ignored_request_info_, kDefaultPriority,
                               &req1);
  EXPECT_EQ(ERR_IO_PENDING, rv);
  EXPECT_EQ(LOAD_STATE_IDLE, req1.handle()->GetLoadState());

  MessageLoop::current()->RunAllPending();

  TestSocketRequest req2(pool_.get(), &request_order_, &completion_count_);
  rv = req2.handle()->Init("a", ignored_request_info_, kDefaultPriority, &req2);
  EXPECT_EQ(ERR_IO_PENDING, rv);
  EXPECT_EQ(LOAD_STATE_WAITING_FOR_CACHE, req1.handle()->GetLoadState());
  EXPECT_EQ(LOAD_STATE_WAITING_FOR_CACHE, req2.handle()->GetLoadState());
}

}  // namespace

}  // namespace net
