// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_modal_dialogs/app_modal_dialog.h"

#include "ui/base/l10n/l10n_util.h"
#include "ui/base/message_box_flags.h"
#include "base/logging.h"
#include "base/string_util.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/browser_window.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/browser/tab_contents/tab_contents_view.h"
#include "grit/generated_resources.h"
#include "grit/locale_settings.h"

AppModalDialog::~AppModalDialog() {
}

void AppModalDialog::CreateAndShowDialog() {
}

// static
void AppModalDialog::ActivateModalDialog() {
  DNOTIMPLEMENTED();
}

void AppModalDialog::CloseModalDialog() {
  DNOTIMPLEMENTED();
}

void AppModalDialog::AcceptWindow() {
  DNOTIMPLEMENTED();
}

void AppModalDialog::CancelWindow() {
  DNOTIMPLEMENTED();
}
