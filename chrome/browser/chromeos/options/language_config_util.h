// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_OPTIONS_LANGUAGE_CONFIG_UTIL_H_
#define CHROME_BROWSER_CHROMEOS_OPTIONS_LANGUAGE_CONFIG_UTIL_H_

#include "chrome/browser/chromeos/language_preferences.h"
#include "views/controls/combobox/combobox.h"

namespace chromeos {

// The combobox model for Language input method prefs.
class LanguageComboboxModel : public ComboboxModel {
 public:
  explicit LanguageComboboxModel(
      const LanguageMultipleChoicePreference* pref_data)
          : pref_data_(pref_data), num_items_(0) {
    // Check how many items are defined in the |pref_data->values_and_ids|
    // array.
    for (size_t i = 0; i < LanguageMultipleChoicePreference::kMaxItems; ++i) {
      if ((pref_data_->values_and_ids)[i].ibus_config_value == NULL) {
        break;
      }
      ++num_items_;
    }
  }

  // Implements ComboboxModel interface.
  virtual int GetItemCount() {
    return num_items_;
  }

  // Implements ComboboxModel interface.
  virtual std::wstring GetItemAt(int index) {
    if (index < 0 || index >= num_items_) {
      LOG(ERROR) << "Index is out of bounds: " << index;
      return L"";
    }
    const int message_id = (pref_data_->values_and_ids)[index].item_message_id;
    return l10n_util::GetString(message_id);
  }

  // Gets a label for the combobox like "Input mode". This function is NOT part
  // of the ComboboxModel interface.
  std::wstring GetLabel() const {
    return l10n_util::GetString(pref_data_->label_message_id);
  }

  // Gets a config value for the ibus configuration daemon (e.g. "KUTEN_TOUTEN",
  // "KUTEN_PERIOD", ..) for an item at zero-origin |index|. This function is
  // NOT part of the ComboboxModel interface.
  std::wstring GetConfigValueAt(int index) const {
    if (index < 0 || index >= num_items_) {
      LOG(ERROR) << "Index is out of bounds: " << index;
      return L"";
    }
    return UTF8ToWide((pref_data_->values_and_ids)[index].ibus_config_value);
  }

  // Gets an index (>= 0) of an item such that GetConfigValueAt(index) is equal
  // to the |config_value|. Returns -1 if such item is not found. This function
  // is NOT part of the ComboboxModel interface.
  int GetIndexFromConfigValue(const std::wstring& config_value) const {
    for (int i = 0; i < num_items_; ++i) {
      if (GetConfigValueAt(i) == config_value) {
        return i;
      }
    }
    return -1;
  }

 private:
  const LanguageMultipleChoicePreference* pref_data_;
  int num_items_;

  DISALLOW_COPY_AND_ASSIGN(LanguageComboboxModel);
};

// The combobox for the dialog which has minimum width.
class LanguageCombobox : public views::Combobox {
 public:
  explicit LanguageCombobox(ComboboxModel* model) : Combobox(model) {
  }

  virtual gfx::Size GetPreferredSize() {
    gfx::Size size = Combobox::GetPreferredSize();
    if (size.width() < kMinComboboxWidth) {
      size.set_width(kMinComboboxWidth);
    }
    return size;
  }

 private:
  static const int kMinComboboxWidth = 100;

  DISALLOW_COPY_AND_ASSIGN(LanguageCombobox);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_OPTIONS_LANGUAGE_CONFIG_UTIL_H_
