/*
 * Copyright 2009, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// This file contains unit tests for the performance timer.

#include "core/cross/precompile.h"

#include "core/cross/performance_timer.h"
#include "tests/common/win/testing_common.h"

namespace o3d {

const char* kTimerName = "MyGroovyTimer";

class PerformanceTimerTest : public testing::Test {
 protected:
  virtual void SetUp();
  virtual void TearDown();
  PerformanceTimer* timer() { return timer_; }

 private:
  PerformanceTimer* timer_;
};

void PerformanceTimerTest::SetUp() {
  timer_ = new PerformanceTimer(kTimerName);
}

void PerformanceTimerTest::TearDown() {
  delete timer_;
}

TEST_F(PerformanceTimerTest, Name) {
  EXPECT_TRUE(!strcmp(timer()->name(), kTimerName));
}

TEST_F(PerformanceTimerTest, StartStop) {
  ASSERT_TRUE(timer() != NULL);
  EXPECT_TRUE(timer()->GetElapsedTime() < 0.00001);
  timer()->Start();
  timer()->Stop();
  EXPECT_TRUE(timer()->GetElapsedTime() < 1.0);
  volatile float a = 0.0f;
  timer()->Start();
  for (int i = 0; i < 1000000; ++i) {
    a += 1;
  }
  timer()->StopAndPrint();
  EXPECT_TRUE(timer()->GetElapsedTime() > 0.0);
}
};
