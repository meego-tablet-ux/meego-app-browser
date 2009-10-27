// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_EXTENSIONS_EXTENSION_ACTION_H_
#define CHROME_COMMON_EXTENSIONS_EXTENSION_ACTION_H_

#include <map>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "googleurl/src/gurl.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"

namespace gfx {
class Canvas;
class Rect;
}

// ExtensionAction encapsulates the state of a browser or page action.
// Instances can have both global and per-tab state. If a property does not have
// a per-tab value, the global value is used instead.
//
// TODO(aa): This should replace ExtensionAction and ExtensionActionState.
class ExtensionAction {
 public:
  // Use this ID to indicate the default state for properties that take a tab_id
  // parameter.
  static const int kDefaultTabId;

  // extension id
  std::string extension_id() const { return extension_id_; }
  void set_extension_id(const std::string& extension_id) {
    extension_id_ = extension_id;
  }

  // popup details
  const GURL& popup_url() const { return popup_url_; }
  void set_popup_url(const GURL& url) { popup_url_ = url; }
  bool has_popup() const { return !popup_url_.is_empty(); }

  // action id -- only used with legacy page actions API
  std::string id() const { return id_; }
  void set_id(const std::string& id) { id_ = id; }

  // static icon paths from manifest -- only used with legacy page actions API.
  std::vector<std::string>* icon_paths() { return &icon_paths_; }

  // title
  void SetTitle(int tab_id, const std::string& title) {
    SetValue(&title_, tab_id, title);
  }
  std::string GetTitle(int tab_id) { return GetValue(&title_, tab_id); }

  // Icons are a bit different because the default value can be set to either a
  // bitmap or a path. However, conceptually, there is only one default icon.
  // Setting the default icon using a path clears the bitmap and vice-versa.
  //
  // To get the default icon, first check for the bitmap. If it is null, check
  // for the path.

  // Icon bitmap.
  void SetIcon(int tab_id, const SkBitmap& bitmap) {
    SetValue(&icon_, tab_id, bitmap);
  }
  SkBitmap GetIcon(int tab_id) { return GetValue(&icon_, tab_id); }

  // Icon index -- for use with icon_paths(), only used in page actions.
  void SetIconIndex(int tab_id, int index) {
    if (static_cast<size_t>(index) >= icon_paths_.size()) {
      NOTREACHED();
      return;
    }
    SetValue(&icon_index_, tab_id, index);
  }
  int GetIconIndex(int tab_id) {
    return GetValue(&icon_index_, tab_id);
  }

  // Non-tab-specific icon path. This is used to support the default_icon key of
  // page and browser actions.
  void set_default_icon_path(const std::string& path) {
    default_icon_path_ = path;
  }
  std::string default_icon_path() {
    return default_icon_path_;
  }

  // badge text
  void SetBadgeText(int tab_id, const std::string& text) {
    SetValue(&badge_text_, tab_id, text);
  }
  std::string GetBadgeText(int tab_id) { return GetValue(&badge_text_, tab_id); }

  // badge text color
  void SetBadgeTextColor(int tab_id, const SkColor& text_color) {
    SetValue(&badge_text_color_, tab_id, text_color);
  }
  SkColor GetBadgeTextColor(int tab_id) {
    return GetValue(&badge_text_color_, tab_id);
  }

  // badge background color
  void SetBadgeBackgroundColor(int tab_id, const SkColor& color) {
    SetValue(&badge_background_color_, tab_id, color);
  }
  SkColor GetBadgeBackgroundColor(int tab_id) {
    return GetValue(&badge_background_color_, tab_id);
  }

  // visibility
  void SetIsVisible(int tab_id, bool value) {
    SetValue(&visible_, tab_id, value);
  }
  bool GetIsVisible(int tab_id) {
    return GetValue(&visible_, tab_id);
  }

  // Remove all tab-specific state.
  void ClearAllValuesForTab(int tab_id);

  // If the specified tab has a badge, paint it into the provided bounds.
  void PaintBadge(gfx::Canvas* canvas, const gfx::Rect& bounds, int tab_id);

 private:
  template <class T>
  struct ValueTraits {
    static T CreateEmpty() {
      return T();
    }
  };

  template<class T>
  void SetValue(std::map<int, T>* map, int tab_id, T val) {
    (*map)[tab_id] = val;
  }

  template<class T>
  T GetValue(std::map<int, T>* map, int tab_id) {
    typename std::map<int, T>::iterator iter = map->find(tab_id);
    if (iter != map->end()) {
      return iter->second;
    } else {
      iter = map->find(kDefaultTabId);
      return iter != map->end() ? iter->second : ValueTraits<T>::CreateEmpty();
    }
  }

  // The id for the extension this action belongs to (as defined in the
  // extension manifest).
  std::string extension_id_;

  // Each of these data items can have both a global state (stored with the key
  // kDefaultTabId), or tab-specific state (stored with the tab_id as the key).
  std::map<int, std::string> title_;
  std::map<int, SkBitmap> icon_;
  std::map<int, int> icon_index_;  // index into icon_paths_
  std::map<int, std::string> badge_text_;
  std::map<int, SkColor> badge_background_color_;
  std::map<int, SkColor> badge_text_color_;
  std::map<int, bool> visible_;

  std::string default_icon_path_;

  // If the action has a popup, it has a URL and a height.
  GURL popup_url_;

  // The id for the ExtensionAction, for example: "RssPageAction". This is
  // needed for compat with an older version of the page actions API.
  std::string id_;

  // A list of paths to icons this action might show. This is needed to support
  // the legacy setIcon({iconIndex:...} method of the page actions API.
  std::vector<std::string> icon_paths_;
};

template<>
struct ExtensionAction::ValueTraits<int> {
  static int CreateEmpty() {
    return -1;
  }
};

#endif  // CHROME_COMMON_EXTENSIONS_EXTENSION_ACTION_H_
