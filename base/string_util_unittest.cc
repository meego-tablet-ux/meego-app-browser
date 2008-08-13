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

#include <math.h>
#include <stdarg.h>

#include <sstream>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/string_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
}

static const struct trim_case {
  const wchar_t* input;
  const TrimPositions positions;
  const wchar_t* output;
  const TrimPositions return_value;
} trim_cases[] = {
  {L" Google Video ", TRIM_LEADING, L"Google Video ", TRIM_LEADING},
  {L" Google Video ", TRIM_TRAILING, L" Google Video", TRIM_TRAILING},
  {L" Google Video ", TRIM_ALL, L"Google Video", TRIM_ALL},
  {L"Google Video", TRIM_ALL, L"Google Video", TRIM_NONE},
  {L"", TRIM_ALL, L"", TRIM_NONE},
  {L"  ", TRIM_LEADING, L"", TRIM_LEADING},
  {L"  ", TRIM_TRAILING, L"", TRIM_TRAILING},
  {L"  ", TRIM_ALL, L"", TRIM_ALL},
  {L"\t\rTest String\n", TRIM_ALL, L"Test String", TRIM_ALL},
  {L"\x2002Test String\x00A0\x3000", TRIM_ALL, L"Test String", TRIM_ALL},
};

static const struct trim_case_ascii {
  const char* input;
  const TrimPositions positions;
  const char* output;
  const TrimPositions return_value;
} trim_cases_ascii[] = {
  {" Google Video ", TRIM_LEADING, "Google Video ", TRIM_LEADING},
  {" Google Video ", TRIM_TRAILING, " Google Video", TRIM_TRAILING},
  {" Google Video ", TRIM_ALL, "Google Video", TRIM_ALL},
  {"Google Video", TRIM_ALL, "Google Video", TRIM_NONE},
  {"", TRIM_ALL, "", TRIM_NONE},
  {"  ", TRIM_LEADING, "", TRIM_LEADING},
  {"  ", TRIM_TRAILING, "", TRIM_TRAILING},
  {"  ", TRIM_ALL, "", TRIM_ALL},
  {"\t\rTest String\n", TRIM_ALL, "Test String", TRIM_ALL},
  {"\x85Test String\xa0\x20", TRIM_ALL, "Test String", TRIM_ALL},
};

TEST(StringUtilTest, TrimWhitespace) {
  std::wstring output;  // Allow contents to carry over to next testcase
  for (size_t i = 0; i < arraysize(trim_cases); ++i) {
    const trim_case& value = trim_cases[i];
    EXPECT_EQ(value.return_value,
              TrimWhitespace(value.input, value.positions, &output));
    EXPECT_EQ(value.output, output);
  }

  // Test that TrimWhitespace() can take the same string for input and output
  output = L"  This is a test \r\n";
  EXPECT_EQ(TRIM_ALL, TrimWhitespace(output, TRIM_ALL, &output));
  EXPECT_EQ(L"This is a test", output);

  // Once more, but with a string of whitespace
  output = L"  \r\n";
  EXPECT_EQ(TRIM_ALL, TrimWhitespace(output, TRIM_ALL, &output));
  EXPECT_EQ(L"", output);

  std::string output_ascii;
  for (size_t i = 0; i < arraysize(trim_cases_ascii); ++i) {
    const trim_case_ascii& value = trim_cases_ascii[i];
    EXPECT_EQ(value.return_value,
              TrimWhitespace(value.input, value.positions, &output_ascii));
    EXPECT_EQ(value.output, output_ascii);
  }
}

static const struct collapse_case {
  const wchar_t* input;
  const bool trim;
  const wchar_t* output;
} collapse_cases[] = {
  {L" Google Video ", false, L"Google Video"},
  {L"Google Video", false, L"Google Video"},
  {L"", false, L""},
  {L"  ", false, L""},
  {L"\t\rTest String\n", false, L"Test String"},
  {L"\x2002Test String\x00A0\x3000", false, L"Test String"},
  {L"    Test     \n  \t String    ", false, L"Test String"},
  {L"\x2002Test\x1680 \x2028 \tString\x00A0\x3000", false, L"Test String"},
  {L"   Test String", false, L"Test String"},
  {L"Test String    ", false, L"Test String"},
  {L"Test String", false, L"Test String"},
  {L"", true, L""},
  {L"\n", true, L""},
  {L"  \r  ", true, L""},
  {L"\nFoo", true, L"Foo"},
  {L"\r  Foo  ", true, L"Foo"},
  {L" Foo bar ", true, L"Foo bar"},
  {L"  \tFoo  bar  \n", true, L"Foo bar"},
  {L" a \r b\n c \r\n d \t\re \t f \n ", true, L"abcde f"},
};

TEST(StringUtilTest, CollapseWhitespace) {
  for (size_t i = 0; i < arraysize(collapse_cases); ++i) {
    const collapse_case& value = collapse_cases[i];
    EXPECT_EQ(value.output, CollapseWhitespace(value.input, value.trim));
  }
}

static const wchar_t* const kConvertRoundtripCases[] = {
  L"Google Video",
  // "网页 图片 资讯更多 »"
  L"\x7f51\x9875\x0020\x56fe\x7247\x0020\x8d44\x8baf\x66f4\x591a\x0020\x00bb",
  //  "Παγκόσμιος Ιστός"
  L"\x03a0\x03b1\x03b3\x03ba\x03cc\x03c3\x03bc\x03b9"
  L"\x03bf\x03c2\x0020\x0399\x03c3\x03c4\x03cc\x03c2",
  // "Поиск страниц на русском"
  L"\x041f\x043e\x0438\x0441\x043a\x0020\x0441\x0442"
  L"\x0440\x0430\x043d\x0438\x0446\x0020\x043d\x0430"
  L"\x0020\x0440\x0443\x0441\x0441\x043a\x043e\x043c",
  // "전체서비스"
  L"\xc804\xccb4\xc11c\xbe44\xc2a4",

  // Test characters that take more than 16 bits. This will depend on whether
  // wchar_t is 16 or 32 bits.
#if defined(WCHAR_T_IS_UTF16)
  L"\xd800\xdf00",
  // ?????  (Mathematical Alphanumeric Symbols (U+011d40 - U+011d44 : A,B,C,D,E)
  L"\xd807\xdd40\xd807\xdd41\xd807\xdd42\xd807\xdd43\xd807\xdd44",
#elif defined(WCHAR_T_IS_UTF32)
  L"\x10300",
  // ?????  (Mathematical Alphanumeric Symbols (U+011d40 - U+011d44 : A,B,C,D,E)
  L"\x11d40\x11d41\x11d42\x11d43\x11d44",
#endif
};

TEST(StringUtilTest, ConvertUTF8AndWide) {
  // we round-trip all the wide strings through UTF-8 to make sure everything
  // agrees on the conversion. This uses the stream operators to test them
  // simultaneously.
  for (size_t i = 0; i < arraysize(kConvertRoundtripCases); ++i) {
    std::ostringstream utf8;
    utf8 << WideToUTF8(kConvertRoundtripCases[i]);
    std::wostringstream wide;
    wide << UTF8ToWide(utf8.str());

    EXPECT_EQ(kConvertRoundtripCases[i], wide.str());
  }
}

TEST(StringUtilTest, ConvertUTF8AndWideEmptyString) {
  // An empty std::wstring should be converted to an empty std::string,
  // and vice versa.
  std::wstring wempty;
  std::string empty;
  EXPECT_EQ(empty, WideToUTF8(wempty));
  EXPECT_EQ(wempty, UTF8ToWide(empty));
}

TEST(StringUtilTest, ConvertUTF8ToWide) {
  struct UTF8ToWideCase {
    const char* utf8;
    const wchar_t* wide;
    bool success;
  } convert_cases[] = {
    // Regular UTF-8 input.
    {"\xe4\xbd\xa0\xe5\xa5\xbd", L"\x4f60\x597d", true},
    // Invalid Unicode code point.
    {"\xef\xbf\xbfHello", L"Hello", false},
    // Truncated UTF-8 sequence.
    {"\xe4\xa0\xe5\xa5\xbd", L"\x597d", false},
    // Truncated off the end.
    {"\xe5\xa5\xbd\xe4\xa0", L"\x597d", false},
    // Non-shortest-form UTF-8.
    {"\xf0\x84\xbd\xa0\xe5\xa5\xbd", L"\x597d", false},
    // This UTF-8 character decodes to a UTF-16 surrogate, which is illegal.
    {"\xed\xb0\x80", L"", false},
    // Non-BMP character. The result will either be in UTF-16 or UTF-32.
#if defined(WCHAR_T_IS_UTF16)
    {"A\xF0\x90\x8C\x80z", L"A\xd800\xdf00z", true},
#elif defined(WCHAR_T_IS_UTF32)
    {"A\xF0\x90\x8C\x80z", L"A\x10300z", true},
#endif
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(convert_cases); i++) {
    std::wstring converted;
    EXPECT_EQ(convert_cases[i].success,
              UTF8ToWide(convert_cases[i].utf8,
                         strlen(convert_cases[i].utf8),
                         &converted));
    std::wstring expected(convert_cases[i].wide);
    EXPECT_EQ(expected, converted);
  }

  // Manually test an embedded NULL.
  std::wstring converted;
  EXPECT_TRUE(UTF8ToWide("\00Z\t", 3, &converted));
  ASSERT_EQ(static_cast<std::wstring::size_type>(3), converted.length());
  EXPECT_EQ(0, converted[0]);
  EXPECT_EQ('Z', converted[1]);
  EXPECT_EQ('\t', converted[2]);

  // Make sure that conversion replaces, not appends.
  EXPECT_TRUE(UTF8ToWide("B", 1, &converted));
  ASSERT_EQ(static_cast<std::wstring::size_type>(1), converted.length());
  EXPECT_EQ('B', converted[0]);
}

#if defined(WCHAR_T_IS_UTF16)
// This test is only valid when wchar_t == UTF-16.
TEST(StringUtilTest, ConvertUTF16ToUTF8) {
  struct UTF16ToUTF8Case {
    const wchar_t* utf16;
    const char* utf8;
    bool success;
  } convert_cases[] = {
    // Regular UTF-16 input.
    {L"\x4f60\x597d", "\xe4\xbd\xa0\xe5\xa5\xbd", true},
    // Test a non-BMP character.
    {L"\xd800\xdf00", "\xF0\x90\x8C\x80", true},
    // Invalid Unicode code point.
    {L"\xffffHello", "Hello", false},
    // The first character is a truncated UTF-16 character.
    {L"\xd800\x597d", "\xe5\xa5\xbd", false},
    // Truncated at the end.
    {L"\x597d\xd800", "\xe5\xa5\xbd", false},
  };

  for (int i = 0; i < arraysize(convert_cases); i++) {
    std::string converted;
    EXPECT_EQ(convert_cases[i].success,
              WideToUTF8(convert_cases[i].utf16,
                         wcslen(convert_cases[i].utf16),
                         &converted));
    std::string expected(convert_cases[i].utf8);
    EXPECT_EQ(expected, converted);
  }
}

#elif defined(WCHAR_T_IS_UTF32)
// This test is only valid when wchar_t == UTF-32.
TEST(StringUtilTest, ConvertUTF32ToUTF8) {
  struct UTF8ToWideCase {
    const wchar_t* utf32;
    const char* utf8;
    bool success;
  } convert_cases[] = {
    // Regular 16-bit input.
    {L"\x4f60\x597d", "\xe4\xbd\xa0\xe5\xa5\xbd", true},
    // Test a non-BMP character.
    {L"A\x10300z", "A\xF0\x90\x8C\x80z", true},
    // Invalid Unicode code points.
    {L"\xffffHello", "Hello", false},
    {L"\xfffffffHello", "Hello", false},
    // The first character is a truncated UTF-16 character.
    {L"\xd800\x597d", "\xe5\xa5\xbd", false},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(convert_cases); i++) {
    std::string converted;
    EXPECT_EQ(convert_cases[i].success,
              WideToUTF8(convert_cases[i].utf32,
                         wcslen(convert_cases[i].utf32),
                         &converted));
    std::string expected(convert_cases[i].utf8);
    EXPECT_EQ(expected, converted);
  }
}
#endif  // defined(WCHAR_T_IS_UTF32)

TEST(StringUtilTest, ConvertMultiString) {
  static wchar_t wmulti[] = {
    L'f', L'o', L'o', L'\0',
    L'b', L'a', L'r', L'\0',
    L'b', L'a', L'z', L'\0',
    L'\0'
  };
  static char multi[] = {
    'f', 'o', 'o', '\0',
    'b', 'a', 'r', '\0',
    'b', 'a', 'z', '\0',
    '\0'
  };
  std::wstring wmultistring;
  memcpy(WriteInto(&wmultistring, arraysize(wmulti)), wmulti, sizeof(wmulti));
  EXPECT_EQ(arraysize(wmulti) - 1, wmultistring.length());
  std::string expected;
  memcpy(WriteInto(&expected, arraysize(multi)), multi, sizeof(multi));
  EXPECT_EQ(arraysize(multi) - 1, expected.length());
  const std::string& converted = WideToUTF8(wmultistring);
  EXPECT_EQ(arraysize(multi) - 1, converted.length());
  EXPECT_EQ(expected, converted);
}

TEST(StringUtilTest, ConvertCodepageUTF8) {
  // Make sure WideToCodepage works like WideToUTF8.
  for (size_t i = 0; i < arraysize(kConvertRoundtripCases); ++i) {
    std::string expected(WideToUTF8(kConvertRoundtripCases[i]));
    std::string utf8;
    EXPECT_TRUE(WideToCodepage(kConvertRoundtripCases[i], kCodepageUTF8,
                               OnStringUtilConversionError::SKIP, &utf8));
    EXPECT_EQ(expected, utf8);
  }
}

TEST(StringUtilTest, ConvertBetweenCodepageAndWide) {
  static const struct {
    const char* codepage_name;
    const char* encoded;
    OnStringUtilConversionError::Type on_error;
    bool success;
    const wchar_t* wide;
  } kConvertCodepageCases[] = {
    // Test a case where the input can no be decoded, using both SKIP and FAIL
    // error handling rules. "A7 41" is valid, but "A6" isn't.
    {"big5",
     "\xA7\x41\xA6",
     OnStringUtilConversionError::FAIL,
     false,
     L""},
    {"big5",
     "\xA7\x41\xA6",
     OnStringUtilConversionError::SKIP,
     true,
     L"\x4F60"},
    // Arabic (ISO-8859)
    {"iso-8859-6",
     "\xC7\xEE\xE4\xD3\xF1\xEE\xE4\xC7\xE5\xEF" " "
     "\xD9\xEE\xE4\xEE\xEA\xF2\xE3\xEF\xE5\xF2",
     OnStringUtilConversionError::FAIL,
     true,
     L"\x0627\x064E\x0644\x0633\x0651\x064E\x0644\x0627\x0645\x064F" L" "
     L"\x0639\x064E\x0644\x064E\x064A\x0652\x0643\x064F\x0645\x0652"},
    // Chinese Simplified (GB2312)
    {"gb2312",
     "\xC4\xE3\xBA\xC3",
     OnStringUtilConversionError::FAIL,
     true,
     L"\x4F60\x597D"},
    // Chinese Traditional (BIG5)
    {"big5",
     "\xA7\x41\xA6\x6E",
     OnStringUtilConversionError::FAIL,
     true,
     L"\x4F60\x597D"},
    // Greek (ISO-8859)
    {"iso-8859-7",
     "\xE3\xE5\xE9\xDC" " " "\xF3\xEF\xF5",
     OnStringUtilConversionError::FAIL,
     true,
     L"\x03B3\x03B5\x03B9\x03AC" L" " L"\x03C3\x03BF\x03C5"},
    // Hebrew (Windows)
    {"windows-1255", /* to be replaced with "iso-8859-8-I"? */
     "\xF9\xD1\xC8\xEC\xE5\xC9\xED",
     OnStringUtilConversionError::FAIL,
     true,
     L"\x05E9\x05C1\x05B8\x05DC\x05D5\x05B9\x05DD"},
    // Hindi Devanagari (ISCII)
    {"iscii-dev",
     "\xEF\x42" "\xC6\xCC\xD7\xE8\xB3\xDA\xCF",
     OnStringUtilConversionError::FAIL,
     true,
     L"\x0928\x092E\x0938\x094D\x0915\x093E\x0930"},
    // Korean (EUC)
    {"euc-kr",
     "\xBE\xC8\xB3\xE7\xC7\xCF\xBC\xBC\xBF\xE4",
     OnStringUtilConversionError::FAIL,
     true,
     L"\xC548\xB155\xD558\xC138\xC694"},
    // Japanese (EUC)
    {"euc-jp",
     "\xA4\xB3\xA4\xF3\xA4\xCB\xA4\xC1\xA4\xCF",
     OnStringUtilConversionError::FAIL,
     true,
     L"\x3053\x3093\x306B\x3061\x306F"},
    // Japanese (ISO-2022)
    {"iso-2022-jp",
     "\x1B\x24\x42" "\x24\x33\x24\x73\x24\x4B\x24\x41\x24\x4F" "\x1B\x28\x42",
     OnStringUtilConversionError::FAIL,
     true,
     L"\x3053\x3093\x306B\x3061\x306F"},
    // Japanese (Shift-JIS)
    {"sjis",
     "\x82\xB1\x82\xF1\x82\xC9\x82\xBF\x82\xCD",
     OnStringUtilConversionError::FAIL,
     true,
     L"\x3053\x3093\x306B\x3061\x306F"},
    // Russian (KOI8)
    {"koi8-r",
     "\xDA\xC4\xD2\xC1\xD7\xD3\xD4\xD7\xD5\xCA\xD4\xC5",
     OnStringUtilConversionError::FAIL,
     true,
     L"\x0437\x0434\x0440\x0430\x0432\x0441\x0442\x0432"
     L"\x0443\x0439\x0442\x0435"},
    // Thai (ISO-8859)
    {"windows-874", /* to be replaced with "iso-8859-11". */
     "\xCA\xC7\xD1\xCA\xB4\xD5" "\xA4\xC3\xD1\xBA",
     OnStringUtilConversionError::FAIL,
     true,
     L"\x0E2A\x0E27\x0E31\x0E2A\x0E14\x0E35"
     L"\x0E04\x0E23\x0e31\x0E1A"},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(kConvertCodepageCases); ++i) {
    std::wstring wide;
    bool success = CodepageToWide(kConvertCodepageCases[i].encoded,
                                  kConvertCodepageCases[i].codepage_name,
                                  kConvertCodepageCases[i].on_error,
                                  &wide);
    EXPECT_EQ(kConvertCodepageCases[i].success, success);
    EXPECT_EQ(kConvertCodepageCases[i].wide, wide);

    // When decoding was successful and nothing was skipped, we also check the
    // reverse conversion.
    if (success &&
        kConvertCodepageCases[i].on_error ==
            OnStringUtilConversionError::FAIL) {
      std::string encoded;
      success = WideToCodepage(wide, kConvertCodepageCases[i].codepage_name,
                               kConvertCodepageCases[i].on_error, &encoded);
      EXPECT_EQ(kConvertCodepageCases[i].success, success);
      EXPECT_EQ(kConvertCodepageCases[i].encoded, encoded);
    }
  }

  // The above cases handled codepage->wide errors, but not wide->codepage.
  // Test that here.
  std::string encoded("Temp data");  // Make sure the string gets cleared.

  // First test going to an encoding that can not represent that character.
  EXPECT_FALSE(WideToCodepage(L"Chinese\xff27", "iso-8859-1",
                              OnStringUtilConversionError::FAIL, &encoded));
  EXPECT_TRUE(encoded.empty());
  EXPECT_TRUE(WideToCodepage(L"Chinese\xff27", "iso-8859-1",
                             OnStringUtilConversionError::SKIP, &encoded));
  EXPECT_STREQ("Chinese", encoded.c_str());

#if defined(WCHAR_T_IS_UTF16)
  // When we're in UTF-16 mode, test an invalid UTF-16 character in the input.
  EXPECT_FALSE(WideToCodepage(L"a\xd800z", "iso-8859-1",
                              OnStringUtilConversionError::FAIL, &encoded));
  EXPECT_TRUE(encoded.empty());
  EXPECT_TRUE(WideToCodepage(L"a\xd800z", "iso-8859-1",
                             OnStringUtilConversionError::SKIP, &encoded));
  EXPECT_STREQ("az", encoded.c_str());
#endif  // WCHAR_T_IS_UTF16

  // Invalid characters should fail.
  EXPECT_TRUE(WideToCodepage(L"a\xffffz", "iso-8859-1",
                             OnStringUtilConversionError::SKIP, &encoded));
  EXPECT_STREQ("az", encoded.c_str());

  // Invalid codepages should fail.
  EXPECT_FALSE(WideToCodepage(L"Hello, world", "awesome-8571-2",
                              OnStringUtilConversionError::SKIP, &encoded));
}

TEST(StringUtilTest, ConvertASCII) {
  static const char* char_cases[] = {
    "Google Video",
    "Hello, world\n",
    "0123ABCDwxyz \a\b\t\r\n!+,.~"
  };

  static const wchar_t* const wchar_cases[] = {
    L"Google Video",
    L"Hello, world\n",
    L"0123ABCDwxyz \a\b\t\r\n!+,.~"
  };

  for (size_t i = 0; i < arraysize(char_cases); ++i) {
    EXPECT_TRUE(IsStringASCII(char_cases[i]));
    std::wstring wide = ASCIIToWide(char_cases[i]);
    EXPECT_EQ(wchar_cases[i], wide);

    EXPECT_TRUE(IsStringASCII(wchar_cases[i]));
    std::string ascii = WideToASCII(wchar_cases[i]);
    EXPECT_EQ(char_cases[i], ascii);
  }

  EXPECT_FALSE(IsStringASCII("Google \x80Video"));
  EXPECT_FALSE(IsStringASCII(L"Google \x80Video"));

  // Convert empty strings.
  std::wstring wempty;
  std::string empty;
  EXPECT_EQ(empty, WideToASCII(wempty));
  EXPECT_EQ(wempty, ASCIIToWide(empty));

  // Convert strings with an embedded NUL character.
  const char chars_with_nul[] = "test\0string";
  const int length_with_nul = arraysize(chars_with_nul) - 1;
  std::string string_with_nul(chars_with_nul, length_with_nul);
  std::wstring wide_with_nul = ASCIIToWide(string_with_nul);
  EXPECT_EQ(static_cast<std::wstring::size_type>(length_with_nul),
            wide_with_nul.length());
  std::string narrow_with_nul = WideToASCII(wide_with_nul);
  EXPECT_EQ(static_cast<std::string::size_type>(length_with_nul),
            narrow_with_nul.length());
  EXPECT_EQ(0, string_with_nul.compare(narrow_with_nul));
}

static const struct {
  const wchar_t* src_w;
  const char*    src_a;
  const char*    dst;
} lowercase_cases[] = {
  {L"FoO", "FoO", "foo"},
  {L"foo", "foo", "foo"},
  {L"FOO", "FOO", "foo"},
};

TEST(StringUtilTest, LowerCaseEqualsASCII) {
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(lowercase_cases); ++i) {
    EXPECT_TRUE(LowerCaseEqualsASCII(lowercase_cases[i].src_w,
                                     lowercase_cases[i].dst));
    EXPECT_TRUE(LowerCaseEqualsASCII(lowercase_cases[i].src_a,
                                     lowercase_cases[i].dst));
  }
}

TEST(StringUtilTest, GetByteDisplayUnits) {
  static const struct {
    int64 bytes;
    DataUnits expected;
  } cases[] = {
    {0, DATA_UNITS_BYTE},
    {512, DATA_UNITS_BYTE},
    {10*1024, DATA_UNITS_KILOBYTE},
    {10*1024*1024, DATA_UNITS_MEGABYTE},
    {10LL*1024*1024*1024, DATA_UNITS_GIGABYTE},
    {~(1LL<<63), DATA_UNITS_GIGABYTE},
#ifdef NDEBUG
    {-1, DATA_UNITS_BYTE},
#endif
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i)
    EXPECT_EQ(cases[i].expected, GetByteDisplayUnits(cases[i].bytes));
}

TEST(StringUtilTest, FormatBytes) {
  static const struct {
    int64 bytes;
    DataUnits units;
    const wchar_t* expected;
    const wchar_t* expected_with_units;
  } cases[] = {
    {0, DATA_UNITS_BYTE, L"0", L"0 B"},
    {512, DATA_UNITS_BYTE, L"512", L"512 B"},
    {512, DATA_UNITS_KILOBYTE, L"0.5", L"0.5 kB"},
    {1024*1024, DATA_UNITS_KILOBYTE, L"1024", L"1024 kB"},
    {1024*1024, DATA_UNITS_MEGABYTE, L"1", L"1 MB"},
    {1024*1024*1024, DATA_UNITS_GIGABYTE, L"1", L"1 GB"},
    {10LL*1024*1024*1024, DATA_UNITS_GIGABYTE, L"10", L"10 GB"},
    {~(1LL<<63), DATA_UNITS_GIGABYTE, L"8589934592", L"8589934592 GB"},
    // Make sure the first digit of the fractional part works.
    {1024*1024 + 103, DATA_UNITS_KILOBYTE, L"1024.1", L"1024.1 kB"},
    {1024*1024 + 205 * 1024, DATA_UNITS_MEGABYTE, L"1.2", L"1.2 MB"},
    {1024*1024*1024 + (927 * 1024*1024), DATA_UNITS_GIGABYTE,
     L"1.9", L"1.9 GB"},
    {10LL*1024*1024*1024, DATA_UNITS_GIGABYTE, L"10", L"10 GB"},
#ifdef NDEBUG
    {-1, DATA_UNITS_BYTE, L"", L""},
#endif
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    EXPECT_EQ(cases[i].expected,
              FormatBytes(cases[i].bytes, cases[i].units, false));
    EXPECT_EQ(cases[i].expected_with_units,
              FormatBytes(cases[i].bytes, cases[i].units, true));
  }
}

TEST(StringUtilTest, ReplaceSubstringsAfterOffset) {
  static const struct {
    wchar_t* str;
    std::wstring::size_type start_offset;
    wchar_t* find_this;
    wchar_t* replace_with;
    wchar_t* expected;
  } cases[] = {
    {L"aaa", 0, L"a", L"b", L"bbb"},
    {L"abb", 0, L"ab", L"a", L"ab"},
    {L"Removing some substrings inging", 0, L"ing", L"", L"Remov some substrs "},
    {L"Not found", 0, L"x", L"0", L"Not found"},
    {L"Not found again", 5, L"x", L"0", L"Not found again"},
    {L" Making it much longer ", 0, L" ", L"Four score and seven years ago",
     L"Four score and seven years agoMakingFour score and seven years agoit"
     L"Four score and seven years agomuchFour score and seven years agolonger"
     L"Four score and seven years ago"},
    {L"Invalid offset", 9999, L"t", L"foobar", L"Invalid offset"},
    {L"Replace me only me once", 9, L"me ", L"", L"Replace me only once"},
    {L"abababab", 2, L"ab", L"c", L"abccc"},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); i++) {
    std::wstring str(cases[i].str);
    ReplaceSubstringsAfterOffset(&str, cases[i].start_offset,
                                 cases[i].find_this, cases[i].replace_with);
    EXPECT_EQ(cases[i].expected, str);
  }
}

TEST(StringUtilTest, IntToString) {
  static const struct {
    int input;
    std::string output;
  } cases[] = {
    {0, "0"},
    {42, "42"},
    {-42, "-42"},
    {INT_MAX, "2147483647"},
    {INT_MIN, "-2147483648"},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i)
    EXPECT_EQ(cases[i].output, IntToString(cases[i].input));
}

TEST(StringUtilTest, Uint64ToString) {
  static const struct {
    uint64 input;
    std::string output;
  } cases[] = {
    {0, "0"},
    {42, "42"},
    {INT_MAX, "2147483647"},
    {kuint64max, "18446744073709551615"},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i)
    EXPECT_EQ(cases[i].output, Uint64ToString(cases[i].input));
}

TEST(StringUtilTest, StringToInt) {
  static const struct {
    std::string input;
    int output;
    bool success;
  } cases[] = {
    {"0", 0, true},
    {"42", 42, true},
    {"-2147483648", INT_MIN, true},
    {"2147483647", INT_MAX, true},
    {"", 0, false},
    {" 42", 42, false},
    {"42 ", 42, false},
    {"\t\n\v\f\r 42", 42, false},
    {"blah42", 0, false},
    {"42blah", 42, false},
    {"blah42blah", 0, false},
    {"-273.15", -273, false},
    {"+98.6", 98, false},
    {"--123", 0, false},
    {"++123", 0, false},
    {"-+123", 0, false},
    {"+-123", 0, false},
    {"-", 0, false},
    {"-2147483649", INT_MIN, false},
    {"-99999999999", INT_MIN, false},
    {"2147483648", INT_MAX, false},
    {"99999999999", INT_MAX, false},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    EXPECT_EQ(cases[i].output, StringToInt(cases[i].input));
    int output;
    EXPECT_EQ(cases[i].success, StringToInt(cases[i].input, &output));
    EXPECT_EQ(cases[i].output, output);

    std::wstring wide_input = ASCIIToWide(cases[i].input);
    EXPECT_EQ(cases[i].output, StringToInt(wide_input));
    EXPECT_EQ(cases[i].success, StringToInt(wide_input, &output));
    EXPECT_EQ(cases[i].output, output);
  }

  // One additional test to verify that conversion of numbers in strings with
  // embedded NUL characters.  The NUL and extra data after it should be
  // interpreted as junk after the number.
  const char input[] = "6\06";
  std::string input_string(input, arraysize(input) - 1);
  int output;
  EXPECT_FALSE(StringToInt(input_string, &output));
  EXPECT_EQ(6, output);

  std::wstring wide_input = ASCIIToWide(input_string);
  EXPECT_FALSE(StringToInt(wide_input, &output));
  EXPECT_EQ(6, output);
}

TEST(StringUtilTest, StringToInt64) {
  static const struct {
    std::string input;
    int64 output;
    bool success;
  } cases[] = {
    {"0", 0, true},
    {"42", 42, true},
    {"-2147483648", INT_MIN, true},
    {"2147483647", INT_MAX, true},
    {"-2147483649", GG_INT64_C(-2147483649), true},
    {"-99999999999", GG_INT64_C(-99999999999), true},
    {"2147483648", GG_INT64_C(2147483648), true},
    {"99999999999", GG_INT64_C(99999999999), true},
    {"9223372036854775807", kint64max, true},
    {"-9223372036854775808", kint64min, true},
    {"09", 9, true},
    {"-09", -9, true},
    {"", 0, false},
    {" 42", 42, false},
    {"42 ", 42, false},
    {"\t\n\v\f\r 42", 42, false},
    {"blah42", 0, false},
    {"42blah", 42, false},
    {"blah42blah", 0, false},
    {"-273.15", -273, false},
    {"+98.6", 98, false},
    {"--123", 0, false},
    {"++123", 0, false},
    {"-+123", 0, false},
    {"+-123", 0, false},
    {"-", 0, false},
    {"-9223372036854775809", kint64min, false},
    {"-99999999999999999999", kint64min, false},
    {"9223372036854775808", kint64max, false},
    {"99999999999999999999", kint64max, false},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    EXPECT_EQ(cases[i].output, StringToInt64(cases[i].input));
    int64 output;
    EXPECT_EQ(cases[i].success, StringToInt64(cases[i].input, &output));
    EXPECT_EQ(cases[i].output, output);

    std::wstring wide_input = ASCIIToWide(cases[i].input);
    EXPECT_EQ(cases[i].output, StringToInt64(wide_input));
    EXPECT_EQ(cases[i].success, StringToInt64(wide_input, &output));
    EXPECT_EQ(cases[i].output, output);
  }

  // One additional test to verify that conversion of numbers in strings with
  // embedded NUL characters.  The NUL and extra data after it should be
  // interpreted as junk after the number.
  const char input[] = "6\06";
  std::string input_string(input, arraysize(input) - 1);
  int64 output;
  EXPECT_FALSE(StringToInt64(input_string, &output));
  EXPECT_EQ(6, output);

  std::wstring wide_input = ASCIIToWide(input_string);
  EXPECT_FALSE(StringToInt64(wide_input, &output));
  EXPECT_EQ(6, output);
}

TEST(StringUtilTest, HexStringToInt) {
  static const struct {
    std::string input;
    int output;
    bool success;
  } cases[] = {
    {"0", 0, true},
    {"42", 66, true},
    {"-42", -66, true},
    {"+42", 66, true},
    {"7fffffff", INT_MAX, true},
    {"80000000", INT_MIN, true},
    {"ffffffff", -1, true},
    {"DeadBeef", 0xdeadbeef, true},
    {"0x42", 66, true},
    {"-0x42", -66, true},
    {"+0x42", 66, true},
    {"0x7fffffff", INT_MAX, true},
    {"0x80000000", INT_MIN, true},
    {"0xffffffff", -1, true},
    {"0XDeadBeef", 0xdeadbeef, true},
    {"0x0f", 15, true},
    {"0f", 15, true},
    {" 45", 0x45, false},
    {"\t\n\v\f\r 0x45", 0x45, false},
    {" 45", 0x45, false},
    {"45 ", 0x45, false},
    {"efgh", 0xef, false},
    {"0xefgh", 0xef, false},
    {"hgfe", 0, false},
    {"100000000", -1, false},  // don't care about |output|, just |success|
    {"-", 0, false},
    {"", 0, false},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    EXPECT_EQ(cases[i].output, HexStringToInt(cases[i].input));
    int output;
    EXPECT_EQ(cases[i].success, HexStringToInt(cases[i].input, &output));
    EXPECT_EQ(cases[i].output, output);

    std::wstring wide_input = ASCIIToWide(cases[i].input);
    EXPECT_EQ(cases[i].output, HexStringToInt(wide_input));
    EXPECT_EQ(cases[i].success, HexStringToInt(wide_input, &output));
    EXPECT_EQ(cases[i].output, output);
  }
  // One additional test to verify that conversion of numbers in strings with
  // embedded NUL characters.  The NUL and extra data after it should be
  // interpreted as junk after the number.
  const char input[] = "0xc0ffee\09";
  std::string input_string(input, arraysize(input) - 1);
  int output;
  EXPECT_FALSE(HexStringToInt(input_string, &output));
  EXPECT_EQ(0xc0ffee, output);

  std::wstring wide_input = ASCIIToWide(input_string);
  EXPECT_FALSE(HexStringToInt(wide_input, &output));
  EXPECT_EQ(0xc0ffee, output);
}

TEST(StringUtilTest, StringToDouble) {
  static const struct {
    std::string input;
    double output;
    bool success;
  } cases[] = {
    {"0", 0.0, true},
    {"42", 42.0, true},
    {"-42", -42.0, true},
    {"123.45", 123.45, true},
    {"-123.45", -123.45, true},
    {"+123.45", 123.45, true},
    {"2.99792458e8", 299792458.0, true},
    {"149597870.691E+3", 149597870691.0, true},
    {"6.", 6.0, true},
    {"9e99999999999999999999", HUGE_VAL, false},
    {"-9e99999999999999999999", -HUGE_VAL, false},
    {"1e-2", 0.01, true},
    {" 1e-2", 0.01, false},
    {"1e-2 ", 0.01, false},
    {"-1E-7", -0.0000001, true},
    {"01e02", 100, true},
    {"2.3e15", 2.3e15, true},
    {"\t\n\v\f\r -123.45e2", -12345.0, false},
    {"+123 e4", 123.0, false},
    {"123e ", 123.0, false},
    {"123e", 123.0, false},
    {" 2.99", 2.99, false},
    {"1e3.4", 1000.0, false},
    {"nothing", 0.0, false},
    {"-", 0.0, false},
    {"+", 0.0, false},
    {"", 0.0, false},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(cases); ++i) {
    EXPECT_DOUBLE_EQ(cases[i].output, StringToDouble(cases[i].input));
    double output;
    EXPECT_EQ(cases[i].success, StringToDouble(cases[i].input, &output));
    EXPECT_DOUBLE_EQ(cases[i].output, output);

    std::wstring wide_input = ASCIIToWide(cases[i].input);
    EXPECT_DOUBLE_EQ(cases[i].output, StringToDouble(wide_input));
    EXPECT_EQ(cases[i].success, StringToDouble(wide_input, &output));
    EXPECT_DOUBLE_EQ(cases[i].output, output);
  }

  // One additional test to verify that conversion of numbers in strings with
  // embedded NUL characters.  The NUL and extra data after it should be
  // interpreted as junk after the number.
  const char input[] = "3.14\0159";
  std::string input_string(input, arraysize(input) - 1);
  double output;
  EXPECT_FALSE(StringToDouble(input_string, &output));
  EXPECT_DOUBLE_EQ(3.14, output);

  std::wstring wide_input = ASCIIToWide(input_string);
  EXPECT_FALSE(StringToDouble(wide_input, &output));
  EXPECT_DOUBLE_EQ(3.14, output);
}

// This checks where we can use the assignment operator for a va_list. We need
// a way to do this since Visual C doesn't support va_copy, but assignment on
// va_list is not guaranteed to be a copy. See StringAppendVT which uses this
// capability.
static void VariableArgsFunc(const char* format, ...) {
  va_list org;
  va_start(org, format);

  va_list dup;
  base::va_copy(dup, org);
  int i1 = va_arg(org, int);
  int j1 = va_arg(org, int);
  char* s1 = va_arg(org, char*);
  double d1 = va_arg(org, double);
  va_end(org);

  int i2 = va_arg(dup, int);
  int j2 = va_arg(dup, int);
  char* s2 = va_arg(dup, char*);
  double d2 = va_arg(dup, double);

  EXPECT_EQ(i1, i2);
  EXPECT_EQ(j1, j2);
  EXPECT_STREQ(s1, s2);
  EXPECT_EQ(d1, d2);

  va_end(dup);
}

TEST(StringUtilTest, VAList) {
  VariableArgsFunc("%d %d %s %lf", 45, 92, "This is interesting", 9.21);
}

TEST(StringUtilTest, StringPrintfEmptyFormat) {
  const char* empty = "";
  EXPECT_EQ("", StringPrintf(empty));
  EXPECT_EQ("", StringPrintf("%s", ""));
}

TEST(StringUtilTest, StringPrintfMisc) {
  EXPECT_EQ("123hello w", StringPrintf("%3d%2s %1c", 123, "hello", 'w'));
  EXPECT_EQ(L"123hello w", StringPrintf(L"%3d%2ls %1c", 123, L"hello", 'w'));
}

TEST(StringUtilTest, StringAppendfStringEmptyParam) {
  std::string value("Hello");
  StringAppendF(&value, "");
  EXPECT_EQ("Hello", value);

  std::wstring valuew(L"Hello");
  StringAppendF(&valuew, L"");
  EXPECT_EQ(L"Hello", valuew);
}

TEST(StringUtilTest, StringAppendfEmptyString) {
  std::string value("Hello");
  StringAppendF(&value, "%s", "");
  EXPECT_EQ("Hello", value);

  std::wstring valuew(L"Hello");
  StringAppendF(&valuew, L"%s", L"");
  EXPECT_EQ(L"Hello", valuew);
}

TEST(StringUtilTest, StringAppendfString) {
  std::string value("Hello");
  StringAppendF(&value, " %s", "World");
  EXPECT_EQ("Hello World", value);

  std::wstring valuew(L"Hello");
  StringAppendF(&valuew, L" %ls", L"World");
  EXPECT_EQ(L"Hello World", valuew);
}

TEST(StringUtilTest, StringAppendfInt) {
  std::string value("Hello");
  StringAppendF(&value, " %d", 123);
  EXPECT_EQ("Hello 123", value);

  std::wstring valuew(L"Hello");
  StringAppendF(&valuew, L" %d", 123);
  EXPECT_EQ(L"Hello 123", valuew);
}

// Make sure that lengths exactly around the initial buffer size are handled
// correctly.
TEST(StringUtilTest, StringPrintfBounds) {
  const int src_len = 1026;
  char src[src_len];
  for (size_t i = 0; i < arraysize(src); i++)
    src[i] = 'A';

  wchar_t srcw[src_len];
  for (size_t i = 0; i < arraysize(srcw); i++)
    srcw[i] = 'A';

  for (int i = 1; i < 3; i++) {
    src[src_len - i] = 0;
    std::string out;
    SStringPrintf(&out, "%s", src);
    EXPECT_STREQ(src, out.c_str());

    srcw[src_len - i] = 0;
    std::wstring outw;
    SStringPrintf(&outw, L"%ls", srcw);
    EXPECT_STREQ(srcw, outw.c_str());
  }
}

// Test very large sprintfs that will cause the buffer to grow.
TEST(StringUtilTest, Grow) {
  char src[1026];
  for (size_t i = 0; i < arraysize(src); i++)
    src[i] = 'A';
  src[1025] = 0;

  char* fmt = "%sB%sB%sB%sB%sB%sB%s";

  std::string out;
  SStringPrintf(&out, fmt, src, src, src, src, src, src, src);

  char* ref = new char[320000];
#if defined(OS_WIN)
  sprintf_s(ref, 320000, fmt, src, src, src, src, src, src, src);
#elif defined(OS_POSIX)
  snprintf(ref, 320000, fmt, src, src, src, src, src, src, src);
#endif

  EXPECT_STREQ(ref, out.c_str());
  delete ref;
}

// Test the boundary condition for the size of the string_util's
// internal buffer.
TEST(StringUtilTest, GrowBoundary) {
  const int string_util_buf_len = 1024;
  // Our buffer should be one larger than the size of StringAppendVT's stack
  // buffer.
  const int buf_len = string_util_buf_len + 1;
  char src[buf_len + 1];  // Need extra one for NULL-terminator.
  for (int i = 0; i < buf_len; ++i)
    src[i] = 'a';
  src[buf_len] = 0;

  std::string out;
  SStringPrintf(&out, "%s", src);

  EXPECT_STREQ(src, out.c_str());
}

// sprintf in Visual Studio fails when given U+FFFF. This tests that the
// failure case is gracefuly handled.
TEST(StringUtilTest, Invalid) {
  wchar_t invalid[2];
  invalid[0] = 0xffff;
  invalid[1] = 0;

  std::wstring out;
  SStringPrintf(&out, L"%ls", invalid);
  EXPECT_STREQ(L"", out.c_str());
}

// Test for SplitString
TEST(StringUtilTest, SplitString) {
  std::vector<std::wstring> r;

  SplitString(L"a,b,c", L',', &r);
  EXPECT_EQ(static_cast<std::vector<std::wstring>::size_type>(3), r.size());
  EXPECT_EQ(r[0], L"a");
  EXPECT_EQ(r[1], L"b");
  EXPECT_EQ(r[2], L"c");
  r.clear();

  SplitString(L"a, b, c", L',', &r);
  EXPECT_EQ(static_cast<std::vector<std::wstring>::size_type>(3), r.size());
  EXPECT_EQ(r[0], L"a");
  EXPECT_EQ(r[1], L"b");
  EXPECT_EQ(r[2], L"c");
  r.clear();

  SplitString(L"a,,c", L',', &r);
  EXPECT_EQ(static_cast<std::vector<std::wstring>::size_type>(3), r.size());
  EXPECT_EQ(r[0], L"a");
  EXPECT_EQ(r[1], L"");
  EXPECT_EQ(r[2], L"c");
  r.clear();

  SplitString(L"", L'*', &r);
  EXPECT_EQ(static_cast<std::vector<std::wstring>::size_type>(1), r.size());
  EXPECT_EQ(r[0], L"");
  r.clear();

  SplitString(L"foo", L'*', &r);
  EXPECT_EQ(static_cast<std::vector<std::wstring>::size_type>(1), r.size());
  EXPECT_EQ(r[0], L"foo");
  r.clear();

  SplitString(L"foo ,", L',', &r);
  EXPECT_EQ(static_cast<std::vector<std::wstring>::size_type>(2), r.size());
  EXPECT_EQ(r[0], L"foo");
  EXPECT_EQ(r[1], L"");
  r.clear();

  SplitString(L",", L',', &r);
  EXPECT_EQ(static_cast<std::vector<std::wstring>::size_type>(2), r.size());
  EXPECT_EQ(r[0], L"");
  EXPECT_EQ(r[1], L"");
  r.clear();

  SplitString(L"\t\ta\t", L'\t', &r);
  EXPECT_EQ(static_cast<std::vector<std::wstring>::size_type>(4), r.size());
  EXPECT_EQ(r[0], L"");
  EXPECT_EQ(r[1], L"");
  EXPECT_EQ(r[2], L"a");
  EXPECT_EQ(r[3], L"");
  r.clear();

  SplitStringDontTrim(L"\t\ta\t", L'\t', &r);
  EXPECT_EQ(static_cast<std::vector<std::wstring>::size_type>(4), r.size());
  EXPECT_EQ(r[0], L"");
  EXPECT_EQ(r[1], L"");
  EXPECT_EQ(r[2], L"a");
  EXPECT_EQ(r[3], L"");
  r.clear();

  SplitString(L"\ta\t\nb\tcc", L'\n', &r);
  EXPECT_EQ(static_cast<std::vector<std::wstring>::size_type>(2), r.size());
  EXPECT_EQ(r[0], L"a");
  EXPECT_EQ(r[1], L"b\tcc");
  r.clear();

  SplitStringDontTrim(L"\ta\t\nb\tcc", L'\n', &r);
  EXPECT_EQ(static_cast<std::vector<std::wstring>::size_type>(2), r.size());
  EXPECT_EQ(r[0], L"\ta\t");
  EXPECT_EQ(r[1], L"b\tcc");
  r.clear();
}

TEST(StringUtilTest, StartsWith) {
  EXPECT_EQ(true, StartsWithASCII("javascript:url", "javascript", true));
  EXPECT_EQ(true, StartsWithASCII("javascript:url", "javascript", false));
  EXPECT_EQ(true, StartsWithASCII("JavaScript:url", "javascript", false));
  EXPECT_EQ(false, StartsWithASCII("java", "javascript", true));
  EXPECT_EQ(false, StartsWithASCII("java", "javascript", false));
}

TEST(StringUtilTest, GetStringFWithOffsets) {
  std::vector<size_t> offsets;

  ReplaceStringPlaceholders(L"Hello, $1. Your number is $2.", L"1", L"2",
                            &offsets);
  EXPECT_EQ(static_cast<std::vector<size_t>::size_type>(2), offsets.size());
  EXPECT_EQ(static_cast<size_t>(7), offsets[0]);
  EXPECT_EQ(static_cast<size_t>(25), offsets[1]);
  offsets.clear();

  ReplaceStringPlaceholders(L"Hello, $2. Your number is $1.", L"1", L"2",
                            &offsets);
  EXPECT_EQ(static_cast<std::vector<size_t>::size_type>(2), offsets.size());
  EXPECT_EQ(static_cast<size_t>(25), offsets[0]);
  EXPECT_EQ(static_cast<size_t>(7), offsets[1]);
  offsets.clear();
}

TEST(StringUtilTest, SplitStringAlongWhitespace) {
  struct TestData {
    const std::wstring input;
    const int expected_result_count;
    const std::wstring output1;
    const std::wstring output2;
  } data[] = {
    { L"a",       1, L"a",  L""   },
    { L" ",       0, L"",   L""   },
    { L" a",      1, L"a",  L""   },
    { L" ab ",    1, L"ab", L""   },
    { L" ab c",   2, L"ab", L"c"  },
    { L" ab c ",  2, L"ab", L"c"  },
    { L" ab cd",  2, L"ab", L"cd" },
    { L" ab cd ", 2, L"ab", L"cd" },
    { L" \ta\t",  1, L"a",  L""   },
    { L" b\ta\t", 2, L"b",  L"a"  },
    { L" b\tat",  2, L"b",  L"at" },
    { L"b\tat",   2, L"b",  L"at" },
    { L"b\t at",  2, L"b",  L"at" },
  };
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(data); ++i) {
    std::vector<std::wstring> results;
    SplitStringAlongWhitespace(data[i].input, &results);
    ASSERT_EQ(static_cast<std::vector<std::wstring>::size_type>(
                  data[i].expected_result_count),
              results.size());
    if (data[i].expected_result_count > 0)
      ASSERT_EQ(data[i].output1, results[0]);
    if (data[i].expected_result_count > 1)
      ASSERT_EQ(data[i].output2, results[1]);
  }
}

TEST(StringUtilTest, MatchPatternTest) {
  EXPECT_EQ(MatchPattern(L"www.google.com", L"*.com"), true);
  EXPECT_EQ(MatchPattern(L"www.google.com", L"*"), true);
  EXPECT_EQ(MatchPattern(L"www.google.com", L"www*.g*.org"), false);
  EXPECT_EQ(MatchPattern(L"Hello", L"H?l?o"), true);
  EXPECT_EQ(MatchPattern(L"www.google.com", L"http://*)"), false);
  EXPECT_EQ(MatchPattern(L"www.msn.com", L"*.COM"), false);
  EXPECT_EQ(MatchPattern(L"Hello*1234", L"He??o\\*1*"), true);
  EXPECT_EQ(MatchPattern(L"", L"*.*"), false);
  EXPECT_EQ(MatchPattern(L"", L"*"), true);
  EXPECT_EQ(MatchPattern(L"", L"?"), true);
  EXPECT_EQ(MatchPattern(L"", L""), true);
  EXPECT_EQ(MatchPattern(L"Hello", L""), false);
  EXPECT_EQ(MatchPattern(L"Hello*", L"Hello*"), true);
  EXPECT_EQ(MatchPattern("Hello*", "Hello*"), true);  // narrow string
}


