// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_LOGIN_MANAGER_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_LOGIN_MANAGER_VIEW_H_

#include <string>
#include "chrome/browser/chromeos/login/wizard_screen.h"
#include "chrome/browser/chromeos/version_loader.h"
#include "views/accelerator.h"
#include "views/controls/button/button.h"
#include "views/controls/textfield/textfield.h"
#include "views/widget/widget_gtk.h"
#include "views/window/window_delegate.h"

namespace chromeos {
class ScreenObserver;
}  // namespace chromeos

namespace views {
class Label;
class NativeButton;
}  // namespace views

class LoginManagerView : public WizardScreen,
                         public views::WindowDelegate,
                         public views::Textfield::Controller,
                         public views::ButtonListener {
 public:
  explicit LoginManagerView(chromeos::ScreenObserver* observer);
  virtual ~LoginManagerView();

  // WizardScreen implementation:
  void Init();

  // Overridden from views::View:
  virtual gfx::Size GetPreferredSize();
  virtual void Layout();

  // Overridden from views::WindowDelegate:
  virtual views::View* GetContentsView();

  // Overridden from views::Textfield::Controller
  // Not thread-safe, by virtue of using SetupSession().
  virtual bool HandleKeystroke(views::Textfield* sender,
                               const views::Textfield::Keystroke& keystroke);
  virtual void ContentsChanged(views::Textfield* sender,
                               const string16& new_contents) {}

  // Overriden from views::ButtonListener.
  virtual void ButtonPressed(views::Button* sender, const views::Event& event);

 private:
  // Given a |username| and |password|, this method attempts to authenticate to
  // the Google accounts servers.
  // Returns true upon success and false on failure.
  bool Authenticate(const std::string& username,
                    const std::string& password);

  // This is not threadsafe; as authentication is supposed to happen on the main
  // thread before any other threads are started, so this should be ok.
  // That said, the only reason we're not threadsafe right now is that we're
  // munging the CommandLine::ForCurrentProcess() to enable auto-client-side-ssl
  // for Googlers.  So, if we can do that differently to make this thread-safe,
  // that'd be A Good Thing (tm).
  void SetupSession(const std::string& username);

  // Callback from chromeos::VersionLoader giving the version.
  void OnOSVersion(chromeos::VersionLoader::Handle handle,
                   std::string version);

  // Attempt to login with the current field values.
  void Login();

  views::Textfield* username_field_;
  views::Textfield* password_field_;
  views::Label* os_version_label_;
  views::Label* title_label_;
  views::Label* username_label_;
  views::Label* password_label_;
  views::Label* error_label_;
  views::NativeButton* sign_in_button_;

  // Handles asynchronously loading the version.
  chromeos::VersionLoader loader_;

  // Used to request the version.
  CancelableRequestConsumer consumer_;

  // Notifications receiver.
  chromeos::ScreenObserver* observer_;

  DISALLOW_COPY_AND_ASSIGN(LoginManagerView);
};

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_LOGIN_MANAGER_VIEW_H_
