// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/env_vars.h"

namespace env_vars {

// We call running in unattended mode (for automated testing) "headless".
// This mode can be enabled using this variable or by the kNoErrorDialogs
// switch.
const char kHeadless[] = "CHROME_HEADLESS";

// The name of the log file.
const char kLogFileName[] = "CHROME_LOG_FILE";

// If this environment variable is set, Chrome on Windows will log
// to Event Tracing for Windows.
const char kEtwLogging[] = "CHROME_ETW_LOGGING";

// CHROME_CRASHED exists if a previous instance of chrome has crashed. This
// triggers the 'restart chrome' dialog. CHROME_RESTART contains the strings
// that are needed to show the dialog.
const char kShowRestart[] = "CHROME_CRASHED";
const char kRestartInfo[] = "CHROME_RESTART";

// The strings RIGHT_TO_LEFT and LEFT_TO_RIGHT indicate the locale direction.
// For example, for Hebrew and Arabic locales, we use RIGHT_TO_LEFT so that the
// dialog is displayed using the right orientation.
const char kRtlLocale[] = "RIGHT_TO_LEFT";
const char kLtrLocale[] = "LEFT_TO_RIGHT";

// If the out-of-process breakpad could not be installed, we set this variable
// according to the process.
const char kNoOOBreakpad[] = "NO_OO_BREAKPAD";

// Number of times to run a given startup_tests unit test.
const char kStartupTestsNumCycles[] = "STARTUP_TESTS_NUMCYCLES";

}  // namespace env_vars
