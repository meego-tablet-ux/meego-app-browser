// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/message_loop.h"
#include "chrome/browser/sync/notifier/chrome_invalidation_client.h"
#include "chrome/browser/sync/notifier/state_writer.h"
#include "chrome/browser/sync/syncable/model_type.h"
#include "jingle/notifier/base/fake_base_task.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace sync_notifier {

using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;

namespace {

const char kClientId[] = "client_id";
const char kClientInfo[] = "client_info";
const char kState[] = "state";

static const int64 kUnknownVersion =
    invalidation::InvalidationListener::UNKNOWN_OBJECT_VERSION;

class MockListener : public ChromeInvalidationClient::Listener {
 public:
  MOCK_METHOD2(OnInvalidate, void(syncable::ModelType,
                                  const std::string& payload));
  MOCK_METHOD0(OnInvalidateAll, void());
};

class MockStateWriter : public StateWriter {
 public:
  MOCK_METHOD1(WriteState, void(const std::string&));
};

class MockCallback {
 public:
  MOCK_METHOD0(Run, void());

  invalidation::Closure* MakeClosure() {
    return invalidation::NewPermanentCallback(this, &MockCallback::Run);
  }
};

}  // namespace

class ChromeInvalidationClientTest : public testing::Test {
 protected:
  virtual void SetUp() {
    client_.Start(kClientId, kClientInfo, kState,
                  &mock_listener_, &mock_state_writer_,
                  fake_base_task_.AsWeakPtr());
  }

  virtual void TearDown() {
    client_.Stop();
    message_loop_.RunAllPending();
  }

  // Simulates DoInformOutboundListener() from network-manager.cc.
  void SimulateInformOutboundListener() {
    // Explicitness hack here to work around broken callback
    // implementations.
    void (invalidation::NetworkCallback::*run_function)(
        invalidation::NetworkEndpoint* const&) =
        &invalidation::NetworkCallback::Run;

    client_.chrome_system_resources_.ScheduleOnListenerThread(
        invalidation::NewPermanentCallback(
            client_.handle_outbound_packet_callback_.get(), run_function,
            client_.invalidation_client_->network_endpoint()));
  }

  // |payload| can be NULL, but not |type_name|.
  void FireInvalidate(const char* type_name,
                      int64 version, const char* payload) {
    const invalidation::ObjectId object_id(
        invalidation::ObjectSource::CHROME_SYNC, type_name);
    std::string payload_tmp = payload ? payload : "";
    const invalidation::Invalidation invalidation(
        object_id, version, payload ? &payload_tmp : NULL, NULL);
    MockCallback mock_callback;
    EXPECT_CALL(mock_callback, Run());
    client_.Invalidate(invalidation, mock_callback.MakeClosure());
  }

  void FireInvalidateAll() {
    MockCallback mock_callback;
    EXPECT_CALL(mock_callback, Run());
    client_.InvalidateAll(mock_callback.MakeClosure());
  }

  MessageLoop message_loop_;
  StrictMock<MockListener> mock_listener_;
  StrictMock<MockStateWriter> mock_state_writer_;
  notifier::FakeBaseTask fake_base_task_;
  ChromeInvalidationClient client_;
};

TEST_F(ChromeInvalidationClientTest, InvalidateBadObjectId) {
  EXPECT_CALL(mock_listener_, OnInvalidateAll());
  FireInvalidate("bad", 1, NULL);
}

TEST_F(ChromeInvalidationClientTest, InvalidateNoPayload) {
  EXPECT_CALL(mock_listener_, OnInvalidate(syncable::BOOKMARKS, ""));
  FireInvalidate("BOOKMARK", 1, NULL);
}

TEST_F(ChromeInvalidationClientTest, InvalidateWithPayload) {
  EXPECT_CALL(mock_listener_, OnInvalidate(syncable::PREFERENCES, "payload"));
  FireInvalidate("PREFERENCE", 1, "payload");
}

TEST_F(ChromeInvalidationClientTest, InvalidateVersion) {
  using ::testing::Mock;

  EXPECT_CALL(mock_listener_, OnInvalidate(syncable::APPS, ""));

  // Should trigger.
  FireInvalidate("APP", 1, NULL);

  Mock::VerifyAndClearExpectations(&mock_listener_);

  // Should be dropped.
  FireInvalidate("APP", 1, NULL);
}

TEST_F(ChromeInvalidationClientTest, InvalidateUnknownVersion) {
  EXPECT_CALL(mock_listener_, OnInvalidate(syncable::EXTENSIONS, ""))
      .Times(2);

  // Should trigger twice.
  FireInvalidate("EXTENSION", kUnknownVersion, NULL);
  FireInvalidate("EXTENSION", kUnknownVersion, NULL);
}

TEST_F(ChromeInvalidationClientTest, InvalidateVersionMultipleTypes) {
  using ::testing::Mock;

  EXPECT_CALL(mock_listener_, OnInvalidate(syncable::APPS, ""));
  EXPECT_CALL(mock_listener_, OnInvalidate(syncable::EXTENSIONS, ""));

  // Should trigger both.
  FireInvalidate("APP", 3, NULL);
  FireInvalidate("EXTENSION", 2, NULL);

  Mock::VerifyAndClearExpectations(&mock_listener_);

  // Should both be dropped.
  FireInvalidate("APP", 1, NULL);
  FireInvalidate("EXTENSION", 1, NULL);

  Mock::VerifyAndClearExpectations(&mock_listener_);

  EXPECT_CALL(mock_listener_, OnInvalidate(syncable::PREFERENCES, ""));
  EXPECT_CALL(mock_listener_, OnInvalidate(syncable::EXTENSIONS, ""));
  EXPECT_CALL(mock_listener_, OnInvalidate(syncable::APPS, ""));

  // Should trigger all three.
  FireInvalidate("PREFERENCE", 5, NULL);
  FireInvalidate("EXTENSION", 3, NULL);
  FireInvalidate("APP", 4, NULL);
}

TEST_F(ChromeInvalidationClientTest, InvalidateAll) {
  EXPECT_CALL(mock_listener_, OnInvalidateAll());
  FireInvalidateAll();
}

// Outbound packet sending should be resilient to
// changing/disappearing base tasks.
TEST_F(ChromeInvalidationClientTest, OutboundPackets) {
  SimulateInformOutboundListener();

  notifier::FakeBaseTask fake_base_task;
  client_.ChangeBaseTask(fake_base_task.AsWeakPtr());

  SimulateInformOutboundListener();

  {
    notifier::FakeBaseTask fake_base_task2;
    client_.ChangeBaseTask(fake_base_task2.AsWeakPtr());
  }

  SimulateInformOutboundListener();
}

// TODO(akalin): Flesh out unit tests.

}  // namespace sync_notifier
