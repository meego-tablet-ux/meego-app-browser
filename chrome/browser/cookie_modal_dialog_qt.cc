// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/cookie_modal_dialog.h"

#include "ui/base/l10n/l10n_util.h"
#include "base/logging.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/views/cookie_prompt_view.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "chrome/common/pref_names.h"
#include "grit/generated_resources.h"
#include "grit/locale_settings.h"

void CookiePromptModalDialog::CreateAndShowDialog() {
  dialog_ = CreateNativeDialog();
}

void CookiePromptModalDialog::AcceptWindow() {
  DNOTIMPLEMENTED();
}

void CookiePromptModalDialog::CancelWindow() {
  DNOTIMPLEMENTED();
}

NativeDialog CookiePromptModalDialog::CreateNativeDialog() {
  DNOTIMPLEMENTED();
  return NULL;
}
