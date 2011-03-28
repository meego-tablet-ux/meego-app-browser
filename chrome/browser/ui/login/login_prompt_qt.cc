// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/login/login_prompt.h"

#include "ui/base/l10n/l10n_util.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/browser_list.h"
#include "content/browser/browser_thread.h"
#include "chrome/browser/ui/login/login_model.h"
#include "chrome/browser/ui/meegotouch/dialog_qt.h"
#include "chrome/browser/password_manager/password_manager.h"
#include "content/browser/renderer_host/resource_dispatcher_host.h"
#include "content/browser/tab_contents/navigation_controller.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "chrome/browser/tab_contents/tab_util.h"
#include "content/common/notification_service.h"
#include "grit/generated_resources.h"
#include "net/url_request/url_request.h"

using webkit_glue::PasswordForm;

class LoginHandlerQt : public LoginHandler,
  public DialogQtResultListener {
  public:
    LoginHandlerQt(net::AuthChallengeInfo* auth_info, net::URLRequest* request)
  : LoginHandler(auth_info, request) {

    };

    virtual ~LoginHandlerQt() {
    delete qDlgModel_;

    };

    // LoginHandler:
    virtual void BuildViewForPasswordManager(PasswordManager* manager,
                                             const string16& explanation) {
      // Create the dialog here.
      qDlgModel_ = new DialogQtModel(DialogQt::DLG_AUTH, false, 
                                     UTF16ToUTF8(explanation).c_str(), NULL,
                                     NULL, NULL);

      Browser* browser = BrowserList::GetLastActive();
      BrowserWindowQt* browser_window = (BrowserWindowQt*)browser->window();
      browser_window->ShowDialog(qDlgModel_, this);
    }
    
    virtual void OnDialogResponse(int result, QString input1, QString input2, bool isSuppress) {
      if(result == DialogQt::Rejected) {
        CancelAuth();
      }else if(result == DialogQt::Accepted) {
         DLOG(INFO)<<"input1: "<<input1.toStdString();
         DLOG(INFO)<<"input2: "<<input2.toStdString();
        SetAuth(WideToUTF16(input1.toStdWString()), WideToUTF16(input2.toStdWString()));
      }
    }

    virtual void OnAutofillDataAvailable(const std::wstring&, const std::wstring&) {

    }

  private: 
    DialogQtModel* qDlgModel_;

};


// static
LoginHandler* LoginHandler::Create(net::AuthChallengeInfo* auth_info,
                                   net::URLRequest* request) {
  return new LoginHandlerQt(auth_info, request);
}
