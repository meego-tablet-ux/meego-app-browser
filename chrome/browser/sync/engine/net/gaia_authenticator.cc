// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/engine/net/gaia_authenticator.h"

#include <string>
#include <utility>
#include <vector>

#include "base/basictypes.h"
#include "base/port.h"
#include "base/string_split.h"
#include "chrome/browser/sync/engine/net/http_return.h"
#include "chrome/common/deprecated/event_sys-inl.h"
#include "googleurl/src/gurl.h"
#include "net/base/escape.h"

using std::pair;
using std::string;
using std::vector;

namespace browser_sync {

static const char kGaiaV1IssueAuthTokenPath[] = "/accounts/IssueAuthToken";

static const char kGetUserInfoPath[] = "/accounts/GetUserInfo";

// Sole constructor with initializers for all fields.
GaiaAuthenticator::GaiaAuthenticator(const string& user_agent,
                                     const string& service_id,
                                     const string& gaia_url)
    : user_agent_(user_agent),
      service_id_(service_id),
      gaia_url_(gaia_url),
      request_count_(0),
      delay_(0),
      next_allowed_auth_attempt_time_(0),
      early_auth_attempt_count_(0),
      message_loop_(NULL) {
  GaiaAuthEvent done = { GaiaAuthEvent::GAIA_AUTHENTICATOR_DESTROYED, None,
                         this };
  channel_ = new Channel(done);
}

GaiaAuthenticator::~GaiaAuthenticator() {
  delete channel_;
}

// mutex_ must be entered before calling this function.
GaiaAuthenticator::AuthParams GaiaAuthenticator::MakeParams(
    const string& user_name,
    const string& password,
    SaveCredentials should_save_credentials,
    const string& captcha_token,
    const string& captcha_value,
    SignIn try_first) {
  AuthParams params;
  params.request_id = ++request_count_;
  params.email = user_name;
  params.password = password;
  params.should_save_credentials = should_save_credentials;
  params.captcha_token = captcha_token;
  params.captcha_value = captcha_value;
  params.authenticator = this;
  params.try_first = try_first;
  return params;
}

bool GaiaAuthenticator::Authenticate(const string& user_name,
                                     const string& password,
                                     SaveCredentials should_save_credentials,
                                     const string& captcha_token,
                                     const string& captcha_value,
                                     SignIn try_first) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);

  AuthParams const params =
    MakeParams(user_name, password, should_save_credentials, captcha_token,
               captcha_value, try_first);
  return AuthenticateImpl(params);
}

bool GaiaAuthenticator::AuthenticateWithLsid(const string& lsid,
                                             bool long_lived) {
  auth_results_.lsid = lsid;
  // We need to lookup the email associated with this LSID cookie in order to
  // update |auth_results_| with the correct values.
  if (LookupEmail(&auth_results_)) {
    auth_results_.email = auth_results_.primary_email;
    return IssueAuthToken(&auth_results_, service_id_, long_lived);
  }
  return false;
}


bool GaiaAuthenticator::AuthenticateImpl(const AuthParams& params) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  AuthResults results;
  const bool succeeded = AuthenticateImpl(params, &results);
  if (params.request_id == request_count_) {
    auth_results_ = results;
    GaiaAuthEvent event = { succeeded ? GaiaAuthEvent::GAIA_AUTH_SUCCEEDED
                                      : GaiaAuthEvent::GAIA_AUTH_FAILED,
                                      results.auth_error, this };
    channel_->NotifyListeners(event);
  }
  return succeeded;
}

// This method makes an HTTP request to the Gaia server, and calls other
// methods to help parse the response. If authentication succeeded, then
// Gaia-issued cookies are available in the respective variables; if
// authentication failed, then the exact error is available as an enum. If the
// client wishes to save the credentials, the last parameter must be true.
// If a subsequent request is made with fresh credentials, the saved credentials
// are wiped out; any subsequent request to the zero-parameter overload of this
// method preserves the saved credentials.
bool GaiaAuthenticator::AuthenticateImpl(const AuthParams& params,
                                         AuthResults* results) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  results->credentials_saved = params.should_save_credentials;
  results->auth_error = ConnectionUnavailable;
  // Save credentials if so requested.
  if (params.should_save_credentials != DONT_SAVE_CREDENTIALS) {
    results->email = params.email.data();
    results->password = params.password;
  } else {  // Explicitly clear previously-saved credentials.
    results->email = "";
    results->password = "";
  }

  // The aim of this code is to start failing requests if due to a logic error
  // in the program we're hammering GAIA.
  time_t now = time(0);
  if (now > next_allowed_auth_attempt_time_) {
    next_allowed_auth_attempt_time_ = now + 1;
    // If we're more than 2 minutes past the allowed time we reset the early
    // attempt count.
    if (now - next_allowed_auth_attempt_time_ > 2 * 60) {
      delay_ = 1;
      early_auth_attempt_count_ = 0;
    }
  } else {
    ++early_auth_attempt_count_;
    // Allow 3 attempts, but then limit.
    if (early_auth_attempt_count_ > 3) {
      delay_ = GetBackoffDelaySeconds(delay_);
      next_allowed_auth_attempt_time_ = now + delay_;
      return false;
    }
  }

  return PerformGaiaRequest(params, results);
}

bool GaiaAuthenticator::PerformGaiaRequest(const AuthParams& params,
                                           AuthResults* results) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  GURL gaia_auth_url(gaia_url_);

  string post_body;
  post_body += "Email=" + EscapeUrlEncodedData(params.email);
  post_body += "&Passwd=" + EscapeUrlEncodedData(params.password);
  post_body += "&source=" + EscapeUrlEncodedData(user_agent_);
  post_body += "&service=" + service_id_;
  if (!params.captcha_token.empty() && !params.captcha_value.empty()) {
    post_body += "&logintoken=" + EscapeUrlEncodedData(params.captcha_token);
    post_body += "&logincaptcha=" + EscapeUrlEncodedData(params.captcha_value);
  }
  post_body += "&PersistentCookie=true";
  // We set it to GOOGLE (and not HOSTED or HOSTED_OR_GOOGLE) because we only
  // allow consumer logins.
  post_body += "&accountType=GOOGLE";

  string message_text;
  unsigned long server_response_code;
  if (!Post(gaia_auth_url, post_body, &server_response_code, &message_text)) {
    results->auth_error = ConnectionUnavailable;
    return false;
  }

  // Parse reply in two different ways, depending on if request failed or
  // succeeded.
  if (RC_FORBIDDEN == server_response_code) {
    ExtractAuthErrorFrom(message_text, results);
    return false;
  } else if (RC_REQUEST_OK == server_response_code) {
    ExtractTokensFrom(message_text, results);
    const bool old_gaia =
      results->auth_token.empty() && !results->lsid.empty();
    const bool long_lived_token =
      params.should_save_credentials == PERSIST_TO_DISK;
    if ((old_gaia || long_lived_token) &&
        !IssueAuthToken(results, service_id_, long_lived_token))
      return false;

    return LookupEmail(results);
  } else {
    results->auth_error = Unknown;
    return false;
  }
}

bool GaiaAuthenticator::LookupEmail(AuthResults* results) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  // Use the provided Gaia server, but change the path to what V1 expects.
  GURL url(gaia_url_);  // Gaia server.
  GURL::Replacements repl;
  // Needs to stay in scope till GURL is out of scope.
  string path(kGetUserInfoPath);
  repl.SetPathStr(path);
  url = url.ReplaceComponents(repl);

  string post_body;
  post_body += "LSID=";
  post_body += EscapeUrlEncodedData(results->lsid);

  unsigned long server_response_code;
  string message_text;
  if (!Post(url, post_body, &server_response_code, &message_text)) {
    return false;
  }

  // Check if we received a valid AuthToken; if not, ignore it.
  if (RC_FORBIDDEN == server_response_code) {
    // Server says we're not authenticated.
    ExtractAuthErrorFrom(message_text, results);
    return false;
  } else if (RC_REQUEST_OK == server_response_code) {
    typedef vector<pair<string, string> > Tokens;
    Tokens tokens;
    base::SplitStringIntoKeyValuePairs(message_text, '=', '\n', &tokens);
    for (Tokens::iterator i = tokens.begin(); i != tokens.end(); ++i) {
      if ("accountType" == i->first) {
        // We never authenticate an email as a hosted account.
        DCHECK_EQ("GOOGLE", i->second);
        results->signin = GMAIL_SIGNIN;
      } else if ("email" == i->first) {
        results->primary_email = i->second;
      }
    }
    return true;
  }
  return false;
}

// We need to call this explicitly when we need to obtain a long-lived session
// token.
bool GaiaAuthenticator::IssueAuthToken(AuthResults* results,
                                       const string& service_id,
                                       bool long_lived) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  // Use the provided Gaia server, but change the path to what V1 expects.
  GURL url(gaia_url_);  // Gaia server.
  GURL::Replacements repl;
  // Needs to stay in scope till GURL is out of scope.
  string path(kGaiaV1IssueAuthTokenPath);
  repl.SetPathStr(path);
  url = url.ReplaceComponents(repl);

  string post_body;
  post_body += "LSID=";
  post_body += EscapeUrlEncodedData(results->lsid);
  post_body += "&service=" + service_id;
  if (long_lived) {
    post_body += "&Session=true";
  }

  unsigned long server_response_code;
  string message_text;
  if (!Post(url, post_body, &server_response_code, &message_text)) {
    return false;
  }

  // Check if we received a valid AuthToken; if not, ignore it.
  if (RC_FORBIDDEN == server_response_code) {
    // Server says we're not authenticated.
    ExtractAuthErrorFrom(message_text, results);
    return false;
  } else if (RC_REQUEST_OK == server_response_code) {
    // Note that the format of message_text is different from what is returned
    // in the first request, or to the sole request that is made to Gaia V2.
    // Specifically, the entire string is the AuthToken, and looks like:
    // "<token>" rather than "AuthToken=<token>". Thus, we need not use
    // ExtractTokensFrom(...), but simply assign the token.
    int last_index = message_text.length() - 1;
    if ('\n' == message_text[last_index])
      message_text.erase(last_index);
    results->auth_token = message_text;
    return true;
  }
  return false;
}

// Helper method that extracts tokens from a successful reply, and saves them
// in the right fields.
void GaiaAuthenticator::ExtractTokensFrom(const string& response,
                                          AuthResults* results) {
  vector<pair<string, string> > tokens;
  base::SplitStringIntoKeyValuePairs(response, '=', '\n', &tokens);
  for (vector<pair<string, string> >::iterator i = tokens.begin();
      i != tokens.end(); ++i) {
    if (i->first == "SID") {
      results->sid = i->second;
    } else if (i->first == "LSID") {
      results->lsid = i->second;
    } else if (i->first == "Auth") {
      results->auth_token = i->second;
    }
  }
}

// Helper method that extracts tokens from a failure response, and saves them
// in the right fields.
void GaiaAuthenticator::ExtractAuthErrorFrom(const string& response,
                                             AuthResults* results) {
  vector<pair<string, string> > tokens;
  base::SplitStringIntoKeyValuePairs(response, '=', '\n', &tokens);
  for (vector<pair<string, string> >::iterator i = tokens.begin();
      i != tokens.end(); ++i) {
    if (i->first == "Error") {
      results->error_msg = i->second;
    } else if (i->first == "Url") {
      results->auth_error_url = i->second;
    } else if (i->first == "CaptchaToken") {
      results->captcha_token = i->second;
    } else if (i->first == "CaptchaUrl") {
      results->captcha_url = i->second;
    }
  }

  // Convert string error messages to enum values. Each case has two different
  // strings; the first one is the most current and the second one is
  // deprecated, but available.
  const string& error_msg = results->error_msg;
  if (error_msg == "BadAuthentication" || error_msg == "badauth") {
    results->auth_error = BadAuthentication;
  } else if (error_msg == "NotVerified" || error_msg == "nv") {
    results->auth_error = NotVerified;
  } else if (error_msg == "TermsNotAgreed" || error_msg == "tna") {
    results->auth_error = TermsNotAgreed;
  } else if (error_msg == "Unknown" || error_msg == "unknown") {
    results->auth_error = Unknown;
  } else if (error_msg == "AccountDeleted" || error_msg == "adel") {
    results->auth_error = AccountDeleted;
  } else if (error_msg == "AccountDisabled" || error_msg == "adis") {
    results->auth_error = AccountDisabled;
  } else if (error_msg == "CaptchaRequired" || error_msg == "cr") {
    results->auth_error = CaptchaRequired;
  } else if (error_msg == "ServiceUnavailable" || error_msg == "ire") {
    results->auth_error = ServiceUnavailable;
  }
}

// Reset all stored credentials, perhaps in preparation for letting a different
// user sign in.
void GaiaAuthenticator::ResetCredentials() {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  AuthResults blank;
  auth_results_ = blank;
}

void GaiaAuthenticator::SetUsernamePassword(const string& username,
                                            const string& password) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  auth_results_.password = password;
  auth_results_.email = username;
}

void GaiaAuthenticator::SetUsername(const string& username) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  auth_results_.email = username;
}

void GaiaAuthenticator::RenewAuthToken(const string& auth_token) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  DCHECK(!this->auth_token().empty());
  auth_results_.auth_token = auth_token;
}
void GaiaAuthenticator::SetAuthToken(const string& auth_token,
                                     SaveCredentials save) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  auth_results_.auth_token = auth_token;
  auth_results_.credentials_saved = save;
}

bool GaiaAuthenticator::Authenticate(const string& user_name,
                                     const string& password,
                                     SaveCredentials should_save_credentials,
                                     SignIn try_first) {
  DCHECK_EQ(MessageLoop::current(), message_loop_);
  const string empty;
  return Authenticate(user_name, password, should_save_credentials, empty,
                      empty, try_first);
}

}  // namespace browser_sync
