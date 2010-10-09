// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/download/download_file.h"

#include "base/file_util.h"
#include "chrome/browser/chrome_thread.h"
#include "chrome/browser/download/download_manager.h"
#include "chrome/browser/download/download_util.h"
#include "chrome/browser/history/download_create_info.h"

DownloadFile::DownloadFile(const DownloadCreateInfo* info,
                           DownloadManager* download_manager)
    : BaseFile(info->save_info.file_path,
               info->url,
               info->referrer_url,
               info->received_bytes,
               info->save_info.file_stream),
      id_(info->download_id),
      child_id_(info->child_id),
      request_id_(info->request_id),
      download_manager_(download_manager) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));

}

DownloadFile::~DownloadFile() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
}

void DownloadFile::DeleteCrDownload() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  FilePath crdownload = download_util::GetCrDownloadPath(full_path_);
  file_util::Delete(crdownload, false);
}

void DownloadFile::CancelDownloadRequest(ResourceDispatcherHost* rdh) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      NewRunnableFunction(&download_util::CancelDownloadRequest,
                          rdh,
                          child_id_,
                          request_id_));
}

DownloadManager* DownloadFile::GetDownloadManager() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  return download_manager_.get();
}
