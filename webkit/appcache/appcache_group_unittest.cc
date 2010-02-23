// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "webkit/appcache/appcache.h"
#include "webkit/appcache/appcache_group.h"
#include "webkit/appcache/appcache_host.h"
#include "webkit/appcache/mock_appcache_service.h"
#include "webkit/appcache/appcache_update_job.h"

namespace {

class TestAppCacheFrontend : public appcache::AppCacheFrontend {
 public:
  TestAppCacheFrontend()
      : last_host_id_(-1), last_cache_id_(-1),
        last_status_(appcache::OBSOLETE) {
  }

  virtual void OnCacheSelected(int host_id, int64 cache_id ,
                               appcache::Status status) {
    last_host_id_ = host_id;
    last_cache_id_ = cache_id;
    last_status_ = status;
  }

  virtual void OnStatusChanged(const std::vector<int>& host_ids,
                               appcache::Status status) {
  }

  virtual void OnEventRaised(const std::vector<int>& host_ids,
                             appcache::EventID event_id) {
  }

  int last_host_id_;
  int64 last_cache_id_;
  appcache::Status last_status_;
};

}  // namespace anon

namespace appcache {

class TestUpdateObserver : public AppCacheGroup::UpdateObserver {
 public:
  TestUpdateObserver() : update_completed_(false), group_has_cache_(false) {
  }

  virtual void OnUpdateComplete(AppCacheGroup* group) {
    update_completed_ = true;
    group_has_cache_ = group->HasCache();
  }

  bool update_completed_;
  bool group_has_cache_;
};

class TestAppCacheHost : public AppCacheHost {
 public:
  TestAppCacheHost(int host_id, AppCacheFrontend* frontend,
                   AppCacheService* service)
      : AppCacheHost(host_id, frontend, service),
        update_completed_(false) {
  }

  virtual void OnUpdateComplete(AppCacheGroup* group) {
    update_completed_ = true;
  }

  bool update_completed_;
};

class AppCacheGroupTest : public testing::Test {
};

TEST(AppCacheGroupTest, AddRemoveCache) {
  MockAppCacheService service;
  scoped_refptr<AppCacheGroup> group =
      new AppCacheGroup(&service, GURL("http://foo.com"), 111);

  base::Time now = base::Time::Now();

  scoped_refptr<AppCache> cache1 = new AppCache(&service, 111);
  cache1->set_complete(true);
  cache1->set_update_time(now);
  group->AddCache(cache1);
  EXPECT_EQ(cache1, group->newest_complete_cache());

  // Adding older cache does not change newest complete cache.
  scoped_refptr<AppCache> cache2 = new AppCache(&service, 222);
  cache2->set_complete(true);
  cache2->set_update_time(now - base::TimeDelta::FromDays(1));
  group->AddCache(cache2);
  EXPECT_EQ(cache1, group->newest_complete_cache());

  // Adding newer cache does change newest complete cache.
  scoped_refptr<AppCache> cache3 = new AppCache(&service, 333);
  cache3->set_complete(true);
  cache3->set_update_time(now + base::TimeDelta::FromDays(1));
  group->AddCache(cache3);
  EXPECT_EQ(cache3, group->newest_complete_cache());

  // Adding cache with same update time uses one with larger ID.
  scoped_refptr<AppCache> cache4 = new AppCache(&service, 444);
  cache4->set_complete(true);
  cache4->set_update_time(now + base::TimeDelta::FromDays(1));  // same as 3
  group->AddCache(cache4);
  EXPECT_EQ(cache4, group->newest_complete_cache());

  scoped_refptr<AppCache> cache5 = new AppCache(&service, 55);  // smaller id
  cache5->set_complete(true);
  cache5->set_update_time(now + base::TimeDelta::FromDays(1));  // same as 4
  group->AddCache(cache5);
  EXPECT_EQ(cache4, group->newest_complete_cache());  // no change

  // Old caches can always be removed.
  group->RemoveCache(cache1);
  EXPECT_FALSE(cache1->owning_group());
  EXPECT_EQ(cache4, group->newest_complete_cache());  // newest unchanged

  // Remove rest of caches.
  group->RemoveCache(cache2);
  EXPECT_FALSE(cache2->owning_group());
  EXPECT_EQ(cache4, group->newest_complete_cache());  // newest unchanged
  group->RemoveCache(cache3);
  EXPECT_FALSE(cache3->owning_group());
  EXPECT_EQ(cache4, group->newest_complete_cache());  // newest unchanged
  group->RemoveCache(cache5);
  EXPECT_FALSE(cache5->owning_group());
  EXPECT_EQ(cache4, group->newest_complete_cache());  // newest unchanged
  group->RemoveCache(cache4);  // newest removed
  EXPECT_FALSE(cache4->owning_group());
  EXPECT_FALSE(group->newest_complete_cache());       // no more newest cache

  // Can remove newest cache if there are older caches.
  group->AddCache(cache1);
  EXPECT_EQ(cache1, group->newest_complete_cache());
  group->AddCache(cache4);
  EXPECT_EQ(cache4, group->newest_complete_cache());
  group->RemoveCache(cache4);  // remove newest
  EXPECT_FALSE(cache4->owning_group());
  EXPECT_FALSE(group->newest_complete_cache());  // newest removed
}

TEST(AppCacheGroupTest, CleanupUnusedGroup) {
  MockAppCacheService service;
  TestAppCacheFrontend frontend;
  AppCacheGroup* group =
      new AppCacheGroup(&service, GURL("http://foo.com"), 111);

  AppCacheHost host1(1, &frontend, &service);
  AppCacheHost host2(2, &frontend, &service);

  base::Time now = base::Time::Now();

  AppCache* cache1 = new AppCache(&service, 111);
  cache1->set_complete(true);
  cache1->set_update_time(now);
  group->AddCache(cache1);
  EXPECT_EQ(cache1, group->newest_complete_cache());

  host1.AssociateCache(cache1);
  EXPECT_EQ(frontend.last_host_id_, host1.host_id());
  EXPECT_EQ(frontend.last_cache_id_, cache1->cache_id());
  EXPECT_EQ(frontend.last_status_, appcache::IDLE);

  host2.AssociateCache(cache1);
  EXPECT_EQ(frontend.last_host_id_, host2.host_id());
  EXPECT_EQ(frontend.last_cache_id_, cache1->cache_id());
  EXPECT_EQ(frontend.last_status_, appcache::IDLE);

  AppCache* cache2 = new AppCache(&service, 222);
  cache2->set_complete(true);
  cache2->set_update_time(now + base::TimeDelta::FromDays(1));
  group->AddCache(cache2);
  EXPECT_EQ(cache2, group->newest_complete_cache());

  // Unassociate all hosts from older cache.
  host1.AssociateCache(NULL);
  host2.AssociateCache(NULL);
  EXPECT_EQ(frontend.last_host_id_, host2.host_id());
  EXPECT_EQ(frontend.last_cache_id_, appcache::kNoCacheId);
  EXPECT_EQ(frontend.last_status_, appcache::UNCACHED);
}

TEST(AppCacheGroupTest, StartUpdate) {
  MockAppCacheService service;
  scoped_refptr<AppCacheGroup> group =
      new AppCacheGroup(&service, GURL("http://foo.com"), 111);

  // Set state to checking to prevent update job from executing fetches.
  group->update_status_ = AppCacheGroup::CHECKING;
  group->StartUpdate();
  AppCacheUpdateJob* update = group->update_job_;
  EXPECT_TRUE(update != NULL);

  // Start another update, check that same update job is in use.
  group->StartUpdateWithHost(NULL);
  EXPECT_EQ(update, group->update_job_);

  // Deleting the update should restore the group to IDLE.
  delete update;
  EXPECT_TRUE(group->update_job_ == NULL);
  EXPECT_EQ(AppCacheGroup::IDLE, group->update_status());
}

TEST(AppCacheGroupTest, CancelUpdate) {
  MockAppCacheService service;
  scoped_refptr<AppCacheGroup> group =
      new AppCacheGroup(&service, GURL("http://foo.com"), 111);

  // Set state to checking to prevent update job from executing fetches.
  group->update_status_ = AppCacheGroup::CHECKING;
  group->StartUpdate();
  AppCacheUpdateJob* update = group->update_job_;
  EXPECT_TRUE(update != NULL);

  // Deleting the group should cancel the update.
  TestUpdateObserver observer;
  group->AddUpdateObserver(&observer);
  group = NULL;  // causes group to be deleted
  EXPECT_TRUE(observer.update_completed_);
  EXPECT_FALSE(observer.group_has_cache_);
}

TEST(AppCacheGroupTest, QueueUpdate) {
  MockAppCacheService service;
  scoped_refptr<AppCacheGroup> group =
      new AppCacheGroup(&service, GURL("http://foo.com"), 111);

  // Set state to checking to prevent update job from executing fetches.
  group->update_status_ = AppCacheGroup::CHECKING;
  group->StartUpdate();
  EXPECT_TRUE(group->update_job_);

  // Pretend group's update job is terminating so that next update is queued.
  group->update_job_->internal_state_ = AppCacheUpdateJob::REFETCH_MANIFEST;
  EXPECT_TRUE(group->update_job_->IsTerminating());

  TestAppCacheFrontend frontend;
  TestAppCacheHost host(1, &frontend, &service);
  host.new_master_entry_url_ = GURL("http://foo.com/bar.txt");
  group->StartUpdateWithNewMasterEntry(&host, host.new_master_entry_url_);
  EXPECT_FALSE(group->queued_updates_.empty());

  group->AddUpdateObserver(&host);
  EXPECT_FALSE(group->FindObserver(&host, group->observers_));
  EXPECT_TRUE(group->FindObserver(&host, group->queued_observers_));

  // Delete update to cause it to complete. Verify no update complete notice
  // sent to host.
  delete group->update_job_;
  EXPECT_EQ(AppCacheGroup::IDLE, group->update_status_);
  EXPECT_TRUE(group->restart_update_task_);
  EXPECT_FALSE(host.update_completed_);

  // Start another update. Cancels task and will run queued updates.
  group->update_status_ = AppCacheGroup::CHECKING;  // prevent actual fetches
  group->StartUpdate();
  EXPECT_TRUE(group->update_job_);
  EXPECT_FALSE(group->restart_update_task_);
  EXPECT_TRUE(group->queued_updates_.empty());
  EXPECT_FALSE(group->update_job_->pending_master_entries_.empty());
  EXPECT_FALSE(group->FindObserver(&host, group->queued_observers_));
  EXPECT_TRUE(group->FindObserver(&host, group->observers_));

  // Delete update to cause it to complete. Verify host is notified.
  delete group->update_job_;
  EXPECT_EQ(AppCacheGroup::IDLE, group->update_status_);
  EXPECT_FALSE(group->restart_update_task_);
  EXPECT_TRUE(host.update_completed_);
}

}  // namespace appcache
