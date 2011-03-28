// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include <QWidget>

#include "base/utf_string_conversions.h"
#include "chrome/browser/importer/importer_host.h"
#include "chrome/browser/importer/importer_observer.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"

namespace importer {

void ShowImportProgressDialog(QWidget* parent,
                              uint16 items,
                              ImporterHost* importer_host,
                              ImporterObserver* importer_observer,
                              const ProfileInfo& browser_profile,
                              Profile* profile,
                              bool first_run) {
  DNOTIMPLEMENTED();
}

}  // namespace importer
