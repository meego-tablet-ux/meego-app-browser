// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_host/download_resource_handler.h"

#include <string>

#include "base/logging.h"
#include "base/metrics/histogram.h"
#include "base/metrics/stats_counters.h"
#include "base/stringprintf.h"
#include "chrome/browser/browser_thread.h"
#include "chrome/browser/download/download_item.h"
#include "chrome/browser/download/download_file_manager.h"
#include "chrome/browser/history/download_create_info.h"
#include "chrome/browser/renderer_host/global_request_id.h"
#include "chrome/browser/renderer_host/resource_dispatcher_host.h"
#include "chrome/browser/renderer_host/resource_dispatcher_host_request_info.h"
#include "chrome/common/resource_response.h"
#include "net/base/io_buffer.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request_context.h"

DownloadResourceHandler::DownloadResourceHandler(
    ResourceDispatcherHost* rdh,
    int render_process_host_id,
    int render_view_id,
    int request_id,
    const GURL& url,
    DownloadFileManager* download_file_manager,
    net::URLRequest* request,
    bool save_as,
    const DownloadSaveInfo& save_info)
    : download_id_(-1),
      global_id_(render_process_host_id, request_id),
      render_view_id_(render_view_id),
      url_(url),
      original_url_(url),
      content_length_(0),
      download_file_manager_(download_file_manager),
      request_(request),
      save_as_(save_as),
      save_info_(save_info),
      buffer_(new DownloadBuffer),
      rdh_(rdh),
      is_paused_(false),
      url_check_pending_(false) {
}

bool DownloadResourceHandler::OnUploadProgress(int request_id,
                                               uint64 position,
                                               uint64 size) {
  return true;
}

// Not needed, as this event handler ought to be the final resource.
bool DownloadResourceHandler::OnRequestRedirected(int request_id,
                                                  const GURL& url,
                                                  ResourceResponse* response,
                                                  bool* defer) {
  url_ = url;
  return true;
}

// Callback when the result of checking a download URL is known.
// TODO(lzheng): We should create a information bar with buttons to ask
// if users want to proceed when the download is malicious.
void DownloadResourceHandler::OnDownloadUrlCheckResult(
    const GURL& url, SafeBrowsingService::UrlCheckResult result) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(url_check_pending_);

  UMA_HISTOGRAM_TIMES("SB2.DownloadUrlCheckDuration",
                      base::TimeTicks::Now() - download_start_time_);

  if (result == SafeBrowsingService::BINARY_MALWARE) {
    // TODO(lzheng): More UI work to show warnings properly on download shelf.
    DLOG(WARNING) << "This url leads to a malware downloading: "
                  << url.spec();
    UpdateDownloadUrlCheckStats(DOWNLOAD_URL_CHECKS_MALWARE);
  }

  url_check_pending_ = false;
  // Note: Release() should be the last line in this call. It is for
  // the AddRef in CheckSafeBrowsing.
  Release();
}

// Send the download creation information to the download thread.
void DownloadResourceHandler::StartDownloadUrlCheck() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  AddRef();
  if (!rdh_->safe_browsing_service()->CheckDownloadUrl(url_, this)) {
    url_check_pending_ = true;
    UpdateDownloadUrlCheckStats(DOWNLOAD_URL_CHECKS_TOTAL);
    // Note: in this case, the AddRef will be balanced in
    // "OnDownloadUrlCheckResult" or "OnRequestClosed".
  } else {
    // Immediately release the AddRef() at the beginning of this function
    // since no more callbacks will happen.
    Release();
    DVLOG(1) << "url: " << url_.spec() << " is safe to download.";
  }
}

// Send the download creation information to the download thread.
bool DownloadResourceHandler::OnResponseStarted(int request_id,
                                                ResourceResponse* response) {
  VLOG(20) << __FUNCTION__ << "()" << DebugString()
           << " request_id = " << request_id;
  DCHECK(!url_check_pending_);
  download_start_time_ = base::TimeTicks::Now();
  StartDownloadUrlCheck();
  std::string content_disposition;
  request_->GetResponseHeaderByName("content-disposition",
                                    &content_disposition);
  set_content_disposition(content_disposition);
  set_content_length(response->response_head.content_length);

  const ResourceDispatcherHostRequestInfo* request_info =
    ResourceDispatcherHost::InfoForRequest(request_);

  download_id_ = download_file_manager_->GetNextId();
  // |download_file_manager_| consumes (deletes):
  DownloadCreateInfo* info = new DownloadCreateInfo;
  info->url = url_;
  info->original_url = original_url_;
  info->referrer_url = GURL(request_->referrer());
  info->start_time = base::Time::Now();
  info->received_bytes = 0;
  info->total_bytes = content_length_;
  info->state = DownloadItem::IN_PROGRESS;
  info->download_id = download_id_;
  info->has_user_gesture = request_info->has_user_gesture();
  info->child_id = global_id_.child_id;
  info->render_view_id = render_view_id_;
  info->request_id = global_id_.request_id;
  info->content_disposition = content_disposition_;
  info->mime_type = response->response_head.mime_type;

  std::string content_type_header;
  if (!response->response_head.headers ||
      !response->response_head.headers->GetMimeType(&content_type_header))
    content_type_header = "";
  info->original_mime_type = content_type_header;

  info->prompt_user_for_save_location =
      save_as_ && save_info_.file_path.empty();
  info->is_dangerous = false;
  info->referrer_charset = request_->context()->referrer_charset();
  info->save_info = save_info_;
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      NewRunnableMethod(
          download_file_manager_, &DownloadFileManager::StartDownload, info));

  // We can't start saving the data before we create the file on disk.
  // The request will be un-paused in DownloadFileManager::CreateDownloadFile.
  rdh_->PauseRequest(global_id_.child_id, global_id_.request_id, true);

  return true;
}

bool DownloadResourceHandler::OnWillStart(int request_id,
                                          const GURL& url,
                                          bool* defer) {
  return true;
}

// Create a new buffer, which will be handed to the download thread for file
// writing and deletion.
bool DownloadResourceHandler::OnWillRead(int request_id, net::IOBuffer** buf,
                                         int* buf_size, int min_size) {
  DCHECK(buf && buf_size);
  if (!read_buffer_) {
    *buf_size = min_size < 0 ? kReadBufSize : min_size;
    read_buffer_ = new net::IOBuffer(*buf_size);
  }
  *buf = read_buffer_.get();
  return true;
}

// Pass the buffer to the download file writer.
bool DownloadResourceHandler::OnReadCompleted(int request_id, int* bytes_read) {
  if (!*bytes_read)
    return true;
  DCHECK(read_buffer_);
  base::AutoLock auto_lock(buffer_->lock);
  bool need_update = buffer_->contents.empty();

  // We are passing ownership of this buffer to the download file manager.
  net::IOBuffer* buffer = NULL;
  read_buffer_.swap(&buffer);
  buffer_->contents.push_back(std::make_pair(buffer, *bytes_read));
  if (need_update) {
    BrowserThread::PostTask(
        BrowserThread::FILE, FROM_HERE,
        NewRunnableMethod(download_file_manager_,
                          &DownloadFileManager::UpdateDownload,
                          download_id_,
                          buffer_));
  }

  // We schedule a pause outside of the read loop if there is too much file
  // writing work to do.
  if (buffer_->contents.size() > kLoadsToWrite)
    StartPauseTimer();

  return true;
}

bool DownloadResourceHandler::OnResponseCompleted(
    int request_id,
    const net::URLRequestStatus& status,
    const std::string& security_info) {
  VLOG(20) << __FUNCTION__ << "()" << DebugString()
           << " request_id = " << request_id
           << " status.status() = " << status.status()
           << " status.os_error() = " << status.os_error();
  BrowserThread::PostTask(
      BrowserThread::FILE, FROM_HERE,
      NewRunnableMethod(download_file_manager_,
                        &DownloadFileManager::OnResponseCompleted,
                        download_id_,
                        buffer_));
  read_buffer_ = NULL;

  // 'buffer_' is deleted by the DownloadFileManager.
  buffer_ = NULL;
  return true;
}

void DownloadResourceHandler::OnRequestClosed() {
  UMA_HISTOGRAM_TIMES("SB2.DownloadDuration",
                      base::TimeTicks::Now() - download_start_time_);
  if (url_check_pending_) {
    DVLOG(1) << "Cancel pending download url checking request: " << this;
    rdh_->safe_browsing_service()->CancelCheck(this);
    UpdateDownloadUrlCheckStats(DOWNLOAD_URL_CHECKS_CANCELED);
    url_check_pending_ = false;
    // Balance the AddRef() from StartDownloadUrlCheck() which would usually be
    // balanced by OnDownloadUrlCheckResult().
    Release();
  }
}

// If the content-length header is not present (or contains something other
// than numbers), the incoming content_length is -1 (unknown size).
// Set the content length to 0 to indicate unknown size to DownloadManager.
void DownloadResourceHandler::set_content_length(const int64& content_length) {
  content_length_ = 0;
  if (content_length > 0)
    content_length_ = content_length;
}

void DownloadResourceHandler::set_content_disposition(
    const std::string& content_disposition) {
  content_disposition_ = content_disposition;
}

void DownloadResourceHandler::CheckWriteProgress() {
  if (!buffer_)
    return;  // The download completed while we were waiting to run.

  size_t contents_size;
  {
    base::AutoLock lock(buffer_->lock);
    contents_size = buffer_->contents.size();
  }

  bool should_pause = contents_size > kLoadsToWrite;

  // We'll come back later and see if it's okay to unpause the request.
  if (should_pause)
    StartPauseTimer();

  if (is_paused_ != should_pause) {
    rdh_->PauseRequest(global_id_.child_id, global_id_.request_id,
                       should_pause);
    is_paused_ = should_pause;
  }
}

DownloadResourceHandler::~DownloadResourceHandler() {
}

void DownloadResourceHandler::StartPauseTimer() {
  if (!pause_timer_.IsRunning())
    pause_timer_.Start(base::TimeDelta::FromMilliseconds(kThrottleTimeMs), this,
                       &DownloadResourceHandler::CheckWriteProgress);
}

std::string DownloadResourceHandler::DebugString() const {
  return base::StringPrintf("{"
                            " url_ = " "\"%s\""
                            " download_id_ = " "%d"
                            " global_id_ = {"
                            " child_id = " "%d"
                            " request_id = " "%d"
                            " }"
                            " render_view_id_ = " "%d"
                            " save_info_.file_path = \"%" PRFilePath "\""
                            " }",
                            url_.spec().c_str(),
                            download_id_,
                            global_id_.child_id,
                            global_id_.request_id,
                            render_view_id_,
                            save_info_.file_path.value().c_str());
}

void DownloadResourceHandler::UpdateDownloadUrlCheckStats(
    SBStatsType stat_type) {
  UMA_HISTOGRAM_ENUMERATION("SB2.DownloadUrlChecks",
                            stat_type,
                            DOWNLOAD_URL_CHECKS_MAX);
}
