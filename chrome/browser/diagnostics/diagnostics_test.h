// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DIAGNOSTICS_DIAGNOSTICS_TEST_H_
#define CHROME_BROWSER_DIAGNOSTICS_DIAGNOSTICS_TEST_H_

#include "base/file_path.h"
#include "base/path_service.h"
#include "base/string16.h"
#include "chrome/browser/diagnostics/diagnostics_model.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"

// Represents a single diagnostic test and encapsulates the common
// functionality across platforms as well.
// It also Implements the TestInfo interface providing the storage
// for the outcome of the test.
// Specific tests need (minimally) only to:
// 1- override ExecuteImpl() to imnplement the test.
// 2- call RecordStopFailure() or RecordFailure() or RecordSuccess()
//    at the end of the test.
// 3- Optionally call observer->OnProgress() if the test is long.
// 4- Optionally call observer->OnSkipped() if the test cannot be run.
class DiagnosticTest : public DiagnosticsModel::TestInfo {
 public:
  // |title| is the human readable, localized string that says that
  // the objective of the test is.
  explicit DiagnosticTest(const string16& title)
      : title_(title), result_(DiagnosticsModel::TEST_NOT_RUN) {}

  virtual ~DiagnosticTest() {}

  // Runs the test. Returning false signals that no more tests should be run.
  // The actual outcome of the test should be set using the RecordXX functions.
  bool Execute(DiagnosticsModel::Observer* observer, DiagnosticsModel* model,
               size_t index) {
    result_ = DiagnosticsModel::TEST_RUNNING;
    observer->OnProgress(index, 0, model);
    bool keep_going = ExecuteImpl(observer);
    observer->OnFinished(index, model);
    return keep_going;
  }

  virtual string16 GetTitle() {
    return title_;
  }

  virtual DiagnosticsModel::TestResult GetResult() {
    return result_;
  }

  virtual string16 GetAdditionalInfo() {
    return additional_info_;
  }

  void RecordStopFailure(const string16& additional_info) {
    RecordOutcome(additional_info, DiagnosticsModel::TEST_FAIL_STOP);
  }

  void RecordFailure(const string16& additional_info) {
    RecordOutcome(additional_info, DiagnosticsModel::TEST_FAIL_CONTINUE);
  }

  void RecordSuccess(const string16& additional_info) {
    RecordOutcome(additional_info, DiagnosticsModel::TEST_OK);
  }

  void RecordOutcome(const string16& additional_info,
                     DiagnosticsModel::TestResult result) {
    additional_info_ = additional_info;
    result_ = result;
  }

  FilePath GetUserDefaultProfileDir() {
    FilePath path;
    if (!PathService::Get(chrome::DIR_USER_DATA, &path))
      return FilePath();
    return path.Append(FilePath::FromWStringHack(chrome::kNotSignedInProfile));
  }

 protected:
  // The id needs to be overriden by derived classes and must uniquely
  // identify this test so other test can refer to it.
  virtual int GetId() = 0;
  // Derived classes override this method do perform the actual test.
  virtual bool ExecuteImpl(DiagnosticsModel::Observer* observer) = 0;

  string16 title_;
  string16 additional_info_;
  DiagnosticsModel::TestResult result_;
};

#endif  // CHROME_BROWSER_DIAGNOSTICS_DIAGNOSTICS_TEST_H_
