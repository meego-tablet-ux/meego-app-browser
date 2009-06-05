// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/command_line.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/native_library.h"
#include "base/process_util.h"
#include "base/string_util.h"

#include "chrome/test/browser/browser_test_runner.h"

// This version of the browser test launcher loads a dynamic library containing
// the tests and executes the them in that library. When the test has been run
// the library is unloaded, to ensure atexit handlers are run and static
// initializers will be run again for the next test.

namespace {

const wchar_t* const kBrowserTesLibBaseName = L"browser_tests";
const wchar_t* const kGTestListTestsFlag = L"gtest_list_tests";

class InProcBrowserTestRunner : public browser_tests::BrowserTestRunner {
 public:
  InProcBrowserTestRunner() : dynamic_lib_(NULL), run_test_proc_(NULL) {
  }

  ~InProcBrowserTestRunner() {
    if (!dynamic_lib_)
      return;
    base::UnloadNativeLibrary(dynamic_lib_);
    LOG(INFO) << "Unloaded " <<
        base::GetNativeLibraryName(kBrowserTesLibBaseName);
  }

  bool Init() {
    FilePath lib_path;
    if (!file_util::GetCurrentDirectory(&lib_path)) {
      LOG(ERROR) << "Failed to retrieve curret directory.";
      return false;
    }

    string16 lib_name = base::GetNativeLibraryName(kBrowserTesLibBaseName);
#if defined(OS_WIN)
    lib_path = lib_path.Append(lib_name);
#else
    lib_path = lib_path.Append(WideToUTF8(lib_name));
#endif
    LOG(INFO) << "Loading '" <<  lib_path.value() << "'";

    dynamic_lib_ = base::LoadNativeLibrary(lib_path);
    if (!dynamic_lib_) {
      LOG(ERROR) << "Failed to find " << lib_name;
      return false;
    }

    run_test_proc_ = reinterpret_cast<RunTestProc>(
        base::GetFunctionPointerFromNativeLibrary(dynamic_lib_, "RunTests"));
    if (!run_test_proc_) {
      LOG(ERROR) <<
          "Failed to find RunTest function in " << lib_name;
      return false;
    }

    return true;
  }

  // Returns true if the test succeeded, false if it failed.
  bool RunTest(const std::string& test_name) {
    std::string filter_flag = StringPrintf("--gtest_filter=%s",
                                           test_name.c_str());
    char* argv[2];
    argv[0] = const_cast<char*>("");
    argv[1] = const_cast<char*>(filter_flag.c_str());
    return RunAsIs(2, argv) == 0;
  }

  // Calls-in to GTest with the arguments we were started with.
  int RunAsIs(int argc, char** argv) {
    return (run_test_proc_)(argc, argv);
  }

 private:
  typedef int (CDECL *RunTestProc)(int, char**);

  base::NativeLibrary dynamic_lib_;
  RunTestProc run_test_proc_;

  DISALLOW_COPY_AND_ASSIGN(InProcBrowserTestRunner);
};

class InProcBrowserTestRunnerFactory
    : public browser_tests::BrowserTestRunnerFactory {
 public:
  InProcBrowserTestRunnerFactory() { }

  virtual browser_tests::BrowserTestRunner* CreateBrowserTestRunner() const {
    return new InProcBrowserTestRunner();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(InProcBrowserTestRunnerFactory);
};

}  // namespace

int main(int argc, char** argv) {
  CommandLine::Init(argc, argv);
  const CommandLine* command_line = CommandLine::ForCurrentProcess();

  if (command_line->HasSwitch(kGTestListTestsFlag)) {
    InProcBrowserTestRunner test_runner;
    if (!test_runner.Init())
      return 1;
    return test_runner.RunAsIs(argc, argv);
  }

  InProcBrowserTestRunnerFactory test_runner_factory;
  return browser_tests::RunTests(test_runner_factory) ? 0 : 1;
}
