// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>
#include <dirent.h>

extern "C" {
#include <sandbox.h>
}

#include "base/file_util.h"
#include "base/file_path.h"
#include "base/hash_tables.h"
#include "base/test/multiprocess_test.h"
#include "base/sys_string_conversions.h"
#include "base/utf_string_conversions.h"
#include "chrome/common/sandbox_mac.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/multiprocess_func_list.h"

// Tests to exercise directory-access-related restrictions of Mac sandbox.

namespace sandbox {

typedef base::hash_map<std::string, SandboxSubstring>
    SandboxVariableSubstitions;

bool QuotePlainString(const std::string& str_utf8, std::string* dst);
bool QuoteStringForRegex(const std::string& str_utf8, std::string* dst);
NSString* BuildAllowDirectoryAccessSandboxString(
    const FilePath& allowed_dir,
    SandboxVariableSubstitions* substitutions);
bool PostProcessSandboxProfile(NSString* in_sandbox_data,
                               NSArray* comments_to_remove,
                               SandboxVariableSubstitions& substitutions,
                               std::string *final_sandbox_profile_str);

}  // namespace sandbox

namespace {

static const char* kSandboxAccessPathKey = "sandbox_dir";
static const char* kDeniedSuffix = "_denied";

class MacDirAccessSandboxTest : public base::MultiProcessTest {
 public:
  bool CheckSandbox(const std::string& directory_to_try) {
    setenv(kSandboxAccessPathKey, directory_to_try.c_str(), 1);
    base::ProcessHandle child_process = SpawnChild("mac_sandbox_path_access",
                                                   false);
    int code = -1;
    if (!base::WaitForExitCode(child_process, &code)) {
      LOG(WARNING) << "base::WaitForExitCode failed";
      return false;
    }
    return code == 0;
  }
};

TEST_F(MacDirAccessSandboxTest, StringEscape) {
  using sandbox::QuotePlainString;

  const struct string_escape_test_data {
  const char* to_escape;
  const char* escaped;
  } string_escape_cases[] = {
    {"", ""},
    {"\b\f\n\r\t\\\"", "\\b\\f\\n\\r\\t\\\\\\\""},
    {"/'", "/'"},
    {"sandwich", "sandwich"},
    {"(sandwich)", "(sandwich)"},
    {"^\u2135.\u2136$", "^\\u2135.\\u2136$"},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(string_escape_cases); ++i) {
    std::string out;
    std::string in(string_escape_cases[i].to_escape);
    EXPECT_TRUE(QuotePlainString(in, &out));
    EXPECT_EQ(string_escape_cases[i].escaped, out);
  }
}

TEST_F(MacDirAccessSandboxTest, RegexEscape) {
  using sandbox::QuoteStringForRegex;

  const std::string kSandboxEscapeSuffix("(/|$)");
  const struct regex_test_data {
    const wchar_t *to_escape;
    const char* escaped;
  } regex_cases[] = {
    {L"", ""},
    {L"/'", "/'"},  // / & ' characters don't need escaping.
    {L"sandwich", "sandwich"},
    {L"(sandwich)", "\\(sandwich\\)"},
  };

  // Check that all characters whose values are smaller than 32 [1F] are
  // rejected by the regex escaping code.
  {
    std::string out;
    char fail_string[] = {31, 0};
    char ok_string[] = {32, 0};
    EXPECT_FALSE(QuoteStringForRegex(fail_string, &out));
    EXPECT_TRUE(QuoteStringForRegex(ok_string, &out));
  }

  // Check that all characters whose values are larger than 126 [7E] are
  // rejected by the regex escaping code.
  {
    std::string out;
    EXPECT_TRUE(QuoteStringForRegex("}", &out));   // } == 0x7D == 125
    EXPECT_FALSE(QuoteStringForRegex("~", &out));  // ~ == 0x7E == 126
    EXPECT_FALSE(QuoteStringForRegex(WideToUTF8(L"^\u2135.\u2136$"), &out));
  }

  {
    for (size_t i = 0; i < ARRAYSIZE_UNSAFE(regex_cases); ++i) {
      std::string out;
      std::string in = WideToUTF8(regex_cases[i].to_escape);
      EXPECT_TRUE(QuoteStringForRegex(in, &out));
      std::string expected("^");
      expected.append(regex_cases[i].escaped);
      expected.append(kSandboxEscapeSuffix);
      EXPECT_EQ(expected, out);
    }
  }

  {
    std::string in_utf8("\\^.$|()[]*+?{}");
    std::string expected;
    expected.push_back('^');
    for (size_t i = 0; i < in_utf8.length(); ++i) {
      expected.push_back('\\');
      expected.push_back(in_utf8[i]);
    }
    expected.append(kSandboxEscapeSuffix);

    std::string out;
    EXPECT_TRUE(QuoteStringForRegex(in_utf8, &out));
    EXPECT_EQ(expected, out);

  }
}

// A class to handle auto-deleting a directory.
class ScopedDirectoryDelete {
 public:
  inline void operator()(FilePath* x) const {
    if (x) {
      file_util::Delete(*x, true);
    }
  }
};

typedef scoped_ptr_malloc<FilePath, ScopedDirectoryDelete> ScopedDirectory;

TEST_F(MacDirAccessSandboxTest, SandboxAccess) {
  using file_util::CreateDirectory;

  FilePath tmp_dir;
  ASSERT_TRUE(file_util::CreateNewTempDirectory("", &tmp_dir));
  // This step is important on OS X since the sandbox only understands "real"
  // paths and the paths CreateNewTempDirectory() returns are empirically in
  // /var which is a symlink to /private/var .
  sandbox::GetCanonicalSandboxPath(&tmp_dir);
  ScopedDirectory cleanup(&tmp_dir);

  const char* sandbox_dir_cases[] = {
    "simple_dir_name",
    "^hello++ $",       // Regex.
    "\\^.$|()[]*+?{}",  // All regex characters.
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(sandbox_dir_cases); ++i) {
    const char* sandbox_dir_name = sandbox_dir_cases[i];
    FilePath sandbox_dir = tmp_dir.Append(sandbox_dir_name);
    ASSERT_TRUE(CreateDirectory(sandbox_dir));
    ScopedDirectory cleanup_sandbox(&sandbox_dir);

    // Create a sibling directory of the sandbox dir, whose name has sandbox dir
    // as a substring but to which access is denied.
    std::string sibling_sandbox_dir_name_denied =
        std::string(sandbox_dir_cases[i]) + kDeniedSuffix;
    FilePath sibling_sandbox_dir = tmp_dir.Append(
                                      sibling_sandbox_dir_name_denied.c_str());
    ASSERT_TRUE(CreateDirectory(sibling_sandbox_dir));
    ScopedDirectory cleanup_sandbox_sibling(&sibling_sandbox_dir);

    EXPECT_TRUE(CheckSandbox(sandbox_dir.value()));
  }
}

MULTIPROCESS_TEST_MAIN(mac_sandbox_path_access) {
  char *sandbox_allowed_dir = getenv(kSandboxAccessPathKey);
  if (!sandbox_allowed_dir)
    return -1;

  // Build up a sandbox profile that only allows access to a single directory.
  NSString *sandbox_profile =
      @"(version 1)" \
      "(deny default)" \
      "(allow signal (target self))" \
      "(allow sysctl-read)" \
      ";ENABLE_DIRECTORY_ACCESS";

  std::string allowed_dir(sandbox_allowed_dir);
  sandbox::SandboxVariableSubstitions substitutions;
  NSString* allow_dir_sandbox_code =
      sandbox::BuildAllowDirectoryAccessSandboxString(
          FilePath(sandbox_allowed_dir),
          &substitutions);
  sandbox_profile = [sandbox_profile
      stringByReplacingOccurrencesOfString:@";ENABLE_DIRECTORY_ACCESS"
                                withString:allow_dir_sandbox_code];

  std::string final_sandbox_profile_str;
  if (!PostProcessSandboxProfile(sandbox_profile,
                                 [NSArray array],
                                 substitutions,
                                 &final_sandbox_profile_str)) {
    LOG(ERROR) << "Call to PostProcessSandboxProfile() failed";
    return -1;
  }


  // Enable Sandbox.
  char* error_buff = NULL;
  int error = sandbox_init(final_sandbox_profile_str.c_str(), 0, &error_buff);
  if (error == -1) {
    LOG(ERROR) << "Failed to Initialize Sandbox: " << error_buff;
    return -1;
  }
  sandbox_free_error(error_buff);

  // Test Sandbox.

  // We should be able to list the contents of the sandboxed directory.
  DIR *file_list = NULL;
  file_list = opendir(sandbox_allowed_dir);
  if (!file_list) {
    PLOG(ERROR) << "Sandbox overly restrictive: call to opendir("
                << sandbox_allowed_dir
                << ") failed";
    return -1;
  }
  closedir(file_list);

  // Test restrictions on accessing files.
  FilePath allowed_dir_path(sandbox_allowed_dir);
  FilePath allowed_file = allowed_dir_path.Append("ok_to_write");
  FilePath denied_file1 =  allowed_dir_path.DirName().Append("cant_access");

  // Try to write a file who's name has the same prefix as the directory we
  // allow access to.
  FilePath basename = allowed_dir_path.BaseName();
  FilePath allowed_parent_dir = allowed_dir_path.DirName();
  std::string tricky_filename = basename.value() + "123";
  FilePath denied_file2 =  allowed_parent_dir.Append(tricky_filename);

  if (open(allowed_file.value().c_str(), O_WRONLY | O_CREAT) <= 0) {
    PLOG(ERROR) << "Sandbox overly restrictive: failed to write ("
                << allowed_file.value()
                << ")";
    return -1;
  }

  // Test that we deny access to a sibling of the sandboxed directory whose
  // name has the sandboxed directory name as a substring. e.g. if the sandbox
  // directory is /foo/baz then test /foo/baz_denied.
  {
    struct stat tmp_stat_info;
    std::string denied_sibling =
        std::string(sandbox_allowed_dir) + kDeniedSuffix;
    if (stat(denied_sibling.c_str(), &tmp_stat_info) > 0) {
      PLOG(ERROR) << "Sandbox breach: was able to stat ("
                  << denied_sibling.c_str()
                  << ")";
      return -1;
    }
  }

  // Test that we can stat parent directories of the "allowed" directory.
  {
    struct stat tmp_stat_info;
    if (stat(allowed_parent_dir.value().c_str(), &tmp_stat_info) != 0) {
      PLOG(ERROR) << "Sandbox overly restrictive: unable to stat ("
                  << allowed_parent_dir.value()
                  << ")";
      return -1;
    }
  }

  // Test that we can't stat files outside the "allowed" directory.
  {
    struct stat tmp_stat_info;
    if (stat(denied_file1.value().c_str(), &tmp_stat_info) > 0) {
      PLOG(ERROR) << "Sandbox breach: was able to stat ("
                  << denied_file1.value()
                  << ")";
      return -1;
    }
  }

  if (open(denied_file1.value().c_str(), O_WRONLY | O_CREAT) > 0) {
    PLOG(ERROR) << "Sandbox breach: was able to write ("
                << denied_file1.value()
                << ")";
    return -1;
  }

  if (open(denied_file2.value().c_str(), O_WRONLY | O_CREAT) > 0) {
    PLOG(ERROR) << "Sandbox breach: was able to write ("
                << denied_file2.value()
                << ")";
    return -1;
  }

  return 0;
}

}  // namespace
