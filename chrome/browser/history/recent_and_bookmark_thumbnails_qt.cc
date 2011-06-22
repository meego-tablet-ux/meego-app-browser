// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/callback.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "base/memory/ref_counted.h"
#include "base/string_util.h"
#include "base/task.h"
#include "base/threading/thread.h"
#include "base/file_path.h"
#include "base/utf_string_conversions.h"
#include "content/browser/cancelable_request.h"
#include "chrome/browser/history/history.h"
#include "chrome/browser/history/recent_and_bookmark_thumbnails_qt.h"
#include "chrome/browser/history/recent_and_bookmark_thumbnails_backend_qt.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/url_constants.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/resource/resource_bundle.h"


namespace history {

static const char* kRecThumbnailThreadName = "Chrome_RecThumbnailThread";

RecentAndBookmarkThumbnailsQt::RecentAndBookmarkThumbnailsQt(Profile* profile)
    : thread_(new base::Thread(kRecThumbnailThreadName)),
      first_launch_(true),
      profile_(profile) {
  DLOG(INFO)<<__FUNCTION__;
  backend_ = NULL;
}


RecentAndBookmarkThumbnailsQt::~RecentAndBookmarkThumbnailsQt() {
  DLOG(INFO)<<__FUNCTION__;
  Cleanup();
}

void RecentAndBookmarkThumbnailsQt::UnloadBackend() {
  DLOG(INFO)<<__FUNCTION__;
  if (!backend_.get())
    return;  // Already unloaded.

  //Task* closing_task =
  //    NewRunnableMethod(backend_.get(), &RecentAndBookmarkThumbnailsBackendQt::Closing);
  backend_ = NULL;
  //ScheduleTask(PRIORITY_NORMAL, closing_task);
}

void RecentAndBookmarkThumbnailsQt::Cleanup() {
  DLOG(INFO)<<__FUNCTION__;
  if (!thread_) {
    // We've already cleaned up.
    return;
  }

  BookmarkModel* model = profile_->GetBookmarkModel();
  if (model)
    model->RemoveObserver(this);
  // Unload the backend.
  UnloadBackend();

  // Delete the thread, which joins with the background thread. We defensively
  // NULL the pointer before deleting it in case somebody tries to use it
  // during shutdown, but this shouldn't happen.
  base::Thread* thread = thread_;
  thread_ = NULL;
  delete thread;
  /* 
  int count  = list_url_.size();
  for(int i=0; i<count; i++) {
    delete list_url_[i];
  }
  */
}

bool RecentAndBookmarkThumbnailsQt::Init(const FilePath& thumbnail_dir) {
  DLOG(INFO)<<__FUNCTION__;
  if (!thread_->Start()) {
    Cleanup();
    return false;
  }
  
  thumbnail_dir_ = thumbnail_dir;
  // Initailize the backend.
  LoadBackendIfNecessary();

  BookmarkModel* model = profile_->GetBookmarkModel();
  if (model)
    model->AddObserver(this);

  //CleanUnusedThumbnails();
  RegisterGetRecentlyClosedTab();
  return true;
}

bool RecentAndBookmarkThumbnailsQt::TabToValue(
    const TabRestoreService::Tab& tab,
    GURL* value) {
  if (tab.navigations.empty())
    return false;

  const TabNavigation& current_navigation = tab.navigations.at(tab.current_navigation_index);

  GURL gurl = current_navigation.virtual_url();
  if (gurl == GURL(chrome::kChromeUINewTabURL))
    return false;
  *value = gurl;

  return true;
}

bool RecentAndBookmarkThumbnailsQt::EnsureTabIsUnique(
    const GURL* value,
    std::set<string16>* unique_items) {
    DCHECK(unique_items);
    string16 url = UTF8ToUTF16(value->spec());
    string16 unique_key = url;
    if (unique_items->find(unique_key) != unique_items->end())
      return false;
    else
      unique_items->insert(unique_key);

    return true;
}

void RecentAndBookmarkThumbnailsQt::RegisterGetRecentlyClosedTab() {
  TabRestoreService* tab_restore_service = profile_->GetTabRestoreService();
  if (tab_restore_service) {
    tab_restore_service->LoadTabsFromLastSession();
    tab_restore_service->AddObserver(this);
    //TabRestoreServiceChanged(tab_restore_service);
  }
}

void RecentAndBookmarkThumbnailsQt::UnregisterGetRecentlyClosedTab() {
  TabRestoreService* tab_restore_service = profile_->GetTabRestoreService();
  if (tab_restore_service) {
    tab_restore_service->RemoveObserver(this);
    first_launch_ = false;
  }
}

void RecentAndBookmarkThumbnailsQt::TabRestoreServiceChanged(
					TabRestoreService* service) {
  CleanUnusedThumbnails(service);
}

void RecentAndBookmarkThumbnailsQt::CleanUnusedThumbnails(
					TabRestoreService* service) {
  DLOG(INFO)<<__FUNCTION__;
  assert(first_launch_);
  if (!first_launch_)
	return;
  list_url_.clear();

  if (service) {
    service->LoadTabsFromLastSession();

    const TabRestoreService::Entries& entries = service->entries();
    DLOG(INFO)<<"entries count: " << entries.size();
    std::set<string16> unique_items;
    int added_count = 0;
    const int max_count = 8;
    for (TabRestoreService::Entries::const_iterator it = entries.begin();
         it != entries.end() && added_count < max_count; ++it) {
      TabRestoreService::Entry* entry = *it;
      GURL value;
      if ((entry->type == TabRestoreService::TAB &&
           TabToValue(*static_cast<TabRestoreService::Tab*>(entry), &value) &&
           EnsureTabIsUnique(&value, &unique_items))) {
        list_url_.push_back(value);
        added_count++;
      } 
    }
  }
  
  if(list_url_.size() == 0) {
    return;
  }
  UnregisterGetRecentlyClosedTab(); 

  ScheduleAndForget(PRIORITY_NORMAL, &RecentAndBookmarkThumbnailsBackendQt::CleanUnusedThumbnails,
                    list_url_);
}

// Insert new row with bookmarked equals flase.
// Or update the thumbnail value only if url row exist.
void RecentAndBookmarkThumbnailsQt::SetRecentPageThumbnail(
				const GURL& page_url,
				const SkBitmap& thumbnail) {
  DLOG(INFO)<<__FUNCTION__;

  if (!HistoryService::CanAddURL(page_url)) 
    return; 

  ScheduleAndForget(PRIORITY_NORMAL, &RecentAndBookmarkThumbnailsBackendQt::SetPageThumbnail,
                    page_url, thumbnail);
}

// Insert new row with thumbnail = null.
// Or update the bookmarekd flag only if url row exist.
void RecentAndBookmarkThumbnailsQt::SetBookmarkPage(
				const GURL& url, bool bookmarked) {
  DLOG(INFO)<<__FUNCTION__<<url.spec()<<":"<<bookmarked;
  if (!HistoryService::CanAddURL(url)) 
    return; 
  ScheduleAndForget(PRIORITY_NORMAL, &RecentAndBookmarkThumbnailsBackendQt::SetBookmarkedPage,
                    url, bookmarked);
}

RecentAndBookmarkThumbnailsQt::Handle RecentAndBookmarkThumbnailsQt::GetRecentPageThumbnail(
				const GURL& page_url,
				CancelableRequestConsumerBase* consumer,
				ThumbnailDataCallback* callback) {
  //if(first_launch_) {
    //CleanUnusedThumbnails();
    //first_launch_=false;
  //}
  DLOG(INFO)<<__FUNCTION__;
  return Schedule(PRIORITY_NORMAL, &RecentAndBookmarkThumbnailsBackendQt::GetPageThumbnail, consumer,
                  new history::GetPageThumbnailRequest(callback), page_url);
}

void RecentAndBookmarkThumbnailsQt::ScheduleTask(SchedulePriority priority,
                                  Task* task) {
  // TODO(brettw): do prioritization.
  thread_->message_loop()->PostTask(FROM_HERE, task);
}

void RecentAndBookmarkThumbnailsQt::BookmarkNodeAdded(BookmarkModel* model,
		                                  const BookmarkNode* parent,
               			                  int index) {
  DLOG(INFO)<<__FUNCTION__;
    const BookmarkNode* node = parent->GetChild(index);
  if(node->is_url()) { 
    SetBookmarkPage(node->GetURL(), true);
  }

}
void RecentAndBookmarkThumbnailsQt::BookmarkNodeRemoved(BookmarkModel* model,
		                                  const BookmarkNode* parent,
               			                  int old_index,
                               			  const BookmarkNode* node) {
  DLOG(INFO)<<__FUNCTION__;
    if(node->is_url()) { 
    SetBookmarkPage(node->GetURL(), false);
  }
}

void RecentAndBookmarkThumbnailsQt::LoadBackendIfNecessary() {
  DLOG(INFO)<<__FUNCTION__;
  if (!thread_ || backend_)
    return;  // Failed to init, or already started loading.
  scoped_refptr<RecentAndBookmarkThumbnailsBackendQt> backend(
      new RecentAndBookmarkThumbnailsBackendQt());
  backend_.swap(backend);
  // Launch in Backend Thread.
  DLOG(INFO)<<__FUNCTION__<<"call init";
  ScheduleAndForget(PRIORITY_UI, &RecentAndBookmarkThumbnailsBackendQt::Init, thumbnail_dir_);
}


}
