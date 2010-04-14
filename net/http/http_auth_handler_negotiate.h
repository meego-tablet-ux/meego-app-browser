// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_AUTH_HANDLER_NEGOTIATE_H_
#define NET_HTTP_HTTP_AUTH_HANDLER_NEGOTIATE_H_

#include "build/build_config.h"

#include <string>

#include "net/base/address_list.h"
#include "net/http/http_auth_handler.h"
#include "net/http/http_auth_handler_factory.h"

#if defined(OS_WIN)
#include "net/http/http_auth_sspi_win.h"
#endif

namespace net {

class SingleRequestHostResolver;

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

    // |disable_cname_lookup()| and |set_disable_cname_lookup()| get/set whether
    // the auth handlers generated by this factory should skip looking up the
    // canonical DNS name of the the host that they are authenticating to when
    // generating the SPN. The default value is false.
    bool disable_cname_lookup() const { return disable_cname_lookup_; }
    void set_disable_cname_lookup(bool disable_cname_lookup) {
      disable_cname_lookup_ = disable_cname_lookup;
    }

    // |use_port()| and |set_use_port()| get/set whether the auth handlers
    // generated by this factory should include the port number of the server
    // they are authenticating to when constructing a Kerberos SPN. The default
    // value is false.
    bool use_port() const { return use_port_; }
    void set_use_port(bool use_port) { use_port_ = use_port; }

    virtual int CreateAuthHandler(HttpAuth::ChallengeTokenizer* challenge,
                                  HttpAuth::Target target,
                                  const GURL& origin,
                                  scoped_refptr<HttpAuthHandler>* handler);

#if defined(OS_WIN)
    // Set the SSPILibrary to use. Typically the only callers which need to
    // use this are unit tests which pass in a mocked-out version of the
    // SSPI library.
    // The caller is responsible for managing the lifetime of |*sspi_library|,
    // and the lifetime must exceed that of this Factory object and all
    // HttpAuthHandler's that this Factory object creates.
    void set_sspi_library(SSPILibrary* sspi_library) {
      sspi_library_ = sspi_library;
    }
#endif  // defined(OS_WIN)
   private:
    bool disable_cname_lookup_;
    bool use_port_;
#if defined(OS_WIN)
    ULONG max_token_length_;
    bool first_creation_;
    bool is_unsupported_;
    SSPILibrary* sspi_library_;
#endif  // defined(OS_WIN)
  };

#if defined(OS_WIN)
  HttpAuthHandlerNegotiate(SSPILibrary* sspi_library, ULONG max_token_length,
                           bool disable_cname_lookup, bool use_port);
#else
  HttpAuthHandlerNegotiate();
#endif

  virtual bool NeedsIdentity();

  virtual bool IsFinalRound();

  virtual bool SupportsDefaultCredentials();

  virtual bool NeedsCanonicalName();

  virtual int GenerateAuthToken(const std::wstring& username,
                                const std::wstring& password,
                                const HttpRequestInfo* request,
                                const ProxyInfo* proxy,
                                std::string* auth_token);

  virtual int GenerateDefaultAuthToken(const HttpRequestInfo* request,
                                       const ProxyInfo* proxy,
                                       std::string* auth_token);

  virtual int ResolveCanonicalName(HostResolver* host_resolver,
                                   CompletionCallback* callback,
                                   const BoundNetLog& net_log);

 protected:
  virtual bool Init(HttpAuth::ChallengeTokenizer* challenge);

 private:
  ~HttpAuthHandlerNegotiate();

#if defined(OS_WIN)
  void OnResolveCanonicalName(int result);
  std::wstring CreateSPN(const AddressList& address_list, const GURL& orign);

  HttpAuthSSPI auth_sspi_;
  AddressList address_list_;
  scoped_ptr<SingleRequestHostResolver> single_resolve_;
  CompletionCallback* user_callback_;
  CompletionCallbackImpl<HttpAuthHandlerNegotiate> resolve_cname_callback_;
  bool disable_cname_lookup_;
  bool use_port_;
  std::wstring spn_;
#endif
};

}  // namespace net

#endif  // NET_HTTP_HTTP_AUTH_HANDLER_NEGOTIATE_H_
