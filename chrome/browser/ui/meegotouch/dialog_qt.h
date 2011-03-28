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

#ifndef CHROME_BROWSER_QT_DIALOG_QT_H_
#define CHROME_BROWSER_QT_DIALOG_QT_H_
#pragma once

#include <QObject>
#include <QString>
#include "browser_window_qt.h"
#include "browser_window_qt.h"

class BrowserWindowQt;
class DialogQtImpl;
class DialogQtModel;
class DialogQtResultListener;

/**
 * Qml dialg
 **/
class DialogQt {
  friend class DialogQtImpl;

public:
  static const int Unknown = 0;

  static const int Accepted = 1;

  static const int Rejected = 2;

  enum DLG_TYPE {DLG_ALERT=0, DLG_CONFIRM, DLG_PROMPT, DLG_UNLOAD, DLG_AUTH};

  DialogQt(BrowserWindowQt* window);

  virtual ~DialogQt();

  void Popup();

  void SetModelAndListener(DialogQtModel* model, DialogQtResultListener* listener);

  QString GetInput();

  void notifyResultListener(int result, QString input1, QString input2, bool isSuppress);

private:
  BrowserWindowQt* window_;

  DialogQtImpl* impl_;

  DialogQtModel* model_;

  DialogQtResultListener* listener_;

};

/**
 * Helper class to interacivie with qml
 **/
class DialogQtImpl:public QObject {
  Q_OBJECT;

 public:
  DialogQtImpl(DialogQt* dialog);

  virtual ~DialogQtImpl() {};

  void Popup();
 
  void CloseDialog();
        
 public Q_SLOTS:
  void OnButtonClicked(int number, QString input1, QString input2, bool isSuppress);

 Q_SIGNALS:
  void popup();
  
  void dismiss();

  void buttonClicked(int number, QString input1, QString input2, bool isSuppress);

 private:
  DialogQt* dialog_;

};

/**
 * Helper class to layout the dialog
 */

class DialogQtModel:public QObject {
  Q_OBJECT;

  public:

    DialogQtModel(DialogQt::DLG_TYPE flag, bool isSuppress, const char* title, const char* content,
			const char* defaultPrompt, bool isBeforeUnload);

    virtual ~DialogQtModel() { };

  public Q_SLOTS:
    int getDialogType() {
	return (int)type_;
    }

    QString getDialogTitle() {
	return title_;
    }

    QString getDialogContent() {
	return content_;
    }

    QString getDefaultPrompt() {
	return defaultPrompt_;
    }

    QString getLeftButtonText() {
	return leftButtonText_;
    }

    QString getRightButtonText() {
	return rightButtonText_;
    }

    QString getSuppressText() {
	return suppressOption_;
    }

    QString getUsernameText() {
	return usernameText_;
    }

    QString getPasswordText() {
	return passwordText_;
    }

    bool isSuppress() {
	return isSuppress_;
    }

  private:

    bool isSuppress_;

    QString title_;

    QString content_;

    QString defaultPrompt_;

    QString leftButtonText_;

    QString rightButtonText_;

    QString suppressOption_;

    QString usernameText_;

    QString passwordText_;

    DialogQt::DLG_TYPE type_;
};

class DialogQtResultListener {
  public:
    virtual void OnDialogResponse(int result, QString input1, QString input2, bool isSuppress) = 0;
};

#endif
