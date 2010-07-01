// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GTK_TRANSLATE_BEFORE_TRANSLATE_INFOBAR_GTK_H_
#define CHROME_BROWSER_GTK_TRANSLATE_BEFORE_TRANSLATE_INFOBAR_GTK_H_

#include "chrome/browser/gtk/translate/translate_infobar_base_gtk.h"

class TranslateInfoBarDelegate;

class BeforeTranslateInfoBar : public TranslateInfoBarBase {
 public:
  explicit BeforeTranslateInfoBar(TranslateInfoBarDelegate* delegate);
  virtual ~BeforeTranslateInfoBar();

  // Overridden from TranslateInfoBarBase:
  virtual void Init();

 protected:
  virtual bool ShowOptionsMenuButton() const { return true; }

 private:
  CHROMEGTK_CALLBACK_0(BeforeTranslateInfoBar, void, OnLanguageModified);
  CHROMEGTK_CALLBACK_0(BeforeTranslateInfoBar, void, OnAcceptPressed);
  CHROMEGTK_CALLBACK_0(BeforeTranslateInfoBar, void, OnDenyPressed);
  CHROMEGTK_CALLBACK_0(BeforeTranslateInfoBar, void, OnNeverTranslatePressed);
  CHROMEGTK_CALLBACK_0(BeforeTranslateInfoBar, void, OnAlwaysTranslatePressed);

  DISALLOW_COPY_AND_ASSIGN(BeforeTranslateInfoBar);
};

#endif  // CHROME_BROWSER_GTK_TRANSLATE_BEFORE_TRANSLATE_INFOBAR_GTK_H_
