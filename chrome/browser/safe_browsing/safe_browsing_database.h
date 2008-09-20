// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SAFE_BROWSING_SAFE_BROWSING_DATABASE_H__
#define CHROME_BROWSER_SAFE_BROWSING_SAFE_BROWSING_DATABASE_H__

#include <deque>
#include <string>
#include <vector>

#include "base/scoped_ptr.h"
#include "base/task.h"
#include "base/time.h"
#include "chrome/browser/safe_browsing/bloom_filter.h"
#include "chrome/browser/safe_browsing/safe_browsing_util.h"

class GURL;

// Encapsulates the database that stores information about phishing and malware
// sites.  There is one on-disk database for all profiles, as it doesn't
// contain user-specific data.  This object is not thread-safe, i.e. all its
// methods should be used on the same thread that it was created on, with the
// exception of NeedToCheckUrl.
class SafeBrowsingDatabase {
 public:
  // Factory method for obtaining a SafeBrowsingDatabase implementation.
  static SafeBrowsingDatabase* Create();

  virtual ~SafeBrowsingDatabase() {}

  // Initializes the database with the given filename.  The callback is
  // executed after finishing a chunk.
  virtual bool Init(const std::wstring& filename,
                    Callback0::Type* chunk_inserted_callback) = 0;

  // Deletes the current database and creates a new one.
  virtual bool ResetDatabase() = 0;

  // This function can be called on any thread to check if the given url may be
  // in the database.  If this function returns false, it is definitely not in
  // the database and ContainsUrl doesn't need to be called.  If it returns
  // true, then the url might be in the database and ContainsUrl needs to be
  // called.  This function can only be called after Init succeeded.
  virtual bool NeedToCheckUrl(const GURL& url);

  // Returns false if the given url is not in the database.  If it returns
  // true, then either "list" is the name of the matching list, or prefix_hits
  // contains the matching hash prefixes.
  virtual bool ContainsUrl(const GURL& url,
                           std::string* matching_list,
                           std::vector<SBPrefix>* prefix_hits,
                           std::vector<SBFullHashResult>* full_hits,
                           Time last_update) = 0;

  // Processes add/sub commands.  Database will free the chunks when it's done.
  virtual void InsertChunks(const std::string& list_name,
                            std::deque<SBChunk>* chunks) = 0;

  // Processs adddel/subdel commands.  Database will free chunk_deletes when
  // it's done.
  virtual void DeleteChunks(std::vector<SBChunkDelete>* chunk_deletes) = 0;

  // Returns the lists and their add/sub chunks.
  virtual void GetListsInfo(std::vector<SBListChunkRanges>* lists) = 0;

  // Call this to make all database operations synchronous.  While useful for
  // testing, this should never be called in chrome.exe because it can lead
  // to blocking user requests.
  virtual void SetSynchronous() = 0;

  // Store the results of a GetHash response. In the case of empty results, we
  // cache the prefixes until the next update so that we don't have to issue
  // further GetHash requests we know will be empty.
  virtual void CacheHashResults(
      const std::vector<SBPrefix>& prefixes,
      const std::vector<SBFullHashResult>& full_hits) = 0;

  // Called when the user's machine has resumed from a lower power state.
  virtual void HandleResume() = 0;

 protected:
  static std::wstring BloomFilterFilename(const std::wstring& db_filename);

  // Load the bloom filter off disk.  Generates one if it can't find it.
  virtual void LoadBloomFilter();

  // Deletes the on-disk bloom filter, i.e. because it's stale.
  virtual void DeleteBloomFilter();

  // Writes the current bloom filter to disk.
  virtual void WriteBloomFilter();

  // Implementation specific bloom filter building.
  virtual void BuildBloomFilter() = 0;
  virtual void AddHostToBloomFilter(int host_key) = 0;

  // Measuring false positive rate. Call this each time we look in the filter.
  virtual void IncrementBloomFilterReadCount() = 0;

  std::wstring bloom_filter_filename_;
  scoped_ptr<BloomFilter> bloom_filter_;
};

#endif  // CHROME_BROWSER_SAFE_BROWSING_SAFE_BROWSING_DATABASE_H__
