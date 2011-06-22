// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/browser_dialogs.h"

#include "base/process_util.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tab_contents/tab_contents_wrapper.h"
#include "chrome/common/logging_chrome.h"
#include "content/common/result_codes.h"
#include "content/browser/renderer_host/render_process_host.h"
#include "content/browser/renderer_host/render_view_host.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace browser {

void ShowHungRendererDialog(TabContents* contents) {
  DNOTIMPLEMENTED();
}

void HideHungRendererDialog(TabContents* contents) {
  DNOTIMPLEMENTED();
}

}  // namespace browser
