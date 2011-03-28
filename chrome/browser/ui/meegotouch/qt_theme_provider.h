// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GTK_QT_THEME_PROVIDER_H_
#define CHROME_BROWSER_GTK_QT_THEME_PROVIDER_H_
#pragma once

#include <map>
#include <vector>

#include "base/scoped_ptr.h"
#include "chrome/browser/prefs/pref_change_registrar.h"
#include "chrome/browser/themes/browser_theme_provider.h"
#include "content/common/notification_observer.h"

// Specialization of BrowserThemeProvider which supplies system colors.
class QtThemeProvider : public BrowserThemeProvider {
 public:
  // Returns QtThemeProvider, casted from our superclass.
  static QtThemeProvider* GetFrom(Profile* profile);

  QtThemeProvider();
  virtual ~QtThemeProvider();

  // BrowserThemeProvider's implementation.
  virtual void SetTheme(Extension* extension);
  virtual void UseDefaultTheme();
  virtual void SetNativeTheme();

  // Returns colors that we pass to webkit to match the system theme.
  const SkColor& get_focus_ring_color() const { return focus_ring_color_; }
  const SkColor& get_thumb_active_color() const { return thumb_active_color_; }
  const SkColor& get_thumb_inactive_color() const {
    return thumb_inactive_color_;
  }
  const SkColor& get_track_color() const { return track_color_; }
  const SkColor& get_active_selection_bg_color() const {
    return active_selection_bg_color_;
  }
  const SkColor& get_active_selection_fg_color() const {
    return active_selection_fg_color_;
  }
  const SkColor& get_inactive_selection_bg_color() const {
    return inactive_selection_bg_color_;
  }
  const SkColor& get_inactive_selection_fg_color() const {
    return inactive_selection_fg_color_;
  }

 private:
  // Load theme data from preferences, possibly picking colors from GTK.
  virtual void LoadThemePrefs();

  // Extracts colors and tints from the QT theme, both for the
  // BrowserThemeProvider interface and the colors we send to webkit.
  // May set theme for BrowserThemeProvider interface if needed.
  void LoadQtValues();

  // Colors that we pass to WebKit. These are generated each time the theme
  // changes.
  SkColor focus_ring_color_;
  SkColor thumb_active_color_;
  SkColor thumb_inactive_color_;
  SkColor track_color_;
  SkColor active_selection_bg_color_;
  SkColor active_selection_fg_color_;
  SkColor inactive_selection_bg_color_;
  SkColor inactive_selection_fg_color_;

};

#endif  // CHROME_BROWSER_GTK_QT_THEME_PROVIDER_H_
