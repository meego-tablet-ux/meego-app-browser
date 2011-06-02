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

#include "selection_handler_qt.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"

#include <QDeclarativeView>
#include <QDeclarativeContext>

SelectionHandlerQtImpl::SelectionHandlerQtImpl(SelectionHandlerQt* handler)
{
    handler_ = handler;
}


void SelectionHandlerQtImpl::Set(int start_x, int start_y, int end_x, int end_y)
{
    emit set(start_x, start_y, end_x, end_y);
}

void SelectionHandlerQtImpl::Show()
{
    emit show();
}

void SelectionHandlerQtImpl::Hide()
{
    emit hide();
}

void SelectionHandlerQtImpl::SetScale(double scale_)
{
    emit setScale(scale_);
}

void SelectionHandlerQtImpl::SetPinchOffset(int offset_x_, int offset_y_)
{
    emit setPinchOffset(offset_x_, offset_y_);
}

void SelectionHandlerQtImpl::SetPinchState(bool in_pinch_)
{
    emit setPinchState(in_pinch_);
}

void SelectionHandlerQtImpl::SetHandlerHeight(int height_)
{
    emit setHandlerHeight(height_);
}

SelectionHandlerQt::SelectionHandlerQt(BrowserWindowQt* window)
{
    window_ = window;
    impl_ = new SelectionHandlerQtImpl(this);

    QDeclarativeView* view = window->DeclarativeView();
    QDeclarativeContext* context = view->rootContext();
    context->setContextProperty("selectionHandler", impl_);
}

SelectionHandlerQt::~SelectionHandlerQt()
{
    delete impl_;
}

void SelectionHandlerQt::showHandler()
{
    impl_->Show();
}

void SelectionHandlerQt::setHandlerAt(int start_x, int start_y, int end_x, int end_y)
{
    impl_->Set(start_x, start_y, end_x, end_y);
}

void SelectionHandlerQt::hideHandler()
{
    impl_->Hide();
}

void SelectionHandlerQt::setScale(double scale_)
{
    impl_->SetScale(scale_);
}

void SelectionHandlerQt::setPinchOffset(int offset_x_, int offset_y_)
{
    impl_->SetPinchOffset(offset_x_, offset_y_);
}

void SelectionHandlerQt::setPinchState(bool in_pinch_)
{
    impl_->SetPinchState(in_pinch_);
}

void SelectionHandlerQt::setHandlerHeight(int height_)
{
    impl_->SetHandlerHeight(height_);
}



