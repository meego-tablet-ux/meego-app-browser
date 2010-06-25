// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GTK_TRANSLATE_AFTER_TRANSLATE_INFOBAR_GTK_H_
#define CHROME_BROWSER_GTK_TRANSLATE_AFTER_TRANSLATE_INFOBAR_GTK_H_

#include "base/task.h"
#include "chrome/browser/gtk/translate/translate_infobar_base_gtk.h"

class TranslateInfoBarDelegate2;

class AfterTranslateInfoBar : public TranslateInfoBarBase {
 public:
  explicit AfterTranslateInfoBar(TranslateInfoBarDelegate2* delegate);
  virtual ~AfterTranslateInfoBar();

  // Overridden from TranslateInfoBarBase:
  virtual void Init();

 protected:
  virtual bool ShowOptionsMenuButton() const { return true; }

 private:
  CHROMEGTK_CALLBACK_0(AfterTranslateInfoBar, void, OnOriginalLanguageModified);
  CHROMEGTK_CALLBACK_0(AfterTranslateInfoBar, void, OnTargetLanguageModified);
  CHROMEGTK_CALLBACK_0(AfterTranslateInfoBar, void, OnRevertPressed);

  // These methods set the original/target language on the
  // TranslateInfobarDelegate.
  void SetOriginalLanguage(int language_index);
  void SetTargetLanguage(int language_index);

  ScopedRunnableMethodFactory<AfterTranslateInfoBar> method_factory_;

  DISALLOW_COPY_AND_ASSIGN(AfterTranslateInfoBar);
};

#endif  // CHROME_BROWSER_GTK_TRANSLATE_AFTER_TRANSLATE_INFOBAR_GTK_H_
