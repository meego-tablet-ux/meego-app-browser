// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/gtk/gtk_theme_provider.h"

#include <gtk/gtk.h>

#include "base/gfx/gtk_util.h"
#include "chrome/browser/metrics/user_metrics.h"
#include "chrome/browser/profile.h"
#include "chrome/common/pref_names.h"
#include "grit/theme_resources.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"

namespace {

// The size of the rendered toolbar image.
const int kToolbarImageWidth = 64;
const int kToolbarImageHeight = 128;

}  // namespace

GtkThemeProvider::GtkThemeProvider()
    : BrowserThemeProvider(),
      fake_window_(gtk_window_new(GTK_WINDOW_TOPLEVEL)) {
  // Only realized widgets receive style-set notifications, which we need to
  // broadcast new theme images and colors.
  gtk_widget_realize(fake_window_);
  g_signal_connect(fake_window_, "style-set", G_CALLBACK(&OnStyleSet), this);
}

GtkThemeProvider::~GtkThemeProvider() {
  gtk_widget_destroy(fake_window_);
}

void GtkThemeProvider::SetTheme(Extension* extension) {
  profile()->GetPrefs()->SetBoolean(prefs::kUsesSystemTheme, false);
  BrowserThemeProvider::SetTheme(extension);
}

void GtkThemeProvider::UseDefaultTheme() {
  profile()->GetPrefs()->SetBoolean(prefs::kUsesSystemTheme, false);
  BrowserThemeProvider::UseDefaultTheme();
}

void GtkThemeProvider::SetNativeTheme() {
  profile()->GetPrefs()->SetBoolean(prefs::kUsesSystemTheme, true);
  ClearAllThemeData();
  LoadGtkValues();
  NotifyThemeChanged();
}

// static
bool GtkThemeProvider::UseSystemThemeGraphics(Profile* profile) {
  return profile->GetPrefs()->GetBoolean(prefs::kUsesSystemTheme);
}

void GtkThemeProvider::LoadThemePrefs() {
  if (profile()->GetPrefs()->GetBoolean(prefs::kUsesSystemTheme)) {
    LoadGtkValues();
  } else {
    BrowserThemeProvider::LoadThemePrefs();
  }
}

SkBitmap* GtkThemeProvider::LoadThemeBitmap(int id) {
  if (id == IDR_THEME_TOOLBAR && UseSystemThemeGraphics(profile())) {
    GtkStyle* style = gtk_rc_get_style(fake_window_);
    GdkColor* color = &style->bg[GTK_STATE_NORMAL];
    SkBitmap* bitmap = new SkBitmap;
    bitmap->setConfig(SkBitmap::kARGB_8888_Config,
                      kToolbarImageWidth, kToolbarImageHeight);
    bitmap->allocPixels();
    bitmap->eraseRGB(color->red >> 8, color->green >> 8, color->blue >> 8);
    return bitmap;
  } else {
    return BrowserThemeProvider::LoadThemeBitmap(id);
  }
}

// static
void GtkThemeProvider::OnStyleSet(GtkWidget* widget,
                                  GtkStyle* previous_style,
                                  GtkThemeProvider* provider) {
  if (provider->profile()->GetPrefs()->GetBoolean(prefs::kUsesSystemTheme)) {
    provider->ClearAllThemeData();
    provider->LoadGtkValues();
    provider->NotifyThemeChanged();
  }
}

void GtkThemeProvider::LoadGtkValues() {
  GtkStyle* style = gtk_rc_get_style(fake_window_);

  SetThemeColorFromGtk(themes::kColorFrame, &style->bg[GTK_STATE_SELECTED]);
  SetThemeColorFromGtk(themes::kColorFrameInactive,
                       &style->bg[GTK_STATE_INSENSITIVE]);
  // TODO(erg): Incognito images.
  SetThemeColorFromGtk(themes::kColorToolbar,
                       &style->bg[GTK_STATE_NORMAL]);
  SetThemeColorFromGtk(themes::kColorTabText,
                       &style->text[GTK_STATE_NORMAL]);
  SetThemeColorFromGtk(themes::kColorBackgroundTabText,
                       &style->text[GTK_STATE_NORMAL]);
  SetThemeColorFromGtk(themes::kColorBookmarkText,
                       &style->text[GTK_STATE_NORMAL]);
  SetThemeColorFromGtk(themes::kColorControlBackground,
                       &style->bg[GTK_STATE_NORMAL]);
  SetThemeColorFromGtk(themes::kColorButtonBackground,
                       &style->bg[GTK_STATE_NORMAL]);

  SetThemeTintFromGtk(themes::kTintButtons, &style->bg[GTK_STATE_SELECTED],
                      themes::kDefaultTintButtons);
  SetThemeTintFromGtk(themes::kTintFrame, &style->bg[GTK_STATE_SELECTED],
                      themes::kDefaultTintFrame);
  SetThemeTintFromGtk(themes::kTintFrameInactive,
                      &style->bg[GTK_STATE_SELECTED],
                      themes::kDefaultTintFrameInactive);
  SetThemeTintFromGtk(themes::kTintFrameIncognito,
                      &style->bg[GTK_STATE_SELECTED],
                      themes::kDefaultTintFrameIncognito);
  SetThemeTintFromGtk(themes::kTintFrameIncognitoInactive,
                      &style->bg[GTK_STATE_SELECTED],
                      themes::kDefaultTintFrameIncognitoInactive);
  SetThemeTintFromGtk(themes::kTintBackgroundTab,
                      &style->bg[GTK_STATE_SELECTED],
                      themes::kDefaultTintBackgroundTab);

  GenerateFrameColors();
  GenerateFrameImages();
}

void GtkThemeProvider::SetThemeColorFromGtk(const char* id, GdkColor* color) {
  SetColor(id, SkColorSetRGB(color->red >> 8,
                             color->green >> 8,
                             color->blue >> 8));
}

void GtkThemeProvider::SetThemeTintFromGtk(const char* id, GdkColor* color,
                                           const skia::HSL& default_tint) {
  skia::HSL hsl;
  skia::SkColorToHSL(SkColorSetRGB((color->red >> 8),
                                   (color->green >> 8),
                                   (color->blue >> 8)), hsl);
  if (default_tint.s != -1)
    hsl.s = default_tint.s;

  if (default_tint.l != -1)
    hsl.l = default_tint.l;
  SetTint(id, hsl);
}

