// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_RENDER_MESSAGES_PARAMS_H_
#define CHROME_COMMON_RENDER_MESSAGES_PARAMS_H_
#pragma once

#include <string>

#include "base/values.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/extensions/extension_extent.h"
#include "chrome/common/extensions/url_pattern.h"
#include "googleurl/src/gurl.h"
#include "ipc/ipc_param_traits.h"

namespace net {
class UploadData;
}

// The type of OSDD that the renderer is giving to the browser.
struct ViewHostMsg_PageHasOSDD_Type {
  enum Type {
    // The Open Search Description URL was detected automatically.
    AUTODETECTED_PROVIDER,

    // The Open Search Description URL was given by Javascript.
    EXPLICIT_PROVIDER,

    // The Open Search Description URL was given by Javascript to be the new
    // default search engine.
    EXPLICIT_DEFAULT_PROVIDER
  };

  Type type;

  ViewHostMsg_PageHasOSDD_Type() : type(AUTODETECTED_PROVIDER) {
  }

  explicit ViewHostMsg_PageHasOSDD_Type(Type t)
      : type(t) {
  }

  static ViewHostMsg_PageHasOSDD_Type Autodetected() {
    return ViewHostMsg_PageHasOSDD_Type(AUTODETECTED_PROVIDER);
  }

  static ViewHostMsg_PageHasOSDD_Type Explicit() {
    return ViewHostMsg_PageHasOSDD_Type(EXPLICIT_PROVIDER);
  }

  static ViewHostMsg_PageHasOSDD_Type ExplicitDefault() {
    return ViewHostMsg_PageHasOSDD_Type(EXPLICIT_DEFAULT_PROVIDER);
  }
};

// The install state of the search provider (not installed, installed, default).
struct ViewHostMsg_GetSearchProviderInstallState_Params {
  enum State {
    // Equates to an access denied error.
    DENIED = -1,

    // DON'T CHANGE THE VALUES BELOW.
    // All of the following values are manidated by the
    // spec for window.external.IsSearchProviderInstalled.

    // The search provider is not installed.
    NOT_INSTALLED = 0,

    // The search provider is in the user's set but is not
    INSTALLED_BUT_NOT_DEFAULT = 1,

    // The search provider is set as the user's default.
    INSTALLED_AS_DEFAULT = 2
  };
  State state;

  ViewHostMsg_GetSearchProviderInstallState_Params()
      : state(DENIED) {
  }

  explicit ViewHostMsg_GetSearchProviderInstallState_Params(State s)
      : state(s) {
  }

  static ViewHostMsg_GetSearchProviderInstallState_Params Denied() {
    return ViewHostMsg_GetSearchProviderInstallState_Params(DENIED);
  }

  static ViewHostMsg_GetSearchProviderInstallState_Params NotInstalled() {
    return ViewHostMsg_GetSearchProviderInstallState_Params(NOT_INSTALLED);
  }

  static ViewHostMsg_GetSearchProviderInstallState_Params
      InstallButNotDefault() {
    return ViewHostMsg_GetSearchProviderInstallState_Params(
        INSTALLED_BUT_NOT_DEFAULT);
  }

  static ViewHostMsg_GetSearchProviderInstallState_Params InstalledAsDefault() {
    return ViewHostMsg_GetSearchProviderInstallState_Params(
        INSTALLED_AS_DEFAULT);
  }
};

// Allows an extension to execute code in a tab.
struct ViewMsg_ExecuteCode_Params {
  ViewMsg_ExecuteCode_Params();
  ViewMsg_ExecuteCode_Params(int request_id, const std::string& extension_id,
                             bool is_javascript, const std::string& code,
                             bool all_frames);
  ~ViewMsg_ExecuteCode_Params();

  // The extension API request id, for responding.
  int request_id;

  // The ID of the requesting extension. To know which isolated world to
  // execute the code inside of.
  std::string extension_id;

  // Whether the code is JavaScript or CSS.
  bool is_javascript;

  // String of code to execute.
  std::string code;

  // Whether to inject into all frames, or only the root frame.
  bool all_frames;
};

struct ViewMsg_ExtensionLoaded_Params {
  ViewMsg_ExtensionLoaded_Params();
  ~ViewMsg_ExtensionLoaded_Params();
  explicit ViewMsg_ExtensionLoaded_Params(const Extension* extension);

  // A copy constructor is needed because this structure can end up getting
  // copied inside the IPC machinery on gcc <= 4.2.
  ViewMsg_ExtensionLoaded_Params(
      const ViewMsg_ExtensionLoaded_Params& other);

  // Creates a new extension from the data in this object.
  scoped_refptr<Extension> ConvertToExtension() const;

  // The subset of the extension manifest data we send to renderers.
  scoped_ptr<DictionaryValue> manifest;

  // The location the extension was installed from.
  Extension::Location location;

  // The path the extension was loaded from. This is used in the renderer only
  // to generate the extension ID for extensions that are loaded unpacked.
  FilePath path;

  // We keep this separate so that it can be used in logging.
  std::string id;
};

// Parameters structure for ViewHostMsg_ExtensionRequest.
struct ViewHostMsg_DomMessage_Params {
  ViewHostMsg_DomMessage_Params();
  ~ViewHostMsg_DomMessage_Params();

  // Message name.
  std::string name;

  // List of message arguments.
  ListValue arguments;

  // URL of the frame request was sent from.
  GURL source_url;

  // Unique request id to match requests and responses.
  int request_id;

  // True if request has a callback specified.
  bool has_callback;

  // True if request is executed in response to an explicit user gesture.
  bool user_gesture;
};

namespace IPC {

class Message;

template <>
struct ParamTraits<ViewHostMsg_PageHasOSDD_Type> {
  typedef ViewHostMsg_PageHasOSDD_Type param_type;
  static void Write(Message* m, const param_type& p);
  static bool Read(const Message* m, void** iter, param_type* p);
  static void Log(const param_type& p, std::string* l);
};

template <>
struct ParamTraits<ViewHostMsg_GetSearchProviderInstallState_Params> {
  typedef ViewHostMsg_GetSearchProviderInstallState_Params param_type;
  static void Write(Message* m, const param_type& p);
  static bool Read(const Message* m, void** iter, param_type* p);
  static void Log(const param_type& p, std::string* l);
};

template <>
struct ParamTraits<ViewMsg_ExecuteCode_Params> {
  typedef ViewMsg_ExecuteCode_Params param_type;
  static void Write(Message* m, const param_type& p);
  static bool Read(const Message* m, void** iter, param_type* p);
  static void Log(const param_type& p, std::string* l);
};

template <>
struct ParamTraits<ViewMsg_ExtensionLoaded_Params> {
  typedef ViewMsg_ExtensionLoaded_Params param_type;
  static void Write(Message* m, const param_type& p);
  static bool Read(const Message* m, void** iter, param_type* p);
  static void Log(const param_type& p, std::string* l);
};

template <>
struct ParamTraits<ViewHostMsg_DomMessage_Params> {
  typedef ViewHostMsg_DomMessage_Params param_type;
  static void Write(Message* m, const param_type& p);
  static bool Read(const Message* m, void** iter, param_type* p);
  static void Log(const param_type& p, std::string* l);
};

}  // namespace IPC

#endif  // CHROME_COMMON_RENDER_MESSAGES_PARAMS_H_
