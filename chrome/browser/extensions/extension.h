// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_H_

#include <string>
#include <vector>

#include "base/file_path.h"
#include "base/scoped_ptr.h"
#include "base/string16.h"
#include "base/values.h"
#include "base/version.h"
#include "chrome/browser/extensions/user_script_master.h"
#include "googleurl/src/gurl.h"

// Represents a Chromium extension.
class Extension {
 public:
  Extension() {}
  explicit Extension(const FilePath& path);

  // The format for extension manifests that this code understands.
  static const unsigned int kExpectedFormatVersion = 1;

  // The name of the manifest inside an extension.
  static const char kManifestFilename[];

  // Keys used in JSON representation of extensions.
  static const wchar_t* kContentScriptsKey;
  static const wchar_t* kDescriptionKey;
  static const wchar_t* kFormatVersionKey;
  static const wchar_t* kIdKey;
  static const wchar_t* kJsKey;
  static const wchar_t* kMatchesKey;
  static const wchar_t* kNameKey;
  static const wchar_t* kRunAtKey;
  static const wchar_t* kVersionKey;
  static const wchar_t* kZipHashKey;
  static const wchar_t* kPluginsDirKey;

  // Some values expected in manifests.
  static const char* kRunAtDocumentStartValue;
  static const char* kRunAtDocumentEndValue;

  // Error messages returned from InitFromValue().
  static const char* kInvalidContentScriptError;
  static const char* kInvalidContentScriptsListError;
  static const char* kInvalidDescriptionError;
  static const char* kInvalidFormatVersionError;
  static const char* kInvalidIdError;
  static const char* kInvalidJsCountError;
  static const char* kInvalidJsError;
  static const char* kInvalidJsListError;
  static const char* kInvalidManifestError;
  static const char* kInvalidMatchCountError;
  static const char* kInvalidMatchError;
  static const char* kInvalidMatchesError;
  static const char* kInvalidNameError;
  static const char* kInvalidRunAtError;
  static const char* kInvalidVersionError;
  static const char* kInvalidZipHashError;
  static const char* kInvalidPluginsDirError;

  // The number of bytes in a legal id.
  static const size_t kIdSize;

  // Creates an absolute url to a resource inside an extension. The
  // |extension_url| argument should be the url() from an Extension object. The
  // |relative_path| can be untrusted user input. The returned URL will either
  // be invalid() or a child of |extension_url|.
  // NOTE: Static so that it can be used from multiple threads.
  static GURL GetResourceURL(const GURL& extension_url,
                             const std::string& relative_path);

  // Creates an absolute path to a resource inside an extension. The
  // |extension_path| argument should be the path() from an Extension object.
  // The |relative_path| can be untrusted user input. The returned path will
  // either be empty or a child of extension_path.
  // NOTE: Static so that it can be used from multiple threads.
  static FilePath GetResourcePath(const FilePath& extension_path,
                                  const std::string& relative_path);

  // The path to the folder the extension is stored in.
  const FilePath& path() const { return path_; }

  // The base URL for the extension.
  const GURL& url() const { return extension_url_; }

  // A human-readable ID for the extension. The convention is to use something
  // like 'com.example.myextension', but this is not currently enforced. An
  // extension's ID is used in things like directory structures and URLs, and
  // is expected to not change across versions. In the case of conflicts,
  // updates will only be allowed if the extension can be validated using the
  // previous version's update key.
  const std::string& id() const { return id_; }

  // The version number for the extension.
  const Version* version() const { return version_.get(); }

  // String representation of the version number.
  const std::string VersionString() const;

  // A human-readable name of the extension.
  const std::string& name() const { return name_; }

  // An optional longer description of the extension.
  const std::string& description() const { return description_; }

  // Paths to the content scripts that the extension contains.
  const UserScriptList& content_scripts() const {
    return content_scripts_;
  }

  // Path to the directory of NPAPI plugins that the extension contains.
  const FilePath& plugins_dir() const {
    return plugins_dir_;
  }

  // Initialize the extension from a parsed manifest.
  bool InitFromValue(const DictionaryValue& value, std::string* error);

 private:
  // The path to the directory the extension is stored in.
  FilePath path_;

  // The base extension url for the extension.
  GURL extension_url_;

  // The extension's ID.
  std::string id_;

  // The extension's version.
  scoped_ptr<Version> version_;

  // The extension's human-readable name.
  std::string name_;

  // An optional description for the extension.
  std::string description_;

  // Paths to the content scripts the extension contains.
  UserScriptList content_scripts_;

  // Path to the directory of NPAPI plugins that the extension contains.
  FilePath plugins_dir_;

  // A SHA1 hash of the contents of the zip file.  Note that this key is only
  // present in the manifest that's prepended to the zip.  The inner manifest
  // will not have this key.
  std::string zip_hash_;

  DISALLOW_COPY_AND_ASSIGN(Extension);
};

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_H_
