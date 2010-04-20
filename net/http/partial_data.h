// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_PARTIAL_DATA_H_
#define NET_HTTP_PARTIAL_DATA_H_

#include <string>

#include "base/basictypes.h"
#include "net/base/completion_callback.h"
#include "net/http/http_byte_range.h"
#include "net/http/http_request_headers.h"

namespace disk_cache {
class Entry;
}

namespace net {

class HttpResponseHeaders;
class IOBuffer;

// This class provides support for dealing with range requests and the
// subsequent partial-content responses. We use sparse cache entries to store
// these requests. This class is tightly integrated with HttpCache::Transaction
// and it is intended to allow a cleaner implementation of that class.
//
// In order to fulfill range requests, we may have to perform a sequence of
// reads from the cache, interleaved with reads from the network / writes to the
// cache. This class basically keeps track of the data required to perform each
// of those individual network / cache requests.
class PartialData {
 public:
  PartialData()
      : range_present_(false), final_range_(false), sparse_entry_(true),
        truncated_(false) {}
  ~PartialData() {}

  // Performs initialization of the object by examining the request |headers|
  // and verifying that we can process the requested range. Returns true if
  // we can process the requested range, and false otherwise.
  bool Init(const HttpRequestHeaders& headers);

  // Sets the headers that we should use to make byte range requests. This is a
  // subset of the request extra headers, with byte-range related headers
  // removed.
  void SetHeaders(const HttpRequestHeaders& headers);

  // Restores the byte-range headers, by appending the byte range to the headers
  // provided to SetHeaders().
  void RestoreHeaders(HttpRequestHeaders* headers) const;

  // Builds the required |headers| to perform the proper cache validation for
  // the next range to be fetched. Returns 0 when there is no need to perform
  // more operations because we reached the end of the request (so 0 bytes
  // should be actually returned to the user), a positive number to indicate
  // that |headers| should be used to validate the cache, or an appropriate
  // error code.
  int PrepareCacheValidation(disk_cache::Entry* entry,
                             HttpRequestHeaders* headers);

  // Returns true if the current range is stored in the cache.
  bool IsCurrentRangeCached() const;

  // Returns true if the current range is the last one needed to fulfill the
  // user's request.
  bool IsLastRange() const;

  // Extracts info from headers already stored in the cache. Returns false if
  // there is any problem with the headers. |truncated| should be true if we
  // have an incomplete 200 entry.
  bool UpdateFromStoredHeaders(const HttpResponseHeaders* headers,
                               disk_cache::Entry* entry, bool truncated);

  // Returns true if the requested range is valid given the stored data.
  bool IsRequestedRangeOK();

  // Returns true if the response headers match what we expect, false otherwise.
  bool ResponseHeadersOK(const HttpResponseHeaders* headers);

  // Fixes the response headers to include the right content length and range.
  void FixResponseHeaders(HttpResponseHeaders* headers);

  // Fixes the content length that we want to store in the cache.
  void FixContentLength(HttpResponseHeaders* headers);

  // Reads up to |data_len| bytes from the cache and stores them in the provided
  // buffer (|data|). Basically, this is just a wrapper around the API of the
  // cache that provides the right arguments for the current range. When the IO
  // operation completes, OnCacheReadCompleted() must be called with the result
  // of the operation.
  int CacheRead(disk_cache::Entry* entry, IOBuffer* data, int data_len,
                CompletionCallback* callback);

  // Writes |data_len| bytes to cache. This is basically a wrapper around the
  // API of the cache that provides the right arguments for the current range.
  int CacheWrite(disk_cache::Entry* entry, IOBuffer* data, int data_len,
                 CompletionCallback* callback);

  // This method should be called when CacheRead() finishes the read, to update
  // the internal state about the current range.
  void OnCacheReadCompleted(int result);

  // This method should be called after receiving data from the network, to
  // update the internal state about the current range.
  void OnNetworkReadCompleted(int result);

 private:
  int64 current_range_start_;
  int64 cached_start_;
  int64 resource_size_;
  int cached_min_len_;
  HttpByteRange byte_range_;  // The range requested by the user.
  // The clean set of extra headers (no ranges).
  HttpRequestHeaders extra_headers_;
  bool range_present_;  // True if next range entry is already stored.
  bool final_range_;
  bool sparse_entry_;
  bool truncated_;  // We have an incomplete 200 stored.

  DISALLOW_COPY_AND_ASSIGN(PartialData);
};

}  // namespace net

#endif  // NET_HTTP_PARTIAL_DATA_H_
