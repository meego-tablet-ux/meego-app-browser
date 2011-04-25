// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_HISTORY_RECENT_AND_BOOKMARK_THUMBNAILS_BACKEND_QT_H_
#define CHROME_BROWSER_HISTORY_RECENT_AND_BOOKMARK_THUMBNAILS_BACKEND_QT_H_
#pragma once

#include "base/file_path.h"
#include "base/ref_counted.h"
#include "base/scoped_ptr.h"
#include "chrome/browser/history/history_types.h"
#include "content/browser/cancelable_request.h"
#include "chrome/browser/history/history_marshaling.h"

class FilePath;
class GURL;
class SkBitmap;

namespace history {

class ThumbnailDatabaseQt;

// Service used by TopSites to have db interaction happen on the DB thread.  All
// public methods are invoked on the ui thread and get funneled to the DB
// thread.
class RecentAndBookmarkThumbnailsBackendQt
    :   public base::RefCountedThreadSafe<RecentAndBookmarkThumbnailsBackendQt>, 
	public CancelableRequestProvider {
 public:
  RecentAndBookmarkThumbnailsBackendQt();

  ~RecentAndBookmarkThumbnailsBackendQt();

  void Init(const FilePath& path);

  // Schedules the db to be shutdown.
  void Shutdown();

  // Sets the thumbnail.
  void SetPageThumbnail(const GURL& url,
                        const SkBitmap& thumbnail);

  void GetPageThumbnail(scoped_refptr<GetPageThumbnailRequest> request,
                        const GURL& page_url);

  void SetBookmarkedPage(const GURL& url, bool bookmarked);
 
  // 
  void CleanUnusedThumbnails(std::vector<GURL> list_url);

  // Deletes the database and recreates it.
  void ResetDatabase();

 private:
  void GetPageThumbnailDirectly(const GURL& page_url, scoped_refptr<RefCountedBytes>* data);

  FilePath db_path_;

  scoped_ptr<ThumbnailDatabaseQt> thumbnail_db_;

  DISALLOW_COPY_AND_ASSIGN(RecentAndBookmarkThumbnailsBackendQt);
};

}  // namespace history


#endif  // CHROME_BROWSER_HISTORY_RECENT_AND_BOOKMARK_THUMBNAILS_BACKEND_QT_H_
