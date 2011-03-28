// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/crypto_module_password_dialog.h"

#include "base/basictypes.h"
#include "base/crypto/crypto_module_blocking_password_delegate.h"
#include "base/synchronization/waitable_event.h"
#include "base/task.h"
#include "base/utf_string_conversions.h"
#include "content/browser/browser_thread.h"
#include "googleurl/src/gurl.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

namespace browser {

void ShowCryptoModulePasswordDialog(const std::string& slot_name,
                                    bool retry,
                                    CryptoModulePasswordReason reason,
                                    const std::string& server,
                                    CryptoModulePasswordCallback* callback) {
  DNOTIMPLEMENTED();
}

base::CryptoModuleBlockingPasswordDelegate*
    NewCryptoModuleBlockingDialogDelegate(
        CryptoModulePasswordReason reason,
        const std::string& server) {
  DNOTIMPLEMENTED();
  return NULL;
}

}  // namespace browser
