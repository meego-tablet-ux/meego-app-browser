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
#include "crash_tab_qt.h"
#include "grit/generated_resources.h"
#include "grit/locale_settings.h"
#include "grit/chromium_strings.h"

/*******************************************************************************************
 * Implement class CrashTabQt
 **/
CrashTabQt::CrashTabQt(BrowserWindowQt* window)
    : window_(window){
  impl_ = new CrashTabQtImpl(this);
  
  QDeclarativeView* view = window_->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();
  context->setContextProperty("browserCrashTabObject", impl_);

  QString q_headContent = QString::fromUtf8(l10n_util::GetStringUTF8(IDS_CRASH_TAB_HEAD_CONTENT).c_str());
  context->setContextProperty("headContent", q_headContent);

  QString q_bodyContent = QString::fromUtf8(l10n_util::GetStringUTF8(IDS_CRASH_TAB_BODY_CONTENT).c_str());
  context->setContextProperty("bodyContent", q_bodyContent);

  QString q_closeButtonContent = QString::fromUtf8(l10n_util::GetStringUTF8(IDS_CRASH_TAB_CLOSE_BUTTON_CONTENT).c_str());
  context->setContextProperty("closeButtonContent", q_closeButtonContent);   
}

CrashTabQt::~CrashTabQt() {
  delete impl_;
}

void CrashTabQt::Popup() {
  impl_->Popup();
}

/*******************************************************************************************
 * Implement class CrashTabQtImpl
 **/

CrashTabQtImpl::CrashTabQtImpl(CrashTabQt* crashtab_qt):
    QObject(),
    crashtab_qt_(crashtab_qt) {
}
 
void CrashTabQtImpl::Popup() {
    emit popup();
}

void CrashTabQtImpl::CloseModel() {
    emit dismiss();
}

void CrashTabQtImpl::onCloseButtonClicked( ) {
    CloseModel();
}

#include "moc_crash_tab_qt.cc"
