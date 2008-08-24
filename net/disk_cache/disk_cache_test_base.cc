// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/disk_cache/disk_cache_test_base.h"

#include "net/disk_cache/backend_impl.h"
#include "net/disk_cache/disk_cache_test_util.h"
#include "net/disk_cache/mem_backend_impl.h"

void DiskCacheTestBase::SetMaxSize(int size) {
  size_ = size;
  if (cache_impl_)
    EXPECT_TRUE(cache_impl_->SetMaxSize(size));

  if (mem_cache_)
    EXPECT_TRUE(mem_cache_->SetMaxSize(size));
}

void DiskCacheTestBase::InitCache() {
  if (mask_)
    implementation_ = true;

  if (memory_only_)
    InitMemoryCache();
  else
    InitDiskCache();

  ASSERT_TRUE(NULL != cache_);
  if (first_cleanup_)
    ASSERT_EQ(0, cache_->GetEntryCount());
}

void DiskCacheTestBase::InitMemoryCache() {
  if (!implementation_) {
    cache_ = disk_cache::CreateInMemoryCacheBackend(size_);
    return;
  }

  mem_cache_ = new disk_cache::MemBackendImpl();
  cache_ = mem_cache_;
  ASSERT_TRUE(NULL != cache_);

  if (size_)
    EXPECT_TRUE(mem_cache_->SetMaxSize(size_));

  ASSERT_TRUE(mem_cache_->Init());
}

void DiskCacheTestBase::InitDiskCache() {
  std::wstring path = GetCachePath();
  if (first_cleanup_)
    ASSERT_TRUE(DeleteCache(path.c_str()));

  if (!implementation_) {
    cache_ = disk_cache::CreateCacheBackend(path, force_creation_, size_);
    return;
  }

  if (mask_)
    cache_impl_ = new disk_cache::BackendImpl(path, mask_);
  else
    cache_impl_ = new disk_cache::BackendImpl(path);

  cache_ = cache_impl_;
  ASSERT_TRUE(NULL != cache_);

  if (size_)
    EXPECT_TRUE(cache_impl_->SetMaxSize(size_));

  ASSERT_TRUE(cache_impl_->Init());
}


void DiskCacheTestBase::TearDown() {
  delete cache_;

  if (!memory_only_) {
    std::wstring path = GetCachePath();
    EXPECT_TRUE(CheckCacheIntegrity(path));
  }
}

// We are expected to leak memory when simulating crashes.
void DiskCacheTestBase::SimulateCrash() {
  ASSERT_TRUE(implementation_ && !memory_only_);
  cache_impl_->ClearRefCountForTest();

  delete cache_impl_;
  std::wstring path = GetCachePath();
  EXPECT_TRUE(CheckCacheIntegrity(path));

  if (mask_)
    cache_impl_ = new disk_cache::BackendImpl(path, mask_);
  else
    cache_impl_ = new disk_cache::BackendImpl(path);
  cache_ = cache_impl_;
  ASSERT_TRUE(NULL != cache_);

  if (size_)
    cache_impl_->SetMaxSize(size_);
  ASSERT_TRUE(cache_impl_->Init());
}

void DiskCacheTestBase::SetTestMode() {
  ASSERT_TRUE(implementation_ && !memory_only_);
  cache_impl_->SetUnitTestMode();
}

