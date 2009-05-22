// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_GLUE_WEBURLREQUEST_IMPL_H_
#define WEBKIT_GLUE_WEBURLREQUEST_IMPL_H_

#include "base/compiler_specific.h"
#include "webkit/glue/weburlrequest.h"

MSVC_PUSH_WARNING_LEVEL(0);
#include "FrameLoadRequest.h"
#include "HistoryItem.h"
MSVC_POP_WARNING();

class WebRequestImpl : public WebRequest {
 public:
  WebRequestImpl();

  explicit WebRequestImpl(const GURL& url);
  explicit WebRequestImpl(const WebCore::ResourceRequest& request);
  explicit WebRequestImpl(const WebCore::FrameLoadRequest& request);

  // WebRequest
  virtual WebRequest* Clone() const;
  virtual void SetURL(const GURL& url);
  virtual GURL GetURL() const;
  virtual void SetMainDocumentURL(const GURL& url);
  virtual GURL GetMainDocumentURL() const;
  virtual WebRequestCachePolicy GetCachePolicy() const;
  virtual void SetCachePolicy(WebRequestCachePolicy policy);
  virtual std::string GetHttpMethod() const;
  virtual void SetHttpMethod(const std::string& method);
  virtual std::string GetHttpHeaderValue(const std::string& field) const;
  virtual void SetHttpHeaderValue(const std::string& field,
      const std::string& value);
  virtual void GetHttpHeaders(HeaderMap* headers) const;
  virtual void SetHttpHeaders(const HeaderMap& headers);
  virtual std::string GetHttpReferrer() const;
  virtual std::string GetSecurityInfo() const;
  virtual void SetSecurityInfo(const std::string& value);
  virtual bool HasUploadData() const;
  virtual void GetUploadData(net::UploadData* data) const;
  virtual void SetUploadData(const net::UploadData& data);
  virtual void SetRequestorID(int requestor_id);

  // WebRequestImpl
  const WebCore::FrameLoadRequest& frame_load_request() const {
    return request_;
  }
  void set_frame_load_request(const WebCore::FrameLoadRequest& request) {
    request_ = request;
  }

 protected:
  WebCore::FrameLoadRequest request_;
};

#endif  // #ifndef WEBKIT_GLUE_WEBURLREQUEST_IMPL_H_
