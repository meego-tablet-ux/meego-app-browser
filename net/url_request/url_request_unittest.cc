// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/url_request/url_request_unittest.h"

#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#include <shlobj.h>
#elif defined(OS_LINUX)
#include "base/nss_init.h"
#endif

#include <algorithm>
#include <string>

#include "base/file_util.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "base/process_util.h"
#include "base/string_piece.h"
#include "base/string_util.h"
#include "net/base/cookie_monster.h"
#include "net/base/load_flags.h"
#include "net/base/load_log.h"
#include "net/base/load_log_unittest.h"
#include "net/base/net_errors.h"
#include "net/base/net_module.h"
#include "net/base/net_util.h"
#include "net/base/upload_data.h"
#include "net/disk_cache/disk_cache.h"
#include "net/ftp/ftp_network_layer.h"
#include "net/http/http_cache.h"
#include "net/http/http_network_layer.h"
#include "net/http/http_response_headers.h"
#include "net/proxy/proxy_service.h"
#include "net/socket/ssl_test_util.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_file_dir_job.h"
#include "net/url_request/url_request_http_job.h"
#include "net/url_request/url_request_test_job.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

using base::Time;

namespace {

class URLRequestTestContext : public URLRequestContext {
 public:
  URLRequestTestContext() {
    host_resolver_ = net::CreateSystemHostResolver();
    proxy_service_ = net::ProxyService::CreateNull();
    ftp_transaction_factory_ = new net::FtpNetworkLayer(host_resolver_);
    ssl_config_service_ = new net::SSLConfigServiceDefaults;
    http_transaction_factory_ =
        new net::HttpCache(
          net::HttpNetworkLayer::CreateFactory(host_resolver_, proxy_service_,
                                               ssl_config_service_),
          disk_cache::CreateInMemoryCacheBackend(0));
    // In-memory cookie store.
    cookie_store_ = new net::CookieMonster();
  }

  virtual ~URLRequestTestContext() {
    delete ftp_transaction_factory_;
    delete http_transaction_factory_;
  }
};

class TestURLRequest : public URLRequest {
 public:
  TestURLRequest(const GURL& url, Delegate* delegate)
      : URLRequest(url, delegate) {
    set_context(new URLRequestTestContext());
  }
};

StringPiece TestNetResourceProvider(int key) {
  return "header";
}

// Do a case-insensitive search through |haystack| for |needle|.
bool ContainsString(const std::string& haystack, const char* needle) {
  std::string::const_iterator it =
      std::search(haystack.begin(),
                  haystack.end(),
                  needle,
                  needle + strlen(needle),
                  CaseInsensitiveCompare<char>());
  return it != haystack.end();
}

void FillBuffer(char* buffer, size_t len) {
  static bool called = false;
  if (!called) {
    called = true;
    int seed = static_cast<int>(Time::Now().ToInternalValue());
    srand(seed);
  }

  for (size_t i = 0; i < len; i++) {
    buffer[i] = static_cast<char>(rand());
    if (!buffer[i])
      buffer[i] = 'g';
  }
}

scoped_refptr<net::UploadData> CreateSimpleUploadData(const char* data) {
  scoped_refptr<net::UploadData> upload = new net::UploadData;
  upload->AppendBytes(data, strlen(data));
  return upload;
}

}  // namespace

// Inherit PlatformTest since we require the autorelease pool on Mac OS X.f
class URLRequestTest : public PlatformTest {
 public:
  ~URLRequestTest() {
    EXPECT_EQ(0u, URLRequest::InstanceTracker::Get()->GetLiveRequests().size());
  }
};

class URLRequestTestHTTP : public URLRequestTest {
 protected:
  static void SetUpTestCase() {
    server_ = HTTPTestServer::CreateForkingServer(
        L"net/data/url_request_unittest/");
  }

  static void TearDownTestCase() {
    server_ = NULL;
  }

  static scoped_refptr<HTTPTestServer> server_;
};

// static
scoped_refptr<HTTPTestServer> URLRequestTestHTTP::server_;

TEST_F(URLRequestTestHTTP, ProxyTunnelRedirectTest) {
  // In this unit test, we're using the HTTPTestServer as a proxy server and
  // issuing a CONNECT request with the magic host name "www.redirect.com".
  // The HTTPTestServer will return a 302 response, which we should not
  // follow.
  ASSERT_TRUE(NULL != server_.get());
  TestDelegate d;
  {
    URLRequest r(GURL("https://www.redirect.com/"), &d);
    std::string proxy("localhost:");
    proxy.append(IntToString(kHTTPDefaultPort));
    r.set_context(new TestURLRequestContext(proxy));

    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();

    EXPECT_EQ(URLRequestStatus::FAILED, r.status().status());
    EXPECT_EQ(net::ERR_TUNNEL_CONNECTION_FAILED, r.status().os_error());
    EXPECT_EQ(1, d.response_started_count());
    // We should not have followed the redirect.
    EXPECT_EQ(0, d.received_redirect_count());
  }
}

TEST_F(URLRequestTestHTTP, UnexpectedServerAuthTest) {
  // In this unit test, we're using the HTTPTestServer as a proxy server and
  // issuing a CONNECT request with the magic host name "www.server-auth.com".
  // The HTTPTestServer will return a 401 response, which we should balk at.
  ASSERT_TRUE(NULL != server_.get());
  TestDelegate d;
  {
    URLRequest r(GURL("https://www.server-auth.com/"), &d);
    std::string proxy("localhost:");
    proxy.append(IntToString(kHTTPDefaultPort));
    r.set_context(new TestURLRequestContext(proxy));

    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();

    EXPECT_EQ(URLRequestStatus::FAILED, r.status().status());
    EXPECT_EQ(net::ERR_TUNNEL_CONNECTION_FAILED, r.status().os_error());
  }
}

TEST_F(URLRequestTestHTTP, GetTest_NoCache) {
  ASSERT_TRUE(NULL != server_.get());
  TestDelegate d;
  {
    TestURLRequest r(server_->TestServerPage(""), &d);

    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();

    EXPECT_EQ(1, d.response_started_count());
    EXPECT_FALSE(d.received_data_before_response());
    EXPECT_NE(0, d.bytes_received());

    // The first and last entries of the LoadLog should be for
    // TYPE_URL_REQUEST_START.
    net::ExpectLogContains(r.load_log(), 0,
                           net::LoadLog::TYPE_URL_REQUEST_START,
                           net::LoadLog::PHASE_BEGIN);
    net::ExpectLogContains(r.load_log(), r.load_log()->events().size() - 1,
                           net::LoadLog::TYPE_URL_REQUEST_START,
                           net::LoadLog::PHASE_END);
  }
}

TEST_F(URLRequestTestHTTP, GetTest) {
  ASSERT_TRUE(NULL != server_.get());
  TestDelegate d;
  {
    TestURLRequest r(server_->TestServerPage(""), &d);

    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();

    EXPECT_EQ(1, d.response_started_count());
    EXPECT_FALSE(d.received_data_before_response());
    EXPECT_NE(0, d.bytes_received());
  }
}

// Test the instance tracking functionality of URLRequest.
TEST_F(URLRequestTest, Tracking) {
  URLRequest::InstanceTracker::Get()->ClearRecentlyDeceased();
  EXPECT_EQ(0u, URLRequest::InstanceTracker::Get()->GetLiveRequests().size());
  EXPECT_EQ(0u,
            URLRequest::InstanceTracker::Get()->GetRecentlyDeceased().size());

  {
    URLRequest req1(GURL("http://req1"), NULL);
    URLRequest req2(GURL("http://req2"), NULL);
    URLRequest req3(GURL("http://req3"), NULL);

    std::vector<URLRequest*> live_reqs =
        URLRequest::InstanceTracker::Get()->GetLiveRequests();
    ASSERT_EQ(3u, live_reqs.size());
    EXPECT_EQ(GURL("http://req1"), live_reqs[0]->original_url());
    EXPECT_EQ(GURL("http://req2"), live_reqs[1]->original_url());
    EXPECT_EQ(GURL("http://req3"), live_reqs[2]->original_url());
  }

  EXPECT_EQ(0u, URLRequest::InstanceTracker::Get()->GetLiveRequests().size());

  URLRequest::InstanceTracker::RecentRequestInfoList recent_reqs =
      URLRequest::InstanceTracker::Get()->GetRecentlyDeceased();

  // Note that the order is reversed from definition order, because
  // this matches the destructor order.
  ASSERT_EQ(3u, recent_reqs.size());
  EXPECT_EQ(GURL("http://req3"), recent_reqs[0].original_url);
  EXPECT_EQ(GURL("http://req2"), recent_reqs[1].original_url);
  EXPECT_EQ(GURL("http://req1"), recent_reqs[2].original_url);
}

// Test the instance tracking functionality of URLRequest.
TEST_F(URLRequestTest, TrackingGraveyardBounded) {
  URLRequest::InstanceTracker::Get()->ClearRecentlyDeceased();
  EXPECT_EQ(0u, URLRequest::InstanceTracker::Get()->GetLiveRequests().size());
  EXPECT_EQ(0u, URLRequest::InstanceTracker::Get()->GetLiveRequests().size());

  const size_t kMaxGraveyardSize =
      URLRequest::InstanceTracker::kMaxGraveyardSize;
  const size_t kMaxURLLen = URLRequest::InstanceTracker::kMaxGraveyardURLSize;

  // Add twice as many requests as will fit in the graveyard.
  for (size_t i = 0; i < kMaxGraveyardSize * 2; ++i)
    URLRequest req(GURL(StringPrintf("http://req%d", i).c_str()), NULL);

  // Check that only the last |kMaxGraveyardSize| requests are in-memory.

  URLRequest::InstanceTracker::RecentRequestInfoList recent_reqs =
      URLRequest::InstanceTracker::Get()->GetRecentlyDeceased();

  ASSERT_EQ(kMaxGraveyardSize, recent_reqs.size());

  for (size_t i = 0; i < kMaxGraveyardSize; ++i) {
    size_t req_number = i + kMaxGraveyardSize;
    GURL url(StringPrintf("http://req%d", req_number).c_str());
    EXPECT_EQ(url, recent_reqs[i].original_url);
  }

  URLRequest::InstanceTracker::Get()->ClearRecentlyDeceased();
  EXPECT_EQ(0u,
            URLRequest::InstanceTracker::Get()->GetRecentlyDeceased().size());

  // Check that very long URLs are truncated.
  std::string big_url_spec("http://");
  big_url_spec.resize(2 * kMaxURLLen, 'x');
  GURL big_url(big_url_spec);
  {
    URLRequest req(big_url, NULL);
  }
  ASSERT_EQ(1u,
            URLRequest::InstanceTracker::Get()->GetRecentlyDeceased().size());
  // The +1 is because GURL canonicalizes with a trailing '/' ... maybe
  // we should just save the std::string rather than the GURL.
  EXPECT_EQ(kMaxURLLen + 1,
            URLRequest::InstanceTracker::Get()->GetRecentlyDeceased()[0]
                .original_url.spec().size());
}

TEST_F(URLRequestTest, QuitTest) {
  // Don't use shared server here because we order it to quit.
  // It would impact other tests.
  scoped_refptr<HTTPTestServer> server =
      HTTPTestServer::CreateServer(L"", NULL);
  ASSERT_TRUE(NULL != server.get());
  server->SendQuit();
  EXPECT_TRUE(server->WaitToFinish(20000));
}

class HTTPSRequestTest : public testing::Test {
};

#if defined(OS_MACOSX)
// Status 6/19/09:
//
// If these tests are enabled on OSX, the first one (HTTPSGetTest)
// will fail.  I didn't track it down explicitly, but did observe that
// the testserver.py kills itself (e.g. "process_util_posix.cc(84)]
// Unable to terminate process.").  tlslite and testserver.py are hard
// to debug (redirection of stdout/stderr to a file so you can't see
// errors; lots of naked "except:" statements, etc), but I did track
// down an SSL auth failure as one cause of it deciding to die
// silently.
//
// The next test, HTTPSMismatchedTest, will look like it hangs by
// looping over calls to SSLHandshake() (Security framework call) as
// called from SSLClientSocketMac::DoHandshake().  Return values are a
// repeating pattern of -9803 (come back later) and -9812 (cert valid
// but root not trusted).  If you don't have the cert in your keychain
// as documented on http://dev.chromium.org/developers/testing, the
// -9812 becomes a -9813 (no root cert).  Interestingly, the handshake
// also appears to be a failure point for other disabled tests, such
// as (SSLClientSocketTest,Connect) in
// net/base/ssl_client_socket_unittest.cc.
//
// Old comment (not sure if obsolete):
// ssl_client_socket_mac.cc crashes currently in GetSSLInfo
// when called on a connection with an unrecognized certificate
#define MAYBE_HTTPSGetTest        DISABLED_HTTPSGetTest
#define MAYBE_HTTPSMismatchedTest DISABLED_HTTPSMismatchedTest
#define MAYBE_HTTPSExpiredTest    DISABLED_HTTPSExpiredTest
#else
#define MAYBE_HTTPSGetTest        HTTPSGetTest
#define MAYBE_HTTPSMismatchedTest HTTPSMismatchedTest
#define MAYBE_HTTPSExpiredTest    HTTPSExpiredTest
#endif

TEST_F(HTTPSRequestTest, MAYBE_HTTPSGetTest) {
  // Note: tools/testserver/testserver.py does not need
  // a working document root to server the pages / and /hello.html,
  // so this test doesn't really need to specify a document root.
  // But if it did, a good one would be net/data/ssl.
  scoped_refptr<HTTPSTestServer> server =
      HTTPSTestServer::CreateGoodServer(L"net/data/ssl");
  ASSERT_TRUE(NULL != server.get());

  TestDelegate d;
  {
    TestURLRequest r(server->TestServerPage(""), &d);

    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();

    EXPECT_EQ(1, d.response_started_count());
    EXPECT_FALSE(d.received_data_before_response());
    EXPECT_NE(0, d.bytes_received());
  }
}

TEST_F(HTTPSRequestTest, MAYBE_HTTPSMismatchedTest) {
  scoped_refptr<HTTPSTestServer> server =
      HTTPSTestServer::CreateMismatchedServer(L"net/data/ssl");
  ASSERT_TRUE(NULL != server.get());

  bool err_allowed = true;
  for (int i = 0; i < 2 ; i++, err_allowed = !err_allowed) {
    TestDelegate d;
    {
      d.set_allow_certificate_errors(err_allowed);
      TestURLRequest r(server->TestServerPage(""), &d);

      r.Start();
      EXPECT_TRUE(r.is_pending());

      MessageLoop::current()->Run();

      EXPECT_EQ(1, d.response_started_count());
      EXPECT_FALSE(d.received_data_before_response());
      EXPECT_TRUE(d.have_certificate_errors());
      if (err_allowed)
        EXPECT_NE(0, d.bytes_received());
      else
        EXPECT_EQ(0, d.bytes_received());
    }
  }
}

TEST_F(HTTPSRequestTest, MAYBE_HTTPSExpiredTest) {
  scoped_refptr<HTTPSTestServer> server =
      HTTPSTestServer::CreateExpiredServer(L"net/data/ssl");
  ASSERT_TRUE(NULL != server.get());

  // Iterate from false to true, just so that we do the opposite of the
  // previous test in order to increase test coverage.
  bool err_allowed = false;
  for (int i = 0; i < 2 ; i++, err_allowed = !err_allowed) {
    TestDelegate d;
    {
      d.set_allow_certificate_errors(err_allowed);
      TestURLRequest r(server->TestServerPage(""), &d);

      r.Start();
      EXPECT_TRUE(r.is_pending());

      MessageLoop::current()->Run();

      EXPECT_EQ(1, d.response_started_count());
      EXPECT_FALSE(d.received_data_before_response());
      EXPECT_TRUE(d.have_certificate_errors());
      if (err_allowed)
        EXPECT_NE(0, d.bytes_received());
      else
        EXPECT_EQ(0, d.bytes_received());
    }
  }
}

TEST_F(URLRequestTestHTTP, CancelTest) {
  TestDelegate d;
  {
    TestURLRequest r(GURL("http://www.google.com/"), &d);

    r.Start();
    EXPECT_TRUE(r.is_pending());

    r.Cancel();

    MessageLoop::current()->Run();

    // We expect to receive OnResponseStarted even though the request has been
    // cancelled.
    EXPECT_EQ(1, d.response_started_count());
    EXPECT_EQ(0, d.bytes_received());
    EXPECT_FALSE(d.received_data_before_response());
  }
}

TEST_F(URLRequestTestHTTP, CancelTest2) {
  ASSERT_TRUE(NULL != server_.get());

  // error C2446: '!=' : no conversion from 'HTTPTestServer *const '
  // to 'const int'

  TestDelegate d;
  {
    TestURLRequest r(server_->TestServerPage(""), &d);

    d.set_cancel_in_response_started(true);

    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();

    EXPECT_EQ(1, d.response_started_count());
    EXPECT_EQ(0, d.bytes_received());
    EXPECT_FALSE(d.received_data_before_response());
    EXPECT_EQ(URLRequestStatus::CANCELED, r.status().status());
  }
}

TEST_F(URLRequestTestHTTP, CancelTest3) {
  ASSERT_TRUE(NULL != server_.get());
  TestDelegate d;
  {
    TestURLRequest r(server_->TestServerPage(""), &d);

    d.set_cancel_in_received_data(true);

    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();

    EXPECT_EQ(1, d.response_started_count());
    // There is no guarantee about how much data was received
    // before the cancel was issued.  It could have been 0 bytes,
    // or it could have been all the bytes.
    // EXPECT_EQ(0, d.bytes_received());
    EXPECT_FALSE(d.received_data_before_response());
    EXPECT_EQ(URLRequestStatus::CANCELED, r.status().status());
  }
}

TEST_F(URLRequestTestHTTP, CancelTest4) {
  ASSERT_TRUE(NULL != server_.get());
  TestDelegate d;
  {
    TestURLRequest r(server_->TestServerPage(""), &d);

    r.Start();
    EXPECT_TRUE(r.is_pending());

    // The request will be implicitly canceled when it is destroyed. The
    // test delegate must not post a quit message when this happens because
    // this test doesn't actually have a message loop. The quit message would
    // get put on this thread's message queue and the next test would exit
    // early, causing problems.
    d.set_quit_on_complete(false);
  }
  // expect things to just cleanup properly.

  // we won't actually get a received reponse here because we've never run the
  // message loop
  EXPECT_FALSE(d.received_data_before_response());
  EXPECT_EQ(0, d.bytes_received());
}

TEST_F(URLRequestTestHTTP, CancelTest5) {
  ASSERT_TRUE(NULL != server_.get());
  scoped_refptr<URLRequestContext> context = new URLRequestTestContext();

  // populate cache
  {
    TestDelegate d;
    URLRequest r(server_->TestServerPage("cachetime"), &d);
    r.set_context(context);
    r.Start();
    MessageLoop::current()->Run();
    EXPECT_EQ(URLRequestStatus::SUCCESS, r.status().status());
  }

  // cancel read from cache (see bug 990242)
  {
    TestDelegate d;
    URLRequest r(server_->TestServerPage("cachetime"), &d);
    r.set_context(context);
    r.Start();
    r.Cancel();
    MessageLoop::current()->Run();

    EXPECT_EQ(URLRequestStatus::CANCELED, r.status().status());
    EXPECT_EQ(1, d.response_started_count());
    EXPECT_EQ(0, d.bytes_received());
    EXPECT_FALSE(d.received_data_before_response());
  }
}

TEST_F(URLRequestTestHTTP, PostTest) {
  ASSERT_TRUE(NULL != server_.get());
  const int kMsgSize = 20000;  // multiple of 10
  const int kIterations = 50;
  char *uploadBytes = new char[kMsgSize+1];
  char *ptr = uploadBytes;
  char marker = 'a';
  for (int idx = 0; idx < kMsgSize/10; idx++) {
    memcpy(ptr, "----------", 10);
    ptr += 10;
    if (idx % 100 == 0) {
      ptr--;
      *ptr++ = marker;
      if (++marker > 'z')
        marker = 'a';
    }
  }
  uploadBytes[kMsgSize] = '\0';

  scoped_refptr<URLRequestContext> context = new URLRequestTestContext();

  for (int i = 0; i < kIterations; ++i) {
    TestDelegate d;
    URLRequest r(server_->TestServerPage("echo"), &d);
    r.set_context(context);
    r.set_method("POST");

    r.AppendBytesToUpload(uploadBytes, kMsgSize);

    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();

    ASSERT_EQ(1, d.response_started_count()) << "request failed: " <<
        (int) r.status().status() << ", os error: " << r.status().os_error();

    EXPECT_FALSE(d.received_data_before_response());
    EXPECT_EQ(uploadBytes, d.data_received());
    EXPECT_EQ(memcmp(uploadBytes, d.data_received().c_str(), kMsgSize), 0);
    EXPECT_EQ(d.data_received().compare(uploadBytes), 0);
  }
  delete[] uploadBytes;
}

TEST_F(URLRequestTestHTTP, PostEmptyTest) {
  ASSERT_TRUE(NULL != server_.get());
  TestDelegate d;
  {
    TestURLRequest r(server_->TestServerPage("echo"), &d);
    r.set_method("POST");

    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();

    ASSERT_EQ(1, d.response_started_count()) << "request failed: " <<
        (int) r.status().status() << ", os error: " << r.status().os_error();

    EXPECT_FALSE(d.received_data_before_response());
    EXPECT_TRUE(d.data_received().empty());
  }
}

TEST_F(URLRequestTestHTTP, PostFileTest) {
  ASSERT_TRUE(NULL != server_.get());
  TestDelegate d;
  {
    TestURLRequest r(server_->TestServerPage("echo"), &d);
    r.set_method("POST");

    FilePath dir;
    PathService::Get(base::DIR_EXE, &dir);
    file_util::SetCurrentDirectory(dir);

    FilePath path;
    PathService::Get(base::DIR_SOURCE_ROOT, &path);
    path = path.Append(FILE_PATH_LITERAL("net"));
    path = path.Append(FILE_PATH_LITERAL("data"));
    path = path.Append(FILE_PATH_LITERAL("url_request_unittest"));
    path = path.Append(FILE_PATH_LITERAL("with-headers.html"));
    r.AppendFileToUpload(path);

    // This file should just be ignored in the upload stream.
    r.AppendFileToUpload(FilePath(FILE_PATH_LITERAL(
        "c:\\path\\to\\non\\existant\\file.randomness.12345")));

    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();

    int64 longsize;
    ASSERT_EQ(true, file_util::GetFileSize(path, &longsize));
    int size = static_cast<int>(longsize);
    scoped_array<char> buf(new char[size]);

    int size_read = static_cast<int>(file_util::ReadFile(path,
        buf.get(), size));
    ASSERT_EQ(size, size_read);

    ASSERT_EQ(1, d.response_started_count()) << "request failed: " <<
        (int) r.status().status() << ", os error: " << r.status().os_error();

    EXPECT_FALSE(d.received_data_before_response());

    ASSERT_EQ(size, d.bytes_received());
    EXPECT_EQ(0, memcmp(d.data_received().c_str(), buf.get(), size));
  }
}

TEST_F(URLRequestTest, AboutBlankTest) {
  TestDelegate d;
  {
    TestURLRequest r(GURL("about:blank"), &d);

    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();

    EXPECT_TRUE(!r.is_pending());
    EXPECT_FALSE(d.received_data_before_response());
    EXPECT_EQ(d.bytes_received(), 0);
  }
}

TEST_F(URLRequestTest, FileTest) {
  FilePath app_path;
  PathService::Get(base::FILE_EXE, &app_path);
  GURL app_url = net::FilePathToFileURL(app_path);

  TestDelegate d;
  {
    TestURLRequest r(app_url, &d);

    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();

    int64 file_size = -1;
    EXPECT_TRUE(file_util::GetFileSize(app_path, &file_size));

    EXPECT_TRUE(!r.is_pending());
    EXPECT_EQ(1, d.response_started_count());
    EXPECT_FALSE(d.received_data_before_response());
    EXPECT_EQ(d.bytes_received(), static_cast<int>(file_size));
  }
}

TEST_F(URLRequestTest, FileTestFullSpecifiedRange) {
  const size_t buffer_size = 4000;
  scoped_array<char> buffer(new char[buffer_size]);
  FillBuffer(buffer.get(), buffer_size);

  FilePath temp_path;
  EXPECT_TRUE(file_util::CreateTemporaryFile(&temp_path));
  GURL temp_url = net::FilePathToFileURL(temp_path);
  file_util::WriteFile(temp_path, buffer.get(), buffer_size);

  int64 file_size;
  EXPECT_TRUE(file_util::GetFileSize(temp_path, &file_size));

  const size_t first_byte_position = 500;
  const size_t last_byte_position = buffer_size - first_byte_position;
  const size_t content_length = last_byte_position - first_byte_position + 1;
  std::string partial_buffer_string(buffer.get() + first_byte_position,
                                    buffer.get() + last_byte_position + 1);

  TestDelegate d;
  {
    TestURLRequest r(temp_url, &d);

    r.SetExtraRequestHeaders(StringPrintf("Range: bytes=%d-%d\n",
                                          first_byte_position,
                                          last_byte_position));
    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();
    EXPECT_TRUE(!r.is_pending());
    EXPECT_EQ(1, d.response_started_count());
    EXPECT_FALSE(d.received_data_before_response());
    EXPECT_EQ(static_cast<int>(content_length), d.bytes_received());
    // Don't use EXPECT_EQ, it will print out a lot of garbage if check failed.
    EXPECT_TRUE(partial_buffer_string == d.data_received());
  }

  EXPECT_TRUE(file_util::Delete(temp_path, false));
}

TEST_F(URLRequestTest, FileTestHalfSpecifiedRange) {
  const size_t buffer_size = 4000;
  scoped_array<char> buffer(new char[buffer_size]);
  FillBuffer(buffer.get(), buffer_size);

  FilePath temp_path;
  EXPECT_TRUE(file_util::CreateTemporaryFile(&temp_path));
  GURL temp_url = net::FilePathToFileURL(temp_path);
  file_util::WriteFile(temp_path, buffer.get(), buffer_size);

  int64 file_size;
  EXPECT_TRUE(file_util::GetFileSize(temp_path, &file_size));

  const size_t first_byte_position = 500;
  const size_t last_byte_position = buffer_size - 1;
  const size_t content_length = last_byte_position - first_byte_position + 1;
  std::string partial_buffer_string(buffer.get() + first_byte_position,
                                    buffer.get() + last_byte_position + 1);

  TestDelegate d;
  {
    TestURLRequest r(temp_url, &d);

    r.SetExtraRequestHeaders(StringPrintf("Range: bytes=%d-\n",
                                          first_byte_position));
    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();
    EXPECT_TRUE(!r.is_pending());
    EXPECT_EQ(1, d.response_started_count());
    EXPECT_FALSE(d.received_data_before_response());
    EXPECT_EQ(static_cast<int>(content_length), d.bytes_received());
    // Don't use EXPECT_EQ, it will print out a lot of garbage if check failed.
    EXPECT_TRUE(partial_buffer_string == d.data_received());
  }

  EXPECT_TRUE(file_util::Delete(temp_path, false));
}

TEST_F(URLRequestTest, FileTestMultipleRanges) {
  const size_t buffer_size = 400000;
  scoped_array<char> buffer(new char[buffer_size]);
  FillBuffer(buffer.get(), buffer_size);

  FilePath temp_path;
  EXPECT_TRUE(file_util::CreateTemporaryFile(&temp_path));
  GURL temp_url = net::FilePathToFileURL(temp_path);
  file_util::WriteFile(temp_path, buffer.get(), buffer_size);

  int64 file_size;
  EXPECT_TRUE(file_util::GetFileSize(temp_path, &file_size));

  TestDelegate d;
  {
    TestURLRequest r(temp_url, &d);

    r.SetExtraRequestHeaders(StringPrintf("Range: bytes=0-0,10-200,200-300\n"));
    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();
    EXPECT_TRUE(d.request_failed());
  }

  EXPECT_TRUE(file_util::Delete(temp_path, false));
}

TEST_F(URLRequestTest, InvalidUrlTest) {
  TestDelegate d;
  {
    TestURLRequest r(GURL("invalid url"), &d);

    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();
    EXPECT_TRUE(d.request_failed());
  }
}

// This test is disabled because it fails on some computers due to proxies
// returning a page in response to this request rather than reporting failure.
TEST_F(URLRequestTest, DISABLED_DnsFailureTest) {
  TestDelegate d;
  {
    URLRequest r(GURL("http://thisisnotavalidurl0123456789foo.com/"), &d);

    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();
    EXPECT_TRUE(d.request_failed());
  }
}

TEST_F(URLRequestTestHTTP, ResponseHeadersTest) {
  ASSERT_TRUE(NULL != server_.get());
  TestDelegate d;
  TestURLRequest req(server_->TestServerPage("files/with-headers.html"), &d);
  req.Start();
  MessageLoop::current()->Run();

  const net::HttpResponseHeaders* headers = req.response_headers();
  std::string header;
  EXPECT_TRUE(headers->GetNormalizedHeader("cache-control", &header));
  EXPECT_EQ("private", header);

  header.clear();
  EXPECT_TRUE(headers->GetNormalizedHeader("content-type", &header));
  EXPECT_EQ("text/html; charset=ISO-8859-1", header);

  // The response has two "X-Multiple-Entries" headers.
  // This verfies our output has them concatenated together.
  header.clear();
  EXPECT_TRUE(headers->GetNormalizedHeader("x-multiple-entries", &header));
  EXPECT_EQ("a, b", header);
}

// TODO(jar): 14801 Remove BZIP code completely.
TEST_F(URLRequestTest, DISABLED_BZip2ContentTest) {
  scoped_refptr<HTTPTestServer> server =
      HTTPTestServer::CreateServer(L"net/data/filter_unittests", NULL);
  ASSERT_TRUE(NULL != server.get());

  // for localhost domain, we also should support bzip2 encoding
  // first, get the original file
  TestDelegate d1;
  TestURLRequest req1(server->TestServerPage("realfiles/google.txt"), &d1);
  req1.Start();
  MessageLoop::current()->Run();

  const std::string& got_content = d1.data_received();

  // second, get bzip2 content
  TestDelegate d2;
  TestURLRequest req2(server->TestServerPage("realbz2files/google.txt"), &d2);
  req2.Start();
  MessageLoop::current()->Run();

  const std::string& got_bz2_content = d2.data_received();

  // compare those two results
  EXPECT_EQ(got_content, got_bz2_content);
}

// TODO(jar): 14801 Remove BZIP code completely.
TEST_F(URLRequestTest, DISABLED_BZip2ContentTest_IncrementalHeader) {
  scoped_refptr<HTTPTestServer> server =
      HTTPTestServer::CreateServer(L"net/data/filter_unittests", NULL);
  ASSERT_TRUE(NULL != server.get());

  // for localhost domain, we also should support bzip2 encoding
  // first, get the original file
  TestDelegate d1;
  TestURLRequest req1(server->TestServerPage("realfiles/google.txt"), &d1);
  req1.Start();
  MessageLoop::current()->Run();

  const std::string& got_content = d1.data_received();

  // second, get bzip2 content.  ask the testserver to send the BZ2 header in
  // two chunks with a delay between them.  this tests our fix for bug 867161.
  TestDelegate d2;
  TestURLRequest req2(server->TestServerPage(
      "realbz2files/google.txt?incremental-header"), &d2);
  req2.Start();
  MessageLoop::current()->Run();

  const std::string& got_bz2_content = d2.data_received();

  // compare those two results
  EXPECT_EQ(got_content, got_bz2_content);
}

#if defined(OS_WIN)
TEST_F(URLRequestTest, ResolveShortcutTest) {
  FilePath app_path;
  PathService::Get(base::DIR_SOURCE_ROOT, &app_path);
  app_path = app_path.AppendASCII("net");
  app_path = app_path.AppendASCII("data");
  app_path = app_path.AppendASCII("url_request_unittest");
  app_path = app_path.AppendASCII("with-headers.html");

  std::wstring lnk_path = app_path.value() + L".lnk";

  HRESULT result;
  IShellLink *shell = NULL;
  IPersistFile *persist = NULL;

  CoInitialize(NULL);
  // Temporarily create a shortcut for test
  result = CoCreateInstance(CLSID_ShellLink, NULL,
                            CLSCTX_INPROC_SERVER, IID_IShellLink,
                            reinterpret_cast<LPVOID*>(&shell));
  ASSERT_TRUE(SUCCEEDED(result));
  result = shell->QueryInterface(IID_IPersistFile,
                                reinterpret_cast<LPVOID*>(&persist));
  ASSERT_TRUE(SUCCEEDED(result));
  result = shell->SetPath(app_path.value().c_str());
  EXPECT_TRUE(SUCCEEDED(result));
  result = shell->SetDescription(L"ResolveShortcutTest");
  EXPECT_TRUE(SUCCEEDED(result));
  result = persist->Save(lnk_path.c_str(), TRUE);
  EXPECT_TRUE(SUCCEEDED(result));
  if (persist)
    persist->Release();
  if (shell)
    shell->Release();

  TestDelegate d;
  {
    TestURLRequest r(net::FilePathToFileURL(FilePath(lnk_path)), &d);

    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();

    WIN32_FILE_ATTRIBUTE_DATA data;
    GetFileAttributesEx(app_path.value().c_str(),
                        GetFileExInfoStandard, &data);
    HANDLE file = CreateFile(app_path.value().c_str(), GENERIC_READ,
                             FILE_SHARE_READ, NULL, OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL, NULL);
    EXPECT_NE(INVALID_HANDLE_VALUE, file);
    scoped_array<char> buffer(new char[data.nFileSizeLow]);
    DWORD read_size;
    BOOL result;
    result = ReadFile(file, buffer.get(), data.nFileSizeLow,
                      &read_size, NULL);
    std::string content(buffer.get(), read_size);
    CloseHandle(file);

    EXPECT_TRUE(!r.is_pending());
    EXPECT_EQ(1, d.received_redirect_count());
    EXPECT_EQ(content, d.data_received());
  }

  // Clean the shortcut
  DeleteFile(lnk_path.c_str());
  CoUninitialize();
}
#endif  // defined(OS_WIN)

TEST_F(URLRequestTestHTTP, ContentTypeNormalizationTest) {
  ASSERT_TRUE(NULL != server_.get());

  TestDelegate d;
  TestURLRequest req(server_->TestServerPage(
      "files/content-type-normalization.html"), &d);
  req.Start();
  MessageLoop::current()->Run();

  std::string mime_type;
  req.GetMimeType(&mime_type);
  EXPECT_EQ("text/html", mime_type);

  std::string charset;
  req.GetCharset(&charset);
  EXPECT_EQ("utf-8", charset);
  req.Cancel();
}

TEST_F(URLRequestTest, FileDirCancelTest) {
  // Put in mock resource provider.
  net::NetModule::SetResourceProvider(TestNetResourceProvider);

  TestDelegate d;
  {
    FilePath file_path;
    PathService::Get(base::DIR_SOURCE_ROOT, &file_path);
    file_path = file_path.Append(FILE_PATH_LITERAL("net"));
    file_path = file_path.Append(FILE_PATH_LITERAL("data"));

    TestURLRequest req(net::FilePathToFileURL(file_path), &d);
    req.Start();
    EXPECT_TRUE(req.is_pending());

    d.set_cancel_in_received_data_pending(true);

    MessageLoop::current()->Run();
  }

  // Take out mock resource provider.
  net::NetModule::SetResourceProvider(NULL);
}

TEST_F(URLRequestTest, FileDirRedirectNoCrash) {
  // There is an implicit redirect when loading a file path that matches a
  // directory and does not end with a slash.  Ensure that following such
  // redirects does not crash.  See http://crbug.com/18686.

  FilePath path;
  PathService::Get(base::DIR_SOURCE_ROOT, &path);
  path = path.Append(FILE_PATH_LITERAL("net"));
  path = path.Append(FILE_PATH_LITERAL("data"));
  path = path.Append(FILE_PATH_LITERAL("url_request_unittest"));

  TestDelegate d;
  d.set_quit_on_redirect(true);
  TestURLRequest req(net::FilePathToFileURL(path), &d);
  req.Start();
  MessageLoop::current()->Run();

  // Let the directory lister have time to finish its work, which will
  // cause the URLRequestFileDirJob's ref count to drop to 1.
  URLRequestFileDirJob* job = static_cast<URLRequestFileDirJob*>(req.job());
  while (!job->list_complete()) {
    PlatformThread::Sleep(10);
    MessageLoop::current()->RunAllPending();
  }

  // Should not crash during this call!
  req.FollowDeferredRedirect();

  // Flush event queue.
  MessageLoop::current()->RunAllPending();
}

TEST_F(URLRequestTestHTTP, RestrictRedirects) {
  ASSERT_TRUE(NULL != server_.get());

  TestDelegate d;
  TestURLRequest req(server_->TestServerPage(
      "files/redirect-to-file.html"), &d);
  req.Start();
  MessageLoop::current()->Run();

  EXPECT_EQ(URLRequestStatus::FAILED, req.status().status());
  EXPECT_EQ(net::ERR_UNSAFE_REDIRECT, req.status().os_error());
}

TEST_F(URLRequestTestHTTP, RedirectToInvalidURL) {
  ASSERT_TRUE(NULL != server_.get());

  TestDelegate d;
  TestURLRequest req(server_->TestServerPage(
      "files/redirect-to-invalid-url.html"), &d);
  req.Start();
  MessageLoop::current()->Run();

  EXPECT_EQ(URLRequestStatus::FAILED, req.status().status());
  EXPECT_EQ(net::ERR_INVALID_URL, req.status().os_error());
}

TEST_F(URLRequestTestHTTP, NoUserPassInReferrer) {
  ASSERT_TRUE(NULL != server_.get());
  TestDelegate d;
  TestURLRequest req(server_->TestServerPage(
      "echoheader?Referer"), &d);
  req.set_referrer("http://user:pass@foo.com/");
  req.Start();
  MessageLoop::current()->Run();

  EXPECT_EQ(std::string("http://foo.com/"), d.data_received());
}

TEST_F(URLRequestTestHTTP, CancelRedirect) {
  ASSERT_TRUE(NULL != server_.get());
  TestDelegate d;
  {
    d.set_cancel_in_received_redirect(true);
    TestURLRequest req(server_->TestServerPage(
        "files/redirect-test.html"), &d);
    req.Start();
    MessageLoop::current()->Run();

    EXPECT_EQ(1, d.response_started_count());
    EXPECT_EQ(0, d.bytes_received());
    EXPECT_FALSE(d.received_data_before_response());
    EXPECT_EQ(URLRequestStatus::CANCELED, req.status().status());
  }
}

TEST_F(URLRequestTestHTTP, DeferredRedirect) {
  ASSERT_TRUE(NULL != server_.get());
  TestDelegate d;
  {
    d.set_quit_on_redirect(true);
    TestURLRequest req(server_->TestServerPage(
        "files/redirect-test.html"), &d);
    req.Start();
    MessageLoop::current()->Run();

    EXPECT_EQ(1, d.received_redirect_count());

    req.FollowDeferredRedirect();
    MessageLoop::current()->Run();

    EXPECT_EQ(1, d.response_started_count());
    EXPECT_FALSE(d.received_data_before_response());
    EXPECT_EQ(URLRequestStatus::SUCCESS, req.status().status());

    FilePath path;
    PathService::Get(base::DIR_SOURCE_ROOT, &path);
    path = path.Append(FILE_PATH_LITERAL("net"));
    path = path.Append(FILE_PATH_LITERAL("data"));
    path = path.Append(FILE_PATH_LITERAL("url_request_unittest"));
    path = path.Append(FILE_PATH_LITERAL("with-headers.html"));

    std::string contents;
    EXPECT_TRUE(file_util::ReadFileToString(path, &contents));
    EXPECT_EQ(contents, d.data_received());
  }
}

TEST_F(URLRequestTestHTTP, CancelDeferredRedirect) {
  ASSERT_TRUE(NULL != server_.get());
  TestDelegate d;
  {
    d.set_quit_on_redirect(true);
    TestURLRequest req(server_->TestServerPage(
        "files/redirect-test.html"), &d);
    req.Start();
    MessageLoop::current()->Run();

    EXPECT_EQ(1, d.received_redirect_count());

    req.Cancel();
    MessageLoop::current()->Run();

    EXPECT_EQ(1, d.response_started_count());
    EXPECT_EQ(0, d.bytes_received());
    EXPECT_FALSE(d.received_data_before_response());
    EXPECT_EQ(URLRequestStatus::CANCELED, req.status().status());
  }
}

TEST_F(URLRequestTestHTTP, VaryHeader) {
  ASSERT_TRUE(NULL != server_.get());

  scoped_refptr<URLRequestContext> context = new URLRequestTestContext();

  // populate the cache
  {
    TestDelegate d;
    URLRequest req(server_->TestServerPage("echoheader?foo"), &d);
    req.set_context(context);
    req.SetExtraRequestHeaders("foo:1");
    req.Start();
    MessageLoop::current()->Run();
  }

  // expect a cache hit
  {
    TestDelegate d;
    URLRequest req(server_->TestServerPage("echoheader?foo"), &d);
    req.set_context(context);
    req.SetExtraRequestHeaders("foo:1");
    req.Start();
    MessageLoop::current()->Run();

    EXPECT_TRUE(req.was_cached());
  }

  // expect a cache miss
  {
    TestDelegate d;
    URLRequest req(server_->TestServerPage("echoheader?foo"), &d);
    req.set_context(context);
    req.SetExtraRequestHeaders("foo:2");
    req.Start();
    MessageLoop::current()->Run();

    EXPECT_FALSE(req.was_cached());
  }
}

TEST_F(URLRequestTestHTTP, BasicAuth) {
  scoped_refptr<URLRequestContext> context = new URLRequestTestContext();
  ASSERT_TRUE(NULL != server_.get());

  // populate the cache
  {
    TestDelegate d;
    d.set_username(L"user");
    d.set_password(L"secret");

    URLRequest r(server_->TestServerPage("auth-basic"), &d);
    r.set_context(context);
    r.Start();

    MessageLoop::current()->Run();

    EXPECT_TRUE(d.data_received().find("user/secret") != std::string::npos);
  }

  // repeat request with end-to-end validation.  since auth-basic results in a
  // cachable page, we expect this test to result in a 304.  in which case, the
  // response should be fetched from the cache.
  {
    TestDelegate d;
    d.set_username(L"user");
    d.set_password(L"secret");

    URLRequest r(server_->TestServerPage("auth-basic"), &d);
    r.set_context(context);
    r.set_load_flags(net::LOAD_VALIDATE_CACHE);
    r.Start();

    MessageLoop::current()->Run();

    EXPECT_TRUE(d.data_received().find("user/secret") != std::string::npos);

    // Should be the same cached document.
    EXPECT_TRUE(r.was_cached());
  }
}

// Check that Set-Cookie headers in 401 responses are respected.
// http://crbug.com/6450
TEST_F(URLRequestTestHTTP, BasicAuthWithCookies) {
  ASSERT_TRUE(NULL != server_.get());

  GURL url_requiring_auth =
      server_->TestServerPage("auth-basic?set-cookie-if-challenged");

  // Request a page that will give a 401 containing a Set-Cookie header.
  // Verify that when the transaction is restarted, it includes the new cookie.
  {
    scoped_refptr<URLRequestContext> context = new URLRequestTestContext();
    TestDelegate d;
    d.set_username(L"user");
    d.set_password(L"secret");

    URLRequest r(url_requiring_auth, &d);
    r.set_context(context);
    r.Start();

    MessageLoop::current()->Run();

    EXPECT_TRUE(d.data_received().find("user/secret") != std::string::npos);

    // Make sure we sent the cookie in the restarted transaction.
    EXPECT_TRUE(d.data_received().find("Cookie: got_challenged=true")
        != std::string::npos);
  }

  // Same test as above, except this time the restart is initiated earlier
  // (without user intervention since identity is embedded in the URL).
  {
    scoped_refptr<URLRequestContext> context = new URLRequestTestContext();
    TestDelegate d;

    GURL::Replacements replacements;
    std::string username("user2");
    std::string password("secret");
    replacements.SetUsernameStr(username);
    replacements.SetPasswordStr(password);
    GURL url_with_identity = url_requiring_auth.ReplaceComponents(replacements);

    URLRequest r(url_with_identity, &d);
    r.set_context(context);
    r.Start();

    MessageLoop::current()->Run();

    EXPECT_TRUE(d.data_received().find("user2/secret") != std::string::npos);

    // Make sure we sent the cookie in the restarted transaction.
    EXPECT_TRUE(d.data_received().find("Cookie: got_challenged=true")
        != std::string::npos);
  }
}

TEST_F(URLRequestTest, DoNotSendCookies) {
  scoped_refptr<HTTPTestServer> server =
      HTTPTestServer::CreateServer(L"", NULL);
  ASSERT_TRUE(NULL != server.get());
  scoped_refptr<URLRequestContext> context = new URLRequestTestContext();

  // Set up a cookie.
  {
    TestDelegate d;
    URLRequest req(server->TestServerPage("set-cookie?CookieToNotSend=1"), &d);
    req.set_context(context);
    req.Start();
    MessageLoop::current()->Run();
  }

  // Verify that the cookie is set.
  {
    TestDelegate d;
    TestURLRequest req(server->TestServerPage("echoheader?Cookie"), &d);
    req.set_context(context);
    req.Start();
    MessageLoop::current()->Run();

    EXPECT_TRUE(d.data_received().find("CookieToNotSend=1")
                != std::string::npos);
  }

  // Verify that the cookie isn't sent when LOAD_DO_NOT_SEND_COOKIES is set.
  {
    TestDelegate d;
    TestURLRequest req(server->TestServerPage("echoheader?Cookie"), &d);
    req.set_load_flags(net::LOAD_DO_NOT_SEND_COOKIES);
    req.set_context(context);
    req.Start();
    MessageLoop::current()->Run();

    EXPECT_TRUE(d.data_received().find("Cookie: CookieToNotSend=1")
                == std::string::npos);
  }
}

TEST_F(URLRequestTest, DoNotSaveCookies) {
  scoped_refptr<HTTPTestServer> server =
      HTTPTestServer::CreateServer(L"", NULL);
  ASSERT_TRUE(NULL != server.get());
  scoped_refptr<URLRequestContext> context = new URLRequestTestContext();

  // Set up a cookie.
  {
    TestDelegate d;
    URLRequest req(server->TestServerPage("set-cookie?CookieToNotUpdate=2"),
                   &d);
    req.set_context(context);
    req.Start();
    MessageLoop::current()->Run();
  }

  // Try to set-up another cookie and update the previous cookie.
  {
    scoped_refptr<URLRequestContext> context = new URLRequestTestContext();
    TestDelegate d;
    URLRequest req(server->TestServerPage(
        "set-cookie?CookieToNotSave=1&CookieToNotUpdate=1"), &d);
    req.set_load_flags(net::LOAD_DO_NOT_SAVE_COOKIES);
    req.set_context(context);
    req.Start();

    MessageLoop::current()->Run();
  }

  // Verify the cookies weren't saved or updated.
  {
    TestDelegate d;
    TestURLRequest req(server->TestServerPage("echoheader?Cookie"), &d);
    req.set_context(context);
    req.Start();
    MessageLoop::current()->Run();

    EXPECT_TRUE(d.data_received().find("CookieToNotSave=1")
                == std::string::npos);
    EXPECT_TRUE(d.data_received().find("CookieToNotUpdate=2")
                != std::string::npos);
  }
}

// In this test, we do a POST which the server will 302 redirect.
// The subsequent transaction should use GET, and should not send the
// Content-Type header.
// http://code.google.com/p/chromium/issues/detail?id=843
TEST_F(URLRequestTestHTTP, Post302RedirectGet) {
  const char kData[] = "hello world";
  ASSERT_TRUE(NULL != server_.get());
  TestDelegate d;
  TestURLRequest req(server_->TestServerPage("files/redirect-to-echoall"), &d);
  req.set_method("POST");
  req.set_upload(CreateSimpleUploadData(kData));

  // Set headers (some of which are specific to the POST).
  req.SetExtraRequestHeaders(
    "Content-Type: multipart/form-data; "
    "boundary=----WebKitFormBoundaryAADeAA+NAAWMAAwZ\r\n"
    "Accept: text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,"
    "text/plain;q=0.8,image/png,*/*;q=0.5\r\n"
    "Accept-Language: en-US,en\r\n"
    "Accept-Charset: ISO-8859-1,*,utf-8\r\n"
    "Content-Length: 11\r\n"
    "Origin: http://localhost:1337/");
  req.Start();
  MessageLoop::current()->Run();

  std::string mime_type;
  req.GetMimeType(&mime_type);
  EXPECT_EQ("text/html", mime_type);

  const std::string& data = d.data_received();

  // Check that the post-specific headers were stripped:
  EXPECT_FALSE(ContainsString(data, "Content-Length:"));
  EXPECT_FALSE(ContainsString(data, "Content-Type:"));
  EXPECT_FALSE(ContainsString(data, "Origin:"));

  // These extra request headers should not have been stripped.
  EXPECT_TRUE(ContainsString(data, "Accept:"));
  EXPECT_TRUE(ContainsString(data, "Accept-Language:"));
  EXPECT_TRUE(ContainsString(data, "Accept-Charset:"));
}

TEST_F(URLRequestTestHTTP, Post307RedirectPost) {
  const char kData[] = "hello world";
  ASSERT_TRUE(NULL != server_.get());
  TestDelegate d;
  TestURLRequest req(server_->TestServerPage("files/redirect307-to-echo"),
      &d);
  req.set_method("POST");
  req.set_upload(CreateSimpleUploadData(kData).get());
  req.SetExtraRequestHeaders(
      "Content-Length: " + UintToString(sizeof(kData) - 1));
  req.Start();
  MessageLoop::current()->Run();
  EXPECT_EQ("POST", req.method());
  EXPECT_EQ(kData, d.data_received());
}

// Custom URLRequestJobs for use with interceptor tests
class RestartTestJob : public URLRequestTestJob {
 public:
  explicit RestartTestJob(URLRequest* request)
    : URLRequestTestJob(request, true) {}
 protected:
  virtual void StartAsync() {
    this->NotifyRestartRequired();
  }
};

class CancelTestJob : public URLRequestTestJob {
 public:
  explicit CancelTestJob(URLRequest* request)
    : URLRequestTestJob(request, true) {}
 protected:
  virtual void StartAsync() {
    request_->Cancel();
  }
};

class CancelThenRestartTestJob : public URLRequestTestJob {
 public:
  explicit CancelThenRestartTestJob(URLRequest* request)
      : URLRequestTestJob(request, true) {
  }
 protected:
  virtual void StartAsync() {
    request_->Cancel();
    this->NotifyRestartRequired();
  }
};

// An Interceptor for use with interceptor tests
class TestInterceptor : URLRequest::Interceptor {
 public:
  TestInterceptor()
      : intercept_main_request_(false), restart_main_request_(false),
        cancel_main_request_(false), cancel_then_restart_main_request_(false),
        simulate_main_network_error_(false),
        intercept_redirect_(false), cancel_redirect_request_(false),
        intercept_final_response_(false), cancel_final_request_(false),
        did_intercept_main_(false), did_restart_main_(false),
        did_cancel_main_(false), did_cancel_then_restart_main_(false),
        did_simulate_error_main_(false),
        did_intercept_redirect_(false), did_cancel_redirect_(false),
        did_intercept_final_(false), did_cancel_final_(false) {
    URLRequest::RegisterRequestInterceptor(this);
  }

  ~TestInterceptor() {
    URLRequest::UnregisterRequestInterceptor(this);
  }

  virtual URLRequestJob* MaybeIntercept(URLRequest* request) {
    if (restart_main_request_) {
      restart_main_request_ = false;
      did_restart_main_ = true;
      return new RestartTestJob(request);
    }
    if (cancel_main_request_) {
      cancel_main_request_ = false;
      did_cancel_main_ = true;
      return new CancelTestJob(request);
    }
    if (cancel_then_restart_main_request_) {
      cancel_then_restart_main_request_ = false;
      did_cancel_then_restart_main_ = true;
      return new CancelThenRestartTestJob(request);
    }
    if (simulate_main_network_error_) {
      simulate_main_network_error_ = false;
      did_simulate_error_main_ = true;
      // will error since the requeted url is not one of its canned urls
      return new URLRequestTestJob(request, true);
    }
    if (!intercept_main_request_)
      return NULL;
    intercept_main_request_ = false;
    did_intercept_main_ = true;
    return new URLRequestTestJob(request,
                                 main_headers_,
                                 main_data_,
                                 true);
  }

  virtual URLRequestJob* MaybeInterceptRedirect(URLRequest* request,
                                                const GURL& location) {
    if (cancel_redirect_request_) {
      cancel_redirect_request_ = false;
      did_cancel_redirect_ = true;
      return new CancelTestJob(request);
    }
    if (!intercept_redirect_)
      return NULL;
    intercept_redirect_ = false;
    did_intercept_redirect_ = true;
    return new URLRequestTestJob(request,
                                 redirect_headers_,
                                 redirect_data_,
                                 true);
  }

  virtual URLRequestJob* MaybeInterceptResponse(URLRequest* request) {
    if (cancel_final_request_) {
      cancel_final_request_ = false;
      did_cancel_final_ = true;
      return new CancelTestJob(request);
    }
    if (!intercept_final_response_)
      return NULL;
    intercept_final_response_ = false;
    did_intercept_final_ = true;
    return new URLRequestTestJob(request,
                                 final_headers_,
                                 final_data_,
                                 true);
  }

  // Whether to intercept the main request, and if so the response to return.
  bool intercept_main_request_;
  std::string main_headers_;
  std::string main_data_;

  // Other actions we take at MaybeIntercept time
  bool restart_main_request_;
  bool cancel_main_request_;
  bool cancel_then_restart_main_request_;
  bool simulate_main_network_error_;

  // Whether to intercept redirects, and if so the response to return.
  bool intercept_redirect_;
  std::string redirect_headers_;
  std::string redirect_data_;

  // Other actions we can take at MaybeInterceptRedirect time
  bool cancel_redirect_request_;

  // Whether to intercept final response, and if so the response to return.
  bool intercept_final_response_;
  std::string final_headers_;
  std::string final_data_;

  // Other actions we can take at MaybeInterceptResponse time
  bool cancel_final_request_;

  // If we did something or not
  bool did_intercept_main_;
  bool did_restart_main_;
  bool did_cancel_main_;
  bool did_cancel_then_restart_main_;
  bool did_simulate_error_main_;
  bool did_intercept_redirect_;
  bool did_cancel_redirect_;
  bool did_intercept_final_;
  bool did_cancel_final_;

  // Static getters for canned response header and data strings

  static std::string ok_data() {
    return URLRequestTestJob::test_data_1();
  }

  static std::string ok_headers() {
    return URLRequestTestJob::test_headers();
  }

  static std::string redirect_data() {
    return std::string();
  }

  static std::string redirect_headers() {
    return URLRequestTestJob::test_redirect_headers();
  }

  static std::string error_data() {
    return std::string("ohhh nooooo mr. bill!");
  }

  static std::string error_headers() {
    return URLRequestTestJob::test_error_headers();
  }
};

TEST_F(URLRequestTest, Intercept) {
  TestInterceptor interceptor;

  // intercept the main request and respond with a simple response
  interceptor.intercept_main_request_ = true;
  interceptor.main_headers_ = TestInterceptor::ok_headers();
  interceptor.main_data_ = TestInterceptor::ok_data();

  TestDelegate d;
  TestURLRequest req(GURL("http://test_intercept/foo"), &d);
  URLRequest::UserData* user_data0 = new URLRequest::UserData();
  URLRequest::UserData* user_data1 = new URLRequest::UserData();
  URLRequest::UserData* user_data2 = new URLRequest::UserData();
  req.SetUserData(NULL, user_data0);
  req.SetUserData(&user_data1, user_data1);
  req.SetUserData(&user_data2, user_data2);
  req.set_method("GET");
  req.Start();
  MessageLoop::current()->Run();

  // Make sure we can retrieve our specific user data
  EXPECT_EQ(user_data0, req.GetUserData(NULL));
  EXPECT_EQ(user_data1, req.GetUserData(&user_data1));
  EXPECT_EQ(user_data2, req.GetUserData(&user_data2));

  // Check the interceptor got called as expected
  EXPECT_TRUE(interceptor.did_intercept_main_);

  // Check we got one good response
  EXPECT_TRUE(req.status().is_success());
  EXPECT_EQ(200, req.response_headers()->response_code());
  EXPECT_EQ(TestInterceptor::ok_data(), d.data_received());
  EXPECT_EQ(1, d.response_started_count());
  EXPECT_EQ(0, d.received_redirect_count());
}

TEST_F(URLRequestTest, InterceptRedirect) {
  TestInterceptor interceptor;

  // intercept the main request and respond with a redirect
  interceptor.intercept_main_request_ = true;
  interceptor.main_headers_ = TestInterceptor::redirect_headers();
  interceptor.main_data_ = TestInterceptor::redirect_data();

  // intercept that redirect and respond a final OK response
  interceptor.intercept_redirect_ = true;
  interceptor.redirect_headers_ =  TestInterceptor::ok_headers();
  interceptor.redirect_data_ = TestInterceptor::ok_data();

  TestDelegate d;
  TestURLRequest req(GURL("http://test_intercept/foo"), &d);
  req.set_method("GET");
  req.Start();
  MessageLoop::current()->Run();

  // Check the interceptor got called as expected
  EXPECT_TRUE(interceptor.did_intercept_main_);
  EXPECT_TRUE(interceptor.did_intercept_redirect_);

  // Check we got one good response
  EXPECT_TRUE(req.status().is_success());
  if (req.status().is_success()) {
    EXPECT_EQ(200, req.response_headers()->response_code());
  }
  EXPECT_EQ(TestInterceptor::ok_data(), d.data_received());
  EXPECT_EQ(1, d.response_started_count());
  EXPECT_EQ(0, d.received_redirect_count());
}

TEST_F(URLRequestTest, InterceptServerError) {
  TestInterceptor interceptor;

  // intercept the main request to generate a server error response
  interceptor.intercept_main_request_ = true;
  interceptor.main_headers_ = TestInterceptor::error_headers();
  interceptor.main_data_ = TestInterceptor::error_data();

  // intercept that error and respond with an OK response
  interceptor.intercept_final_response_ = true;
  interceptor.final_headers_ = TestInterceptor::ok_headers();
  interceptor.final_data_ = TestInterceptor::ok_data();

  TestDelegate d;
  TestURLRequest req(GURL("http://test_intercept/foo"), &d);
  req.set_method("GET");
  req.Start();
  MessageLoop::current()->Run();

  // Check the interceptor got called as expected
  EXPECT_TRUE(interceptor.did_intercept_main_);
  EXPECT_TRUE(interceptor.did_intercept_final_);

  // Check we got one good response
  EXPECT_TRUE(req.status().is_success());
  EXPECT_EQ(200, req.response_headers()->response_code());
  EXPECT_EQ(TestInterceptor::ok_data(), d.data_received());
  EXPECT_EQ(1, d.response_started_count());
  EXPECT_EQ(0, d.received_redirect_count());
}

TEST_F(URLRequestTest, InterceptNetworkError) {
  TestInterceptor interceptor;

  // intercept the main request to simulate a network error
  interceptor.simulate_main_network_error_ = true;

  // intercept that error and respond with an OK response
  interceptor.intercept_final_response_ = true;
  interceptor.final_headers_ = TestInterceptor::ok_headers();
  interceptor.final_data_ = TestInterceptor::ok_data();

  TestDelegate d;
  TestURLRequest req(GURL("http://test_intercept/foo"), &d);
  req.set_method("GET");
  req.Start();
  MessageLoop::current()->Run();

  // Check the interceptor got called as expected
  EXPECT_TRUE(interceptor.did_simulate_error_main_);
  EXPECT_TRUE(interceptor.did_intercept_final_);

  // Check we received one good response
  EXPECT_TRUE(req.status().is_success());
  EXPECT_EQ(200, req.response_headers()->response_code());
  EXPECT_EQ(TestInterceptor::ok_data(), d.data_received());
  EXPECT_EQ(1, d.response_started_count());
  EXPECT_EQ(0, d.received_redirect_count());
}

TEST_F(URLRequestTest, InterceptRestartRequired) {
  TestInterceptor interceptor;

  // restart the main request
  interceptor.restart_main_request_ = true;

  // then intercept the new main request and respond with an OK response
  interceptor.intercept_main_request_ = true;
  interceptor.main_headers_ = TestInterceptor::ok_headers();
  interceptor.main_data_ = TestInterceptor::ok_data();

  TestDelegate d;
  TestURLRequest req(GURL("http://test_intercept/foo"), &d);
  req.set_method("GET");
  req.Start();
  MessageLoop::current()->Run();

  // Check the interceptor got called as expected
  EXPECT_TRUE(interceptor.did_restart_main_);
  EXPECT_TRUE(interceptor.did_intercept_main_);

  // Check we received one good response
  EXPECT_TRUE(req.status().is_success());
  if (req.status().is_success()) {
    EXPECT_EQ(200, req.response_headers()->response_code());
  }
  EXPECT_EQ(TestInterceptor::ok_data(), d.data_received());
  EXPECT_EQ(1, d.response_started_count());
  EXPECT_EQ(0, d.received_redirect_count());
}

TEST_F(URLRequestTest, InterceptRespectsCancelMain) {
  TestInterceptor interceptor;

  // intercept the main request and cancel from within the restarted job
  interceptor.cancel_main_request_ = true;

  // setup to intercept final response and override it with an OK response
  interceptor.intercept_final_response_ = true;
  interceptor.final_headers_ = TestInterceptor::ok_headers();
  interceptor.final_data_ = TestInterceptor::ok_data();

  TestDelegate d;
  TestURLRequest req(GURL("http://test_intercept/foo"), &d);
  req.set_method("GET");
  req.Start();
  MessageLoop::current()->Run();

  // Check the interceptor got called as expected
  EXPECT_TRUE(interceptor.did_cancel_main_);
  EXPECT_FALSE(interceptor.did_intercept_final_);

  // Check we see a canceled request
  EXPECT_FALSE(req.status().is_success());
  EXPECT_EQ(URLRequestStatus::CANCELED, req.status().status());
}

TEST_F(URLRequestTest, InterceptRespectsCancelRedirect) {
  TestInterceptor interceptor;

  // intercept the main request and respond with a redirect
  interceptor.intercept_main_request_ = true;
  interceptor.main_headers_ = TestInterceptor::redirect_headers();
  interceptor.main_data_ = TestInterceptor::redirect_data();

  // intercept the redirect and cancel from within that job
  interceptor.cancel_redirect_request_ = true;

  // setup to intercept final response and override it with an OK response
  interceptor.intercept_final_response_ = true;
  interceptor.final_headers_ = TestInterceptor::ok_headers();
  interceptor.final_data_ = TestInterceptor::ok_data();

  TestDelegate d;
  TestURLRequest req(GURL("http://test_intercept/foo"), &d);
  req.set_method("GET");
  req.Start();
  MessageLoop::current()->Run();

  // Check the interceptor got called as expected
  EXPECT_TRUE(interceptor.did_intercept_main_);
  EXPECT_TRUE(interceptor.did_cancel_redirect_);
  EXPECT_FALSE(interceptor.did_intercept_final_);

  // Check we see a canceled request
  EXPECT_FALSE(req.status().is_success());
  EXPECT_EQ(URLRequestStatus::CANCELED, req.status().status());
}

TEST_F(URLRequestTest, InterceptRespectsCancelFinal) {
  TestInterceptor interceptor;

  // intercept the main request to simulate a network error
  interceptor.simulate_main_network_error_ = true;

  // setup to intercept final response and cancel from within that job
  interceptor.cancel_final_request_ = true;

  TestDelegate d;
  TestURLRequest req(GURL("http://test_intercept/foo"), &d);
  req.set_method("GET");
  req.Start();
  MessageLoop::current()->Run();

  // Check the interceptor got called as expected
  EXPECT_TRUE(interceptor.did_simulate_error_main_);
  EXPECT_TRUE(interceptor.did_cancel_final_);

  // Check we see a canceled request
  EXPECT_FALSE(req.status().is_success());
  EXPECT_EQ(URLRequestStatus::CANCELED, req.status().status());
}

TEST_F(URLRequestTest, InterceptRespectsCancelInRestart) {
  TestInterceptor interceptor;

  // intercept the main request and cancel then restart from within that job
  interceptor.cancel_then_restart_main_request_ = true;

  // setup to intercept final response and override it with an OK response
  interceptor.intercept_final_response_ = true;
  interceptor.final_headers_ = TestInterceptor::ok_headers();
  interceptor.final_data_ = TestInterceptor::ok_data();

  TestDelegate d;
  TestURLRequest req(GURL("http://test_intercept/foo"), &d);
  req.set_method("GET");
  req.Start();
  MessageLoop::current()->Run();

  // Check the interceptor got called as expected
  EXPECT_TRUE(interceptor.did_cancel_then_restart_main_);
  EXPECT_FALSE(interceptor.did_intercept_final_);

  // Check we see a canceled request
  EXPECT_FALSE(req.status().is_success());
  EXPECT_EQ(URLRequestStatus::CANCELED, req.status().status());
}

class URLRequestTestFTP : public URLRequestTest {
 protected:
  static void SetUpTestCase() {
    server_ = FTPTestServer::CreateServer(L"");
  }

  static void TearDownTestCase() {
    server_ = NULL;
  }

  static scoped_refptr<FTPTestServer> server_;
};

// static
scoped_refptr<FTPTestServer> URLRequestTestFTP::server_;

TEST_F(URLRequestTestFTP, FTPGetTestAnonymous) {
  ASSERT_TRUE(NULL != server_.get());
  FilePath app_path;
  PathService::Get(base::DIR_SOURCE_ROOT, &app_path);
  app_path = app_path.AppendASCII("LICENSE");
  TestDelegate d;
  {
    TestURLRequest r(server_->TestServerPage("/LICENSE"), &d);
    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();

    int64 file_size = 0;
    file_util::GetFileSize(app_path, &file_size);

    EXPECT_FALSE(r.is_pending());
    EXPECT_EQ(1, d.response_started_count());
    EXPECT_FALSE(d.received_data_before_response());
    EXPECT_EQ(d.bytes_received(), static_cast<int>(file_size));
  }
}

TEST_F(URLRequestTestFTP, FTPGetTest) {
  ASSERT_TRUE(NULL != server_.get());
  FilePath app_path;
  PathService::Get(base::DIR_SOURCE_ROOT, &app_path);
  app_path = app_path.AppendASCII("LICENSE");
  TestDelegate d;
  {
    TestURLRequest r(server_->TestServerPage("/LICENSE", "chrome", "chrome"),
                     &d);
    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();

    int64 file_size = 0;
    file_util::GetFileSize(app_path, &file_size);

    EXPECT_FALSE(r.is_pending());
    EXPECT_EQ(1, d.response_started_count());
    EXPECT_FALSE(d.received_data_before_response());
    EXPECT_EQ(d.bytes_received(), static_cast<int>(file_size));
  }
}

TEST_F(URLRequestTestFTP, FTPCheckWrongPassword) {
  ASSERT_TRUE(NULL != server_.get());
  FilePath app_path;
  PathService::Get(base::DIR_SOURCE_ROOT, &app_path);
  app_path = app_path.AppendASCII("LICENSE");
  TestDelegate d;
  {
    TestURLRequest r(server_->TestServerPage("/LICENSE",
                                             "chrome", "wrong_password"), &d);
    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();

    int64 file_size = 0;
    file_util::GetFileSize(app_path, &file_size);

    EXPECT_FALSE(r.is_pending());
    EXPECT_EQ(1, d.response_started_count());
    EXPECT_FALSE(d.received_data_before_response());
    EXPECT_EQ(d.bytes_received(), 0);
  }
}

TEST_F(URLRequestTestFTP, FTPCheckWrongPasswordRestart) {
  ASSERT_TRUE(NULL != server_.get());
  FilePath app_path;
  PathService::Get(base::DIR_SOURCE_ROOT, &app_path);
  app_path = app_path.AppendASCII("LICENSE");
  TestDelegate d;
  // Set correct login credentials. The delegate will be asked for them when
  // the initial login with wrong credentials will fail.
  d.set_username(L"chrome");
  d.set_password(L"chrome");
  {
    TestURLRequest r(server_->TestServerPage("/LICENSE",
                                             "chrome", "wrong_password"), &d);
    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();

    int64 file_size = 0;
    file_util::GetFileSize(app_path, &file_size);

    EXPECT_FALSE(r.is_pending());
    EXPECT_EQ(1, d.response_started_count());
    EXPECT_FALSE(d.received_data_before_response());
    EXPECT_EQ(d.bytes_received(), static_cast<int>(file_size));
  }
}

TEST_F(URLRequestTestFTP, FTPCheckWrongUser) {
  ASSERT_TRUE(NULL != server_.get());
  FilePath app_path;
  PathService::Get(base::DIR_SOURCE_ROOT, &app_path);
  app_path = app_path.AppendASCII("LICENSE");
  TestDelegate d;
  {
    TestURLRequest r(server_->TestServerPage("/LICENSE",
                                             "wrong_user", "chrome"), &d);
    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();

    int64 file_size = 0;
    file_util::GetFileSize(app_path, &file_size);

    EXPECT_FALSE(r.is_pending());
    EXPECT_EQ(1, d.response_started_count());
    EXPECT_FALSE(d.received_data_before_response());
    EXPECT_EQ(d.bytes_received(), 0);
  }
}

TEST_F(URLRequestTestFTP, FTPCheckWrongUserRestart) {
  ASSERT_TRUE(NULL != server_.get());
  FilePath app_path;
  PathService::Get(base::DIR_SOURCE_ROOT, &app_path);
  app_path = app_path.AppendASCII("LICENSE");
  TestDelegate d;
  // Set correct login credentials. The delegate will be asked for them when
  // the initial login with wrong credentials will fail.
  d.set_username(L"chrome");
  d.set_password(L"chrome");
  {
    TestURLRequest r(server_->TestServerPage("/LICENSE",
                                             "wrong_user", "chrome"), &d);
    r.Start();
    EXPECT_TRUE(r.is_pending());

    MessageLoop::current()->Run();

    int64 file_size = 0;
    file_util::GetFileSize(app_path, &file_size);

    EXPECT_FALSE(r.is_pending());
    EXPECT_EQ(1, d.response_started_count());
    EXPECT_FALSE(d.received_data_before_response());
    EXPECT_EQ(d.bytes_received(), static_cast<int>(file_size));
  }
}
