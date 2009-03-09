// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/path_service.h"
#include "base/string_util.h"
#include "chrome/browser/extensions/extension.h"
#include "chrome/browser/extensions/extensions_ui.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/json_value_serializer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
  static DictionaryValue* DeserializeJSONTestData(const FilePath& path,
      std::string *error) {
    Value* value;

    JSONFileValueSerializer serializer(path.ToWStringHack());
    value = serializer.Deserialize(error);

    return static_cast<DictionaryValue*>(value);
  }

  static bool CompareExpectedAndActualOutput(const FilePath& extension_path,
      const FilePath& expected_output_path) {
    // TODO(rafaelw): Using the extension_path passed in above, causes this
    // unit test to fail on linux. The Values come back valid, but the
    // UserScript.path() values return "".
#if defined(OS_WIN)
    FilePath path(FILE_PATH_LITERAL("c:\\foo"));
#elif defined(OS_POSIX)
  FilePath path(FILE_PATH_LITERAL("/foo"));
#endif
    Extension extension(path);
    std::string error;

    FilePath manifest_path = extension_path.AppendASCII(
        Extension::kManifestFilename);
    scoped_ptr<DictionaryValue> extension_data(DeserializeJSONTestData(
        manifest_path, &error));
    EXPECT_EQ("", error);
    EXPECT_TRUE(extension.InitFromValue(*extension_data, &error));
    EXPECT_EQ("", error);

    scoped_ptr<DictionaryValue>expected_output_data(DeserializeJSONTestData(
        expected_output_path, &error));
    EXPECT_EQ("", error);

    // Produce test output.
    scoped_ptr<DictionaryValue> actual_output_data(
        ExtensionsDOMHandler::CreateExtensionDetailValue(&extension));

    // Compare the outputs.
    return expected_output_data->Equals(actual_output_data.get());
  }
}  // namespace

class ExtensionUITest : public testing::Test {
};

TEST(ExtensionUITest, GenerateExtensionsJSONData) {
  FilePath data_test_dir_path, extension_path, expected_output_path;
  EXPECT_TRUE(PathService::Get(chrome::DIR_TEST_DATA, &data_test_dir_path));

  // Test Extension1
  extension_path = data_test_dir_path.AppendASCII("extensions")
      .AppendASCII("good")
      .AppendASCII("extension1")
      .AppendASCII("1");

  expected_output_path = data_test_dir_path.AppendASCII("extensions")
      .AppendASCII("ui")
      .AppendASCII("create_extension_detail_value_expected_output")
      .AppendASCII("good-extension1.json");

  EXPECT_TRUE(CompareExpectedAndActualOutput(extension_path,
      expected_output_path)) << extension_path.value();

  // Test Extension2
  extension_path = data_test_dir_path.AppendASCII("extensions")
      .AppendASCII("good")
      .AppendASCII("extension2")
      .AppendASCII("2");

  expected_output_path = data_test_dir_path.AppendASCII("extensions")
      .AppendASCII("ui")
      .AppendASCII("create_extension_detail_value_expected_output")
      .AppendASCII("good-extension2.json");

  EXPECT_TRUE(CompareExpectedAndActualOutput(extension_path,
      expected_output_path)) << extension_path.value();

  // Test Extension3
  extension_path = data_test_dir_path.AppendASCII("extensions")
      .AppendASCII("good")
      .AppendASCII("extension3")
      .AppendASCII("1.0");

  expected_output_path = data_test_dir_path.AppendASCII("extensions")
      .AppendASCII("ui")
      .AppendASCII("create_extension_detail_value_expected_output")
      .AppendASCII("good-extension3.json");

  EXPECT_TRUE(CompareExpectedAndActualOutput(extension_path,
      expected_output_path)) << extension_path.value();
}
