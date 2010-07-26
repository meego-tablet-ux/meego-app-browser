// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_VARY_DATA_H__
#define NET_HTTP_HTTP_VARY_DATA_H__
#pragma once

#include "base/md5.h"

class Pickle;

namespace net {

struct HttpRequestInfo;
class HttpResponseHeaders;

// Used to implement the HTTP/1.1 Vary header.  This class contains a MD5 hash
// over the request headers indicated by a Vary header.
//
// While RFC 2616 requires strict request header comparisons, it is much
// cheaper to store a MD5 sum, which should be sufficient.  Storing a hash also
// avoids messy privacy issues as some of the request headers could hold
// sensitive data (e.g., cookies).
//
// NOTE: This class does not hold onto the contents of the Vary header.
// Instead, it relies on the consumer to store that and to supply it again to
// the MatchesRequest function for comparing against future HTTP requests.
//
class HttpVaryData {
 public:
  HttpVaryData();

  bool is_valid() const { return is_valid_; }

  // Initialize from a request and its corresponding response headers.
  //
  // Returns true if a Vary header was found in the response headers and that
  // Vary header was not empty and did not contain the '*' value.  Upon
  // success, the object is also marked as valid such that is_valid() will
  // return true.  Otherwise, false is returned to indicate that this object
  // is marked as invalid.
  //
  bool Init(const HttpRequestInfo& request_info,
            const HttpResponseHeaders& response_headers);

  // Initialize from a pickle that contains data generated by a call to the
  // vary data's Persist method.
  //
  // Upon success, true is returned and the object is marked as valid such that
  // is_valid() will return true.  Otherwise, false is returned to indicate
  // that this object is marked as invalid.
  //
  bool InitFromPickle(const Pickle& pickle, void** pickle_iter);

  // Call this method to persist the vary data. Illegal to call this on an
  // invalid object.
  void Persist(Pickle* pickle) const;

  // Call this method to test if the given request matches the previous request
  // with which this vary data corresponds.  The |cached_response_headers| must
  // be the same response headers used to generate this vary data.
  bool MatchesRequest(const HttpRequestInfo& request_info,
                      const HttpResponseHeaders& cached_response_headers) const;

 private:
  // Returns the corresponding request header value.
  static std::string GetRequestValue(const HttpRequestInfo& request_info,
                                     const std::string& request_header);

  // Append to the MD5 context for the given request header.
  static void AddField(const HttpRequestInfo& request_info,
                       const std::string& request_header,
                       MD5Context* context);

  // A digested version of the request headers corresponding to the Vary header.
  MD5Digest request_digest_;

  // True when request_digest_ contains meaningful data.
  bool is_valid_;
};

}  // namespace net

#endif  // NET_HTTP_HTTP_VARY_DATA_H__
