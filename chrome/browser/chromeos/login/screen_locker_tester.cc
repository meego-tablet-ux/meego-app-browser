// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/screen_locker_tester.h"

#include <gdk/gdkkeysyms.h>

#include "app/l10n_util.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/chromeos/login/mock_authenticator.h"
#include "chrome/browser/chromeos/login/screen_locker.h"
#include "chrome/browser/chromeos/login/screen_lock_view.h"
#include "views/controls/button/button.h"
#include "views/controls/label.h"
#include "views/controls/textfield/textfield.h"

namespace chromeos {

test::ScreenLockerTester* ScreenLocker::GetTester() {
  return new test::ScreenLockerTester();
}

namespace test {

bool ScreenLockerTester::IsOpen() {
  return chromeos::ScreenLocker::screen_locker_ != NULL;
}

void ScreenLockerTester::InjectMockAuthenticator(const char* password) {
  DCHECK(ScreenLocker::screen_locker_);
  ScreenLocker::screen_locker_->SetAuthenticator(
      new MockAuthenticator(ScreenLocker::screen_locker_, "", password));
}

void ScreenLockerTester::EnterPassword(const char* password) {
  DCHECK(ScreenLocker::screen_locker_);
  views::Textfield* pass = GetPasswordField();
  pass->SetText(ASCIIToUTF16(password));
  GdkEvent* event = gdk_event_new(GDK_KEY_PRESS);

  event->key.keyval = GDK_Return;
  views::Textfield::Keystroke ret(&event->key);
  ScreenLocker::screen_locker_->screen_lock_view_->HandleKeystroke(pass, ret);

  gdk_event_free(event);
}

views::Textfield* ScreenLockerTester::GetPasswordField() {
  DCHECK(ScreenLocker::screen_locker_);
  return ScreenLocker::screen_locker_->screen_lock_view_->password_field_;
}

}  // namespace test

}  // namespace chromeos
