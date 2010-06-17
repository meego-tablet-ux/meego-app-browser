// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_NEW_USER_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_NEW_USER_VIEW_H_

#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/ref_counted.h"
#include "base/scoped_ptr.h"
#include "chrome/browser/chromeos/login/language_switch_menu.h"
#include "chrome/browser/chromeos/login/login_html_dialog.h"
#include "views/accelerator.h"
#include "views/controls/button/button.h"
#include "views/controls/button/menu_button.h"
#include "views/controls/link.h"
#include "views/controls/textfield/textfield.h"
#include "views/view.h"

namespace views {
class Label;
class NativeButton;
class Throbber;
}  // namespace views

namespace chromeos {

// View that is used for new user login. It asks for username and password,
// allows to specify language preferences or initiate new account creation.
class NewUserView : public views::View,
                    public views::Textfield::Controller,
                    public views::LinkController,
                    public views::ButtonListener,
                    public LoginHtmlDialog::Delegate {
 public:
  // Delegate class to get notifications from the view.
  class Delegate {
  public:
    virtual ~Delegate() {}

    // User provided |username|, |password| and initiated login.
    virtual void OnLogin(const std::string& username,
                         const std::string& password) = 0;

    // Initiates off the record (incognito) login.
    virtual void OnLoginOffTheRecord() = 0;

    // User initiated new account creation.
    virtual void OnCreateAccount() = 0;

    // User started typing so clear all error messages.
    virtual void ClearErrors() = 0;
  };

  // If |need_border| is true, RoundedRect border and background are required.
  NewUserView(Delegate* delegate, bool need_border);
  virtual ~NewUserView();

  // Initialize view layout.
  void Init();

  // Update strings from the resources. Executed on language change.
  void UpdateLocalizedStrings();

  // Resets password text and sets the enabled state of the password.
  void ClearAndEnablePassword();

  // Stops throbber shown during login.
  void StopThrobber();

  // Returns bounds of password field in screen coordinates.
  gfx::Rect GetPasswordBounds() const;

  // Overridden from views::View:
  virtual gfx::Size GetPreferredSize();
  virtual void Layout();
  virtual void RequestFocus();

  // Overridden from views::WindowDelegate:
  virtual views::View* GetContentsView();

  // Setters for textfields.
  void SetUsername(const std::string& username);
  void SetPassword(const std::string& password);

  // Attempt to login with the current field values.
  void Login();

  // Overridden from views::Textfield::Controller
  // Not thread-safe, by virtue of using SetupSession().
  virtual bool HandleKeystroke(views::Textfield* sender,
                               const views::Textfield::Keystroke& keystroke);
  virtual void ContentsChanged(views::Textfield* sender,
                               const string16& new_contents) {}

  // Overridden from views::ButtonListener.
  virtual void ButtonPressed(views::Button* sender, const views::Event& event);

  // Overridden from views::LinkController.
  virtual void LinkActivated(views::Link* source, int event_flags);

  virtual bool AcceleratorPressed(const views::Accelerator& accelerator);

  // LoginHtmlDialog::Delegate implementation.
  virtual void OnDialogClosed() {}

 protected:
  // views::View overrides:
  virtual void ViewHierarchyChanged(bool is_add, views::View *parent,
                                    views::View *child);
  virtual void NativeViewHierarchyChanged(bool attached,
                                          gfx::NativeView native_view,
                                          views::RootView* root_view);
  virtual void LocaleChanged();

 private:
  // Returns corresponding native window.
  gfx::NativeWindow GetNativeWindow() const;

  // Enables/disables input controls (textfields, buttons).
  void EnableInputControls(bool enabled);
  void FocusFirstField();

  // Creates Link control and adds it as a child.
  void InitLink(views::Link** link);

  // Delete and recreate native controls that fail to update preferred size
  // after string update.
  void RecreateNativeControls();

  views::Textfield* username_field_;
  views::Textfield* password_field_;
  views::Label* title_label_;
  views::NativeButton* sign_in_button_;
  views::Link* create_account_link_;
  views::Link* cant_access_account_link_;
  views::Link* browse_without_signin_link_;
  views::MenuButton* languages_menubutton_;
  views::Throbber* throbber_;

  views::Accelerator accel_focus_user_;
  views::Accelerator accel_focus_pass_;

  // Notifications receiver.
  Delegate* delegate_;

  ScopedRunnableMethodFactory<NewUserView> focus_grabber_factory_;

  LanguageSwitchMenu language_switch_menu_;

  // Dialog used to display help like "Can't access your account".
  scoped_ptr<LoginHtmlDialog> dialog_;

  // Indicates that this view was created when focus manager was unavailable
  // (on the hidden tab, for example).
  bool focus_delayed_;

  // True when login is in process.
  bool login_in_process_;

  // If true, this view needs RoundedRect border and background.
  bool need_border_;

  FRIEND_TEST_ALL_PREFIXES(LoginScreenTest, IncognitoLogin);
  friend class LoginScreenTest;

  DISALLOW_COPY_AND_ASSIGN(NewUserView);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_NEW_USER_VIEW_H_
