// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/utf_string_conversions.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/webui/html_dialog_ui.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "content/common/native_web_keyboard_event.h"
#include "ipc/ipc_message.h"

namespace browser {

gfx::NativeWindow ShowHtmlDialog(gfx::NativeWindow parent, Profile* profile,
                                 HtmlDialogUIDelegate* delegate) {
  DNOTIMPLEMENTED();
  return NULL;
}

} // namespace browser

