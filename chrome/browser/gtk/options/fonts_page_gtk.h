// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The fonts page of the fonts & languages options dialog, which contains font
// family and size settings, as well as the default encoding option.

#ifndef CHROME_BROWSER_GTK_OPTIONS_FONTS_PAGE_GTK_H_
#define CHROME_BROWSER_GTK_OPTIONS_FONTS_PAGE_GTK_H_

#include <gtk/gtk.h>
#include <vector>

#include "chrome/browser/character_encoding.h"
#include "chrome/browser/options_page_base.h"
#include "chrome/common/pref_member.h"

class FontsPageGtk : public OptionsPageBase {
 public:
  explicit FontsPageGtk(Profile* profile);
  virtual ~FontsPageGtk();

  GtkWidget* get_page_widget() const {
    return page_;
  }

 private:
  void Init();
  void InitDefaultEncodingComboBox();

  // Overridden from OptionsPageBase.
  virtual void NotifyPrefChanged(const std::wstring* pref_name);

  // Retrieve the font selection from the button and save it to the prefs.  Also
  // ensure the button(s) are displayed in the proper size, as the
  // GtkFontSelector returns the value in points not pixels.
  void SetFontsFromButton(StringPrefMember* name_pref,
                          IntegerPrefMember* size_pref,
                          GtkFontButton* font_button);

  // Callbacks
  static void OnSerifFontSet(GtkFontButton* font_button,
                             FontsPageGtk* fonts_page);
  static void OnSansFontSet(GtkFontButton* font_button,
                            FontsPageGtk* fonts_page);
  static void OnFixedFontSet(GtkFontButton* font_button,
                             FontsPageGtk* fonts_page);
  static void OnDefaultEncodingChanged(GtkComboBox* combo_box,
                                       FontsPageGtk* fonts_page);

  // The font chooser widgets
  GtkWidget* serif_font_button_;
  GtkWidget* sans_font_button_;
  GtkWidget* fixed_font_button_;

  // The default encoding combobox widget.
  GtkWidget* default_encoding_combobox_;

  // The widget containing the options for this page.
  GtkWidget* page_;

  // Font name preferences.
  StringPrefMember serif_name_;
  StringPrefMember sans_serif_name_;
  StringPrefMember fixed_width_name_;

  // Font size preferences, in pixels.
  IntegerPrefMember variable_width_size_;
  IntegerPrefMember fixed_width_size_;

  // Default encoding preference.
  StringPrefMember default_encoding_;
  std::vector<CharacterEncoding::EncodingInfo> sorted_encoding_list_;

  DISALLOW_COPY_AND_ASSIGN(FontsPageGtk);
};

#endif  // CHROME_BROWSER_GTK_OPTIONS_FONTS_PAGE_GTK_H_
