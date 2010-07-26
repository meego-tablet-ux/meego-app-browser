// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TEST_TEST_SERVER_H_
#define NET_TEST_TEST_SERVER_H_
#pragma once

#include "build/build_config.h"

#include <string>

#include "base/compiler_specific.h"
#include "base/file_path.h"
#include "base/process_util.h"

#if defined(OS_WIN)
#include "base/scoped_handle_win.h"
#endif

#if defined(USE_NSS)
#include "base/ref_counted.h"
#include "net/base/x509_certificate.h"
#endif

namespace net {

// This object bounds the lifetime of an external python-based HTTP/HTTPS/FTP
// server that can provide various responses useful for testing.
// A few basic convenience methods are provided, but no
// URL handling methods (those belong at a higher layer, e.g. in
// url_request_unittest.h).

class TestServerLauncher {
 public:
  TestServerLauncher();
  virtual ~TestServerLauncher();

  enum Protocol {
    ProtoHTTP, ProtoFTP
  };

  // Load the test root cert, if it hasn't been loaded yet.
  bool LoadTestRootCert() WARN_UNUSED_RESULT;

  // Start src/net/tools/testserver/testserver.py and
  // ask it to serve the given protocol.
  // If protocol is HTTP, and cert_path is not empty, serves HTTPS.
  // file_root_url specifies the root url on the server that documents will be
  // served out of. This is /files/ by default.
  // Returns true on success, false if files not found or root cert
  // not trusted.
  bool Start(net::TestServerLauncher::Protocol protocol,
             const std::string& host_name, int port,
             const FilePath& document_root,
             const FilePath& cert_path,
             const std::wstring& file_root_url) WARN_UNUSED_RESULT;

  // Stop the server started by Start().
  bool Stop();

  // If you access the server's Kill url, it will exit by itself
  // without a call to Stop().
  // WaitToFinish is handy in that case.
  // It returns true if the server exited cleanly.
  bool WaitToFinish(int milliseconds) WARN_UNUSED_RESULT;

  // Paths to a good, an expired, and an invalid server certificate
  // (use as arguments to Start()).
  FilePath GetOKCertPath();
  FilePath GetExpiredCertPath();

  FilePath GetDocumentRootPath() { return document_root_dir_; }

  // Issuer name of the root cert that should be trusted for the test to work.
  static const wchar_t kCertIssuerName[];

  // Hostname to use for test server
  static const char kHostName[];

  // Different hostname to use for test server (that still resolves to same IP)
  static const char kMismatchedHostName[];

  // Port to use for test server
  static const int kOKHTTPSPort;

  // Port to use for bad test server
  static const int kBadHTTPSPort;

 private:
  // Wait a while for the server to start, return whether
  // we were able to make a connection to it.
  bool WaitToStart(const std::string& host_name, int port) WARN_UNUSED_RESULT;

  // Append to PYTHONPATH so Python can find pyftpdlib and tlslite.
  void SetPythonPath();

  // Path to our test root certificate.
  FilePath GetRootCertPath();

  // Returns false if our test root certificate is not trusted.
  bool CheckCATrusted() WARN_UNUSED_RESULT;

  // Initilize the certificate path.
  void InitCertPath();

  FilePath document_root_dir_;

  FilePath cert_dir_;

  base::ProcessHandle process_handle_;

#if defined(OS_WIN)
  // JobObject used to clean up orphaned child processes.
  ScopedHandle job_handle_;
#endif

#if defined(USE_NSS)
  scoped_refptr<X509Certificate> cert_;
#endif

  DISALLOW_COPY_AND_ASSIGN(TestServerLauncher);
};

#if defined(OS_WIN)
// Launch test server as a job so that it is not orphaned if the test case is
// abnormally terminated.
bool LaunchTestServerAsJob(const std::wstring& cmdline,
                           bool start_hidden,
                           base::ProcessHandle* process_handle,
                           ScopedHandle* job_handle);
#endif

}  // namespace net

#endif  // NET_TEST_TEST_SERVER_H_
