// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/extension.h"

#include "app/resource_bundle.h"
#include "base/basictypes.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/string_util.h"
#include "base/stl_util-inl.h"
#include "base/third_party/nss/blapi.h"
#include "base/third_party/nss/sha256.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/extensions/extension_error_reporter.h"
#include "chrome/common/extensions/extension_error_utils.h"
#include "chrome/common/extensions/user_script.h"
#include "chrome/common/notification_service.h"
#include "chrome/common/url_constants.h"
#include "net/base/base64.h"
#include "net/base/net_util.h"

#if defined(OS_WIN)
#include "base/registry.h"
#endif

namespace keys = extension_manifest_keys;
namespace values = extension_manifest_values;
namespace errors = extension_manifest_errors;

namespace {
const int kPEMOutputColumns = 65;

// KEY MARKERS
const char kKeyBeginHeaderMarker[] = "-----BEGIN";
const char kKeyBeginFooterMarker[] = "-----END";
const char kKeyInfoEndMarker[] = "KEY-----";
const char kPublic[] = "PUBLIC";
const char kPrivate[] = "PRIVATE";

const int kRSAKeySize = 1024;

// Converts a normal hexadecimal string into the alphabet used by extensions.
// We use the characters 'a'-'p' instead of '0'-'f' to avoid ever having a
// completely numeric host, since some software interprets that as an IP
// address.
static void ConvertHexadecimalToIDAlphabet(std::string* id) {
  for (size_t i = 0; i < id->size(); ++i)
    (*id)[i] = HexStringToInt(id->substr(i, 1)) + 'a';
}

// Returns true if the given string is an API permission (see kPermissionNames).
static bool IsAPIPermission(const std::string& str) {
  for (size_t i = 0; i < Extension::kNumPermissions; ++i) {
    if (str == Extension::kPermissionNames[i])
      return true;
  }

  return false;
}

}  // namespace

// static
int Extension::id_counter_ = 0;

const char Extension::kManifestFilename[] = "manifest.json";
const char Extension::kLocaleFolder[] = "_locales";
const char Extension::kMessagesFilename[] = "messages.json";

// A list of all the keys allowed by themes.
static const wchar_t* kValidThemeKeys[] = {
  keys::kDescription,
  keys::kName,
  keys::kPublicKey,
  keys::kSignature,
  keys::kTheme,
  keys::kVersion,
  keys::kUpdateURL
};

#if defined(OS_WIN)
const char* Extension::kExtensionRegistryPath =
    "Software\\Google\\Chrome\\Extensions";
#endif

// first 16 bytes of SHA256 hashed public key.
const size_t Extension::kIdSize = 16;

const char Extension::kMimeType[] = "application/x-chrome-extension";

const int Extension::kIconSizes[] = {
  EXTENSION_ICON_LARGE,
  EXTENSION_ICON_MEDIUM,
  EXTENSION_ICON_SMALL,
  EXTENSION_ICON_BITTY
};

const char* Extension::kPermissionNames[] = {
  "tabs",
  "bookmarks",
};
const size_t Extension::kNumPermissions =
    arraysize(Extension::kPermissionNames);

Extension::~Extension() {
  STLDeleteValues(&page_actions_);
}

const std::string Extension::VersionString() const {
  return version_->GetString();
}

// static
bool Extension::IsExtension(const FilePath& file_name) {
  return file_name.MatchesExtension(
      FilePath::StringType(FILE_PATH_LITERAL(".")) +
      chrome::kExtensionFileExtension);
}

// static
bool Extension::IdIsValid(const std::string& id) {
  // Verify that the id is legal.
  if (id.size() != (kIdSize * 2))
    return false;

  // We only support lowercase IDs, because IDs can be used as URL components
  // (where GURL will lowercase it).
  std::string temp = StringToLowerASCII(id);
  for (size_t i = 0; i < temp.size(); i++)
    if (temp[i] < 'a' || temp[i] > 'p')
      return false;

  return true;
}

// static
GURL Extension::GetResourceURL(const GURL& extension_url,
                               const std::string& relative_path) {
  DCHECK(extension_url.SchemeIs(chrome::kExtensionScheme));
  DCHECK(extension_url.path() == "/");

  GURL ret_val = GURL(extension_url.spec() + relative_path);
  DCHECK(StartsWithASCII(ret_val.spec(), extension_url.spec(), false));

  return ret_val;
}

const ExtensionAction* Extension::GetExtensionAction(
    std::string id, ExtensionAction::ExtensionActionType action_type) const {
  if (action_type == ExtensionAction::BROWSER_ACTION) {
    DCHECK(id.empty());  // Multiple browser actions are not allowed.
    return browser_action_.get();
  } else {
    ExtensionActionMap::const_iterator it = page_actions_.find(id);
    if (it == page_actions_.end())
      return NULL;

    return it->second;
  }
}

Extension::Location Extension::ExternalExtensionInstallType(
    std::string registry_path) {
#if defined(OS_WIN)
  HKEY reg_root = HKEY_LOCAL_MACHINE;
  RegKey key;
  registry_path.append("\\");
  registry_path.append(id_);
  if (key.Open(reg_root, ASCIIToWide(registry_path).c_str()))
    return Extension::EXTERNAL_REGISTRY;
#endif
  return Extension::EXTERNAL_PREF;
}

bool Extension::GenerateIdFromPublicKey(const std::string& input,
                                        std::string* output) {
  CHECK(output);
  if (input.length() == 0)
    return false;

  const uint8* ubuf = reinterpret_cast<const unsigned char*>(input.data());
  SHA256Context ctx;
  SHA256_Begin(&ctx);
  SHA256_Update(&ctx, ubuf, input.length());
  uint8 hash[Extension::kIdSize];
  SHA256_End(&ctx, hash, NULL, sizeof(hash));
  *output = StringToLowerASCII(HexEncode(hash, sizeof(hash)));
  ConvertHexadecimalToIDAlphabet(output);

  return true;
}

// Helper method that loads a UserScript object from a dictionary in the
// content_script list of the manifest.
bool Extension::LoadUserScriptHelper(const DictionaryValue* content_script,
                                     int definition_index, std::string* error,
                                     UserScript* result) {
  // run_at
  if (content_script->HasKey(keys::kRunAt)) {
    std::string run_location;
    if (!content_script->GetString(keys::kRunAt, &run_location)) {
      *error = ExtensionErrorUtils::FormatErrorMessage(errors::kInvalidRunAt,
          IntToString(definition_index));
      return false;
    }

    if (run_location == values::kRunAtDocumentStart) {
      result->set_run_location(UserScript::DOCUMENT_START);
    } else if (run_location == values::kRunAtDocumentEnd) {
      result->set_run_location(UserScript::DOCUMENT_END);
    } else {
      *error = ExtensionErrorUtils::FormatErrorMessage(errors::kInvalidRunAt,
          IntToString(definition_index));
      return false;
    }
  }

  // matches
  ListValue* matches = NULL;
  if (!content_script->GetList(keys::kMatches, &matches)) {
    *error = ExtensionErrorUtils::FormatErrorMessage(errors::kInvalidMatches,
        IntToString(definition_index));
    return false;
  }

  if (matches->GetSize() == 0) {
    *error = ExtensionErrorUtils::FormatErrorMessage(errors::kInvalidMatchCount,
        IntToString(definition_index));
    return false;
  }
  for (size_t j = 0; j < matches->GetSize(); ++j) {
    std::string match_str;
    if (!matches->GetString(j, &match_str)) {
      *error = ExtensionErrorUtils::FormatErrorMessage(errors::kInvalidMatch,
          IntToString(definition_index), IntToString(j));
      return false;
    }

    URLPattern pattern;
    if (!pattern.Parse(match_str)) {
      *error = ExtensionErrorUtils::FormatErrorMessage(errors::kInvalidMatch,
          IntToString(definition_index), IntToString(j));
      return false;
    }

    result->add_url_pattern(pattern);
  }

  // js and css keys
  ListValue* js = NULL;
  if (content_script->HasKey(keys::kJs) &&
      !content_script->GetList(keys::kJs, &js)) {
    *error = ExtensionErrorUtils::FormatErrorMessage(errors::kInvalidJsList,
        IntToString(definition_index));
    return false;
  }

  ListValue* css = NULL;
  if (content_script->HasKey(keys::kCss) &&
      !content_script->GetList(keys::kCss, &css)) {
    *error = ExtensionErrorUtils::FormatErrorMessage(errors::kInvalidCssList,
        IntToString(definition_index));
    return false;
  }

  // The manifest needs to have at least one js or css user script definition.
  if (((js ? js->GetSize() : 0) + (css ? css->GetSize() : 0)) == 0) {
    *error = ExtensionErrorUtils::FormatErrorMessage(errors::kMissingFile,
        IntToString(definition_index));
    return false;
  }

  if (js) {
    for (size_t script_index = 0; script_index < js->GetSize();
         ++script_index) {
      Value* value;
      std::wstring relative;
      if (!js->Get(script_index, &value) || !value->GetAsString(&relative)) {
        *error = ExtensionErrorUtils::FormatErrorMessage(errors::kInvalidJs,
            IntToString(definition_index), IntToString(script_index));
        return false;
      }
      // TODO(georged): Make GetResourceURL accept wstring too
      GURL url = GetResourceURL(WideToUTF8(relative));
      FilePath path = GetResourcePath(WideToUTF8(relative));
      result->js_scripts().push_back(UserScript::File(path, url));
    }
  }

  if (css) {
    for (size_t script_index = 0; script_index < css->GetSize();
         ++script_index) {
      Value* value;
      std::wstring relative;
      if (!css->Get(script_index, &value) || !value->GetAsString(&relative)) {
        *error = ExtensionErrorUtils::FormatErrorMessage(errors::kInvalidCss,
            IntToString(definition_index), IntToString(script_index));
        return false;
      }
      // TODO(georged): Make GetResourceURL accept wstring too
      GURL url = GetResourceURL(WideToUTF8(relative));
      FilePath path = GetResourcePath(WideToUTF8(relative));
      result->css_scripts().push_back(UserScript::File(path, url));
    }
  }

  return true;
}

// Helper method that loads a PageAction or BrowserAction object from a
// dictionary in the page_actions list or browser_action key of the manifest.
ExtensionAction* Extension::LoadExtensionActionHelper(
    const DictionaryValue* page_action, int definition_index,
    std::string* error, ExtensionAction::ExtensionActionType action_type) {
  scoped_ptr<ExtensionAction> result(new ExtensionAction());
  result->set_extension_id(id());
  result->set_type(action_type);

  ListValue* icons = NULL;
  // Read the page action |icons|.
  if (!page_action->HasKey(keys::kPageActionIcons) ||
      !page_action->GetList(keys::kPageActionIcons, &icons) ||
      icons->GetSize() == 0) {
    // Icons are only required for page actions.
    if (action_type == ExtensionAction::PAGE_ACTION) {
      *error = ExtensionErrorUtils::FormatErrorMessage(
          errors::kInvalidPageActionIconPaths, IntToString(definition_index));
      return NULL;
    }
  }

  int icon_count = 0;
  if (icons) {
    for (ListValue::const_iterator iter = icons->begin();
         iter != icons->end(); ++iter) {
      std::string path;
      if (!(*iter)->GetAsString(&path) || path.empty()) {
        *error = ExtensionErrorUtils::FormatErrorMessage(
            errors::kInvalidPageActionIconPath,
            IntToString(definition_index), IntToString(icon_count));
        return NULL;
      }

      result->AddIconPath(path);
      ++icon_count;
    }
  }

  if (action_type == ExtensionAction::BROWSER_ACTION) {
    result->set_id("");  // Not needed (only 1 browser action per extension).
  } else {
    // Read the page action |id|.
    std::string id;
    if (!page_action->GetString(keys::kPageActionId, &id)) {
      *error = ExtensionErrorUtils::FormatErrorMessage(
          errors::kInvalidPageActionId, IntToString(definition_index));
      return NULL;
    }
    result->set_id(id);
  }

  // Read the page action |name|.
  std::string name;
  if (!page_action->GetString(keys::kName, &name)) {
    *error = ExtensionErrorUtils::FormatErrorMessage(errors::kInvalidName,
        IntToString(definition_index));
    return NULL;
  }
  result->set_name(name);

  // Read the action's |popup| (optional).
  DictionaryValue* popup = NULL;
  if (page_action->HasKey(keys::kPageActionPopup) &&
     !page_action->GetDictionary(keys::kPageActionPopup, &popup)) {
    *error = errors::kInvalidPageActionPopup;
    return NULL;
  }
  if (popup) {
    std::string url_str;
    if (!popup->GetString(keys::kPageActionPopupPath, &url_str)) {
      *error = ExtensionErrorUtils::FormatErrorMessage(
          errors::kInvalidPageActionPopupPath, "<missing>");
      return NULL;
    }
    GURL url = GetResourceURL(url_str);
    if (!url.is_valid()) {
      *error = ExtensionErrorUtils::FormatErrorMessage(
          errors::kInvalidPageActionPopupPath, url_str);
      return NULL;
    }
    result->set_popup_url(url);

    int height;
    if (!popup->GetInteger(keys::kPageActionPopupHeight, &height)) {
      *error = ExtensionErrorUtils::FormatErrorMessage(
          errors::kInvalidPageActionPopupHeight, "<missing>");
      return NULL;
    }
    result->set_popup_height(height);
  }

  return result.release();
}

bool Extension::ContainsNonThemeKeys(const DictionaryValue& source) {
  // Generate a map of allowable keys
  static std::map<std::wstring, bool> theme_keys;
  static bool theme_key_mapped = false;
  if (!theme_key_mapped) {
    for (size_t i = 0; i < arraysize(kValidThemeKeys); ++i) {
      theme_keys[kValidThemeKeys[i]] = true;
    }
    theme_key_mapped = true;
  }

  // Go through all the root level keys and verify that they're in the map
  // of keys allowable by themes. If they're not, then make a not of it for
  // later.
  DictionaryValue::key_iterator iter = source.begin_keys();
  while (iter != source.end_keys()) {
    std::wstring key = (*iter);
    if (theme_keys.find(key) == theme_keys.end())
      return true;
    ++iter;
  }
  return false;
}

// static
FilePath Extension::GetResourcePath(const FilePath& extension_path,
                                    const std::string& relative_path) {
  // Build up a file:// URL and convert that back to a FilePath.  This avoids
  // URL encoding and path separator issues.

  // Convert the extension's root to a file:// URL.
  GURL extension_url = net::FilePathToFileURL(extension_path);
  if (!extension_url.is_valid())
    return FilePath();

  // Append the requested path.
  GURL::Replacements replacements;
  std::string new_path(extension_url.path());
  new_path += "/";
  new_path += relative_path;
  replacements.SetPathStr(new_path);
  GURL file_url = extension_url.ReplaceComponents(replacements);
  if (!file_url.is_valid())
    return FilePath();

  // Convert the result back to a FilePath.
  FilePath ret_val;
  if (!net::FileURLToFilePath(file_url, &ret_val))
    return FilePath();

  // Double-check that the path we ended up with is actually inside the
  // extension root.
  if (!extension_path.IsParent(ret_val))
    return FilePath();

  return ret_val;
}

Extension::Extension(const FilePath& path)
    : is_theme_(false), background_page_ready_(false) {
  DCHECK(path.IsAbsolute());
  location_ = INVALID;

#if defined(OS_WIN)
  // Normalize any drive letter to upper-case. We do this for consistency with
  // net_utils::FilePathToFileURL(), which does the same thing, to make string
  // comparisons simpler.
  std::wstring path_str = path.value();
  if (path_str.size() >= 2 && path_str[0] >= L'a' && path_str[0] <= L'z' &&
      path_str[1] == ':')
    path_str[0] += ('A' - 'a');

  path_ = FilePath(path_str);
#else
  path_ = path;
#endif
}

// TODO(rafaelw): Move ParsePEMKeyBytes, ProducePEM & FormatPEMForOutput to a
// util class in base:
// http://code.google.com/p/chromium/issues/detail?id=13572
bool Extension::ParsePEMKeyBytes(const std::string& input,
                                 std::string* output) {
  DCHECK(output);
  if (!output)
    return false;
  if (input.length() == 0)
    return false;

  std::string working = input;
  if (StartsWithASCII(working, kKeyBeginHeaderMarker, true)) {
    working = CollapseWhitespaceASCII(working, true);
    size_t header_pos = working.find(kKeyInfoEndMarker,
      sizeof(kKeyBeginHeaderMarker) - 1);
    if (header_pos == std::string::npos)
      return false;
    size_t start_pos = header_pos + sizeof(kKeyInfoEndMarker) - 1;
    size_t end_pos = working.rfind(kKeyBeginFooterMarker);
    if (end_pos == std::string::npos)
      return false;
    if (start_pos >= end_pos)
      return false;

    working = working.substr(start_pos, end_pos - start_pos);
    if (working.length() == 0)
      return false;
  }

  return net::Base64Decode(working, output);
}

bool Extension::ProducePEM(const std::string& input,
                                  std::string* output) {
  CHECK(output);
  if (input.length() == 0)
    return false;

  return net::Base64Encode(input, output);
}

bool Extension::FormatPEMForFileOutput(const std::string input,
                                       std::string* output,
                                       bool is_public) {
  CHECK(output);
  if (input.length() == 0)
    return false;
  *output = "";
  output->append(kKeyBeginHeaderMarker);
  output->append(" ");
  output->append(is_public ? kPublic : kPrivate);
  output->append(" ");
  output->append(kKeyInfoEndMarker);
  output->append("\n");
  for (size_t i = 0; i < input.length(); ) {
    int slice = std::min<int>(input.length() - i, kPEMOutputColumns);
    output->append(input.substr(i, slice));
    output->append("\n");
    i += slice;
  }
  output->append(kKeyBeginFooterMarker);
  output->append(" ");
  output->append(is_public ? kPublic : kPrivate);
  output->append(" ");
  output->append(kKeyInfoEndMarker);
  output->append("\n");

  return true;
}

// static
// TODO(aa): A problem with this code is that we silently allow upgrades to
// extensions that require less permissions than the current version, but then
// we don't silently allow them to go back. In order to fix this, we would need
// to remember the max set of permissions we ever granted a single extension.
bool Extension::IsPrivilegeIncrease(Extension* old_extension,
                                    Extension* new_extension) {
  // If the old extension had native code access, we don't need to go any
  // further. Things can't get any worse.
  if (old_extension->plugins().size() > 0)
    return false;

  // Otherwise, if the new extension has a plugin, it's a privilege increase.
  if (new_extension->plugins().size() > 0)
    return true;

  // If we are increasing the set of hosts we have access to, it's a privilege
  // increase.
  if (!old_extension->HasAccessToAllHosts()) {
    if (new_extension->HasAccessToAllHosts())
      return true;

    std::set<std::string> old_hosts =
        old_extension->GetEffectiveHostPermissions();
    std::set<std::string> new_hosts =
        new_extension->GetEffectiveHostPermissions();

    std::set<std::string> difference;
    std::set_difference(new_hosts.begin(), new_hosts.end(),
                        old_hosts.begin(), old_hosts.end(),
                        std::insert_iterator<std::set<std::string> >(
                            difference, difference.end()));
    if (difference.size() > 0)
      return true;
  }

  // If we're going from not having api permissions to having them, it's a
  // privilege increase.
  if (old_extension->api_permissions().size() == 0 &&
      new_extension->api_permissions().size() > 0)
    return true;

  // Nothing much has changed.
  return false;
}

bool Extension::InitFromValue(const DictionaryValue& source, bool require_id,
                              std::string* error) {
  if (source.HasKey(keys::kPublicKey)) {
    std::string public_key_bytes;
    if (!source.GetString(keys::kPublicKey, &public_key_) ||
      !ParsePEMKeyBytes(public_key_, &public_key_bytes) ||
      !GenerateIdFromPublicKey(public_key_bytes, &id_)) {
        *error = errors::kInvalidKey;
        return false;
    }
  } else if (require_id) {
    *error = errors::kInvalidKey;
    return false;
  } else {
    // Generate a random ID
    id_ = StringPrintf("%x", NextGeneratedId());

    // pad the string out to kIdSize*2 chars with zeroes.
    id_.insert(0, Extension::kIdSize*2 - id_.length(), '0');

    // Convert to our mp-decimal.
    ConvertHexadecimalToIDAlphabet(&id_);
  }

  // Make a copy of the manifest so we can store it in prefs.
  manifest_value_.reset(static_cast<DictionaryValue*>(source.DeepCopy()));

  // Initialize the URL.
  extension_url_ = GURL(std::string(chrome::kExtensionScheme) +
                        chrome::kStandardSchemeSeparator + id_ + "/");

  // Initialize version.
  std::string version_str;
  if (!source.GetString(keys::kVersion, &version_str)) {
    *error = errors::kInvalidVersion;
    return false;
  }
  version_.reset(Version::GetVersionFromString(version_str));
  if (!version_.get() || version_->components().size() > 4) {
    *error = errors::kInvalidVersion;
    return false;
  }

  // Initialize & localize name.
  if (!source.GetString(keys::kName, &name_)) {
    *error = errors::kInvalidName;
    return false;
  } else if (message_bundle_.get()) {
    std::string l10n_name =
      message_bundle_->GetL10nMessage(ExtensionMessageBundle::kExtensionName);
    if (!l10n_name.empty())
      name_ = l10n_name;
  }

  // Initialize & localize description (if present).
  if (source.HasKey(keys::kDescription)) {
    if (!source.GetString(keys::kDescription, &description_)) {
      *error = errors::kInvalidDescription;
      return false;
    } else if (message_bundle_.get()) {
      std::string l10n_description = message_bundle_->GetL10nMessage(
          ExtensionMessageBundle::kExtensionDescription);
      if (!l10n_description.empty())
        description_ = l10n_description;
    }
  }

  // Initialize update url (if present).
  if (source.HasKey(keys::kUpdateURL)) {
    std::string tmp;
    if (!source.GetString(keys::kUpdateURL, &tmp)) {
      *error = ExtensionErrorUtils::FormatErrorMessage(
          errors::kInvalidUpdateURL, "");
      return false;
    }
    update_url_ = GURL(tmp);
    if (!update_url_.is_valid() || update_url_.has_ref()) {
      *error = ExtensionErrorUtils::FormatErrorMessage(
          errors::kInvalidUpdateURL, tmp);
      return false;
    }
  }

  // Initialize icons (if present).
  if (source.HasKey(keys::kIcons)) {
    DictionaryValue* icons_value = NULL;
    if (!source.GetDictionary(keys::kIcons, &icons_value)) {
      *error = errors::kInvalidIcons;
      return false;
    }

    for (size_t i = 0; i < arraysize(kIconSizes); ++i) {
      std::wstring key = ASCIIToWide(IntToString(kIconSizes[i]));
      if (icons_value->HasKey(key)) {
        std::string icon_path;
        if (!icons_value->GetString(key, &icon_path)) {
          *error = ExtensionErrorUtils::FormatErrorMessage(
              errors::kInvalidIconPath, WideToASCII(key));
          return false;
        }
        icons_[kIconSizes[i]] = icon_path;
      }
    }
  }

  // Initialize themes (if present).
  is_theme_ = false;
  if (source.HasKey(keys::kTheme)) {
    // Themes cannot contain extension keys.
    if (ContainsNonThemeKeys(source)) {
      *error = errors::kThemesCannotContainExtensions;
      return false;
    }

    DictionaryValue* theme_value;
    if (!source.GetDictionary(keys::kTheme, &theme_value)) {
      *error = errors::kInvalidTheme;
      return false;
    }
    is_theme_ = true;

    DictionaryValue* images_value;
    if (theme_value->GetDictionary(keys::kThemeImages, &images_value)) {
      // Validate that the images are all strings
      DictionaryValue::key_iterator iter = images_value->begin_keys();
      while (iter != images_value->end_keys()) {
        std::string val;
        if (!images_value->GetString(*iter, &val)) {
          *error = errors::kInvalidThemeImages;
          return false;
        }
        ++iter;
      }
      theme_images_.reset(
          static_cast<DictionaryValue*>(images_value->DeepCopy()));
    }

    DictionaryValue* colors_value;
    if (theme_value->GetDictionary(keys::kThemeColors, &colors_value)) {
      // Validate that the colors are all three-item lists
      DictionaryValue::key_iterator iter = colors_value->begin_keys();
      while (iter != colors_value->end_keys()) {
        std::string val;
        int color = 0;
        ListValue* color_list;
        if (colors_value->GetList(*iter, &color_list)) {
          if (color_list->GetSize() == 3 ||
              color_list->GetSize() == 4) {
            if (color_list->GetInteger(0, &color) &&
                color_list->GetInteger(1, &color) &&
                color_list->GetInteger(2, &color)) {
              if (color_list->GetSize() == 4) {
                double alpha;
                int alpha_int;
                if (color_list->GetReal(3, &alpha) ||
                    color_list->GetInteger(3, &alpha_int)) {
                  ++iter;
                  continue;
                }
              } else {
                ++iter;
                continue;
              }
            }
          }
        }
        *error = errors::kInvalidThemeColors;
        return false;
      }
      theme_colors_.reset(
          static_cast<DictionaryValue*>(colors_value->DeepCopy()));
    }

    DictionaryValue* tints_value;
    if (theme_value->GetDictionary(keys::kThemeTints, &tints_value)) {
      // Validate that the tints are all reals.
      DictionaryValue::key_iterator iter = tints_value->begin_keys();
      while (iter != tints_value->end_keys()) {
        ListValue* tint_list;
        double v = 0;
        int vi = 0;
        if (!tints_value->GetList(*iter, &tint_list) ||
            tint_list->GetSize() != 3 ||
            !(tint_list->GetReal(0, &v) || tint_list->GetInteger(0, &vi)) ||
            !(tint_list->GetReal(1, &v) || tint_list->GetInteger(1, &vi)) ||
            !(tint_list->GetReal(2, &v) || tint_list->GetInteger(2, &vi))) {
          *error = errors::kInvalidThemeTints;
          return false;
        }
        ++iter;
      }
      theme_tints_.reset(
          static_cast<DictionaryValue*>(tints_value->DeepCopy()));
    }

    DictionaryValue* display_properties_value;
    if (theme_value->GetDictionary(keys::kThemeDisplayProperties,
        &display_properties_value)) {
      theme_display_properties_.reset(
          static_cast<DictionaryValue*>(display_properties_value->DeepCopy()));
    }

    return true;
  }

  // Initialize privacy blacklists (optional).
  if (source.HasKey(keys::kPrivacyBlacklists)) {
    ListValue* blacklists;
    if (!source.GetList(keys::kPrivacyBlacklists, &blacklists)) {
      *error = errors::kInvalidPrivacyBlacklists;
      return false;
    }

    for (size_t i = 0; i < blacklists->GetSize(); ++i) {
      std::string path;
      if (!blacklists->GetString(i, &path)) {
        *error = ExtensionErrorUtils::FormatErrorMessage(
            errors::kInvalidPrivacyBlacklistsPath, IntToString(i));
        return false;
      }
      privacy_blacklists_.push_back(PrivacyBlacklistInfo());
      privacy_blacklists_.back().path = path_.AppendASCII(path);
    }
  }

  // Initialize plugins (optional).
  if (source.HasKey(keys::kPlugins)) {
    ListValue* list_value;
    if (!source.GetList(keys::kPlugins, &list_value)) {
      *error = errors::kInvalidPlugins;
      return false;
    }

    for (size_t i = 0; i < list_value->GetSize(); ++i) {
      DictionaryValue* plugin_value;
      std::string path;
      bool is_public = false;

      if (!list_value->GetDictionary(i, &plugin_value)) {
        *error = errors::kInvalidPlugins;
        return false;
      }

      // Get plugins[i].path.
      if (!plugin_value->GetString(keys::kPluginsPath, &path)) {
        *error = ExtensionErrorUtils::FormatErrorMessage(
            errors::kInvalidPluginsPath, IntToString(i));
        return false;
      }

      // Get plugins[i].content (optional).
      if (plugin_value->HasKey(keys::kPluginsPublic)) {
        if (!plugin_value->GetBoolean(keys::kPluginsPublic, &is_public)) {
          *error = ExtensionErrorUtils::FormatErrorMessage(
              errors::kInvalidPluginsPublic, IntToString(i));
          return false;
        }
      }

      plugins_.push_back(PluginInfo());
      plugins_.back().path = path_.AppendASCII(path);
      plugins_.back().is_public = is_public;
    }
  }

  // Initialize background url (optional).
  if (source.HasKey(keys::kBackground)) {
    std::string background_str;
    if (!source.GetString(keys::kBackground, &background_str)) {
      *error = errors::kInvalidBackground;
      return false;
    }
    background_url_ = GetResourceURL(background_str);
  }

  // Initialize toolstrips (optional).
  if (source.HasKey(keys::kToolstrips)) {
    ListValue* list_value;
    if (!source.GetList(keys::kToolstrips, &list_value)) {
      *error = errors::kInvalidToolstrips;
      return false;
    }

    for (size_t i = 0; i < list_value->GetSize(); ++i) {
      ToolstripInfo toolstrip;
      DictionaryValue* toolstrip_value;
      std::string toolstrip_path;
      if (list_value->GetString(i, &toolstrip_path)) {
        // Support a simple URL value for backwards compatibility.
        // TODO(erikkay) Perhaps deprecate this in the future.
        toolstrip.toolstrip = GetResourceURL(toolstrip_path);
      } else if (list_value->GetDictionary(i, &toolstrip_value)) {
        if (!toolstrip_value->GetString(keys::kToolstripPath,
                                        &toolstrip_path)) {
          *error = ExtensionErrorUtils::FormatErrorMessage(
              errors::kInvalidToolstrip, IntToString(i));
          return false;
        }
        toolstrip.toolstrip = GetResourceURL(toolstrip_path);
        if (toolstrip_value->HasKey(keys::kToolstripMolePath)) {
          std::string mole_path;
          if (!toolstrip_value->GetString(keys::kToolstripMolePath,
                                          &mole_path)) {
            *error = ExtensionErrorUtils::FormatErrorMessage(
                errors::kInvalidToolstrip, IntToString(i));
            return false;
          }
          // TODO(erikkay) is there a better way to get this dynamically
          // from the content itself?
          int height;
          if (!toolstrip_value->GetInteger(keys::kToolstripMoleHeight,
                                           &height) || (height < 0)) {
            *error = ExtensionErrorUtils::FormatErrorMessage(
                errors::kInvalidToolstrip, IntToString(i));
            return false;
          }
          toolstrip.mole = GetResourceURL(mole_path);
          toolstrip.mole_height = height;
        }
      } else {
        *error = ExtensionErrorUtils::FormatErrorMessage(
            errors::kInvalidToolstrip, IntToString(i));
        return false;
      }
      toolstrips_.push_back(toolstrip);
    }
  }

  // Initialize content scripts (optional).
  if (source.HasKey(keys::kContentScripts)) {
    ListValue* list_value;
    if (!source.GetList(keys::kContentScripts, &list_value)) {
      *error = errors::kInvalidContentScriptsList;
      return false;
    }

    for (size_t i = 0; i < list_value->GetSize(); ++i) {
      DictionaryValue* content_script;
      if (!list_value->GetDictionary(i, &content_script)) {
        *error = ExtensionErrorUtils::FormatErrorMessage(
            errors::kInvalidContentScript, IntToString(i));
        return false;
      }

      UserScript script;
      if (!LoadUserScriptHelper(content_script, i, error, &script))
        return false;  // Failed to parse script context definition
      script.set_extension_id(id());
      content_scripts_.push_back(script);
    }
  }

  // Initialize page actions (optional).
  if (source.HasKey(keys::kPageActions)) {
    ListValue* list_value;
    if (!source.GetList(keys::kPageActions, &list_value)) {
      *error = errors::kInvalidPageActionsList;
      return false;
    }

    for (size_t i = 0; i < list_value->GetSize(); ++i) {
      DictionaryValue* page_action_value;
      if (!list_value->GetDictionary(i, &page_action_value)) {
        *error = ExtensionErrorUtils::FormatErrorMessage(
            errors::kInvalidPageAction, IntToString(i));
        return false;
      }

      ExtensionAction* contextual_action =
          LoadExtensionActionHelper(page_action_value, i, error,
                                    ExtensionAction::PAGE_ACTION);
      if (!contextual_action)
        return false;  // Failed to parse page action definition.
      page_actions_[contextual_action->id()] = contextual_action;
    }
  }

  if (source.HasKey(keys::kBrowserAction)) {
    DictionaryValue* browser_action_value;
    if (!source.GetDictionary(keys::kBrowserAction, &browser_action_value)) {
      *error = ExtensionErrorUtils::FormatErrorMessage(
          errors::kInvalidBrowserAction, "");
      return false;
    }

    browser_action_.reset(
        LoadExtensionActionHelper(browser_action_value, 0, error,
                                  ExtensionAction::BROWSER_ACTION));
    if (!browser_action_.get())
      return false;  // Failed to parse browser action definition.

    browser_action_state_.reset(
        new ExtensionActionState(browser_action_->name(), 0));
  }

  // Initialize the permissions (optional).
  if (source.HasKey(keys::kPermissions)) {
    ListValue* permissions = NULL;
    if (!source.GetList(keys::kPermissions, &permissions)) {
      *error = ExtensionErrorUtils::FormatErrorMessage(
          errors::kInvalidPermissions, "");
      return false;
    }

    for (size_t i = 0; i < permissions->GetSize(); ++i) {
      std::string permission_str;
      if (!permissions->GetString(i, &permission_str)) {
        *error = ExtensionErrorUtils::FormatErrorMessage(
            errors::kInvalidPermission, IntToString(i));
        return false;
      }

      // Check if it's a module permission.  If so, enable that permission.
      if (IsAPIPermission(permission_str)) {
        api_permissions_.push_back(permission_str);
        continue;
      }

      // Otherwise, it's a host pattern permission.
      URLPattern pattern;
      if (!pattern.Parse(permission_str)) {
        *error = ExtensionErrorUtils::FormatErrorMessage(
            errors::kInvalidPermission, IntToString(i));
        return false;
      }

      // Only accept http/https persmissions at the moment.
      if ((pattern.scheme() != chrome::kHttpScheme) &&
          (pattern.scheme() != chrome::kHttpsScheme)) {
        *error = ExtensionErrorUtils::FormatErrorMessage(
            errors::kInvalidPermissionScheme, IntToString(i));
        return false;
      }

      // The path component is not used for host permissions, so we force it to
      // match all paths.
      pattern.set_path("/*");

      host_permissions_.push_back(pattern);
    }
  }

  // Chrome URL overrides (optional)
  if (source.HasKey(keys::kChromeURLOverrides)) {
    DictionaryValue* overrides;
    if (!source.GetDictionary(keys::kChromeURLOverrides, &overrides)) {
      *error = errors::kInvalidChromeURLOverrides;
      return false;
    }
    // Validate that the overrides are all strings
    DictionaryValue::key_iterator iter = overrides->begin_keys();
    while (iter != overrides->end_keys()) {
      // For now, only allow the new tab page.  Others will work when we remove
      // this check, but let's keep it simple for now.
      // TODO(erikkay) enable other pages as well
      if (WideToUTF8(*iter) != chrome::kChromeUINewTabHost) {
        *error = errors::kInvalidChromeURLOverrides;
        return false;
      }
      std::string val;
      if (!overrides->GetString(*iter, &val)) {
        *error = errors::kInvalidChromeURLOverrides;
        return false;
      }
      // Replace the entry with a fully qualified chrome-extension:// URL.
      GURL url = GetResourceURL(val);
      overrides->SetString(*iter, url.spec());
      ++iter;
    }
    chrome_url_overrides_.reset(
        static_cast<DictionaryValue*>(overrides->DeepCopy()));
  }

  return true;
}

std::set<FilePath> Extension::GetBrowserImages() {
  std::set<FilePath> image_paths;

  // extension icons
  for (std::map<int, std::string>::iterator iter = icons_.begin();
       iter != icons_.end(); ++iter) {
    image_paths.insert(FilePath::FromWStringHack(UTF8ToWide(iter->second)));
  }

  // theme images
  DictionaryValue* theme_images = GetThemeImages();
  if (theme_images) {
    for (DictionaryValue::key_iterator it = theme_images->begin_keys();
         it != theme_images->end_keys(); ++it) {
      std::wstring val;
      if (theme_images->GetString(*it, &val)) {
        image_paths.insert(FilePath::FromWStringHack(val));
      }
    }
  }

  // page action icons
  for (ExtensionActionMap::const_iterator it = page_actions().begin();
       it != page_actions().end(); ++it) {
    const std::vector<std::string>& icon_paths = it->second->icon_paths();
    for (std::vector<std::string>::const_iterator iter = icon_paths.begin();
         iter != icon_paths.end(); ++iter) {
      image_paths.insert(FilePath::FromWStringHack(UTF8ToWide(*iter)));
    }
  }

  return image_paths;
}

bool Extension::GetBackgroundPageReady() {
  return background_page_ready_ || background_url().is_empty();
}

void Extension::SetBackgroundPageReady() {
  DCHECK(!background_url().is_empty());
  background_page_ready_ = true;
  NotificationService::current()->Notify(
      NotificationType::EXTENSION_BACKGROUND_PAGE_READY,
      Source<Extension>(this),
      NotificationService::NoDetails());
}

FilePath Extension::GetIconPath(Icons icon) {
  std::map<int, std::string>::const_iterator iter =
      icons_.find(Extension::EXTENSION_ICON_LARGE);
  if (iter == icons_.end())
    return FilePath();
  return GetResourcePath(iter->second);
}

bool Extension::CanAccessHost(const GURL& url) const {
  for (HostPermissions::const_iterator host = host_permissions_.begin();
       host != host_permissions_.end(); ++host) {
    if (host->MatchesUrl(url))
      return true;
  }

  return false;
}

const std::set<std::string> Extension::GetEffectiveHostPermissions() const {
  std::set<std::string> effective_hosts;

  for (HostPermissions::const_iterator host = host_permissions_.begin();
       host != host_permissions_.end(); ++host)
    effective_hosts.insert(host->host());

  for (UserScriptList::const_iterator content_script = content_scripts_.begin();
       content_script != content_scripts_.end(); ++content_script) {
    UserScript::PatternList::const_iterator pattern =
        content_script->url_patterns().begin();
    for (; pattern != content_script->url_patterns().end(); ++pattern)
      effective_hosts.insert(pattern->host());
  }

  return effective_hosts;
}

bool Extension::HasAccessToAllHosts() const {
  for (HostPermissions::const_iterator host = host_permissions_.begin();
       host != host_permissions_.end(); ++host) {
    if (host->match_subdomains() && host->host().empty())
      return true;
  }

  for (UserScriptList::const_iterator content_script = content_scripts_.begin();
       content_script != content_scripts_.end(); ++content_script) {
    UserScript::PatternList::const_iterator pattern =
        content_script->url_patterns().begin();
    for (; pattern != content_script->url_patterns().end(); ++pattern) {
      if (pattern->match_subdomains() && pattern->host().empty())
        return true;
    }
  }

  return false;
}
