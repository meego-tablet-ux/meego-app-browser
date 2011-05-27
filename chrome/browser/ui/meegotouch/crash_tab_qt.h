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

#ifndef CHROME_BROWSER_QT_CRASH_TAB_QT_H_
#define CHROME_BROWSER_QT_CRASH_TAB_QT_H_
#pragma once

#include <QObject>
#include <QString>
#include "browser_window_qt.h"

class BrowserWindowQt;
class CrashTabQtImpl;


/**
 * Qml dialg
 **/
class CrashTabQt {
  
public:

  CrashTabQt(BrowserWindowQt* window);

  virtual ~CrashTabQt();

  void Popup();

private:
  BrowserWindowQt* window_;

  CrashTabQtImpl* impl_;
};

/**
 * Helper class to interacivie with qml
 **/
class CrashTabQtImpl: public QObject {

  Q_OBJECT

public:
  CrashTabQtImpl(CrashTabQt* crashtab_qt);

  virtual ~CrashTabQtImpl() {};

  void Popup();
 
  void CloseModel();
        
 public Q_SLOTS:
  void onCloseButtonClicked();

 Q_SIGNALS:
  void popup();
  
  void dismiss();

 private:
  CrashTabQt* crashtab_qt_;

};

#endif
