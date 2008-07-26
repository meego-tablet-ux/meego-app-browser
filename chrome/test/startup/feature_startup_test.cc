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

#include <windows.h>

#include "base/file_util.h"
#include "base/path_service.h"
#include "base/perftimer.h"
#include "base/time.h"
#include "chrome/app/chrome_dll_resource.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/win_util.h"
#include "chrome/test/automation/browser_proxy.h"
#include "chrome/test/ui/ui_test.h"
#include "net/base/net_util.h"

namespace {

// Returns the directory name where the "typical" user data is that we use for
// testing.
std::wstring ComputeTypicalUserDataSource() {
  std::wstring source_history_file;
  EXPECT_TRUE(PathService::Get(chrome::DIR_TEST_DATA,
                               &source_history_file));
  file_util::AppendToPath(&source_history_file, L"profiles");
  file_util::AppendToPath(&source_history_file, L"typical_history");
  return source_history_file;
}

class NewTabUIStartupTest : public UITest {
 public:
  NewTabUIStartupTest() {
    show_window_ = true;
  }

  void SetUp() {}
  void TearDown() {}

  static const int kNumCycles = 5;

  void PrintTimings(const char* label, TimeDelta timings[kNumCycles]) {
    printf("\n%s = [", label);
    for (int i = 0; i < kNumCycles; ++i) {
      if (i > 0)
        printf(",");
      printf("%.2f", timings[i].InMillisecondsF());
    }
    printf("]\n");
  }

  // Run the test, by bringing up a browser and timing the new tab startup.
  // |want_warm| is true if we should output warm-disk timings, false if
  // we should report cold timings.
  void RunStartupTest(bool want_warm) {
    // Install the location of the test profile file.
    set_template_user_data(ComputeTypicalUserDataSource());

    TimeDelta timings[kNumCycles];
    for (int i = 0; i < kNumCycles; ++i) {
      UITest::SetUp();

      // Switch to the "new tab" tab, which should be any new tab after the
      // first (the first is about:blank).
      BrowserProxy* window = automation()->GetBrowserWindow(0);
      ASSERT_TRUE(window);
      int old_tab_count = -1;
      ASSERT_TRUE(window->GetTabCount(&old_tab_count));
      ASSERT_EQ(1, old_tab_count);

      // Hit ctl-t and wait for the tab to load.
      window->ApplyAccelerator(IDC_NEWTAB);
      int new_tab_count = -1;
      ASSERT_TRUE(window->WaitForTabCountToChange(old_tab_count, &new_tab_count,
                                                  5000));
      ASSERT_EQ(2, new_tab_count);
      int load_time;
      ASSERT_TRUE(automation()->WaitForInitialNewTabUILoad(&load_time));
      timings[i] = TimeDelta::FromMilliseconds(load_time);

      if (want_warm) {
        // Bring up a second tab, now that we've already shown one tab.
        old_tab_count = new_tab_count;
        new_tab_count = -1;
        window->ApplyAccelerator(IDC_NEWTAB);
        ASSERT_TRUE(window->WaitForTabCountToChange(old_tab_count, &new_tab_count,
                                                    5000));
        ASSERT_EQ(3, new_tab_count);
        ASSERT_TRUE(automation()->WaitForInitialNewTabUILoad(&load_time));
        timings[i] = TimeDelta::FromMilliseconds(load_time);
      }

      delete window;
      UITest::TearDown();
    }

    // The buildbot log-scraper looks for this "__.._pages" line to tell when
    // the test has completed and how many pages it loaded.
    printf("\n__ts_pages = [about:blank]\n");
    PrintTimings("__ts_timings", timings);
  }
};

// The name of this test is important, since the buildbot runs with a gTest
// filter.
typedef NewTabUIStartupTest NewTabUIStartupTestReference;

}  // namespace

TEST_F(NewTabUIStartupTest, PerfCold) {
  RunStartupTest(false);
}

TEST_F(NewTabUIStartupTest, DISABLED_PerfWarm) {
  RunStartupTest(true);
}

TEST_F(NewTabUIStartupTestReference, FakePerfForLogScraperCold) {
  // Print an empty reference-test result line so the log-scraper is happy.
  // TODO(pamg): really run the test with a reference build?
  TimeDelta timings[kNumCycles];
  for (int i = 0; i < kNumCycles; ++i)
    timings[i] = TimeDelta::FromMilliseconds(0);
  PrintTimings("__ts_reference_timings", timings);
}

TEST_F(NewTabUIStartupTestReference, FakePerfForLogScraperWarm) {
  // Print an empty reference-test result line so the log-scraper is happy.
  // TODO(pamg): really run the test with a reference build?
  TimeDelta timings[kNumCycles];
  for (int i = 0; i < kNumCycles; ++i)
    timings[i] = TimeDelta::FromMilliseconds(0);
  PrintTimings("__ts_reference_timings", timings);
}