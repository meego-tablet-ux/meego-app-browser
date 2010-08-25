// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/proxy/proxy_config_service_common_unittest.h"

#include <string>
#include <vector>

#include "net/proxy/proxy_config.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace {

// Helper to verify that |expected_proxy| matches |actual_proxy|. If it does
// not, then |*did_fail| is set to true, and |*failure_details| is filled with
// a description of the failure.
void MatchesProxyServerHelper(const char* failure_message,
                              const char* expected_proxy,
                              const ProxyServer& actual_proxy,
                              ::testing::AssertionResult* failure_details,
                              bool* did_fail) {
  std::string actual_proxy_string;
  if (actual_proxy.is_valid())
    actual_proxy_string = actual_proxy.ToURI();

  if (std::string(expected_proxy) != actual_proxy_string) {
    *failure_details
        << failure_message << ". Was expecting: \"" << expected_proxy
        << "\" but got: \"" << actual_proxy_string << "\"";
    *did_fail = true;
  }
}

std::string FlattenProxyBypass(const ProxyBypassRules& bypass_rules) {
  std::string flattened_proxy_bypass;
  for (ProxyBypassRules::RuleList::const_iterator it =
       bypass_rules.rules().begin();
       it != bypass_rules.rules().end(); ++it) {
    if (!flattened_proxy_bypass.empty())
      flattened_proxy_bypass += ",";
    flattened_proxy_bypass += (*it)->ToString();
  }
  return flattened_proxy_bypass;
}

}  // namespace

::testing::AssertionResult ProxyRulesExpectation::Matches(
    const ProxyConfig::ProxyRules& rules) const {
  ::testing::AssertionResult failure_details = ::testing::AssertionFailure();
  bool failed = false;

  if (rules.type != type) {
    failure_details << "Type mismatch. Expected: "
                    << type << " but was: " << rules.type;
    failed = true;
  }

  MatchesProxyServerHelper("Bad single_proxy", single_proxy,
                           rules.single_proxy, &failure_details, &failed);
  MatchesProxyServerHelper("Bad proxy_for_http", proxy_for_http,
                           rules.proxy_for_http, &failure_details, &failed);
  MatchesProxyServerHelper("Bad proxy_for_https", proxy_for_https,
                           rules.proxy_for_https, &failure_details, &failed);
  MatchesProxyServerHelper("Bad fallback_proxy", fallback_proxy,
                           rules.fallback_proxy, &failure_details, &failed);

  std::string actual_flattened_bypass = FlattenProxyBypass(rules.bypass_rules);
  if (std::string(flattened_bypass_rules) != actual_flattened_bypass) {
    failure_details
        << "Bad bypass rules. Expected: \"" << flattened_bypass_rules
        << "\" but got: \"" << actual_flattened_bypass << "\"";
    failed = true;
  }

  if (rules.reverse_bypass != reverse_bypass) {
    failure_details << "Bad reverse_bypass. Expected: " << reverse_bypass
                    << " but got: " << rules.reverse_bypass;
    failed = true;
  }

  return failed ? failure_details : ::testing::AssertionSuccess();
}

// static
ProxyRulesExpectation ProxyRulesExpectation::Empty() {
  return ProxyRulesExpectation(ProxyConfig::ProxyRules::TYPE_NO_RULES,
                               "", "", "", "", "", "", false);
}

// static
ProxyRulesExpectation ProxyRulesExpectation::EmptyWithBypass(
    const char* flattened_bypass_rules) {
  return ProxyRulesExpectation(ProxyConfig::ProxyRules::TYPE_NO_RULES,
                               "", "", "", "", "", flattened_bypass_rules,
                               false);
}

// static
ProxyRulesExpectation ProxyRulesExpectation::Single(
    const char* single_proxy,
    const char* flattened_bypass_rules) {
  return ProxyRulesExpectation(ProxyConfig::ProxyRules::TYPE_SINGLE_PROXY,
                               single_proxy, "", "", "", "",
                               flattened_bypass_rules, false);
}

// static
ProxyRulesExpectation ProxyRulesExpectation::PerScheme(
    const char* proxy_http,
    const char* proxy_https,
    const char* proxy_ftp,
    const char* flattened_bypass_rules) {
  return ProxyRulesExpectation(ProxyConfig::ProxyRules::TYPE_PROXY_PER_SCHEME,
                               "", proxy_http, proxy_https, proxy_ftp, "",
                               flattened_bypass_rules, false);
}

// static
ProxyRulesExpectation ProxyRulesExpectation::PerSchemeWithSocks(
    const char* proxy_http,
    const char* proxy_https,
    const char* proxy_ftp,
    const char* socks_proxy,
    const char* flattened_bypass_rules) {
  return ProxyRulesExpectation(ProxyConfig::ProxyRules::TYPE_PROXY_PER_SCHEME,
                               "", proxy_http, proxy_https, proxy_ftp,
                               socks_proxy, flattened_bypass_rules, false);
}

// static
ProxyRulesExpectation ProxyRulesExpectation::PerSchemeWithBypassReversed(
    const char* proxy_http,
    const char* proxy_https,
    const char* proxy_ftp,
    const char* flattened_bypass_rules) {
  return ProxyRulesExpectation(ProxyConfig::ProxyRules::TYPE_PROXY_PER_SCHEME,
                               "", proxy_http, proxy_https, proxy_ftp, "",
                               flattened_bypass_rules, true);
}

}  // namespace net
