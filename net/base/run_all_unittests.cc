// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "base/metrics/histogram.h"
#include "base/nss_util.h"
#include "net/base/net_test_suite.h"
#include "net/http/http_stream_factory.h"
#if defined(OS_WIN)
#include "net/socket/ssl_client_socket_nss_factory.h"
#endif

int main(int argc, char** argv) {
  // Record histograms, so we can get histograms data in tests.
  base::StatisticsRecorder recorder;
  NetTestSuite test_suite(argc, argv);

#if defined(OS_WIN)
  // Use NSS for SSL on Windows.  TODO(wtc): this should eventually be hidden
  // inside DefaultClientSocketFactory::CreateSSLClientSocket.
  net::ClientSocketFactory::SetSSLClientSocketFactory(
      net::SSLClientSocketNSSFactory);
  // We want to be sure to init NSPR on the main thread.
  base::EnsureNSPRInit();
#endif

  net::HttpStreamFactory::set_create_new_spdy_session_for_http(true);

  return test_suite.Run();
}
