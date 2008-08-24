// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>
#include <string>
#include <cstdio>

#include "base/message_loop.h"
#include "base/file_util.h"
#include "base/path_service.h"
#include "base/shared_memory.h"
#include "base/string_util.h"
#include "chrome/browser/visitedlink_master.h"
#include "chrome/renderer/visitedlink_slave.h"
#include "googleurl/src/gurl.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// a nice long URL that we can append numbers to to get new URLs
const char g_test_prefix[] =
  "http://www.google.com/products/foo/index.html?id=45028640526508376&seq=";
const int g_test_count = 1000;

// Returns a test URL for index |i|
GURL TestURL(int i) {
  return GURL(StringPrintf("%s%d", g_test_prefix, i));
}

// when testing in single-threaded mode
VisitedLinkMaster* g_master = NULL;
std::vector<VisitedLinkSlave*> g_slaves;

VisitedLinkMaster::PostNewTableEvent SynchronousBroadcastNewTableEvent;
void SynchronousBroadcastNewTableEvent(SharedMemory* table) {
  if (table) {
    for (std::vector<VisitedLinkSlave>::size_type i = 0;
         i < (int)g_slaves.size(); i++) {
      SharedMemoryHandle new_handle = NULL;
      table->ShareToProcess(GetCurrentProcess(), &new_handle);
      g_slaves[i]->Init(new_handle);
    }
  }
}

class VisitedLinkTest : public testing::Test {
 protected:
  // Initialize the history system. This should be called before InitVisited().
  bool InitHistory() {
    history_service_ = new HistoryService;
    return history_service_->Init(history_dir_);
  }

  // Initializes the visited link objects. Pass in the size that you want a
  // freshly created table to be. 0 means use the default.
  //
  // |suppress_rebuild| is set when we're not testing rebuilding, see
  // the VisitedLinkMaster constructor.
  bool InitVisited(int initial_size, bool suppress_rebuild) {
    // Initialize the visited link system.
    master_.reset(new VisitedLinkMaster(NULL,
                                        SynchronousBroadcastNewTableEvent,
                                        history_service_, suppress_rebuild,
                                        visited_file_, initial_size));
    return master_->Init();
  }

  // May be called multiple times (some tests will do this to clear things,
  // and TearDown will do this to make sure eveything is shiny before quitting.
  void ClearDB() {
    if (master_.get())
      master_.reset(NULL);

    if (history_service_.get()) {
      history_service_->SetOnBackendDestroyTask(new MessageLoop::QuitTask);
      history_service_->Cleanup();
      history_service_ = NULL;

      // Wait for the backend class to terminate before deleting the files and
      // moving to the next test. Note: if this never terminates, somebody is
      // probably leaking a reference to the history backend, so it never calls
      // our destroy task.
      MessageLoop::current()->Run();
    }
  }

  // Loads the database from disk and makes sure that the same URLs are present
  // as were generated by TestIO_Create(). This also checks the URLs with a
  // slave to make sure it reads the data properly.
  void Reload() {
    // Clean up after our caller, who may have left the database open.
    ClearDB();

    ASSERT_TRUE(InitHistory());
    ASSERT_TRUE(InitVisited(0, true));
    master_->DebugValidate();

    // check that the table has the proper number of entries
    int used_count = master_->GetUsedCount();
    ASSERT_EQ(used_count, g_test_count);

    // Create a slave database.
    VisitedLinkSlave slave;
    SharedMemoryHandle new_handle = NULL;
    master_->ShareToProcess(GetCurrentProcess(), &new_handle);
    bool success = slave.Init(new_handle);
    ASSERT_TRUE(success);
    g_slaves.push_back(&slave);

    bool found;
    for (int i = 0; i < g_test_count; i++) {
      GURL cur = TestURL(i);
      found = master_->IsVisited(cur);
      EXPECT_TRUE(found) << "URL " << i << "not found in master.";

      found = slave.IsVisited(cur);
      EXPECT_TRUE(found) << "URL " << i << "not found in slave.";
    }

    // test some random URL so we know that it returns false sometimes too
    found = master_->IsVisited(GURL("http://unfound.site/"));
    ASSERT_FALSE(found);
    found = slave.IsVisited(GURL("http://unfound.site/"));
    ASSERT_FALSE(found);

    master_->DebugValidate();

    g_slaves.clear();
  }

  // testing::Test
  virtual void SetUp() {
    PathService::Get(base::DIR_TEMP, &history_dir_);
    file_util::AppendToPath(&history_dir_, L"VisitedLinkTest");
    file_util::Delete(history_dir_, true);
    file_util::CreateDirectory(history_dir_);

    visited_file_ = history_dir_;
    file_util::AppendToPath(&visited_file_, L"VisitedLinks");
  }

  virtual void TearDown() {
    ClearDB();
    file_util::Delete(history_dir_, true);
  }

  // Filenames for the services;
  std::wstring history_dir_;
  std::wstring visited_file_;

  scoped_ptr<VisitedLinkMaster> master_;
  scoped_refptr<HistoryService> history_service_;
};

} // namespace

// This test creates and reads some databases to make sure the data is
// preserved throughout those operations.
TEST_F(VisitedLinkTest, DatabaseIO) {
  ASSERT_TRUE(InitHistory());
  ASSERT_TRUE(InitVisited(0, true));

  for (int i = 0; i < g_test_count; i++)
    master_->AddURL(TestURL(i));

  // Test that the database was written properly
  Reload();
}

// Checks that we can delete things properly when there are collisions.
TEST_F(VisitedLinkTest, Delete) {
  static const int32 kInitialSize = 17;
  ASSERT_TRUE(InitHistory());
  ASSERT_TRUE(InitVisited(kInitialSize, true));

  // Add a cluster from 14-17 wrapping around to 0. These will all hash to the
  // same value.
  const int kFingerprint0 = kInitialSize * 0 + 14;
  const int kFingerprint1 = kInitialSize * 1 + 14;
  const int kFingerprint2 = kInitialSize * 2 + 14;
  const int kFingerprint3 = kInitialSize * 3 + 14;
  const int kFingerprint4 = kInitialSize * 4 + 14;
  master_->AddFingerprint(kFingerprint0);  // @14
  master_->AddFingerprint(kFingerprint1);  // @15
  master_->AddFingerprint(kFingerprint2);  // @16
  master_->AddFingerprint(kFingerprint3);  // @0
  master_->AddFingerprint(kFingerprint4);  // @1

  // Deleting 14 should move the next value up one slot (we do not specify an
  // order).
  EXPECT_EQ(kFingerprint3, master_->hash_table_[0]);
  master_->DeleteFingerprint(kFingerprint3, false);
  EXPECT_EQ(0, master_->hash_table_[1]);
  EXPECT_NE(0, master_->hash_table_[0]);

  // Deleting the other four should leave the table empty.
  master_->DeleteFingerprint(kFingerprint0, false);
  master_->DeleteFingerprint(kFingerprint1, false);
  master_->DeleteFingerprint(kFingerprint2, false);
  master_->DeleteFingerprint(kFingerprint4, false);

  EXPECT_EQ(0, master_->used_items_);
  for (int i = 0; i < kInitialSize; i++)
    EXPECT_EQ(0, master_->hash_table_[i]) << "Hash table has values in it.";
}

// When we delete more than kBigDeleteThreshold we trigger different behavior
// where the entire file is rewritten.
TEST_F(VisitedLinkTest, BigDelete) {
  ASSERT_TRUE(InitHistory());
  ASSERT_TRUE(InitVisited(16381, true));

  // Add the base set of URLs that won't be deleted.
  // Reload() will test for these.
  for (int32 i = 0; i < g_test_count; i++)
    master_->AddURL(TestURL(i));

  // Add more URLs than necessary to trigger this case.
  const int kTestDeleteCount = VisitedLinkMaster::kBigDeleteThreshold + 2;
  std::set<GURL> urls_to_delete;
  for (int32 i = g_test_count; i < g_test_count + kTestDeleteCount; i++) {
    GURL url(TestURL(i));
    master_->AddURL(url);
    urls_to_delete.insert(url);
  }

  master_->DeleteURLs(urls_to_delete);
  master_->DebugValidate();

  Reload();
}

TEST_F(VisitedLinkTest, DeleteAll) {
  ASSERT_TRUE(InitHistory());
  ASSERT_TRUE(InitVisited(0, true));

  {
    VisitedLinkSlave slave;
    SharedMemoryHandle new_handle = NULL;
    master_->ShareToProcess(GetCurrentProcess(), &new_handle);
    ASSERT_TRUE(slave.Init(new_handle));
    g_slaves.push_back(&slave);

    // Add the test URLs.
    for (int i = 0; i < g_test_count; i++) {
      master_->AddURL(TestURL(i));
      ASSERT_EQ(i + 1, master_->GetUsedCount());
    }
    master_->DebugValidate();

    // Make sure the slave picked up the adds.
    for (int i = 0; i < g_test_count; i++)
      EXPECT_TRUE(slave.IsVisited(TestURL(i)));

    // Clear the table and make sure the slave picked it up.
    master_->DeleteAllURLs();
    EXPECT_EQ(0, master_->GetUsedCount());
    for (int i = 0; i < g_test_count; i++) {
      EXPECT_FALSE(master_->IsVisited(TestURL(i)));
      EXPECT_FALSE(slave.IsVisited(TestURL(i)));
    }

    // Close the database.
    g_slaves.clear();
    ClearDB();
  }

  // Reopen and validate.
  ASSERT_TRUE(InitHistory());
  ASSERT_TRUE(InitVisited(0, true));
  master_->DebugValidate();
  EXPECT_EQ(0, master_->GetUsedCount());
  for (int i = 0; i < g_test_count; i++)
    EXPECT_FALSE(master_->IsVisited(TestURL(i)));
}

// This tests that the master correctly resizes its tables when it gets too
// full, notifies its slaves of the change, and updates the disk.
TEST_F(VisitedLinkTest, Resizing) {
  // Create a very small database.
  const int32 initial_size = 17;
  ASSERT_TRUE(InitHistory());
  ASSERT_TRUE(InitVisited(initial_size, true));

  // ...and a slave
  VisitedLinkSlave slave;
  SharedMemoryHandle new_handle = NULL;
  master_->ShareToProcess(GetCurrentProcess(), &new_handle);
  bool success = slave.Init(new_handle);
  ASSERT_TRUE(success);
  g_slaves.push_back(&slave);

  int32 used_count = master_->GetUsedCount();
  ASSERT_EQ(used_count, 0);

  for (int i = 0; i < g_test_count; i++) {
    master_->AddURL(TestURL(i));
    used_count = master_->GetUsedCount();
    ASSERT_EQ(i + 1, used_count);
  }

  // Verify that the table got resized sufficiently.
  int32 table_size;
  VisitedLinkCommon::Fingerprint* table;
  master_->GetUsageStatistics(&table_size, &table);
  used_count = master_->GetUsedCount();
  ASSERT_GT(table_size, used_count);
  ASSERT_EQ(used_count, g_test_count) <<
                "table count doesn't match the # of things we added";

  // Verify that the slave got the resize message and has the same
  // table information.
  int32 child_table_size;
  VisitedLinkCommon::Fingerprint* child_table;
  slave.GetUsageStatistics(&child_table_size, &child_table);
  ASSERT_EQ(table_size, child_table_size);
  for (int32 i = 0; i < table_size; i++) {
    ASSERT_EQ(table[i], child_table[i]);
  }

  master_->DebugValidate();
  g_slaves.clear();

  // This tests that the file is written correctly by reading it in using
  // a new database.
  Reload();
}

// Tests that if the database doesn't exist, it will be rebuilt from history.
TEST_F(VisitedLinkTest, Rebuild) {
  ASSERT_TRUE(InitHistory());

  // Add half of our URLs to history. This needs to be done before we
  // initialize the visited link DB.
  int history_count = g_test_count / 2;
  for (int i = 0; i < history_count; i++)
    history_service_->AddPage(TestURL(i));

  // Initialize the visited link DB. Since the visited links file doesn't exist
  // and we don't suppress history rebuilding, this will load from history.
  ASSERT_TRUE(InitVisited(0, false));

  // While the table is rebuilding, add the rest of the URLs to the visited
  // link system. This isn't guaranteed to happen during the rebuild, so we
  // can't be 100% sure we're testing the right thing, but in practice is.
  // All the adds above will generally take some time queuing up on the
  // history thread, and it will take a while to catch up to actually
  // processing the rebuild that has queued behind it. We will generally
  // finish adding all of the URLs before it has even found the first URL.
  for (int i = history_count; i < g_test_count; i++)
    master_->AddURL(TestURL(i));

  // Add one more and then delete it.
  master_->AddURL(TestURL(g_test_count));
  std::set<GURL> deleted_urls;
  deleted_urls.insert(TestURL(g_test_count));
  master_->DeleteURLs(deleted_urls);

  // Wait for the rebuild to complete. The task will terminate the message
  // loop when the rebuild is done. There's no chance that the rebuild will
  // complete before we set the task because the rebuild completion message
  // is posted to the message loop; until we Run() it, rebuild can not
  // complete.
  master_->set_rebuild_complete_task(new MessageLoop::QuitTask);
  MessageLoop::current()->Run();

  // Test that all URLs were written to the database properly.
  Reload();

  // Make sure the extra one was *not* written (Reload won't test this).
  EXPECT_FALSE(master_->IsVisited(TestURL(g_test_count)));
}

