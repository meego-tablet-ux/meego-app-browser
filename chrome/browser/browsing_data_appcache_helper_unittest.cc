// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data_appcache_helper.h"

#include "base/stl_util-inl.h"
#include "chrome/test/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
class TestCompletionCallback : public CallbackRunner<Tuple0> {
 public:
  TestCompletionCallback()
      : have_result_(false) {
  }

  bool have_result() const { return have_result_; }

  virtual void RunWithParams(const Tuple0& params) {
    have_result_ = true;
  }
 private:
  bool have_result_;
};

}  // namespace

TEST(CannedBrowsingDataAppCacheHelperTest, SetInfo) {
  TestingProfile profile;

  GURL manifest1("http://example1.com/manifest.xml");
  GURL manifest2("http://example2.com/path1/manifest.xml");
  GURL manifest3("http://example2.com/path2/manifest.xml");

  scoped_refptr<CannedBrowsingDataAppCacheHelper> helper =
      new CannedBrowsingDataAppCacheHelper(&profile);
  helper->AddAppCache(manifest1);
  helper->AddAppCache(manifest2);
  helper->AddAppCache(manifest3);

  TestCompletionCallback callback;
  helper->StartFetching(&callback);
  ASSERT_TRUE(callback.have_result());

  std::map<GURL, appcache::AppCacheInfoVector>& collection =
      helper->info_collection()->infos_by_origin;

  ASSERT_EQ(2u, collection.size());
  EXPECT_TRUE(ContainsKey(collection, manifest1.GetOrigin()));
  ASSERT_EQ(1u, collection[manifest1.GetOrigin()].size());
  EXPECT_EQ(manifest1, collection[manifest1.GetOrigin()].at(0).manifest_url);

  EXPECT_TRUE(ContainsKey(collection, manifest2.GetOrigin()));
  EXPECT_EQ(2u, collection[manifest2.GetOrigin()].size());
  std::set<GURL> manifest_results;
  manifest_results.insert(collection[manifest2.GetOrigin()].at(0).manifest_url);
  manifest_results.insert(collection[manifest2.GetOrigin()].at(1).manifest_url);
  EXPECT_TRUE(ContainsKey(manifest_results, manifest2));
  EXPECT_TRUE(ContainsKey(manifest_results, manifest3));
}
