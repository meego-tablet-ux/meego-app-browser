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

#include "chrome/browser/ui/meegotouch/menu_qt.h"

#include <map>

#include <QDeclarativeEngine>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDeclarativeItem>
#include <QGraphicsLineItem>
#include <QStringList>

#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/menu_model.h"
#include "ui/base/message_box_flags.h"
#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/stl_util-inl.h"
#include "base/utf_string_conversions.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"
#include "dialog_qt.h"
#include "grit/generated_resources.h"
#include "grit/locale_settings.h"

/*******************************************************************************************
 * Implement class DialogQt
 **/
DialogQt::DialogQt(BrowserWindowQt* window)
    : window_(window), listener_(NULL), model_(NULL) {
  impl_ = new DialogQtImpl(this);

  QDeclarativeView* view = window_->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();
  context->setContextProperty("browserDialogObject", impl_);
}

DialogQt::~DialogQt() {
  delete impl_;
}

void DialogQt::Popup() {
  impl_->Popup();
}

QString DialogQt::GetInput() {
    return NULL;
}

void DialogQt::notifyResultListener(int result, QString input1, QString input2, bool isSuppress) {
  if(listener_ != NULL)
      listener_->OnDialogResponse(result, input1, input2, isSuppress);

}

void DialogQt::SetModelAndListener(DialogQtModel* model, DialogQtResultListener* listener) {
  model_ = model;
  listener_ = listener;

  if(model_ == NULL)
    return;

  QDeclarativeView* view = window_->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();
  context->setContextProperty("browserDialogModel", model_);
}

/*******************************************************************************************
 * Implement class DialogQtImpl
 **/

DialogQtImpl::DialogQtImpl(DialogQt* dialog):
      QObject(),
      dialog_(dialog) {

}
 
void DialogQtImpl::Popup() {
    DLOG(INFO)<<__FUNCTION__;
    emit popup();
}

void DialogQtImpl::CloseDialog() {
    DLOG(INFO)<<__FUNCTION__;
    emit dismiss();
}

void DialogQtImpl::OnButtonClicked(int number, QString input1, QString input2, bool isSuppress) {
    DLOG(INFO)<<__FUNCTION__;
    int result = DialogQt::Unknown;   
    if (number == 1)
	    result = DialogQt::Accepted;
    else if(number == 2)
	    result = DialogQt::Rejected;
    
    	    DLOG(INFO)<<"input1: "<<input1.toStdString();
    	    DLOG(INFO)<<"input2: "<<input2.toStdString();

    dialog_->notifyResultListener(result, input1, input2, isSuppress);

    CloseDialog();
}


/*******************************************************************************************
 * Implement class DialogQtImpl
 **/
DialogQtModel::DialogQtModel(DialogQt::DLG_TYPE flag, bool isSuppress, const char* title, const char* content,
                        const char* defaultPrompt, bool isBeforeUnload)
		: isSuppress_(isSuppress), title_(QString(title)), type_(flag),
		  content_(QString(content)), defaultPrompt_(QString(defaultPrompt)) {
    
    suppressOption_ = QString::fromStdString(l10n_util::GetStringUTF8(
        			IDS_JAVASCRIPT_MESSAGEBOX_SUPPRESS_OPTION));
    leftButtonText_ = QString::fromStdString(l10n_util::GetStringUTF8(IDS_OK));
    rightButtonText_ = QString::fromStdString(l10n_util::GetStringUTF8(IDS_CANCEL));

    if( isBeforeUnload ) {
	type_ = DialogQt::DLG_CONFIRM;
	//title_ = QString("Confirm navigation");
	leftButtonText_ = QString::fromStdString(l10n_util::GetStringUTF8(
      				IDS_BEFOREUNLOAD_MESSAGEBOX_OK_BUTTON_LABEL));
	rightButtonText_ = QString::fromStdString(l10n_util::GetStringUTF8(
        			IDS_BEFOREUNLOAD_MESSAGEBOX_CANCEL_BUTTON_LABEL));
    }

    if( type_ == DialogQt::DLG_AUTH ) {
	leftButtonText_ = QString::fromStdString(l10n_util::GetStringUTF8(
      				IDS_LOGIN_DIALOG_OK_BUTTON_LABEL));
	usernameText_ = QString::fromStdString(l10n_util::GetStringUTF8(
				IDS_LOGIN_DIALOG_USERNAME_FIELD));
	passwordText_ = QString::fromStdString(l10n_util::GetStringUTF8(
				IDS_LOGIN_DIALOG_PASSWORD_FIELD));

    }
}

#include "moc_dialog_qt.cc"
