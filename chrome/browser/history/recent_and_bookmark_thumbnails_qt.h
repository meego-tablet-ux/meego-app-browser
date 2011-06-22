// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_HISTORY_RECENT_AND_BOOKMARK_THUMBNAILS_QT_H_
#define CHROME_BROWSER_HISTORY_RECENT_AND_BOOKMARK_THUMBNAILS_QT_H_
#pragma once

#include <list>
#include <set>
#include <string>
#include <utility>

#include "base/basictypes.h"
#include "base/gtest_prod_util.h"
#include "base/memory/ref_counted.h"
#include "base/memory/ref_counted_memory.h"
#include "base/synchronization/lock.h"
#include "base/time.h"
#include "base/timer.h"
#include "content/browser/cancelable_request.h"
#include "googleurl/src/gurl.h"
#include "chrome/browser/sessions/tab_restore_service.h"
#include "chrome/browser/sessions/tab_restore_service_observer.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "chrome/browser/bookmarks/bookmark_model_observer.h"

class FilePath;
class SkBitmap;
class TabRestoreService;

namespace base {
class Thread;
}

namespace history {

class RecentAndBookmarkThumbnailsBackendQt;

// Stores the thumbnail data for recently visited and bookmark sites.
//
// All methods must be invoked on the UI thread. All mutations
// to internal state happen on the UI thread and are scheduled to 
// interaction with db.
//
// Because chromium use TopSites as the interface to ThumbnailGenerator
// So we put the instance of this class inside TopSites for convenience.
class RecentAndBookmarkThumbnailsQt
      : public CancelableRequestProvider,
	public BookmarkModelObserver,
        public TabRestoreServiceObserver  {
 public:
  RecentAndBookmarkThumbnailsQt(Profile* profile);

  ~RecentAndBookmarkThumbnailsQt();

  bool Init(const FilePath& path);

  // Save the thumbnail into database.
  void SetRecentPageThumbnail(const GURL& url,
                        const SkBitmap& thumbnail);

  typedef Callback2<Handle, scoped_refptr<RefCountedBytes> >::Type
      ThumbnailDataCallback;

  // Requests a page thumbnail. See ThumbnailDataCallback definition above.
  Handle GetRecentPageThumbnail(const GURL& page_url,
                          CancelableRequestConsumerBase* consumer,
                          ThumbnailDataCallback* callback);

  // Update bookmark column in database. Or add a new row in database if the
  // item doesn't exist. Will set the thumbnail data when page load complete.
  void SetBookmarkPage(const GURL& url, bool bookmarked);

  // Overide BookmarkModelObserver.
  // Update the bookmarked flag in database for bookmarked page.
  virtual void BookmarkNodeAdded(BookmarkModel* model,
                                 const BookmarkNode* parent,
                                 int index);
  virtual void BookmarkNodeRemoved(BookmarkModel* model,
                                   const BookmarkNode* parent,
                                   int old_index,
                                   const BookmarkNode* node);
  virtual void BookmarkModelBeingDeleted(BookmarkModel* model) {};
  virtual void BookmarkNodeMoved(BookmarkModel* model,
                                 const BookmarkNode* old_parent,
                                 int old_index,
                                 const BookmarkNode* new_parent,
                                 int new_index) {};
  virtual void BookmarkNodeChanged(BookmarkModel* model,
                                   const BookmarkNode* node) {};
  virtual void BookmarkNodeChildrenReordered(BookmarkModel* model,
                                             const BookmarkNode* node) {};
  virtual void BookmarkNodeFaviconLoaded(BookmarkModel* model,
                                         const BookmarkNode* node) {};
  virtual void Loaded(BookmarkModel* model) {};

 protected:
  enum SchedulePriority {
    PRIORITY_UI,      // The highest priority (must respond to UI events).
    PRIORITY_NORMAL,  // Normal stuff like adding a page.
    PRIORITY_LOW,     // Low priority things like indexing or expiration.
  };
 private:
  void UnloadBackend();

  void Cleanup();

  // Register restore service for clear Recent Thumbnails database.
  void RegisterGetRecentlyClosedTab();
  
  void UnregisterGetRecentlyClosedTab();

  virtual void TabRestoreServiceChanged(TabRestoreService* service); 

  virtual void TabRestoreServiceDestroyed(TabRestoreService* service) {}

  bool TabToValue(const TabRestoreService::Tab& tab, GURL* value);

  bool EnsureTabIsUnique(const GURL* value, std::set<string16>* unique_items);

  void CleanUnusedThumbnails(TabRestoreService* service);

  // Initializes the backend.
  void LoadBackendIfNecessary();

  // Call to schedule a given task for running on the history thread with the
  // specified priority. The task will have ownership taken.
  void ScheduleTask(SchedulePriority priority, Task* task);

  // Schedule ------------------------------------------------------------------
  //
  // Functions for scheduling operations on the history thread that have a
  // handle and may be cancelable. For fire-and-forget operations, see
  // ScheduleAndForget below.

  template<typename BackendFunc, class RequestType>
  Handle Schedule(SchedulePriority priority,
                  BackendFunc func,  // Function to call on the HistoryBackend.
                  CancelableRequestConsumerBase* consumer,
                  RequestType* request) {
    DCHECK(thread_) << "History service being called after cleanup";
    LoadBackendIfNecessary();
    if (consumer)
      AddRequest(request, consumer);
    ScheduleTask(priority,
                 NewRunnableMethod(backend_.get(), func,
                                   scoped_refptr<RequestType>(request)));
    return request->handle();
  }

  template<typename BackendFunc, class RequestType, typename ArgA>
  Handle Schedule(SchedulePriority priority,
                  BackendFunc func,  // Function to call on the HistoryBackend.
                  CancelableRequestConsumerBase* consumer,
                  RequestType* request,
                  const ArgA& a) {
    DCHECK(thread_) << "History service being called after cleanup";
    LoadBackendIfNecessary();
    if (consumer)
      AddRequest(request, consumer);
    ScheduleTask(priority,
                 NewRunnableMethod(backend_.get(), func,
                                   scoped_refptr<RequestType>(request),
                                   a));
    return request->handle();
  }

  template<typename BackendFunc,
           class RequestType,  // Descendant of CancelableRequstBase.
           typename ArgA,
           typename ArgB>
  Handle Schedule(SchedulePriority priority,
                  BackendFunc func,  // Function to call on the HistoryBackend.
                  CancelableRequestConsumerBase* consumer,
                  RequestType* request,
                  const ArgA& a,
                  const ArgB& b) {
    DCHECK(thread_) << "History service being called after cleanup";
    LoadBackendIfNecessary();
    if (consumer)
      AddRequest(request, consumer);
    ScheduleTask(priority,
                 NewRunnableMethod(backend_.get(), func,
                                   scoped_refptr<RequestType>(request),
                                   a, b));
    return request->handle();
  }

  template<typename BackendFunc,
           class RequestType,  // Descendant of CancelableRequstBase.
           typename ArgA,
           typename ArgB,
           typename ArgC>
  Handle Schedule(SchedulePriority priority,
                  BackendFunc func,  // Function to call on the HistoryBackend.
                  CancelableRequestConsumerBase* consumer,
                  RequestType* request,
                  const ArgA& a,
                  const ArgB& b,
                  const ArgC& c) {
    DCHECK(thread_) << "History service being called after cleanup";
    LoadBackendIfNecessary();
    if (consumer)
      AddRequest(request, consumer);
    ScheduleTask(priority,
                 NewRunnableMethod(backend_.get(), func,
                                   scoped_refptr<RequestType>(request),
                                   a, b, c));
    return request->handle();
  }


  // ScheduleAndForget ---------------------------------------------------------
  //
  // Functions for scheduling operations on the history thread that do not need
  // any callbacks and are not cancelable.

  template<typename BackendFunc>
  void ScheduleAndForget(SchedulePriority priority,
                         BackendFunc func) {  // Function to call on backend.
    DCHECK(thread_) << "History service being called after cleanup";
    LoadBackendIfNecessary();
    ScheduleTask(priority, NewRunnableMethod(backend_.get(), func));
  }

  template<typename BackendFunc, typename ArgA>
  void ScheduleAndForget(SchedulePriority priority,
                         BackendFunc func,  // Function to call on backend.
                         const ArgA& a) {
    DCHECK(thread_) << "History service being called after cleanup";
    LoadBackendIfNecessary();
    ScheduleTask(priority, NewRunnableMethod(backend_.get(), func, a));
  }

  template<typename BackendFunc, typename ArgA, typename ArgB>
  void ScheduleAndForget(SchedulePriority priority,
                         BackendFunc func,  // Function to call on backend.
                         const ArgA& a,
                         const ArgB& b) {
    DCHECK(thread_) << "History service being called after cleanup";
    LoadBackendIfNecessary();
    ScheduleTask(priority, NewRunnableMethod(backend_.get(), func,
                                             a, b));
  }

  template<typename BackendFunc, typename ArgA, typename ArgB, typename ArgC>
  void ScheduleAndForget(SchedulePriority priority,
                         BackendFunc func,  // Function to call on backend.
                         const ArgA& a,
                         const ArgB& b,
                         const ArgC& c) {
    DCHECK(thread_) << "History service being called after cleanup";
    LoadBackendIfNecessary();
    ScheduleTask(priority, NewRunnableMethod(backend_.get(), func,
                                             a, b, c));
  }

  template<typename BackendFunc,
           typename ArgA,
           typename ArgB,
           typename ArgC,
           typename ArgD>
  void ScheduleAndForget(SchedulePriority priority,
                         BackendFunc func,  // Function to call on backend.
                         const ArgA& a,
                         const ArgB& b,
                         const ArgC& c,
                         const ArgD& d) {
    DCHECK(thread_) << "History service being called after cleanup";
    LoadBackendIfNecessary();
    ScheduleTask(priority, NewRunnableMethod(backend_.get(), func,
                                             a, b, c, d));
  }


  // The thread used by the history service to run complicated operations
  base::Thread* thread_;
  
  Profile* profile_;

  bool first_launch_;
  
  FilePath thumbnail_dir_;
  
  std::vector<GURL> list_url_;

  scoped_refptr<RecentAndBookmarkThumbnailsBackendQt> backend_;

  DISALLOW_COPY_AND_ASSIGN(RecentAndBookmarkThumbnailsQt);
};

}

#endif  // CHROME_BROWSER_HISTORY_RECENT_AND_BOOKMARK_THUMBNAILS_QT_H_
