/*
 * Copyright (c) 2010, Intel Corporation. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above 
 * copyright notice, this list of conditions and the following disclaimer 
 * in the documentation and/or other materials provided with the 
 * distribution.
 *     * Neither the name of Intel Corporation nor the names of its 
 * contributors may be used to endorse or promote products derived from 
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "ui/base/l10n/l10n_util.h"
#include "base/string_number_conversions.h"
#include "base/string16.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/browser_window.h"
#include "chrome/browser/download/download_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"

#include "ui/base/message_box_flags.h"
#include "chrome/browser/ui/meegotouch/download_in_progress_dialog_qt.h"

DownloadInProgressDialogQt::DownloadInProgressDialogQt(Browser* browser)
    : browser_(browser) {
  int download_count = browser->profile()->GetDownloadManager()->
      in_progress_count();

  std::string warning_text;
  std::string explanation_text;
  std::string ok_button_text;
  std::string cancel_button_text;
  string16 product_name = l10n_util::GetStringUTF16(IDS_PRODUCT_NAME);
  if (download_count == 1) {
    warning_text =
        l10n_util::GetStringFUTF8(IDS_SINGLE_DOWNLOAD_REMOVE_CONFIRM_WARNING,
                                  product_name);
    explanation_text =
        l10n_util::GetStringFUTF8(
            IDS_SINGLE_DOWNLOAD_REMOVE_CONFIRM_EXPLANATION,
            product_name);
    ok_button_text = l10n_util::GetStringUTF8(
        IDS_SINGLE_DOWNLOAD_REMOVE_CONFIRM_OK_BUTTON_LABEL);
    cancel_button_text = l10n_util::GetStringUTF8(
        IDS_SINGLE_DOWNLOAD_REMOVE_CONFIRM_CANCEL_BUTTON_LABEL);
  } else {
    warning_text =
        l10n_util::GetStringFUTF8(IDS_MULTIPLE_DOWNLOADS_REMOVE_CONFIRM_WARNING,
                                  product_name,
                                  base::IntToString16(download_count));
    explanation_text =
        l10n_util::GetStringFUTF8(
            IDS_MULTIPLE_DOWNLOADS_REMOVE_CONFIRM_EXPLANATION, product_name);
    ok_button_text = l10n_util::GetStringUTF8(
        IDS_MULTIPLE_DOWNLOADS_REMOVE_CONFIRM_OK_BUTTON_LABEL);
    cancel_button_text = l10n_util::GetStringUTF8(
        IDS_MULTIPLE_DOWNLOADS_REMOVE_CONFIRM_CANCEL_BUTTON_LABEL);
  }

  qDlgModel_ = new DialogQtModel(DialogQt::DLG_CONFIRM, false, 
			warning_text.c_str(), explanation_text.c_str(), NULL, false);
}


void DownloadInProgressDialogQt::show() {
  ((BrowserWindowQt* )browser_->window())->ShowDialog(qDlgModel_, this);
}

void DownloadInProgressDialogQt::OnDialogResponse(int result, QString input, QString input2, bool isSuppress) {
  browser_->InProgressDownloadResponse(result == DialogQt::Accepted);

  // Now that the dialog is gone, delete itselt here.
  delete this;
}
