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

#include <QtGui>
#include <QtDeclarative>

#include <QDeclarativeEngine>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDeclarativeItem>
#include <QGraphicsLineItem>

#include <stdio.h>
#include "webkit/glue/hwfmenu_qt.h"


CallFMenuClass::CallFMenuClass(QObject *parent)
  : QObject(parent),
  m_menu_hiden(true),
  m_volume_percentage(20),
  m_video_current(0),
  m_video_total(0),
  m_events(1),
  m_artype(0),
  m_launchednum(0),

  m_play_s(1),
  m_backward_s(0),
  m_forward_s(0),
  m_fullscreen_s(1),
  m_ext_s(0) 
{
    
}

QString CallFMenuClass::getURLName() const
{
  return m_url;
}

void CallFMenuClass::setURLName(const QString &url)
{
  m_url = url;
}

int CallFMenuClass::getVideoCurTime() const
{
  return m_video_current;
}

void CallFMenuClass::setVideoCurTime(int cur)
{
  m_video_current = cur;

  return;
}

int CallFMenuClass::getVideoDurTime() const
{
  return m_video_total;
}

void CallFMenuClass::setVideoDurTime(int dur)
{
  m_video_total = dur;
  return;
}

bool CallFMenuClass::getMenuHiden() const
{
  return m_menu_hiden;
}

void CallFMenuClass::setMenuHiden(bool hiden)
{
  m_menu_hiden = hiden;
  return;
}

int CallFMenuClass::getEvents() const
{
  return m_events;
}

void CallFMenuClass::relEvents()
{
  if(m_events) m_events--;
  return;
}

int CallFMenuClass::getLaunchedFlag() const
{
  return m_launchednum;
}

bool CallFMenuClass::setLaunchedFlag(int n)
{
  m_launchednum = n;
  return true;
}

int CallFMenuClass::getARtype() const
{
  return m_artype;
}

void CallFMenuClass::setARtype(int t)
{
  m_artype = t;
  return;
}

int CallFMenuClass::getVolumePercentage() const
{
  return m_volume_percentage;
}

void CallFMenuClass::setVolumePercentage(int s)
{
  m_volume_percentage = s;
  return;
}

void CallFMenuClass::SyncWriteStatus(int backward, 
  int play,
  int forward,
  int currentvideotime,
  int currentvolumpercetage,
  int fullscreen,
  int ar,
  int videoduration,
  int ext)
{
  m_backward_s = backward;
  m_forward_s = forward;
  m_play_s = play;

  m_volume_percentage = currentvolumpercetage;
  m_fullscreen_s = fullscreen;
  m_artype = ar;

  if(currentvideotime&&videoduration){
    m_video_current = currentvideotime;
    m_video_total = videoduration;
  }

  m_ext_s = ext;
  m_events++;

  return;
}

#include "webkit/glue/moc_hwfmenu_qt.cc"
