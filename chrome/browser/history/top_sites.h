// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_HISTORY_TOP_SITES_H_
#define CHROME_BROWSER_HISTORY_TOP_SITES_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/gtest_prod_util.h"
#include "base/lock.h"
#include "base/timer.h"
#include "base/ref_counted.h"
#include "base/ref_counted_memory.h"
#include "chrome/browser/cancelable_request.h"
#include "chrome/browser/history/history_types.h"
#include "chrome/browser/history/history.h"
#include "chrome/browser/history/page_usage_data.h"
#include "chrome/common/notification_service.h"
#include "chrome/common/thumbnail_score.h"
#include "googleurl/src/gurl.h"

class SkBitmap;
class Profile;

namespace history {

class TopSitesBackend;
class TopSitesDatabase;
class TopSitesTest;

typedef std::vector<MostVisitedURL> MostVisitedURLList;

// Stores the data for the top "most visited" sites. This includes a cache of
// the most visited data from history, as well as the corresponding thumbnails
// of those sites.
//
// This class IS threadsafe. It is designed to be used from the UI thread of
// the browser (where history requests must be kicked off and received from)
// and from the I/O thread (where new tab page requests come in). Handling the
// new tab page requests on the I/O thread without proxying to the UI thread is
// a nontrivial performance win, especially when the browser is starting and
// the UI thread is busy.
class TopSites : public NotificationObserver,
                 public base::RefCountedThreadSafe<TopSites>,
                 public CancelableRequestProvider {
 public:
  explicit TopSites(Profile* profile);

  class MockHistoryService {
    // A mockup of a HistoryService used for testing TopSites.
   public:
    virtual HistoryService::Handle QueryMostVisitedURLs(
        int result_count, int days_back,
        CancelableRequestConsumerBase* consumer,
        HistoryService::QueryMostVisitedURLsCallback* callback) = 0;
    virtual ~MockHistoryService() {}
    virtual void GetPageThumbnail(
        const GURL& page_url,
        CancelableRequestConsumerTSimple<size_t>* consumer,
        HistoryService::ThumbnailDataCallback* callback,
        size_t index) = 0;
  };

  struct Images {
    scoped_refptr<RefCountedBytes> thumbnail;
    ThumbnailScore thumbnail_score;

    // TODO(brettw): this will eventually store the favicon.
    // scoped_refptr<RefCountedBytes> favicon;
  };

  // Initializes TopSites.
  void Init(const FilePath& db_name);

  // Sets the given thumbnail for the given URL. Returns true if the thumbnail
  // was updated. False means either the URL wasn't known to us, or we felt
  // that our current thumbnail was superior to the given one.
  bool SetPageThumbnail(const GURL& url,
                        const SkBitmap& thumbnail,
                        const ThumbnailScore& score);

  // Callback for GetMostVisitedURLs.
  typedef Callback1<const MostVisitedURLList&>::Type GetTopSitesCallback;

  // Returns a list of most visited URLs via a callback.
  // NOTE: the callback may be called immediately if we have the data cached.
  void GetMostVisitedURLs(CancelableRequestConsumer* consumer,
                          GetTopSitesCallback* callback);

  // Get a thumbnail for a given page. Returns true iff we have the thumbnail.
  bool GetPageThumbnail(const GURL& url, RefCountedBytes** data) const;

  // For testing with a HistoryService mock.
  void SetMockHistoryService(MockHistoryService* mhs);

  // Start reading thumbnails from the ThumbnailDatabase.
  void StartMigration();

 private:
  friend class base::RefCountedThreadSafe<TopSites>;
  friend class TopSitesTest;
  FRIEND_TEST_ALL_PREFIXES(TopSitesTest, GetMostVisited);
  FRIEND_TEST_ALL_PREFIXES(TopSitesTest, RealDatabase);
  FRIEND_TEST_ALL_PREFIXES(TopSitesTest, MockDatabase);
  FRIEND_TEST_ALL_PREFIXES(TopSitesTest, DeleteNotifications);
  FRIEND_TEST_ALL_PREFIXES(TopSitesTest, GetUpdateDelay);
  FRIEND_TEST_ALL_PREFIXES(TopSitesTest, MigrationTest);
  FRIEND_TEST_ALL_PREFIXES(TopSitesTest, QueueingRequestsForTopSites);
  FRIEND_TEST_ALL_PREFIXES(TopSitesTest, CancelingRequestsForTopSites);
  FRIEND_TEST_ALL_PREFIXES(TopSitesTest, AddTemporaryThumbnail);

  ~TopSites();

  // Sets the thumbnail without writing to the database. Useful when
  // reading last known top sites from the DB.
  // Returns true if the thumbnail was set, false if the existing one is better.
  bool SetPageThumbnailNoDB(const GURL& url,
                            const RefCountedBytes* thumbnail_data,
                            const ThumbnailScore& score);

  // A version of SetPageThumbnail that takes RefCountedBytes as
  // returned by HistoryService.
  bool SetPageThumbnail(const GURL& url,
                        const RefCountedBytes* thumbnail,
                        const ThumbnailScore& score);

  // Query history service for the list of available thumbnails.
  void StartQueryForMostVisited();

  // Query history service for the thumbnail for a given url. |index|
  // is the index into top_sites_.
  void StartQueryForThumbnail(size_t index);

  // Called when history service returns a list of top URLs.
  void OnTopSitesAvailable(CancelableRequestProvider::Handle handle,
                           MostVisitedURLList data);

  // Called when history service returns a thumbnail.
  void OnThumbnailAvailable(CancelableRequestProvider::Handle handle,
                            scoped_refptr<RefCountedBytes> thumbnail);

  // Saves the set of the top URLs visited by this user. The 0th item is the
  // most popular.
  // DANGER! This will clear all data from the input argument.
  void StoreMostVisited(MostVisitedURLList* most_visited);

  // Saves the given set of redirects. The redirects are in order of the
  // given vector, so [0] -> [1] -> [2].
  void StoreRedirectChain(const RedirectList& redirects,
                          size_t destination);

  // Each item in the most visited view can redirect elsewhere. This returns
  // the canonical URL one identifying the site if the given URL does appear
  // in the "top sites" list.
  //
  // If the given URL is not in the top sites, this will return an empty GURL.
  GURL GetCanonicalURL(const GURL& url) const;

  // Finds the given URL in the redirect chain for the given TopSite, and
  // returns the distance from the destination in hops that the given URL is.
  // The URL is assumed to be in the list. The destination is 0.
  static int GetRedirectDistanceForURL(const MostVisitedURL& most_visited,
                                       const GURL& url);

  // Generates the diff of things that happened between "old" and "new."
  //
  // The URLs that are in "new" but not "old" will be have their index into
  // "new" put in |added_urls|. The URLs that are in "old" but not "new" will
  // have their index into "old" put into |deleted_urls|.
  //
  // URLs appearing in both old and new lists but having different indices will
  // have their index into "new" be put into |moved_urls|.
  static void DiffMostVisited(const MostVisitedURLList& old_list,
                              const MostVisitedURLList& new_list,
                              std::vector<size_t>* added_urls,
                              std::vector<size_t>* deleted_urls,
                              std::vector<size_t>* moved_urls);

  // Implementation of NotificationObserver.
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

  // Returns the delay until the next update of history is needed.
  // Uses num_urls_changed
  base::TimeDelta GetUpdateDelay();

  // The following methods must be run on the DB thread since they
  // access the database.

  // Reads the database from disk. Called on startup to get the last
  // known top sites.
  void ReadDatabase();

  // Write a thumbnail to database.
  void WriteThumbnailToDB(const MostVisitedURL& url,
                          int url_rank,
                          const TopSites::Images& thumbnail);

  // Updates the top sites list and writes the difference to disk.
  void UpdateMostVisited(MostVisitedURLList most_visited);

  // Deletes the database file, then reinitializes the database.
  void ResetDatabase();

  // Called after TopSites completes migration.
  void OnMigrationDone();

  // Add a thumbnail for an unknown url. See temp_thumbnails_map_.
  void AddTemporaryThumbnail(const GURL& url,
                             const RefCountedBytes* thumbnail,
                             const ThumbnailScore& score);

  Profile* profile_;
  // A mockup to use for testing. If NULL, use the real HistoryService
  // from the profile_. See SetMockHistoryService.
  MockHistoryService* mock_history_service_;
  CancelableRequestConsumerTSimple<size_t> cancelable_consumer_;
  mutable Lock lock_;

  // The cached version of the top sites. The 0th item in this vector is the
  // #1 site.
  MostVisitedURLList top_sites_;

  // The images corresponding to the top_sites. This is indexed by the URL of
  // the top site, so this doesn't have to be shuffled around when the ordering
  // changes of the top sites. Some top_sites_ entries may not have images.
  std::map<GURL, Images> top_images_;

  // Generated from the redirects to and from the most visited pages, this
  // maps the redirects to the index into top_sites_ that contains it.
  std::map<GURL, size_t> canonical_urls_;

  // Timer for updating TopSites data.
  base::OneShotTimer<TopSites> timer_;

  scoped_ptr<TopSitesDatabase> db_;
  FilePath db_path_;

  NotificationRegistrar registrar_;

  // The number of URLs changed on the last update.
  size_t last_num_urls_changed_;

  // Are we in the middle of migration from ThumbnailsDatabase to
  // TopSites?
  bool migration_in_progress_;

  // URLs for which we are expecting thumbnails.
  std::set<GURL> migration_pending_urls_;

  // The map of requests for the top sites list. Can only be
  // non-empty at startup. After we read the top sites from the DB, we'll
  // always have a cached list.
  typedef std::set<scoped_refptr<CancelableRequest<GetTopSitesCallback> > >
      PendingCallbackSet;
  PendingCallbackSet pending_callbacks_;

  // Are we waiting for the top sites from HistoryService?
  bool waiting_for_results_;

  // Stores thumbnails for unknown pages. When SetPageThumbnail is
  // called, if we don't know about that URL yet and we don't have
  // enough Top Sites (new profile), we store it until the next
  // UpdateMostVisitedURLs call.
  std::map<GURL, Images> temp_thumbnails_map_;

  // TODO(brettw): use the blacklist.
  // std::set<GURL> blacklist_;

  DISALLOW_COPY_AND_ASSIGN(TopSites);
};

}  // namespace history

#endif  // CHROME_BROWSER_HISTORY_TOP_SITES_H_
