// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "app/text_elider.h"
#include "base/file_path.h"
#include "base/i18n/rtl.h"
#include "base/scoped_ptr.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "gfx/font.h"
#include "googleurl/src/gurl.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const wchar_t kEllipsis[] = L"\x2026";

struct Testcase {
  const std::string input;
  const std::wstring output;
};

struct FileTestcase {
  const FilePath::StringType input;
  const std::wstring output;
};

struct UTF16Testcase {
  const string16 input;
  const string16 output;
};

struct TestData {
  const std::string a;
  const std::string b;
  const int compare_result;
};

void RunTest(Testcase* testcases, size_t num_testcases) {
  static const gfx::Font font;
  for (size_t i = 0; i < num_testcases; ++i) {
    const GURL url(testcases[i].input);
    // Should we test with non-empty language list?
    // That's kinda redundant with net_util_unittests.
    EXPECT_EQ(WideToUTF16(testcases[i].output),
              ElideUrl(url, font, font.GetStringWidth(testcases[i].output),
                       std::wstring()));
  }
}

}  // namespace

// Test eliding of commonplace URLs.
TEST(TextEliderTest, TestGeneralEliding) {
  const std::wstring kEllipsisStr(kEllipsis);
  Testcase testcases[] = {
    {"http://www.google.com/intl/en/ads/",
     L"www.google.com/intl/en/ads/"},
    {"http://www.google.com/intl/en/ads/", L"www.google.com/intl/en/ads/"},
// TODO(port): make this test case work on mac.
#if !defined(OS_MACOSX)
    {"http://www.google.com/intl/en/ads/",
     L"google.com/intl/" + kEllipsisStr + L"/ads/"},
#endif
    {"http://www.google.com/intl/en/ads/",
     L"google.com/" + kEllipsisStr + L"/ads/"},
    {"http://www.google.com/intl/en/ads/", L"google.com/" + kEllipsisStr},
    {"http://www.google.com/intl/en/ads/", L"goog" + kEllipsisStr},
    {"https://subdomain.foo.com/bar/filename.html",
     L"subdomain.foo.com/bar/filename.html"},
    {"https://subdomain.foo.com/bar/filename.html",
     L"subdomain.foo.com/" + kEllipsisStr + L"/filename.html"},
    {"http://subdomain.foo.com/bar/filename.html",
     kEllipsisStr + L"foo.com/" + kEllipsisStr + L"/filename.html"},
    {"http://www.google.com/intl/en/ads/?aLongQueryWhichIsNotRequired",
     L"www.google.com/intl/en/ads/?aLongQ" + kEllipsisStr},
  };

  RunTest(testcases, arraysize(testcases));
}

// Test eliding of empty strings, URLs with ports, passwords, queries, etc.
TEST(TextEliderTest, TestMoreEliding) {
  const std::wstring kEllipsisStr(kEllipsis);
  Testcase testcases[] = {
    {"http://www.google.com/foo?bar", L"www.google.com/foo?bar"},
    {"http://xyz.google.com/foo?bar", L"xyz.google.com/foo?" + kEllipsisStr},
    {"http://xyz.google.com/foo?bar", L"xyz.google.com/foo" + kEllipsisStr},
    {"http://xyz.google.com/foo?bar", L"xyz.google.com/fo" + kEllipsisStr},
    {"http://a.b.com/pathname/c?d", L"a.b.com/" + kEllipsisStr + L"/c?d"},
    {"", L""},
    {"http://foo.bar..example.com...hello/test/filename.html",
     L"foo.bar..example.com...hello/" + kEllipsisStr + L"/filename.html"},
    {"http://foo.bar../", L"foo.bar.."},
    {"http://xn--1lq90i.cn/foo", L"\x5317\x4eac.cn/foo"},
    {"http://me:mypass@secrethost.com:99/foo?bar#baz",
     L"secrethost.com:99/foo?bar#baz"},
    {"http://me:mypass@ss%xxfdsf.com/foo", L"ss%25xxfdsf.com/foo"},
    {"mailto:elgoato@elgoato.com", L"mailto:elgoato@elgoato.com"},
    {"javascript:click(0)", L"javascript:click(0)"},
    {"https://chess.eecs.berkeley.edu:4430/login/arbitfilename",
     L"chess.eecs.berkeley.edu:4430/login/arbitfilename"},
    {"https://chess.eecs.berkeley.edu:4430/login/arbitfilename",
     kEllipsisStr + L"berkeley.edu:4430/" + kEllipsisStr + L"/arbitfilename"},

    // Unescaping.
    {"http://www/%E4%BD%A0%E5%A5%BD?q=%E4%BD%A0%E5%A5%BD#\xe4\xbd\xa0",
     L"www/\x4f60\x597d?q=\x4f60\x597d#\x4f60"},

    // Invalid unescaping for path. The ref will always be valid UTF-8. We don't
    // bother to do too many edge cases, since these are handled by the escaper
    // unittest.
    {"http://www/%E4%A0%E5%A5%BD?q=%E4%BD%A0%E5%A5%BD#\xe4\xbd\xa0",
     L"www/%E4%A0%E5%A5%BD?q=\x4f60\x597d#\x4f60"},
  };

  RunTest(testcases, arraysize(testcases));
}

// Test eliding of file: URLs.
TEST(TextEliderTest, TestFileURLEliding) {
  const std::wstring kEllipsisStr(kEllipsis);
  Testcase testcases[] = {
    {"file:///C:/path1/path2/path3/filename",
     L"file:///C:/path1/path2/path3/filename"},
    {"file:///C:/path1/path2/path3/filename",
     L"C:/path1/path2/path3/filename"},
// GURL parses "file:///C:path" differently on windows than it does on posix.
#if defined(OS_WIN)
    {"file:///C:path1/path2/path3/filename",
     L"C:/path1/path2/" + kEllipsisStr + L"/filename"},
    {"file:///C:path1/path2/path3/filename",
     L"C:/path1/" + kEllipsisStr + L"/filename"},
    {"file:///C:path1/path2/path3/filename",
     L"C:/" + kEllipsisStr + L"/filename"},
#endif
    {"file://filer/foo/bar/file", L"filer/foo/bar/file"},
    {"file://filer/foo/bar/file", L"filer/foo/" + kEllipsisStr + L"/file"},
    {"file://filer/foo/bar/file", L"filer/" + kEllipsisStr + L"/file"},
  };

  RunTest(testcases, arraysize(testcases));
}

TEST(TextEliderTest, TestFilenameEliding) {
  const std::wstring kEllipsisStr(kEllipsis);
  const FilePath::StringType kPathSeparator =
      FilePath::StringType().append(1, FilePath::kSeparators[0]);

  FileTestcase testcases[] = {
    {FILE_PATH_LITERAL(""), L""},
    {FILE_PATH_LITERAL("."), L"."},
    {FILE_PATH_LITERAL("filename.exe"), L"filename.exe"},
    {FILE_PATH_LITERAL(".longext"), L".longext"},
    {FILE_PATH_LITERAL("pie"), L"pie"},
    {FILE_PATH_LITERAL("c:") + kPathSeparator + FILE_PATH_LITERAL("path") +
      kPathSeparator + FILE_PATH_LITERAL("filename.pie"),
      L"filename.pie"},
    {FILE_PATH_LITERAL("c:") + kPathSeparator + FILE_PATH_LITERAL("path") +
      kPathSeparator + FILE_PATH_LITERAL("longfilename.pie"),
      L"long" + kEllipsisStr + L".pie"},
    {FILE_PATH_LITERAL("http://path.com/filename.pie"), L"filename.pie"},
    {FILE_PATH_LITERAL("http://path.com/longfilename.pie"),
      L"long" + kEllipsisStr + L".pie"},
    {FILE_PATH_LITERAL("piesmashingtacularpants"), L"pie" + kEllipsisStr},
    {FILE_PATH_LITERAL(".piesmashingtacularpants"), L".pie" + kEllipsisStr},
    {FILE_PATH_LITERAL("cheese."), L"cheese."},
    {FILE_PATH_LITERAL("file name.longext"),
      L"file" + kEllipsisStr + L".longext"},
    {FILE_PATH_LITERAL("fil ename.longext"),
      L"fil " + kEllipsisStr + L".longext"},
    {FILE_PATH_LITERAL("filename.longext"),
      L"file" + kEllipsisStr + L".longext"},
    {FILE_PATH_LITERAL("filename.middleext.longext"),
      L"filename.mid" + kEllipsisStr + L".longext"}
  };

  static const gfx::Font font;
  for (size_t i = 0; i < arraysize(testcases); ++i) {
    FilePath filepath(testcases[i].input);
    string16 expected = WideToUTF16(testcases[i].output);
    expected = base::i18n::GetDisplayStringInLTRDirectionality(expected);
    EXPECT_EQ(expected, ElideFilename(filepath,
        font,
        font.GetStringWidth(testcases[i].output)));
  }
}

TEST(TextEliderTest, ElideTextLongStrings) {
  const string16 kEllipsisStr(WideToUTF16(kEllipsis));
  string16 data_scheme(UTF8ToUTF16("data:text/plain,"));
  size_t data_scheme_length = data_scheme.length();

  string16 ten_a(10, 'a');
  string16 hundred_a(100, 'a');
  string16 thousand_a(1000, 'a');
  string16 ten_thousand_a(10000, 'a');
  string16 hundred_thousand_a(100000, 'a');
  string16 million_a(1000000, 'a');

  size_t number_of_as = 156;
  string16 long_string_end(
      data_scheme + string16(number_of_as, 'a') + kEllipsisStr);
  UTF16Testcase testcases_end[] = {
     {data_scheme + ten_a,              data_scheme + ten_a},
     {data_scheme + hundred_a,          data_scheme + hundred_a},
     {data_scheme + thousand_a,         long_string_end},
     {data_scheme + ten_thousand_a,     long_string_end},
     {data_scheme + hundred_thousand_a, long_string_end},
     {data_scheme + million_a,          long_string_end},
  };

  const gfx::Font font;
  int ellipsis_width = font.GetStringWidth(UTF16ToWideHack(kEllipsisStr));
  for (size_t i = 0; i < arraysize(testcases_end); ++i) {
    // Compare sizes rather than actual contents because if the test fails,
    // output is rather long.
    EXPECT_EQ(testcases_end[i].output.size(),
              ElideText(testcases_end[i].input, font,
                        font.GetStringWidth(UTF16ToWideHack(
                            testcases_end[i].output)),
                        false).size());
    EXPECT_EQ(kEllipsisStr,
              ElideText(testcases_end[i].input, font, ellipsis_width, false));
  }

  size_t number_of_trailing_as = (data_scheme_length + number_of_as) / 2;
  string16 long_string_middle(data_scheme +
      string16(number_of_as - number_of_trailing_as, 'a') + kEllipsisStr +
      string16(number_of_trailing_as, 'a'));
  UTF16Testcase testcases_middle[] = {
     {data_scheme + ten_a,              data_scheme + ten_a},
     {data_scheme + hundred_a,          data_scheme + hundred_a},
     {data_scheme + thousand_a,         long_string_middle},
     {data_scheme + ten_thousand_a,     long_string_middle},
     {data_scheme + hundred_thousand_a, long_string_middle},
     {data_scheme + million_a,          long_string_middle},
  };

  for (size_t i = 0; i < arraysize(testcases_middle); ++i) {
    // Compare sizes rather than actual contents because if the test fails,
    // output is rather long.
    EXPECT_EQ(testcases_middle[i].output.size(),
              ElideText(testcases_middle[i].input, font,
                        font.GetStringWidth(UTF16ToWideHack(
                            testcases_middle[i].output)),
                        false).size());
    EXPECT_EQ(kEllipsisStr,
              ElideText(testcases_middle[i].input, font, ellipsis_width,
                        false));
  }
}

// Verifies display_url is set correctly.
TEST(TextEliderTest, SortedDisplayURL) {
  gfx::SortedDisplayURL d_url(GURL("http://www.google.com"), std::wstring());
  EXPECT_EQ("www.google.com", UTF16ToASCII(d_url.display_url()));
}

// Verifies DisplayURL::Compare works correctly.
TEST(TextEliderTest, SortedDisplayURLCompare) {
  UErrorCode create_status = U_ZERO_ERROR;
  scoped_ptr<icu::Collator> collator(
      icu::Collator::createInstance(create_status));
  if (!U_SUCCESS(create_status))
    return;

  TestData tests[] = {
    // IDN comparison. Hosts equal, so compares on path.
    { "http://xn--1lq90i.cn/a", "http://xn--1lq90i.cn/b", -1},

    // Because the host and after host match, this compares the full url.
    { "http://www.x/b", "http://x/b", -1 },

    // Because the host and after host match, this compares the full url.
    { "http://www.a:1/b", "http://a:1/b", 1 },

    // The hosts match, so these end up comparing on the after host portion.
    { "http://www.x:0/b", "http://x:1/b", -1 },
    { "http://www.x/a", "http://x/b", -1 },
    { "http://x/b", "http://www.x/a", 1 },

    // Trivial Equality.
    { "http://a/", "http://a/", 0 },

    // Compares just hosts.
    { "http://www.a/", "http://b/", -1 },
  };

  for (size_t i = 0; i < arraysize(tests); ++i) {
    gfx::SortedDisplayURL url1(GURL(tests[i].a), std::wstring());
    gfx::SortedDisplayURL url2(GURL(tests[i].b), std::wstring());
    EXPECT_EQ(tests[i].compare_result, url1.Compare(url2, collator.get()));
    EXPECT_EQ(-tests[i].compare_result, url2.Compare(url1, collator.get()));
  }
}

TEST(TextEliderTest, ElideString) {
  struct TestData {
    const wchar_t* input;
    int max_len;
    bool result;
    const wchar_t* output;
  } cases[] = {
    { L"Hello", 0, true, L"" },
    { L"", 0, false, L"" },
    { L"Hello, my name is Tom", 1, true, L"H" },
    { L"Hello, my name is Tom", 2, true, L"He" },
    { L"Hello, my name is Tom", 3, true, L"H.m" },
    { L"Hello, my name is Tom", 4, true, L"H..m" },
    { L"Hello, my name is Tom", 5, true, L"H...m" },
    { L"Hello, my name is Tom", 6, true, L"He...m" },
    { L"Hello, my name is Tom", 7, true, L"He...om" },
    { L"Hello, my name is Tom", 10, true, L"Hell...Tom" },
    { L"Hello, my name is Tom", 100, false, L"Hello, my name is Tom" }
  };
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    std::wstring output;
    EXPECT_EQ(cases[i].result,
              gfx::ElideString(cases[i].input, cases[i].max_len, &output));
    EXPECT_TRUE(output == cases[i].output);
  }
}
