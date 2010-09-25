// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PLUGIN_DOWNLOAD_HELPER_H_
#define CHROME_BROWSER_PLUGIN_DOWNLOAD_HELPER_H_
#pragma once

#include <string>
#include "build/build_config.h"

#if defined(OS_WIN)
#include "base/file_path.h"
#include "gfx/native_widget_types.h"
#include "net/base/file_stream.h"
#include "net/url_request/url_request.h"

// The PluginDownloadUrlHelper is used to handle one download URL request
// from the plugin. Each download request is handled by a new instance
// of this class.
class PluginDownloadUrlHelper : public URLRequest::Delegate {
  static const int kDownloadFileBufferSize = 32768;
 public:
  // The delegate receives notification about the status of downloads
  // initiated.
  class Delegate {
   public:
    virtual ~Delegate() {}

    virtual void OnDownloadCompleted(const FilePath& download_path,
                                     bool success) {}
  };

  PluginDownloadUrlHelper(const std::string& download_url,
                          int source_pid, gfx::NativeWindow caller_window,
                          PluginDownloadUrlHelper::Delegate* delegate);
  ~PluginDownloadUrlHelper();

  void InitiateDownload(URLRequestContext* request_context);

  // URLRequest::Delegate
  virtual void OnAuthRequired(URLRequest* request,
                              net::AuthChallengeInfo* auth_info);
  virtual void OnSSLCertificateError(URLRequest* request,
                                     int cert_error,
                                     net::X509Certificate* cert);
  virtual void OnResponseStarted(URLRequest* request);
  virtual void OnReadCompleted(URLRequest* request, int bytes_read);

  void OnDownloadCompleted(URLRequest* request);

 protected:
  void DownloadCompletedHelper(bool success);

  // The download file request initiated by the plugin.
  URLRequest* download_file_request_;
  // Handle to the downloaded file.
  scoped_ptr<net::FileStream> download_file_;
  // The full path of the downloaded file.
  FilePath download_file_path_;
  // The buffer passed off to URLRequest::Read.
  scoped_refptr<net::IOBuffer> download_file_buffer_;
  // TODO(port): this comment doesn't describe the situation on Posix.
  // The window handle for sending the WM_COPYDATA notification,
  // indicating that the download completed.
  gfx::NativeWindow download_file_caller_window_;

  std::string download_url_;
  int download_source_child_unique_id_;

  PluginDownloadUrlHelper::Delegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(PluginDownloadUrlHelper);
};

#endif  // OS_WIN

#endif  // CHROME_BROWSER_PLUGIN_DOWNLOAD_HELPER_H_


