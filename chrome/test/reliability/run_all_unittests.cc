// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/process_util.h"
#include "chrome/test/reliability/reliability_test_suite.h"

int main(int argc, char **argv) {
  base::EnableTerminationOnHeapCorruption();
  return ReliabilityTestSuite(argc, argv).Run();
}

