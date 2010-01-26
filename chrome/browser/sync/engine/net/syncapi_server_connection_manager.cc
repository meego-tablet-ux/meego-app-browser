// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/engine/net/syncapi_server_connection_manager.h"

#include "chrome/browser/sync/engine/net/http_return.h"
#include "chrome/browser/sync/engine/syncapi.h"

using browser_sync::HttpResponse;
using std::string;

namespace sync_api {

bool SyncAPIBridgedPost::Init(const char* path, const string& auth_token,
    const string& payload, HttpResponse* response) {
  string sync_server;
  int sync_server_port = 0;
  bool use_ssl = false;
  GetServerParams(&sync_server, &sync_server_port, &use_ssl);
  std::string connection_url = MakeConnectionURL(sync_server, path, use_ssl);

  HttpPostProviderInterface* http = factory_->Create();
  http->SetUserAgent(scm_->user_agent().c_str());
  http->SetURL(connection_url.c_str(), sync_server_port);

  if (!auth_token.empty()) {
    string headers = "Authorization: GoogleLogin auth=" + auth_token;
    http->SetExtraRequestHeaders(headers.c_str());
  }

  // Must be octet-stream, or the payload may be parsed for a cookie.
  http->SetPostPayload("application/octet-stream", payload.length(),
                       payload.data());

  // Issue the POST, blocking until it finishes.
  int os_error_code = 0;
  int response_code = 0;
  if (!http->MakeSynchronousPost(&os_error_code, &response_code)) {
    LOG(INFO) << "Http POST failed, error returns: " << os_error_code;
    response->server_status = HttpResponse::IO_ERROR;
    return false;
  }

  // We got a server response, copy over response codes and content.
  response->response_code = response_code;
  response->content_length =
      static_cast<int64>(http->GetResponseContentLength());
  response->payload_length =
      static_cast<int64>(http->GetResponseContentLength());
  if (response->response_code < 400)
    response->server_status = HttpResponse::SERVER_CONNECTION_OK;
  else if (response->response_code == browser_sync::RC_UNAUTHORIZED)
    response->server_status = HttpResponse::SYNC_AUTH_ERROR;
  else
    response->server_status = HttpResponse::SYNC_SERVER_ERROR;

  // Write the content into our buffer.
  buffer_.assign(http->GetResponseContent(), http->GetResponseContentLength());

  // We're done with the HttpPostProvider.
  factory_->Destroy(http);
  return true;
}

SyncAPIServerConnectionManager::~SyncAPIServerConnectionManager() {}

}  // namespace sync_api
