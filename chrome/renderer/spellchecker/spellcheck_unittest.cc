// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/glue/webkit_glue.h"

#include "base/file_util.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "base/platform_file.h"
#include "base/sys_string_conversions.h"
#include "base/utf_string_conversions.h"
#include "chrome/renderer/spellchecker/spellcheck.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/spellcheck_common.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

FilePath GetHunspellDirectory() {
  FilePath hunspell_directory;
  if (!PathService::Get(base::DIR_SOURCE_ROOT, &hunspell_directory))
    return FilePath();

  hunspell_directory = hunspell_directory.AppendASCII("third_party");
  hunspell_directory = hunspell_directory.AppendASCII("hunspell_dictionaries");
  return hunspell_directory;
}

class SpellCheckTest : public testing::Test {
 public:
  SpellCheckTest() {
    ReinitializeSpellCheck("en-US");
  }

  void ReinitializeSpellCheck(const std::string& language) {
    spell_check_.reset(new SpellCheck());

    FilePath hunspell_directory = GetHunspellDirectory();
    EXPECT_FALSE(hunspell_directory.empty());
    base::PlatformFile file = base::CreatePlatformFile(
        SpellCheckCommon::GetVersionedFileName(language, hunspell_directory),
        base::PLATFORM_FILE_OPEN | base::PLATFORM_FILE_READ, NULL, NULL);
    spell_check_->Init(
        file, std::vector<std::string>(), language);
  }

  virtual ~SpellCheckTest() {
  }

  SpellCheck* spell_check() { return spell_check_.get(); }

 private:
  scoped_ptr<SpellCheck> spell_check_;
};

// Operates unit tests for the webkit_glue::SpellCheckWord() function
// with the US English dictionary.
// The unit tests in this function consist of:
//   * Tests for the function with empty strings;
//   * Tests for the function with a valid English word;
//   * Tests for the function with a valid non-English word;
//   * Tests for the function with a valid English word with a preceding
//     space character;
//   * Tests for the function with a valid English word with a preceding
//     non-English word;
//   * Tests for the function with a valid English word with a following
//     space character;
//   * Tests for the function with a valid English word with a following
//     non-English word;
//   * Tests for the function with two valid English words concatenated
//     with space characters or non-English words;
//   * Tests for the function with an invalid English word;
//   * Tests for the function with an invalid English word with a preceding
//     space character;
//   * Tests for the function with an invalid English word with a preceding
//     non-English word;
//   * Tests for the function with2 an invalid English word with a following
//     space character;
//   * Tests for the function with an invalid English word with a following
//     non-English word, and;
//   * Tests for the function with two invalid English words concatenated
//     with space characters or non-English words.
// A test with a "[ROBUSTNESS]" mark shows it is a robustness test and it uses
// grammartically incorrect string.
// TODO(hbono): Please feel free to add more tests.
TEST_F(SpellCheckTest, SpellCheckStrings_EN_US) {
  static const struct {
    // A string to be tested.
    const wchar_t* input;
    // An expected result for this test case.
    //   * true: the input string does not have any invalid words.
    //   * false: the input string has one or more invalid words.
    bool expected_result;
    // The position and the length of the first invalid word.
    int misspelling_start;
    int misspelling_length;
  } kTestCases[] = {
    // Empty strings.
    {L"", true, 0, 0},
    {L" ", true, 0, 0},
    {L"\xA0", true, 0, 0},
    {L"\x3000", true, 0, 0},

    // A valid English word "hello".
    {L"hello", true, 0, 0},
    // A valid Chinese word (meaning "hello") consisiting of two CJKV
    // ideographs
    {L"\x4F60\x597D", true, 0, 0},
    // A valid Korean word (meaning "hello") consisting of five hangul
    // syllables
    {L"\xC548\xB155\xD558\xC138\xC694", true, 0, 0},
    // A valid Japanese word (meaning "hello") consisting of five Hiragana
    // letters
    {L"\x3053\x3093\x306B\x3061\x306F", true, 0, 0},
    // A valid Hindi word (meaning ?) consisting of six Devanagari letters
    // (This word is copied from "http://b/issue?id=857583".)
    {L"\x0930\x093E\x091C\x0927\x093E\x0928", true, 0, 0},
    // A valid English word "affix" using a Latin ligature 'ffi'
    {L"a\xFB03x", true, 0, 0},
    // A valid English word "hello" (fullwidth version)
    {L"\xFF28\xFF45\xFF4C\xFF4C\xFF4F", true, 0, 0},
    // Two valid Greek words (meaning "hello") consisting of seven Greek
    // letters
    {L"\x03B3\x03B5\x03B9\x03AC" L" " L"\x03C3\x03BF\x03C5", true, 0, 0},
    // A valid Russian word (meainng "hello") consisting of twelve Cyrillic
    // letters
    {L"\x0437\x0434\x0440\x0430\x0432\x0441"
     L"\x0442\x0432\x0443\x0439\x0442\x0435", true, 0, 0},
    // A valid English contraction
    {L"isn't", true, 0, 0},
    // A valid English word enclosed with underscores.
    {L"_hello_", true, 0, 0},

    // A valid English word with a preceding whitespace
    {L" " L"hello", true, 0, 0},
    // A valid English word with a preceding no-break space
    {L"\xA0" L"hello", true, 0, 0},
    // A valid English word with a preceding ideographic space
    {L"\x3000" L"hello", true, 0, 0},
    // A valid English word with a preceding Chinese word
    {L"\x4F60\x597D" L"hello", true, 0, 0},
    // [ROBUSTNESS] A valid English word with a preceding Korean word
    {L"\xC548\xB155\xD558\xC138\xC694" L"hello", true, 0, 0},
    // A valid English word with a preceding Japanese word
    {L"\x3053\x3093\x306B\x3061\x306F" L"hello", true, 0, 0},
    // [ROBUSTNESS] A valid English word with a preceding Hindi word
    {L"\x0930\x093E\x091C\x0927\x093E\x0928" L"hello", true, 0, 0},
    // [ROBUSTNESS] A valid English word with two preceding Greek words
    {L"\x03B3\x03B5\x03B9\x03AC" L" " L"\x03C3\x03BF\x03C5"
     L"hello", true, 0, 0},
    // [ROBUSTNESS] A valid English word with a preceding Russian word
    {L"\x0437\x0434\x0440\x0430\x0432\x0441"
     L"\x0442\x0432\x0443\x0439\x0442\x0435" L"hello", true, 0, 0},

    // A valid English word with a following whitespace
    {L"hello" L" ", true, 0, 0},
    // A valid English word with a following no-break space
    {L"hello" L"\xA0", true, 0, 0},
    // A valid English word with a following ideographic space
    {L"hello" L"\x3000", true, 0, 0},
    // A valid English word with a following Chinese word
    {L"hello" L"\x4F60\x597D", true, 0, 0},
    // [ROBUSTNESS] A valid English word with a following Korean word
    {L"hello" L"\xC548\xB155\xD558\xC138\xC694", true, 0, 0},
    // A valid English word with a following Japanese word
    {L"hello" L"\x3053\x3093\x306B\x3061\x306F", true, 0, 0},
    // [ROBUSTNESS] A valid English word with a following Hindi word
    {L"hello" L"\x0930\x093E\x091C\x0927\x093E\x0928", true, 0, 0},
    // [ROBUSTNESS] A valid English word with two following Greek words
    {L"hello"
     L"\x03B3\x03B5\x03B9\x03AC" L" " L"\x03C3\x03BF\x03C5", true, 0, 0},
    // [ROBUSTNESS] A valid English word with a following Russian word
    {L"hello" L"\x0437\x0434\x0440\x0430\x0432\x0441"
     L"\x0442\x0432\x0443\x0439\x0442\x0435", true, 0, 0},

    // Two valid English words concatenated with a whitespace
    {L"hello" L" " L"hello", true, 0, 0},
    // Two valid English words concatenated with a no-break space
    {L"hello" L"\xA0" L"hello", true, 0, 0},
    // Two valid English words concatenated with an ideographic space
    {L"hello" L"\x3000" L"hello", true, 0, 0},
    // Two valid English words concatenated with a Chinese word
    {L"hello" L"\x4F60\x597D" L"hello", true, 0, 0},
    // [ROBUSTNESS] Two valid English words concatenated with a Korean word
    {L"hello" L"\xC548\xB155\xD558\xC138\xC694" L"hello", true, 0, 0},
    // Two valid English words concatenated with a Japanese word
    {L"hello" L"\x3053\x3093\x306B\x3061\x306F" L"hello", true, 0, 0},
    // [ROBUSTNESS] Two valid English words concatenated with a Hindi word
    {L"hello" L"\x0930\x093E\x091C\x0927\x093E\x0928" L"hello" , true, 0, 0},
    // [ROBUSTNESS] Two valid English words concatenated with two Greek words
    {L"hello" L"\x03B3\x03B5\x03B9\x03AC" L" " L"\x03C3\x03BF\x03C5"
     L"hello", true, 0, 0},
    // [ROBUSTNESS] Two valid English words concatenated with a Russian word
    {L"hello" L"\x0437\x0434\x0440\x0430\x0432\x0441"
     L"\x0442\x0432\x0443\x0439\x0442\x0435" L"hello", true, 0, 0},
    // [ROBUSTNESS] Two valid English words concatenated with a contraction
    // character.
    {L"hello:hello", true, 0, 0},

    // An invalid English word
    {L"ifmmp", false, 0, 5},
    // An invalid English word "bffly" containing a Latin ligature 'ffl'
    {L"b\xFB04y", false, 0, 3},
    // An invalid English word "ifmmp" (fullwidth version)
    {L"\xFF29\xFF46\xFF4D\xFF4D\xFF50", false, 0, 5},
    // An invalid English contraction
    {L"jtm'u", false, 0, 5},
    // An invalid English word enclosed with underscores.
    {L"_ifmmp_", false, 1, 5},

    // An invalid English word with a preceding whitespace
    {L" " L"ifmmp", false, 1, 5},
    // An invalid English word with a preceding no-break space
    {L"\xA0" L"ifmmp", false, 1, 5},
    // An invalid English word with a preceding ideographic space
    {L"\x3000" L"ifmmp", false, 1, 5},
    // An invalid English word with a preceding Chinese word
    {L"\x4F60\x597D" L"ifmmp", false, 2, 5},
    // [ROBUSTNESS] An invalid English word with a preceding Korean word
    {L"\xC548\xB155\xD558\xC138\xC694" L"ifmmp", false, 5, 5},
    // An invalid English word with a preceding Japanese word
    {L"\x3053\x3093\x306B\x3061\x306F" L"ifmmp", false, 5, 5},
    // [ROBUSTNESS] An invalid English word with a preceding Hindi word
    {L"\x0930\x093E\x091C\x0927\x093E\x0928" L"ifmmp", false, 6, 5},
    // [ROBUSTNESS] An invalid English word with two preceding Greek words
    {L"\x03B3\x03B5\x03B9\x03AC" L" " L"\x03C3\x03BF\x03C5"
     L"ifmmp", false, 8, 5},
    // [ROBUSTNESS] An invalid English word with a preceding Russian word
    {L"\x0437\x0434\x0440\x0430\x0432\x0441"
     L"\x0442\x0432\x0443\x0439\x0442\x0435" L"ifmmp", false, 12, 5},

    // An invalid English word with a following whitespace
    {L"ifmmp" L" ", false, 0, 5},
    // An invalid English word with a following no-break space
    {L"ifmmp" L"\xA0", false, 0, 5},
    // An invalid English word with a following ideographic space
    {L"ifmmp" L"\x3000", false, 0, 5},
    // An invalid English word with a following Chinese word
    {L"ifmmp" L"\x4F60\x597D", false, 0, 5},
    // [ROBUSTNESS] An invalid English word with a following Korean word
    {L"ifmmp" L"\xC548\xB155\xD558\xC138\xC694", false, 0, 5},
    // An invalid English word with a following Japanese word
    {L"ifmmp" L"\x3053\x3093\x306B\x3061\x306F", false, 0, 5},
    // [ROBUSTNESS] An invalid English word with a following Hindi word
    {L"ifmmp" L"\x0930\x093E\x091C\x0927\x093E\x0928", false, 0, 5},
    // [ROBUSTNESS] An invalid English word with two following Greek words
    {L"ifmmp"
     L"\x03B3\x03B5\x03B9\x03AC" L" " L"\x03C3\x03BF\x03C5", false, 0, 5},
    // [ROBUSTNESS] An invalid English word with a following Russian word
    {L"ifmmp" L"\x0437\x0434\x0440\x0430\x0432\x0441"
     L"\x0442\x0432\x0443\x0439\x0442\x0435", false, 0, 5},

    // Two invalid English words concatenated with a whitespace
    {L"ifmmp" L" " L"ifmmp", false, 0, 5},
    // Two invalid English words concatenated with a no-break space
    {L"ifmmp" L"\xA0" L"ifmmp", false, 0, 5},
    // Two invalid English words concatenated with an ideographic space
    {L"ifmmp" L"\x3000" L"ifmmp", false, 0, 5},
    // Two invalid English words concatenated with a Chinese word
    {L"ifmmp" L"\x4F60\x597D" L"ifmmp", false, 0, 5},
    // [ROBUSTNESS] Two invalid English words concatenated with a Korean word
    {L"ifmmp" L"\xC548\xB155\xD558\xC138\xC694" L"ifmmp", false, 0, 5},
    // Two invalid English words concatenated with a Japanese word
    {L"ifmmp" L"\x3053\x3093\x306B\x3061\x306F" L"ifmmp", false, 0, 5},
    // [ROBUSTNESS] Two invalid English words concatenated with a Hindi word
    {L"ifmmp" L"\x0930\x093E\x091C\x0927\x093E\x0928" L"ifmmp" , false, 0, 5},
    // [ROBUSTNESS] Two invalid English words concatenated with two Greek words
    {L"ifmmp" L"\x03B3\x03B5\x03B9\x03AC" L" " L"\x03C3\x03BF\x03C5"
     L"ifmmp", false, 0, 5},
    // [ROBUSTNESS] Two invalid English words concatenated with a Russian word
    {L"ifmmp" L"\x0437\x0434\x0440\x0430\x0432\x0441"
     L"\x0442\x0432\x0443\x0439\x0442\x0435" L"ifmmp", false, 0, 5},
    // [ROBUSTNESS] Two invalid English words concatenated with a contraction
    // character.
    {L"ifmmp:ifmmp", false, 0, 11},

    // [REGRESSION] Issue 13432: "Any word of 13 or 14 characters is not
    // spellcheck" <http://crbug.com/13432>.
    {L"qwertyuiopasd", false, 0, 13},
    {L"qwertyuiopasdf", false, 0, 14},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(kTestCases); ++i) {
    size_t input_length = 0;
    if (kTestCases[i].input != NULL) {
      input_length = wcslen(kTestCases[i].input);
    }
    int misspelling_start;
    int misspelling_length;
    bool result = spell_check()->SpellCheckWord(
        WideToUTF16(kTestCases[i].input).c_str(),
        static_cast<int>(input_length),
        0,
        &misspelling_start,
        &misspelling_length, NULL);

    EXPECT_EQ(kTestCases[i].expected_result, result);
    EXPECT_EQ(kTestCases[i].misspelling_start, misspelling_start);
    EXPECT_EQ(kTestCases[i].misspelling_length, misspelling_length);
  }
}

TEST_F(SpellCheckTest, SpellCheckSuggestions_EN_US) {
  static const struct {
    // A string to be tested.
    const wchar_t* input;
    // An expected result for this test case.
    //   * true: the input string does not have any invalid words.
    //   * false: the input string has one or more invalid words.
    bool expected_result;
    // The position and the length of the first invalid word.
    int misspelling_start;
    int misspelling_length;

    // A suggested word that should occur.
    const wchar_t* suggested_word;
  } kTestCases[] = {
    {L"ello", false, 0, 0, L"hello"},
    {L"ello", false, 0, 0, L"cello"},
    {L"wate", false, 0, 0, L"water"},
    {L"wate", false, 0, 0, L"waste"},
    {L"wate", false, 0, 0, L"sate"},
    {L"wate", false, 0, 0, L"ate"},
    {L"jum", false, 0, 0, L"jump"},
    {L"jum", false, 0, 0, L"hum"},
    {L"jum", false, 0, 0, L"sum"},
    {L"jum", false, 0, 0, L"um"},
    // A regression test for Issue 36523.
    {L"privliged", false, 0, 0, L"privileged"},
    // TODO (Sidchat): add many more examples.
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(kTestCases); ++i) {
    std::vector<string16> suggestions;
    size_t input_length = 0;
    if (kTestCases[i].input != NULL) {
      input_length = wcslen(kTestCases[i].input);
    }
    int misspelling_start;
    int misspelling_length;
    bool result = spell_check()->SpellCheckWord(
        WideToUTF16(kTestCases[i].input).c_str(),
        static_cast<int>(input_length),
        0,
        &misspelling_start,
        &misspelling_length,
        &suggestions);

    // Check for spelling.
    EXPECT_EQ(kTestCases[i].expected_result, result);

    // Check if the suggested words occur.
    bool suggested_word_is_present = false;
    for (int j = 0; j < static_cast<int>(suggestions.size()); j++) {
      if (suggestions.at(j).compare(WideToUTF16(kTestCases[i].suggested_word))
          == 0) {
        suggested_word_is_present = true;
        break;
      }
    }

    EXPECT_TRUE(suggested_word_is_present);
  }
}

// This test verifies our spellchecker can split a text into words and check
// the spelling of each word in the text.
TEST_F(SpellCheckTest, SpellCheckText) {
  static const struct {
    const char* language;
    const wchar_t* input;
  } kTestCases[] = {
    {
      // Catalan
      "ca-ES",
      L"La missi\x00F3 de Google \x00E9s organitzar la informaci\x00F3 "
      L"del m\x00F3n i fer que sigui \x00FAtil i accessible universalment."
    }, {
      // Czech
      "cs-CZ",
      L"Posl\x00E1n\x00EDm spole\x010Dnosti Google je "
      L"uspo\x0159\x00E1\x0064\x0061t informace z cel\x00E9ho sv\x011Bta "
      L"tak, aby byly v\x0161\x0065obecn\x011B p\x0159\x00EDstupn\x00E9 "
      L"a u\x017Eite\x010Dn\x00E9."
    }, {
      // Danish
      "da-DK",
      L"Googles "
      L"mission er at organisere verdens information og g\x00F8re den "
      L"almindeligt tilg\x00E6ngelig og nyttig."
    }, {
      // German
      "de-DE",
      L"Das Ziel von Google besteht darin, die auf der Welt vorhandenen "
      L"Informationen zu organisieren und allgemein zug\x00E4nglich und "
      L"nutzbar zu machen."
    }, {
      // Greek
      "el-GR",
      L"\x0391\x03C0\x03BF\x03C3\x03C4\x03BF\x03BB\x03AE "
      L"\x03C4\x03B7\x03C2 Google \x03B5\x03AF\x03BD\x03B1\x03B9 "
      L"\x03BD\x03B1 \x03BF\x03C1\x03B3\x03B1\x03BD\x03CE\x03BD\x03B5\x03B9 "
      L"\x03C4\x03B9\x03C2 "
      L"\x03C0\x03BB\x03B7\x03C1\x03BF\x03C6\x03BF\x03C1\x03AF\x03B5\x03C2 "
      L"\x03C4\x03BF\x03C5 \x03BA\x03CC\x03C3\x03BC\x03BF\x03C5 "
      L"\x03BA\x03B1\x03B9 \x03BD\x03B1 \x03C4\x03B9\x03C2 "
      L"\x03BA\x03B1\x03B8\x03B9\x03C3\x03C4\x03AC "
      L"\x03C0\x03C1\x03BF\x03C3\x03B2\x03AC\x03C3\x03B9\x03BC\x03B5\x03C2 "
      L"\x03BA\x03B1\x03B9 \x03C7\x03C1\x03AE\x03C3\x03B9\x03BC\x03B5\x03C2."
    }, {
      // English (Australia)
      "en-AU",
      // L"Google's " - to be added.
      L"mission is to organise the world's information and make it "
      L"universally accessible and useful."
    }, {
      // English (United Kingdom)
      "en-GB",
      // L"Google's " - to be added.
      L"mission is to organise the world's information and make it "
      L"universally accessible and useful."
    }, {
      // English (United States)
      "en-US",
      L"Google's mission is to organize the world's information and make it "
      L"universally accessible and useful."
    }, {
      // Bulgarian
      "bg-BG",
      L"\x041c\x0438\x0441\x0438\x044f\x0442\x0430 "
      L"\x043d\x0430 Google \x0435 \x0434\x0430 \x043e"
      L"\x0440\x0433\x0430\x043d\x0438\x0437\x0438\x0440"
      L"\x0430 \x0441\x0432\x0435\x0442\x043e\x0432"
      L"\x043d\x0430\x0442\x0430 \x0438\x043d\x0444"
      L"\x043e\x0440\x043c\x0430\x0446\x0438\x044f "
      L"\x0438 \x0434\x0430 \x044f \x043d"
      L"\x0430\x043f\x0440\x0430\x0432\x0438 \x0443"
      L"\x043d\x0438\x0432\x0435\x0440\x0441\x0430\x043b"
      L"\x043d\x043e \x0434\x043e\x0441\x0442\x044a"
      L"\x043f\x043d\x0430 \x0438 \x043f\x043e"
      L"\x043b\x0435\x0437\x043d\x0430."
    }, {
      // Spanish
      "es-ES",
      L"La misi\x00F3n de "
      // L"Google" - to be added.
      L" es organizar la informaci\x00F3n mundial "
      L"para que resulte universalmente accesible y \x00FAtil."
    }, {
      // Estonian
      "et-EE",
      // L"Google'ile " - to be added.
      L"\x00FClesanne on korraldada maailma teavet ja teeb selle "
      L"k\x00F5igile k\x00E4ttesaadavaks ja kasulikuks.",
    }, {
      // French
      "fr-FR",
      L"Google a pour mission d'organiser les informations \x00E0 "
      L"l'\x00E9\x0063helle mondiale dans le but de les rendre accessibles "
      L"et utiles \x00E0 tous."
    }, {
      // Hebrew
      "he-IL",
      L"\x05D4\x05DE\x05E9\x05D9\x05DE\x05D4 \x05E9\x05DC Google "
      L"\x05D4\x05D9\x05D0 \x05DC\x05D0\x05E8\x05D2\x05DF "
      L"\x05D0\x05EA \x05D4\x05DE\x05D9\x05D3\x05E2 "
      L"\x05D4\x05E2\x05D5\x05DC\x05DE\x05D9 "
      L"\x05D5\x05DC\x05D4\x05E4\x05D5\x05DA \x05D0\x05D5\x05EA\x05D5 "
      L"\x05DC\x05D6\x05DE\x05D9\x05DF "
      L"\x05D5\x05E9\x05D9\x05DE\x05D5\x05E9\x05D9 \x05D1\x05DB\x05DC "
      L"\x05D4\x05E2\x05D5\x05DC\x05DD. "
      // Two words with ASCII double/single quoation marks.
      L"\x05DE\x05E0\x05DB\x0022\x05DC \x05E6\x0027\x05D9\x05E4\x05E1"
    }, {
      // Hindi
      "hi-IN",
      L"Google \x0915\x093E \x092E\x093F\x0936\x0928 "
      L"\x0926\x0941\x0928\x093F\x092F\x093E \x0915\x0940 "
      L"\x091C\x093E\x0928\x0915\x093E\x0930\x0940 \x0915\x094B "
      L"\x0935\x094D\x092F\x0935\x0938\x094D\x0925\x093F\x0924 "
      L"\x0915\x0930\x0928\x093E \x0914\x0930 \x0909\x0938\x0947 "
      L"\x0938\x093E\x0930\x094D\x0935\x092D\x094C\x092E\x093F\x0915 "
      L"\x0930\x0942\x092A \x0938\x0947 \x092A\x0939\x0941\x0901\x091A "
      L"\x092E\x0947\x0902 \x0914\x0930 \x0909\x092A\x092F\x094B\x0917\x0940 "
      L"\x092C\x0928\x093E\x0928\x093E \x0939\x0948."
    }, {
      // Hungarian
      "hu-HU",
      L"A Google azt a k\x00FCldet\x00E9st v\x00E1llalta mag\x00E1ra, "
      L"hogy a vil\x00E1gon fellelhet\x0151 inform\x00E1\x0063i\x00F3kat "
      L"rendszerezze \x00E9s \x00E1ltal\x00E1nosan el\x00E9rhet\x0151v\x00E9, "
      L"illetve haszn\x00E1lhat\x00F3v\x00E1 tegye."
    }, {
      // Croatian
      "hr-HR",
      // L"Googleova " - to be added.
      L"je misija organizirati svjetske informacije i u\x010Diniti ih "
      // L"univerzalno " - to be added.
      L"pristupa\x010Dnima i korisnima."
    }, {
      // Indonesian
      "id-ID",
      L"Misi Google adalah untuk mengelola informasi dunia dan membuatnya "
      L"dapat diakses dan bermanfaat secara universal."
    }, {
      // Italian
      "it-IT",
      L"La missione di Google \x00E8 organizzare le informazioni a livello "
      L"mondiale e renderle universalmente accessibili e fruibili."
    }, {
      // Lithuanian
      "lt-LT",
      L"\x201EGoogle\x201C tikslas \x2013 rinkti ir sisteminti pasaulio "
      L"informacij\x0105 bei padaryti j\x0105 prieinam\x0105 ir "
      L"nauding\x0105 visiems."
    }, {
      // Latvian
      "lv-LV",
      L"Google uzdevums ir k\x0101rtot pasaules inform\x0101"
      L"ciju un padar\x012Bt to univers\x0101li pieejamu un noder\x012Bgu."
    }, {
      // Norwegian
      "nb-NO",
      // L"Googles " - to be added.
      L"m\x00E5l er \x00E5 organisere informasjonen i verden og "
      L"gj\x00F8re den tilgjengelig og nyttig for alle."
    }, {
      // Dutch
      "nl-NL",
      L"Het doel van Google is om alle informatie wereldwijd toegankelijk "
      L"en bruikbaar te maken."
    }, {
      // Polish
      "pl-PL",
      L"Misj\x0105 Google jest uporz\x0105" L"dkowanie \x015Bwiatowych "
      L"zasob\x00F3w informacji, aby sta\x0142y si\x0119 one powszechnie "
      L"dost\x0119pne i u\x017Cyteczne."
    }, {
      // Portuguese (Brazil)
      "pt-BR",
      L"A miss\x00E3o do "
#if !defined(OS_MACOSX)
      L"Google "
#endif
      L"\x00E9 organizar as informa\x00E7\x00F5"
      L"es do mundo todo e "
#if !defined(OS_MACOSX)
      L"torn\x00E1-las "
#endif
      L"acess\x00EDveis e "
      // L"\x00FAteis " - to be added.
      L"em car\x00E1ter universal."
    }, {
      // Portuguese (Portugal)
      "pt-PT",
      L"O "
#if !defined(OS_MACOSX)
      L"Google "
#endif
      L"tem por miss\x00E3o organizar a informa\x00E7\x00E3o do "
      L"mundo e "
#if !defined(OS_MACOSX)
      L"torn\x00E1-la "
#endif
      L"universalmente acess\x00EDvel e \x00FAtil"
    }, {
      // Romanian
      "ro-RO",
      L"Misiunea Google este de a organiza informa\x021B3iile lumii \x0219i de "
      L"a le face accesibile \x0219i utile la nivel universal."
    }, {
      // Russian
      "ru-RU",
      L"\x041C\x0438\x0441\x0441\x0438\x044F Google "
      L"\x0441\x043E\x0441\x0442\x043E\x0438\x0442 \x0432 "
      L"\x043E\x0440\x0433\x0430\x043D\x0438\x0437\x0430\x0446\x0438\x0438 "
      L"\x043C\x0438\x0440\x043E\x0432\x043E\x0439 "
      L"\x0438\x043D\x0444\x043E\x0440\x043C\x0430\x0446\x0438\x0438, "
      L"\x043E\x0431\x0435\x0441\x043F\x0435\x0447\x0435\x043D\x0438\x0438 "
      L"\x0435\x0435 "
      L"\x0434\x043E\x0441\x0442\x0443\x043F\x043D\x043E\x0441\x0442\x0438 "
      L"\x0438 \x043F\x043E\x043B\x044C\x0437\x044B \x0434\x043B\x044F "
      L"\x0432\x0441\x0435\x0445."
      // A Russian word including U+0451. (Bug 15558 <http://crbug.com/15558>)
      L"\u0451\u043B\u043A\u0430"
    }, {
      // Serbian
      "sr",
      L"\x0047\x006f\x006f\x0067\x006c\x0065\x002d\x043e\x0432\x0430 "
      L"\x043c\x0438\x0441\x0438\x0458\x0430 \x0458\x0435 \x0434\x0430 "
      L"\x043e\x0440\x0433\x0430\x043d\x0438\x0437\x0443\x0458\x0435 "
      L"\x0441\x0432\x0435 "
      L"\x0438\x043d\x0444\x043e\x0440\x043c\x0430\x0446\x0438\x0458\x0435 "
      L"\x043d\x0430 \x0441\x0432\x0435\x0442\x0443 \x0438 "
      L"\x0443\x0447\x0438\x043d\x0438 \x0438\x0445 "
      L"\x0443\x043d\x0438\x0432\x0435\x0440\x0437\x0430\x043b\x043d\x043e "
      L"\x0434\x043e\x0441\x0442\x0443\x043f\x043d\x0438\x043c \x0438 "
      L"\x043a\x043e\x0440\x0438\x0441\x043d\x0438\x043c."
    }, {
      // Slovak
      "sk-SK",
      L"Spolo\x010Dnos\x0165 Google si dala za \x00FAlohu usporiada\x0165 "
      L"inform\x00E1\x0063ie "
      L"z cel\x00E9ho sveta a zabezpe\x010Di\x0165, "
      L"aby boli v\x0161eobecne dostupn\x00E9 a u\x017Eito\x010Dn\x00E9."
    }, {
      // Slovenian
      "sl-SI",
      // L"Googlovo " - to be added.
      L"poslanstvo je organizirati svetovne informacije in "
      L"omogo\x010Diti njihovo dostopnost in s tem uporabnost za vse."
    }, {
      // Swedish
      "sv-SE",
      L"Googles m\x00E5ls\x00E4ttning \x00E4r att ordna v\x00E4rldens "
      L"samlade information och g\x00F6ra den tillg\x00E4nglig f\x00F6r alla."
    }, {
      // Turkish
      "tr-TR",
      // L"Google\x2019\x0131n " - to be added.
      L"misyonu, d\x00FCnyadaki t\x00FCm bilgileri "
      L"organize etmek ve evrensel olarak eri\x015Filebilir ve "
      L"kullan\x0131\x015Fl\x0131 k\x0131lmakt\x0131r."
    }, {
      // Ukranian
      "uk-UA",
      L"\x041c\x0456\x0441\x0456\x044f "
      L"\x043a\x043e\x043c\x043f\x0430\x043d\x0456\x0457 Google "
      L"\x043f\x043e\x043b\x044f\x0433\x0430\x0454 \x0432 "
      L"\x0442\x043e\x043c\x0443, \x0449\x043e\x0431 "
      L"\x0443\x043f\x043e\x0440\x044f\x0434\x043a\x0443\x0432\x0430\x0442"
      L"\x0438 \x0456\x043d\x0444\x043e\x0440\x043c\x0430\x0446\x0456\x044e "
      L"\x0437 \x0443\x0441\x044c\x043e\x0433\x043e "
      L"\x0441\x0432\x0456\x0442\x0443 \x0442\x0430 "
      L"\x0437\x0440\x043e\x0431\x0438\x0442\x0438 \x0457\x0457 "
      L"\x0443\x043d\x0456\x0432\x0435\x0440\x0441\x0430\x043b\x044c\x043d"
      L"\x043e \x0434\x043e\x0441\x0442\x0443\x043f\x043d\x043e\x044e "
      L"\x0442\x0430 \x043a\x043e\x0440\x0438\x0441\x043d\x043e\x044e."
    }, {
      // Vietnamese
      "vi-VN",
      L"Nhi\x1EC7m v\x1EE5 c\x1EE7\x0061 "
      L"Google la \x0111\x1EC3 t\x1ED5 ch\x1EE9\x0063 "
      L"c\x00E1\x0063 th\x00F4ng tin c\x1EE7\x0061 "
      L"th\x1EBF gi\x1EDBi va l\x00E0m cho n\x00F3 universal c\x00F3 "
      L"th\x1EC3 truy c\x1EADp va h\x1EEFu d\x1EE5ng h\x01A1n."
    },
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(kTestCases); ++i) {
    ReinitializeSpellCheck(kTestCases[i].language);
    size_t input_length = 0;
    if (kTestCases[i].input != NULL)
      input_length = wcslen(kTestCases[i].input);

    int misspelling_start = 0;
    int misspelling_length = 0;
    bool result = spell_check()->SpellCheckWord(
        WideToUTF16(kTestCases[i].input).c_str(),
        static_cast<int>(input_length),
        0,
        &misspelling_start,
        &misspelling_length, NULL);

    EXPECT_EQ(true, result) << kTestCases[i].language;
    EXPECT_EQ(0, misspelling_start);
    EXPECT_EQ(0, misspelling_length);
  }
}

TEST_F(SpellCheckTest, GetAutoCorrectionWord_EN_US) {
  static const struct {
    // A misspelled word.
    const char* input;

    // An expected result for this test case.
    // Should be an empty string if there are no suggestions for auto correct.
    const char* expected_result;
  } kTestCases[] = {
    {"teh", "the"},
    {"moer", "more"},
    {"watre", "water"},
    {"noen", ""},
    {"what", ""},
  };
  spell_check()->EnableAutoSpellCorrect(true);

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(kTestCases); ++i) {
    string16 misspelled_word(UTF8ToUTF16(kTestCases[i].input));
    string16 expected_autocorrect_word(
        UTF8ToUTF16(kTestCases[i].expected_result));
    string16 autocorrect_word = spell_check()->GetAutoCorrectionWord(
        misspelled_word, 0);

    // Check for spelling.
    EXPECT_EQ(expected_autocorrect_word, autocorrect_word);
  }
}

}  // namespace
