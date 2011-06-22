// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/options/options_window.h"

#include "ui/base/l10n/l10n_util.h"
#include "base/message_loop.h"
#include "base/scoped_ptr.h"
#include "chrome/browser/accessibility_events.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/prefs/pref_member.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/window_sizer.h"
#include "content/common/notification_service.h"
#include "chrome/common/pref_names.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"

///////////////////////////////////////////////////////////////////////////////
// Factory/finder method:

// ShowOptionsWindow for non ChromeOS build. For ChromeOS build, see
// chrome/browser/chromeos/options/options_window_view.h
void ShowOptionsWindow(OptionsPage page,
                       OptionsGroup highlight_group,
                       Profile* profile) {
  DNOTIMPLEMENTED();
}
