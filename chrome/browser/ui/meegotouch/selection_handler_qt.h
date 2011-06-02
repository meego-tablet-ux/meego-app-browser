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

#ifndef CHROME_BROWSER_SELECTION_HANDLER_QT_H_
#define CHROME_BROWSER_SELECTION_HANDLER_QT_H_
#pragma once

#include <QObject>

class SelectionHandlerQt;
class BrowserWindowQt;


/**
 ** Helper class to interacivie with qml
 ***/
class SelectionHandlerQtImpl: public QObject {
    Q_OBJECT

        public:
    SelectionHandlerQtImpl(SelectionHandlerQt* handler);
  virtual ~SelectionHandlerQtImpl() {};

public:
  void Show();
  void Set(int start_x, int start_y, int end_x, int end_y);
  void SetScale(double scale_);
  void SetPinchOffset(int offset_x_, int offset_y_);
  void SetPinchState(bool in_pinch_);
  void SetHandlerHeight(int height_);
  void Hide();
  

 public Q_SLOTS:

 Q_SIGNALS:
  void show();
  void set(int start_x, int start_y, int end_x, int end_y);
  void hide();

  void setScale(double scale_);
  void setPinchOffset(int offset_x_, int offset_y_);
  void setPinchState(bool in_pinch_);
  void setHandlerHeight(int height_);
  

private:

  SelectionHandlerQt* handler_;
};

class SelectionHandlerQt 
{
public:
    SelectionHandlerQt(BrowserWindowQt* window);
    virtual ~SelectionHandlerQt();

public:
    void setHandlerAt(int start_x, int start_y, int end_x, int end_y);
    void hideHandler();
    void showHandler();

    void setScale(double scale_);
    void setPinchOffset(int offset_x_, int offset_y_);
    void setPinchState(bool in_pinch_);
    void setHandlerHeight(int height_);
    

private:
    SelectionHandlerQtImpl* impl_;
    BrowserWindowQt* window_;
};

#endif  // CHROME_BROWSER_SELECTION_HANDLER_QT_H_
