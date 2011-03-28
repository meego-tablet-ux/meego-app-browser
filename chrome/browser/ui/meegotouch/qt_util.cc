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

#include <time.h>
#include <QPoint>
#include <QDeclarativeView>

#include "base/logging.h"
#include "chrome/browser/ui/meegotouch/qt_util.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"


namespace gtk_util {

void EnumerateTopLevelWindows(ui::EnumerateWindowsDelegate* delegate) {
}

void GetWidgetSizeFromCharacters(
    GtkWidget* widget, double width_chars, double height_lines,
    int* width, int* height) {
}

string16 GetStockPreferencesMenuLabel() {
  DNOTIMPLEMENTED();
  return string16();
}

unsigned int XTimeNow() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

QPoint MapScenePosToOrientationAngle(QPoint p, QtMobility::QOrientationReading::Orientation angle) {

  Browser* browser = BrowserList::GetLastActive();
  BrowserWindowQt* browser_window = (BrowserWindowQt*)browser->window();

  QRectF rect = browser_window->DeclarativeView()->scene()->sceneRect();
  QSize vs_size(int(rect.width()), int(rect.height()));

  switch (angle) {
    case QtMobility::QOrientationReading::TopUp:
      return QPoint(p.x(), p.y());
    case QtMobility::QOrientationReading::RightUp:
      return QPoint(p.y(), vs_size.width() - p.x());
    case QtMobility::QOrientationReading::TopDown:
      return QPoint(vs_size.width() - p.x(), vs_size.height() - p.y());
    case QtMobility::QOrientationReading::LeftUp:
      return QPoint(vs_size.height() - p.y(), p.x());
    default:
      LOG(ERROR) << "should not reach here";
      return QPoint(0, 0);
    }
}

} // namespace gtk_util

