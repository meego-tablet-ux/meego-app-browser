// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_EXTENSIONS_EXTENSION_MESSAGE_BUNDLE_H_
#define CHROME_COMMON_EXTENSIONS_EXTENSION_MESSAGE_BUNDLE_H_

#include <map>
#include <string>
#include <vector>

#include "base/linked_ptr.h"
#include "base/values.h"

// Contains localized extension messages for one locale. Any messages that the
// locale does not provide are pulled from the default locale.
class ExtensionMessageBundle {
 public:
  typedef std::map<std::string, std::string> SubstitutionMap;
  typedef std::vector<linked_ptr<DictionaryValue> > CatalogVector;

  // JSON keys of interest for messages file.
  static const wchar_t* kContentKey;
  static const wchar_t* kMessageKey;
  static const wchar_t* kPlaceholdersKey;

  // Begin/end markers for placeholders and messages
  static const char* kPlaceholderBegin;
  static const char* kPlaceholderEnd;
  static const char* kMessageBegin;
  static const char* kMessageEnd;

  // Reserved message names in the dictionary.
  // Update i18n documentation when adding new reserved value.
  static const char* kUILocaleKey;
  // See http://code.google.com/apis/gadgets/docs/i18n.html#BIDI for
  // description.
  // TODO(cira): point to chrome docs once they are out.
  static const char* kBidiDirectionKey;
  static const char* kBidiReversedDirectionKey;
  static const char* kBidiStartEdgeKey;
  static const char* kBidiEndEdgeKey;

  // Values for some of the reserved messages.
  static const char* kBidiLeftEdgeValue;
  static const char* kBidiRightEdgeValue;

  // Creates ExtensionMessageBundle or returns NULL if there was an error.
  // Expects locale_catalogs to be sorted from more specific to less specific,
  // with default catalog at the end.
  static ExtensionMessageBundle* Create(const CatalogVector& locale_catalogs,
                                        std::string* error);

  // Get message from the catalog with given key.
  // Returned message has all of the internal placeholders resolved to their
  // value (content).
  // Returns empty string if it can't find a message.
  // We don't use simple GetMessage name, since there is a global
  // #define GetMessage GetMessageW override in Chrome code.
  std::string GetL10nMessage(const std::string& name) const;

  // Get message from the given catalog with given key.
  static std::string GetL10nMessage(const std::string& name,
                                    const SubstitutionMap& dictionary);

  // Number of messages in the catalog.
  // Used for unittesting only.
  size_t size() const { return dictionary_.size(); }

  // Replaces all __MSG_message__ with values from the catalog.
  // Returns false if there is a message in text that's not defined in the
  // dictionary.
  bool ReplaceMessages(std::string* text, std::string* error) const;

  // Replaces each occurance of variable placeholder with its value.
  // I.e. replaces __MSG_name__ with value from the catalog with the key "name".
  // Returns false if for a valid message/placeholder name there is no matching
  // replacement.
  // Public for easier unittesting.
  static bool ReplaceVariables(const SubstitutionMap& variables,
                               const std::string& var_begin,
                               const std::string& var_end,
                               std::string* message,
                               std::string* error);

  // Getter for dictionary_.
  const SubstitutionMap* dictionary() const { return &dictionary_; }

 private:
  // Testing friend.
  friend class ExtensionMessageBundleTest;

  // Use Create to create ExtensionMessageBundle instance.
  ExtensionMessageBundle();

  // Initializes the instance from the contents of vector of catalogs.
  // If the key is not present in more specific catalog we fall back to next one
  // (less specific).
  // Returns false on error.
  bool Init(const CatalogVector& locale_catalogs, std::string* error);

  // Appends locale specific reserved messages to the dictionary.
  // Returns false if there was a conflict with user defined messages.
  bool AppendReservedMessagesForLocale(const std::string& application_locale,
                                       std::string* error);

  // Helper methods that navigate JSON tree and return simplified message.
  // They replace all $PLACEHOLDERS$ with their value, and return just key/value
  // of the message.
  bool GetMessageValue(const std::wstring& wkey,
                       const DictionaryValue& catalog,
                       std::string* value,
                       std::string* error) const;

  // Get all placeholders for a given message from JSON subtree.
  bool GetPlaceholders(const DictionaryValue& name_tree,
                       const std::string& name_key,
                       SubstitutionMap* placeholders,
                       std::string* error) const;

  // For a given message, replaces all placeholders with their actual value.
  // Returns false if replacement failed (see ReplaceVariables).
  bool ReplacePlaceholders(const SubstitutionMap& placeholders,
                           std::string* message,
                           std::string* error) const;

  // Allow only ascii 0-9, a-z, A-Z, and _ in the variable name.
  // Returns false if the input is empty or if it has illegal characters.
  template<typename str>
  static bool IsValidName(const str& name);

  // Holds all messages for application locale.
  SubstitutionMap dictionary_;
};

#endif  // CHROME_COMMON_EXTENSIONS_EXTENSION_MESSAGE_BUNDLE_H_
