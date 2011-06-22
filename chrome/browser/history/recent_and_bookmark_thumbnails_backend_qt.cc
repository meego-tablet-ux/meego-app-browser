// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/file_util.h"
#include "base/message_loop.h"
#include "base/metrics/histogram.h"
#include "base/memory/scoped_vector.h"
#include "base/string_util.h"
#include "base/time.h"
#include "chrome/browser/history/recent_and_bookmark_thumbnails_backend_qt.h"
#include "chrome/browser/history/thumbnail_database_qt.h"
#include "content/browser/browser_thread.h"
#include "chrome/browser/history/history_marshaling.h"

using base::TimeTicks;

namespace history {

RecentAndBookmarkThumbnailsBackendQt::RecentAndBookmarkThumbnailsBackendQt() {

}

RecentAndBookmarkThumbnailsBackendQt::~RecentAndBookmarkThumbnailsBackendQt() {
  DLOG(INFO)<<__FUNCTION__;
  if (thumbnail_db_.get()) {
    thumbnail_db_->CommitTransaction();
    thumbnail_db_.reset();
  }
}
void RecentAndBookmarkThumbnailsBackendQt::Init(const FilePath& path) {

  DLOG(INFO)<<__FUNCTION__;
  thumbnail_db_.reset(new ThumbnailDatabaseQt());

  db_path_ = path;

  if (!thumbnail_db_->Init(path)) {
    LOG(WARNING) << "Could not initialize the thumbnail database.";
    assert(false);
    thumbnail_db_.reset();
  }

  if (thumbnail_db_.get()) {
    thumbnail_db_->BeginTransaction();
  }
}

void RecentAndBookmarkThumbnailsBackendQt::Shutdown() {
  DLOG(INFO)<<__FUNCTION__;
}

void RecentAndBookmarkThumbnailsBackendQt::CleanUnusedThumbnails(std::vector<GURL> list_url) {
  DLOG(INFO)<<__FUNCTION__;
  //dump
  DLOG(INFO)<<"dump:";
  int count = list_url.size();
  for(int i=0; i<count; i++) {
    DLOG(INFO)<<list_url[i].spec();
  }
  
  int saved_count = thumbnail_db_->ThumbnailsCountExcludeBookmarked();
  DLOG(INFO)<<"num in DB: " << saved_count;
  if(saved_count < kRecThumbnailMaxNum)
    return;
  
  thumbnail_db_->CleanUnusedThumbnails(list_url);
}

void RecentAndBookmarkThumbnailsBackendQt::SetPageThumbnail(const GURL& url,
                        				const SkBitmap& thumbnail) {
  DLOG(INFO)<<__FUNCTION__;
  if (!thumbnail_db_.get())
    return;

  //if(thumbnail_db_->IsThumbnailValid(url))
  //  return;

  if(thumbnail_db_->HasThisPage(url)) {
    thumbnail_db_->SetPageThumbnail(url, thumbnail);
  }else{
    bool default_bookmarked_state = false;
    thumbnail_db_->InsertNewRow(url, default_bookmarked_state, thumbnail);
  }
}

void RecentAndBookmarkThumbnailsBackendQt::SetBookmarkedPage(
						const GURL& url, 
						bool bookmarked) {
  DLOG(INFO)<<__FUNCTION__;
  if (!thumbnail_db_.get())
    return;
  if(thumbnail_db_->HasThisPage(url)) {
    thumbnail_db_->UpdateBookmarkedColumn(url, bookmarked);
  }else{
    if(bookmarked)
    	thumbnail_db_->InsertNewRow(url, bookmarked);
  }

}

void RecentAndBookmarkThumbnailsBackendQt::GetPageThumbnail(
    scoped_refptr<history::GetPageThumbnailRequest> request,
    const GURL& page_url) {
  DLOG(INFO)<<__FUNCTION__;
  if (request->canceled())
    return;

  scoped_refptr<RefCountedBytes> data;
  GetPageThumbnailDirectly(page_url, &data);

  request->ForwardResult(GetPageThumbnailRequest::TupleType(
      request->handle(), data));
}

void RecentAndBookmarkThumbnailsBackendQt::GetPageThumbnailDirectly(
    const GURL& page_url,
    scoped_refptr<RefCountedBytes>* data) {
  DLOG(INFO)<<__FUNCTION__;
  if (thumbnail_db_.get()) {
    *data = new RefCountedBytes;

    // Time the result.
    TimeTicks beginning_time = TimeTicks::Now();

    bool success = thumbnail_db_->GetPageThumbnail(page_url, &(*data)->data);

    if (!success)
      *data = NULL;  // This will tell the callback there was an error.

    UMA_HISTOGRAM_TIMES("History.GetPageThumbnail",
                        TimeTicks::Now() - beginning_time);
  }
}


void RecentAndBookmarkThumbnailsBackendQt::ResetDatabase() {
  DLOG(INFO)<<__FUNCTION__;

}

}  // namespace history
