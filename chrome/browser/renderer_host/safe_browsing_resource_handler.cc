// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_host/safe_browsing_resource_handler.h"

#include "base/logging.h"
#include "chrome/browser/renderer_host/resource_dispatcher_host.h"
#include "chrome/browser/renderer_host/resource_message_filter.h"
#include "chrome/common/notification_service.h"
#include "net/base/net_errors.h"
#include "net/base/io_buffer.h"

// Maximum time to wait for a gethash response from the Safe Browsing servers.
static const int kMaxGetHashMs = 1000;

SafeBrowsingResourceHandler::SafeBrowsingResourceHandler(
    ResourceHandler* handler,
    int render_process_host_id,
    int render_view_id,
    const GURL& url,
    ResourceType::Type resource_type,
    SafeBrowsingService* safe_browsing,
    ResourceDispatcherHost* resource_dispatcher_host,
    ResourceDispatcherHost::Receiver* receiver)
    : next_handler_(handler),
      render_process_host_id_(render_process_host_id),
      render_view_id_(render_view_id),
      paused_request_id_(-1),
      in_safe_browsing_check_(false),
      displaying_blocking_page_(false),
      safe_browsing_(safe_browsing),
      queued_error_request_id_(-1),
      rdh_(resource_dispatcher_host),
      resource_type_(resource_type),
      redirect_id_(-1) {
  if (safe_browsing_->CheckUrl(url, this)) {
    safe_browsing_result_ = SafeBrowsingService::URL_SAFE;
    safe_browsing_->LogPauseDelay(base::TimeDelta());  // No delay.
  } else {
    AddRef();
    in_safe_browsing_check_ = true;
    // Can't pause now because it's too early, so we'll do it in OnWillRead.
  }

  registrar_.Add(this, NotificationType::RESOURCE_MESSAGE_FILTER_SHUTDOWN,
                 Source<ResourceMessageFilter>(
                     static_cast<ResourceMessageFilter*>(receiver)));
}

SafeBrowsingResourceHandler::~SafeBrowsingResourceHandler() {
}

bool SafeBrowsingResourceHandler::OnUploadProgress(int request_id,
                                                   uint64 position,
                                                   uint64 size) {
  return next_handler_->OnUploadProgress(request_id, position, size);
}

bool SafeBrowsingResourceHandler::OnRequestRedirected(
    int request_id,
    const GURL& new_url,
    ResourceResponse* response,
    bool* defer) {
  if (in_safe_browsing_check_) {
    // Defer following the redirect until the SafeBrowsing check is complete.
    // Store the redirect context so we can pass it on to other handlers once we
    // have completed our check.
    redirect_response_ = response;
    redirect_url_ = new_url;
    redirect_id_ = request_id;
    *defer = true;
    return true;
  }

  if (safe_browsing_->CheckUrl(new_url, this)) {
    safe_browsing_result_ = SafeBrowsingService::URL_SAFE;
    safe_browsing_->LogPauseDelay(base::TimeDelta());  // No delay.
  } else {
    AddRef();
    in_safe_browsing_check_ = true;
    // Can't pause now because it's too early, so we'll do it in OnWillRead.
  }

  return next_handler_->OnRequestRedirected(
      request_id, new_url, response, defer);
}

bool SafeBrowsingResourceHandler::OnResponseStarted(
    int request_id, ResourceResponse* response) {
  return next_handler_->OnResponseStarted(request_id, response);
}

void SafeBrowsingResourceHandler::OnGetHashTimeout() {
  if (!in_safe_browsing_check_)
    return;

  safe_browsing_->CancelCheck(this);
  OnUrlCheckResult(GURL::EmptyGURL(), SafeBrowsingService::URL_SAFE);
}

bool SafeBrowsingResourceHandler::OnWillRead(int request_id,
                                             net::IOBuffer** buf, int* buf_size,
                                             int min_size) {
  if (in_safe_browsing_check_ && pause_time_.is_null()) {
    pause_time_ = base::Time::Now();
    MessageLoop::current()->PostDelayedTask(
        FROM_HERE,
        NewRunnableMethod(this, &SafeBrowsingResourceHandler::OnGetHashTimeout),
        kMaxGetHashMs);
  }

  if (in_safe_browsing_check_ || displaying_blocking_page_) {
    rdh_->PauseRequest(render_process_host_id_, request_id, true);
    paused_request_id_ = request_id;
  }

  bool rv = next_handler_->OnWillRead(request_id, buf, buf_size, min_size);
  // TODO(willchan): Remove after debugging bug 16371.
  if (rv)
    CHECK((*buf)->data());
  return rv;
}

bool SafeBrowsingResourceHandler::OnReadCompleted(int request_id,
                                                  int* bytes_read) {
  return next_handler_->OnReadCompleted(request_id, bytes_read);
}

bool SafeBrowsingResourceHandler::OnResponseCompleted(
    int request_id, const URLRequestStatus& status,
    const std::string& security_info) {
  if ((in_safe_browsing_check_ ||
       safe_browsing_result_ != SafeBrowsingService::URL_SAFE) &&
      status.status() == URLRequestStatus::FAILED &&
      status.os_error() == net::ERR_NAME_NOT_RESOLVED) {
    // Got a DNS error while the safebrowsing check is in progress or we
    // already know that the site is unsafe.  Don't show the the dns error
    // page.
    queued_error_.reset(new URLRequestStatus(status));
    queued_error_request_id_ = request_id;
    queued_security_info_ = security_info;
    return true;
  }

  return next_handler_->OnResponseCompleted(request_id, status, security_info);
}

// SafeBrowsingService::Client implementation, called on the IO thread once
// the URL has been classified.
void SafeBrowsingResourceHandler::OnUrlCheckResult(
    const GURL& url, SafeBrowsingService::UrlCheckResult result) {
  DCHECK(in_safe_browsing_check_);
  DCHECK(!displaying_blocking_page_);

  safe_browsing_result_ = result;
  in_safe_browsing_check_ = false;

  if (result == SafeBrowsingService::URL_SAFE) {
    // Resume following any redirect response we've deferred.
    if (redirect_id_ != -1) {
      ResumeRedirect();
      return;
    }

    if (paused_request_id_ != -1) {
      rdh_->PauseRequest(render_process_host_id_, paused_request_id_, false);
      paused_request_id_ = -1;
    }

    base::TimeDelta pause_delta;
    if (!pause_time_.is_null())
      pause_delta = base::Time::Now() - pause_time_;
    safe_browsing_->LogPauseDelay(pause_delta);

    if (queued_error_.get()) {
      next_handler_->OnResponseCompleted(
          queued_error_request_id_, *queued_error_.get(),
          queued_security_info_);
      queued_error_.reset();
      queued_security_info_.clear();
    }

    Release();
  } else {
    displaying_blocking_page_ = true;
    safe_browsing_->DisplayBlockingPage(
        url, resource_type_, result, this, render_process_host_id_,
        render_view_id_);
  }
}

// SafeBrowsingService::Client implementation, called on the IO thread when
// the user has decided to proceed with the current request, or go back.
void SafeBrowsingResourceHandler::OnBlockingPageComplete(bool proceed) {
  DCHECK(displaying_blocking_page_);
  displaying_blocking_page_ = false;

  if (proceed) {
    // Resume following any deferred redirect.
    if (redirect_id_ != -1) {
      ResumeRedirect();
      return;
    }

    safe_browsing_result_ = SafeBrowsingService::URL_SAFE;
    if (paused_request_id_ != -1) {
      rdh_->PauseRequest(render_process_host_id_, paused_request_id_, false);
      paused_request_id_ = -1;
    }

    if (queued_error_.get()) {
      next_handler_->OnResponseCompleted(
          queued_error_request_id_, *queued_error_.get(),
          queued_security_info_);
      queued_error_.reset();
      queued_security_info_.clear();
    }
  } else {
    rdh_->CancelRequest(render_process_host_id_, paused_request_id_, false);
  }

  Release();
}

void SafeBrowsingResourceHandler::Observe(NotificationType type,
                                          const NotificationSource& source,
                                          const NotificationDetails& details) {
  DCHECK(type.value == NotificationType::RESOURCE_MESSAGE_FILTER_SHUTDOWN);
  if (in_safe_browsing_check_) {
    safe_browsing_->CancelCheck(this);
    in_safe_browsing_check_ = false;
    Release();
  }
}

void SafeBrowsingResourceHandler::ResumeRedirect() {
  DCHECK(redirect_id_ != -1);

  // Give the other resource handlers a chance to handle the redirect.
  bool defer = false;
  next_handler_->OnRequestRedirected(redirect_id_, redirect_url_,
                                     redirect_response_, &defer);
  if (!defer) {
    // TODO(wtc): should we pass a new first party for cookies URL?
    rdh_->FollowDeferredRedirect(render_process_host_id_, redirect_id_,
                                 false, GURL());
  }

  redirect_response_ = NULL;
  redirect_id_ = -1;
  Release();
}
