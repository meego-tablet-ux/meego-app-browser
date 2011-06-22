// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/extensions/extension_uninstall_dialog.h"
#include "chrome/common/extensions/extension.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

class Profile;

//static
void ExtensionUninstallDialog::Show(
    Profile* profile,
    ExtensionUninstallDialog::Delegate* delegate,
    const Extension* extension,
    SkBitmap* icon) {
    DNOTIMPLEMENTED();
}
