// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/meegotouch/qt_theme_provider.h"

#include <set>

#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/profiles/profile.h"

// static
QtThemeProvider* QtThemeProvider::GetFrom(Profile* profile) {
  return static_cast<QtThemeProvider*>(
      ThemeServiceFactory::GetForProfile(profile));
}

QtThemeProvider::QtThemeProvider()
    : BrowserThemeProvider() {
}

QtThemeProvider::~QtThemeProvider() {
}

void QtThemeProvider::SetTheme(Extension* extension) {
  LoadQtValues();
  BrowserThemeProvider::SetTheme(extension);
}

void QtThemeProvider::UseDefaultTheme() {
  LoadQtValues();
  BrowserThemeProvider::UseDefaultTheme();
}

void QtThemeProvider::SetNativeTheme() {
  LoadQtValues();
  BrowserThemeProvider::SetNativeTheme();
}

void QtThemeProvider::LoadThemePrefs() {
  LoadQtValues();
  BrowserThemeProvider::LoadThemePrefs();
}

void QtThemeProvider::LoadQtValues() {
  // Temporary solution: use default hard-coded settings.
  // May replace them by extracting theme settings from QML

  // This color is from frame boder color in QT 
  focus_ring_color_ = SkColorSetARGB(127, 102, 102, 102);

  // Below three colors are from default values in GtkThemeProvider
  thumb_active_color_ = SkColorSetRGB(244, 244, 244);
  thumb_inactive_color_ = SkColorSetRGB(234, 234, 234);
  track_color_ = SkColorSetRGB(211, 211, 211);

  // Selection colors from MTextEditStyle selection colors
  active_selection_bg_color_ = SkColorSetRGB(127, 177, 51);
  active_selection_fg_color_ = SK_ColorWHITE;

  // Below two colors are from default values in GtkThemeProvider
  inactive_selection_bg_color_ = SkColorSetRGB(200, 200, 200);
  inactive_selection_fg_color_ = SkColorSetRGB(50, 50, 50);
}
