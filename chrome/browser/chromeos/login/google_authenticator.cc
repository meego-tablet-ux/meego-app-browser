// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/google_authenticator.h"

#include <string>
#include <vector>

#include "base/file_path.h"
#include "base/file_util.h"
#include "base/lock.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/sha2.h"
#include "base/string_util.h"
#include "base/third_party/nss/blapi.h"
#include "base/third_party/nss/sha256.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_thread.h"
#include "chrome/browser/chromeos/cros/cryptohome_library.h"
#include "chrome/browser/chromeos/login/auth_response_handler.h"
#include "chrome/browser/chromeos/login/authentication_notification_details.h"
#include "chrome/browser/chromeos/login/login_status_consumer.h"
#include "chrome/browser/chromeos/login/ownership_service.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/profile_manager.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/net/gaia/gaia_authenticator2.h"
#include "chrome/common/net/gaia/gaia_constants.h"
#include "chrome/common/notification_service.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request_status.h"
#include "third_party/libjingle/source/talk/base/urlencode.h"

using base::Time;
using base::TimeDelta;
using file_util::GetFileSize;
using file_util::PathExists;
using file_util::ReadFile;
using file_util::ReadFileToString;

namespace chromeos {

// static
const char GoogleAuthenticator::kLocalaccountFile[] = "localaccount";

// static
const int GoogleAuthenticator::kClientLoginTimeoutMs = 10000;
// static
const int GoogleAuthenticator::kLocalaccountRetryIntervalMs = 20;

const int kPassHashLen = 32;

GoogleAuthenticator::GoogleAuthenticator(LoginStatusConsumer* consumer)
    : Authenticator(consumer),
      unlock_(false),
      try_again_(true),
      checked_for_localaccount_(false) {
  CHECK(chromeos::CrosLibrary::Get()->EnsureLoaded());
  // If not already owned, this is a no-op.  If it is, this loads the owner's
  // public key off of disk.
  OwnershipService::GetSharedInstance()->StartLoadOwnerKeyAttempt();
}

GoogleAuthenticator::~GoogleAuthenticator() {}

void GoogleAuthenticator::CancelClientLogin() {
  if (gaia_authenticator_->HasPendingFetch()) {
    LOG(INFO) << "Canceling ClientLogin attempt.";
    gaia_authenticator_->CancelRequest();

    ChromeThread::PostTask(
        ChromeThread::FILE, FROM_HERE,
        NewRunnableMethod(this,
                          &GoogleAuthenticator::LoadLocalaccount,
                          std::string(kLocalaccountFile)));

    CheckOffline(LoginFailure(LoginFailure::LOGIN_TIMED_OUT));
  }
}

void GoogleAuthenticator::TryClientLogin() {
  gaia_authenticator_->StartClientLogin(
      username_,
      password_,
      GaiaConstants::kContactsService,
      login_token_,
      login_captcha_,
      GaiaAuthenticator2::HostedAccountsAllowed);

  ChromeThread::PostDelayedTask(
      ChromeThread::UI,
      FROM_HERE,
      NewRunnableMethod(this,
                        &GoogleAuthenticator::CancelClientLogin),
      kClientLoginTimeoutMs);
}

void GoogleAuthenticator::PrepareClientLoginAttempt(
    const std::string& password,
    const std::string& token,
    const std::string& captcha) {

  // Save so we can retry.
  password_.assign(password);
  login_token_.assign(token);
  login_captcha_.assign(captcha);
}

void GoogleAuthenticator::ClearClientLoginAttempt() {
  password_.clear();
  login_token_.clear();
  login_captcha_.clear();
}

bool GoogleAuthenticator::AuthenticateToLogin(
    Profile* profile,
    const std::string& username,
    const std::string& password,
    const std::string& login_token,
    const std::string& login_captcha) {
  unlock_ = false;

  // TODO(cmasone): Figure out how to parallelize fetch, username/password
  // processing without impacting testability.
  username_.assign(Canonicalize(username));
  ascii_hash_.assign(HashPassword(password));

  gaia_authenticator_.reset(
      new GaiaAuthenticator2(this,
                             GaiaConstants::kChromeOSSource,
                             profile->GetRequestContext()));
  // Will be used for retries.
  PrepareClientLoginAttempt(password, login_token, login_captcha);
  TryClientLogin();
  return true;
}

bool GoogleAuthenticator::AuthenticateToUnlock(const std::string& username,
                                               const std::string& password) {
  username_.assign(Canonicalize(username));
  ascii_hash_.assign(HashPassword(password));
  unlock_ = true;
  LoadLocalaccount(kLocalaccountFile);
  if (!localaccount_.empty() && localaccount_ == username) {
    LOG(INFO) << "unlocking local account";
    ChromeThread::PostTask(
        ChromeThread::UI, FROM_HERE,
        NewRunnableMethod(this,
                          &GoogleAuthenticator::OnLoginSuccess,
                          GaiaAuthConsumer::ClientLoginResult(), false));
  } else {
    ChromeThread::PostTask(
        ChromeThread::UI, FROM_HERE,
        NewRunnableMethod(this, &GoogleAuthenticator::CheckOffline,
                          LoginFailure(LoginFailure::UNLOCK_FAILED)));
  }
  return true;
}

void GoogleAuthenticator::LoginOffTheRecord() {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::UI));
  int mount_error = chromeos::kCryptohomeMountErrorNone;
  if (CrosLibrary::Get()->GetCryptohomeLibrary()->MountForBwsi(&mount_error)) {
    AuthenticationNotificationDetails details(true);
    NotificationService::current()->Notify(
        NotificationType::LOGIN_AUTHENTICATION,
        NotificationService::AllSources(),
        Details<AuthenticationNotificationDetails>(&details));
    consumer_->OnOffTheRecordLoginSuccess();
  } else {
    LOG(ERROR) << "Could not mount tmpfs: " << mount_error;
    consumer_->OnLoginFailure(
        LoginFailure(LoginFailure::COULD_NOT_MOUNT_TMPFS));
  }
}

void GoogleAuthenticator::OnClientLoginSuccess(
    const GaiaAuthConsumer::ClientLoginResult& credentials) {

  LOG(INFO) << "Online login successful!";
  ClearClientLoginAttempt();

  ChromeThread::PostTask(
      ChromeThread::UI, FROM_HERE,
      NewRunnableMethod(this,
                        &GoogleAuthenticator::OnLoginSuccess,
                        credentials, false));
}

void GoogleAuthenticator::OnClientLoginFailure(
   const GoogleServiceAuthError& error) {

  if (error.state() == GoogleServiceAuthError::REQUEST_CANCELED) {
    if (try_again_) {
      try_again_ = false;
      LOG(ERROR) << "Login attempt canceled!?!?  Trying again.";
      TryClientLogin();
      return;
    }
    LOG(ERROR) << "Login attempt canceled again?  Already retried...";
  }

  ClearClientLoginAttempt();

  if (error.state() == GoogleServiceAuthError::TWO_FACTOR) {
    LOG(WARNING) << "Two factor authenticated. Sync will not work.";
    OnClientLoginSuccess(GaiaAuthConsumer::ClientLoginResult());
    return;
  }

  ChromeThread::PostTask(
      ChromeThread::FILE, FROM_HERE,
      NewRunnableMethod(this,
                        &GoogleAuthenticator::LoadLocalaccount,
                        std::string(kLocalaccountFile)));

  LoginFailure failure_details = LoginFailure::FromNetworkAuthFailure(error);

  if (error.state() == GoogleServiceAuthError::CONNECTION_FAILED) {
    // The fetch failed for network reasons, try offline login.
    ChromeThread::PostTask(
        ChromeThread::UI, FROM_HERE,
        NewRunnableMethod(this, &GoogleAuthenticator::CheckOffline,
                          failure_details));
    return;
  }

  // The fetch succeeded, but ClientLogin said no, or we exhausted retries.
  ChromeThread::PostTask(
      ChromeThread::UI, FROM_HERE,
      NewRunnableMethod(this,
        &GoogleAuthenticator::CheckLocalaccount,
        failure_details));
}

void GoogleAuthenticator::OnLoginSuccess(
    const GaiaAuthConsumer::ClientLoginResult& credentials,
    bool request_pending) {
  // Send notification of success
  AuthenticationNotificationDetails details(true);
  NotificationService::current()->Notify(
      NotificationType::LOGIN_AUTHENTICATION,
      NotificationService::AllSources(),
      Details<AuthenticationNotificationDetails>(&details));

  int mount_error = chromeos::kCryptohomeMountErrorNone;
  if (unlock_ ||
      (CrosLibrary::Get()->GetCryptohomeLibrary()->Mount(username_.c_str(),
                                                         ascii_hash_.c_str(),
                                                         &mount_error))) {
    consumer_->OnLoginSuccess(username_, credentials, request_pending);
  } else if (!unlock_ &&
             mount_error == chromeos::kCryptohomeMountErrorKeyFailure) {
    consumer_->OnPasswordChangeDetected(credentials);
  } else {
    OnLoginFailure(LoginFailure(LoginFailure::COULD_NOT_MOUNT_CRYPTOHOME));
  }
}

void GoogleAuthenticator::CheckOffline(const LoginFailure& error) {
  LOG(INFO) << "Attempting offline login";
  if (CrosLibrary::Get()->GetCryptohomeLibrary()->CheckKey(
          username_.c_str(),
          ascii_hash_.c_str())) {
    // The fetch didn't succeed, but offline login did.
    LOG(INFO) << "Offline login successful!";
    OnLoginSuccess(GaiaAuthConsumer::ClientLoginResult(), false);
  } else {
    // We couldn't hit the network, and offline login failed.
    GoogleAuthenticator::CheckLocalaccount(error);
  }
}

void GoogleAuthenticator::CheckLocalaccount(const LoginFailure& error) {
  {
    AutoLock for_this_block(localaccount_lock_);
    LOG(INFO) << "Checking localaccount";
    if (!checked_for_localaccount_) {
      ChromeThread::PostDelayedTask(
          ChromeThread::UI,
          FROM_HERE,
          NewRunnableMethod(this,
                            &GoogleAuthenticator::CheckLocalaccount,
                            error),
          kLocalaccountRetryIntervalMs);
      return;
    }
  }
  int mount_error = chromeos::kCryptohomeMountErrorNone;
  if (!localaccount_.empty() && localaccount_ == username_) {
    if (CrosLibrary::Get()->GetCryptohomeLibrary()->MountForBwsi(
        &mount_error)) {
      LOG(WARNING) << "Logging in with localaccount: " << localaccount_;
      consumer_->OnLoginSuccess(username_,
                                GaiaAuthConsumer::ClientLoginResult(),
                                false);
    } else {
      LOG(ERROR) << "Could not mount tmpfs for local account: " << mount_error;
      OnLoginFailure(
          LoginFailure(LoginFailure::COULD_NOT_MOUNT_TMPFS));
    }
  } else {
    OnLoginFailure(error);
  }
}

void GoogleAuthenticator::OnLoginFailure(const LoginFailure& error) {
  // Send notification of failure
  AuthenticationNotificationDetails details(false);
  NotificationService::current()->Notify(
      NotificationType::LOGIN_AUTHENTICATION,
      NotificationService::AllSources(),
      Details<AuthenticationNotificationDetails>(&details));
  LOG(WARNING) << "Login failed: " << error.GetErrorString();
  consumer_->OnLoginFailure(error);
}

void GoogleAuthenticator::RecoverEncryptedData(const std::string& old_password,
    const GaiaAuthConsumer::ClientLoginResult& credentials) {

  std::string old_hash = HashPassword(old_password);
  if (CrosLibrary::Get()->GetCryptohomeLibrary()->MigrateKey(username_,
                                                             old_hash,
                                                             ascii_hash_)) {
    OnLoginSuccess(credentials, false);
    return;
  }
  // User seems to have given us the wrong old password...
  consumer_->OnPasswordChangeDetected(credentials);
}

void GoogleAuthenticator::ResyncEncryptedData(
    const GaiaAuthConsumer::ClientLoginResult& credentials) {

  if (CrosLibrary::Get()->GetCryptohomeLibrary()->Remove(username_)) {
    OnLoginSuccess(credentials, false);
  } else {
    OnLoginFailure(LoginFailure(LoginFailure::DATA_REMOVAL_FAILED));
  }
}

void GoogleAuthenticator::LoadSystemSalt() {
  if (!system_salt_.empty())
    return;
  system_salt_ = CrosLibrary::Get()->GetCryptohomeLibrary()->GetSystemSalt();
  CHECK(!system_salt_.empty());
  CHECK_EQ(system_salt_.size() % 2, 0U);
}

void GoogleAuthenticator::LoadLocalaccount(const std::string& filename) {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::FILE));
  {
    AutoLock for_this_block(localaccount_lock_);
    if (checked_for_localaccount_)
      return;
  }
  FilePath localaccount_file;
  std::string localaccount;
  if (PathService::Get(base::DIR_EXE, &localaccount_file)) {
    localaccount_file = localaccount_file.Append(filename);
    LOG(INFO) << "looking for localaccount in " << localaccount_file.value();

    ReadFileToString(localaccount_file, &localaccount);
    TrimWhitespaceASCII(localaccount, TRIM_TRAILING, &localaccount);
    LOG(INFO) << "Loading localaccount: " << localaccount;
  } else {
    LOG(INFO) << "Assuming no localaccount";
  }
  SetLocalaccount(localaccount);
}

void GoogleAuthenticator::SetLocalaccount(const std::string& new_name) {
  localaccount_ = new_name;
  {  // extra braces for clarity about AutoLock scope.
    AutoLock for_this_block(localaccount_lock_);
    checked_for_localaccount_ = true;
  }
}


std::string GoogleAuthenticator::HashPassword(const std::string& password) {
  // Get salt, ascii encode, update sha with that, then update with ascii
  // of password, then end.
  std::string ascii_salt = SaltAsAscii();
  unsigned char passhash_buf[kPassHashLen];
  char ascii_buf[kPassHashLen + 1];

  // Hash salt and password
  SHA256Context ctx;
  SHA256_Begin(&ctx);
  SHA256_Update(&ctx,
                reinterpret_cast<const unsigned char*>(ascii_salt.data()),
                static_cast<unsigned int>(ascii_salt.length()));
  SHA256_Update(&ctx,
                reinterpret_cast<const unsigned char*>(password.data()),
                static_cast<unsigned int>(password.length()));
  SHA256_End(&ctx,
             passhash_buf,
             NULL,
             static_cast<unsigned int>(sizeof(passhash_buf)));

  std::vector<unsigned char> passhash(passhash_buf,
                                      passhash_buf + sizeof(passhash_buf));
  BinaryToHex(passhash,
              passhash.size() / 2,  // only want top half, at least for now.
              ascii_buf,
              sizeof(ascii_buf));
  return std::string(ascii_buf, sizeof(ascii_buf) - 1);
}

std::string GoogleAuthenticator::SaltAsAscii() {
  LoadSystemSalt();  // no-op if it's already loaded.
  unsigned int salt_len = system_salt_.size();
  char ascii_salt[2 * salt_len + 1];
  if (GoogleAuthenticator::BinaryToHex(system_salt_,
                                       salt_len,
                                       ascii_salt,
                                       sizeof(ascii_salt))) {
    return std::string(ascii_salt, sizeof(ascii_salt) - 1);
  } else {
    return std::string();
  }
}

// static
bool GoogleAuthenticator::BinaryToHex(const std::vector<unsigned char>& binary,
                                      const unsigned int binary_len,
                                      char* hex_string,
                                      const unsigned int len) {
  if (len < 2*binary_len)
    return false;
  memset(hex_string, 0, len);
  for (uint i = 0, j = 0; i < binary_len; i++, j+=2)
    snprintf(hex_string + j, len - j, "%02x", binary[i]);
  return true;
}

}  // namespace chromeos
