// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Copyright (c) 2010, Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/meegotouch/js_modal_dialog_qt.h"

#include "ui/base/l10n/l10n_util.h"
#include "ui/base/message_box_flags.h"
#include "base/logging.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/ui/meegotouch/qt_util.h"
#include "chrome/browser/ui/app_modal_dialogs/js_modal_dialog.h"
#include "grit/generated_resources.h"
#include "grit/locale_settings.h"

/***************************************************************************
 *	JSModalDialogQt
 */
JSModalDialogQt::JSModalDialogQt(JavaScriptAppModalDialog* dialog,
                                   gfx::NativeWindow parent_window)
    	: jsDialog_(dialog) {
   int flag = jsDialog_->dialog_flags();
   DialogQt::DLG_TYPE type;
   if(flag == ui::MessageBoxFlags::kIsJavascriptAlert) {
        type = DialogQt::DLG_ALERT;
   } else if (flag == ui::MessageBoxFlags::kIsJavascriptConfirm) {
	type = DialogQt::DLG_CONFIRM;
   } else if (flag == ui::MessageBoxFlags::kIsJavascriptPrompt) {
	type = DialogQt::DLG_PROMPT;
   } else {
	type = DialogQt::DLG_ALERT;
   } 

   qDlgModel_ = new DialogQtModel(type, jsDialog_->display_suppress_checkbox(), 
			WideToUTF8(jsDialog_->title()).c_str(), WideToUTF8(jsDialog_->message_text()).c_str(),
			WideToUTF8(jsDialog_->default_prompt_text()).c_str(), jsDialog_->is_before_unload_dialog());

}

JSModalDialogQt::~JSModalDialogQt() {
    delete qDlgModel_;
}

int JSModalDialogQt::GetAppModalDialogButtons() const {
  switch (jsDialog_->dialog_flags()) {
    case ui::MessageBoxFlags::kIsJavascriptAlert:
      return ui::MessageBoxFlags::DIALOGBUTTON_OK;

    case ui::MessageBoxFlags::kIsJavascriptConfirm:
      return ui::MessageBoxFlags::DIALOGBUTTON_OK |
        ui::MessageBoxFlags::DIALOGBUTTON_CANCEL;

    case ui::MessageBoxFlags::kIsJavascriptPrompt:
      return ui::MessageBoxFlags::DIALOGBUTTON_OK;

    default:
      NOTREACHED();
      return 0;
  }
}

void JSModalDialogQt::ShowAppModalDialog() {
    Browser* browser = BrowserList::GetLastActive();
    BrowserWindowQt* browser_window = (BrowserWindowQt*)browser->window();
    browser_window->ShowDialog(qDlgModel_, this);
}

void JSModalDialogQt::ActivateAppModalDialog() {
    //qDlg_->activateWindow();
    //qDlg_->raise();
}

void JSModalDialogQt::CloseAppModalDialog() {
    HandleDialogResponse(DialogQt::Rejected, NULL);
}

void JSModalDialogQt::AcceptAppModalDialog() {
    HandleDialogResponse(DialogQt::Accepted, NULL);
}

void JSModalDialogQt::CancelAppModalDialog() {
    HandleDialogResponse(DialogQt::Rejected, NULL);
}

void JSModalDialogQt::HandleDialogResponse(int response_id, QString input, bool isSuppress) {
  switch (response_id) {
    case DialogQt::Accepted:
 	    if (ui::MessageBoxFlags::kIsJavascriptPrompt == jsDialog_->dialog_flags() 
			  && input != NULL) {
    	    jsDialog_->OnAccept(input.toStdWString(), isSuppress);
	    }else {
     	    jsDialog_->OnAccept(std::wstring(), isSuppress);
	    }
        break;

    case DialogQt::Rejected:
        jsDialog_->OnCancel(true);
        break;

    default:
        NOTREACHED();
  }

  // Now that the dialog is gone, delete itselt here.
  delete this;
}

void JSModalDialogQt::OnDialogResponse(int response_id, QString input1, QString input2, bool isSuppress) {
  HandleDialogResponse(response_id, input1, isSuppress);
}

/***************************************************************************
 * NativeAppModalDialog, static:
 */
NativeAppModalDialog* NativeAppModalDialog::CreateNativeJavaScriptPrompt(
    JavaScriptAppModalDialog* dialog,
    gfx::NativeWindow parent_window) {
  return new JSModalDialogQt(dialog, parent_window);
}

