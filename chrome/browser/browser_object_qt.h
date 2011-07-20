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

#ifndef BROWSER_OBJECT_QT_H
#define BROWSER_OBJECT_QT_H

#include <QObject>
#include <QEvent>
#include <QDeclarativeItem>

class QDeclarativeView;
class QApplication;

// Implements a 'BrowserObject' class used by QML
class BrowserObject : public QObject {
  Q_OBJECT
public:
  BrowserObject(QObject *obj = 0); 
  QApplication * getApplication();
  QDeclarativeView * getDeclarativeView();

public Q_SLOTS:
  void runMain(const QStringList &parameters, QObject *object);
  void handleCall(const QStringList &parameters);

signals:
  // emit call signal to send parameters to browser core
  void call(const QStringList &parameters);

private:
  // this is a helper function to adjust paths
  void adjustPaths(QStringList &);
  // convert args to c-style
  void convertArgs(const QStringList &);

  QDeclarativeView *view_;
  //pointer list to store arguments
  QVector<char *> argv_;
  // argument list in qbytearray
  QList<QByteArray> arg_list_;
};

extern BrowserObject *g_browser_object;

#endif
