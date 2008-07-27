// Copyright 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <vector>

#include "config.h"

#pragma warning(push, 0)
#include "KURL.h"
#include "ResourceResponse.h"
#include "ResourceHandle.h"
#include "ResourceHandleClient.h"
#include "String.h"
#pragma warning(pop)

#include "base/basictypes.h"
#include "webkit/glue/glue_util.h"
#include "webkit/glue/multipart_response_delegate.h"
#include "testing/gtest/include/gtest/gtest.h"

using namespace WebCore;
using namespace std;

namespace {

class MultipartResponseTest : public testing::Test {
};

class MockResourceHandleClient : public ResourceHandleClient {
 public:
  MockResourceHandleClient() { Reset(); }

  virtual void didReceiveResponse(ResourceHandle* handle,
                                  const ResourceResponse& response) {
    ++received_response_;
    resource_response_ = response;
    data_.clear();
  }
  virtual void didReceiveData(ResourceHandle* handle,
                              const char* data, int data_length,
                              int length_received) {
    ++received_data_;
    data_.append(data, data_length);
  }
  
  void Reset() {
    received_response_ = received_data_ = 0;
    data_.clear();
    resource_response_ = ResourceResponse();
  }

  int received_response_, received_data_;
  string data_;
  ResourceResponse resource_response_;
};

}  // namespace

// We can't put this in an anonymous function because it's a friend class for
// access to private members.
TEST(MultipartResponseTest, Functions) {
  // PushOverLine tests

  ResourceResponse response(KURL(), "multipart/x-mixed-replace", 0, "en-US",
                            String());
  response.setHTTPHeaderField(String("Foo"), String("Bar"));
  response.setHTTPHeaderField(String("Content-type"), String("text/plain"));
  MockResourceHandleClient client;
  MultipartResponseDelegate delegate(&client, NULL, response, "bound");

  struct {
    const char* input;
    const int position;
    const int expected;
  } line_tests[] = {
    { "Line", 0, 0 },
    { "Line", 2, 0 },
    { "Line", 10, 0 },
    { "\r\nLine", 0, 2 },
    { "\nLine", 0, 1 },
    { "\n\nLine", 0, 2 },
    { "\rLine", 0, 1 },
    { "Line\r\nLine", 4, 2 },
    { "Line\nLine", 4, 1 },
    { "Line\n\nLine", 4, 2 },
    { "Line\rLine", 4, 1 },
    { "Line\r\rLine", 4, 1 },
  };
  for (int i = 0; i < arraysize(line_tests); ++i) {
    EXPECT_EQ(line_tests[i].expected,
              delegate.PushOverLine(line_tests[i].input,
                                    line_tests[i].position));
  }

  // ParseHeaders tests
  struct {
    const char* data;
    const bool rv;
    const int received_response_calls;
    const char* newdata;
  } header_tests[] = {
    { "This is junk", false, 0, "This is junk" },
    { "Foo: bar\nBaz:\n\nAfter:\n", true, 1, "After:\n" },
    { "Foo: bar\nBaz:\n", false, 0, "Foo: bar\nBaz:\n" },
    { "Foo: bar\r\nBaz:\r\n\r\nAfter:\r\n", true, 1, "After:\r\n" },
    { "Foo: bar\r\nBaz:\r\n", false, 0, "Foo: bar\r\nBaz:\r\n" },
    { "Foo: bar\nBaz:\r\n\r\nAfter:\n\n", true, 1, "After:\n\n" },
    { "Foo: bar\r\nBaz:\n", false, 0, "Foo: bar\r\nBaz:\n" },
    { "\r\n", true, 1, "" },
  };
  for (int i = 0; i < arraysize(header_tests); ++i) {
    client.Reset();
    delegate.data_.assign(header_tests[i].data);
    EXPECT_EQ(header_tests[i].rv,
              delegate.ParseHeaders());
    EXPECT_EQ(header_tests[i].received_response_calls,
              client.received_response_);
    EXPECT_EQ(string(header_tests[i].newdata),
              delegate.data_);
  }
  // Test that the resource response is filled in correctly when parsing
  // headers.
  client.Reset();
  string test_header("content-type: image/png\ncontent-length: 10\n\n");
  delegate.data_.assign(test_header);
  EXPECT_TRUE(delegate.ParseHeaders());
  EXPECT_TRUE(delegate.data_.length() == 0);
  EXPECT_EQ(webkit_glue::StringToStdWString(
              client.resource_response_.httpHeaderField(
                String("Content-Type"))),
            wstring(L"image/png"));
  EXPECT_EQ(webkit_glue::StringToStdWString(
              client.resource_response_.httpHeaderField(
                String("content-length"))),
            wstring(L"10"));
  // This header is passed from the original request.
  EXPECT_EQ(webkit_glue::StringToStdWString(
              client.resource_response_.httpHeaderField(String("foo"))),
            wstring(L"Bar"));
  
  // FindBoundary tests
  struct {
    const char* boundary;
    const char* data;
    const size_t position;
  } boundary_tests[] = {
    { "bound", "bound", 0 },
    { "bound", "--bound", 0 },
    { "bound", "junkbound", 4 },
    { "bound", "junk--bound", 4 },
    { "foo", "bound", string::npos },
    { "bound", "--boundbound", 0 },
  };
  for (int i = 0; i < arraysize(boundary_tests); ++i) {
    delegate.boundary_.assign(boundary_tests[i].boundary);
    delegate.data_.assign(boundary_tests[i].data);
    EXPECT_EQ(boundary_tests[i].position,
              delegate.FindBoundary());
  }
}

namespace {

TEST(MultipartResponseTest, MissingBoundaries) {
  ResourceResponse response(KURL(), "multipart/x-mixed-replace", 0, "en-US",
                            String());
  response.setHTTPHeaderField(String("Foo"), String("Bar"));
  response.setHTTPHeaderField(String("Content-type"), String("text/plain"));
  MockResourceHandleClient client;
  MultipartResponseDelegate delegate(&client, NULL, response, "bound");

  // No start boundary
  string no_start_boundary(
    "Content-type: text/plain\n\n"
    "This is a sample response\n"
    "--bound--"
    "ignore junk after end token --bound\n\nTest2\n");
  delegate.OnReceivedData(no_start_boundary.c_str(),
                          static_cast<int>(no_start_boundary.length()));
  EXPECT_EQ(1, client.received_response_);
  EXPECT_EQ(1, client.received_data_);
  EXPECT_EQ(string("This is a sample response\n"),
            client.data_);

  delegate.OnCompletedRequest();
  EXPECT_EQ(1, client.received_response_);
  EXPECT_EQ(1, client.received_data_);

  // No end boundary
  client.Reset();
  MultipartResponseDelegate delegate2(&client, NULL, response, "bound");
  string no_end_boundary(
    "bound\nContent-type: text/plain\n\n"
    "This is a sample response\n");
  delegate2.OnReceivedData(no_end_boundary.c_str(),
                          static_cast<int>(no_end_boundary.length()));
  EXPECT_EQ(1, client.received_response_);
  EXPECT_EQ(0, client.received_data_);
  EXPECT_EQ(string(), client.data_);

  delegate2.OnCompletedRequest();
  EXPECT_EQ(1, client.received_response_);
  EXPECT_EQ(1, client.received_data_);
  EXPECT_EQ(string("This is a sample response\n"),
            client.data_);

  // Neither boundary
  client.Reset();
  MultipartResponseDelegate delegate3(&client, NULL, response, "bound");
  string no_boundaries(
    "Content-type: text/plain\n\n"
    "This is a sample response\n");
  delegate3.OnReceivedData(no_boundaries.c_str(),
                          static_cast<int>(no_boundaries.length()));
  EXPECT_EQ(1, client.received_response_);
  EXPECT_EQ(0, client.received_data_);
  EXPECT_EQ(string(), client.data_);

  delegate3.OnCompletedRequest();
  EXPECT_EQ(1, client.received_response_);
  EXPECT_EQ(1, client.received_data_);
  EXPECT_EQ(string("This is a sample response\n"),
            client.data_);
}


// Used in for tests that break the data in various places.
struct TestChunk {
  const int start_pos;  // offset in data
  const int end_pos;    // end offset in data
  const int expected_responses;
  const int expected_received_data;
  const char* expected_data;
};

void VariousChunkSizesTest(const TestChunk chunks[], int chunks_size, int responses,
                           int received_data, const char* completed_data) {
  const string data(
    "--bound\n"                    // 0-7
    "Content-type: image/png\n\n"  // 8-32
    "datadatadatadatadata"         // 33-52
    "--bound\n"                    // 53-60
    "Content-type: image/jpg\n\n"  // 61-85
    "foofoofoofoofoo"              // 86-100
    "--bound--");                  // 101-109

  ResourceResponse response(KURL(), "multipart/x-mixed-replace", 0, "en-US",
                            String());
  MockResourceHandleClient client;
  MultipartResponseDelegate delegate(&client, NULL, response, "bound");
  
  for (int i = 0; i < chunks_size; ++i) {
    ASSERT(chunks[i].start_pos < chunks[i].end_pos);
    string chunk = data.substr(chunks[i].start_pos,
                               chunks[i].end_pos - chunks[i].start_pos);
    delegate.OnReceivedData(chunk.c_str(), static_cast<int>(chunk.length()));
    EXPECT_EQ(chunks[i].expected_responses,
              client.received_response_);
    EXPECT_EQ(chunks[i].expected_received_data,
              client.received_data_);
    EXPECT_EQ(string(chunks[i].expected_data),
              client.data_);
  }
  // Check final state
  delegate.OnCompletedRequest();
  EXPECT_EQ(responses,
            client.received_response_);
  EXPECT_EQ(received_data,
            client.received_data_);
  EXPECT_EQ(string(completed_data),
            client.data_);
}

TEST(MultipartResponseTest, BreakInBoundary) {
  // Break in the first boundary
  const TestChunk bound1[] = {
    { 0, 4, 0, 0, ""},
    { 4, 110, 2, 2, "foofoofoofoofoo" },
  };
  VariousChunkSizesTest(bound1, arraysize(bound1),
                        2, 2, "foofoofoofoofoo");

  // Break in first and second
  const TestChunk bound2[] = {
    { 0, 4, 0, 0, ""},
    { 4, 55, 1, 0, "" },
    { 55, 65, 1, 1, "datadatadatadatadata" },
    { 65, 110, 2, 2, "foofoofoofoofoo" },
  };
  VariousChunkSizesTest(bound2, arraysize(bound2),
                        2, 2, "foofoofoofoofoo");

  // Break in second only
  const TestChunk bound3[] = {
    { 0, 55, 1, 0, "" },
    { 55, 110, 2, 2, "foofoofoofoofoo" },
  };
  VariousChunkSizesTest(bound3, arraysize(bound3),
                        2, 2, "foofoofoofoofoo");
}

TEST(MultipartResponseTest, BreakInHeaders) {
  // Break in first header
  const TestChunk header1[] = {
    { 0, 10, 0, 0, "" },
    { 10, 35, 1, 0, "" },
    { 35, 110, 2, 2, "foofoofoofoofoo" },
  };
  VariousChunkSizesTest(header1, arraysize(header1),
                        2, 2, "foofoofoofoofoo");

  // Break in both headers
  const TestChunk header2[] = {
    { 0, 10, 0, 0, "" },
    { 10, 65, 1, 1, "datadatadatadatadata" },
    { 65, 110, 2, 2, "foofoofoofoofoo" },
  };
  VariousChunkSizesTest(header2, arraysize(header2),
                        2, 2, "foofoofoofoofoo");

  // Break at end of a header
  const TestChunk header3[] = {
    { 0, 33, 1, 0, "" },
    { 33, 65, 1, 1, "datadatadatadatadata" },
    { 65, 110, 2, 2, "foofoofoofoofoo" },
  };
  VariousChunkSizesTest(header3, arraysize(header3),
                        2, 2, "foofoofoofoofoo");
}

TEST(MultipartResponseTest, BreakInData) {
  // All data as one chunk
  const TestChunk data1[] = {
    { 0, 110, 2, 2, "foofoofoofoofoo" },
  };
  VariousChunkSizesTest(data1, arraysize(data1),
                        2, 2, "foofoofoofoofoo");

  // breaks in data segment
  const TestChunk data2[] = {
    { 0, 35, 1, 0, "" },
    { 35, 65, 1, 1, "datadatadatadatadata" },
    { 65, 90, 2, 1, "" },
    { 90, 110, 2, 2, "foofoofoofoofoo" },
  };
  VariousChunkSizesTest(data2, arraysize(data2),
                        2, 2, "foofoofoofoofoo");
  
  // Incomplete send
  const TestChunk data3[] = {
    { 0, 35, 1, 0, "" },
    { 35, 90, 2, 1, "" },
  };
  VariousChunkSizesTest(data3, arraysize(data3),
                        2, 2, "foof");
}

TEST(MultipartResponseTest, MultipleBoundaries) {
  // Test multiple boundaries back to back
  ResourceResponse response(KURL(), "multipart/x-mixed-replace", 0, "en-US",
                            String());
  MockResourceHandleClient client;
  MultipartResponseDelegate delegate(&client, NULL, response, "bound");
  
  string data("--bound\r\n\r\n--bound\r\n\r\nfoofoo--bound--");
  delegate.OnReceivedData(data.c_str(), static_cast<int>(data.length()));
  EXPECT_EQ(2,
            client.received_response_);
  EXPECT_EQ(1,
            client.received_data_);
  EXPECT_EQ(string("foofoo"),
            client.data_);
}

}  // namespace
