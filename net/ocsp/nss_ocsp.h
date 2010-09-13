// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_OCSP_NSS_OCSP_H_
#define NET_OCSP_NSS_OCSP_H_
#pragma once

class MessageLoopForIO;
class URLRequestContext;

namespace net {

// Sets the MessageLoop for OCSP.  This should be called before EnsureOCSPInit()
// if you want to control the message loop for OCSP.
void SetMessageLoopForOCSP(MessageLoopForIO* message_loop);

// Initializes OCSP handlers for NSS.  This must be called before any
// certificate verification functions.  This function is thread-safe, and OCSP
// handlers will only ever be initialized once.
void EnsureOCSPInit();

// Set URLRequestContext for OCSP handlers.
void SetURLRequestContextForOCSP(URLRequestContext* request_context);
URLRequestContext* GetURLRequestContextForOCSP();

}  // namespace net

#endif  // NET_OCSP_NSS_OCSP_H_
