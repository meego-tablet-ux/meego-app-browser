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

#ifndef CHROME_BROWSER_QT_SSL_MODAL_DIALOG_QT_H_
#define CHROME_BROWSER_QT_SSL_MODAL_DIALOG_QT_H_
#pragma once

#include <string>
#include "chrome/browser/ui/app_modal_dialogs/app_modal_dialog.h"
#include "chrome/browser/ssl/ssl_blocking_page.h"

class TabContents;
class SSLBlockingPage;

class SSLAppModalDialog:public AppModalDialog
{
  public:
    SSLAppModalDialog(TabContents* tab_contents);
    virtual ~SSLAppModalDialog();
    void CreateAndShowDialog();
    NativeAppModalDialog* CreateNativeDialog(){return NULL;}
    void SetPageHandler(SSLCertErrorHandler* handler, SSLBlockingPage::Delegate* delegate, SSLBlockingPage::ErrorLevel error_level);
    SSLBlockingPage* GetPageHandler();
    void SetDetails(DictionaryValue* strings);
    DictionaryValue* GetDetails();
    void ProcessCommand(const std::string& command);
  private:
    SSLBlockingPage* ssl_page_;
    DictionaryValue* strings_;
};

#endif
