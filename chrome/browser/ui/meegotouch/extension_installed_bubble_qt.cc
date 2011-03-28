// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/i18n/rtl.h"
#include "base/message_loop.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/extensions/extension_action.h"
#include "content/common/notification_details.h"
#include "content/common/notification_source.h"
#include "content/common/notification_type.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace {

const int kHorizontalColumnSpacing = 10;
const int kIconPadding = 3;
const int kIconSize = 43;
const int kTextColumnVerticalSpacing = 7;
const int kTextColumnWidth = 350;

// When showing the bubble for a new browser action, we may have to wait for
// the toolbar to finish animating to know where the item's final position
// will be.
const int kAnimationWaitRetries = 10;
const int kAnimationWaitMS = 50;

// Padding between content and edge of info bubble.
const int kContentBorder = 7;

}  // namespace

namespace browser {

void ShowExtensionInstalledBubble(
    const Extension* extension,
    Browser* browser,
    const SkBitmap& icon,
    Profile* profile) {
  DNOTIMPLEMENTED();
}

} // namespace browser
