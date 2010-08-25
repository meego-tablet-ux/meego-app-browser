// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"

#include "base/file_path.h"
#include "base/path_service.h"
#include "chrome/browser/importer/firefox_proxy_settings.h"
#include "chrome/common/chrome_paths.h"
#include "net/proxy/proxy_config.h"

class FirefoxProxySettingsTest : public testing::Test {
};

class TestFirefoxProxySettings : public FirefoxProxySettings {
 public:
  TestFirefoxProxySettings() {}

  static bool TestGetSettingsFromFile(const FilePath& pref_file,
                                      FirefoxProxySettings* settings) {
    return GetSettingsFromFile(pref_file, settings);
  }
};

TEST_F(FirefoxProxySettingsTest, TestParse) {
  FirefoxProxySettings settings;

  FilePath js_pref_path;
  ASSERT_TRUE(PathService::Get(chrome::DIR_TEST_DATA, &js_pref_path));
  js_pref_path = js_pref_path.AppendASCII("firefox3_pref.js");

  EXPECT_TRUE(TestFirefoxProxySettings::TestGetSettingsFromFile(js_pref_path,
                                                                &settings));
  EXPECT_EQ(FirefoxProxySettings::MANUAL, settings.config_type());
  EXPECT_EQ("http_proxy", settings.http_proxy());
  EXPECT_EQ(1111, settings.http_proxy_port());
  EXPECT_EQ("ssl_proxy", settings.ssl_proxy());
  EXPECT_EQ(2222, settings.ssl_proxy_port());
  EXPECT_EQ("ftp_proxy", settings.ftp_proxy());
  EXPECT_EQ(3333, settings.ftp_proxy_port());
  EXPECT_EQ("gopher_proxy", settings.gopher_proxy());
  EXPECT_EQ(4444, settings.gopher_proxy_port());
  EXPECT_EQ("socks_host", settings.socks_host());
  EXPECT_EQ(5555, settings.socks_port());
  EXPECT_EQ(FirefoxProxySettings::V4, settings.socks_version());
  ASSERT_EQ(3U, settings.proxy_bypass_list().size());
  EXPECT_EQ("localhost", settings.proxy_bypass_list()[0]);
  EXPECT_EQ("127.0.0.1", settings.proxy_bypass_list()[1]);
  EXPECT_EQ("noproxy.com", settings.proxy_bypass_list()[2]);
  EXPECT_EQ("", settings.autoconfig_url());

  // Test that ToProxyConfig() properly translates into a net::ProxyConfig.
  net::ProxyConfig config;
  EXPECT_TRUE(settings.ToProxyConfig(&config));

  EXPECT_EQ(
      "Automatic settings:\n"
      "  Auto-detect: No\n"
      "  Custom PAC script: [None]\n"
      "Manual settings:\n"
      "  Proxy server: \n"
      "    HTTP: http_proxy:1111\n"
      "    HTTPS: ssl_proxy:2222\n"
      "    FTP: ftp_proxy:3333\n"
      "    (fallback): socks4://socks_host:5555\n"
      "  Bypass list: \n"
      "    *localhost\n"
      "    127.0.0.1\n"
      "    *noproxy.com",
      config.ToString());
}

TEST_F(FirefoxProxySettingsTest, TestParseAutoConfigUrl) {
  FirefoxProxySettings settings;

  FilePath js_pref_path;
  ASSERT_TRUE(PathService::Get(chrome::DIR_TEST_DATA, &js_pref_path));
  js_pref_path = js_pref_path.AppendASCII("firefox3_pref_pac_url.js");

  EXPECT_TRUE(TestFirefoxProxySettings::TestGetSettingsFromFile(js_pref_path,
                                                                &settings));
  EXPECT_EQ(FirefoxProxySettings::AUTO_FROM_URL, settings.config_type());

  // Everything should be empty except for the autoconfig URL.
  EXPECT_EQ("http://custom-pac-url/", settings.autoconfig_url());
  EXPECT_EQ("", settings.http_proxy());
  EXPECT_EQ(0, settings.http_proxy_port());
  EXPECT_EQ("", settings.ssl_proxy());
  EXPECT_EQ(0, settings.ssl_proxy_port());
  EXPECT_EQ("", settings.ftp_proxy());
  EXPECT_EQ(0, settings.ftp_proxy_port());
  EXPECT_EQ("", settings.gopher_proxy());
  EXPECT_EQ(0, settings.gopher_proxy_port());
  EXPECT_EQ("", settings.socks_host());
  EXPECT_EQ(0, settings.socks_port());
  EXPECT_EQ(0, settings.socks_port());
  EXPECT_EQ(0U, settings.proxy_bypass_list().size());

  // Test that ToProxyConfig() properly translates into a net::ProxyConfig.
  net::ProxyConfig config;
  EXPECT_TRUE(settings.ToProxyConfig(&config));

  EXPECT_EQ(
      "Automatic settings:\n"
      "  Auto-detect: No\n"
      "  Custom PAC script: http://custom-pac-url/\n"
      "Manual settings:\n"
      "  Proxy server: [None]\n"
      "  Bypass list: [None]",
      config.ToString());
}
