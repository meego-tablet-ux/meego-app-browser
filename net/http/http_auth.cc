// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_auth.h"

#include <algorithm>

#include "base/basictypes.h"
#include "base/string_util.h"
#include "net/http/http_auth_handler_basic.h"
#include "net/http/http_auth_handler_digest.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_util.h"

namespace net {

// static
HttpAuthHandler* HttpAuth::ChooseBestChallenge(
    const HttpResponseHeaders* headers, Target target) {
  // Choose the challenge whose authentication handler gives the maximum score.
  scoped_ptr<HttpAuthHandler> best;
  const std::string header_name = GetChallengeHeaderName(target);
  std::string cur_challenge;
  void* iter = NULL;
  while (headers->EnumerateHeader(&iter, header_name, &cur_challenge)) {
    scoped_ptr<HttpAuthHandler> cur(
        CreateAuthHandler(cur_challenge, target));
    if (cur.get() && (!best.get() || best->score() < cur->score()))
      best.reset(cur.release());
  }
  return best.release();
}

// static
HttpAuthHandler* HttpAuth::CreateAuthHandler(const std::string& challenge,
                                             Target target) {
  // Find the right auth handler for the challenge's scheme.
  ChallengeTokenizer props(challenge.begin(), challenge.end());
  scoped_ptr<HttpAuthHandler> handler;

  if (LowerCaseEqualsASCII(props.scheme(), "basic")) {
    handler.reset(new HttpAuthHandlerBasic());
  } else if (LowerCaseEqualsASCII(props.scheme(), "digest")) {
    handler.reset(new HttpAuthHandlerDigest());
  }
  if (handler.get()) {
    if (!handler->InitFromChallenge(challenge.begin(), challenge.end(),
                                    target)) {
      // Invalid/unsupported challenge.
      return NULL; 
    }
  }
  return handler.release();
}

void HttpAuth::ChallengeTokenizer::Init(std::string::const_iterator begin,
                                        std::string::const_iterator end) {
  // The first space-separated token is the auth-scheme.
  // NOTE: we are more permissive than RFC 2617 which says auth-scheme
  // is separated by 1*SP.
  StringTokenizer tok(begin, end, HTTP_LWS);
  if (!tok.GetNext()) {
    valid_ = false;
    return;
  }

  // Save the scheme's position.
  scheme_begin_ = tok.token_begin();
  scheme_end_ = tok.token_end();

  // Everything past scheme_end_ is a (comma separated) value list.
  if (scheme_end_ != end)
    props_ = HttpUtil::ValuesIterator(scheme_end_ + 1, end, ',');
}

// We expect properties to be formatted as one of:
//   name="value"
//   name=value
//   name=
bool HttpAuth::ChallengeTokenizer::GetNext() {
  if (!props_.GetNext())
    return false;

  // Set the value as everything. Next we will split out the name.
  value_begin_ = props_.value_begin();
  value_end_ = props_.value_end();
  name_begin_ = name_end_ = value_end_;

  // Scan for the equals sign.
  std::string::const_iterator equals = std::find(value_begin_, value_end_, '=');
  if (equals == value_end_ || equals == value_begin_)
    return valid_ = false; // Malformed

  // Verify that the equals sign we found wasn't inside of quote marks.
  for (std::string::const_iterator it = value_begin_; it != equals; ++it) {
    if (HttpUtil::IsQuote(*it))
      return valid_ = false; // Malformed
  }

  name_begin_ = value_begin_;
  name_end_ = equals;
  value_begin_ = equals + 1;
  
  if (value_begin_ != value_end_ && HttpUtil::IsQuote(*value_begin_)) {
    // Trim surrounding quotemarks off the value
    if (*value_begin_ != *(value_end_ - 1))
      return valid_ = false; // Malformed -- mismatching quotes.
    value_is_quoted_ = true;
  } else {
    value_is_quoted_ = false;
  }
  return true;
}

// If value() has quotemarks, unquote it.
std::string HttpAuth::ChallengeTokenizer::unquoted_value() const {
  return HttpUtil::Unquote(value_begin_, value_end_);
}

// static
std::string HttpAuth::GetChallengeHeaderName(Target target) {
  switch(target) {
    case AUTH_PROXY:
      return "Proxy-Authenticate";
    case AUTH_SERVER:
      return "WWW-Authenticate";
    default:
      NOTREACHED();
      return "";
  }
}

// static
std::string HttpAuth::GetAuthorizationHeaderName(Target target) {
  switch(target) {
    case AUTH_PROXY:
      return "Proxy-Authorization";
    case AUTH_SERVER:
      return "Authorization";
    default:
      NOTREACHED();
      return "";
  }
}

}  // namespace net
