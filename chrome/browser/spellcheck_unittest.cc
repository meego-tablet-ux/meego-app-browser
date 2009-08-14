// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/glue/webkit_glue.h"

#include "base/file_util.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "chrome/browser/spellchecker.h"
#include "chrome/common/chrome_paths.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
const FilePath::CharType kTempCustomDictionaryFile[] =
    FILE_PATH_LITERAL("temp_custom_dictionary.txt");
}  // namespace

class SpellCheckTest : public testing::Test {
 private:
  MessageLoop message_loop_;
};

// Represents a special initialization function used only for the unit tests
// in this file.
extern void InitHunspellWithFiles(FILE* file_aff_hunspell,
                                  FILE* file_dic_hunspell);

FilePath GetHunspellDirectory() {
  FilePath hunspell_directory;
  if (!PathService::Get(base::DIR_SOURCE_ROOT, &hunspell_directory))
    return FilePath();

  hunspell_directory = hunspell_directory.AppendASCII("chrome");
  hunspell_directory = hunspell_directory.AppendASCII("third_party");
  hunspell_directory = hunspell_directory.AppendASCII("hunspell");
  hunspell_directory = hunspell_directory.AppendASCII("dictionaries");
  return hunspell_directory;
}

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
    {NULL, true, 0, 0},
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

  FilePath hunspell_directory = GetHunspellDirectory();
  ASSERT_FALSE(hunspell_directory.empty());

  scoped_refptr<SpellChecker> spell_checker(new SpellChecker(
      hunspell_directory, "en-US", NULL, FilePath()));

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(kTestCases); ++i) {
    size_t input_length = 0;
    if (kTestCases[i].input != NULL) {
      input_length = wcslen(kTestCases[i].input);
    }
    int misspelling_start;
    int misspelling_length;
    bool result = spell_checker->SpellCheckWord(kTestCases[i].input,
                                                static_cast<int>(input_length),
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
  } kTestCases[] = {    // A valid English word with a preceding whitespace
    // We need to have separate test cases here, since hunspell and the OS X
    // spellchecking service occasionally differ on what they consider a valid
    // suggestion for a given word, although these lists could likely be
    // integrated somewhat.
#if defined(OS_MACOSX)
    // These words come from the wikipedia page of the most commonly
    // misspelled words in english.
    // (http://en.wikipedia.org/wiki/Commonly_misspelled_words).
    {L"absense", false, 0,0,L"absence"},
    {L"acceptible", false, 0,0,L"acceptable"},
    {L"accidentaly", false, 0,0,L"accidentally"},
    {L"accomodate", false, 0,0,L"accommodate"},
    {L"acheive", false, 0,0,L"achieve"},
    {L"acknowlege", false, 0,0,L"acknowledge"},
    {L"acquaintence", false, 0,0,L"acquaintance"},
    {L"aquire", false, 0,0,L"acquire"},
    {L"aquit", false, 0,0,L"acquit"},
    {L"acrage", false, 0,0,L"acreage"},
    {L"adress", false, 0,0,L"address"},
    {L"adultary", false, 0,0,L"adultery"},
    {L"advertize", false, 0,0,L"advertise"},
    {L"adviseable", false, 0,0,L"advisable"},
    {L"agression", false, 0,0,L"aggression"},
    {L"alchohol", false, 0,0,L"alcohol"},
    {L"alege", false, 0,0,L"allege"},
    {L"allegaince", false, 0,0,L"allegiance"},
    {L"allmost", false, 0,0,L"almost"},
    // Ideally, this test should pass. It works in firefox, but not in hunspell
    // or OS X.
    // {L"alot", false, 0,0,L"a lot"},
    {L"amatuer", false, 0,0,L"amateur"},
    {L"ammend", false, 0,0,L"amend"},
    {L"amung", false, 0,0,L"among"},
    {L"anually", false, 0,0,L"annually"},
    {L"apparant", false, 0,0,L"apparent"},
    {L"artic", false, 0,0,L"arctic"},
    {L"arguement", false, 0,0,L"argument"},
    {L"athiest", false, 0,0,L"atheist"},
    {L"athelete", false, 0,0,L"athlete"},
    {L"avrage", false, 0,0,L"average"},
    {L"awfull", false, 0,0,L"awful"},
    {L"ballance", false, 0,0,L"balance"},
    {L"basicly", false, 0,0,L"basically"},
    {L"becuase", false, 0,0,L"because"},
    {L"becomeing", false, 0,0,L"becoming"},
    {L"befor", false, 0,0,L"before"},
    {L"begining", false, 0,0,L"beginning"},
    {L"beleive", false, 0,0,L"believe"},
    {L"bellweather", false, 0,0,L"bellwether"},
    {L"benifit", false, 0,0,L"benefit"},
    {L"bouy", false, 0,0,L"buoy"},
    {L"briliant", false, 0,0,L"brilliant"},
    {L"burgler", false, 0,0,L"burglar"},
    {L"camoflage", false, 0,0,L"camouflage"},
    {L"carrer", false, 0,0,L"career"},
    {L"carefull", false, 0,0,L"careful"},
    {L"Carribean", false, 0,0,L"Caribbean"},
    {L"catagory", false, 0,0,L"category"},
    {L"cauhgt", false, 0,0,L"caught"},
    {L"cieling", false, 0,0,L"ceiling"},
    {L"cemetary", false, 0,0,L"cemetery"},
    {L"certin", false, 0,0,L"certain"},
    {L"changable", false, 0,0,L"changeable"},
    {L"cheif", false, 0,0,L"chief"},
    {L"citezen", false, 0,0,L"citizen"},
    {L"collaegue", false, 0,0,L"colleague"},
    {L"colum", false, 0,0,L"column"},
    {L"comming", false, 0,0,L"coming"},
    {L"commited", false, 0,0,L"committed"},
    {L"compitition", false, 0,0,L"competition"},
    {L"conceed", false, 0,0,L"concede"},
    {L"congradulate", false, 0,0,L"congratulate"},
    // TODO(pwicks): This fails as a result of 13432.
    // Once that is fixed, uncomment this.
    // {L"consciencious", false, 0,0,L"conscientious"},
    {L"concious", false, 0,0,L"conscious"},
    {L"concensus", false, 0,0,L"consensus"},
    {L"contraversy", false, 0,0,L"controversy"},
    {L"conveniance", false, 0,0,L"convenience"},
    {L"critecize", false, 0,0,L"criticize"},
    {L"dacquiri", false, 0,0,L"daiquiri"},
    {L"decieve", false, 0,0,L"deceive"},
    {L"dicide", false, 0,0,L"decide"},
    {L"definate", false, 0,0,L"definite"},
    {L"definitly", false, 0,0,L"definitely"},
    {L"deposite", false, 0,0,L"deposit"},
    {L"desparate", false, 0,0,L"desperate"},
    {L"develope", false, 0,0,L"develop"},
    {L"diffrence", false, 0,0,L"difference"},
    {L"dilema", false, 0,0,L"dilemma"},
    {L"disapear", false, 0,0,L"disappear"},
    {L"disapoint", false, 0,0,L"disappoint"},
    {L"disasterous", false, 0,0,L"disastrous"},
    {L"disipline", false, 0,0,L"discipline"},
    {L"drunkeness", false, 0,0,L"drunkenness"},
    {L"dumbell", false, 0,0,L"dumbbell"},
    {L"durring", false, 0,0,L"during"},
    {L"easely", false, 0,0,L"easily"},
    {L"eigth", false, 0,0,L"eight"},
    {L"embarass", false, 0,0,L"embarrass"},
    {L"enviroment", false, 0,0,L"environment"},
    {L"equiped", false, 0,0,L"equipped"},
    {L"equiptment", false, 0,0,L"equipment"},
    {L"exagerate", false, 0,0,L"exaggerate"},
    {L"excede", false, 0,0,L"exceed"},
    {L"exellent", false, 0,0,L"excellent"},
    {L"exsept", false, 0,0,L"except"},
    {L"exercize", false, 0,0,L"exercise"},
    {L"exilerate", false, 0,0,L"exhilarate"},
    {L"existance", false, 0,0,L"existence"},
    {L"experiance", false, 0,0,L"experience"},
    {L"experament", false, 0,0,L"experiment"},
    {L"explaination", false, 0,0,L"explanation"},
    {L"extreem", false, 0,0,L"extreme"},
    {L"familier", false, 0,0,L"familiar"},
    {L"facinating", false, 0,0,L"fascinating"},
    {L"firey", false, 0,0,L"fiery"},
    {L"finaly", false, 0,0,L"finally"},
    {L"flourescent", false, 0,0,L"fluorescent"},
    {L"foriegn", false, 0,0,L"foreign"},
    {L"fourty", false, 0,0,L"forty"},
    {L"foreward", false, 0,0,L"forward"},
    {L"freind", false, 0,0,L"friend"},
    {L"fullfil", false, 0,0,L"fulfill"},
    {L"fundemental", false, 0,0,L"fundamental"},
    {L"guage", false, 0,0,L"gauge"},
    {L"generaly", false, 0,0,L"generally"},
    {L"goverment", false, 0,0,L"government"},
    {L"grammer", false, 0,0,L"grammar"},
    {L"gratefull", false, 0,0,L"grateful"},
    {L"garantee", false, 0,0,L"guarantee"},
    {L"guidence", false, 0,0,L"guidance"},
    {L"happyness", false, 0,0,L"happiness"},
    {L"harrass", false, 0,0,L"harass"},
    {L"heighth", false, 0,0,L"height"},
    {L"heirarchy", false, 0,0,L"hierarchy"},
    {L"humerous", false, 0,0,L"humorous"},
    {L"hygene", false, 0,0,L"hygiene"},
    {L"hipocrit", false, 0,0,L"hypocrite"},
    {L"idenity", false, 0,0,L"identity"},
    {L"ignorence", false, 0,0,L"ignorance"},
    {L"imaginery", false, 0,0,L"imaginary"},
    {L"immitate", false, 0,0,L"imitate"},
    {L"immitation", false, 0,0,L"imitation"},
    {L"imediately", false, 0,0,L"immediately"},
    {L"incidently", false, 0,0,L"incidentally"},
    {L"independant", false, 0,0,L"independent"},
    // TODO(pwicks): This fails as a result of 13432.
    // Once that is fixed, uncomment this.
    // {L"indispensible", false, 0,0,L"indispensable"},
    {L"innoculate", false, 0,0,L"inoculate"},
    {L"inteligence", false, 0,0,L"intelligence"},
    {L"intresting", false, 0,0,L"interesting"},
    {L"interuption", false, 0,0,L"interruption"},
    {L"irrelevent", false, 0,0,L"irrelevant"},
    {L"irritible", false, 0,0,L"irritable"},
    {L"iland", false, 0,0,L"island"},
    {L"jellous", false, 0,0,L"jealous"},
    {L"knowlege", false, 0,0,L"knowledge"},
    {L"labratory", false, 0,0,L"laboratory"},
    {L"liesure", false, 0,0,L"leisure"},
    {L"lenght", false, 0,0,L"length"},
    {L"liason", false, 0,0,L"liaison"},
    {L"libary", false, 0,0,L"library"},
    {L"lisence", false, 0,0,L"license"},
    {L"lonelyness", false, 0,0,L"loneliness"},
    {L"lieing", false, 0,0,L"lying"},
    {L"maintenence", false, 0,0,L"maintenance"},
    {L"manuever", false, 0,0,L"maneuver"},
    {L"marrige", false, 0,0,L"marriage"},
    {L"mathmatics", false, 0,0,L"mathematics"},
    {L"medcine", false, 0,0,L"medicine"},
    {L"medeval", false, 0,0,L"medieval"},
    {L"momento", false, 0,0,L"memento"},
    {L"millenium", false, 0,0,L"millennium"},
    {L"miniture", false, 0,0,L"miniature"},
    {L"minite", false, 0,0,L"minute"},
    {L"mischevous", false, 0,0,L"mischievous"},
    {L"mispell", false, 0,0,L"misspell"},
    // Maybe this one should pass, as it works in hunspell, but not in firefox.
    // {L"misterius", false, 0,0,L"mysterious"},
    {L"naturaly", false, 0,0,L"naturally"},
    {L"neccessary", false, 0,0,L"necessary"},
    {L"neice", false, 0,0,L"niece"},
    {L"nieghbor", false, 0,0,L"neighbor"},
    {L"nieghbour", false, 0,0,L"neighbor"},
    {L"niether", false, 0,0,L"neither"},
    {L"noticable", false, 0,0,L"noticeable"},
    {L"occassion", false, 0,0,L"occasion"},
    {L"occasionaly", false, 0,0,L"occasionally"},
    {L"occurrance", false, 0,0,L"occurrence"},
    {L"occured", false, 0,0,L"occurred"},
    {L"oficial", false, 0,0,L"official"},
    {L"offen", false, 0,0,L"often"},
    {L"ommision", false, 0,0,L"omission"},
    {L"oprate", false, 0,0,L"operate"},
    {L"oppurtunity", false, 0,0,L"opportunity"},
    {L"orignal", false, 0,0,L"original"},
    {L"outragous", false, 0,0,L"outrageous"},
    {L"parrallel", false, 0,0,L"parallel"},
    {L"parliment", false, 0,0,L"parliament"},
    {L"particurly", false, 0,0,L"particularly"},
    {L"passtime", false, 0,0,L"pastime"},
    {L"peculier", false, 0,0,L"peculiar"},
    {L"percieve", false, 0,0,L"perceive"},
    {L"pernament", false, 0,0,L"permanent"},
    {L"perseverence", false, 0,0,L"perseverance"},
    {L"personaly", false, 0,0,L"personally"},
    {L"personell", false, 0,0,L"personnel"},
    {L"persaude", false, 0,0,L"persuade"},
    {L"pichure", false, 0,0,L"picture"},
    {L"peice", false, 0,0,L"piece"},
    {L"plagerize", false, 0,0,L"plagiarize"},
    {L"playright", false, 0,0,L"playwright"},
    {L"plesant", false, 0,0,L"pleasant"},
    {L"pollitical", false, 0,0,L"political"},
    {L"posession", false, 0,0,L"possession"},
    {L"potatos", false, 0,0,L"potatoes"},
    {L"practicle", false, 0,0,L"practical"},
    {L"preceed", false, 0,0,L"precede"},
    {L"predjudice", false, 0,0,L"prejudice"},
    {L"presance", false, 0,0,L"presence"},
    {L"privelege", false, 0,0,L"privilege"},
    // This one should probably work. It does in FF and Hunspell.
    // {L"probly", false, 0,0,L"probably"},
    {L"proffesional", false, 0,0,L"professional"},
    {L"professer", false, 0,0,L"professor"},
    {L"promiss", false, 0,0,L"promise"},
    // TODO(pwicks): This fails as a result of 13432.
    // Once that is fixed, uncomment this.
    // {L"pronounciation", false, 0,0,L"pronunciation"},
    {L"prufe", false, 0,0,L"proof"},
    {L"psycology", false, 0,0,L"psychology"},
    {L"publically", false, 0,0,L"publicly"},
    {L"quanity", false, 0,0,L"quantity"},
    {L"quarentine", false, 0,0,L"quarantine"},
    {L"questionaire", false, 0,0,L"questionnaire"},
    {L"readible", false, 0,0,L"readable"},
    {L"realy", false, 0,0,L"really"},
    {L"recieve", false, 0,0,L"receive"},
    {L"reciept", false, 0,0,L"receipt"},
    {L"reconize", false, 0,0,L"recognize"},
    {L"recomend", false, 0,0,L"recommend"},
    {L"refered", false, 0,0,L"referred"},
    {L"referance", false, 0,0,L"reference"},
    {L"relevent", false, 0,0,L"relevant"},
    {L"religous", false, 0,0,L"religious"},
    {L"repitition", false, 0,0,L"repetition"},
    {L"restarant", false, 0,0,L"restaurant"},
    {L"rythm", false, 0,0,L"rhythm"},
    {L"rediculous", false, 0,0,L"ridiculous"},
    {L"sacrefice", false, 0,0,L"sacrifice"},
    {L"saftey", false, 0,0,L"safety"},
    {L"sissors", false, 0,0,L"scissors"},
    {L"secratary", false, 0,0,L"secretary"},
    {L"sieze", false, 0,0,L"seize"},
    {L"seperate", false, 0,0,L"separate"},
    {L"sargent", false, 0,0,L"sergeant"},
    {L"shineing", false, 0,0,L"shining"},
    {L"similer", false, 0,0,L"similar"},
    {L"sinceerly", false, 0,0,L"sincerely"},
    {L"speach", false, 0,0,L"speech"},
    {L"stoping", false, 0,0,L"stopping"},
    {L"strenght", false, 0,0,L"strength"},
    {L"succede", false, 0,0,L"succeed"},
    {L"succesful", false, 0,0,L"successful"},
    {L"supercede", false, 0,0,L"supersede"},
    {L"surelly", false, 0,0,L"surely"},
    {L"suprise", false, 0,0,L"surprise"},
    {L"temperture", false, 0,0,L"temperature"},
    {L"temprary", false, 0,0,L"temporary"},
    {L"tomatos", false, 0,0,L"tomatoes"},
    {L"tommorrow", false, 0,0,L"tomorrow"},
    {L"tounge", false, 0,0,L"tongue"},
    {L"truely", false, 0,0,L"truly"},
    {L"twelth", false, 0,0,L"twelfth"},
    {L"tyrany", false, 0,0,L"tyranny"},
    {L"underate", false, 0,0,L"underrate"},
    {L"untill", false, 0,0,L"until"},
    {L"unuseual", false, 0,0,L"unusual"},
    {L"upholstry", false, 0,0,L"upholstery"},
    {L"usible", false, 0,0,L"usable"},
    {L"useing", false, 0,0,L"using"},
    {L"usualy", false, 0,0,L"usually"},
    {L"vaccuum", false, 0,0,L"vacuum"},
    {L"vegatarian", false, 0,0,L"vegetarian"},
    {L"vehical", false, 0,0,L"vehicle"},
    {L"visious", false, 0,0,L"vicious"},
    {L"villege", false, 0,0,L"village"},
    {L"wierd", false, 0,0,L"weird"},
    {L"wellcome", false, 0,0,L"welcome"},
    {L"wellfare", false, 0,0,L"welfare"},
    {L"wilfull", false, 0,0,L"willful"},
    {L"withold", false, 0,0,L"withhold"},
    {L"writting", false, 0,0,L"writing"},
#else
    {L"ello", false, 0, 0, L"hello"},
    {L"ello", false, 0, 0, L"cello"},
    {L"wate", false, 0, 0, L"water"},
    {L"wate", false, 0, 0, L"waste"},
    {L"wate", false, 0, 0, L"sate"},
    {L"wate", false, 0, 0, L"rate"},
    {L"jum", false, 0, 0, L"jump"},
    {L"jum", false, 0, 0, L"rum"},
    {L"jum", false, 0, 0, L"sum"},
    {L"jum", false, 0, 0, L"tum"},
#endif //!OS_MACOSX
    // TODO (Sidchat): add many more examples.
  };

  FilePath hunspell_directory = GetHunspellDirectory();
  ASSERT_FALSE(hunspell_directory.empty());

  scoped_refptr<SpellChecker> spell_checker(new SpellChecker(
      hunspell_directory, "en-US", NULL, FilePath()));

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(kTestCases); ++i) {
    std::vector<std::wstring> suggestions;
    size_t input_length = 0;
    if (kTestCases[i].input != NULL) {
      input_length = wcslen(kTestCases[i].input);
    }
    int misspelling_start;
    int misspelling_length;
    bool result = spell_checker->SpellCheckWord(kTestCases[i].input,
                                                static_cast<int>(input_length),
                                                &misspelling_start,
                                                &misspelling_length,
                                                &suggestions);

    // Check for spelling.
    EXPECT_EQ(kTestCases[i].expected_result, result);

    // Check if the suggested words occur.
    bool suggested_word_is_present = false;
    for (int j=0; j < static_cast<int>(suggestions.size()); j++) {
      if (suggestions.at(j).compare(kTestCases[i].suggested_word) == 0) {
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
      L"mission is to organize the world's information and make it "
      L"universally accessible and useful."
    }, {
      // English (United Kingdom)
      "en-GB",
      // L"Google's " - to be added.
      L"mission is to organize the world's information and make it "
      L"universally accessible and useful."
    }, {
      // English (United States)
      "en-US",
      L"Google's mission is to organize the world's information and make it "
      L"universally accessible and useful."
    }, {
      // Spanish
      "es-ES",
      L"La misi\x00F3n de Google es organizar la informaci\x00F3n mundial "
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
      L"\x05D4\x05E2\x05D5\x05DC\x05DD."
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
      L"A miss\x00E3o do Google \x00E9 organizar as informa\x00E7\x00F5"
      L"es do mundo todo e torn\x00E1-las acess\x00EDveis e "
      // L"\x00FAteis " - to be added.
      L"em car\x00E1ter universal."
    }, {
      // Portuguese (Portugal)
      "pt-PT",
      L"O Google tem por miss\x00E3o organizar a informa\x00E7\x00E3o do "
      L"mundo e torn\x00E1-la universalmente acess\x00EDvel e \x00FAtil"
    }, {
      // Romanian
      "ro-RO",
      L"Misiunea Google este de "
      // L"a " - to be added.
      L"organiza informa\x0163iile lumii \x015Fi de "
      // L"a " - to be added.
      L"le face accesibile \x015Fi utile la nivel universal."
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
    }, {
      // Slovak
      "sk-SK",
      L"Spolo\x010Dnos\x0165 Google si dala za \x00FAlohu usporiada\x0165 "
      L"inform\x00E1\x0063ie "
      L"z cel\x00E9ho sveta a zabezpe\x010Di\x0165, "
      L"aby boli v\x0161eobecne dostupn\x00E9 a u\x017Eito\x010Dn\x00E9."
    }, {
      // Slovenian
      "sl-SL",
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
      // Vietnamese
      "vi-VN",
      L"Nhi\x1EC7m v\x1EE5 c\x1EE7\x0061 "
      L"Google la \x0111\x1EC3 t\x1ED5 ch\x1EE9\x0063 "
      L"c\x00E1\x0063 th\x00F4ng tin c\x1EE7\x0061 "
      L"th\x1EBF gi\x1EDBi va l\x00E0m cho n\x00F3 universal c\x00F3 "
      L"th\x1EC3 truy c\x1EADp va h\x1EEFu d\x1EE5ng h\x01A1n."
    },
  };

  FilePath hunspell_directory = GetHunspellDirectory();
  ASSERT_FALSE(hunspell_directory.empty());

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(kTestCases); ++i) {
    scoped_refptr<SpellChecker> spell_checker(new SpellChecker(
        hunspell_directory, kTestCases[i].language, NULL, FilePath()));

    size_t input_length = 0;
    if (kTestCases[i].input != NULL)
      input_length = wcslen(kTestCases[i].input);

    int misspelling_start = 0;
    int misspelling_length = 0;
    bool result = spell_checker->SpellCheckWord(kTestCases[i].input,
                                                static_cast<int>(input_length),
                                                &misspelling_start,
                                                &misspelling_length, NULL);

    EXPECT_EQ(true, result);
    EXPECT_EQ(0, misspelling_start);
    EXPECT_EQ(0, misspelling_length);
  }
}

// This test Adds words to the SpellChecker and veifies that it remembers them.
TEST_F(SpellCheckTest, DISABLED_SpellCheckAddToDictionary_EN_US) {
  static const struct {
    // A string to be added to SpellChecker.
    const wchar_t* word_to_add;
  } kTestCases[] = {  // Words to be added to the SpellChecker.
    {L"Googley"},
    {L"Googleplex"},
    {L"Googler"},
  };

  FilePath custom_dictionary_file(kTempCustomDictionaryFile);
  FilePath hunspell_directory = GetHunspellDirectory();
  ASSERT_FALSE(hunspell_directory.empty());

  scoped_refptr<SpellChecker> spell_checker(new SpellChecker(
      hunspell_directory, "en-US", NULL, custom_dictionary_file));

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(kTestCases); ++i) {
    // Add the word to spellchecker.
    spell_checker->AddWord(std::wstring(kTestCases[i].word_to_add));

    // Now check whether it is added to Spellchecker.
    std::vector<std::wstring> suggestions;
    size_t input_length = 0;
    if (kTestCases[i].word_to_add != NULL) {
      input_length = wcslen(kTestCases[i].word_to_add);
    }
    int misspelling_start;
    int misspelling_length;
    bool result = spell_checker->SpellCheckWord(kTestCases[i].word_to_add,
                                                static_cast<int>(input_length),
                                                &misspelling_start,
                                                &misspelling_length,
                                                &suggestions);

    // Check for spelling.
    EXPECT_TRUE(result);
  }

  // Now initialize another spellchecker to see that AddToWord is permanent.
  scoped_refptr<SpellChecker> spell_checker_new(new SpellChecker(
      hunspell_directory, "en-US", NULL, custom_dictionary_file));

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(kTestCases); ++i) {
    // Now check whether it is added to Spellchecker.
    std::vector<std::wstring> suggestions;
    size_t input_length = 0;
    if (kTestCases[i].word_to_add != NULL) {
      input_length = wcslen(kTestCases[i].word_to_add);
    }
    int misspelling_start;
    int misspelling_length;
    bool result = spell_checker_new->SpellCheckWord(
        kTestCases[i].word_to_add,
        static_cast<int>(input_length),
        &misspelling_start,
        &misspelling_length,
        &suggestions);

    // Check for spelling.
    EXPECT_TRUE(result);
  }

  // Remove the temp custom dictionary file.
  file_util::Delete(custom_dictionary_file, false);
}

// SpellChecker should suggest custome words for misspelled words.
TEST_F(SpellCheckTest, DISABLED_SpellCheckSuggestionsAddToDictionary_EN_US) {
  static const struct {
    // A string to be added to SpellChecker.
    const wchar_t* word_to_add;
  } kTestCases[] = {  // word to be added to SpellChecker
    {L"Googley"},
    {L"Googleplex"},
    {L"Googler"},
  };

  FilePath custom_dictionary_file(kTempCustomDictionaryFile);
  FilePath hunspell_directory = GetHunspellDirectory();
  ASSERT_FALSE(hunspell_directory.empty());

  scoped_refptr<SpellChecker> spell_checker(new SpellChecker(
      hunspell_directory, "en-US", NULL, custom_dictionary_file));

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(kTestCases); ++i) {
    // Add the word to spellchecker.
    spell_checker->AddWord(std::wstring(kTestCases[i].word_to_add));
  }

  // Now check to see whether the custom words are suggested for
  // misspelled but similar words.
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
  } kTestCasesToBeTested[] = {
    {L"oogley", false, 0, 0, L"Googley"},
    {L"oogler", false, 0, 0, L"Googler"},
    {L"oogleplex", false, 0, 0, L"Googleplex"},
  };

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(kTestCasesToBeTested); ++i) {
    std::vector<std::wstring> suggestions;
    size_t input_length = 0;
    if (kTestCasesToBeTested[i].input != NULL) {
      input_length = wcslen(kTestCasesToBeTested[i].input);
    }
    int misspelling_start;
    int misspelling_length;
    bool result = spell_checker->SpellCheckWord(kTestCasesToBeTested[i].input,
                                                static_cast<int>(input_length),
                                                &misspelling_start,
                                                &misspelling_length,
                                                &suggestions);

    // Check for spelling.
    EXPECT_EQ(result, kTestCasesToBeTested[i].expected_result);

    // Check if the suggested words occur.
    bool suggested_word_is_present = false;
    for (int j=0; j < static_cast<int>(suggestions.size()); j++) {
      if (suggestions.at(j).compare(kTestCasesToBeTested[i].suggested_word) ==
                                    0) {
        suggested_word_is_present = true;
        break;
      }
    }

    EXPECT_TRUE(suggested_word_is_present);
  }

  // Remove the temp custom dictionary file.
  file_util::Delete(custom_dictionary_file, false);
}

TEST_F(SpellCheckTest, GetAutoCorrectionWord_EN_US) {
  static const struct {
    // A misspelled word.
    const wchar_t* input;

    // An expected result for this test case.
    // Should be an empty string if there are no suggestions for auto correct.
    const wchar_t* expected_result;
  } kTestCases[] = {
    {L"teh", L"the"},
    {L"moer", L"more"},
    {L"watre", L"water"},
    {L"noen", L""},
    {L"what", L""},
  };

  FilePath hunspell_directory = GetHunspellDirectory();
  ASSERT_FALSE(hunspell_directory.empty());

  scoped_refptr<SpellChecker> spell_checker(new SpellChecker(
      hunspell_directory, "en-US", NULL, FilePath()));
  spell_checker->EnableAutoSpellCorrect(true);

  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(kTestCases); ++i) {
    std::wstring misspelled_word(kTestCases[i].input);
    std::wstring expected_autocorrect_word(kTestCases[i].expected_result);
    std::wstring autocorrect_word;
    spell_checker->GetAutoCorrectionWord(misspelled_word, &autocorrect_word);

    // Check for spelling.
    EXPECT_EQ(expected_autocorrect_word, autocorrect_word);
  }
}
