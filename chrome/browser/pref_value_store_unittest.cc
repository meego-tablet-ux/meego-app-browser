// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "app/test/data/resource.h"
#include "base/scoped_ptr.h"
#include "base/values.h"
#include "chrome/browser/dummy_pref_store.h"
#include "chrome/browser/pref_value_store.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Mock;

// Names of the preferences used in this test program.
namespace prefs {
  const wchar_t kCurrentThemeID[] = L"extensions.theme.id";
  const wchar_t kDeleteCache[] = L"browser.clear_data.cache";
  const wchar_t kExtensionPref[] = L"extension.pref";
  const wchar_t kHomepage[] = L"homepage";
  const wchar_t kMaxTabs[] = L"tabs.max_tabs";
  const wchar_t kMissingPref[] = L"this.pref.does_not_exist";
  const wchar_t kRecommendedPref[] = L"this.pref.recommended_value_only";
  const wchar_t kSampleDict[] = L"sample.dict";
  const wchar_t kSampleList[] = L"sample.list";
}

// Potentailly expected values of all preferences used in this test program.
namespace user {
  const int kMaxTabsValue = 31;
  const bool kDeleteCacheValue = true;
  const std::wstring kCurrentThemeIDValue = L"abcdefg";
  const std::wstring kHomepageValue = L"http://www.google.com";
}

namespace enforced {
  const std::wstring kHomepageValue = L"http://www.topeka.com";
}

namespace extension {
  const std::wstring kCurrentThemeIDValue = L"set by extension";
  const std::wstring kHomepageValue = L"http://www.chromium.org";
}

namespace recommended {
  const int kMaxTabsValue = 10;
  const bool kRecommendedPrefValue = true;
}

class PrefValueStoreTest : public testing::Test {
 protected:
  scoped_ptr<PrefValueStore> pref_value_store_;

  // |PrefStore|s are owned by the |PrefValueStore|.
  DummyPrefStore* enforced_pref_store_;
  DummyPrefStore* extension_pref_store_;
  DummyPrefStore* recommended_pref_store_;
  DummyPrefStore* user_pref_store_;

  // Preferences are owned by the individual |DummyPrefStores|.
  DictionaryValue* enforced_prefs_;
  DictionaryValue* extension_prefs_;
  DictionaryValue* user_prefs_;
  DictionaryValue* recommended_prefs_;

  virtual void SetUp() {
    // Create dummy user preferences.
    enforced_prefs_= CreateEnforcedPrefs();
    extension_prefs_ = CreateExtensionPrefs();
    user_prefs_ = CreateUserPrefs();
    recommended_prefs_ = CreateRecommendedPrefs();

    // Create |DummyPrefStore|s.
    enforced_pref_store_ = new DummyPrefStore();
    enforced_pref_store_->set_prefs(enforced_prefs_);
    extension_pref_store_ = new DummyPrefStore();
    extension_pref_store_->set_prefs(extension_prefs_);
    user_pref_store_ = new DummyPrefStore();
    user_pref_store_->set_read_only(false);
    user_pref_store_->set_prefs(user_prefs_);
    recommended_pref_store_ = new DummyPrefStore();
    recommended_pref_store_->set_prefs(recommended_prefs_);

    // Create a new pref-value-store.
    pref_value_store_.reset(new PrefValueStore(enforced_pref_store_,
                                               extension_pref_store_,
                                               user_pref_store_,
                                               recommended_pref_store_));
  }

  // Creates a new dictionary and stores some sample user preferences
  // in it.
  DictionaryValue* CreateUserPrefs() {
    DictionaryValue* user_prefs = new DictionaryValue();
    user_prefs->SetBoolean(prefs::kDeleteCache, user::kDeleteCacheValue);
    user_prefs->SetInteger(prefs::kMaxTabs, user::kMaxTabsValue);
    user_prefs->SetString(prefs::kCurrentThemeID, user::kCurrentThemeIDValue);
    user_prefs->SetString(prefs::kHomepage, user::kHomepageValue);
    return user_prefs;
  }

  DictionaryValue* CreateEnforcedPrefs() {
    DictionaryValue* enforced_prefs = new DictionaryValue();
    enforced_prefs->SetString(prefs::kHomepage, enforced::kHomepageValue);
    return enforced_prefs;
  }

  DictionaryValue* CreateExtensionPrefs() {
    DictionaryValue* extension_prefs = new DictionaryValue();
    extension_prefs->SetString(prefs::kCurrentThemeID,
        extension::kCurrentThemeIDValue);
    extension_prefs->SetString(prefs::kHomepage, extension::kHomepageValue);
    return extension_prefs;
  }

  DictionaryValue* CreateRecommendedPrefs() {
    DictionaryValue* recommended_prefs = new DictionaryValue();
    recommended_prefs->SetInteger(prefs::kMaxTabs, recommended::kMaxTabsValue);
    recommended_prefs->SetBoolean(
        prefs::kRecommendedPref,
        recommended::kRecommendedPrefValue);
    return recommended_prefs;  }

  DictionaryValue* CreateSampleDictValue() {
    DictionaryValue* sample_dict = new DictionaryValue();
    sample_dict->SetBoolean(L"issample", true);
    sample_dict->SetInteger(L"value", 4);
    sample_dict->SetString(L"descr", L"Sample Test Dictionary");
    return sample_dict;
  }

  ListValue* CreateSampleListValue() {
    ListValue* sample_list = new ListValue();
    sample_list->Set(0, Value::CreateIntegerValue(0));
    sample_list->Set(1, Value::CreateIntegerValue(1));
    sample_list->Set(2, Value::CreateIntegerValue(2));
    sample_list->Set(3, Value::CreateIntegerValue(3));
    return sample_list;
  }

  virtual void TearDown() {}
};


TEST_F(PrefValueStoreTest, IsReadOnly) {
  enforced_pref_store_->set_read_only(true);
  extension_pref_store_->set_read_only(true);
  user_pref_store_->set_read_only(true);
  recommended_pref_store_->set_read_only(true);
  EXPECT_TRUE(pref_value_store_->ReadOnly());

  user_pref_store_->set_read_only(false);
  EXPECT_FALSE(pref_value_store_->ReadOnly());
}

TEST_F(PrefValueStoreTest, GetValue) {
  Value* value;

  // Test getting an enforced value overwriting a user-defined and
  // extension-defined value.
  value = NULL;
  ASSERT_TRUE(pref_value_store_->GetValue(prefs::kHomepage, &value));
  std::wstring actual_str_value;
  EXPECT_TRUE(value->GetAsString(&actual_str_value));
  EXPECT_EQ(enforced::kHomepageValue, actual_str_value);

  // Test getting an extension value overwriting a user-defined value.
  value = NULL;
  ASSERT_TRUE(pref_value_store_->GetValue(prefs::kCurrentThemeID, &value));
  EXPECT_TRUE(value->GetAsString(&actual_str_value));
  EXPECT_EQ(extension::kCurrentThemeIDValue, actual_str_value);

  // Test getting a user-set value.
  value = NULL;
  ASSERT_TRUE(pref_value_store_->GetValue(prefs::kDeleteCache, &value));
  bool actual_bool_value = false;
  EXPECT_TRUE(value->GetAsBoolean(&actual_bool_value));
  EXPECT_EQ(user::kDeleteCacheValue, actual_bool_value);

  // Test getting a user set value overwriting a recommended value.
  value = NULL;
  ASSERT_TRUE(pref_value_store_->GetValue(prefs::kMaxTabs, &value));
  int actual_int_value = -1;
  EXPECT_TRUE(value->GetAsInteger(&actual_int_value));
  EXPECT_EQ(user::kMaxTabsValue, actual_int_value);

  // Test getting a recommended value.
  value = NULL;
  ASSERT_TRUE(pref_value_store_->GetValue(prefs::kRecommendedPref, &value));
  actual_bool_value = false;
  EXPECT_TRUE(value->GetAsBoolean(&actual_bool_value));
  EXPECT_EQ(recommended::kRecommendedPrefValue, actual_bool_value);

  // Test getting a preference value that the |PrefValueStore|
  // does not contain.
  FundamentalValue tmp_dummy_value(true);
  Value* v_null = &tmp_dummy_value;
  ASSERT_FALSE(pref_value_store_->GetValue(prefs::kMissingPref, &v_null));
  ASSERT_TRUE(v_null == NULL);
}

TEST_F(PrefValueStoreTest, HasPrefPath) {
  // Enforced preference
  EXPECT_TRUE(pref_value_store_->HasPrefPath(prefs::kHomepage));
  // User preference
  EXPECT_TRUE(pref_value_store_->HasPrefPath(prefs::kDeleteCache));
  // Recommended preference
  EXPECT_TRUE(pref_value_store_->HasPrefPath(prefs::kRecommendedPref));
  // Unknown preference
  EXPECT_FALSE(pref_value_store_->HasPrefPath(prefs::kMissingPref));
}

TEST_F(PrefValueStoreTest, ReadPrefs) {
  pref_value_store_->ReadPrefs();
  // The ReadPrefs method of the |DummyPrefStore| deletes the |pref_store|s
  // internal dictionary and creates a new empty dictionary. Hence this
  // dictionary does not contain any of the preloaded preferences.
  // This shows that the ReadPrefs method of the |DummyPrefStore| was called.
  EXPECT_FALSE(pref_value_store_->HasPrefPath(prefs::kDeleteCache));
}

TEST_F(PrefValueStoreTest, WritePrefs) {
  user_pref_store_->set_prefs_written(false);
  pref_value_store_->WritePrefs();
  ASSERT_TRUE(user_pref_store_->get_prefs_written());
}

TEST_F(PrefValueStoreTest, SetUserPrefValue) {
  Value* new_value = NULL;
  Value* actual_value = NULL;

  // Test that enforced values can not be set.
  ASSERT_TRUE(pref_value_store_->PrefValueIsManaged(prefs::kHomepage));
  // The Ownership is tranfered to |PrefValueStore|.
  new_value = Value::CreateStringValue(L"http://www.youtube.com");
  pref_value_store_->SetUserPrefValue(prefs::kHomepage, new_value);

  ASSERT_TRUE(pref_value_store_->GetValue(prefs::kHomepage, &actual_value));
  std::wstring value_str;
  actual_value->GetAsString(&value_str);
  ASSERT_EQ(enforced::kHomepageValue, value_str);

  // User preferences values can be set
  ASSERT_FALSE(pref_value_store_->PrefValueIsManaged(prefs::kMaxTabs));
  actual_value = NULL;
  pref_value_store_->GetValue(prefs::kMaxTabs, &actual_value);
  int int_value;
  EXPECT_TRUE(actual_value->GetAsInteger(&int_value));
  EXPECT_EQ(user::kMaxTabsValue, int_value);

  new_value = Value::CreateIntegerValue(1);
  pref_value_store_->SetUserPrefValue(prefs::kMaxTabs, new_value);
  actual_value = NULL;
  pref_value_store_->GetValue(prefs::kMaxTabs, &actual_value);
  EXPECT_TRUE(new_value->Equals(actual_value));

  // Set and Get |DictionaryValue|
  DictionaryValue* expected_dict_value = CreateSampleDictValue();
  pref_value_store_->SetUserPrefValue(prefs::kSampleDict, expected_dict_value);

  actual_value = NULL;
  std::wstring key(prefs::kSampleDict);
  pref_value_store_->GetValue(key , &actual_value);

  ASSERT_EQ(expected_dict_value, actual_value);
  ASSERT_TRUE(expected_dict_value->Equals(actual_value));

  // Set and Get a |ListValue|
  ListValue* expected_list_value = CreateSampleListValue();
  pref_value_store_->SetUserPrefValue(prefs::kSampleList, expected_list_value);

  actual_value = NULL;
  key = prefs::kSampleList;
  pref_value_store_->GetValue(key, &actual_value);

  ASSERT_EQ(expected_list_value, actual_value);
  ASSERT_TRUE(expected_list_value->Equals(actual_value));
}

TEST_F(PrefValueStoreTest, PrefValueIsManaged) {
  // Test an enforced preference.
  ASSERT_TRUE(pref_value_store_->HasPrefPath(prefs::kHomepage));
  EXPECT_TRUE(pref_value_store_->PrefValueIsManaged(prefs::kHomepage));

  // Test a user preference.
  ASSERT_TRUE(pref_value_store_->HasPrefPath(prefs::kMaxTabs));
  EXPECT_FALSE(pref_value_store_->PrefValueIsManaged(prefs::kMaxTabs));

  // Test a preference from the recommended pref store.
  ASSERT_TRUE(pref_value_store_->HasPrefPath(prefs::kRecommendedPref));
  EXPECT_FALSE(pref_value_store_->PrefValueIsManaged(prefs::kRecommendedPref));

  // Test a preference for which the PrefValueStore does not contain a value.
  ASSERT_FALSE(pref_value_store_->HasPrefPath(prefs::kMissingPref));
  EXPECT_FALSE(pref_value_store_->PrefValueIsManaged(prefs::kMissingPref));
}

