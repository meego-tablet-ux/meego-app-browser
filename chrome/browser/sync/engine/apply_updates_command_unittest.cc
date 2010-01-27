// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/engine/apply_updates_command.h"
#include "chrome/browser/sync/protocol/bookmark_specifics.pb.h"
#include "chrome/browser/sync/sessions/sync_session.h"
#include "chrome/browser/sync/syncable/directory_manager.h"
#include "chrome/browser/sync/syncable/syncable.h"
#include "chrome/browser/sync/syncable/syncable_id.h"
#include "chrome/test/sync/engine/test_directory_setter_upper.h"
#include "testing/gtest/include/gtest/gtest.h"

using std::string;
using syncable::ScopedDirLookup;
using syncable::WriteTransaction;
using syncable::ReadTransaction;
using syncable::MutableEntry;
using syncable::Entry;
using syncable::Id;
using syncable::UNITTEST;

namespace browser_sync {
using sessions::SyncSessionContext;
using sessions::SyncSession;

// A test fixture for tests exercising ApplyUpdatesCommand.
class ApplyUpdatesCommandTest : public testing::Test,
                                public SyncSession::Delegate,
                                public ModelSafeWorkerRegistrar {
 public:
  // SyncSession::Delegate implementation.
  virtual void OnSilencedUntil(const base::TimeTicks& silenced_until) {
   FAIL() << "Should not get silenced.";
  }
  virtual bool IsSyncingCurrentlySilenced() {
   ADD_FAILURE() << "No requests for silenced state should be made.";
   return false;
  }
  virtual void OnReceivedLongPollIntervalUpdate(
     const base::TimeDelta& new_interval) {
   FAIL() << "Should not get poll interval update.";
  }
  virtual void OnReceivedShortPollIntervalUpdate(
     const base::TimeDelta& new_interval) {
   FAIL() << "Should not get poll interval update.";
  }

  // ModelSafeWorkerRegistrar implementation.
  virtual void GetWorkers(std::vector<ModelSafeWorker*>* out) {}
  virtual void GetModelSafeRoutingInfo(ModelSafeRoutingInfo* out) {}

 protected:
  ApplyUpdatesCommandTest() : next_revision_(1) {}
  virtual ~ApplyUpdatesCommandTest() {}
  virtual void SetUp() {
    syncdb_.SetUp();
    context_.reset(new SyncSessionContext(NULL, syncdb_.manager(), this));
    context_->set_account_name(syncdb_.name());
  }
  virtual void TearDown() {
    syncdb_.TearDown();
  }

 protected:

  // Create a new unapplied update.
  void CreateUnappliedNewItemWithParent(const string& item_id,
                                        const string& parent_id) {
    ScopedDirLookup dir(syncdb_.manager(), syncdb_.name());
    ASSERT_TRUE(dir.good());
    WriteTransaction trans(dir, UNITTEST, __FILE__, __LINE__);
    MutableEntry entry(&trans, syncable::CREATE_NEW_UPDATE_ITEM,
        Id::CreateFromServerId(item_id));
    ASSERT_TRUE(entry.good());
    entry.Put(syncable::SERVER_VERSION, next_revision_++);
    entry.Put(syncable::IS_UNAPPLIED_UPDATE, true);

    entry.Put(syncable::SERVER_NON_UNIQUE_NAME, item_id);
    entry.Put(syncable::SERVER_PARENT_ID, Id::CreateFromServerId(parent_id));
    entry.Put(syncable::SERVER_IS_DIR, true);
    sync_pb::EntitySpecifics default_bookmark_specifics;
    default_bookmark_specifics.MutableExtension(sync_pb::bookmark);
    entry.Put(syncable::SERVER_SPECIFICS, default_bookmark_specifics);
  }

  TestDirectorySetterUpper syncdb_;
  ApplyUpdatesCommand apply_updates_command_;
  scoped_ptr<SyncSessionContext> context_;
 private:
  int64 next_revision_;
  DISALLOW_COPY_AND_ASSIGN(ApplyUpdatesCommandTest);
};

TEST_F(ApplyUpdatesCommandTest, Simple) {
  string root_server_id = syncable::kNullId.GetServerId();
  CreateUnappliedNewItemWithParent("parent", root_server_id);
  CreateUnappliedNewItemWithParent("child", "parent");

  SyncSession session(context_.get(), this);
  apply_updates_command_.ModelChangingExecuteImpl(&session);

  sessions::StatusController* status = session.status_controller();
  EXPECT_EQ(2, status->update_progress().AppliedUpdatesSize())
      << "All updates should have been attempted";
  EXPECT_EQ(0, status->conflict_progress()->ConflictingItemsSize())
      << "Simple update shouldn't result in conflicts";
  EXPECT_EQ(2, status->update_progress().SuccessfullyAppliedUpdateCount())
      << "All items should have been successfully applied";
}

TEST_F(ApplyUpdatesCommandTest, UpdateWithChildrenBeforeParents) {
  // Set a bunch of updates which are difficult to apply in the order
  // they're received due to dependencies on other unseen items.
  string root_server_id = syncable::kNullId.GetServerId();
  CreateUnappliedNewItemWithParent("a_child_created_first", "parent");
  CreateUnappliedNewItemWithParent("x_child_created_first", "parent");
  CreateUnappliedNewItemWithParent("parent", root_server_id);
  CreateUnappliedNewItemWithParent("a_child_created_second", "parent");
  CreateUnappliedNewItemWithParent("x_child_created_second", "parent");

  SyncSession session(context_.get(), this);
  apply_updates_command_.ModelChangingExecuteImpl(&session);

  sessions::StatusController* status = session.status_controller();
  EXPECT_EQ(5, status->update_progress().AppliedUpdatesSize())
      << "All updates should have been attempted";
  EXPECT_EQ(0, status->conflict_progress()->ConflictingItemsSize())
      << "Simple update shouldn't result in conflicts, even if out-of-order";
  EXPECT_EQ(5, status->update_progress().SuccessfullyAppliedUpdateCount())
      << "All updates should have been successfully applied";
}

TEST_F(ApplyUpdatesCommandTest, NestedItemsWithUnknownParent) {
  // We shouldn't be able to do anything with either of these items.
  CreateUnappliedNewItemWithParent("some_item", "unknown_parent");
  CreateUnappliedNewItemWithParent("some_other_item", "some_item");

  SyncSession session(context_.get(), this);
  apply_updates_command_.ModelChangingExecuteImpl(&session);

  sessions::StatusController* status = session.status_controller();
  EXPECT_EQ(2, status->update_progress().AppliedUpdatesSize())
      << "All updates should have been attempted";
  EXPECT_EQ(2, status->conflict_progress()->ConflictingItemsSize())
      << "All updates with an unknown ancestors should be in conflict";
  EXPECT_EQ(0, status->update_progress().SuccessfullyAppliedUpdateCount())
      << "No item with an unknown ancestor should be applied";
}

TEST_F(ApplyUpdatesCommandTest, ItemsBothKnownAndUnknown) {
  // See what happens when there's a mixture of good and bad updates.
  string root_server_id = syncable::kNullId.GetServerId();
  CreateUnappliedNewItemWithParent("first_unknown_item", "unknown_parent");
  CreateUnappliedNewItemWithParent("first_known_item", root_server_id);
  CreateUnappliedNewItemWithParent("second_unknown_item", "unknown_parent");
  CreateUnappliedNewItemWithParent("second_known_item", "first_known_item");
  CreateUnappliedNewItemWithParent("third_known_item", "fourth_known_item");
  CreateUnappliedNewItemWithParent("fourth_known_item", root_server_id);

  SyncSession session(context_.get(), this);
  apply_updates_command_.ModelChangingExecuteImpl(&session);

  sessions::StatusController* status = session.status_controller();
  EXPECT_EQ(6, status->update_progress().AppliedUpdatesSize())
      << "All updates should have been attempted";
  EXPECT_EQ(2, status->conflict_progress()->ConflictingItemsSize())
      << "The updates with unknown ancestors should be in conflict";
  EXPECT_EQ(4, status->update_progress().SuccessfullyAppliedUpdateCount())
      << "The updates with known ancestors should be successfully applied";
}

}  // namespace browser_sync
