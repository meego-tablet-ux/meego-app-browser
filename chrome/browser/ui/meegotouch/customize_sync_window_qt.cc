// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "ui/base/l10n/l10n_util.h"
#include "base/message_loop.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/options/options_window.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/glue/data_type_controller.h"
#include "chrome/browser/sync/profile_sync_service.h"
#include "chrome/common/pref_names.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "grit/locale_settings.h"

///////////////////////////////////////////////////////////////////////////////
// Factory/finder method:

void ShowCustomizeSyncWindow(Profile* profile) {
  DNOTIMPLEMENTED();
}

bool CustomizeSyncWindowOk() {
  DNOTIMPLEMENTED();
  return true;
}

void CustomizeSyncWindowCancel() {
  DNOTIMPLEMENTED();
}
