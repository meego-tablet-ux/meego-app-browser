/*
 * Copyright (c) 2011, Intel Corporation. All rights reserved.
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
#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/stl_util-inl.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/ui/meegotouch/crash_modal_dialog_qt.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"
#include "grit/generated_resources.h"
#include "grit/locale_settings.h"
#include "grit/chromium_strings.h"

CrashAppModalDialog::CrashAppModalDialog (
    TabContents* tab_contents)
    : AppModalDialog (tab_contents,UTF16ToWide(l10n_util::GetStringUTF16(IDS_CRASH_TAB_HEAD_CONTENT))){
  model_ = new CrashTabQtModel();
  }

void CrashAppModalDialog::CreateAndShowDialog(){
  Browser* browser = BrowserList::GetLastActive();
  BrowserWindowQt *browser_window = (BrowserWindowQt*)browser->window();
  browser_window->ShowCrashDialog(model_, this);
}

CrashAppModalDialog::~CrashAppModalDialog(){
  delete model_;
}

void CrashAppModalDialog::HandleDialogResponse(){
  CompleteDialog();
  delete this;
}
