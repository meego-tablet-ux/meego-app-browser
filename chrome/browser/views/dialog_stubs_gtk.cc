// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains stub implementations of the functions declared in
// browser_dialogs.h that are currently unimplemented in GTK-views.

#include "base/logging.h"
#include "chrome/browser/fonts_languages_window.h"
#include "chrome/browser/options_window.h"
#include "chrome/browser/views/browser_dialogs.h"

namespace browser {

void ShowBugReportView(views::Widget* parent,
                       Profile* profile,
                       TabContents* tab) {
  NOTIMPLEMENTED();
}

void ShowClearBrowsingDataView(views::Widget* parent,
                               Profile* profile) {
  NOTIMPLEMENTED();
}

void ShowSelectProfileDialog() {
  NOTIMPLEMENTED();
}

void ShowImporterView(views::Widget* parent,
                      Profile* profile) {
  NOTIMPLEMENTED();
}

void ShowBookmarkManagerView(Profile* profile) {
  NOTIMPLEMENTED();
}

void ShowAboutChromeView(views::Widget* parent,
                         Profile* profile) {
  NOTIMPLEMENTED();
}

void ShowHtmlDialogView(gfx::NativeWindow parent, Browser* browser,
                        HtmlDialogUIDelegate* delegate) {
  NOTIMPLEMENTED();
}

void ShowPasswordsExceptionsWindowView(Profile* profile) {
  NOTIMPLEMENTED();
}

void ShowKeywordEditorView(Profile* profile) {
  NOTIMPLEMENTED();
}

void ShowNewProfileDialog() {
  NOTIMPLEMENTED();
}

void ShowTaskManager() {
  NOTIMPLEMENTED();
}

void EditSearchEngine(gfx::NativeWindow parent,
                      const TemplateURL* template_url,
                      EditSearchEngineControllerDelegate* delegate,
                      Profile* profile) {
  NOTIMPLEMENTED();
}

void ShowRepostFormWarningDialog(gfx::NativeWindow parent_window,
                                 TabContents* tab_contents) {
  NOTIMPLEMENTED();
}

}  // namespace browser

void ShowOptionsWindow(OptionsPage page,
                       OptionsGroup highlight_group,
                       Profile* profile) {
  NOTIMPLEMENTED();
}
void ShowFontsLanguagesWindow(gfx::NativeWindow window,
                              FontsLanguagesPage page,
                              Profile* profile) {
  NOTIMPLEMENTED();
}
