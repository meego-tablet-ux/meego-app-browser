// Copyright (c) 2009, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
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

#include "breakpad/linux/memory.h"
#include "testing/gtest/include/gtest/gtest.h"

using namespace google_breakpad;

namespace {
typedef testing::Test PageAllocatorTest;
}

TEST(PageAllocatorTest, Setup) {
  PageAllocator allocator;
}

TEST(PageAllocatorTest, SmallObjects) {
  PageAllocator allocator;

  for (unsigned i = 1; i < 1024; ++i) {
    uint8_t *p = reinterpret_cast<uint8_t*>(allocator.Alloc(i));
    ASSERT_FALSE(p == NULL);
    memset(p, 0, i);
  }
}

TEST(PageAllocatorTest, LargeObject) {
  PageAllocator allocator;

  uint8_t *p = reinterpret_cast<uint8_t*>(allocator.Alloc(10000));
  ASSERT_FALSE(p == NULL);
  for (unsigned i = 1; i < 10; ++i) {
    uint8_t *p = reinterpret_cast<uint8_t*>(allocator.Alloc(i));
    ASSERT_FALSE(p == NULL);
    memset(p, 0, i);
  }
}

namespace {
typedef testing::Test WastefulVectorTest;
}

TEST(WastefulVectorTest, Setup) {
  PageAllocator allocator_;
  wasteful_vector<int> v(&allocator_);
  ASSERT_EQ(v.size(), 0u);
}

TEST(WastefulVectorTest, Simple) {
  PageAllocator allocator_;
  wasteful_vector<int> v(&allocator_);

  for (unsigned i = 0; i < 256; ++i)
    v.push_back(i);
  ASSERT_EQ(v.size(), 256u);
  for (unsigned i = 0; i < 256; ++i)
    ASSERT_EQ(v[i], i);
}
