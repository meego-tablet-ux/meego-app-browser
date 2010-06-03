// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_PASSIVE_LOG_COLLECTOR_H_
#define CHROME_BROWSER_NET_PASSIVE_LOG_COLLECTOR_H_

#include <deque>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/hash_tables.h"
#include "base/ref_counted.h"
#include "base/time.h"
#include "chrome/browser/net/chrome_net_log.h"
#include "net/base/net_log.h"

// PassiveLogCollector watches the NetLog event stream, and saves the network
// event for recent requests, in a circular buffer.
//
// This is done so that when a network problem is encountered (performance
// problem, or error), about:net-internals can be opened shortly after the
// problem and it will contain a trace for the problem request.
//
// (This is in contrast to the "active logging" which captures every single
// network event, but requires capturing to have been enabled *prior* to
// encountering the problem. Active capturing is enabled as long as
// about:net-internals is open).
//
// The data captured by PassiveLogCollector is grouped by NetLog::Source, into
// a RequestInfo structure. These in turn are grouped by NetLog::SourceType, and
// owned by a RequestTrackerBase instance for the specific source type.
class PassiveLogCollector : public ChromeNetLog::Observer {
 public:
  // This structure encapsulates all of the parameters of a captured event,
  // including an "order" field that identifies when it was captured relative
  // to other events.
  struct Entry {
    Entry(uint32 order,
          net::NetLog::EventType type,
          const base::TimeTicks& time,
          net::NetLog::Source source,
          net::NetLog::EventPhase phase,
          net::NetLog::EventParameters* params)
        : order(order), type(type), time(time), source(source), phase(phase),
          params(params) {
    }

    uint32 order;
    net::NetLog::EventType type;
    base::TimeTicks time;
    net::NetLog::Source source;
    net::NetLog::EventPhase phase;
    scoped_refptr<net::NetLog::EventParameters> params;
  };

  typedef std::vector<Entry> EntryList;
  typedef std::vector<net::NetLog::Source> SourceDependencyList;

  // TODO(eroman): Rename to SourceInfo.
  struct RequestInfo {
    RequestInfo()
        : source_id(net::NetLog::Source::kInvalidId),
          num_entries_truncated(0), reference_count(0), is_alive(true) {}

    // Returns the URL that corresponds with this source. This is
    // only meaningful for certain source types (URL_REQUEST, SOCKET_STREAM).
    // For the rest, it will return an empty string.
    std::string GetURL() const;

    uint32 source_id;
    EntryList entries;
    size_t num_entries_truncated;

    // List of other sources which contain information relevant to this
    // request (for example, a url request might depend on the log items
    // for a connect job and for a socket that were bound to it.)
    SourceDependencyList dependencies;

    // Holds the count of how many other sources have added this as a
    // dependent source. When it is 0, it means noone has referenced it so it
    // can be deleted normally.
    int reference_count;

    // |is_alive| is set to false once the request has been added to the
    // tracker's graveyard (it may still be kept around due to a non-zero
    // reference_count, but it is still considered "dead").
    bool is_alive;
  };

  typedef std::vector<RequestInfo> RequestInfoList;

  // This class stores and manages the passively logged information for
  // URLRequests/SocketStreams/ConnectJobs.
  class RequestTrackerBase {
   public:
    RequestTrackerBase(size_t max_graveyard_size, PassiveLogCollector* parent);

    virtual ~RequestTrackerBase();

    void OnAddEntry(const Entry& entry);

    // Clears all the passively logged data from this tracker.
    void Clear();

    // Appends all the captured entries to |out|. The ordering is undefined.
    void AppendAllEntries(EntryList* out) const;

#ifdef UNIT_TEST
    // Helper used to inspect the current state by unit-tests.
    // Retuns a copy of the requests held by the tracker.
    RequestInfoList GetAllDeadOrAliveRequests(bool is_alive) const {
      RequestInfoList result;
      for (SourceIDToInfoMap::const_iterator it = requests_.begin();
           it != requests_.end(); ++it) {
        if (it->second.is_alive == is_alive)
          result.push_back(it->second);
      }
      return result;
    }
#endif

   protected:
    enum Action {
      ACTION_NONE,
      ACTION_DELETE,
      ACTION_MOVE_TO_GRAVEYARD,
    };

    // Makes |info| hold a reference to |source|. This way |source| will be
    // kept alive at least as long as |info|.
    void AddReferenceToSourceDependency(const net::NetLog::Source& source,
                                        RequestInfo* info);

   private:
    typedef base::hash_map<uint32, RequestInfo> SourceIDToInfoMap;
    typedef std::deque<uint32> DeletionQueue;

    // Updates |out_info| with the information from |entry|. Returns an action
    // to perform for this map entry on completion.
    virtual Action DoAddEntry(const Entry& entry, RequestInfo* out_info) = 0;

    // Removes |source_id| from |requests_|. This also releases any references
    // to dependencies held by this source.
    void DeleteRequestInfo(uint32 source_id);

    // Adds |source_id| to the FIFO queue (graveyard) for deletion.
    void AddToDeletionQueue(uint32 source_id);

    // Adds/Releases a reference from the source with ID |source_id|.
    // Use |offset=-1| to do a release, and |offset=1| for an addref.
    void AdjustReferenceCountForSource(int offset, uint32 source_id);

    // Releases all the references to sources held by |info|.
    void ReleaseAllReferencesToDependencies(RequestInfo* info);

    // This map contains all of the requests being tracked by this tracker.
    // (It includes both the "live" requests, and the "dead" ones.)
    SourceIDToInfoMap requests_;

    size_t max_graveyard_size_;

    // FIFO queue for entries in |requests_| that are no longer alive, and
    // can be deleted. This buffer is also called "graveyard" elsewhere. We
    // queue requests for deletion so they can persist a bit longer.
    DeletionQueue deletion_queue_;

    PassiveLogCollector* parent_;

    DISALLOW_COPY_AND_ASSIGN(RequestTrackerBase);
  };

  // Specialization of RequestTrackerBase for handling ConnectJobs.
  class ConnectJobTracker : public RequestTrackerBase {
   public:
    static const size_t kMaxGraveyardSize;

    explicit ConnectJobTracker(PassiveLogCollector* parent);

   protected:
    virtual Action DoAddEntry(const Entry& entry, RequestInfo* out_info);
   private:
    DISALLOW_COPY_AND_ASSIGN(ConnectJobTracker);
  };

  // Specialization of RequestTrackerBase for handling Sockets.
  class SocketTracker : public RequestTrackerBase {
   public:
    static const size_t kMaxGraveyardSize;

    SocketTracker();

   protected:
    virtual Action DoAddEntry(const Entry& entry, RequestInfo* out_info);

   private:
    DISALLOW_COPY_AND_ASSIGN(SocketTracker);
  };

  // Specialization of RequestTrackerBase for handling URLRequest/SocketStream.
  class RequestTracker : public RequestTrackerBase {
   public:
    static const size_t kMaxGraveyardSize;

    explicit RequestTracker(PassiveLogCollector* parent);

   protected:
    virtual Action DoAddEntry(const Entry& entry, RequestInfo* out_info);

   private:
    DISALLOW_COPY_AND_ASSIGN(RequestTracker);
  };

  // Specialization of RequestTrackerBase for handling
  // SOURCE_INIT_PROXY_RESOLVER.
  class InitProxyResolverTracker : public RequestTrackerBase {
   public:
    static const size_t kMaxGraveyardSize;

    InitProxyResolverTracker();

   protected:
    virtual Action DoAddEntry(const Entry& entry, RequestInfo* out_info);

   private:
    DISALLOW_COPY_AND_ASSIGN(InitProxyResolverTracker);
  };

  // Tracks the log entries for the last seen SOURCE_SPDY_SESSION.
  class SpdySessionTracker : public RequestTrackerBase {
   public:
    static const size_t kMaxGraveyardSize;

    SpdySessionTracker();

   protected:
    virtual Action DoAddEntry(const Entry& entry, RequestInfo* out_info);

   private:
    DISALLOW_COPY_AND_ASSIGN(SpdySessionTracker);
  };

  PassiveLogCollector();
  ~PassiveLogCollector();

  // Observer implementation:
  virtual void OnAddEntry(net::NetLog::EventType type,
                          const base::TimeTicks& time,
                          const net::NetLog::Source& source,
                          net::NetLog::EventPhase phase,
                          net::NetLog::EventParameters* params);

  // Returns the tracker to use for sources of type |source_type|, or NULL.
  RequestTrackerBase* GetTrackerForSourceType(
      net::NetLog::SourceType source_type);

  // Clears all of the passively logged data.
  void Clear();

  // Fills |out| with the full list of events that have been passively
  // captured. The list is ordered by capture time.
  void GetAllCapturedEvents(EntryList* out) const;

 private:
  FRIEND_TEST_ALL_PREFIXES(PassiveLogCollectorTest,
                           HoldReferenceToDependentSource);

  ConnectJobTracker connect_job_tracker_;
  SocketTracker socket_tracker_;
  RequestTracker url_request_tracker_;
  RequestTracker socket_stream_tracker_;
  InitProxyResolverTracker init_proxy_resolver_tracker_;
  SpdySessionTracker spdy_session_tracker_;

  // This array maps each NetLog::SourceType to one of the tracker instances
  // defined above. Use of this array avoid duplicating the list of trackers
  // elsewhere.
  RequestTrackerBase* trackers_[net::NetLog::SOURCE_COUNT];

  // The count of how many events have flowed through this log. Used to set the
  // "order" field on captured events.
  uint32 num_events_seen_;

  DISALLOW_COPY_AND_ASSIGN(PassiveLogCollector);
};

#endif  // CHROME_BROWSER_NET_PASSIVE_LOG_COLLECTOR_H_
