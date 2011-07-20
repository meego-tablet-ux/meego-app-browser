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

#include <QDeclarativeItem>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QApplication>
#include <QDesktopWidget>
#include <QGraphicsScene>
#include <QObject>
#include <QFile>
#include "browser_wrapper_qt.h"

BrowserWrapper::BrowserWrapper(QObject *obj)
  : QObject(obj), view_(NULL), ignore_once_(false)
{
  getBrowserArgs(getApplication()->arguments());
  // post an event with event handler and then we can get the control 
  getApplication()->postEvent(this, new QEvent(Internal_TYPE));
}

bool BrowserWrapper::event(QEvent *e)
{
  if (e->type() == Internal_TYPE) {
    // get the control and run browser
    emit load(arg_sl_);
    return true;
  }
  return QObject::event(e);
}

void BrowserWrapper::getBrowserArgs(const QStringList &parameters)
{
  if (parameters.length() == 0) {
    return;
  }
  arg_sl_.clear();

  QString all;
  for (int i = 1; i < parameters.length(); i++) {
    if (parameters[i] == "--cmd" && i + 1 < parameters.length()) {
      all += parameters[++i];
      all += " ";
    } else if (parameters[i] == "--cdata" && i + 1 < parameters.length()) {
      all += parameters[++i];
      all += " ";
    }
  }
  all = all.trimmed();
  arg_sl_ = all.split(' ', QString::SkipEmptyParts);
  //add the first parameter (app path and name) for browser needs it
  arg_sl_.push_front(parameters.at(0));
}

void BrowserWrapper::arguments(const QStringList &parameters)
{
  QString all;
  for (int i = 0 ; i < parameters.size(); i++) {
    all += parameters.at(i);
  }
  all = all.trimmed();
  QStringList args = all.split(' ', QString::SkipEmptyParts);
  // meego-qml-launcher will emit parameters after browser starts, so ingore once
  if (!ignore_once_) {
    if (arg_sl_ == args) {
      ignore_once_ = true;
      arg_sl_.clear();
      return;
    }
  }

  emit call(args);
}

bool BrowserWrapper::appMode()
{
  for (int i = 0; i < arg_sl_.size(); i++ ) {
    QString str = arg_sl_.at(i);
    if (str.startsWith("--app=")) {
      return true;
    }
  }
  return false;
}

void BrowserWrapper::transfer(QObject *obj)
{
  // here we set objects as NULL to avoid qml warnings
  view_ = static_cast<QDeclarativeView *>(obj);
  if (view_) {
    QDeclarativeEngine *engine = view_->engine();
    QDeclarativeContext *context = engine->rootContext();
    context->setContextProperty("dpiX", getApplication()->desktop()->logicalDpiX());
    context->setContextProperty("dpiY", getApplication()->desktop()->logicalDpiY());

    bool fullscreen = false;
    context->setContextProperty("is_fullscreen", fullscreen);
    context->setContextProperty("is_appmode", appMode());

    context->setContextProperty("browserToolbarModel", NULL);
    context->setContextProperty("browserWindow", NULL);
    context->setContextProperty("downloadsObject", NULL);
    context->setContextProperty("bookmarkBubbleObject", NULL);
    context->setContextProperty("browserMenuObject", NULL);
    context->setContextProperty("PopupListModel", NULL);
    context->setContextProperty("fullscreenBubbleObject", NULL);
    context->setContextProperty("selectFileDialogObject", NULL);
    context->setContextProperty("browserDialogObject", NULL);
    context->setContextProperty("browserCrashTabObject", NULL);
    context->setContextProperty("browserNewTabObject", NULL);
    context->setContextProperty("tabSideBarModel", NULL);
    context->setContextProperty("findBarModel", NULL);
    context->setContextProperty("selectionHandler", NULL);
    context->setContextProperty("bookmarkOthersGridModel", NULL);
    context->setContextProperty("bookmarkBarGridModel", NULL);
    context->setContextProperty("sslDialogModel", NULL);
    context->setContextProperty("infobarContainerModel", NULL);
    context->setContextProperty("bookmarkBarModel", NULL);
    context->setContextProperty("autocompleteEditViewModel", NULL);
    context->setContextProperty("autocompletePopupViewModel", NULL);
    context->setContextProperty("tabSideBarModel", NULL);
    context->setContextProperty("findBarModel", NULL);
    context->setContextProperty("selectionHandler", NULL);
    context->setContextProperty("bookmarkOthersGridModel", NULL);
    context->setContextProperty("bookmarkBarGridModel", NULL);
    context->setContextProperty("sslDialogModel", NULL);
    context->setContextProperty("infobarContainerModel", NULL);
    context->setContextProperty("bookmarkBarModel", NULL);
  }
}

QApplication * BrowserWrapper::getApplication()
{
  return static_cast<QApplication *>(QApplication::instance());
}

QDeclarativeView * BrowserWrapper::getDeclarativeView()
{
  return view_;
}

#include "moc_browser_wrapper_qt.cc"

