// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains the tests for the RingBuffer class.

#include "gpu/command_buffer/client/ring_buffer.h"
#include "base/at_exit.h"
#include "base/callback.h"
#include "base/message_loop.h"
#include "base/scoped_nsautorelease_pool.h"
#include "gpu/command_buffer/client/cmd_buffer_helper.h"
#include "gpu/command_buffer/service/cmd_buffer_engine.h"
#include "gpu/command_buffer/service/mocks.h"
#include "gpu/command_buffer/service/command_buffer_service.h"
#include "gpu/command_buffer/service/gpu_processor.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gpu {

using testing::Return;
using testing::Mock;
using testing::Truly;
using testing::Sequence;
using testing::DoAll;
using testing::Invoke;
using testing::_;

class BaseRingBufferTest : public testing::Test {
 protected:
  static const unsigned int kBaseOffset = 128;
  static const unsigned int kBufferSize = 1024;

  virtual void SetUp() {
    api_mock_.reset(new AsyncAPIMock);
    // ignore noops in the mock - we don't want to inspect the internals of the
    // helper.
    EXPECT_CALL(*api_mock_, DoCommand(cmd::kNoop, 0, _))
        .WillRepeatedly(Return(error::kNoError));
    // Forward the SetToken calls to the engine
    EXPECT_CALL(*api_mock_.get(), DoCommand(cmd::kSetToken, 1, _))
        .WillRepeatedly(DoAll(Invoke(api_mock_.get(), &AsyncAPIMock::SetToken),
                              Return(error::kNoError)));

    command_buffer_.reset(new CommandBufferService);
    command_buffer_->Initialize(kBufferSize / sizeof(CommandBufferEntry));
    Buffer ring_buffer = command_buffer_->GetRingBuffer();

    parser_ = new CommandParser(ring_buffer.ptr,
                                ring_buffer.size,
                                0,
                                ring_buffer.size,
                                0,
                                api_mock_.get());

    gpu_processor_.reset(new GPUProcessor(
        command_buffer_.get(), NULL, parser_, INT_MAX));
    command_buffer_->SetPutOffsetChangeCallback(NewCallback(
        gpu_processor_.get(), &GPUProcessor::ProcessCommands));

    api_mock_->set_engine(gpu_processor_.get());

    helper_.reset(new CommandBufferHelper(command_buffer_.get()));
    helper_->Initialize();
  }

  int32 GetToken() {
    return command_buffer_->GetState().token;
  }

  virtual void TearDown() {
    helper_.release();
  }

  base::ScopedNSAutoreleasePool autorelease_pool_;
  base::AtExitManager at_exit_manager_;
  MessageLoop message_loop_;
  scoped_ptr<AsyncAPIMock> api_mock_;
  scoped_ptr<CommandBufferService> command_buffer_;
  scoped_ptr<GPUProcessor> gpu_processor_;
  CommandParser* parser_;
  scoped_ptr<CommandBufferHelper> helper_;
};

#ifndef COMPILER_MSVC
const unsigned int BaseRingBufferTest::kBaseOffset;
const unsigned int BaseRingBufferTest::kBufferSize;
#endif

// Test fixture for RingBuffer test - Creates a RingBuffer, using a
// CommandBufferHelper with a mock AsyncAPIInterface for its interface (calling
// it directly, not through the RPC mechanism), making sure Noops are ignored
// and SetToken are properly forwarded to the engine.
class RingBufferTest : public BaseRingBufferTest {
 protected:
  virtual void SetUp() {
    BaseRingBufferTest::SetUp();
    allocator_.reset(new RingBuffer(kBaseOffset, kBufferSize, helper_.get()));
  }

  virtual void TearDown() {
    // If the GPUProcessor posts any tasks, this forces them to run.
    MessageLoop::current()->RunAllPending();

    allocator_.release();

    BaseRingBufferTest::TearDown();
  }

  scoped_ptr<RingBuffer> allocator_;
};

// Checks basic alloc and free.
TEST_F(RingBufferTest, TestBasic) {
  const unsigned int kSize = 16;
  EXPECT_EQ(kBufferSize, allocator_->GetLargestFreeOrPendingSize());
  EXPECT_EQ(kBufferSize, allocator_->GetLargestFreeSizeNoWaiting());
  RingBuffer::Offset offset = allocator_->Alloc(kSize);
  EXPECT_GE(kBufferSize, offset - kBaseOffset + kSize);
  EXPECT_EQ(kBufferSize, allocator_->GetLargestFreeOrPendingSize());
  EXPECT_EQ(kBufferSize - kSize, allocator_->GetLargestFreeSizeNoWaiting());
  int32 token = helper_.get()->InsertToken();
  allocator_->FreePendingToken(offset, token);
}

// Checks the free-pending-token mechanism.
TEST_F(RingBufferTest, TestFreePendingToken) {
  const unsigned int kSize = 16;
  const unsigned int kAllocCount = kBufferSize / kSize;
  CHECK(kAllocCount * kSize == kBufferSize);

  // Allocate several buffers to fill in the memory.
  int32 tokens[kAllocCount];
  for (unsigned int ii = 0; ii < kAllocCount; ++ii) {
    RingBuffer::Offset offset = allocator_->Alloc(kSize);
    EXPECT_GE(kBufferSize, offset - kBaseOffset + kSize);
    tokens[ii] = helper_.get()->InsertToken();
    allocator_->FreePendingToken(offset, tokens[ii]);
  }

  EXPECT_EQ(kBufferSize - (kSize * kAllocCount),
            allocator_->GetLargestFreeSizeNoWaiting());

  // This allocation will need to reclaim the space freed above, so that should
  // process the commands until a token is passed.
  RingBuffer::Offset offset1 = allocator_->Alloc(kSize);
  EXPECT_EQ(kBaseOffset, offset1);

  // Check that the token has indeed passed.
  EXPECT_LE(tokens[0], GetToken());

  allocator_->FreePendingToken(offset1, helper_.get()->InsertToken());
}

// Tests GetLargestFreeSizeNoWaiting
TEST_F(RingBufferTest, TestGetLargestFreeSizeNoWaiting) {
  EXPECT_EQ(kBufferSize, allocator_->GetLargestFreeSizeNoWaiting());

  RingBuffer::Offset offset = allocator_->Alloc(kBufferSize);
  EXPECT_EQ(0u, allocator_->GetLargestFreeSizeNoWaiting());
  allocator_->FreePendingToken(offset, helper_.get()->InsertToken());
}

// Test fixture for RingBufferWrapper test - Creates a
// RingBufferWrapper, using a CommandBufferHelper with a mock
// AsyncAPIInterface for its interface (calling it directly, not through the
// RPC mechanism), making sure Noops are ignored and SetToken are properly
// forwarded to the engine.
class RingBufferWrapperTest : public BaseRingBufferTest {
 protected:
  virtual void SetUp() {
    BaseRingBufferTest::SetUp();

    // Though allocating this buffer isn't strictly necessary, it makes
    // allocations point to valid addresses, so they could be used for
    // something.
    buffer_.reset(new int8[kBufferSize + kBaseOffset]);
    buffer_start_ = buffer_.get() + kBaseOffset;
    allocator_.reset(new RingBufferWrapper(
        kBaseOffset, kBufferSize, helper_.get(), buffer_start_));
  }

  virtual void TearDown() {
    // If the GPUProcessor posts any tasks, this forces them to run.
    MessageLoop::current()->RunAllPending();

    allocator_.release();
    buffer_.release();

    BaseRingBufferTest::TearDown();
  }

  scoped_ptr<RingBufferWrapper> allocator_;
  scoped_array<int8> buffer_;
  int8* buffer_start_;
};

// Checks basic alloc and free.
TEST_F(RingBufferWrapperTest, TestBasic) {
  const unsigned int kSize = 16;
  void* pointer = allocator_->Alloc(kSize);
  ASSERT_TRUE(pointer);
  EXPECT_LE(buffer_start_, static_cast<int8*>(pointer));
  EXPECT_GE(kBufferSize, static_cast<int8*>(pointer) - buffer_start_ + kSize);

  allocator_->FreePendingToken(pointer, helper_.get()->InsertToken());

  int8* pointer_int8 = allocator_->AllocTyped<int8>(kSize);
  ASSERT_TRUE(pointer_int8);
  EXPECT_LE(buffer_start_, pointer_int8);
  EXPECT_GE(buffer_start_ + kBufferSize, pointer_int8 + kSize);
  allocator_->FreePendingToken(pointer_int8, helper_.get()->InsertToken());

  unsigned int* pointer_uint = allocator_->AllocTyped<unsigned int>(kSize);
  ASSERT_TRUE(pointer_uint);
  EXPECT_LE(buffer_start_, reinterpret_cast<int8*>(pointer_uint));
  EXPECT_GE(buffer_start_ + kBufferSize,
            reinterpret_cast<int8* >(pointer_uint + kSize));

  // Check that it did allocate kSize * sizeof(unsigned int). We can't tell
  // directly, except from the remaining size.
  EXPECT_EQ(kBufferSize - kSize - kSize - kSize * sizeof(*pointer_uint),
            allocator_->GetLargestFreeSizeNoWaiting());
  allocator_->FreePendingToken(pointer_uint, helper_.get()->InsertToken());
}

// Checks the free-pending-token mechanism.
TEST_F(RingBufferWrapperTest, TestFreePendingToken) {
  const unsigned int kSize = 16;
  const unsigned int kAllocCount = kBufferSize / kSize;
  CHECK(kAllocCount * kSize == kBufferSize);

  // Allocate several buffers to fill in the memory.
  int32 tokens[kAllocCount];
  for (unsigned int ii = 0; ii < kAllocCount; ++ii) {
    void* pointer = allocator_->Alloc(kSize);
    EXPECT_TRUE(pointer != NULL);
    tokens[ii] = helper_.get()->InsertToken();
    allocator_->FreePendingToken(pointer, helper_.get()->InsertToken());
  }

  EXPECT_EQ(kBufferSize - (kSize * kAllocCount),
            allocator_->GetLargestFreeSizeNoWaiting());

  // This allocation will need to reclaim the space freed above, so that should
  // process the commands until the token is passed.
  void* pointer1 = allocator_->Alloc(kSize);
  EXPECT_EQ(buffer_start_, static_cast<int8*>(pointer1));

  // Check that the token has indeed passed.
  EXPECT_LE(tokens[0], GetToken());

  allocator_->FreePendingToken(pointer1, helper_.get()->InsertToken());
}

}  // namespace gpu
