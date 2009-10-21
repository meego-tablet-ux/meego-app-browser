// Copyright (c) 2009 The Chromium Authos. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/compiler_specific.h"
#include "base/pickle.h"
#include "base/thread.h"
#include "base/waitable_event.h"
#include "net/base/io_buffer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "webkit/appcache/appcache_response.h"
#include "webkit/appcache/mock_appcache_service.h"

using net::IOBuffer;

namespace appcache {

class AppCacheResponseTest : public testing::Test {
 public:

  // Test Harness -------------------------------------------------------------

  // Helper class used to verify test results
  class MockStorageDelegate : public AppCacheStorage::Delegate {
   public:
    explicit MockStorageDelegate(AppCacheResponseTest* test)
        : loaded_info_id_(0), test_(test) {
    }

    virtual void OnResponseInfoLoaded(AppCacheResponseInfo* info,
                                      int64 response_id) {
      loaded_info_ = info;
      loaded_info_id_ = response_id;
      test_->ScheduleNextTask();
    }

    scoped_refptr<AppCacheResponseInfo> loaded_info_;
    int64 loaded_info_id_;
    AppCacheResponseTest* test_;
  };

  // Helper class run a test on our io_thread. The io_thread
  // is spun up once and reused for all tests.
  template <class Method>
  class WrapperTask : public Task {
   public:
    WrapperTask(AppCacheResponseTest* test, Method method)
        : test_(test), method_(method) {
    }

    virtual void Run() {
      test_->SetUpTest();
      (test_->*method_)();
    }

   private:
    AppCacheResponseTest* test_;
    Method method_;
  };

  static void SetUpTestCase() {
    io_thread_.reset(new base::Thread("AppCacheResponseTest Thread"));
    base::Thread::Options options(MessageLoop::TYPE_IO, 0);
    io_thread_->StartWithOptions(options);
  }

  static void TearDownTestCase() {
    io_thread_.reset(NULL);
  }

  AppCacheResponseTest()
      : ALLOW_THIS_IN_INITIALIZER_LIST(method_factory_(this)),
        ALLOW_THIS_IN_INITIALIZER_LIST(read_callback_(
            this, &AppCacheResponseTest::OnReadComplete)),
        ALLOW_THIS_IN_INITIALIZER_LIST(read_info_callback_(
            this, &AppCacheResponseTest::OnReadInfoComplete)),
        ALLOW_THIS_IN_INITIALIZER_LIST(write_callback_(
            this, &AppCacheResponseTest::OnWriteComplete)),
        ALLOW_THIS_IN_INITIALIZER_LIST(write_info_callback_(
            this, &AppCacheResponseTest::OnWriteInfoComplete)) {
  }

  template <class Method>
  void RunTestOnIOThread(Method method) {
    test_finished_event_ .reset(new base::WaitableEvent(false, false));
    io_thread_->message_loop()->PostTask(
        FROM_HERE, new WrapperTask<Method>(this, method));
    test_finished_event_->Wait();
  }

  void SetUpTest() {
    DCHECK(MessageLoop::current() == io_thread_->message_loop());
    DCHECK(task_stack_.empty());
    storage_delegate_.reset(new MockStorageDelegate(this));
    service_.reset(new MockAppCacheService());
    expected_read_result_ = 0;
    expected_write_result_ = 0;
    written_response_id_ = 0;
    should_delete_reader_in_completion_callback_ = false;
    should_delete_writer_in_completion_callback_ = false;
    reader_deletion_count_down_ = 0;
    writer_deletion_count_down_ = 0;
    read_callback_was_called_ = false;
    write_callback_was_called_ = false;
  }

  void TearDownTest() {
    DCHECK(MessageLoop::current() == io_thread_->message_loop());
    while (!task_stack_.empty()) {
      delete task_stack_.top().first;
      task_stack_.pop();
    }
    reader_.reset();
    read_buffer_ = NULL;
    read_info_buffer_ = NULL;
    writer_.reset();
    write_buffer_ = NULL;
    write_info_buffer_ = NULL;
    storage_delegate_.reset();
    service_.reset();
  }

  void TestFinished() {
    // We unwind the stack prior to finishing up to let stack
    // based objects get deleted.
    DCHECK(MessageLoop::current() == io_thread_->message_loop());
    MessageLoop::current()->PostTask(FROM_HERE,
        method_factory_.NewRunnableMethod(
            &AppCacheResponseTest::TestFinishedUnwound));
  }

  void TestFinishedUnwound() {
    TearDownTest();
    test_finished_event_->Signal();
  }

  void PushNextTask(Task* task) {
    task_stack_.push(std::pair<Task*, bool>(task, false));
  }

  void PushNextTaskAsImmediate(Task* task) {
    task_stack_.push(std::pair<Task*, bool>(task, true));
  }

  void ScheduleNextTask() {
    DCHECK(MessageLoop::current() == io_thread_->message_loop());
    if (task_stack_.empty()) {
      TestFinished();
      return;
    }
    scoped_ptr<Task> task(task_stack_.top().first);
    bool immediate = task_stack_.top().second;
    task_stack_.pop();
    if (immediate)
      task->Run();
    else
      MessageLoop::current()->PostTask(FROM_HERE, task.release());
  }

  // Wrappers to call AppCacheResponseReader/Writer Read and Write methods

  void WriteBasicResponse() {
    static const char* kRawHttpHeaders =
        "HTTP/1.0 200 OK\r\nContent-Length: 5\r\n\r\n";
    static const char* kRawHttpBody = "Hello";
    WriteResponse(MakeHttpResponseInfo(kRawHttpHeaders), kRawHttpBody);
  }

  void WriteResponse(net::HttpResponseInfo* head, const char* body) {
    PushNextTask(method_factory_.NewRunnableMethod(
        &AppCacheResponseTest::WriteResponseBody,
        new net::WrappedIOBuffer(body), strlen(body)));
    WriteResponseHead(head);
  }

  void WriteResponseHead(net::HttpResponseInfo* head) {
    EXPECT_FALSE(writer_->IsWritePending());
    expected_write_result_ = GetHttpResponseInfoSize(head);
    write_info_buffer_ = new HttpResponseInfoIOBuffer(head);
    writer_->WriteInfo(write_info_buffer_, &write_info_callback_);
  }

  void WriteResponseBody(scoped_refptr<IOBuffer> io_buffer, int buf_len) {
    EXPECT_FALSE(writer_->IsWritePending());
    write_buffer_ = io_buffer;
    expected_write_result_ = buf_len;
    writer_->WriteData(
        write_buffer_, buf_len, &write_callback_);
  }

  void ReadResponseBody(scoped_refptr<IOBuffer> io_buffer, int buf_len) {
    EXPECT_FALSE(reader_->IsReadPending());
    read_buffer_ = io_buffer;
    expected_read_result_ = buf_len;
    reader_->ReadData(
        read_buffer_, buf_len, &read_callback_);
  }

  // AppCacheResponseReader / Writer completion callbacks

  void OnWriteInfoComplete(int result) {
    EXPECT_FALSE(writer_->IsWritePending());
    EXPECT_EQ(expected_write_result_, result);
    ScheduleNextTask();
  }

  void OnWriteComplete(int result) {
    EXPECT_FALSE(writer_->IsWritePending());
    write_callback_was_called_ = true;
    EXPECT_EQ(expected_write_result_, result);
    if (should_delete_writer_in_completion_callback_ &&
        --writer_deletion_count_down_ == 0) {
      writer_.reset();
    }
    ScheduleNextTask();
  }

  void OnReadInfoComplete(int result) {
    EXPECT_FALSE(reader_->IsReadPending());
    EXPECT_EQ(expected_read_result_, result);
    ScheduleNextTask();
  }

  void OnReadComplete(int result) {
    EXPECT_FALSE(reader_->IsReadPending());
    read_callback_was_called_ = true;
    EXPECT_EQ(expected_read_result_, result);
    if (should_delete_reader_in_completion_callback_ &&
        --reader_deletion_count_down_ == 0) {
      reader_.reset();
    }
    ScheduleNextTask();
  }

  // Helpers to work with HttpResponseInfo objects

  net::HttpResponseInfo* MakeHttpResponseInfo(const char* raw_headers) {
    net::HttpResponseInfo* info = new net::HttpResponseInfo;
    info->request_time = base::Time::Now();
    info->response_time = base::Time::Now();
    info->was_cached = false;
    info->headers = new net::HttpResponseHeaders(raw_headers);
    return info;
  }

  int GetHttpResponseInfoSize(const net::HttpResponseInfo* info) {
    Pickle pickle;
    return PickleHttpResonseInfo(&pickle, info);
  }

  bool CompareHttpResponseInfos(const net::HttpResponseInfo* info1,
                                const net::HttpResponseInfo* info2) {
    Pickle pickle1;
    Pickle pickle2;
    PickleHttpResonseInfo(&pickle1, info1);
    PickleHttpResonseInfo(&pickle2, info2);
    return (pickle1.size() == pickle2.size()) &&
           (0 == memcmp(pickle1.data(), pickle2.data(), pickle1.size()));
  }

  int PickleHttpResonseInfo(Pickle* pickle, const net::HttpResponseInfo* info) {
    const bool kSkipTransientHeaders = true;
    const bool kTruncated = false;
    info->Persist(pickle, kSkipTransientHeaders, kTruncated);
    return pickle->size();
  }

  // Helpers to fill and verify blocks of memory with a value

  void FillData(char value, char* data, int data_len) {
    memset(data, value, data_len);
  }

  bool CheckData(char value, const char* data, int data_len) {
    for (int i = 0; i < data_len; ++i, ++data) {
      if (*data != value)
        return false;
    }
    return true;
  }

  // Individual Tests ---------------------------------------------------------
  // Most of the individual tests involve multiple async steps. Each test
  // is delineated with a section header.

  // DelegateReferences -------------------------------------------------------
  // TODO(michaeln): maybe this one belongs in appcache_storage_unittest.cc
  void DelegateReferences() {
    typedef scoped_refptr<AppCacheStorage::DelegateReference>
        ScopedDelegateReference;
    MockStorageDelegate delegate(this);
    ScopedDelegateReference delegate_reference1;
    ScopedDelegateReference delegate_reference2;

    EXPECT_FALSE(service_->storage()->GetDelegateReference(&delegate));

    delegate_reference1 =
        service_->storage()->GetOrCreateDelegateReference(&delegate);
    EXPECT_TRUE(delegate_reference1.get());
    EXPECT_TRUE(delegate_reference1->HasOneRef());
    EXPECT_TRUE(service_->storage()->GetDelegateReference(&delegate));
    EXPECT_EQ(&delegate,
              service_->storage()->GetDelegateReference(&delegate)->delegate);
    EXPECT_EQ(service_->storage()->GetDelegateReference(&delegate),
              service_->storage()->GetOrCreateDelegateReference(&delegate));
    delegate_reference1 = NULL;
    EXPECT_FALSE(service_->storage()->GetDelegateReference(&delegate));

    delegate_reference1 =
        service_->storage()->GetOrCreateDelegateReference(&delegate);
    service_->storage()->CancelDelegateCallbacks(&delegate);
    EXPECT_TRUE(delegate_reference1.get());
    EXPECT_TRUE(delegate_reference1->HasOneRef());
    EXPECT_FALSE(delegate_reference1->delegate);
    EXPECT_FALSE(service_->storage()->GetDelegateReference(&delegate));

    delegate_reference2 =
        service_->storage()->GetOrCreateDelegateReference(&delegate);
    EXPECT_TRUE(delegate_reference2.get());
    EXPECT_TRUE(delegate_reference2->HasOneRef());
    EXPECT_EQ(&delegate, delegate_reference2->delegate);
    EXPECT_NE(delegate_reference1.get(), delegate_reference2.get());

    TestFinished();
  }

  // ReadNonExistentResponse -------------------------------------------
  static const int64 kNoSuchResponseId = 123;

  void ReadNonExistentResponse() {
    // 1. Attempt to ReadInfo
    // 2. Attempt to ReadData

    reader_.reset(service_->storage()->CreateResponseReader(
        GURL(), kNoSuchResponseId));

    // Push tasks in reverse order
    PushNextTask(method_factory_.NewRunnableMethod(
       &AppCacheResponseTest::ReadNonExistentData));
    PushNextTask(method_factory_.NewRunnableMethod(
       &AppCacheResponseTest::ReadNonExistentInfo));
    ScheduleNextTask();
  }

  void ReadNonExistentInfo() {
    EXPECT_FALSE(reader_->IsReadPending());
    read_info_buffer_ = new HttpResponseInfoIOBuffer();
    reader_->ReadInfo(read_info_buffer_, &read_info_callback_);
    EXPECT_TRUE(reader_->IsReadPending());
    expected_read_result_ = net::ERR_CACHE_MISS;
  }

  void ReadNonExistentData() {
    EXPECT_FALSE(reader_->IsReadPending());
    read_buffer_ = new IOBuffer(kBlockSize);
    reader_->ReadData(read_buffer_, kBlockSize, &read_callback_);
    EXPECT_TRUE(reader_->IsReadPending());
    expected_read_result_ = net::ERR_CACHE_MISS;
  }

  // LoadResponseInfo_Miss ----------------------------------------------------
  void LoadResponseInfo_Miss() {
    PushNextTask(method_factory_.NewRunnableMethod(
        &AppCacheResponseTest::LoadResponseInfo_Miss_Verify));
    service_->storage()->LoadResponseInfo(GURL(), kNoSuchResponseId,
                                          storage_delegate_.get());
  }

  void LoadResponseInfo_Miss_Verify() {
    EXPECT_EQ(kNoSuchResponseId, storage_delegate_->loaded_info_id_);
    EXPECT_TRUE(!storage_delegate_->loaded_info_.get());
    TestFinished();
  }

  // LoadResponseInfo_Hit ----------------------------------------------------
  void LoadResponseInfo_Hit() {
    // This tests involves multiple async steps.
    // 1. Write a response headers and body to storage
    //   a. headers
    //   b. body
    // 2. Use LoadResponseInfo to read the response headers back out
    PushNextTask(method_factory_.NewRunnableMethod(
        &AppCacheResponseTest::LoadResponseInfo_Hit_Step2));
    writer_.reset(service_->storage()->CreateResponseWriter(GURL()));
    written_response_id_ = writer_->response_id();
    WriteBasicResponse();
  }

  void LoadResponseInfo_Hit_Step2() {
    writer_.reset();
    PushNextTask(method_factory_.NewRunnableMethod(
        &AppCacheResponseTest::LoadResponseInfo_Hit_Verify));
    service_->storage()->LoadResponseInfo(GURL(), written_response_id_,
                                          storage_delegate_.get());
  }

  void LoadResponseInfo_Hit_Verify() {
    EXPECT_EQ(written_response_id_, storage_delegate_->loaded_info_id_);
    EXPECT_TRUE(storage_delegate_->loaded_info_.get());
    EXPECT_TRUE(CompareHttpResponseInfos(
        write_info_buffer_->http_info.get(),
        storage_delegate_->loaded_info_->http_response_info()));
    TestFinished();
  }

  // WriteThenVariouslyReadResponse -------------------------------------------
  static const int kNumBlocks = 4;
  static const int kBlockSize = 1024;

  void WriteThenVariouslyReadResponse() {
    // This tests involves multiple async steps.
    // 1. First, write a large body using multiple writes, we don't bother
    //    with a response head for this test.
    // 2. Read the entire body, using multiple reads
    // 3. Read the entire body, using one read.
    // 4. Attempt to read beyond the EOF.
    // 5. Read just a range.
    // 6. Attempt to read beyond EOF of a range.

    // Push tasks in reverse order
    PushNextTask(method_factory_.NewRunnableMethod(
       &AppCacheResponseTest::ReadRangeFullyBeyondEOF));
    PushNextTask(method_factory_.NewRunnableMethod(
       &AppCacheResponseTest::ReadRangePartiallyBeyondEOF));
    PushNextTask(method_factory_.NewRunnableMethod(
       &AppCacheResponseTest::ReadPastEOF));
    PushNextTask(method_factory_.NewRunnableMethod(
       &AppCacheResponseTest::ReadRange));
    PushNextTask(method_factory_.NewRunnableMethod(
       &AppCacheResponseTest::ReadPastEOF));
    PushNextTask(method_factory_.NewRunnableMethod(
       &AppCacheResponseTest::ReadAllAtOnce));
    PushNextTask(method_factory_.NewRunnableMethod(
       &AppCacheResponseTest::ReadInBlocks));
    PushNextTask(method_factory_.NewRunnableMethod(
       &AppCacheResponseTest::WriteOutBlocks));

    // Get them going.
    ScheduleNextTask();
  }

  void WriteOutBlocks() {
    writer_.reset(service_->storage()->CreateResponseWriter(GURL()));
    written_response_id_ = writer_->response_id();
    for (int i = 0; i < kNumBlocks; ++i) {
      PushNextTask(method_factory_.NewRunnableMethod(
          &AppCacheResponseTest::WriteOneBlock, kNumBlocks - i));
    }
    ScheduleNextTask();
  }

  void WriteOneBlock(int block_number) {
    scoped_refptr<IOBuffer> io_buffer =
        new IOBuffer(kBlockSize);
    FillData(block_number, io_buffer->data(), kBlockSize);
    WriteResponseBody(io_buffer, kBlockSize);
  }

  void ReadInBlocks() {
    writer_.reset();
    reader_.reset(service_->storage()->CreateResponseReader(
        GURL(), written_response_id_));
    for (int i = 0; i < kNumBlocks; ++i) {
      PushNextTask(method_factory_.NewRunnableMethod(
          &AppCacheResponseTest::ReadOneBlock, kNumBlocks - i));
    }
    ScheduleNextTask();
  }

  void ReadOneBlock(int block_number) {
    PushNextTask(method_factory_.NewRunnableMethod(
        &AppCacheResponseTest::VerifyOneBlock, block_number));
    ReadResponseBody(new IOBuffer(kBlockSize), kBlockSize);
  }

  void VerifyOneBlock(int block_number) {
    EXPECT_TRUE(CheckData(block_number, read_buffer_->data(), kBlockSize));
    ScheduleNextTask();
  }

  void ReadAllAtOnce() {
    PushNextTask(method_factory_.NewRunnableMethod(
        &AppCacheResponseTest::VerifyAllAtOnce));
    reader_.reset(service_->storage()->CreateResponseReader(
        GURL(), written_response_id_));
    int big_size = kNumBlocks * kBlockSize;
    ReadResponseBody(new IOBuffer(big_size), big_size);
  }

  void VerifyAllAtOnce() {
    char* p = read_buffer_->data();
    for (int i = 0; i < kNumBlocks; ++i, p += kBlockSize)
      EXPECT_TRUE(CheckData(i + 1, p, kBlockSize));
    ScheduleNextTask();
  }

  void ReadPastEOF() {
    EXPECT_FALSE(reader_->IsReadPending());
    read_buffer_ = new IOBuffer(kBlockSize);
    expected_read_result_ = 0;
    reader_->ReadData(
        read_buffer_, kBlockSize, &read_callback_);
  }

  void ReadRange() {
    PushNextTask(method_factory_.NewRunnableMethod(
        &AppCacheResponseTest::VerifyRange));
    reader_.reset(service_->storage()->CreateResponseReader(
        GURL(), written_response_id_));
    reader_->SetReadRange(kBlockSize, kBlockSize);
    ReadResponseBody(new IOBuffer(kBlockSize), kBlockSize);
  }

  void VerifyRange() {
    EXPECT_TRUE(CheckData(2, read_buffer_->data(), kBlockSize));
    ScheduleNextTask();  // ReadPastEOF is scheduled next
  }

  void ReadRangePartiallyBeyondEOF() {
    PushNextTask(method_factory_.NewRunnableMethod(
        &AppCacheResponseTest::VerifyRangeBeyondEOF));
    reader_.reset(service_->storage()->CreateResponseReader(
        GURL(), written_response_id_));
    reader_->SetReadRange(kBlockSize, kNumBlocks * kBlockSize);
    ReadResponseBody(new IOBuffer(kNumBlocks * kBlockSize),
                     kNumBlocks * kBlockSize);
    expected_read_result_ = (kNumBlocks - 1) * kBlockSize;
  }

  void VerifyRangeBeyondEOF() {
    // Just verify the first 1k
    VerifyRange();
  }

  void ReadRangeFullyBeyondEOF() {
    reader_.reset(service_->storage()->CreateResponseReader(
        GURL(), written_response_id_));
    reader_->SetReadRange((kNumBlocks * kBlockSize) + 1, kBlockSize);
    ReadResponseBody(new IOBuffer(kBlockSize), kBlockSize);
    expected_read_result_ = 0;
  }

  // IOChaining -------------------------------------------
  void IOChaining() {
    // 1. Write several blocks out initiating the subsequent write
    //    from within the completion callback of the previous write.
    // 2. Read and verify several blocks in similarly chaining reads.

    // Push tasks in reverse order
    PushNextTaskAsImmediate(method_factory_.NewRunnableMethod(
       &AppCacheResponseTest::ReadInBlocksImmediately));
    PushNextTaskAsImmediate(method_factory_.NewRunnableMethod(
       &AppCacheResponseTest::WriteOutBlocksImmediately));

    // Get them going.
    ScheduleNextTask();
  }

  void WriteOutBlocksImmediately() {
    writer_.reset(service_->storage()->CreateResponseWriter(GURL()));
    written_response_id_ = writer_->response_id();
    for (int i = 0; i < kNumBlocks; ++i) {
      PushNextTaskAsImmediate(method_factory_.NewRunnableMethod(
          &AppCacheResponseTest::WriteOneBlock, kNumBlocks - i));
    }
    ScheduleNextTask();
  }

  void ReadInBlocksImmediately() {
    writer_.reset();
    reader_.reset(service_->storage()->CreateResponseReader(
        GURL(), written_response_id_));
    for (int i = 0; i < kNumBlocks; ++i) {
      PushNextTaskAsImmediate(method_factory_.NewRunnableMethod(
          &AppCacheResponseTest::ReadOneBlockImmediately, kNumBlocks - i));
    }
    ScheduleNextTask();
  }

  void ReadOneBlockImmediately(int block_number) {
    PushNextTaskAsImmediate(method_factory_.NewRunnableMethod(
        &AppCacheResponseTest::VerifyOneBlock, block_number));
    ReadResponseBody(new IOBuffer(kBlockSize), kBlockSize);
  }

  // DeleteWithinCallbacks -------------------------------------------
  void DeleteWithinCallbacks() {
    // 1. Write out a few blocks normally, and upon
    //    completion of the last write, delete the writer.
    // 2. Read in a few blocks normally, and upon completion
    //    of the last read, delete the reader.

    should_delete_reader_in_completion_callback_ = true;
    reader_deletion_count_down_ = kNumBlocks;
    should_delete_writer_in_completion_callback_ = true;
    writer_deletion_count_down_ = kNumBlocks;

    PushNextTask(method_factory_.NewRunnableMethod(
       &AppCacheResponseTest::ReadInBlocks));
    PushNextTask(method_factory_.NewRunnableMethod(
       &AppCacheResponseTest::WriteOutBlocks));
    ScheduleNextTask();
  }

  // DeleteWithIOPending -------------------------------------------
  void DeleteWithIOPending() {
    // 1. Write a few blocks normally.
    // 2. Start a write, delete with it pending.
    // 3. Start a read, delete with it pending.
    PushNextTask(method_factory_.NewRunnableMethod(
       &AppCacheResponseTest::ReadThenDelete));
    PushNextTask(method_factory_.NewRunnableMethod(
       &AppCacheResponseTest::WriteThenDelete));
    PushNextTask(method_factory_.NewRunnableMethod(
       &AppCacheResponseTest::WriteOutBlocks));
    ScheduleNextTask();
  }

  void WriteThenDelete() {
    write_callback_was_called_ = false;
    WriteOneBlock(5);
    EXPECT_TRUE(writer_->IsWritePending());
    writer_.reset();
    ScheduleNextTask();
  }

  void ReadThenDelete() {
    read_callback_was_called_ = false;
    reader_.reset(service_->storage()->CreateResponseReader(
        GURL(), written_response_id_));
    ReadResponseBody(new IOBuffer(kBlockSize), kBlockSize);
    EXPECT_TRUE(reader_->IsReadPending());
    reader_.reset();

    // Wait a moment to verify no callbacks.
    MessageLoop::current()->PostDelayedTask(FROM_HERE,
        method_factory_.NewRunnableMethod(
            &AppCacheResponseTest::VerifyNoCallbacks),
        10);
  }

  void VerifyNoCallbacks() {
    EXPECT_TRUE(!write_callback_was_called_);
    EXPECT_TRUE(!read_callback_was_called_);
    TestFinished();
  }

  // Data members

  ScopedRunnableMethodFactory<AppCacheResponseTest> method_factory_;
  scoped_ptr<base::WaitableEvent> test_finished_event_;
  scoped_ptr<MockStorageDelegate> storage_delegate_;
  scoped_ptr<MockAppCacheService> service_;
  std::stack<std::pair<Task*, bool> > task_stack_;

  scoped_ptr<AppCacheResponseReader> reader_;
  scoped_refptr<HttpResponseInfoIOBuffer> read_info_buffer_;
  scoped_refptr<IOBuffer> read_buffer_;
  int expected_read_result_;
  net::CompletionCallbackImpl<AppCacheResponseTest> read_callback_;
  net::CompletionCallbackImpl<AppCacheResponseTest> read_info_callback_;
  bool should_delete_reader_in_completion_callback_;
  int reader_deletion_count_down_;
  bool read_callback_was_called_;

  int64 written_response_id_;
  scoped_ptr<AppCacheResponseWriter> writer_;
  scoped_refptr<HttpResponseInfoIOBuffer> write_info_buffer_;
  scoped_refptr<IOBuffer> write_buffer_;
  int expected_write_result_;
  net::CompletionCallbackImpl<AppCacheResponseTest> write_callback_;
  net::CompletionCallbackImpl<AppCacheResponseTest> write_info_callback_;
  bool should_delete_writer_in_completion_callback_;
  int writer_deletion_count_down_;
  bool write_callback_was_called_;

  static scoped_ptr<base::Thread> io_thread_;
};

// static
scoped_ptr<base::Thread> AppCacheResponseTest::io_thread_;

TEST_F(AppCacheResponseTest, DelegateReferences) {
  RunTestOnIOThread(&AppCacheResponseTest::DelegateReferences);
}

TEST_F(AppCacheResponseTest, ReadNonExistentResponse) {
  RunTestOnIOThread(&AppCacheResponseTest::ReadNonExistentResponse);
}

TEST_F(AppCacheResponseTest, LoadResponseInfo_Miss) {
  RunTestOnIOThread(&AppCacheResponseTest::LoadResponseInfo_Miss);
}

TEST_F(AppCacheResponseTest, LoadResponseInfo_Hit) {
  RunTestOnIOThread(&AppCacheResponseTest::LoadResponseInfo_Hit);
}

TEST_F(AppCacheResponseTest, WriteThenVariouslyReadResponse) {
  RunTestOnIOThread(&AppCacheResponseTest::WriteThenVariouslyReadResponse);
}

TEST_F(AppCacheResponseTest, IOChaining) {
  RunTestOnIOThread(&AppCacheResponseTest::IOChaining);
}

TEST_F(AppCacheResponseTest, DeleteWithinCallbacks) {
  RunTestOnIOThread(&AppCacheResponseTest::DeleteWithinCallbacks);
}

TEST_F(AppCacheResponseTest, DeleteWithIOPending) {
  RunTestOnIOThread(&AppCacheResponseTest::DeleteWithIOPending);
}

}  // namespace appcache

