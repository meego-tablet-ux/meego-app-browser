// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_loop.h"
#include "base/scoped_temp_dir.h"
#include "chrome/browser/browser_thread.h"
#include "chrome/browser/net/gaia/token_service.h"
#include "chrome/browser/policy/configuration_policy_pref_store.h"
#include "chrome/browser/policy/device_management_policy_provider.h"
#include "chrome/browser/policy/mock_configuration_policy_store.h"
#include "chrome/browser/policy/mock_device_management_backend.h"
#include "chrome/common/net/gaia/gaia_constants.h"
#include "chrome/common/notification_service.h"
#include "chrome/common/policy_constants.h"
#include "chrome/test/mock_notification_observer.h"
#include "chrome/test/testing_device_token_fetcher.h"
#include "chrome/test/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"

const char kTestToken[] = "device_policy_provider_test_auth_token";

namespace policy {

using ::testing::_;
using ::testing::InSequence;
using ::testing::Mock;

class DeviceManagementPolicyProviderTest : public testing::Test {
 public:
  DeviceManagementPolicyProviderTest()
      : ui_thread_(BrowserThread::UI, &loop_),
        file_thread_(BrowserThread::FILE, &loop_) {}

  virtual ~DeviceManagementPolicyProviderTest() {}

  virtual void SetUp() {
    profile_.reset(new TestingProfile);
    CreateNewBackend();
    CreateNewProvider();
  }

  void CreateNewBackend() {
    backend_ = new MockDeviceManagementBackend;
  }

  void CreateNewProvider() {
    provider_.reset(new DeviceManagementPolicyProvider(
        ConfigurationPolicyPrefStore::GetChromePolicyDefinitionList(),
        backend_,
        profile_.get()));
    provider_->SetDeviceTokenFetcher(
        new TestingDeviceTokenFetcher(backend_,
                                      profile_.get(),
                                      provider_->GetTokenPath()));
    loop_.RunAllPending();
  }

  void SimulateSuccessfulLoginAndRunPending() {
    loop_.RunAllPending();
    profile_->GetTokenService()->IssueAuthTokenForTest(
        GaiaConstants::kDeviceManagementService, kTestToken);
    TestingDeviceTokenFetcher* fetcher =
        static_cast<TestingDeviceTokenFetcher*>(
            provider_->token_fetcher_.get());
    fetcher->SimulateLogin(kTestManagedDomainUsername);
    loop_.RunAllPending();
  }

  void SimulateSuccessfulInitialPolicyFetch() {
    MockConfigurationPolicyStore store;
    EXPECT_CALL(*backend_, ProcessRegisterRequest(_, _, _, _)).WillOnce(
        MockDeviceManagementBackendSucceedRegister());
    EXPECT_CALL(*backend_, ProcessPolicyRequest(_, _, _, _)).WillOnce(
        MockDeviceManagementBackendSucceedBooleanPolicy(
            key::kDisableSpdy, true));
    SimulateSuccessfulLoginAndRunPending();
    EXPECT_CALL(store, Apply(kPolicyDisableSpdy, _)).Times(1);
    provider_->Provide(&store);
    ASSERT_EQ(1U, store.policy_map().size());
    Mock::VerifyAndClearExpectations(backend_);
    Mock::VerifyAndClearExpectations(&store);
  }

  virtual void TearDown() {
    loop_.RunAllPending();
  }

  MockDeviceManagementBackend* backend_;  // weak
  scoped_ptr<DeviceManagementPolicyProvider> provider_;

 protected:
  void SetRefreshDelays(DeviceManagementPolicyProvider* provider,
                        int64 policy_refresh_rate_ms,
                        int64 policy_refresh_max_earlier_ms,
                        int64 policy_refresh_error_delay_ms,
                        int64 token_fetch_error_delay_ms) {
    provider->set_policy_refresh_rate_ms(policy_refresh_rate_ms);
    provider->set_policy_refresh_max_earlier_ms(policy_refresh_max_earlier_ms);
    provider->set_policy_refresh_error_delay_ms(policy_refresh_error_delay_ms);
    provider->set_token_fetch_error_delay_ms(token_fetch_error_delay_ms);
  }

 private:
  MessageLoop loop_;
  BrowserThread ui_thread_;
  BrowserThread file_thread_;
  scoped_ptr<Profile> profile_;

  DISALLOW_COPY_AND_ASSIGN(DeviceManagementPolicyProviderTest);
};

// If there's no login and no previously-fetched policy, the provider should
// provide an empty policy.
TEST_F(DeviceManagementPolicyProviderTest, InitialProvideNoLogin) {
  MockConfigurationPolicyStore store;
  EXPECT_CALL(store, Apply(_, _)).Times(0);
  provider_->Provide(&store);
  EXPECT_TRUE(store.policy_map().empty());
}

// If the login is successful and there's no previously-fetched policy, the
// policy should be fetched from the server and should be available the first
// time the Provide method is called.
TEST_F(DeviceManagementPolicyProviderTest, InitialProvideWithLogin) {
  SimulateSuccessfulInitialPolicyFetch();
}

// If the login succeed but the device management backend is unreachable,
// there should be no policy provided if there's no previously-fetched policy,
TEST_F(DeviceManagementPolicyProviderTest, EmptyProvideWithFailedBackend) {
  MockConfigurationPolicyStore store;
  EXPECT_CALL(*backend_, ProcessRegisterRequest(_, _, _, _)).WillOnce(
      MockDeviceManagementBackendFailRegister(
          DeviceManagementBackend::kErrorRequestFailed));
  EXPECT_CALL(*backend_, ProcessPolicyRequest(_, _, _, _)).Times(0);
  SimulateSuccessfulLoginAndRunPending();
  EXPECT_CALL(store, Apply(kPolicyDisableSpdy, _)).Times(0);
  provider_->Provide(&store);
  EXPECT_TRUE(store.policy_map().empty());
}

// If a policy has been fetched previously, if should be available even before
// the login succeeds or the device management backend is available.
TEST_F(DeviceManagementPolicyProviderTest, SecondProvide) {
  // Pre-fetch and persist a policy
  SimulateSuccessfulInitialPolicyFetch();

  // Simulate a app relaunch by constructing a new provider. Policy should be
  // refreshed (since that might be the purpose of the app relaunch).
  CreateNewBackend();
  EXPECT_CALL(*backend_, ProcessPolicyRequest(_, _, _, _)).WillOnce(
      MockDeviceManagementBackendSucceedBooleanPolicy(
          key::kDisableSpdy, true));
  CreateNewProvider();
  Mock::VerifyAndClearExpectations(backend_);

  // Simulate another app relaunch, this time against a failing backend.
  // Cached policy should still be available.
  CreateNewBackend();
  MockConfigurationPolicyStore store;
  EXPECT_CALL(*backend_, ProcessPolicyRequest(_, _, _, _)).WillOnce(
      MockDeviceManagementBackendFailPolicy(
          DeviceManagementBackend::kErrorRequestFailed));
  CreateNewProvider();
  SimulateSuccessfulLoginAndRunPending();
  EXPECT_CALL(store, Apply(kPolicyDisableSpdy, _)).Times(1);
  provider_->Provide(&store);
  ASSERT_EQ(1U, store.policy_map().size());
}

// When policy is successfully fetched from the device management server, it
// should force a policy refresh.
TEST_F(DeviceManagementPolicyProviderTest, FetchTriggersRefresh) {
  MockNotificationObserver observer;
  NotificationRegistrar registrar;
  registrar.Add(&observer,
                NotificationType::POLICY_CHANGED,
                NotificationService::AllSources());
  EXPECT_CALL(observer, Observe(_, _, _)).Times(1);
  SimulateSuccessfulInitialPolicyFetch();
}

TEST_F(DeviceManagementPolicyProviderTest, ErrorCausesNewRequest) {
  InSequence s;
  SetRefreshDelays(provider_.get(), 1000 * 1000, 0, 0, 0);
  EXPECT_CALL(*backend_, ProcessRegisterRequest(_, _, _, _)).WillOnce(
      MockDeviceManagementBackendFailRegister(
          DeviceManagementBackend::kErrorRequestFailed));
  EXPECT_CALL(*backend_, ProcessRegisterRequest(_, _, _, _)).WillOnce(
      MockDeviceManagementBackendSucceedRegister());
  EXPECT_CALL(*backend_, ProcessPolicyRequest(_, _, _, _)).WillOnce(
      MockDeviceManagementBackendFailPolicy(
          DeviceManagementBackend::kErrorRequestFailed));
  EXPECT_CALL(*backend_, ProcessPolicyRequest(_, _, _, _)).WillOnce(
      MockDeviceManagementBackendFailPolicy(
          DeviceManagementBackend::kErrorRequestFailed));
  EXPECT_CALL(*backend_, ProcessPolicyRequest(_, _, _, _)).WillOnce(
      MockDeviceManagementBackendSucceedBooleanPolicy(key::kDisableSpdy, true));
  SimulateSuccessfulLoginAndRunPending();
}

TEST_F(DeviceManagementPolicyProviderTest, RefreshPolicies) {
  InSequence s;
  SetRefreshDelays(provider_.get(), 0, 0, 1000 * 1000, 1000);
  EXPECT_CALL(*backend_, ProcessRegisterRequest(_, _, _, _)).WillOnce(
      MockDeviceManagementBackendSucceedRegister());
  EXPECT_CALL(*backend_, ProcessPolicyRequest(_, _, _, _)).WillOnce(
      MockDeviceManagementBackendSucceedBooleanPolicy(key::kDisableSpdy, true));
  EXPECT_CALL(*backend_, ProcessPolicyRequest(_, _, _, _)).WillOnce(
      MockDeviceManagementBackendSucceedBooleanPolicy(key::kDisableSpdy, true));
  EXPECT_CALL(*backend_, ProcessPolicyRequest(_, _, _, _)).WillOnce(
      MockDeviceManagementBackendSucceedBooleanPolicy(key::kDisableSpdy, true));
  EXPECT_CALL(*backend_, ProcessPolicyRequest(_, _, _, _)).WillOnce(
      MockDeviceManagementBackendFailPolicy(
          DeviceManagementBackend::kErrorRequestFailed));
  SimulateSuccessfulLoginAndRunPending();
}

// The client should try to re-register the device if the device server reports
// back that it doesn't recognize the device token on a policy request.
TEST_F(DeviceManagementPolicyProviderTest, DeviceNotFound) {
  InSequence s;
  EXPECT_CALL(*backend_, ProcessRegisterRequest(_, _, _, _)).WillOnce(
      MockDeviceManagementBackendSucceedRegister());
  EXPECT_CALL(*backend_, ProcessPolicyRequest(_, _, _, _)).WillOnce(
      MockDeviceManagementBackendFailPolicy(
          DeviceManagementBackend::kErrorServiceDeviceNotFound));
  EXPECT_CALL(*backend_, ProcessRegisterRequest(_, _, _, _)).WillOnce(
      MockDeviceManagementBackendSucceedRegister());
  EXPECT_CALL(*backend_, ProcessPolicyRequest(_, _, _, _)).WillOnce(
      MockDeviceManagementBackendSucceedBooleanPolicy(key::kDisableSpdy, true));
  SimulateSuccessfulLoginAndRunPending();
}

// The client should try to re-register the device if the device server reports
// back that the device token is invalid on a policy request.
TEST_F(DeviceManagementPolicyProviderTest, InvalidTokenOnPolicyRequest) {
  InSequence s;
  EXPECT_CALL(*backend_, ProcessRegisterRequest(_, _, _, _)).WillOnce(
      MockDeviceManagementBackendSucceedRegister());
  EXPECT_CALL(*backend_, ProcessPolicyRequest(_, _, _, _)).WillOnce(
      MockDeviceManagementBackendFailPolicy(
          DeviceManagementBackend::kErrorServiceManagementTokenInvalid));
  EXPECT_CALL(*backend_, ProcessRegisterRequest(_, _, _, _)).WillOnce(
      MockDeviceManagementBackendSucceedRegister());
  EXPECT_CALL(*backend_, ProcessPolicyRequest(_, _, _, _)).WillOnce(
      MockDeviceManagementBackendSucceedBooleanPolicy(key::kDisableSpdy, true));
  SimulateSuccessfulLoginAndRunPending();
}

}  // namespace policy
