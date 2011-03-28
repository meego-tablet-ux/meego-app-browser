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


#include "chrome/browser/ui/meegotouch/fullscreen_exit_bubble_qt.h"

#include "ui/base/l10n/l10n_util.h"
#include "grit/generated_resources.h"

#include <QDeclarativeView>
#include <QDeclarativeContext>

FullscreenExitBubbleQt::FullscreenExitBubbleQt(BrowserWindowQt* window, bool fullscreen)
    : window_(window),
      fullscreen_(fullscreen) {
  impl_ = new FullscreenExitBubbleQtImpl(this);
  InitWidgets();
}

FullscreenExitBubbleQt::~FullscreenExitBubbleQt() {
}

void FullscreenExitBubbleQt::SetFullscreen(bool fullscreen) {
  if (fullscreen_ == fullscreen)
    return;

  QDeclarativeView* view = window_->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();
  context->setContextProperty("is_fullscreen", fullscreen);

  fullscreen_ = fullscreen;
  if (fullscreen_)
    impl_->EnterFullscreen();
  else
    impl_->ExitFullscreen();

}

bool FullscreenExitBubbleQt::IsFullscreen() {
  return fullscreen_;
}

void FullscreenExitBubbleQt::InitWidgets() {
  QDeclarativeView* view = window_->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();

  context->setContextProperty("fullscreenBubbleObject", impl_);

  QString label = QString::fromStdString(l10n_util::GetStringUTF8(IDS_EXIT_FULLSCREEN_MODE));
  label.replace(QString("($1)"), QString(""));

  context->setContextProperty("fullscreenBubbleLabel", label);
  context->setContextProperty("fullscreenBubbleYes", QString::fromStdString(l10n_util::GetStringUTF8(IDS_CONFIRM_MESSAGEBOX_YES_BUTTON_LABEL)));
  context->setContextProperty("fullscreenBubbleNo", QString::fromStdString(l10n_util::GetStringUTF8(IDS_CONFIRM_MESSAGEBOX_NO_BUTTON_LABEL)));
}

FullscreenExitBubbleQtImpl::FullscreenExitBubbleQtImpl(FullscreenExitBubbleQt* bubble)
    : bubble_(bubble) {
}

void FullscreenExitBubbleQtImpl::EnterFullscreen() {
  Q_EMIT enterFullscreen();
}

void FullscreenExitBubbleQtImpl::ExitFullscreen() {
  Q_EMIT exitFullscreen();
}

void FullscreenExitBubbleQtImpl::OnYesButton() {
  bubble_->SetFullscreen(false);
  ExitFullscreen();
}

#include "moc_fullscreen_exit_bubble_qt.cc"
