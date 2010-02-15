// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_AUTH_HANDLER_NEGOTIATE_H_
#define NET_HTTP_HTTP_AUTH_HANDLER_NEGOTIATE_H_

#include "build/build_config.h"

#include <string>

#include "net/http/http_auth_handler.h"
#include "net/http/http_auth_handler_factory.h"

#if defined(OS_WIN)
#include "net/http/http_auth_sspi_win.h"
#endif

namespace net {

// Handler for WWW-Authenticate: Negotiate protocol.
//
// See http://tools.ietf.org/html/rfc4178 and http://tools.ietf.org/html/rfc4559
// for more information about the protocol.

class HttpAuthHandlerNegotiate : public HttpAuthHandler {
 public:
  class Factory : public HttpAuthHandlerFactory {
   public:
    Factory();
    virtual ~Factory();

    virtual int CreateAuthHandler(HttpAuth::ChallengeTokenizer* challenge,
                                  HttpAuth::Target target,
                                  const GURL& origin,
                                  scoped_refptr<HttpAuthHandler>* handler);
  };

  HttpAuthHandlerNegotiate();

  virtual bool NeedsIdentity();

  virtual bool IsFinalRound();

  virtual bool AllowDefaultCredentials();

  virtual int GenerateAuthToken(const std::wstring& username,
                                const std::wstring& password,
                                const HttpRequestInfo* request,
                                const ProxyInfo* proxy,
                                std::string* auth_token);

  virtual int GenerateDefaultAuthToken(const HttpRequestInfo* request,
                                       const ProxyInfo* proxy,
                                       std::string* auth_token);

 protected:
  virtual bool Init(HttpAuth::ChallengeTokenizer* challenge);

 private:
  ~HttpAuthHandlerNegotiate();

#if defined(OS_WIN)
  HttpAuthSSPI auth_sspi_;
#endif
};

}  // namespace net

#endif  // NET_HTTP_HTTP_AUTH_HANDLER_NEGOTIATE_H_
