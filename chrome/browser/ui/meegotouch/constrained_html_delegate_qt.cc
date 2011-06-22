// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/constrained_html_ui.h"

#include "chrome/browser/ui/webui/html_dialog_tab_contents_delegate.h"
#include "chrome/browser/ui/webui/html_dialog_ui.h"
#include "content/browser/renderer_host/render_view_host.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "chrome/browser/ui/meegotouch/qt_util.h"
#include "chrome/browser/ui/meegotouch/tab_contents_container_qt.h"
#include "content/common/notification_source.h"
#include "ipc/ipc_message.h"
#include "ui/gfx/rect.h"

// static
ConstrainedWindow* ConstrainedHtmlUI::CreateConstrainedHtmlDialog(
    Profile* profile,
    HtmlDialogUIDelegate* delegate,
    TabContents* overshadowed) {
  NOTIMPLEMENTED();
  return 0;
}
