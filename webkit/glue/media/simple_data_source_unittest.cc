// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/callback.h"
#include "media/base/filters.h"
#include "media/base/mock_filter_host.h"
#include "media/base/mock_filters.h"
#include "net/base/net_errors.h"
#include "third_party/WebKit/WebKit/chromium/public/WebFrame.h"
#include "third_party/WebKit/WebKit/chromium/public/WebURLError.h"
#include "third_party/WebKit/WebKit/chromium/public/WebURLLoader.h"
#include "third_party/WebKit/WebKit/chromium/public/WebURLRequest.h"
#include "third_party/WebKit/WebKit/chromium/public/WebURLResponse.h"
#include "webkit/glue/media/simple_data_source.h"
#include "webkit/glue/mock_webframe.h"
#include "webkit/glue/mock_weburlloader_impl.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::SetArgumentPointee;
using ::testing::StrictMock;
using ::testing::WithArgs;

using WebKit::WebURLError;
using WebKit::WebURLLoader;
using WebKit::WebURLRequest;
using WebKit::WebURLResponse;

namespace {

const int kDataSize = 1024;
const char kHttpUrl[] = "http://test";
const char kHttpsUrl[] = "https://test";
const char kFileUrl[] = "file://test";
const char kDataUrl[] =
    "data:text/plain;base64,YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXoK";
const char kDataUrlDecoded[] = "abcdefghijklmnopqrstuvwxyz";
const char kInvalidUrl[] = "whatever://test";

}  // namespace

namespace webkit_glue {

class SimpleDataSourceTest : public testing::Test {
 public:
  SimpleDataSourceTest() {
    for (int i = 0; i < kDataSize; ++i) {
      data_[i] = i;
    }
  }

  virtual ~SimpleDataSourceTest() {
    ignore_result(frame_.release());
  }

  void InitializeDataSource(const char* url) {
    gurl_ = GURL(url);

    frame_.reset(new NiceMock<MockWebFrame>());
    url_loader_ = new NiceMock<MockWebURLLoader>();

    data_source_ = new SimpleDataSource(MessageLoop::current(),
                                        frame_.get());

    // There is no need to provide a message loop to data source.
    data_source_->set_host(&host_);
    data_source_->SetURLLoaderForTest(url_loader_);

    InSequence s;

    data_source_->Initialize(url, callback_.NewCallback());
    MessageLoop::current()->RunAllPending();
  }

  void RequestSucceeded(bool is_loaded) {
    WebURLResponse response(gurl_);
    response.setExpectedContentLength(kDataSize);

    data_source_->didReceiveResponse(NULL, response);
    int64 size;
    EXPECT_TRUE(data_source_->GetSize(&size));
    EXPECT_EQ(kDataSize, size);

    for (int i = 0; i < kDataSize; ++i) {
      data_source_->didReceiveData(NULL, data_ + i, 1);
    }

    EXPECT_CALL(host_, SetLoaded(is_loaded));

    InSequence s;
    EXPECT_CALL(host_, SetTotalBytes(kDataSize));
    EXPECT_CALL(host_, SetBufferedBytes(kDataSize));
    EXPECT_CALL(callback_, OnFilterCallback());
    EXPECT_CALL(callback_, OnCallbackDestroyed());

    data_source_->didFinishLoading(NULL, 0);

    // Let the tasks to be executed.
    MessageLoop::current()->RunAllPending();
  }

  void RequestFailed() {
    InSequence s;
    EXPECT_CALL(host_, SetError(media::PIPELINE_ERROR_NETWORK));
    EXPECT_CALL(callback_, OnFilterCallback());
    EXPECT_CALL(callback_, OnCallbackDestroyed());

    WebURLError error;
    error.reason = net::ERR_FAILED;
    data_source_->didFail(NULL, error);

    // Let the tasks to be executed.
    MessageLoop::current()->RunAllPending();
  }

  void DestroyDataSource() {
    StrictMock<media::MockFilterCallback> callback;
    EXPECT_CALL(callback, OnFilterCallback());
    EXPECT_CALL(callback, OnCallbackDestroyed());

    data_source_->Stop(callback.NewCallback());
    MessageLoop::current()->RunAllPending();

    data_source_ = NULL;
  }

  void AsyncRead() {
    for (int i = 0; i < kDataSize; ++i) {
      uint8 buffer[1];

      EXPECT_CALL(*this, ReadCallback(1));
      data_source_->Read(
          i, 1, buffer,
          NewCallback(this, &SimpleDataSourceTest::ReadCallback));
      EXPECT_EQ(static_cast<uint8>(data_[i]), buffer[0]);
    }
  }

  MOCK_METHOD1(ReadCallback, void(size_t size));

 protected:
  GURL gurl_;
  scoped_ptr<MessageLoop> message_loop_;
  NiceMock<MockWebURLLoader>* url_loader_;
  scoped_refptr<SimpleDataSource> data_source_;
  StrictMock<media::MockFilterHost> host_;
  StrictMock<media::MockFilterCallback> callback_;
  scoped_ptr<NiceMock<MockWebFrame> > frame_;

  char data_[kDataSize];

  DISALLOW_COPY_AND_ASSIGN(SimpleDataSourceTest);
};

TEST_F(SimpleDataSourceTest, InitializeHTTP) {
  InitializeDataSource(kHttpUrl);
  RequestSucceeded(false);
  DestroyDataSource();
}

TEST_F(SimpleDataSourceTest, InitializeHTTPS) {
  InitializeDataSource(kHttpsUrl);
  RequestSucceeded(false);
  DestroyDataSource();
}

TEST_F(SimpleDataSourceTest, InitializeFile) {
  InitializeDataSource(kFileUrl);
  RequestSucceeded(true);
  DestroyDataSource();
}

TEST_F(SimpleDataSourceTest, InitializeData) {
  frame_.reset(new NiceMock<MockWebFrame>());
  url_loader_ = new NiceMock<MockWebURLLoader>();

  data_source_ = new SimpleDataSource(MessageLoop::current(),
                                      frame_.get());
  EXPECT_TRUE(data_source_->IsUrlSupported(kDataUrl));

  // There is no need to provide a message loop to data source.
  data_source_->set_host(&host_);
  data_source_->SetURLLoaderForTest(url_loader_);

  EXPECT_CALL(host_, SetLoaded(true));
  EXPECT_CALL(host_, SetTotalBytes(sizeof(kDataUrlDecoded)));
  EXPECT_CALL(host_, SetBufferedBytes(sizeof(kDataUrlDecoded)));
  EXPECT_CALL(callback_, OnFilterCallback());
  EXPECT_CALL(callback_, OnCallbackDestroyed());

  data_source_->Initialize(kDataUrl, callback_.NewCallback());
  MessageLoop::current()->RunAllPending();

  DestroyDataSource();
}

TEST_F(SimpleDataSourceTest, RequestFailed) {
  InitializeDataSource(kHttpUrl);
  RequestFailed();
  DestroyDataSource();
}

TEST_F(SimpleDataSourceTest, StopWhenDownloading) {
  InitializeDataSource(kHttpUrl);

  EXPECT_CALL(*url_loader_, cancel());
  EXPECT_CALL(callback_, OnCallbackDestroyed());
  DestroyDataSource();
}

TEST_F(SimpleDataSourceTest, AsyncRead) {
  InitializeDataSource(kFileUrl);
  RequestSucceeded(true);
  AsyncRead();
  DestroyDataSource();
}

}  // namespace webkit_glue
