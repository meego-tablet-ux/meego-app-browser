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

#include <fstream>
#include <iostream>

#include "base/base_paths.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/process_util.h"
#include "base/scoped_ptr.h"
#include "base/string_util.h"
#include "chrome/installer/util/helper.h"
#include "chrome/installer/util/work_item.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
  class SetupHelperTest : public testing::Test {
   protected:
    virtual void SetUp() {
      // Name a subdirectory of the user temp directory.
      ASSERT_TRUE(PathService::Get(base::DIR_TEMP, &test_dir_));
      file_util::AppendToPath(&test_dir_, L"SetupHelperTest");

      // Create a fresh, empty copy of this test directory.
      file_util::Delete(test_dir_, true);
      CreateDirectory(test_dir_.c_str(), NULL);
      ASSERT_TRUE(file_util::PathExists(test_dir_));

      // Create a log file
      std::wstring log_file;
      ASSERT_TRUE(file_util::CreateTemporaryFileName(&log_file));
      ASSERT_TRUE(file_util::PathExists(log_file));

      logging::InitLogging(log_file.c_str(),
                           logging::LOG_ONLY_TO_FILE,
                           logging::LOCK_LOG_FILE,
                           logging::DELETE_OLD_LOG_FILE);
      logging::SetMinLogLevel(logging::LOG_INFO);
    }

    virtual void TearDown() {
      logging::CloseLogFile();
      // Clean up test directory
      ASSERT_TRUE(file_util::Delete(test_dir_, false));
      ASSERT_FALSE(file_util::PathExists(test_dir_));
    }

    // the path to temporary directory used to contain the test operations
    std::wstring test_dir_;
  };

  // Simple function to dump some text into a new file.
  void CreateTextFile(const std::wstring& filename,
                      const std::wstring& contents) {
    std::ofstream file;
    file.open(filename.c_str());
    ASSERT_TRUE(file.is_open());
    file << contents;
    file.close();
  }

  wchar_t text_content_1[] = L"delete me";
  wchar_t text_content_2[] = L"delete me as well";
};

// Delete version directories. Everything lower than the given version
// should be deleted.
TEST_F(SetupHelperTest, Delete) {
  // Create a Chrome dir
  std::wstring chrome_dir(test_dir_);
  file_util::AppendToPath(&chrome_dir, L"chrome");
  CreateDirectory(chrome_dir.c_str(), NULL);
  ASSERT_TRUE(file_util::PathExists(chrome_dir));

  std::wstring chrome_dir_1(chrome_dir);
  file_util::AppendToPath(&chrome_dir_1, L"1.0.1.0");
  CreateDirectory(chrome_dir_1.c_str(), NULL);
  ASSERT_TRUE(file_util::PathExists(chrome_dir_1));

  std::wstring chrome_dir_2(chrome_dir);
  file_util::AppendToPath(&chrome_dir_2, L"1.0.2.0");
  CreateDirectory(chrome_dir_2.c_str(), NULL);
  ASSERT_TRUE(file_util::PathExists(chrome_dir_2));

  std::wstring chrome_dir_3(chrome_dir);
  file_util::AppendToPath(&chrome_dir_3, L"1.0.3.0");
  CreateDirectory(chrome_dir_3.c_str(), NULL);
  ASSERT_TRUE(file_util::PathExists(chrome_dir_3));

  std::wstring chrome_dir_4(chrome_dir);
  file_util::AppendToPath(&chrome_dir_4, L"1.0.4.0");
  CreateDirectory(chrome_dir_4.c_str(), NULL);
  ASSERT_TRUE(file_util::PathExists(chrome_dir_4));

  std::wstring chrome_dll_1(chrome_dir_1);
  file_util::AppendToPath(&chrome_dll_1, L"chrome.dll");
  CreateTextFile(chrome_dll_1, text_content_1);
  ASSERT_TRUE(file_util::PathExists(chrome_dll_1));

  std::wstring chrome_dll_2(chrome_dir_2);
  file_util::AppendToPath(&chrome_dll_2, L"chrome.dll");
  CreateTextFile(chrome_dll_2, text_content_1);
  ASSERT_TRUE(file_util::PathExists(chrome_dll_2));

  std::wstring chrome_dll_3(chrome_dir_3);
  file_util::AppendToPath(&chrome_dll_3, L"chrome.dll");
  CreateTextFile(chrome_dll_3, text_content_1);
  ASSERT_TRUE(file_util::PathExists(chrome_dll_3));

  std::wstring chrome_dll_4(chrome_dir_4);
  file_util::AppendToPath(&chrome_dll_4, L"chrome.dll");
  CreateTextFile(chrome_dll_4, text_content_1);
  ASSERT_TRUE(file_util::PathExists(chrome_dll_4));

  std::wstring latest_version(L"1.0.4.0");
  installer::RemoveOldVersionDirs(chrome_dir, latest_version);

  // old versions should be gone
  EXPECT_FALSE(file_util::PathExists(chrome_dir_1));
  EXPECT_FALSE(file_util::PathExists(chrome_dir_2));
  EXPECT_FALSE(file_util::PathExists(chrome_dir_3));
  // the latest version should stay
  EXPECT_TRUE(file_util::PathExists(chrome_dll_4));
}

// Delete older version directories, keeping the one in used intact.
TEST_F(SetupHelperTest, DeleteInUsed) {
  // Create a Chrome dir
  std::wstring chrome_dir(test_dir_);
  file_util::AppendToPath(&chrome_dir, L"chrome");
  CreateDirectory(chrome_dir.c_str(), NULL);
  ASSERT_TRUE(file_util::PathExists(chrome_dir));

  std::wstring chrome_dir_1(chrome_dir);
  file_util::AppendToPath(&chrome_dir_1, L"1.0.1.0");
  CreateDirectory(chrome_dir_1.c_str(), NULL);
  ASSERT_TRUE(file_util::PathExists(chrome_dir_1));

  std::wstring chrome_dir_2(chrome_dir);
  file_util::AppendToPath(&chrome_dir_2, L"1.0.2.0");
  CreateDirectory(chrome_dir_2.c_str(), NULL);
  ASSERT_TRUE(file_util::PathExists(chrome_dir_2));

  std::wstring chrome_dir_3(chrome_dir);
  file_util::AppendToPath(&chrome_dir_3, L"1.0.3.0");
  CreateDirectory(chrome_dir_3.c_str(), NULL);
  ASSERT_TRUE(file_util::PathExists(chrome_dir_3));

  std::wstring chrome_dir_4(chrome_dir);
  file_util::AppendToPath(&chrome_dir_4, L"1.0.4.0");
  CreateDirectory(chrome_dir_4.c_str(), NULL);
  ASSERT_TRUE(file_util::PathExists(chrome_dir_4));

  std::wstring chrome_dll_1(chrome_dir_1);
  file_util::AppendToPath(&chrome_dll_1, L"chrome.dll");
  CreateTextFile(chrome_dll_1, text_content_1);
  ASSERT_TRUE(file_util::PathExists(chrome_dll_1));

  std::wstring chrome_dll_2(chrome_dir_2);
  file_util::AppendToPath(&chrome_dll_2, L"chrome.dll");
  CreateTextFile(chrome_dll_2, text_content_1);
  ASSERT_TRUE(file_util::PathExists(chrome_dll_2));

  // Open the file to make it in use.
  std::ofstream file;
  file.open(chrome_dll_2.c_str());

  std::wstring chrome_othera_2(chrome_dir_2);
  file_util::AppendToPath(&chrome_othera_2, L"othera.dll");
  CreateTextFile(chrome_othera_2, text_content_2);
  ASSERT_TRUE(file_util::PathExists(chrome_othera_2));

  std::wstring chrome_otherb_2(chrome_dir_2);
  file_util::AppendToPath(&chrome_otherb_2, L"otherb.dll");
  CreateTextFile(chrome_otherb_2, text_content_2);
  ASSERT_TRUE(file_util::PathExists(chrome_otherb_2));

  std::wstring chrome_dll_3(chrome_dir_3);
  file_util::AppendToPath(&chrome_dll_3, L"chrome.dll");
  CreateTextFile(chrome_dll_3, text_content_1);
  ASSERT_TRUE(file_util::PathExists(chrome_dll_3));

  std::wstring chrome_dll_4(chrome_dir_4);
  file_util::AppendToPath(&chrome_dll_4, L"chrome.dll");
  CreateTextFile(chrome_dll_4, text_content_1);
  ASSERT_TRUE(file_util::PathExists(chrome_dll_4));

  std::wstring latest_version(L"1.0.4.0");
  installer::RemoveOldVersionDirs(chrome_dir, latest_version);

  // old versions not in used should be gone
  EXPECT_FALSE(file_util::PathExists(chrome_dir_1));
  EXPECT_FALSE(file_util::PathExists(chrome_dir_3));
  // every thing under in used version should stay
  EXPECT_TRUE(file_util::PathExists(chrome_dir_2));
  EXPECT_TRUE(file_util::PathExists(chrome_dll_2));
  EXPECT_TRUE(file_util::PathExists(chrome_othera_2));
  EXPECT_TRUE(file_util::PathExists(chrome_otherb_2));
  // the latest version should stay
  EXPECT_TRUE(file_util::PathExists(chrome_dll_4));
}
