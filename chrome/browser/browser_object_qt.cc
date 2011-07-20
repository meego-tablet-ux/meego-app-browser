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
#include "browser_object_qt.h"
#include <iostream>
#include "base/file_util.h"
#include "base/path_service.h"
#include "base/at_exit.h"
#include "content/common/content_switches.h"
#include "chrome/browser/browser_main.h"

BrowserObject * g_browser_object = NULL;

BrowserObject::BrowserObject(QObject *obj)
  : QObject(obj)
{
  g_browser_object = this;
}

extern void RunChromeMain(int argc, const char** argv);

QApplication * BrowserObject::getApplication()
{
  return static_cast<QApplication *>(QApplication::instance());
}

QDeclarativeView * BrowserObject::getDeclarativeView()
{
  return view_;
}

void BrowserObject::handleCall(const QStringList &parameters)
{
  emit call(parameters);
}

void BrowserObject::runMain(const QStringList &parameters, QObject *obj)
{
  view_ = static_cast<QDeclarativeView *>(obj);
  QStringList args = parameters;
  adjustPaths(args);
  convertArgs(args);
  getApplication()->processEvents(QEventLoop::AllEvents);

  RunChromeMain(argv_.size(), (const char **)argv_.data());
}

void BrowserObject::adjustPaths(QStringList &args)
{
  // check where is meego-browser-wrapper and set subprocess path
  const QString name = "/meego-browser-wrapper";

  QString childPath;

  QString subProcessPath("--");
  subProcessPath += switches::kBrowserSubprocessPath;
  subProcessPath += "=";

  bool found = false;
  for (int i = 0; i < args.size(); i++) {
    if(args.at(i).startsWith(subProcessPath, Qt::CaseInsensitive)){
      childPath = args.at(i).right(args.at(i).size() - subProcessPath.size());
      found = true;
      break;
    }
  }

  // we have to set sub process path for browser internally uses this option
  // otherwise, it forks render process by '/proc/self/exe', which is unexpected
  if (!found) {
    FilePath filepath;
    file_util::GetCurrentDirectory(&filepath);
    childPath = filepath.value().c_str() + name;
    if (!QFile::exists(childPath)) {
      childPath = "/usr/lib/meego-app-browser/" + name;
    }
    args.push_back(subProcessPath + childPath);
  }

  DLOG(INFO) << "sub-process-path = " << childPath.toUtf8().data();
}

void BrowserObject::convertArgs(const QStringList &args)
{
  argv_.clear();
  arg_list_.clear();

  for (int i = 0; i < args.size(); i ++) {
    QString str = args.at(i);
    if (!str.isEmpty()) {
      arg_list_.push_back(str.toUtf8());
      argv_.push_back(arg_list_.back().data());
    }
  }
}

#include "moc_browser_object_qt.cc"

