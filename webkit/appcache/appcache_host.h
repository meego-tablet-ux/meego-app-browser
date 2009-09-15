// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_APPCACHE_APPCACHE_HOST_H_
#define WEBKIT_APPCACHE_APPCACHE_HOST_H_

#include "base/ref_counted.h"
#include "base/task.h"
#include "base/weak_ptr.h"
#include "googleurl/src/gurl.h"
#include "testing/gtest/include/gtest/gtest_prod.h"
#include "webkit/appcache/appcache.h"
#include "webkit/appcache/appcache_interfaces.h"
#include "webkit/appcache/appcache_service.h"

class URLRequest;

namespace appcache {

class AppCache;
class AppCacheFrontend;
class AppCacheGroup;
class AppCacheRequestHandler;

typedef Callback2<Status, void*>::Type GetStatusCallback;
typedef Callback2<bool, void*>::Type StartUpdateCallback;
typedef Callback2<bool, void*>::Type SwapCacheCallback;

// Server-side representation of an application cache host.
class AppCacheHost : public base::SupportsWeakPtr<AppCacheHost>,
                     public AppCacheService::LoadClient {
 public:
  AppCacheHost(int host_id, AppCacheFrontend* frontend,
               AppCacheService* service);
  ~AppCacheHost();

  // Support for cache selection and scriptable method calls.
  void SelectCache(const GURL& document_url,
                   const int64 cache_document_was_loaded_from,
                   const GURL& manifest_url);
  void MarkAsForeignEntry(const GURL& document_url,
                          int64 cache_document_was_loaded_from);
  void GetStatusWithCallback(GetStatusCallback* callback,
                             void* callback_param);
  void StartUpdateWithCallback(StartUpdateCallback* callback,
                               void* callback_param);
  void SwapCacheWithCallback(SwapCacheCallback* callback,
                             void* callback_param);

  // Support for loading resources out of the appcache.
  // Returns NULL if the host is not associated with a complete cache.
  AppCacheRequestHandler* CreateRequestHandler(URLRequest* request,
                                               bool is_main_request);

  // Establishes an association between this host and a cache. 'cache' may be
  // NULL to break any existing association. Associations are established
  // either thru the cache selection algorithm implemented (in this class),
  // or by the update algorithm (see AppCacheUpdateJob).
  void AssociateCache(AppCache* cache);

  int host_id() const { return host_id_; }
  AppCacheService* service() const { return service_; }
  AppCacheFrontend* frontend() const { return frontend_; }
  AppCache* associated_cache() const { return associated_cache_.get(); }

 private:
  bool is_selection_pending() const {
    return pending_selected_cache_id_ != kNoCacheId ||
           !pending_selected_manifest_url_.is_empty();
  }
  Status GetStatus();
  void LoadCache(int64 cache_id);
  void LoadOrCreateGroup(const GURL& manifest_url);

  // LoadClient impl
  virtual void CacheLoadedCallback(AppCache* cache, int64 cache_id);
  virtual void GroupLoadedCallback(AppCacheGroup* group,
                                   const GURL& manifest_url);

  void FinishCacheSelection(AppCache* cache, AppCacheGroup* group);
  void DoPendingGetStatus();
  void DoPendingStartUpdate();
  void DoPendingSwapCache();

  // Identifies the corresponding appcache host in the child process.
  int host_id_;

  // The cache associated with this host, if any.
  scoped_refptr<AppCache> associated_cache_;

  // The reference to the group ensures the group exists
  // while we have an association with a cache in the group.
  scoped_refptr<AppCacheGroup> group_;

  // Cache loading is async, if we're loading a specific cache or group
  // for the purposes of cache selection, one or the other of these will
  // indicate which cache or group is being loaded.
  int64 pending_selected_cache_id_;
  GURL pending_selected_manifest_url_;

  // A new master entry to be added to the cache, may be empty.
  GURL new_master_entry_url_;

  // The frontend proxy to deliver notifications to the child process.
  AppCacheFrontend* frontend_;

  // Our central service object.
  AppCacheService* service_;

  // Since these are synchronous scriptable api calls in the client,
  // there can only be one type of callback pending.
  // Also, we have to wait until we have a cache selection prior
  // to responding to these calls, as cache selection involves
  // async loading of a cache or a group from storage.
  GetStatusCallback* pending_get_status_callback_;
  StartUpdateCallback* pending_start_update_callback_;
  SwapCacheCallback* pending_swap_cache_callback_;
  void* pending_callback_param_;

  FRIEND_TEST(AppCacheTest, CleanupUnusedCache);
  FRIEND_TEST(AppCacheGroupTest, CleanupUnusedGroup);
  FRIEND_TEST(AppCacheHostTest, Basic);
  FRIEND_TEST(AppCacheHostTest, SelectNoCache);
  FRIEND_TEST(AppCacheHostTest, ForeignEntry);
  FRIEND_TEST(AppCacheHostTest, FailedCacheLoad);
  FRIEND_TEST(AppCacheHostTest, FailedGroupLoad);
  DISALLOW_COPY_AND_ASSIGN(AppCacheHost);
};

}  // namespace appcache

#endif  // WEBKIT_APPCACHE_APPCACHE_HOST_H_
