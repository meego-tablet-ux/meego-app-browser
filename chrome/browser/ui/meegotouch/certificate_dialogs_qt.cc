// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "base/base64.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "base/task.h"
#include "content/browser/browser_thread.h"
#include "chrome/browser/ui/shell_dialogs.h"
#include "chrome/common/net/x509_certificate_model.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

void ShowCertSelectFileDialog(SelectFileDialog* select_file_dialog,
                              SelectFileDialog::Type type,
                              const FilePath& suggested_path,
                              gfx::NativeWindow parent,
                              void* params) {
  DNOTIMPLEMENTED();
}

void ShowCertExportDialog(gfx::NativeWindow parent,
                          net::X509Certificate::OSCertHandle cert) {
  DNOTIMPLEMENTED();
}
