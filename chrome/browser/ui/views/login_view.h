// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_LOGIN_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_LOGIN_VIEW_H_
#pragma once

#include "base/task.h"
#include "chrome/browser/ui/login/login_model.h"
#include "views/view.h"

namespace views {
class Label;
class Textfield;
class LoginModel;
}  // namespace views

// This class is responsible for displaying the contents of a login window
// for HTTP/FTP authentication.
class LoginView : public views::View, public LoginModelObserver {
 public:
  // |model| is observed for the entire lifetime of the LoginView.
  // Therefore |model| should not be destroyed before the LoginView
  // object.
  LoginView(const std::wstring& explanation, LoginModel* model);
  virtual ~LoginView();

  // Access the data in the username/password text fields.
  std::wstring GetUsername();
  std::wstring GetPassword();

  // LoginModelObserver implementation.
  virtual void OnAutofillDataAvailable(const std::wstring& username,
                                       const std::wstring& password) OVERRIDE;

  // Used by LoginHandlerWin to set the initial focus.
  views::View* GetInitiallyFocusedView();

 private:
  // Non-owning refs to the input text fields.
  views::Textfield* username_field_;
  views::Textfield* password_field_;

  // Button labels
  views::Label* username_label_;
  views::Label* password_label_;

  // Authentication message.
  views::Label* message_label_;

  // If not null, points to a model we need to notify of our own destruction
  // so it doesn't try and access this when its too late.
  LoginModel* login_model_;

  DISALLOW_COPY_AND_ASSIGN(LoginView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_LOGIN_VIEW_H_
