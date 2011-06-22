// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/meegotouch/qt_theme_service.h"

#include <set>

#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/profiles/profile.h"

// static
QtThemeService* QtThemeService::GetFrom(Profile* profile) {
  return static_cast<QtThemeService*>(
      ThemeServiceFactory::GetForProfile(profile));
}

QtThemeService::QtThemeService()
    : ThemeService() {
}

QtThemeService::~QtThemeService() {
}

void QtThemeService::SetTheme(Extension* extension) {
  LoadQtValues();
  ThemeService::SetTheme(extension);
}

void QtThemeService::UseDefaultTheme() {
  LoadQtValues();
  ThemeService::UseDefaultTheme();
}

void QtThemeService::SetNativeTheme() {
  LoadQtValues();
  ThemeService::SetNativeTheme();
}

void QtThemeService::LoadThemePrefs() {
  LoadQtValues();
  ThemeService::LoadThemePrefs();
}

void QtThemeService::LoadQtValues() {
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

QImage* QtThemeService::GetPixbufNamed(int id) const {
  return GetPixbufImpl(id, false);
}

QImage* QtThemeService::GetRTLEnabledPixbufNamed(int id) const {
  return GetPixbufImpl(id, true);
}

QImage* QtThemeService::GetPixbufImpl(int id, bool rtl_enabled) const {
  DCHECK(CalledOnValidThread());
  DNOTIMPLEMENTED();
  return NULL;
}
