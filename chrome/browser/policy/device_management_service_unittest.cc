// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/message_loop.h"
#include "base/string_split.h"
#include "chrome/browser/browser_thread.h"
#include "chrome/browser/policy/device_management_backend_impl.h"
#include "chrome/browser/policy/device_management_backend_mock.h"
#include "chrome/browser/policy/device_management_service.h"
#include "chrome/browser/policy/proto/device_management_constants.h"
#include "chrome/common/net/test_url_fetcher_factory.h"
#include "chrome/test/test_url_request_context_getter.h"
#include "net/base/escape.h"
#include "net/url_request/url_request_status.h"
#include "net/url_request/url_request_unittest.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gmock/include/gmock/gmock.h"

using testing::_;

namespace policy {

const char kServiceUrl[] = "https://example.com/management_service";

// Encoded error response messages for testing the error code paths.
const char kResponseEmpty[] = "\x08\x00";
const char kResponseErrorManagementNotSupported[] = "\x08\x01";
const char kResponseErrorDeviceNotFound[] = "\x08\x02";
const char kResponseErrorManagementTokenInvalid[] = "\x08\x03";
const char kResponseErrorActivationPending[] = "\x08\x04";

#define PROTO_STRING(name) (std::string(name, arraysize(name) - 1))

// Some helper constants.
const char kAuthToken[] = "auth-token";
const char kDMToken[] = "device-management-token";
const char kDeviceId[] = "device-id";

// Unit tests for the device management policy service. The tests are run
// against a TestURLFetcherFactory that is used to short-circuit the request
// without calling into the actual network stack.
template<typename TESTBASE>
class DeviceManagementServiceTestBase : public TESTBASE {
 protected:
  DeviceManagementServiceTestBase()
      : request_context_(new TestURLRequestContextGetter()),
        io_thread_(BrowserThread::IO, &loop_) {
    ResetService();
    service_->Initialize(request_context_.get());
  }

  virtual void SetUp() {
    URLFetcher::set_factory(&factory_);
  }

  virtual void TearDown() {
    URLFetcher::set_factory(NULL);
    backend_.reset();
    service_.reset();
    request_context_ = NULL;
    loop_.RunAllPending();
  }

  void ResetService() {
    backend_.reset();
    service_.reset(new DeviceManagementService(kServiceUrl));
    backend_.reset(service_->CreateBackend());
  }

  TestURLFetcherFactory factory_;
  scoped_refptr<TestURLRequestContextGetter> request_context_;
  scoped_ptr<DeviceManagementService> service_;
  scoped_ptr<DeviceManagementBackend> backend_;

 private:
  MessageLoopForUI loop_;
  BrowserThread io_thread_;
};

struct FailedRequestParams {
  FailedRequestParams(DeviceManagementBackend::ErrorCode expected_error,
                      URLRequestStatus::Status request_status,
                      int http_status,
                      const std::string& response)
      : expected_error_(expected_error),
        request_status_(request_status, 0),
        http_status_(http_status),
        response_(response) {}

  DeviceManagementBackend::ErrorCode expected_error_;
  URLRequestStatus request_status_;
  int http_status_;
  std::string response_;
};

// A parameterized test case for erroneous response situations, they're mostly
// the same for all kinds of requests.
class DeviceManagementServiceFailedRequestTest
    : public DeviceManagementServiceTestBase<
          testing::TestWithParam<FailedRequestParams> > {
};

TEST_P(DeviceManagementServiceFailedRequestTest, RegisterRequest) {
  DeviceRegisterResponseDelegateMock mock;
  EXPECT_CALL(mock, OnError(GetParam().expected_error_));
  em::DeviceRegisterRequest request;
  backend_->ProcessRegisterRequest(kAuthToken, kDeviceId, request, &mock);
  TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);

  fetcher->delegate()->OnURLFetchComplete(fetcher,
                                          GURL(kServiceUrl),
                                          GetParam().request_status_,
                                          GetParam().http_status_,
                                          ResponseCookies(),
                                          GetParam().response_);
}

TEST_P(DeviceManagementServiceFailedRequestTest, UnregisterRequest) {
  DeviceUnregisterResponseDelegateMock mock;
  EXPECT_CALL(mock, OnError(GetParam().expected_error_));
  em::DeviceUnregisterRequest request;
  backend_->ProcessUnregisterRequest(kDMToken, kDeviceId, request, &mock);
  TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);

  fetcher->delegate()->OnURLFetchComplete(fetcher,
                                          GURL(kServiceUrl),
                                          GetParam().request_status_,
                                          GetParam().http_status_,
                                          ResponseCookies(),
                                          GetParam().response_);
}

TEST_P(DeviceManagementServiceFailedRequestTest, PolicyRequest) {
  DevicePolicyResponseDelegateMock mock;
  EXPECT_CALL(mock, OnError(GetParam().expected_error_));
  em::DevicePolicyRequest request;
  request.set_policy_scope(kChromePolicyScope);
  em::DevicePolicySettingRequest* setting_request =
      request.add_setting_request();
  setting_request->set_key(kChromeDevicePolicySettingKey);
  backend_->ProcessPolicyRequest(kDMToken, kDeviceId, request, &mock);
  TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);

  fetcher->delegate()->OnURLFetchComplete(fetcher,
                                          GURL(kServiceUrl),
                                          GetParam().request_status_,
                                          GetParam().http_status_,
                                          ResponseCookies(),
                                          GetParam().response_);
}

INSTANTIATE_TEST_CASE_P(
    DeviceManagementServiceFailedRequestTestInstance,
    DeviceManagementServiceFailedRequestTest,
    testing::Values(
        FailedRequestParams(
            DeviceManagementBackend::kErrorRequestFailed,
            URLRequestStatus::FAILED,
            200,
            PROTO_STRING(kResponseEmpty)),
        FailedRequestParams(
            DeviceManagementBackend::kErrorHttpStatus,
            URLRequestStatus::SUCCESS,
            500,
            PROTO_STRING(kResponseEmpty)),
        FailedRequestParams(
            DeviceManagementBackend::kErrorResponseDecoding,
            URLRequestStatus::SUCCESS,
            200,
            PROTO_STRING("Not a protobuf.")),
        FailedRequestParams(
            DeviceManagementBackend::kErrorServiceManagementNotSupported,
            URLRequestStatus::SUCCESS,
            200,
            PROTO_STRING(kResponseErrorManagementNotSupported)),
        FailedRequestParams(
            DeviceManagementBackend::kErrorServiceDeviceNotFound,
            URLRequestStatus::SUCCESS,
            200,
            PROTO_STRING(kResponseErrorDeviceNotFound)),
        FailedRequestParams(
            DeviceManagementBackend::kErrorServiceManagementTokenInvalid,
            URLRequestStatus::SUCCESS,
            200,
            PROTO_STRING(kResponseErrorManagementTokenInvalid)),
        FailedRequestParams(
            DeviceManagementBackend::kErrorServiceActivationPending,
            URLRequestStatus::SUCCESS,
            200,
            PROTO_STRING(kResponseErrorActivationPending))));

// Simple query parameter parser for testing.
class QueryParams {
 public:
  explicit QueryParams(const std::string& query) {
    base::SplitStringIntoKeyValuePairs(query, '=', '&', &params_);
  }

  bool Check(const std::string& name, const std::string& expected_value) {
    bool found = false;
    for (ParamMap::const_iterator i(params_.begin()); i != params_.end(); ++i) {
      std::string unescaped_name(
          UnescapeURLComponent(i->first,
                               UnescapeRule::NORMAL |
                               UnescapeRule::SPACES |
                               UnescapeRule::URL_SPECIAL_CHARS |
                               UnescapeRule::CONTROL_CHARS |
                               UnescapeRule::REPLACE_PLUS_WITH_SPACE));
      if (unescaped_name == name) {
        if (found)
          return false;
        found = true;
        std::string unescaped_value(
            UnescapeURLComponent(i->second,
                                 UnescapeRule::NORMAL |
                                 UnescapeRule::SPACES |
                                 UnescapeRule::URL_SPECIAL_CHARS |
                                 UnescapeRule::CONTROL_CHARS |
                                 UnescapeRule::REPLACE_PLUS_WITH_SPACE));
        if (unescaped_value != expected_value)
          return false;
      }
    }
    return found;
  }

 private:
  typedef std::vector<std::pair<std::string, std::string> > ParamMap;
  ParamMap params_;
};

class DeviceManagementServiceTest
    : public DeviceManagementServiceTestBase<testing::Test> {
 protected:
  void CheckURLAndQueryParams(const GURL& request_url,
                              const std::string& request_type,
                              const std::string& device_id) {
    const GURL service_url(kServiceUrl);
    EXPECT_EQ(service_url.scheme(), request_url.scheme());
    EXPECT_EQ(service_url.host(), request_url.host());
    EXPECT_EQ(service_url.port(), request_url.port());
    EXPECT_EQ(service_url.path(), request_url.path());

    QueryParams query_params(request_url.query());
    EXPECT_TRUE(query_params.Check(
        DeviceManagementBackendImpl::kParamRequest, request_type));
    EXPECT_TRUE(query_params.Check(
        DeviceManagementBackendImpl::kParamDeviceID, device_id));
    EXPECT_TRUE(query_params.Check(
        DeviceManagementBackendImpl::kParamDeviceType,
        DeviceManagementBackendImpl::kValueDeviceType));
    EXPECT_TRUE(query_params.Check(
        DeviceManagementBackendImpl::kParamAppType,
        DeviceManagementBackendImpl::kValueAppType));
    EXPECT_TRUE(query_params.Check(
        DeviceManagementBackendImpl::kParamAgent,
        DeviceManagementBackendImpl::GetAgentString()));
  }
};

MATCHER_P(MessageEquals, reference, "") {
  std::string reference_data;
  std::string arg_data;
  return arg.SerializeToString(&arg_data) &&
         reference.SerializeToString(&reference_data) &&
         arg_data == reference_data;
}

TEST_F(DeviceManagementServiceTest, RegisterRequest) {
  DeviceRegisterResponseDelegateMock mock;
  em::DeviceRegisterResponse expected_response;
  expected_response.set_device_management_token(kDMToken);
  EXPECT_CALL(mock, HandleRegisterResponse(MessageEquals(expected_response)));
  em::DeviceRegisterRequest request;
  backend_->ProcessRegisterRequest(kDMToken, kDeviceId, request, &mock);
  TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);

  CheckURLAndQueryParams(fetcher->original_url(),
                         DeviceManagementBackendImpl::kValueRequestRegister,
                         kDeviceId);

  em::DeviceManagementRequest expected_request_wrapper;
  expected_request_wrapper.mutable_register_request()->CopyFrom(request);
  std::string expected_request_data;
  ASSERT_TRUE(expected_request_wrapper.SerializeToString(
      &expected_request_data));
  EXPECT_EQ(expected_request_data, fetcher->upload_data());

  // Generate the response.
  std::string response_data;
  em::DeviceManagementResponse response_wrapper;
  response_wrapper.set_error(em::DeviceManagementResponse::SUCCESS);
  response_wrapper.mutable_register_response()->CopyFrom(expected_response);
  ASSERT_TRUE(response_wrapper.SerializeToString(&response_data));
  URLRequestStatus status(URLRequestStatus::SUCCESS, 0);
  fetcher->delegate()->OnURLFetchComplete(fetcher,
                                          GURL(kServiceUrl),
                                          status,
                                          200,
                                          ResponseCookies(),
                                          response_data);
}

TEST_F(DeviceManagementServiceTest, UnregisterRequest) {
  DeviceUnregisterResponseDelegateMock mock;
  em::DeviceUnregisterResponse expected_response;
  EXPECT_CALL(mock, HandleUnregisterResponse(MessageEquals(expected_response)));
  em::DeviceUnregisterRequest request;
  backend_->ProcessUnregisterRequest(kDMToken, kDeviceId, request, &mock);
  TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);

  // Check the data the fetcher received.
  const GURL& request_url(fetcher->original_url());
  const GURL service_url(kServiceUrl);
  EXPECT_EQ(service_url.scheme(), request_url.scheme());
  EXPECT_EQ(service_url.host(), request_url.host());
  EXPECT_EQ(service_url.port(), request_url.port());
  EXPECT_EQ(service_url.path(), request_url.path());

  CheckURLAndQueryParams(fetcher->original_url(),
                         DeviceManagementBackendImpl::kValueRequestUnregister,
                         kDeviceId);

  em::DeviceManagementRequest expected_request_wrapper;
  expected_request_wrapper.mutable_unregister_request()->CopyFrom(request);
  std::string expected_request_data;
  ASSERT_TRUE(expected_request_wrapper.SerializeToString(
      &expected_request_data));
  EXPECT_EQ(expected_request_data, fetcher->upload_data());

  // Generate the response.
  std::string response_data;
  em::DeviceManagementResponse response_wrapper;
  response_wrapper.set_error(em::DeviceManagementResponse::SUCCESS);
  response_wrapper.mutable_unregister_response()->CopyFrom(expected_response);
  ASSERT_TRUE(response_wrapper.SerializeToString(&response_data));
  URLRequestStatus status(URLRequestStatus::SUCCESS, 0);
  fetcher->delegate()->OnURLFetchComplete(fetcher,
                                          GURL(kServiceUrl),
                                          status,
                                          200,
                                          ResponseCookies(),
                                          response_data);
}

TEST_F(DeviceManagementServiceTest, PolicyRequest) {
  DevicePolicyResponseDelegateMock mock;
  em::DevicePolicyResponse expected_response;
  em::DevicePolicySetting* policy_setting = expected_response.add_setting();
  policy_setting->set_policy_key(kChromeDevicePolicySettingKey);
  policy_setting->set_watermark("fresh");
  em::GenericSetting* policy_value = policy_setting->mutable_policy_value();
  em::GenericNamedValue* named_value = policy_value->add_named_value();
  named_value->set_name("HomepageLocation");
  named_value->mutable_value()->set_value_type(
      em::GenericValue::VALUE_TYPE_STRING);
  named_value->mutable_value()->set_string_value("http://www.chromium.org");
  named_value = policy_value->add_named_value();
  named_value->set_name("HomepageIsNewTabPage");
  named_value->mutable_value()->set_value_type(
      em::GenericValue::VALUE_TYPE_BOOL);
  named_value->mutable_value()->set_bool_value(false);
  EXPECT_CALL(mock, HandlePolicyResponse(MessageEquals(expected_response)));

  em::DevicePolicyRequest request;
  request.set_policy_scope(kChromePolicyScope);
  em::DevicePolicySettingRequest* setting_request =
      request.add_setting_request();
  setting_request->set_key(kChromeDevicePolicySettingKey);
  setting_request->set_watermark("stale");
  backend_->ProcessPolicyRequest(kDMToken, kDeviceId, request, &mock);
  TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);

  CheckURLAndQueryParams(fetcher->original_url(),
                         DeviceManagementBackendImpl::kValueRequestPolicy,
                         kDeviceId);

  em::DeviceManagementRequest expected_request_wrapper;
  expected_request_wrapper.mutable_policy_request()->CopyFrom(request);
  std::string expected_request_data;
  ASSERT_TRUE(expected_request_wrapper.SerializeToString(
      &expected_request_data));
  EXPECT_EQ(expected_request_data, fetcher->upload_data());

  // Generate the response.
  std::string response_data;
  em::DeviceManagementResponse response_wrapper;
  response_wrapper.set_error(em::DeviceManagementResponse::SUCCESS);
  response_wrapper.mutable_policy_response()->CopyFrom(expected_response);
  ASSERT_TRUE(response_wrapper.SerializeToString(&response_data));
  URLRequestStatus status(URLRequestStatus::SUCCESS, 0);
  fetcher->delegate()->OnURLFetchComplete(fetcher,
                                          GURL(kServiceUrl),
                                          status,
                                          200,
                                          ResponseCookies(),
                                          response_data);
}

TEST_F(DeviceManagementServiceTest, CancelRegisterRequest) {
  DeviceRegisterResponseDelegateMock mock;
  EXPECT_CALL(mock, HandleRegisterResponse(_)).Times(0);
  em::DeviceRegisterRequest request;
  backend_->ProcessRegisterRequest(kAuthToken, kDeviceId, request, &mock);
  TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);

  // There shouldn't be any callbacks.
  backend_.reset();
}

TEST_F(DeviceManagementServiceTest, CancelUnregisterRequest) {
  DeviceUnregisterResponseDelegateMock mock;
  EXPECT_CALL(mock, HandleUnregisterResponse(_)).Times(0);
  em::DeviceUnregisterRequest request;
  backend_->ProcessUnregisterRequest(kDMToken, kDeviceId, request, &mock);
  TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);

  // There shouldn't be any callbacks.
  backend_.reset();
}

TEST_F(DeviceManagementServiceTest, CancelPolicyRequest) {
  DevicePolicyResponseDelegateMock mock;
  EXPECT_CALL(mock, HandlePolicyResponse(_)).Times(0);
  em::DevicePolicyRequest request;
  request.set_policy_scope(kChromePolicyScope);
  em::DevicePolicySettingRequest* setting_request =
      request.add_setting_request();
  setting_request->set_key(kChromeDevicePolicySettingKey);
  setting_request->set_watermark("stale");
  backend_->ProcessPolicyRequest(kDMToken, kDeviceId, request, &mock);
  TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);

  // There shouldn't be any callbacks.
  backend_.reset();
}

TEST_F(DeviceManagementServiceTest, JobQueueing) {
  // Start with a non-initialized service.
  ResetService();

  // Make a request. We should not see any fetchers being created.
  DeviceRegisterResponseDelegateMock mock;
  em::DeviceRegisterResponse expected_response;
  expected_response.set_device_management_token(kDMToken);
  EXPECT_CALL(mock, HandleRegisterResponse(MessageEquals(expected_response)));
  em::DeviceRegisterRequest request;
  backend_->ProcessRegisterRequest(kAuthToken, kDeviceId, request, &mock);
  TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  ASSERT_FALSE(fetcher);

  // Now initialize the service. That should start the job.
  service_->Initialize(request_context_.get());
  fetcher = factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  factory_.RemoveFetcherFromMap(0);

  // Check that the request is processed as expected.
  std::string response_data;
  em::DeviceManagementResponse response_wrapper;
  response_wrapper.set_error(em::DeviceManagementResponse::SUCCESS);
  response_wrapper.mutable_register_response()->CopyFrom(expected_response);
  ASSERT_TRUE(response_wrapper.SerializeToString(&response_data));
  URLRequestStatus status(URLRequestStatus::SUCCESS, 0);
  fetcher->delegate()->OnURLFetchComplete(fetcher,
                                          GURL(kServiceUrl),
                                          status,
                                          200,
                                          ResponseCookies(),
                                          response_data);
}

}  // namespace policy
