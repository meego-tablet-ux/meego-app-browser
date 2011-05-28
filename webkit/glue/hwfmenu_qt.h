 /****************************************************************************
 **
 ** Copyright (c) 2011 Intel Corporation.
 ** All rights reserved.
 **
 ** Redistribution and use in source and binary forms, with or without
 ** modification, are permitted provided that the following conditions are
 ** met:
 **   * Redistributions of source code must retain the above copyright
 **     notice, this list of conditions and the following disclaimer.
 **   * Redistributions in binary form must reproduce the above copyright
 **     notice, this list of conditions and the following disclaimer in
 **     the documentation and/or other materials provided with the
 **     distribution.
 **   * Neither the name of Intel Corporation nor the names of its
 **     contributors may be used to endorse or promote
 **     products derived from this software without specific prior written
 **     permission.
 **
 ** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 ** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 ** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 ** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 ** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 ** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 ** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 ** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 ** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 ** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 ** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 **
 ****************************************************************************/

#ifndef CALL_ZFMENUCLASS_H
#define CALL_ZFMENUCLASS_H
#pragma once

#include <QtGui>
#include <QtDeclarative>
#include <QDeclarativeEngine>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDeclarativeItem>
#include <QGraphicsLineItem>

#include <stdio.h>

class CallFMenuClass : public QObject
{
  Q_OBJECT

public:
  CallFMenuClass(QObject *parent = 0);
  ~CallFMenuClass() {}

  QString getURLName() const;
  void setURLName(const QString &);

  int getVideoCurTime() const;
  void setVideoCurTime(int);

  int getVideoDurTime() const;
  void setVideoDurTime(int);

  int getVolumePercentage() const;
  void setVolumePercentage(int);

  int getEvents() const;
  void relEvents();

  int getARtype() const;
  void setARtype(int);

  bool getMenuHiden() const;

  void VideoActive(int c, int t) {emit videoRun(c,t);}
  void VideoRun() {emit videoRun(m_video_current,m_video_total);}
  void SyncRead() {emit syncRead(m_backward_s,m_play_s,m_forward_s,m_fullscreen_s,m_artype);}


public Q_SLOTS:
  void cMethod() {
    emit cppSignal();
    emit videoRun(m_video_current,m_video_total);
    emit syncRead(m_backward_s,m_play_s,m_forward_s,m_fullscreen_s,m_artype);
  }


  void SyncWriteStatus(int,int,int,int,int,int,int,int,int);
  void setMenuHiden(bool);

Q_SIGNALS:
  void popupAt(int x, int y);
  void initSAt(int b,int p,int f,int cv,int ca,int s,int t);
  void cppSignal();
  void syncRead(int cback,int cplay,int cforward,int cfullscreen,int ctype);
  void videoRun(int current, int total);
  
private:
  QString m_url;
  bool m_menu_hiden;
  int m_volume_percentage;
  int m_video_current;
  int m_video_total;

  int m_events;
  int m_artype;

  int m_play_s;
  int m_backward_s;
  int m_forward_s;
  int m_fullscreen_s;
  int m_ext_s;
};
#endif
