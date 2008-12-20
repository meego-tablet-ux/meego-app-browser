// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/renderer_host/save_file_resource_handler.h"

#include "chrome/browser/download/save_file_manager.h"

SaveFileResourceHandler::SaveFileResourceHandler(int render_process_host_id,
                                                 int render_view_id,
                                                 const std::string& url,
                                                 SaveFileManager* manager)
    : save_id_(-1),
      render_process_id_(render_process_host_id),
      render_view_id_(render_view_id),
      read_buffer_(NULL),
      url_(UTF8ToWide(url)),
      content_length_(0),
      save_manager_(manager) {
}

bool SaveFileResourceHandler::OnRequestRedirected(int request_id,
                                                  const GURL& url) {
  final_url_ = UTF8ToWide(url.spec());
  return true;
}

bool SaveFileResourceHandler::OnResponseStarted(int request_id,
                                                ResourceResponse* response) {
  save_id_ = save_manager_->GetNextId();
  // |save_manager_| consumes (deletes):
  SaveFileCreateInfo* info = new SaveFileCreateInfo;
  info->url = url_;
  info->final_url = final_url_;
  info->total_bytes = content_length_;
  info->save_id = save_id_;
  info->render_process_id = render_process_id_;
  info->render_view_id = render_view_id_;
  info->request_id = request_id;
  info->content_disposition = content_disposition_;
  info->save_source = SaveFileCreateInfo::SAVE_FILE_FROM_NET;
  save_manager_->GetSaveLoop()->PostTask(FROM_HERE,
      NewRunnableMethod(save_manager_,
                        &SaveFileManager::StartSave,
                        info));
  return true;
}

bool SaveFileResourceHandler::OnWillRead(int request_id,
                                         char** buf, int* buf_size,
                                         int min_size) {
  DCHECK(buf && buf_size);
  if (!read_buffer_) {
    *buf_size = min_size < 0 ? kReadBufSize : min_size;
    read_buffer_ = new char[*buf_size];
  }
  *buf = read_buffer_;
  return true;
}

bool SaveFileResourceHandler::OnReadCompleted(int request_id, int* bytes_read) {
  DCHECK(read_buffer_);
  save_manager_->GetSaveLoop()->PostTask(FROM_HERE,
      NewRunnableMethod(save_manager_,
                        &SaveFileManager::UpdateSaveProgress,
                        save_id_,
                        read_buffer_,
                        *bytes_read));
  read_buffer_ = NULL;
  return true;
}

bool SaveFileResourceHandler::OnResponseCompleted(
    int request_id,
    const URLRequestStatus& status) {
  save_manager_->GetSaveLoop()->PostTask(FROM_HERE,
      NewRunnableMethod(save_manager_,
                        &SaveFileManager::SaveFinished,
                        save_id_,
                        url_,
                        render_process_id_,
                        status.is_success() && !status.is_io_pending()));
  delete [] read_buffer_;
  return true;
}
