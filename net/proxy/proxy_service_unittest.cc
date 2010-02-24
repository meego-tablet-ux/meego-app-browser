// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/proxy/proxy_service.h"

#include <vector>

#include "base/format_macros.h"
#include "base/logging.h"
#include "base/string_util.h"
#include "googleurl/src/gurl.h"
#include "net/base/load_log.h"
#include "net/base/load_log_unittest.h"
#include "net/base/mock_network_change_notifier.h"
#include "net/base/net_errors.h"
#include "net/base/test_completion_callback.h"
#include "net/proxy/mock_proxy_resolver.h"
#include "net/proxy/proxy_config_service.h"
#include "net/proxy/proxy_resolver.h"
#include "net/proxy/proxy_script_fetcher.h"
#include "testing/gtest/include/gtest/gtest.h"

// TODO(eroman): Write a test which exercises
//              ProxyService::SuspendAllPendingRequests().
namespace net {
namespace {

class MockProxyConfigService: public ProxyConfigService {
 public:
  MockProxyConfigService() {}  // Direct connect.
  explicit MockProxyConfigService(const ProxyConfig& pc) : config(pc) {}
  explicit MockProxyConfigService(const std::string& pac_url) {
    config.set_pac_url(GURL(pac_url));
  }

  virtual int GetProxyConfig(ProxyConfig* results) {
    *results = config;
    return OK;
  }

  ProxyConfig config;
};

}  // namespace

// A mock ProxyScriptFetcher. No result will be returned to the fetch client
// until we call NotifyFetchCompletion() to set the results.
class MockProxyScriptFetcher : public ProxyScriptFetcher {
 public:
  MockProxyScriptFetcher()
      : pending_request_callback_(NULL), pending_request_bytes_(NULL) {
  }

  // ProxyScriptFetcher implementation.
  virtual int Fetch(const GURL& url,
                    std::string* bytes,
                    CompletionCallback* callback) {
    DCHECK(!has_pending_request());

    // Save the caller's information, and have them wait.
    pending_request_url_ = url;
    pending_request_callback_ = callback;
    pending_request_bytes_ = bytes;
    return ERR_IO_PENDING;
  }

  void NotifyFetchCompletion(int result, const std::string& bytes) {
    DCHECK(has_pending_request());
    *pending_request_bytes_ = bytes;
    CompletionCallback* callback = pending_request_callback_;
    pending_request_callback_ = NULL;
    callback->Run(result);
  }

  virtual void Cancel() {}

  const GURL& pending_request_url() const {
    return pending_request_url_;
  }

  bool has_pending_request() const {
    return pending_request_callback_ != NULL;
  }

 private:
  GURL pending_request_url_;
  CompletionCallback* pending_request_callback_;
  std::string* pending_request_bytes_;
};

TEST(ProxyServiceTest, Direct) {
  MockAsyncProxyResolver* resolver = new MockAsyncProxyResolver;
  scoped_refptr<ProxyService> service(
      new ProxyService(new MockProxyConfigService, resolver, NULL));

  GURL url("http://www.google.com/");

  ProxyInfo info;
  TestCompletionCallback callback;
  scoped_refptr<LoadLog> log(new LoadLog(LoadLog::kUnbounded));
  int rv = service->ResolveProxy(url, &info, &callback, NULL, log);
  EXPECT_EQ(OK, rv);
  EXPECT_TRUE(resolver->pending_requests().empty());
  EXPECT_TRUE(NULL == service->init_proxy_resolver_log());

  EXPECT_TRUE(info.is_direct());

  // Check the LoadLog was filled correctly.
  EXPECT_EQ(5u, log->entries().size());
  EXPECT_TRUE(LogContainsBeginEvent(*log, 0, LoadLog::TYPE_PROXY_SERVICE));
  EXPECT_TRUE(LogContainsEndEvent(*log, 4, LoadLog::TYPE_PROXY_SERVICE));
}

TEST(ProxyServiceTest, PAC) {
  MockProxyConfigService* config_service =
      new MockProxyConfigService("http://foopy/proxy.pac");

  MockAsyncProxyResolver* resolver = new MockAsyncProxyResolver;

  scoped_refptr<ProxyService> service(
      new ProxyService(config_service, resolver, NULL));

  GURL url("http://www.google.com/");

  ProxyInfo info;
  TestCompletionCallback callback;
  scoped_refptr<LoadLog> log(new LoadLog(LoadLog::kUnbounded));
  int rv = service->ResolveProxy(url, &info, &callback, NULL, log);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  EXPECT_EQ(GURL("http://foopy/proxy.pac"),
            resolver->pending_set_pac_script_request()->pac_url());
  EXPECT_FALSE(NULL == service->init_proxy_resolver_log());
  resolver->pending_set_pac_script_request()->CompleteNow(OK);

  ASSERT_EQ(1u, resolver->pending_requests().size());
  EXPECT_EQ(url, resolver->pending_requests()[0]->url());

  // Set the result in proxy resolver.
  resolver->pending_requests()[0]->results()->UseNamedProxy("foopy");
  resolver->pending_requests()[0]->CompleteNow(OK);

  EXPECT_EQ(OK, callback.WaitForResult());
  EXPECT_FALSE(info.is_direct());
  EXPECT_EQ("foopy:80", info.proxy_server().ToURI());

  // Check the LoadLog was filled correctly.
  EXPECT_EQ(7u, log->entries().size());
  EXPECT_TRUE(LogContainsBeginEvent(*log, 0, LoadLog::TYPE_PROXY_SERVICE));
  EXPECT_TRUE(LogContainsBeginEvent(
      *log, 3, LoadLog::TYPE_PROXY_SERVICE_WAITING_FOR_INIT_PAC));
  EXPECT_TRUE(LogContainsEndEvent(
      *log, 4, LoadLog::TYPE_PROXY_SERVICE_WAITING_FOR_INIT_PAC));
  EXPECT_TRUE(LogContainsEndEvent(*log, 6, LoadLog::TYPE_PROXY_SERVICE));
}

// Test that the proxy resolver does not see the URL's username/password
// or its reference section.
TEST(ProxyServiceTest, PAC_NoIdentityOrHash) {
  MockProxyConfigService* config_service =
      new MockProxyConfigService("http://foopy/proxy.pac");

  MockAsyncProxyResolver* resolver = new MockAsyncProxyResolver;

  scoped_refptr<ProxyService> service(
      new ProxyService(config_service, resolver, NULL));

  GURL url("http://username:password@www.google.com/?ref#hash#hash");

  ProxyInfo info;
  TestCompletionCallback callback;
  int rv = service->ResolveProxy(url, &info, &callback, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  EXPECT_EQ(GURL("http://foopy/proxy.pac"),
            resolver->pending_set_pac_script_request()->pac_url());
  resolver->pending_set_pac_script_request()->CompleteNow(OK);

  ASSERT_EQ(1u, resolver->pending_requests().size());
  // The URL should have been simplified, stripping the username/password/hash.
  EXPECT_EQ(GURL("http://www.google.com/?ref"),
                 resolver->pending_requests()[0]->url());

  // We end here without ever completing the request -- destruction of
  // ProxyService will cancel the outstanding request.
}

TEST(ProxyServiceTest, PAC_FailoverWithoutDirect) {
  MockProxyConfigService* config_service =
      new MockProxyConfigService("http://foopy/proxy.pac");
  MockAsyncProxyResolver* resolver = new MockAsyncProxyResolver;

  scoped_refptr<ProxyService> service(
      new ProxyService(config_service, resolver, NULL));

  GURL url("http://www.google.com/");

  ProxyInfo info;
  TestCompletionCallback callback1;
  int rv = service->ResolveProxy(url, &info, &callback1, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  EXPECT_EQ(GURL("http://foopy/proxy.pac"),
            resolver->pending_set_pac_script_request()->pac_url());
  resolver->pending_set_pac_script_request()->CompleteNow(OK);

  ASSERT_EQ(1u, resolver->pending_requests().size());
  EXPECT_EQ(url, resolver->pending_requests()[0]->url());

  // Set the result in proxy resolver.
  resolver->pending_requests()[0]->results()->UseNamedProxy("foopy:8080");
  resolver->pending_requests()[0]->CompleteNow(OK);

  EXPECT_EQ(OK, callback1.WaitForResult());
  EXPECT_FALSE(info.is_direct());
  EXPECT_EQ("foopy:8080", info.proxy_server().ToURI());

  // Now, imagine that connecting to foopy:8080 fails: there is nothing
  // left to fallback to, since our proxy list was NOT terminated by
  // DIRECT.
  TestCompletionCallback callback2;
  rv = service->ReconsiderProxyAfterError(url, &info, &callback2, NULL, NULL);
  // ReconsiderProxyAfterError returns error indicating nothing left.
  EXPECT_EQ(ERR_FAILED, rv);
  EXPECT_TRUE(info.is_empty());
}

// The proxy list could potentially contain the DIRECT fallback choice
// in a location other than the very end of the list, and could even
// specify it multiple times.
//
// This is not a typical usage, but we will obey it.
// (If we wanted to disallow this type of input, the right place to
// enforce it would be in parsing the PAC result string).
//
// This test will use the PAC result string:
//
//   "DIRECT ; PROXY foobar:10 ; DIRECT ; PROXY foobar:20"
//
// For which we expect it to try DIRECT, then foobar:10, then DIRECT again,
// then foobar:20, and then give up and error.
//
// The important check of this test is to make sure that DIRECT is not somehow
// cached as being a bad proxy.
TEST(ProxyServiceTest, PAC_FailoverAfterDirect) {
  MockProxyConfigService* config_service =
      new MockProxyConfigService("http://foopy/proxy.pac");
  MockAsyncProxyResolver* resolver = new MockAsyncProxyResolver;

  scoped_refptr<ProxyService> service(
      new ProxyService(config_service, resolver, NULL));

  GURL url("http://www.google.com/");

  ProxyInfo info;
  TestCompletionCallback callback1;
  int rv = service->ResolveProxy(url, &info, &callback1, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  EXPECT_EQ(GURL("http://foopy/proxy.pac"),
            resolver->pending_set_pac_script_request()->pac_url());
  resolver->pending_set_pac_script_request()->CompleteNow(OK);

  ASSERT_EQ(1u, resolver->pending_requests().size());
  EXPECT_EQ(url, resolver->pending_requests()[0]->url());

  // Set the result in proxy resolver.
  resolver->pending_requests()[0]->results()->UsePacString(
      "DIRECT ; PROXY foobar:10 ; DIRECT ; PROXY foobar:20");
  resolver->pending_requests()[0]->CompleteNow(OK);

  EXPECT_EQ(OK, callback1.WaitForResult());
  EXPECT_TRUE(info.is_direct());

  // Fallback 1.
  TestCompletionCallback callback2;
  rv = service->ReconsiderProxyAfterError(url, &info, &callback2, NULL, NULL);
  EXPECT_EQ(OK, rv);
  EXPECT_FALSE(info.is_direct());
  EXPECT_EQ("foobar:10", info.proxy_server().ToURI());

  // Fallback 2.
  TestCompletionCallback callback3;
  rv = service->ReconsiderProxyAfterError(url, &info, &callback3, NULL, NULL);
  EXPECT_EQ(OK, rv);
  EXPECT_TRUE(info.is_direct());

  // Fallback 3.
  TestCompletionCallback callback4;
  rv = service->ReconsiderProxyAfterError(url, &info, &callback4, NULL, NULL);
  EXPECT_EQ(OK, rv);
  EXPECT_FALSE(info.is_direct());
  EXPECT_EQ("foobar:20", info.proxy_server().ToURI());

  // Fallback 4 -- Nothing to fall back to!
  TestCompletionCallback callback5;
  rv = service->ReconsiderProxyAfterError(url, &info, &callback5, NULL, NULL);
  EXPECT_EQ(ERR_FAILED, rv);
  EXPECT_TRUE(info.is_empty());
}

TEST(ProxyServiceTest, ProxyResolverFails) {
  // Test what happens when the ProxyResolver fails. The download and setting
  // of the PAC script have already succeeded, so this corresponds with a
  // javascript runtime error while calling FindProxyForURL().

  MockProxyConfigService* config_service =
      new MockProxyConfigService("http://foopy/proxy.pac");

  MockAsyncProxyResolver* resolver = new MockAsyncProxyResolver;

  scoped_refptr<ProxyService> service(
      new ProxyService(config_service, resolver, NULL));

  // Start first resolve request.
  GURL url("http://www.google.com/");
  ProxyInfo info;
  TestCompletionCallback callback1;
  int rv = service->ResolveProxy(url, &info, &callback1, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  EXPECT_EQ(GURL("http://foopy/proxy.pac"),
            resolver->pending_set_pac_script_request()->pac_url());
  resolver->pending_set_pac_script_request()->CompleteNow(OK);

  ASSERT_EQ(1u, resolver->pending_requests().size());
  EXPECT_EQ(url, resolver->pending_requests()[0]->url());

  // Fail the first resolve request in MockAsyncProxyResolver.
  resolver->pending_requests()[0]->CompleteNow(ERR_FAILED);

  // Although the proxy resolver failed the request, ProxyService implicitly
  // falls-back to DIRECT.
  EXPECT_EQ(OK, callback1.WaitForResult());
  EXPECT_TRUE(info.is_direct());

  // The second resolve request will try to run through the proxy resolver,
  // regardless of whether the first request failed in it.
  TestCompletionCallback callback2;
  rv = service->ResolveProxy(url, &info, &callback2, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  ASSERT_EQ(1u, resolver->pending_requests().size());
  EXPECT_EQ(url, resolver->pending_requests()[0]->url());

  // This time we will have the resolver succeed (perhaps the PAC script has
  // a dependency on the current time).
  resolver->pending_requests()[0]->results()->UseNamedProxy("foopy_valid:8080");
  resolver->pending_requests()[0]->CompleteNow(OK);

  EXPECT_EQ(OK, callback2.WaitForResult());
  EXPECT_FALSE(info.is_direct());
  EXPECT_EQ("foopy_valid:8080", info.proxy_server().ToURI());
}

TEST(ProxyServiceTest, ProxyFallback) {
  // Test what happens when we specify multiple proxy servers and some of them
  // are bad.

  MockProxyConfigService* config_service =
      new MockProxyConfigService("http://foopy/proxy.pac");

  MockAsyncProxyResolver* resolver = new MockAsyncProxyResolver;

  scoped_refptr<ProxyService> service(
      new ProxyService(config_service, resolver, NULL));

  GURL url("http://www.google.com/");

  // Get the proxy information.
  ProxyInfo info;
  TestCompletionCallback callback1;
  int rv = service->ResolveProxy(url, &info, &callback1, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  EXPECT_EQ(GURL("http://foopy/proxy.pac"),
            resolver->pending_set_pac_script_request()->pac_url());
  resolver->pending_set_pac_script_request()->CompleteNow(OK);

  ASSERT_EQ(1u, resolver->pending_requests().size());
  EXPECT_EQ(url, resolver->pending_requests()[0]->url());

  // Set the result in proxy resolver.
  resolver->pending_requests()[0]->results()->UseNamedProxy(
      "foopy1:8080;foopy2:9090");
  resolver->pending_requests()[0]->CompleteNow(OK);

  // The first item is valid.
  EXPECT_EQ(OK, callback1.WaitForResult());
  EXPECT_FALSE(info.is_direct());
  EXPECT_EQ("foopy1:8080", info.proxy_server().ToURI());

  // Fake an error on the proxy.
  TestCompletionCallback callback2;
  rv = service->ReconsiderProxyAfterError(url, &info, &callback2, NULL, NULL);
  EXPECT_EQ(OK, rv);

  // The second proxy should be specified.
  EXPECT_EQ("foopy2:9090", info.proxy_server().ToURI());

  TestCompletionCallback callback3;
  rv = service->ResolveProxy(url, &info, &callback3, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  ASSERT_EQ(1u, resolver->pending_requests().size());
  EXPECT_EQ(url, resolver->pending_requests()[0]->url());

  // Set the result in proxy resolver -- the second result is already known
  // to be bad, so we will not try to use it initially.
  resolver->pending_requests()[0]->results()->UseNamedProxy(
      "foopy3:7070;foopy1:8080;foopy2:9090");
  resolver->pending_requests()[0]->CompleteNow(OK);

  EXPECT_EQ(OK, callback3.WaitForResult());
  EXPECT_FALSE(info.is_direct());
  EXPECT_EQ("foopy3:7070", info.proxy_server().ToURI());

  // We fake another error. It should now try the third one.
  TestCompletionCallback callback4;
  rv = service->ReconsiderProxyAfterError(url, &info, &callback4, NULL, NULL);
  EXPECT_EQ(OK, rv);
  EXPECT_EQ("foopy2:9090", info.proxy_server().ToURI());

  // We fake another error. At this point we have tried all of the
  // proxy servers we thought were valid; next we try the proxy server
  // that was in our bad proxies map (foopy1:8080).
  TestCompletionCallback callback5;
  rv = service->ReconsiderProxyAfterError(url, &info, &callback5, NULL, NULL);
  EXPECT_EQ(OK, rv);
  EXPECT_EQ("foopy1:8080", info.proxy_server().ToURI());

  // Fake another error, the last proxy is gone, the list should now be empty,
  // so there is nothing left to try.
  TestCompletionCallback callback6;
  rv = service->ReconsiderProxyAfterError(url, &info, &callback6, NULL, NULL);
  EXPECT_EQ(ERR_FAILED, rv);
  EXPECT_FALSE(info.is_direct());
  EXPECT_TRUE(info.is_empty());

  // TODO(nsylvain): Test that the proxy can be retried after the delay.
}

// This test is similar to ProxyFallback, but this time we have an explicit
// fallback choice to DIRECT.
TEST(ProxyServiceTest, ProxyFallbackToDirect) {
  MockProxyConfigService* config_service =
      new MockProxyConfigService("http://foopy/proxy.pac");

  MockAsyncProxyResolver* resolver = new MockAsyncProxyResolver;

  scoped_refptr<ProxyService> service(
      new ProxyService(config_service, resolver, NULL));

  GURL url("http://www.google.com/");

  // Get the proxy information.
  ProxyInfo info;
  TestCompletionCallback callback1;
  int rv = service->ResolveProxy(url, &info, &callback1, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  EXPECT_EQ(GURL("http://foopy/proxy.pac"),
            resolver->pending_set_pac_script_request()->pac_url());
  resolver->pending_set_pac_script_request()->CompleteNow(OK);

  ASSERT_EQ(1u, resolver->pending_requests().size());
  EXPECT_EQ(url, resolver->pending_requests()[0]->url());

  // Set the result in proxy resolver.
  resolver->pending_requests()[0]->results()->UsePacString(
      "PROXY foopy1:8080; PROXY foopy2:9090; DIRECT");
  resolver->pending_requests()[0]->CompleteNow(OK);

  // Get the first result.
  EXPECT_EQ(OK, callback1.WaitForResult());
  EXPECT_FALSE(info.is_direct());
  EXPECT_EQ("foopy1:8080", info.proxy_server().ToURI());

  // Fake an error on the proxy.
  TestCompletionCallback callback2;
  rv = service->ReconsiderProxyAfterError(url, &info, &callback2, NULL, NULL);
  EXPECT_EQ(OK, rv);

  // Now we get back the second proxy.
  EXPECT_EQ("foopy2:9090", info.proxy_server().ToURI());

  // Fake an error on this proxy as well.
  TestCompletionCallback callback3;
  rv = service->ReconsiderProxyAfterError(url, &info, &callback3, NULL, NULL);
  EXPECT_EQ(OK, rv);

  // Finally, we get back DIRECT.
  EXPECT_TRUE(info.is_direct());

  // Now we tell the proxy service that even DIRECT failed.
  TestCompletionCallback callback4;
  rv = service->ReconsiderProxyAfterError(url, &info, &callback4, NULL, NULL);
  // There was nothing left to try after DIRECT, so we are out of
  // choices.
  EXPECT_EQ(ERR_FAILED, rv);
}

TEST(ProxyServiceTest, ProxyFallback_NewSettings) {
  // Test proxy failover when new settings are available.

  MockProxyConfigService* config_service =
      new MockProxyConfigService("http://foopy/proxy.pac");

  MockAsyncProxyResolver* resolver = new MockAsyncProxyResolver;

  scoped_refptr<ProxyService> service(
      new ProxyService(config_service, resolver, NULL));

  GURL url("http://www.google.com/");

  // Get the proxy information.
  ProxyInfo info;
  TestCompletionCallback callback1;
  int rv = service->ResolveProxy(url, &info, &callback1, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  EXPECT_EQ(GURL("http://foopy/proxy.pac"),
            resolver->pending_set_pac_script_request()->pac_url());
  resolver->pending_set_pac_script_request()->CompleteNow(OK);

  ASSERT_EQ(1u, resolver->pending_requests().size());
  EXPECT_EQ(url, resolver->pending_requests()[0]->url());

  // Set the result in proxy resolver.
  resolver->pending_requests()[0]->results()->UseNamedProxy(
      "foopy1:8080;foopy2:9090");
  resolver->pending_requests()[0]->CompleteNow(OK);

  // The first item is valid.
  EXPECT_EQ(OK, callback1.WaitForResult());
  EXPECT_FALSE(info.is_direct());
  EXPECT_EQ("foopy1:8080", info.proxy_server().ToURI());

  // Fake an error on the proxy, and also a new configuration on the proxy.
  config_service->config = ProxyConfig();
  config_service->config.set_pac_url(GURL("http://foopy-new/proxy.pac"));

  TestCompletionCallback callback2;
  rv = service->ReconsiderProxyAfterError(url, &info, &callback2, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  EXPECT_EQ(GURL("http://foopy-new/proxy.pac"),
            resolver->pending_set_pac_script_request()->pac_url());
  resolver->pending_set_pac_script_request()->CompleteNow(OK);

  ASSERT_EQ(1u, resolver->pending_requests().size());
  EXPECT_EQ(url, resolver->pending_requests()[0]->url());

  resolver->pending_requests()[0]->results()->UseNamedProxy(
      "foopy1:8080;foopy2:9090");
  resolver->pending_requests()[0]->CompleteNow(OK);

  // The first proxy is still there since the configuration changed.
  EXPECT_EQ(OK, callback2.WaitForResult());
  EXPECT_EQ("foopy1:8080", info.proxy_server().ToURI());

  // We fake another error. It should now ignore the first one.
  TestCompletionCallback callback3;
  rv = service->ReconsiderProxyAfterError(url, &info, &callback3, NULL, NULL);
  EXPECT_EQ(OK, rv);
  EXPECT_EQ("foopy2:9090", info.proxy_server().ToURI());

  // We simulate a new configuration.
  config_service->config = ProxyConfig();
  config_service->config.set_pac_url(GURL("http://foopy-new2/proxy.pac"));

  // We fake another error. It should go back to the first proxy.
  TestCompletionCallback callback4;
  rv = service->ReconsiderProxyAfterError(url, &info, &callback4, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  EXPECT_EQ(GURL("http://foopy-new2/proxy.pac"),
            resolver->pending_set_pac_script_request()->pac_url());
  resolver->pending_set_pac_script_request()->CompleteNow(OK);

  ASSERT_EQ(1u, resolver->pending_requests().size());
  EXPECT_EQ(url, resolver->pending_requests()[0]->url());

  resolver->pending_requests()[0]->results()->UseNamedProxy(
      "foopy1:8080;foopy2:9090");
  resolver->pending_requests()[0]->CompleteNow(OK);

  EXPECT_EQ(OK, callback4.WaitForResult());
  EXPECT_EQ("foopy1:8080", info.proxy_server().ToURI());
}

TEST(ProxyServiceTest, ProxyFallback_BadConfig) {
  // Test proxy failover when the configuration is bad.

  MockProxyConfigService* config_service =
      new MockProxyConfigService("http://foopy/proxy.pac");

  MockAsyncProxyResolver* resolver = new MockAsyncProxyResolver;

  scoped_refptr<ProxyService> service(
      new ProxyService(config_service, resolver, NULL));

  GURL url("http://www.google.com/");

  // Get the proxy information.
  ProxyInfo info;
  TestCompletionCallback callback1;
  int rv = service->ResolveProxy(url, &info, &callback1, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  EXPECT_EQ(GURL("http://foopy/proxy.pac"),
            resolver->pending_set_pac_script_request()->pac_url());
  resolver->pending_set_pac_script_request()->CompleteNow(OK);
  ASSERT_EQ(1u, resolver->pending_requests().size());
  EXPECT_EQ(url, resolver->pending_requests()[0]->url());

  resolver->pending_requests()[0]->results()->UseNamedProxy(
      "foopy1:8080;foopy2:9090");
  resolver->pending_requests()[0]->CompleteNow(OK);

  // The first item is valid.
  EXPECT_EQ(OK, callback1.WaitForResult());
  EXPECT_FALSE(info.is_direct());
  EXPECT_EQ("foopy1:8080", info.proxy_server().ToURI());

  // Fake a proxy error.
  TestCompletionCallback callback2;
  rv = service->ReconsiderProxyAfterError(url, &info, &callback2, NULL, NULL);
  EXPECT_EQ(OK, rv);

  // The first proxy is ignored, and the second one is selected.
  EXPECT_FALSE(info.is_direct());
  EXPECT_EQ("foopy2:9090", info.proxy_server().ToURI());

  // Fake a PAC failure.
  ProxyInfo info2;
  TestCompletionCallback callback3;
  rv = service->ResolveProxy(url, &info2, &callback3, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  ASSERT_EQ(1u, resolver->pending_requests().size());
  EXPECT_EQ(url, resolver->pending_requests()[0]->url());

  // This simulates a javascript runtime error in the PAC script.
  resolver->pending_requests()[0]->CompleteNow(ERR_FAILED);

  // Although the resolver failed, the ProxyService will implicitly fall-back
  // to a DIRECT connection.
  EXPECT_EQ(OK, callback3.WaitForResult());
  EXPECT_TRUE(info2.is_direct());
  EXPECT_FALSE(info2.is_empty());

  // The PAC script will work properly next time and successfully return a
  // proxy list. Since we have not marked the configuration as bad, it should
  // "just work" the next time we call it.
  ProxyInfo info3;
  TestCompletionCallback callback4;
  rv = service->ReconsiderProxyAfterError(url, &info3, &callback4, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  ASSERT_EQ(1u, resolver->pending_requests().size());
  EXPECT_EQ(url, resolver->pending_requests()[0]->url());

  resolver->pending_requests()[0]->results()->UseNamedProxy(
      "foopy1:8080;foopy2:9090");
  resolver->pending_requests()[0]->CompleteNow(OK);

  // The first proxy is not there since the it was added to the bad proxies
  // list by the earlier ReconsiderProxyAfterError().
  EXPECT_EQ(OK, callback4.WaitForResult());
  EXPECT_FALSE(info3.is_direct());
  EXPECT_EQ("foopy1:8080", info3.proxy_server().ToURI());
}

TEST(ProxyServiceTest, ProxyBypassList) {
  // Test that the proxy bypass rules are consulted.

  TestCompletionCallback callback[2];
  ProxyInfo info[2];
  ProxyConfig config;
  config.proxy_rules().ParseFromString("foopy1:8080;foopy2:9090");
  config.set_auto_detect(false);
  config.proxy_rules().bypass_rules.ParseFromString("*.org");

  scoped_refptr<ProxyService> service(new ProxyService(
      new MockProxyConfigService(config), new MockAsyncProxyResolver, NULL));

  int rv;
  GURL url1("http://www.webkit.org");
  GURL url2("http://www.webkit.com");

  // Request for a .org domain should bypass proxy.
  rv = service->ResolveProxy(url1, &info[0], &callback[0], NULL, NULL);
  EXPECT_EQ(OK, rv);
  EXPECT_TRUE(info[0].is_direct());

  // Request for a .com domain hits the proxy.
  rv = service->ResolveProxy(url2, &info[1], &callback[1], NULL, NULL);
  EXPECT_EQ(OK, rv);
  EXPECT_EQ("foopy1:8080", info[1].proxy_server().ToURI());
}


TEST(ProxyServiceTest, PerProtocolProxyTests) {
  ProxyConfig config;
  config.proxy_rules().ParseFromString("http=foopy1:8080;https=foopy2:8080");
  config.set_auto_detect(false);
  {
    scoped_refptr<ProxyService> service(new ProxyService(
        new MockProxyConfigService(config), new MockAsyncProxyResolver, NULL));
    GURL test_url("http://www.msn.com");
    ProxyInfo info;
    TestCompletionCallback callback;
    int rv = service->ResolveProxy(test_url, &info, &callback, NULL, NULL);
    EXPECT_EQ(OK, rv);
    EXPECT_FALSE(info.is_direct());
    EXPECT_EQ("foopy1:8080", info.proxy_server().ToURI());
  }
  {
    scoped_refptr<ProxyService> service(new ProxyService(
        new MockProxyConfigService(config), new MockAsyncProxyResolver, NULL));
    GURL test_url("ftp://ftp.google.com");
    ProxyInfo info;
    TestCompletionCallback callback;
    int rv = service->ResolveProxy(test_url, &info, &callback, NULL, NULL);
    EXPECT_EQ(OK, rv);
    EXPECT_TRUE(info.is_direct());
    EXPECT_EQ("direct://", info.proxy_server().ToURI());
  }
  {
    scoped_refptr<ProxyService> service(new ProxyService(
        new MockProxyConfigService(config), new MockAsyncProxyResolver, NULL));
    GURL test_url("https://webbranch.techcu.com");
    ProxyInfo info;
    TestCompletionCallback callback;
    int rv = service->ResolveProxy(test_url, &info, &callback, NULL, NULL);
    EXPECT_EQ(OK, rv);
    EXPECT_FALSE(info.is_direct());
    EXPECT_EQ("foopy2:8080", info.proxy_server().ToURI());
  }
  {
    config.proxy_rules().ParseFromString("foopy1:8080");
    scoped_refptr<ProxyService> service(new ProxyService(
        new MockProxyConfigService(config), new MockAsyncProxyResolver, NULL));
    GURL test_url("http://www.microsoft.com");
    ProxyInfo info;
    TestCompletionCallback callback;
    int rv = service->ResolveProxy(test_url, &info, &callback, NULL, NULL);
    EXPECT_EQ(OK, rv);
    EXPECT_FALSE(info.is_direct());
    EXPECT_EQ("foopy1:8080", info.proxy_server().ToURI());
  }
}

// If only HTTP and a SOCKS proxy are specified, check if ftp/https queries
// fall back to the SOCKS proxy.
TEST(ProxyServiceTest, DefaultProxyFallbackToSOCKS) {
  ProxyConfig config;
  config.proxy_rules().ParseFromString("http=foopy1:8080;socks=foopy2:1080");
  config.set_auto_detect(false);
  EXPECT_EQ(ProxyConfig::ProxyRules::TYPE_PROXY_PER_SCHEME,
            config.proxy_rules().type);

  {
    scoped_refptr<ProxyService> service(new ProxyService(
        new MockProxyConfigService(config), new MockAsyncProxyResolver, NULL));
    GURL test_url("http://www.msn.com");
    ProxyInfo info;
    TestCompletionCallback callback;
    int rv = service->ResolveProxy(test_url, &info, &callback, NULL, NULL);
    EXPECT_EQ(OK, rv);
    EXPECT_FALSE(info.is_direct());
    EXPECT_EQ("foopy1:8080", info.proxy_server().ToURI());
  }
  {
    scoped_refptr<ProxyService> service(new ProxyService(
        new MockProxyConfigService(config), new MockAsyncProxyResolver, NULL));
    GURL test_url("ftp://ftp.google.com");
    ProxyInfo info;
    TestCompletionCallback callback;
    int rv = service->ResolveProxy(test_url, &info, &callback, NULL, NULL);
    EXPECT_EQ(OK, rv);
    EXPECT_FALSE(info.is_direct());
    EXPECT_EQ("socks4://foopy2:1080", info.proxy_server().ToURI());
  }
  {
    scoped_refptr<ProxyService> service(new ProxyService(
        new MockProxyConfigService(config), new MockAsyncProxyResolver, NULL));
    GURL test_url("https://webbranch.techcu.com");
    ProxyInfo info;
    TestCompletionCallback callback;
    int rv = service->ResolveProxy(test_url, &info, &callback, NULL, NULL);
    EXPECT_EQ(OK, rv);
    EXPECT_FALSE(info.is_direct());
    EXPECT_EQ("socks4://foopy2:1080", info.proxy_server().ToURI());
  }
  {
    scoped_refptr<ProxyService> service(new ProxyService(
        new MockProxyConfigService(config), new MockAsyncProxyResolver, NULL));
    GURL test_url("unknown://www.microsoft.com");
    ProxyInfo info;
    TestCompletionCallback callback;
    int rv = service->ResolveProxy(test_url, &info, &callback, NULL, NULL);
    EXPECT_EQ(OK, rv);
    EXPECT_FALSE(info.is_direct());
    EXPECT_EQ("socks4://foopy2:1080", info.proxy_server().ToURI());
  }
}

// Test cancellation of an in-progress request.
TEST(ProxyServiceTest, CancelInProgressRequest) {
  MockProxyConfigService* config_service =
      new MockProxyConfigService("http://foopy/proxy.pac");

  MockAsyncProxyResolver* resolver = new MockAsyncProxyResolver;

  scoped_refptr<ProxyService> service(
      new ProxyService(config_service, resolver, NULL));

  // Start 3 requests.

  ProxyInfo info1;
  TestCompletionCallback callback1;
  int rv = service->ResolveProxy(
      GURL("http://request1"), &info1, &callback1, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // Nothing has been sent to the proxy resolver yet, since the proxy
  // resolver has not been configured yet.
  ASSERT_EQ(0u, resolver->pending_requests().size());

  // Successfully initialize the PAC script.
  EXPECT_EQ(GURL("http://foopy/proxy.pac"),
            resolver->pending_set_pac_script_request()->pac_url());
  resolver->pending_set_pac_script_request()->CompleteNow(OK);

  ASSERT_EQ(1u, resolver->pending_requests().size());
  EXPECT_EQ(GURL("http://request1"), resolver->pending_requests()[0]->url());

  ProxyInfo info2;
  TestCompletionCallback callback2;
  ProxyService::PacRequest* request2;
  rv = service->ResolveProxy(
      GURL("http://request2"), &info2, &callback2, &request2, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);
  ASSERT_EQ(2u, resolver->pending_requests().size());
  EXPECT_EQ(GURL("http://request2"), resolver->pending_requests()[1]->url());

  ProxyInfo info3;
  TestCompletionCallback callback3;
  rv = service->ResolveProxy(
      GURL("http://request3"), &info3, &callback3, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);
  ASSERT_EQ(3u, resolver->pending_requests().size());
  EXPECT_EQ(GURL("http://request3"), resolver->pending_requests()[2]->url());

  // Cancel the second request
  service->CancelPacRequest(request2);

  ASSERT_EQ(2u, resolver->pending_requests().size());
  EXPECT_EQ(GURL("http://request1"), resolver->pending_requests()[0]->url());
  EXPECT_EQ(GURL("http://request3"), resolver->pending_requests()[1]->url());

  // Complete the two un-cancelled requests.
  // We complete the last one first, just to mix it up a bit.
  resolver->pending_requests()[1]->results()->UseNamedProxy("request3:80");
  resolver->pending_requests()[1]->CompleteNow(OK);

  resolver->pending_requests()[0]->results()->UseNamedProxy("request1:80");
  resolver->pending_requests()[0]->CompleteNow(OK);

  // Complete and verify that requests ran as expected.
  EXPECT_EQ(OK, callback1.WaitForResult());
  EXPECT_EQ("request1:80", info1.proxy_server().ToURI());

  EXPECT_FALSE(callback2.have_result());  // Cancelled.
  ASSERT_EQ(1u, resolver->cancelled_requests().size());
  EXPECT_EQ(GURL("http://request2"), resolver->cancelled_requests()[0]->url());

  EXPECT_EQ(OK, callback3.WaitForResult());
  EXPECT_EQ("request3:80", info3.proxy_server().ToURI());
}

// Test the initial PAC download for resolver that expects bytes.
TEST(ProxyServiceTest, InitialPACScriptDownload) {
  MockProxyConfigService* config_service =
      new MockProxyConfigService("http://foopy/proxy.pac");

  MockAsyncProxyResolverExpectsBytes* resolver =
      new MockAsyncProxyResolverExpectsBytes;

  scoped_refptr<ProxyService> service(
      new ProxyService(config_service, resolver, NULL));

  MockProxyScriptFetcher* fetcher = new MockProxyScriptFetcher;
  service->SetProxyScriptFetcher(fetcher);

  // Start 3 requests.

  ProxyInfo info1;
  TestCompletionCallback callback1;
  int rv = service->ResolveProxy(
      GURL("http://request1"), &info1, &callback1, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // The first request should have triggered download of PAC script.
  EXPECT_TRUE(fetcher->has_pending_request());
  EXPECT_EQ(GURL("http://foopy/proxy.pac"), fetcher->pending_request_url());

  ProxyInfo info2;
  TestCompletionCallback callback2;
  rv = service->ResolveProxy(
      GURL("http://request2"), &info2, &callback2, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  ProxyInfo info3;
  TestCompletionCallback callback3;
  rv = service->ResolveProxy(
      GURL("http://request3"), &info3, &callback3, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // Nothing has been sent to the resolver yet.
  EXPECT_TRUE(resolver->pending_requests().empty());

  // At this point the ProxyService should be waiting for the
  // ProxyScriptFetcher to invoke its completion callback, notifying it of
  // PAC script download completion.
  fetcher->NotifyFetchCompletion(OK, "pac-v1");

  // Now that the PAC script is downloaded, it will have been sent to the proxy
  // resolver.
  EXPECT_EQ("pac-v1", resolver->pending_set_pac_script_request()->pac_bytes());
  resolver->pending_set_pac_script_request()->CompleteNow(OK);

  ASSERT_EQ(3u, resolver->pending_requests().size());
  EXPECT_EQ(GURL("http://request1"), resolver->pending_requests()[0]->url());
  EXPECT_EQ(GURL("http://request2"), resolver->pending_requests()[1]->url());
  EXPECT_EQ(GURL("http://request3"), resolver->pending_requests()[2]->url());

  // Complete all the requests (in some order).
  // Note that as we complete requests, they shift up in |pending_requests()|.

  resolver->pending_requests()[2]->results()->UseNamedProxy("request3:80");
  resolver->pending_requests()[2]->CompleteNow(OK);

  resolver->pending_requests()[0]->results()->UseNamedProxy("request1:80");
  resolver->pending_requests()[0]->CompleteNow(OK);

  resolver->pending_requests()[0]->results()->UseNamedProxy("request2:80");
  resolver->pending_requests()[0]->CompleteNow(OK);

  // Complete and verify that requests ran as expected.
  EXPECT_EQ(OK, callback1.WaitForResult());
  EXPECT_EQ("request1:80", info1.proxy_server().ToURI());

  EXPECT_EQ(OK, callback2.WaitForResult());
  EXPECT_EQ("request2:80", info2.proxy_server().ToURI());

  EXPECT_EQ(OK, callback3.WaitForResult());
  EXPECT_EQ("request3:80", info3.proxy_server().ToURI());
}

// Test changing the ProxyScriptFetcher while PAC download is in progress.
TEST(ProxyServiceTest, ChangeScriptFetcherWhilePACDownloadInProgress) {
  MockProxyConfigService* config_service =
      new MockProxyConfigService("http://foopy/proxy.pac");

  MockAsyncProxyResolverExpectsBytes* resolver =
      new MockAsyncProxyResolverExpectsBytes;

  scoped_refptr<ProxyService> service(
      new ProxyService(config_service, resolver, NULL));

  MockProxyScriptFetcher* fetcher = new MockProxyScriptFetcher;
  service->SetProxyScriptFetcher(fetcher);

  // Start 2 requests.

  ProxyInfo info1;
  TestCompletionCallback callback1;
  int rv = service->ResolveProxy(
      GURL("http://request1"), &info1, &callback1, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // The first request should have triggered download of PAC script.
  EXPECT_TRUE(fetcher->has_pending_request());
  EXPECT_EQ(GURL("http://foopy/proxy.pac"), fetcher->pending_request_url());

  ProxyInfo info2;
  TestCompletionCallback callback2;
  rv = service->ResolveProxy(
      GURL("http://request2"), &info2, &callback2, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // At this point the ProxyService should be waiting for the
  // ProxyScriptFetcher to invoke its completion callback, notifying it of
  // PAC script download completion.

  // We now change out the ProxyService's script fetcher. We should restart
  // the initialization with the new fetcher.

  fetcher = new MockProxyScriptFetcher;
  service->SetProxyScriptFetcher(fetcher);

  // Nothing has been sent to the resolver yet.
  EXPECT_TRUE(resolver->pending_requests().empty());

  fetcher->NotifyFetchCompletion(OK, "pac-v1");

  // Now that the PAC script is downloaded, it will have been sent to the proxy
  // resolver.
  EXPECT_EQ("pac-v1", resolver->pending_set_pac_script_request()->pac_bytes());
  resolver->pending_set_pac_script_request()->CompleteNow(OK);

  ASSERT_EQ(2u, resolver->pending_requests().size());
  EXPECT_EQ(GURL("http://request1"), resolver->pending_requests()[0]->url());
  EXPECT_EQ(GURL("http://request2"), resolver->pending_requests()[1]->url());
}

// Test cancellation of a request, while the PAC script is being fetched.
TEST(ProxyServiceTest, CancelWhilePACFetching) {
  MockProxyConfigService* config_service =
      new MockProxyConfigService("http://foopy/proxy.pac");

  MockAsyncProxyResolverExpectsBytes* resolver =
      new MockAsyncProxyResolverExpectsBytes;

  scoped_refptr<ProxyService> service(
      new ProxyService(config_service, resolver, NULL));

  MockProxyScriptFetcher* fetcher = new MockProxyScriptFetcher;
  service->SetProxyScriptFetcher(fetcher);

  // Start 3 requests.
  ProxyInfo info1;
  TestCompletionCallback callback1;
  ProxyService::PacRequest* request1;
  scoped_refptr<LoadLog> log1(new LoadLog(LoadLog::kUnbounded));
  int rv = service->ResolveProxy(
      GURL("http://request1"), &info1, &callback1, &request1, log1);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // The first request should have triggered download of PAC script.
  EXPECT_TRUE(fetcher->has_pending_request());
  EXPECT_EQ(GURL("http://foopy/proxy.pac"), fetcher->pending_request_url());

  ProxyInfo info2;
  TestCompletionCallback callback2;
  ProxyService::PacRequest* request2;
  rv = service->ResolveProxy(
      GURL("http://request2"), &info2, &callback2, &request2, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  ProxyInfo info3;
  TestCompletionCallback callback3;
  rv = service->ResolveProxy(
      GURL("http://request3"), &info3, &callback3, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // Nothing has been sent to the resolver yet.
  EXPECT_TRUE(resolver->pending_requests().empty());

  // Cancel the first 2 requests.
  service->CancelPacRequest(request1);
  service->CancelPacRequest(request2);

  // At this point the ProxyService should be waiting for the
  // ProxyScriptFetcher to invoke its completion callback, notifying it of
  // PAC script download completion.
  fetcher->NotifyFetchCompletion(OK, "pac-v1");

  // Now that the PAC script is downloaded, it will have been sent to the
  // proxy resolver.
  EXPECT_EQ("pac-v1", resolver->pending_set_pac_script_request()->pac_bytes());
  resolver->pending_set_pac_script_request()->CompleteNow(OK);

  ASSERT_EQ(1u, resolver->pending_requests().size());
  EXPECT_EQ(GURL("http://request3"), resolver->pending_requests()[0]->url());

  // Complete all the requests.
  resolver->pending_requests()[0]->results()->UseNamedProxy("request3:80");
  resolver->pending_requests()[0]->CompleteNow(OK);

  EXPECT_EQ(OK, callback3.WaitForResult());
  EXPECT_EQ("request3:80", info3.proxy_server().ToURI());

  EXPECT_TRUE(resolver->cancelled_requests().empty());

  EXPECT_FALSE(callback1.have_result());  // Cancelled.
  EXPECT_FALSE(callback2.have_result());  // Cancelled.

  // Check the LoadLog for request 1 (which was cancelled) got filled properly.
  EXPECT_EQ(6u, log1->entries().size());
  EXPECT_TRUE(LogContainsBeginEvent(*log1, 0, LoadLog::TYPE_PROXY_SERVICE));
  EXPECT_TRUE(LogContainsBeginEvent(
      *log1, 3, LoadLog::TYPE_PROXY_SERVICE_WAITING_FOR_INIT_PAC));
  // Note that TYPE_PROXY_SERVICE_WAITING_FOR_INIT_PAC is never completed before
  // the cancellation occured.
  EXPECT_TRUE(LogContainsEvent(
      *log1, 4, LoadLog::TYPE_CANCELLED, LoadLog::PHASE_NONE));
  EXPECT_TRUE(LogContainsEndEvent(*log1, 5, LoadLog::TYPE_PROXY_SERVICE));
}

// Test that if auto-detect fails, we fall-back to the custom pac.
TEST(ProxyServiceTest, FallbackFromAutodetectToCustomPac) {
  ProxyConfig config;
  config.set_auto_detect(true);
  config.set_pac_url(GURL("http://foopy/proxy.pac"));
  config.proxy_rules().ParseFromString("http=foopy:80");  // Won't be used.

  MockProxyConfigService* config_service = new MockProxyConfigService(config);
  MockAsyncProxyResolverExpectsBytes* resolver =
      new MockAsyncProxyResolverExpectsBytes;
  scoped_refptr<ProxyService> service(
      new ProxyService(config_service, resolver, NULL));

  MockProxyScriptFetcher* fetcher = new MockProxyScriptFetcher;
  service->SetProxyScriptFetcher(fetcher);

  // Start 2 requests.

  ProxyInfo info1;
  TestCompletionCallback callback1;
  int rv = service->ResolveProxy(
      GURL("http://request1"), &info1, &callback1, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  ProxyInfo info2;
  TestCompletionCallback callback2;
  ProxyService::PacRequest* request2;
  rv = service->ResolveProxy(
      GURL("http://request2"), &info2, &callback2, &request2, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // Check that nothing has been sent to the proxy resolver yet.
  ASSERT_EQ(0u, resolver->pending_requests().size());

  // It should be trying to auto-detect first -- FAIL the autodetect during
  // the script download.
  EXPECT_TRUE(fetcher->has_pending_request());
  EXPECT_EQ(GURL("http://wpad/wpad.dat"), fetcher->pending_request_url());
  fetcher->NotifyFetchCompletion(ERR_FAILED, "");

  // Next it should be trying the custom PAC url.
  EXPECT_TRUE(fetcher->has_pending_request());
  EXPECT_EQ(GURL("http://foopy/proxy.pac"), fetcher->pending_request_url());
  fetcher->NotifyFetchCompletion(OK, "custom-pac-script");

  EXPECT_EQ("custom-pac-script",
            resolver->pending_set_pac_script_request()->pac_bytes());
  resolver->pending_set_pac_script_request()->CompleteNow(OK);

  // Now finally, the pending requests should have been sent to the resolver
  // (which was initialized with custom PAC script).

  ASSERT_EQ(2u, resolver->pending_requests().size());
  EXPECT_EQ(GURL("http://request1"), resolver->pending_requests()[0]->url());
  EXPECT_EQ(GURL("http://request2"), resolver->pending_requests()[1]->url());

  // Complete the pending requests.
  resolver->pending_requests()[1]->results()->UseNamedProxy("request2:80");
  resolver->pending_requests()[1]->CompleteNow(OK);
  resolver->pending_requests()[0]->results()->UseNamedProxy("request1:80");
  resolver->pending_requests()[0]->CompleteNow(OK);

  // Verify that requests ran as expected.
  EXPECT_EQ(OK, callback1.WaitForResult());
  EXPECT_EQ("request1:80", info1.proxy_server().ToURI());

  EXPECT_EQ(OK, callback2.WaitForResult());
  EXPECT_EQ("request2:80", info2.proxy_server().ToURI());
}

// This is the same test as FallbackFromAutodetectToCustomPac, except
// the auto-detect script fails parsing rather than downloading.
TEST(ProxyServiceTest, FallbackFromAutodetectToCustomPac2) {
  ProxyConfig config;
  config.set_auto_detect(true);
  config.set_pac_url(GURL("http://foopy/proxy.pac"));
  config.proxy_rules().ParseFromString("http=foopy:80");  // Won't be used.

  MockProxyConfigService* config_service = new MockProxyConfigService(config);
  MockAsyncProxyResolverExpectsBytes* resolver =
      new MockAsyncProxyResolverExpectsBytes;
  scoped_refptr<ProxyService> service(
      new ProxyService(config_service, resolver, NULL));

  MockProxyScriptFetcher* fetcher = new MockProxyScriptFetcher;
  service->SetProxyScriptFetcher(fetcher);

  // Start 2 requests.

  ProxyInfo info1;
  TestCompletionCallback callback1;
  int rv = service->ResolveProxy(
      GURL("http://request1"), &info1, &callback1, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  ProxyInfo info2;
  TestCompletionCallback callback2;
  ProxyService::PacRequest* request2;
  rv = service->ResolveProxy(
      GURL("http://request2"), &info2, &callback2, &request2, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // Check that nothing has been sent to the proxy resolver yet.
  ASSERT_EQ(0u, resolver->pending_requests().size());

  // It should be trying to auto-detect first -- succeed the download.
  EXPECT_TRUE(fetcher->has_pending_request());
  EXPECT_EQ(GURL("http://wpad/wpad.dat"), fetcher->pending_request_url());
  fetcher->NotifyFetchCompletion(OK, "invalid-script-contents");

  // Simulate a parse error.
  EXPECT_EQ("invalid-script-contents",
            resolver->pending_set_pac_script_request()->pac_bytes());
  resolver->pending_set_pac_script_request()->CompleteNow(
      ERR_PAC_SCRIPT_FAILED);

  // Next it should be trying the custom PAC url.
  EXPECT_TRUE(fetcher->has_pending_request());
  EXPECT_EQ(GURL("http://foopy/proxy.pac"), fetcher->pending_request_url());
  fetcher->NotifyFetchCompletion(OK, "custom-pac-script");

  EXPECT_EQ("custom-pac-script",
            resolver->pending_set_pac_script_request()->pac_bytes());
  resolver->pending_set_pac_script_request()->CompleteNow(OK);

  // Now finally, the pending requests should have been sent to the resolver
  // (which was initialized with custom PAC script).

  ASSERT_EQ(2u, resolver->pending_requests().size());
  EXPECT_EQ(GURL("http://request1"), resolver->pending_requests()[0]->url());
  EXPECT_EQ(GURL("http://request2"), resolver->pending_requests()[1]->url());

  // Complete the pending requests.
  resolver->pending_requests()[1]->results()->UseNamedProxy("request2:80");
  resolver->pending_requests()[1]->CompleteNow(OK);
  resolver->pending_requests()[0]->results()->UseNamedProxy("request1:80");
  resolver->pending_requests()[0]->CompleteNow(OK);

  // Verify that requests ran as expected.
  EXPECT_EQ(OK, callback1.WaitForResult());
  EXPECT_EQ("request1:80", info1.proxy_server().ToURI());

  EXPECT_EQ(OK, callback2.WaitForResult());
  EXPECT_EQ("request2:80", info2.proxy_server().ToURI());
}

// Test that if all of auto-detect, a custom PAC script, and manual settings
// are given, then we will try them in that order.
TEST(ProxyServiceTest, FallbackFromAutodetectToCustomToManual) {
  ProxyConfig config;
  config.set_auto_detect(true);
  config.set_pac_url(GURL("http://foopy/proxy.pac"));
  config.proxy_rules().ParseFromString("http=foopy:80");

  MockProxyConfigService* config_service = new MockProxyConfigService(config);
  MockAsyncProxyResolverExpectsBytes* resolver =
      new MockAsyncProxyResolverExpectsBytes;
  scoped_refptr<ProxyService> service(
      new ProxyService(config_service, resolver, NULL));

  MockProxyScriptFetcher* fetcher = new MockProxyScriptFetcher;
  service->SetProxyScriptFetcher(fetcher);

  // Start 2 requests.

  ProxyInfo info1;
  TestCompletionCallback callback1;
  int rv = service->ResolveProxy(
      GURL("http://request1"), &info1, &callback1, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  ProxyInfo info2;
  TestCompletionCallback callback2;
  ProxyService::PacRequest* request2;
  rv = service->ResolveProxy(
      GURL("http://request2"), &info2, &callback2, &request2, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // Check that nothing has been sent to the proxy resolver yet.
  ASSERT_EQ(0u, resolver->pending_requests().size());

  // It should be trying to auto-detect first -- fail the download.
  EXPECT_TRUE(fetcher->has_pending_request());
  EXPECT_EQ(GURL("http://wpad/wpad.dat"), fetcher->pending_request_url());
  fetcher->NotifyFetchCompletion(ERR_FAILED, "");

  // Next it should be trying the custom PAC url -- fail the download.
  EXPECT_TRUE(fetcher->has_pending_request());
  EXPECT_EQ(GURL("http://foopy/proxy.pac"), fetcher->pending_request_url());
  fetcher->NotifyFetchCompletion(ERR_FAILED, "");

  // Since we never managed to initialize a ProxyResolver, nothing should have
  // been sent to it.
  ASSERT_EQ(0u, resolver->pending_requests().size());

  // Verify that requests ran as expected -- they should have fallen back to
  // the manual proxy configuration for HTTP urls.
  EXPECT_EQ(OK, callback1.WaitForResult());
  EXPECT_EQ("foopy:80", info1.proxy_server().ToURI());

  EXPECT_EQ(OK, callback2.WaitForResult());
  EXPECT_EQ("foopy:80", info2.proxy_server().ToURI());
}

// Test that the bypass rules are NOT applied when using autodetect.
TEST(ProxyServiceTest, BypassDoesntApplyToPac) {
  ProxyConfig config;
  config.set_auto_detect(true);
  config.set_pac_url(GURL("http://foopy/proxy.pac"));
  config.proxy_rules().ParseFromString("http=foopy:80");  // Not used.
  config.proxy_rules().bypass_rules.ParseFromString("www.google.com");

  MockProxyConfigService* config_service = new MockProxyConfigService(config);
  MockAsyncProxyResolverExpectsBytes* resolver =
      new MockAsyncProxyResolverExpectsBytes;
  scoped_refptr<ProxyService> service(
      new ProxyService(config_service, resolver, NULL));

  MockProxyScriptFetcher* fetcher = new MockProxyScriptFetcher;
  service->SetProxyScriptFetcher(fetcher);

  // Start 1 requests.

  ProxyInfo info1;
  TestCompletionCallback callback1;
  int rv = service->ResolveProxy(
      GURL("http://www.google.com"), &info1, &callback1, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // Check that nothing has been sent to the proxy resolver yet.
  ASSERT_EQ(0u, resolver->pending_requests().size());

  // It should be trying to auto-detect first -- succeed the download.
  EXPECT_TRUE(fetcher->has_pending_request());
  EXPECT_EQ(GURL("http://wpad/wpad.dat"), fetcher->pending_request_url());
  fetcher->NotifyFetchCompletion(OK, "auto-detect");

  EXPECT_EQ("auto-detect",
            resolver->pending_set_pac_script_request()->pac_bytes());
  resolver->pending_set_pac_script_request()->CompleteNow(OK);

  ASSERT_EQ(1u, resolver->pending_requests().size());
  EXPECT_EQ(GURL("http://www.google.com"),
            resolver->pending_requests()[0]->url());

  // Complete the pending request.
  resolver->pending_requests()[0]->results()->UseNamedProxy("request1:80");
  resolver->pending_requests()[0]->CompleteNow(OK);

  // Verify that request ran as expected.
  EXPECT_EQ(OK, callback1.WaitForResult());
  EXPECT_EQ("request1:80", info1.proxy_server().ToURI());

  // Start another request, it should pickup the bypass item.
  ProxyInfo info2;
  TestCompletionCallback callback2;
  rv = service->ResolveProxy(
      GURL("http://www.google.com"), &info2, &callback2, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  ASSERT_EQ(1u, resolver->pending_requests().size());
  EXPECT_EQ(GURL("http://www.google.com"),
            resolver->pending_requests()[0]->url());

  // Complete the pending request.
  resolver->pending_requests()[0]->results()->UseNamedProxy("request2:80");
  resolver->pending_requests()[0]->CompleteNow(OK);

  EXPECT_EQ(OK, callback2.WaitForResult());
  EXPECT_EQ("request2:80", info2.proxy_server().ToURI());
}

// Delete the ProxyService while InitProxyResolver has an outstanding
// request to the script fetcher. When run under valgrind, should not
// have any memory errors (used to be that the ProxyScriptFetcher was
// being deleted prior to the InitProxyResolver).
TEST(ProxyServiceTest, DeleteWhileInitProxyResolverHasOutstandingFetch) {
  ProxyConfig config;
  config.set_pac_url(GURL("http://foopy/proxy.pac"));

  MockProxyConfigService* config_service = new MockProxyConfigService(config);
  MockAsyncProxyResolverExpectsBytes* resolver =
      new MockAsyncProxyResolverExpectsBytes;
  scoped_refptr<ProxyService> service(
      new ProxyService(config_service, resolver, NULL));

  MockProxyScriptFetcher* fetcher = new MockProxyScriptFetcher;
  service->SetProxyScriptFetcher(fetcher);

  // Start 1 request.

  ProxyInfo info1;
  TestCompletionCallback callback1;
  int rv = service->ResolveProxy(
      GURL("http://www.google.com"), &info1, &callback1, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // Check that nothing has been sent to the proxy resolver yet.
  ASSERT_EQ(0u, resolver->pending_requests().size());

  // InitProxyResolver should have issued a request to the ProxyScriptFetcher
  // and be waiting on that to complete.
  EXPECT_TRUE(fetcher->has_pending_request());
  EXPECT_EQ(GURL("http://foopy/proxy.pac"), fetcher->pending_request_url());

  // Delete the ProxyService
  service = NULL;
}

// Delete the ProxyService while InitProxyResolver has an outstanding
// request to the proxy resolver. When run under valgrind, should not
// have any memory errors (used to be that the ProxyResolver was
// being deleted prior to the InitProxyResolver).
TEST(ProxyServiceTest, DeleteWhileInitProxyResolverHasOutstandingSet) {
  MockProxyConfigService* config_service =
      new MockProxyConfigService("http://foopy/proxy.pac");

  MockAsyncProxyResolver* resolver = new MockAsyncProxyResolver;

  scoped_refptr<ProxyService> service(
      new ProxyService(config_service, resolver, NULL));

  GURL url("http://www.google.com/");

  ProxyInfo info;
  TestCompletionCallback callback;
  int rv = service->ResolveProxy(url, &info, &callback, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  EXPECT_EQ(GURL("http://foopy/proxy.pac"),
            resolver->pending_set_pac_script_request()->pac_url());

  // Delete the ProxyService.
  service = NULL;
}

TEST(ProxyServiceTest, ResetProxyConfigService) {
  ProxyConfig config1;
  config1.proxy_rules().ParseFromString("foopy1:8080");
  config1.set_auto_detect(false);
  scoped_refptr<ProxyService> service(new ProxyService(
      new MockProxyConfigService(config1),
      new MockAsyncProxyResolverExpectsBytes,
      NULL));

  ProxyInfo info;
  TestCompletionCallback callback1;
  int rv = service->ResolveProxy(
      GURL("http://request1"), &info, &callback1, NULL, NULL);
  EXPECT_EQ(OK, rv);
  EXPECT_EQ("foopy1:8080", info.proxy_server().ToURI());

  ProxyConfig config2;
  config2.proxy_rules().ParseFromString("foopy2:8080");
  config2.set_auto_detect(false);
  service->ResetConfigService(new MockProxyConfigService(config2));
  TestCompletionCallback callback2;
  rv = service->ResolveProxy(
      GURL("http://request2"), &info, &callback2, NULL, NULL);
  EXPECT_EQ(OK, rv);
  EXPECT_EQ("foopy2:8080", info.proxy_server().ToURI());
}

// Check that after we have done the auto-detect test, and the configuration
// is updated (with no change), we don't re-try the autodetect test.
// Regression test for http://crbug.com/18526 -- the configuration was being
// mutated to cancel out the automatic settings, which meant UpdateConfig()
// thought it had received a new configuration.
TEST(ProxyServiceTest, UpdateConfigAfterFailedAutodetect) {
  ProxyConfig config;
  config.set_auto_detect(true);

  MockProxyConfigService* config_service = new MockProxyConfigService(config);
  MockAsyncProxyResolver* resolver = new MockAsyncProxyResolver;
  scoped_refptr<ProxyService> service(
      new ProxyService(config_service, resolver, NULL));

  // Start 1 requests.

  ProxyInfo info1;
  TestCompletionCallback callback1;
  int rv = service->ResolveProxy(
      GURL("http://www.google.com"), &info1, &callback1, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // Check that nothing has been sent to the proxy resolver yet.
  ASSERT_EQ(0u, resolver->pending_requests().size());

  // Fail the setting of autodetect script.
  EXPECT_EQ(GURL(), resolver->pending_set_pac_script_request()->pac_url());
  resolver->pending_set_pac_script_request()->CompleteNow(ERR_FAILED);

  // Verify that request ran as expected -- should have fallen back to direct.
  EXPECT_EQ(OK, callback1.WaitForResult());
  EXPECT_TRUE(info1.is_direct());

  // Force the ProxyService to pull down a new proxy configuration.
  // (Even though the configuration isn't old/bad).
  service->UpdateConfig(NULL);

  // Start another request -- the effective configuration has not
  // changed, so we shouldn't re-run the autodetect step.
  // Rather, it should complete synchronously as direct-connect.
  ProxyInfo info2;
  TestCompletionCallback callback2;
  rv = service->ResolveProxy(
      GURL("http://www.google.com"), &info2, &callback2, NULL, NULL);
  EXPECT_EQ(OK, rv);

  EXPECT_TRUE(info2.is_direct());
}

// Test that when going from a configuration that required PAC to one
// that does NOT, we unset the variable |should_use_proxy_resolver_|.
TEST(ProxyServiceTest, UpdateConfigFromPACToDirect) {
  ProxyConfig config;
  config.set_auto_detect(true);

  MockProxyConfigService* config_service = new MockProxyConfigService(config);
  MockAsyncProxyResolver* resolver = new MockAsyncProxyResolver;
  scoped_refptr<ProxyService> service(
      new ProxyService(config_service, resolver, NULL));

  // Start 1 request.

  ProxyInfo info1;
  TestCompletionCallback callback1;
  int rv = service->ResolveProxy(
      GURL("http://www.google.com"), &info1, &callback1, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // Check that nothing has been sent to the proxy resolver yet.
  ASSERT_EQ(0u, resolver->pending_requests().size());

  // Successfully set the autodetect script.
  EXPECT_EQ(GURL(), resolver->pending_set_pac_script_request()->pac_url());
  resolver->pending_set_pac_script_request()->CompleteNow(OK);

  // Complete the pending request.
  ASSERT_EQ(1u, resolver->pending_requests().size());
  resolver->pending_requests()[0]->results()->UseNamedProxy("request1:80");
  resolver->pending_requests()[0]->CompleteNow(OK);

  // Verify that request ran as expected.
  EXPECT_EQ(OK, callback1.WaitForResult());
  EXPECT_EQ("request1:80", info1.proxy_server().ToURI());

  // Force the ProxyService to pull down a new proxy configuration.
  // (Even though the configuration isn't old/bad).
  //
  // This new configuration no longer has auto_detect set, so
  // requests should complete synchronously now as direct-connect.
  config.set_auto_detect(false);
  config_service->config = config;
  service->UpdateConfig(NULL);

  // Start another request -- the effective configuration has changed.
  ProxyInfo info2;
  TestCompletionCallback callback2;
  rv = service->ResolveProxy(
      GURL("http://www.google.com"), &info2, &callback2, NULL, NULL);
  EXPECT_EQ(OK, rv);

  EXPECT_TRUE(info2.is_direct());
}

TEST(ProxyServiceTest, NetworkChangeTriggersPacRefetch) {
  MockProxyConfigService* config_service =
      new MockProxyConfigService("http://foopy/proxy.pac");

  MockAsyncProxyResolverExpectsBytes* resolver =
      new MockAsyncProxyResolverExpectsBytes;

  MockNetworkChangeNotifier network_change_notifier;

  scoped_refptr<ProxyService> service(
      new ProxyService(config_service, resolver, &network_change_notifier));

  MockProxyScriptFetcher* fetcher = new MockProxyScriptFetcher;
  service->SetProxyScriptFetcher(fetcher);

  // Start 1 request.

  ProxyInfo info1;
  TestCompletionCallback callback1;
  int rv = service->ResolveProxy(
      GURL("http://request1"), &info1, &callback1, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // The first request should have triggered initial download of PAC script.
  EXPECT_TRUE(fetcher->has_pending_request());
  EXPECT_EQ(GURL("http://foopy/proxy.pac"), fetcher->pending_request_url());

  // Nothing has been sent to the resolver yet.
  EXPECT_TRUE(resolver->pending_requests().empty());

  // At this point the ProxyService should be waiting for the
  // ProxyScriptFetcher to invoke its completion callback, notifying it of
  // PAC script download completion.
  fetcher->NotifyFetchCompletion(OK, "pac-v1");

  // Now that the PAC script is downloaded, the request will have been sent to
  // the proxy resolver.
  EXPECT_EQ("pac-v1", resolver->pending_set_pac_script_request()->pac_bytes());
  resolver->pending_set_pac_script_request()->CompleteNow(OK);

  ASSERT_EQ(1u, resolver->pending_requests().size());
  EXPECT_EQ(GURL("http://request1"), resolver->pending_requests()[0]->url());

  // Complete the pending request.
  resolver->pending_requests()[0]->results()->UseNamedProxy("request1:80");
  resolver->pending_requests()[0]->CompleteNow(OK);

  // Wait for completion callback, and verify that the request ran as expected.
  EXPECT_EQ(OK, callback1.WaitForResult());
  EXPECT_EQ("request1:80", info1.proxy_server().ToURI());

  // Now simluate a change in the network. The ProxyConfigService is still
  // going to return the same PAC URL as before, but this URL needs to be
  // refetched on the new network.

  network_change_notifier.NotifyIPAddressChange();

  // Start a second request.
  ProxyInfo info2;
  TestCompletionCallback callback2;
  rv = service->ResolveProxy(
      GURL("http://request2"), &info2, &callback2, NULL, NULL);
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // This second request should have triggered the re-download of the PAC
  // script (since we marked the network as having changed).
  EXPECT_TRUE(fetcher->has_pending_request());
  EXPECT_EQ(GURL("http://foopy/proxy.pac"), fetcher->pending_request_url());

  // Nothing has been sent to the resolver yet.
  EXPECT_TRUE(resolver->pending_requests().empty());

  // Simulate the PAC script fetch as having completed (this time with
  // different data).
  fetcher->NotifyFetchCompletion(OK, "pac-v2");

  // Now that the PAC script is downloaded, the second request will have been
  // sent to the proxy resolver.
  EXPECT_EQ("pac-v2", resolver->pending_set_pac_script_request()->pac_bytes());
  resolver->pending_set_pac_script_request()->CompleteNow(OK);

  ASSERT_EQ(1u, resolver->pending_requests().size());
  EXPECT_EQ(GURL("http://request2"), resolver->pending_requests()[0]->url());

  // Complete the pending second request.
  resolver->pending_requests()[0]->results()->UseNamedProxy("request2:80");
  resolver->pending_requests()[0]->CompleteNow(OK);

  // Wait for completion callback, and verify that the request ran as expected.
  EXPECT_EQ(OK, callback2.WaitForResult());
  EXPECT_EQ("request2:80", info2.proxy_server().ToURI());
}

}  // namespace net
