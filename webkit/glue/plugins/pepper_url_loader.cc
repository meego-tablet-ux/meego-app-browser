// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/glue/plugins/pepper_url_loader.h"

#include "base/logging.h"
#include "third_party/ppapi/c/pp_completion_callback.h"
#include "third_party/ppapi/c/pp_errors.h"
#include "third_party/ppapi/c/ppb_url_loader.h"
#include "third_party/WebKit/WebKit/chromium/public/WebDocument.h"
#include "third_party/WebKit/WebKit/chromium/public/WebElement.h"
#include "third_party/WebKit/WebKit/chromium/public/WebFrame.h"
#include "third_party/WebKit/WebKit/chromium/public/WebKit.h"
#include "third_party/WebKit/WebKit/chromium/public/WebKitClient.h"
#include "third_party/WebKit/WebKit/chromium/public/WebPluginContainer.h"
#include "webkit/glue/plugins/pepper_plugin_instance.h"
#include "webkit/glue/plugins/pepper_url_request_info.h"
#include "webkit/glue/plugins/pepper_url_response_info.h"

using WebKit::WebFrame;
using WebKit::WebURL;
using WebKit::WebURLError;
using WebKit::WebURLLoader;
using WebKit::WebURLRequest;
using WebKit::WebURLResponse;

#ifdef _MSC_VER
// Do not warn about use of std::copy with raw pointers.
#pragma warning(disable : 4996)
#endif

namespace pepper {

namespace {

PP_Resource Create(PP_Instance instance_id) {
  PluginInstance* instance = PluginInstance::FromPPInstance(instance_id);
  if (!instance)
    return 0;

  URLLoader* loader = new URLLoader(instance);
  loader->AddRef();  // AddRef for the caller.

  return loader->GetResource();
}

bool IsURLLoader(PP_Resource resource) {
  return !!Resource::GetAs<URLLoader>(resource).get();
}

int32_t Open(PP_Resource loader_id,
             PP_Resource request_id,
             PP_CompletionCallback callback) {
  scoped_refptr<URLLoader> loader(Resource::GetAs<URLLoader>(loader_id));
  if (!loader.get())
    return PP_Error_BadResource;

  scoped_refptr<URLRequestInfo> request(
      Resource::GetAs<URLRequestInfo>(request_id));
  if (!request.get())
    return PP_Error_BadResource;

  return loader->Open(request, callback);
}

int32_t FollowRedirect(PP_Resource loader_id,
                       PP_CompletionCallback callback) {
  scoped_refptr<URLLoader> loader(Resource::GetAs<URLLoader>(loader_id));
  if (!loader.get())
    return PP_Error_BadResource;

  return loader->FollowRedirect(callback);
}

bool GetUploadProgress(PP_Resource loader_id,
                       int64_t* bytes_sent,
                       int64_t* total_bytes_to_be_sent) {
  scoped_refptr<URLLoader> loader(Resource::GetAs<URLLoader>(loader_id));
  if (!loader.get())
    return false;

  *bytes_sent = loader->bytes_sent();
  *total_bytes_to_be_sent = loader->total_bytes_to_be_sent();
  return true;
}

bool GetDownloadProgress(PP_Resource loader_id,
                         int64_t* bytes_received,
                         int64_t* total_bytes_to_be_received) {
  scoped_refptr<URLLoader> loader(Resource::GetAs<URLLoader>(loader_id));
  if (!loader.get())
    return false;

  *bytes_received = loader->bytes_received();
  *total_bytes_to_be_received = loader->total_bytes_to_be_received();
  return true;
}

PP_Resource GetResponseInfo(PP_Resource loader_id) {
  scoped_refptr<URLLoader> loader(Resource::GetAs<URLLoader>(loader_id));
  if (!loader.get())
    return 0;

  URLResponseInfo* response_info = loader->response_info();
  if (!response_info)
    return 0;
  response_info->AddRef();  // AddRef for the caller.

  return response_info->GetResource();
}

int32_t ReadResponseBody(PP_Resource loader_id,
                         char* buffer,
                         int32_t bytes_to_read,
                         PP_CompletionCallback callback) {
  scoped_refptr<URLLoader> loader(Resource::GetAs<URLLoader>(loader_id));
  if (!loader.get())
    return PP_Error_BadResource;

  return loader->ReadResponseBody(buffer, bytes_to_read, callback);
}

void Close(PP_Resource loader_id) {
  scoped_refptr<URLLoader> loader(Resource::GetAs<URLLoader>(loader_id));
  if (!loader.get())
    return;

  loader->Close();
}

const PPB_URLLoader ppb_urlloader = {
  &Create,
  &IsURLLoader,
  &Open,
  &FollowRedirect,
  &GetUploadProgress,
  &GetDownloadProgress,
  &GetResponseInfo,
  &ReadResponseBody,
  &Close
};

}  // namespace

URLLoader::URLLoader(PluginInstance* instance)
    : Resource(instance->module()),
      instance_(instance),
      pending_callback_(),
      bytes_sent_(0),
      total_bytes_to_be_sent_(0),
      bytes_received_(0),
      total_bytes_to_be_received_(0) {
}

URLLoader::~URLLoader() {
}

// static
const PPB_URLLoader* URLLoader::GetInterface() {
  return &ppb_urlloader;
}

int32_t URLLoader::Open(URLRequestInfo* request,
                        PP_CompletionCallback callback) {
  if (loader_.get())
    return PP_Error_InProgress;

  // We only support non-blocking calls.
  if (!callback.func)
    return PP_Error_BadArgument;

  WebURLRequest web_request(request->web_request());

  WebFrame* frame = instance_->container()->element().document().frame();
  if (!frame)
    return PP_Error_Failed;
  frame->setReferrerForRequest(web_request, WebURL());  // Use default.
  frame->dispatchWillSendRequest(web_request);

  loader_.reset(WebKit::webKitClient()->createURLLoader());
  if (!loader_.get()) {
    loader_.reset();
    return PP_Error_Failed;
  }
  loader_->loadAsynchronously(web_request, this);

  pending_callback_ = callback;

  // Notify completion when we receive a redirect or response headers.
  return PP_Error_WouldBlock;
}

int32_t URLLoader::FollowRedirect(PP_CompletionCallback callback) {
  NOTIMPLEMENTED();  // TODO(darin): Implement me.
  return PP_Error_Failed;
}

int32_t URLLoader::ReadResponseBody(char* buffer, int32_t bytes_to_read,
                                    PP_CompletionCallback callback) {
  if (bytes_to_read <= 0 || !buffer)
    return PP_Error_BadArgument;
  if (pending_callback_.func)
    return PP_Error_InProgress;

  // We only support non-blocking calls.
  if (!callback.func)
    return PP_Error_BadArgument;

  user_buffer_ = buffer;
  user_buffer_size_ = bytes_to_read;

  if (!buffer_.empty())
    return FillUserBuffer();

  pending_callback_ = callback;
  return PP_Error_WouldBlock;
}

void URLLoader::Close() {
  NOTIMPLEMENTED();  // TODO(darin): Implement me.
}

void URLLoader::willSendRequest(WebURLLoader* loader,
                                WebURLRequest& new_request,
                                const WebURLResponse& redirect_response) {
  NOTIMPLEMENTED();  // TODO(darin): Allow the plugin to inspect redirects.
}

void URLLoader::didSendData(WebURLLoader* loader,
                            unsigned long long bytes_sent,
                            unsigned long long total_bytes_to_be_sent) {
  // TODO(darin): Bounds check input?
  bytes_sent_ = static_cast<int64_t>(bytes_sent);
  total_bytes_to_be_sent_ = static_cast<int64_t>(total_bytes_to_be_sent);
}

void URLLoader::didReceiveResponse(WebURLLoader* loader,
                                   const WebURLResponse& response) {
  // TODO(darin): Initialize response_info_.
  RunCallback(PP_OK);
}

void URLLoader::didReceiveData(WebURLLoader* loader,
                               const char* data,
                               int data_length) {
  buffer_.insert(buffer_.end(), data, data + data_length);
  if (user_buffer_) {
    RunCallback(FillUserBuffer());
  } else {
    DCHECK(!pending_callback_.func);
  }
}

void URLLoader::didFinishLoading(WebURLLoader* loader) {
  RunCallback(PP_OK);
}

void URLLoader::didFail(WebURLLoader* loader, const WebURLError& error) {
  // TODO(darin): Provide more detailed error information.
  RunCallback(PP_Error_Failed);
}

void URLLoader::RunCallback(int32_t result) {
  if (!pending_callback_.func)
    return;

  PP_CompletionCallback callback = {0};
  std::swap(callback, pending_callback_);
  PP_RunCompletionCallback(&callback, result);
}

size_t URLLoader::FillUserBuffer() {
  DCHECK(user_buffer_);
  DCHECK(user_buffer_size_);

  size_t bytes_to_copy = std::min(buffer_.size(), user_buffer_size_);
  std::copy(buffer_.begin(), buffer_.begin() + bytes_to_copy, user_buffer_);
  buffer_.erase(buffer_.begin(), buffer_.begin() + bytes_to_copy);

  // Reset for next time.
  user_buffer_ = NULL;
  user_buffer_size_ = 0;
  return bytes_to_copy;
}

}  // namespace pepper
