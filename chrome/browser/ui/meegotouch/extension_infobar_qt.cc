// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/resource/resource_bundle.h"
#include "chrome/browser/extensions/extension_host.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/extensions/extension_resource.h"
#include "chrome/browser/extensions/extension_infobar_delegate.h"
#include "grit/theme_resources.h"

class InfoBar;

InfoBar* ExtensionInfoBarDelegate::CreateInfoBar() {
  DNOTIMPLEMENTED();
  return NULL;
}
