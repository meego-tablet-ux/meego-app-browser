// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_REMOTING_SETUP_FLOW_LOGIN_STEP_H_
#define CHROME_BROWSER_REMOTING_SETUP_FLOW_LOGIN_STEP_H_

#include "chrome/browser/remoting/setup_flow.h"
#include "chrome/common/net/gaia/gaia_auth_consumer.h"
#include "chrome/common/net/gaia/gaia_auth_fetcher.h"

namespace remoting {

// Implementation of login step for remoting setup flow.
class SetupFlowLoginStep : public SetupFlowStepBase, public GaiaAuthConsumer {
 public:
  SetupFlowLoginStep();
  virtual ~SetupFlowLoginStep();

  // SetupFlowStep implementation.
  virtual void HandleMessage(const std::string& message,
                             const ListValue* args);
  virtual void Cancel();

  // GaiaAuthConsumer implementation.
  virtual void OnClientLoginSuccess(
      const GaiaAuthConsumer::ClientLoginResult& credentials);
  virtual void OnClientLoginFailure(
      const GoogleServiceAuthError& error);
  virtual void OnIssueAuthTokenSuccess(const std::string& service,
                                       const std::string& auth_token);
  virtual void OnIssueAuthTokenFailure(const std::string& service,
                                       const GoogleServiceAuthError& error);

 protected:
  virtual void DoStart();

 private:
  void OnUserSubmittedAuth(const std::string& user,
                           const std::string& password,
                           const std::string& captcha);

  void ShowGaiaLogin(const DictionaryValue& args);
  void ShowGaiaSuccessAndSettingUp();
  void ShowGaiaFailed(const GoogleServiceAuthError& error);

  // Fetcher to obtain the Chromoting Directory token.
  scoped_ptr<GaiaAuthFetcher> authenticator_;
  std::string login_;
  std::string remoting_token_;
  std::string talk_token_;

  DISALLOW_COPY_AND_ASSIGN(SetupFlowLoginStep);
};

}  // namespace remoting

#endif  // CHROME_BROWSER_REMOTING_SETUP_FLOW_LOGIN_STEP_H_
