// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_NOTIFIER_COMMUNICATOR_LOGIN_FAILURE_H_
#define CHROME_BROWSER_SYNC_NOTIFIER_COMMUNICATOR_LOGIN_FAILURE_H_

#include "talk/base/common.h"
#include "talk/xmpp/xmppengine.h"

namespace notifier {

class LoginFailure {
 public:
  enum LoginError {
    // Check the xmpp_error for more information.
    XMPP_ERROR,

    // If the certificate has expired, it usually means that the computer's
    // clock isn't set correctly.
    CERTIFICATE_EXPIRED_ERROR,

    // Apparently, there is a proxy that needs authentication information.
    PROXY_AUTHENTICATION_ERROR,
  };

  explicit LoginFailure(LoginError error);
  LoginFailure(LoginError error,
               buzz::XmppEngine::Error xmpp_error,
               int subcode);

  // Used as the first level of error information.
  LoginError error() const {
    return error_;
  }

  // Returns the XmppEngine only. Valid if and only if error() == XMPP_ERROR.
  //
  // Handler should mimic logic from PhoneWindow::ShowConnectionError
  // (except that the DiagnoseConnectionError has already been done).
  buzz::XmppEngine::Error xmpp_error() const;

 private:
  LoginError error_;
  buzz::XmppEngine::Error xmpp_error_;
  int subcode_;

  DISALLOW_COPY_AND_ASSIGN(LoginFailure);
};

}  // namespace notifier

#endif  // CHROME_BROWSER_SYNC_NOTIFIER_COMMUNICATOR_LOGIN_FAILURE_H_
