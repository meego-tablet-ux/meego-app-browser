// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/thumbnail_store.h"

#include <string.h>
#include <algorithm>

#include "base/basictypes.h"
#include "base/file_util.h"
#include "base/gfx/jpeg_codec.h"
#include "base/md5.h"
#include "base/string_util.h"
#include "base/thread.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profile.h"
#include "chrome/common/pref_service.h"
#include "chrome/common/sqlite_utils.h"
#include "googleurl/src/gurl.h"
#include "third_party/skia/include/core/SkBitmap.h"


ThumbnailStore::ThumbnailStore()
    : cache_(NULL),
      db_(NULL),
      hs_(NULL),
      url_blacklist_(NULL) {
}

ThumbnailStore::~ThumbnailStore() {
  // Ensure that shutdown was called.
  DCHECK(hs_ == NULL);
}

void ThumbnailStore::Init(const FilePath& db_name,
                          Profile* profile) {
  // Load thumbnails already in the database.
  g_browser_process->file_thread()->message_loop()->PostTask(FROM_HERE,
      NewRunnableMethod(this, &ThumbnailStore::InitializeFromDB,
                        db_name, MessageLoop::current()));

  // Take ownership of a reference to the HistoryService.
  hs_ = profile->GetHistoryService(Profile::EXPLICIT_ACCESS);

  // Store a pointer to a persistent table of blacklisted URLs.
  url_blacklist_ = profile->GetPrefs()->
    GetMutableDictionary(prefs::kNTPMostVisitedURLsBlacklist);

  // Get the list of most visited URLs and redirect information from the
  // HistoryService.
  seconds_to_next_update_ = kInitialUpdateIntervalSecs;
  UpdateURLData();

  // Register to get notified when the history is cleared.
  registrar_.Add(this, NotificationType::HISTORY_URLS_DELETED,
                 Source<Profile>(profile));
}

bool ThumbnailStore::SetPageThumbnail(const GURL& url,
                                      const SkBitmap& thumbnail,
                                      const ThumbnailScore& score,
                                      bool fetch_redirects) {
  if (!cache_.get())
    return false;

  if (!ShouldStoreThumbnailForURL(url) ||
      (cache_->find(url) != cache_->end() &&
      !ShouldReplaceThumbnailWith((*cache_)[url].score_, score)))
    return true;

  base::TimeTicks encode_start = base::TimeTicks::Now();

  // Encode the SkBitmap to jpeg.
  scoped_refptr<RefCountedBytes> jpeg_data = new RefCountedBytes;
  SkAutoLockPixels thumbnail_lock(thumbnail);
  bool encoded = JPEGCodec::Encode(
      reinterpret_cast<unsigned char*>(thumbnail.getAddr32(0, 0)),
      JPEGCodec::FORMAT_BGRA, thumbnail.width(),
      thumbnail.height(),
      static_cast<int>(thumbnail.rowBytes()), 90,
      &jpeg_data->data);

  base::TimeDelta delta = base::TimeTicks::Now() - encode_start;
  HISTOGRAM_TIMES("Thumbnail.Encode", delta);

  if (!encoded)
    return false;

  // Update the cache_ with the new thumbnail.
  (*cache_)[url] = CacheEntry(jpeg_data, score, true);

  // Get redirects for this URL.
  if (fetch_redirects) {
    hs_->QueryRedirectsTo(url, &consumer_,
        NewCallback(this, &ThumbnailStore::OnRedirectsForURLAvailable));
  }

  return true;
}

bool ThumbnailStore::GetPageThumbnail(
    const GURL& url,
    RefCountedBytes** data) {
  if (!cache_.get() || IsURLBlacklisted(url))
    return false;

  // Look up the |url| in the redirect list to find the final destination
  // which is the key into the |cache_|.
  history::RedirectMap::iterator it = redirect_urls_->find(url);
  if (it != redirect_urls_->end()) {
    // Return the first available thumbnail starting at the end of the
    // redirect list.
    history::RedirectList::reverse_iterator rit;
    for (rit = it->second->data.rbegin();
        rit != it->second->data.rend(); ++rit) {
      if (cache_->find(*rit) != cache_->end()) {
        *data = (*cache_)[*rit].data_.get();
        (*data)->AddRef();
        return true;
      }
    }
  }

  // TODO(meelapshah) bug 14643: check past redirect lists

  if (cache_->find(url) == cache_->end())
    return false;

  *data = (*cache_)[url].data_.get();
  (*data)->AddRef();
  return true;
}

void ThumbnailStore::Shutdown() {
  // We must release our reference to the HistoryService here to prevent
  // shutdown issues. Please refer to the comment in HistoryService::Cleanup
  // for details.
  hs_ = NULL;

  // The source of notifications is the Profile. We may outlive the Profile so
  // we unregister for notifications here.
  registrar_.RemoveAll();

  // Stop the timer to ensure that UpdateURLData is not called during shutdown.
  timer_.Stop();

  // Write the cache to disk.  This will schedule the disk operations to be run
  // on the file_thread.  Note that Join() does not need to be called with the
  // file_thread because when the disk operation is scheduled, it will hold a
  // reference to |this| keeping this object alive.
  CleanCacheData();
}

void ThumbnailStore::OnRedirectsForURLAvailable(
    HistoryService::Handle handle,
    GURL url,
    bool success,
    history::RedirectList* redirects) {
  if (!success)
    return;

  if (redirects->empty()) {
    (*redirect_urls_)[url] = new RefCountedVector<GURL>;
  } else {
    const GURL start_url = redirects->back();
    std::reverse(redirects->begin(), redirects->end() - 1);
    *(redirects->end() - 1) = url;
    (*redirect_urls_)[start_url] = new RefCountedVector<GURL>(*redirects);
  }
}

void ThumbnailStore::Observe(NotificationType type,
                             const NotificationSource& source,
                             const NotificationDetails& details) {
  if (type.value != NotificationType::HISTORY_URLS_DELETED) {
    NOTREACHED();
    return;
  }

  Details<history::URLsDeletedDetails> url_details(details);
  // If all history was cleared, clear all of our data and reset the update
  // timer.
  if (url_details->all_history) {
    most_visited_urls_.reset();
    redirect_urls_.reset();
    cache_.reset();

    timer_.Stop();
    seconds_to_next_update_ = kInitialUpdateIntervalSecs;
    timer_.Start(base::TimeDelta::FromSeconds(seconds_to_next_update_), this,
        &ThumbnailStore::UpdateURLData);
  }
}

void ThumbnailStore::UpdateURLData() {
  int result_count = ThumbnailStore::kMaxCacheSize + url_blacklist_->GetSize();
  hs_->QueryTopURLsAndRedirects(result_count, &consumer_,
      NewCallback(this, &ThumbnailStore::OnURLDataAvailable));
}

void ThumbnailStore::OnURLDataAvailable(std::vector<GURL>* urls,
                                        history::RedirectMap* redirects) {
  DCHECK(urls);
  DCHECK(redirects);

  most_visited_urls_.reset(new std::vector<GURL>(*urls));
  redirect_urls_.reset(new history::RedirectMap(*redirects));
  CleanCacheData();

  // Schedule the next update.
  if (seconds_to_next_update_ < kMaxUpdateIntervalSecs)
    seconds_to_next_update_ *= 2;
  timer_.Start(base::TimeDelta::FromSeconds(seconds_to_next_update_), this,
      &ThumbnailStore::UpdateURLData);
}

void ThumbnailStore::CleanCacheData() {
  if (!cache_.get())
    return;

  scoped_refptr<RefCountedVector<GURL> > urls_to_delete =
      new RefCountedVector<GURL>;
  Cache* data_to_save = new Cache;  // CommitCacheToDB will delete this

  // Iterate the cache, storing urls to be deleted and dirty cache entries to
  // be written to disk.
  for (Cache::iterator cache_it = cache_->begin();
       cache_it != cache_->end();) {
    const GURL* url = NULL;
    // For each URL in the cache, search the RedirectMap for the originating
    // URL.
    for (history::RedirectMap::iterator it = redirect_urls_->begin();
         it != redirect_urls_->end(); ++it) {
      if (cache_it->first == it->first ||
          (it->second->data.size() &&
          cache_it->first == it->second->data.back())) {
        url = &it->first;
        break;
      }
    }

    // If this URL is blacklisted or not in the most visited list, mark it for
    // deletion. Otherwise, if the cache entry is dirty, mark it to be written
    // to disk.
    if (url == NULL || IsURLBlacklisted(*url) || !IsPopular(*url)) {
      // Note that we don't check whether the cache entry is dirty or not. If
      // it is not dirty, then the thumbnail exists on disk and must be
      // deleted. If it is dirty, it may exist on disk so we delete it anyways.
      urls_to_delete->data.push_back(cache_it->first);
      cache_->erase(cache_it++);
    } else {
      if (cache_it->second.dirty_) {
        data_to_save->insert(*cache_it);
        cache_it->second.dirty_ = false;
      }
      ++cache_it;
    }
  }

  g_browser_process->file_thread()->message_loop()->PostTask(FROM_HERE,
      NewRunnableMethod(this, &ThumbnailStore::CommitCacheToDB,
                        urls_to_delete, data_to_save));
}

void ThumbnailStore::CommitCacheToDB(
    scoped_refptr<RefCountedVector<GURL> > urls_to_delete,
    Cache* data) const {
  scoped_ptr<Cache> data_to_save(data);
  if (!db_)
    return;

  int rv = sqlite3_exec(db_, "BEGIN TRANSACTION", NULL, NULL, NULL);
  DCHECK(rv == SQLITE_OK) << "Failed to begin transaction";

  // Delete old thumbnails.
  if (urls_to_delete.get()) {
    for (std::vector<GURL>::iterator it = urls_to_delete->data.begin();
        it != urls_to_delete->data.end(); ++it) {
      SQLITE_UNIQUE_STATEMENT(statement, *statement_cache_,
          "DELETE FROM thumbnails WHERE url=?");
      statement->bind_string(0, it->spec());
      if (statement->step() != SQLITE_DONE)
        NOTREACHED();
    }
  }

  // Update cached thumbnails.
  if (data_to_save.get()) {
    for (Cache::iterator it = data_to_save->begin();
         it != data_to_save->end(); ++it) {
      SQLITE_UNIQUE_STATEMENT(statement, *statement_cache_,
          "INSERT OR REPLACE INTO thumbnails "
          "(url, boring_score, good_clipping, at_top, time_taken, data) "
          "VALUES (?,?,?,?,?,?)");
      statement->bind_string(0, it->first.spec());
      statement->bind_double(1, it->second.score_.boring_score);
      statement->bind_bool(2, it->second.score_.good_clipping);
      statement->bind_bool(3, it->second.score_.at_top);
      statement->bind_int64(4, it->second.score_.time_at_snapshot.
                                  ToInternalValue());
      statement->bind_blob(5, &it->second.data_->data[0],
                           static_cast<int>(it->second.data_->data.size()));
      if (statement->step() != SQLITE_DONE)
        DLOG(WARNING) << "Unable to insert thumbnail for URL";
    }
  }

  rv = sqlite3_exec(db_, "COMMIT", NULL, NULL, NULL);
  DCHECK(rv == SQLITE_OK) << "Failed to commit transaction";
}

void ThumbnailStore::InitializeFromDB(const FilePath& db_name,
                                      MessageLoop* cb_loop) {
  if (OpenSqliteDb(db_name, &db_) != SQLITE_OK)
    return;

  // Use a large page size since the thumbnails we are storing are typically
  // large, a small cache size since we cache in memory and don't go to disk
  // often, and take exclusive access since nobody else uses this db.
  sqlite3_exec(db_, "PRAGMA page_size=4096 "
                    "PRAGMA cache_size=64 "
                    "PRAGMA locking_mode=EXCLUSIVE", NULL, NULL, NULL);

  statement_cache_ = new SqliteStatementCache;

  // Use local DBCloseScoper so that if we cannot create the table and
  // need to return, the |db_| and |statement_cache_| are closed properly.
  history::DBCloseScoper scoper(&db_, &statement_cache_);

  if (!DoesSqliteTableExist(db_, "thumbnails")) {
    if (sqlite3_exec(db_, "CREATE TABLE thumbnails ("
          "url LONGVARCHAR PRIMARY KEY,"
          "boring_score DOUBLE DEFAULT 1.0,"
          "good_clipping INTEGER DEFAULT 0,"
          "at_top INTEGER DEFAULT 0,"
          "time_taken INTEGER DEFAULT 0,"
          "data BLOB)", NULL, NULL, NULL) != SQLITE_OK)
      return;
  }

  statement_cache_->set_db(db_);

  // Now we can use a DBCloseScoper at the object scope.
  scoper.Detach();
  close_scoper_.Attach(&db_, &statement_cache_);

  if (cb_loop)
    GetAllThumbnailsFromDisk(cb_loop);
}

void ThumbnailStore::GetAllThumbnailsFromDisk(MessageLoop* cb_loop) {
  ThumbnailStore::Cache* cache = new ThumbnailStore::Cache;

  SQLITE_UNIQUE_STATEMENT(statement, *statement_cache_,
      "SELECT * FROM thumbnails");

  while (statement->step() == SQLITE_ROW) {
    GURL url(statement->column_string(0));
    ThumbnailScore score(statement->column_double(1),      // Boring score
                         statement->column_bool(2),        // Good clipping
                         statement->column_bool(3),        // At top
                         base::Time::FromInternalValue(
                            statement->column_int64(4)));  // Time taken
    scoped_refptr<RefCountedBytes> data = new RefCountedBytes;
    if (statement->column_blob_as_vector(5, &data->data))
      (*cache)[url] = CacheEntry(data, score, false);
  }

  cb_loop->PostTask(FROM_HERE,
      NewRunnableMethod(this, &ThumbnailStore::OnDiskDataAvailable, cache));
}

void ThumbnailStore::OnDiskDataAvailable(ThumbnailStore::Cache* cache) {
  if (cache)
    cache_.reset(cache);
}

bool ThumbnailStore::ShouldStoreThumbnailForURL(const GURL& url) const {
  if (IsURLBlacklisted(url) || cache_->size() >= ThumbnailStore::kMaxCacheSize)
    return false;

  return most_visited_urls_->size() < ThumbnailStore::kMaxCacheSize ||
         IsPopular(url);
}

bool ThumbnailStore::IsURLBlacklisted(const GURL& url) const {
  if (url_blacklist_)
    return url_blacklist_->HasKey(GetDictionaryKeyForURL(url.spec()));
  return false;
}

std::wstring ThumbnailStore::GetDictionaryKeyForURL(
    const std::string& url) const {
  return ASCIIToWide(MD5String(url));
}

bool ThumbnailStore::IsPopular(const GURL& url) const {
  return most_visited_urls_->end() != find(most_visited_urls_->begin(),
                                           most_visited_urls_->end(),
                                           url);
}
