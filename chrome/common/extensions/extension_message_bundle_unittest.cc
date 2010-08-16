// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/extension_message_bundle.h"

#include <string>
#include <vector>

#include "base/i18n/rtl.h"
#include "base/linked_ptr.h"
#include "base/scoped_ptr.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/extensions/extension_error_utils.h"
#include "chrome/common/extensions/extension_l10n_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace errors = extension_manifest_errors;

class ExtensionMessageBundleTest : public testing::Test {
 protected:
  enum BadDictionary {
    INVALID_NAME,
    NAME_NOT_A_TREE,
    EMPTY_NAME_TREE,
    MISSING_MESSAGE,
    PLACEHOLDER_NOT_A_TREE,
    EMPTY_PLACEHOLDER_TREE,
    CONTENT_MISSING,
    MESSAGE_PLACEHOLDER_DOESNT_MATCH,
  };

  // Helper method for dictionary building.
  void SetDictionary(const std::string& name,
                     DictionaryValue* subtree,
                     DictionaryValue* target) {
    target->Set(name, static_cast<Value*>(subtree));
  }

  void CreateContentTree(const std::string& name,
                         const std::string& content,
                         DictionaryValue* dict) {
    DictionaryValue* content_tree = new DictionaryValue;
    content_tree->SetString(ExtensionMessageBundle::kContentKey, content);
    SetDictionary(name, content_tree, dict);
  }

  void CreatePlaceholdersTree(DictionaryValue* dict) {
    DictionaryValue* placeholders_tree = new DictionaryValue;
    CreateContentTree("a", "A", placeholders_tree);
    CreateContentTree("b", "B", placeholders_tree);
    CreateContentTree("c", "C", placeholders_tree);
    SetDictionary(ExtensionMessageBundle::kPlaceholdersKey,
                  placeholders_tree,
                  dict);
  }

  void CreateMessageTree(const std::string& name,
                         const std::string& message,
                         bool create_placeholder_subtree,
                         DictionaryValue* dict) {
    DictionaryValue* message_tree = new DictionaryValue;
    if (create_placeholder_subtree)
      CreatePlaceholdersTree(message_tree);
    message_tree->SetString(ExtensionMessageBundle::kMessageKey, message);
    SetDictionary(name, message_tree, dict);
  }

  // Caller owns the memory.
  DictionaryValue* CreateGoodDictionary() {
    DictionaryValue* dict = new DictionaryValue;
    CreateMessageTree("n1", "message1 $a$ $b$", true, dict);
    CreateMessageTree("n2", "message2 $c$", true, dict);
    CreateMessageTree("n3", "message3", false, dict);
    return dict;
  }

  // Caller owns the memory.
  DictionaryValue* CreateBadDictionary(enum BadDictionary what_is_bad) {
    DictionaryValue* dict = CreateGoodDictionary();
    // Now remove/break things.
    switch (what_is_bad) {
      case INVALID_NAME:
        CreateMessageTree("n 5", "nevermind", false, dict);
        break;
      case NAME_NOT_A_TREE:
        dict->SetString("n4", "whatever");
        break;
      case EMPTY_NAME_TREE: {
          DictionaryValue* empty_tree = new DictionaryValue;
          SetDictionary("n4", empty_tree, dict);
        }
        break;
      case MISSING_MESSAGE:
        dict->Remove("n1.message", NULL);
        break;
      case PLACEHOLDER_NOT_A_TREE:
        dict->SetString("n1.placeholders", "whatever");
        break;
      case EMPTY_PLACEHOLDER_TREE: {
          DictionaryValue* empty_tree = new DictionaryValue;
          SetDictionary("n1.placeholders", empty_tree, dict);
        }
        break;
      case CONTENT_MISSING:
         dict->Remove("n1.placeholders.a.content", NULL);
        break;
      case MESSAGE_PLACEHOLDER_DOESNT_MATCH:
        DictionaryValue* value;
        dict->Remove("n1.placeholders.a", NULL);
        dict->GetDictionary("n1.placeholders", &value);
        CreateContentTree("x", "X", value);
        break;
    }

    return dict;
  }

  unsigned int ReservedMessagesCount() {
    // Update when adding new reserved messages.
    return 5U;
  }

  void CheckReservedMessages(ExtensionMessageBundle* handler) {
    std::string ui_locale = extension_l10n_util::CurrentLocaleOrDefault();
    EXPECT_EQ(ui_locale,
              handler->GetL10nMessage(ExtensionMessageBundle::kUILocaleKey));

    std::string text_dir = "ltr";
    if (base::i18n::GetTextDirectionForLocale(ui_locale.c_str()) ==
        base::i18n::RIGHT_TO_LEFT)
      text_dir = "rtl";

    EXPECT_EQ(text_dir, handler->GetL10nMessage(
        ExtensionMessageBundle::kBidiDirectionKey));
  }

  bool AppendReservedMessages(const std::string& application_locale) {
    std::string error;
    return handler_->AppendReservedMessagesForLocale(
        application_locale, &error);
  }

  std::string CreateMessageBundle() {
    std::string error;
    handler_.reset(ExtensionMessageBundle::Create(catalogs_, &error));

    return error;
  }

  void ClearDictionary() {
    handler_->dictionary_.clear();
  }

  scoped_ptr<ExtensionMessageBundle> handler_;
  std::vector<linked_ptr<DictionaryValue> > catalogs_;
};

TEST_F(ExtensionMessageBundleTest, ReservedMessagesCount) {
  ASSERT_EQ(5U, ReservedMessagesCount());
}

TEST_F(ExtensionMessageBundleTest, InitEmptyDictionaries) {
  CreateMessageBundle();
  EXPECT_TRUE(handler_.get() != NULL);
  EXPECT_EQ(0U + ReservedMessagesCount(), handler_->size());
  CheckReservedMessages(handler_.get());
}

TEST_F(ExtensionMessageBundleTest, InitGoodDefaultDict) {
  catalogs_.push_back(linked_ptr<DictionaryValue>(CreateGoodDictionary()));
  CreateMessageBundle();

  EXPECT_TRUE(handler_.get() != NULL);
  EXPECT_EQ(3U + ReservedMessagesCount(), handler_->size());

  EXPECT_EQ("message1 A B", handler_->GetL10nMessage("n1"));
  EXPECT_EQ("message2 C", handler_->GetL10nMessage("n2"));
  EXPECT_EQ("message3", handler_->GetL10nMessage("n3"));
  CheckReservedMessages(handler_.get());
}

TEST_F(ExtensionMessageBundleTest, InitAppDictConsultedFirst) {
  catalogs_.push_back(linked_ptr<DictionaryValue>(CreateGoodDictionary()));
  catalogs_.push_back(linked_ptr<DictionaryValue>(CreateGoodDictionary()));

  DictionaryValue* app_dict = catalogs_[0].get();
  // Flip placeholders in message of n1 tree.
  app_dict->SetString("n1.message", "message1 $b$ $a$");
  // Remove one message from app dict.
  app_dict->Remove("n2", NULL);
  // Replace n3 with N3.
  app_dict->Remove("n3", NULL);
  CreateMessageTree("N3", "message3_app_dict", false, app_dict);

  CreateMessageBundle();

  EXPECT_TRUE(handler_.get() != NULL);
  EXPECT_EQ(3U + ReservedMessagesCount(), handler_->size());

  EXPECT_EQ("message1 B A", handler_->GetL10nMessage("n1"));
  EXPECT_EQ("message2 C", handler_->GetL10nMessage("n2"));
  EXPECT_EQ("message3_app_dict", handler_->GetL10nMessage("n3"));
  CheckReservedMessages(handler_.get());
}

TEST_F(ExtensionMessageBundleTest, InitBadAppDict) {
  catalogs_.push_back(
      linked_ptr<DictionaryValue>(CreateBadDictionary(INVALID_NAME)));
  catalogs_.push_back(linked_ptr<DictionaryValue>(CreateGoodDictionary()));

  std::string error = CreateMessageBundle();

  EXPECT_TRUE(handler_.get() == NULL);
  EXPECT_EQ("Name of a key \"n 5\" is invalid. Only ASCII [a-z], "
            "[A-Z], [0-9] and \"_\" are allowed.", error);

  catalogs_[0].reset(CreateBadDictionary(NAME_NOT_A_TREE));
  handler_.reset(ExtensionMessageBundle::Create(catalogs_, &error));
  EXPECT_TRUE(handler_.get() == NULL);
  EXPECT_EQ("Not a valid tree for key n4.", error);

  catalogs_[0].reset(CreateBadDictionary(EMPTY_NAME_TREE));
  handler_.reset(ExtensionMessageBundle::Create(catalogs_, &error));
  EXPECT_TRUE(handler_.get() == NULL);
  EXPECT_EQ("There is no \"message\" element for key n4.", error);

  catalogs_[0].reset(CreateBadDictionary(MISSING_MESSAGE));
  handler_.reset(ExtensionMessageBundle::Create(catalogs_, &error));
  EXPECT_TRUE(handler_.get() == NULL);
  EXPECT_EQ("There is no \"message\" element for key n1.", error);

  catalogs_[0].reset(CreateBadDictionary(PLACEHOLDER_NOT_A_TREE));
  handler_.reset(ExtensionMessageBundle::Create(catalogs_, &error));
  EXPECT_TRUE(handler_.get() == NULL);
  EXPECT_EQ("Not a valid \"placeholders\" element for key n1.", error);

  catalogs_[0].reset(CreateBadDictionary(EMPTY_PLACEHOLDER_TREE));
  handler_.reset(ExtensionMessageBundle::Create(catalogs_, &error));
  EXPECT_TRUE(handler_.get() == NULL);
  EXPECT_EQ("Variable $a$ used but not defined.", error);

  catalogs_[0].reset(CreateBadDictionary(CONTENT_MISSING));
  handler_.reset(ExtensionMessageBundle::Create(catalogs_, &error));
  EXPECT_TRUE(handler_.get() == NULL);
  EXPECT_EQ("Invalid \"content\" element for key n1.", error);

  catalogs_[0].reset(CreateBadDictionary(MESSAGE_PLACEHOLDER_DOESNT_MATCH));
  handler_.reset(ExtensionMessageBundle::Create(catalogs_, &error));
  EXPECT_TRUE(handler_.get() == NULL);
  EXPECT_EQ("Variable $a$ used but not defined.", error);
}

TEST_F(ExtensionMessageBundleTest, ReservedMessagesOverrideDeveloperMessages) {
  catalogs_.push_back(linked_ptr<DictionaryValue>(CreateGoodDictionary()));

  DictionaryValue* dict = catalogs_[0].get();
  CreateMessageTree(ExtensionMessageBundle::kUILocaleKey, "x", false, dict);

  std::string error = CreateMessageBundle();

  EXPECT_TRUE(handler_.get() == NULL);
  std::string expected_error = ExtensionErrorUtils::FormatErrorMessage(
      errors::kReservedMessageFound, ExtensionMessageBundle::kUILocaleKey);
  EXPECT_EQ(expected_error, error);
}

TEST_F(ExtensionMessageBundleTest, AppendReservedMessagesForLTR) {
  CreateMessageBundle();

  ASSERT_TRUE(handler_.get() != NULL);
  ClearDictionary();
  ASSERT_TRUE(AppendReservedMessages("en_US"));

  EXPECT_EQ("en_US",
            handler_->GetL10nMessage(ExtensionMessageBundle::kUILocaleKey));
  EXPECT_EQ("ltr", handler_->GetL10nMessage(
      ExtensionMessageBundle::kBidiDirectionKey));
  EXPECT_EQ("rtl", handler_->GetL10nMessage(
      ExtensionMessageBundle::kBidiReversedDirectionKey));
  EXPECT_EQ("left", handler_->GetL10nMessage(
      ExtensionMessageBundle::kBidiStartEdgeKey));
  EXPECT_EQ("right", handler_->GetL10nMessage(
      ExtensionMessageBundle::kBidiEndEdgeKey));
}

TEST_F(ExtensionMessageBundleTest, AppendReservedMessagesForRTL) {
  CreateMessageBundle();

  ASSERT_TRUE(handler_.get() != NULL);
  ClearDictionary();
  ASSERT_TRUE(AppendReservedMessages("he"));

  EXPECT_EQ("he",
            handler_->GetL10nMessage(ExtensionMessageBundle::kUILocaleKey));
  EXPECT_EQ("rtl", handler_->GetL10nMessage(
      ExtensionMessageBundle::kBidiDirectionKey));
  EXPECT_EQ("ltr", handler_->GetL10nMessage(
      ExtensionMessageBundle::kBidiReversedDirectionKey));
  EXPECT_EQ("right", handler_->GetL10nMessage(
      ExtensionMessageBundle::kBidiStartEdgeKey));
  EXPECT_EQ("left", handler_->GetL10nMessage(
      ExtensionMessageBundle::kBidiEndEdgeKey));
}

TEST_F(ExtensionMessageBundleTest, IsValidNameCheckValidCharacters) {
  EXPECT_TRUE(ExtensionMessageBundle::IsValidName(std::string("a__BV_9")));
  EXPECT_TRUE(ExtensionMessageBundle::IsValidName(std::string("@@a__BV_9")));
  EXPECT_FALSE(ExtensionMessageBundle::IsValidName(std::string("$a__BV_9$")));
  EXPECT_FALSE(ExtensionMessageBundle::IsValidName(std::string("a-BV-9")));
  EXPECT_FALSE(ExtensionMessageBundle::IsValidName(std::string("a#BV!9")));
  EXPECT_FALSE(ExtensionMessageBundle::IsValidName(std::string("a<b")));
}

struct ReplaceVariables {
  const char* original;
  const char* result;
  const char* error;
  const char* begin_delimiter;
  const char* end_delimiter;
  bool pass;
};

TEST(ExtensionMessageBundle, ReplaceMessagesInText) {
  const char* kMessageBegin = ExtensionMessageBundle::kMessageBegin;
  const char* kMessageEnd = ExtensionMessageBundle::kMessageEnd;
  const char* kPlaceholderBegin = ExtensionMessageBundle::kPlaceholderBegin;
  const char* kPlaceholderEnd = ExtensionMessageBundle::kPlaceholderEnd;

  static ReplaceVariables test_cases[] = {
    // Message replacement.
    { "This is __MSG_siMPle__ message", "This is simple message",
      "", kMessageBegin, kMessageEnd, true },
    { "This is __MSG_", "This is __MSG_",
      "", kMessageBegin, kMessageEnd, true },
    { "This is __MSG__simple__ message", "This is __MSG__simple__ message",
      "Variable __MSG__simple__ used but not defined.",
      kMessageBegin, kMessageEnd, false },
    { "__MSG_LoNg__", "A pretty long replacement",
      "", kMessageBegin, kMessageEnd, true },
    { "A __MSG_SimpLE__MSG_ a", "A simpleMSG_ a",
      "", kMessageBegin, kMessageEnd, true },
    { "A __MSG_simple__MSG_long__", "A simpleMSG_long__",
      "", kMessageBegin, kMessageEnd, true },
    { "A __MSG_simple____MSG_long__", "A simpleA pretty long replacement",
      "", kMessageBegin, kMessageEnd, true },
    { "__MSG_d1g1ts_are_ok__", "I are d1g1t",
      "", kMessageBegin, kMessageEnd, true },
    // Placeholder replacement.
    { "This is $sImpLe$ message", "This is simple message",
       "", kPlaceholderBegin, kPlaceholderEnd, true },
    { "This is $", "This is $",
       "", kPlaceholderBegin, kPlaceholderEnd, true },
    { "This is $$sIMPle$ message", "This is $simple message",
       "", kPlaceholderBegin, kPlaceholderEnd, true },
    { "$LONG_V$", "A pretty long replacement",
       "", kPlaceholderBegin, kPlaceholderEnd, true },
    { "A $simple$$ a", "A simple$ a",
       "", kPlaceholderBegin, kPlaceholderEnd, true },
    { "A $simple$long_v$", "A simplelong_v$",
       "", kPlaceholderBegin, kPlaceholderEnd, true },
    { "A $simple$$long_v$", "A simpleA pretty long replacement",
       "", kPlaceholderBegin, kPlaceholderEnd, true },
    { "This is $bad name$", "This is $bad name$",
       "", kPlaceholderBegin, kPlaceholderEnd, true },
    { "This is $missing$", "This is $missing$",
       "Variable $missing$ used but not defined.",
       kPlaceholderBegin, kPlaceholderEnd, false },
  };

  ExtensionMessageBundle::SubstitutionMap messages;
  messages.insert(std::make_pair("simple", "simple"));
  messages.insert(std::make_pair("long", "A pretty long replacement"));
  messages.insert(std::make_pair("long_v", "A pretty long replacement"));
  messages.insert(std::make_pair("bad name", "Doesn't matter"));
  messages.insert(std::make_pair("d1g1ts_are_ok", "I are d1g1t"));

  for (size_t i = 0; i < arraysize(test_cases); ++i) {
    std::string text = test_cases[i].original;
    std::string error;
    EXPECT_EQ(test_cases[i].pass,
      ExtensionMessageBundle::ReplaceVariables(messages,
                                               test_cases[i].begin_delimiter,
                                               test_cases[i].end_delimiter,
                                               &text,
                                               &error));
    EXPECT_EQ(test_cases[i].result, text);
  }
}

///////////////////////////////////////////////////////////////////////////////
//
// Renderer helper functions test.
//
///////////////////////////////////////////////////////////////////////////////

TEST(GetExtensionToL10nMessagesMapTest, ReturnsTheSameObject) {
  ExtensionToL10nMessagesMap* map1 = GetExtensionToL10nMessagesMap();
  ASSERT_TRUE(NULL != map1);

  ExtensionToL10nMessagesMap* map2 = GetExtensionToL10nMessagesMap();
  ASSERT_EQ(map1, map2);
}

TEST(GetExtensionToL10nMessagesMapTest, ReturnsNullForUnknownExtensionId) {
  const std::string extension_id("some_unique_12334212314234_id");
  L10nMessagesMap* map = GetL10nMessagesMap(extension_id);
  EXPECT_TRUE(NULL == map);
}

TEST(GetExtensionToL10nMessagesMapTest, ReturnsMapForKnownExtensionId) {
  const std::string extension_id("some_unique_121212121212121_id");
  // Store a map for given id.
  L10nMessagesMap messages;
  messages.insert(std::make_pair("message_name", "message_value"));
  (*GetExtensionToL10nMessagesMap())[extension_id] = messages;

  L10nMessagesMap* map = GetL10nMessagesMap(extension_id);
  ASSERT_TRUE(NULL != map);
  EXPECT_EQ(1U, map->size());
  EXPECT_EQ("message_value", (*map)["message_name"]);
}
