// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/l10n/l10n_util.h"
#include "base/rand_util.h"
#include "base/string_util.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/extensions/extension_install_ui.h"
#include "chrome/common/extensions/extension.h"
#include "grit/generated_resources.h"
#include "skia/ext/image_operations.h"

class Profile;

void ShowExtensionInstallDialog2(
    Profile* profile,
    ExtensionInstallUI::Delegate* delegate,
    const Extension* extension,
    SkBitmap* icon,
    const std::vector<string16>& permissions,
    ExtensionInstallUI::PromptType type) {
  DNOTIMPLEMENTED();
}
