// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_BASE_NET_LOG_H_
#define NET_BASE_NET_LOG_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/ref_counted.h"
#include "base/scoped_ptr.h"
#include "base/time.h"
#include "net/base/net_log.h"

namespace net {

// NetLog is the destination for log messages generated by the network stack.
// Each log message has a "source" field which identifies the specific entity
// that generated the message (for example, which URLRequest or which
// SocketStream).
//
// To avoid needing to pass in the "source id" to the logging functions, NetLog
// is usually accessed through a BoundNetLog, which will always pass in a
// specific source ID.
//
// Note that NetLog is NOT THREADSAFE.
//
// ******** The NetLog (and associated logging) is a work in progress ********
//
// TODO(eroman): Remove the 'const' qualitifer from the BoundNetLog methods.
// TODO(eroman): Remove the AddString() and AddStringLiteral() methods.
//               These are a carry-over from old approach. Really, consumers
//               should be calling AddEventWithParameters(), and passing a
//               custom EventParameters* object that encapsulates all of the
//               interesting state.
// TODO(eroman): Remove NetLogUtil. Pretty printing should only be done from
//               javascript, and should be very context-aware.
// TODO(eroman): Move Capturing*NetLog to its own file. (And eventually remove
//               all the consumers of it).
// TODO(eroman): Make the DNS jobs emit directly into the NetLog.
// TODO(eroman): Start a new Source each time URLRequest redirects
//               (simpler to reason about each as a separate entity).
// TODO(eroman): Add the URLRequest load flags to the start entry.

class NetLog {
 public:
  enum EventType {
#define EVENT_TYPE(label) TYPE_ ## label,
#include "net/base/net_log_event_type_list.h"
#undef EVENT_TYPE
  };

  // The 'phase' of an event trace (whether it marks the beginning or end
  // of an event.).
  enum EventPhase {
    PHASE_NONE,
    PHASE_BEGIN,
    PHASE_END,
  };

  // The "source" identifies the entity that generated the log message.
  enum SourceType {
    SOURCE_NONE,
    SOURCE_URL_REQUEST,
    SOURCE_SOCKET_STREAM,
    SOURCE_INIT_PROXY_RESOLVER,
    SOURCE_CONNECT_JOB,
    SOURCE_SOCKET,
  };

  // Identifies the entity that generated this log. The |id| field should
  // uniquely identify the source, and is used by log observers to infer
  // message groupings. Can use NetLog::NextID() to create unique IDs.
  struct Source {
    static const uint32 kInvalidId = 0;

    Source() : type(SOURCE_NONE), id(kInvalidId) {}
    Source(SourceType type, uint32 id) : type(type), id(id) {}
    bool is_valid() { return id != kInvalidId; }

    SourceType type;
    uint32 id;
  };

  // Base class for associating additional parameters with an event. Log
  // observers need to know what specific derivations of EventParameters a
  // particular EventType uses, in order to get at the individual components.
  class EventParameters : public base::RefCounted<EventParameters> {
   public:
    EventParameters() {}
    virtual ~EventParameters() {}

    // Serializes the parameters to a string representation (this should be a
    // lossless conversion).
    virtual std::string ToString() const = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(EventParameters);
  };

  NetLog() {}
  virtual ~NetLog() {}

  // Emits an event to the log stream.
  //  |type| - The type of the event.
  //  |time| - The time when the event occurred.
  //  |source| - The source that generated the event.
  //  |phase| - An optional parameter indicating whether this is the start/end
  //            of an action.
  //  |extra_parameters| - Optional (may be NULL) parameters for this event.
  //                       The specific subclass of EventParameters is defined
  //                       by the contract for events of this |type|.
  virtual void AddEntry(EventType type,
                        const base::TimeTicks& time,
                        const Source& source,
                        EventPhase phase,
                        EventParameters* extra_parameters) = 0;

  // Returns a unique ID which can be used as a source ID.
  virtual uint32 NextID() = 0;

  // Returns true if more complicated messages should be sent to the log.
  // TODO(eroman): This is a carry-over from refactoring; figure out
  //               something better.
  virtual bool HasListener() const = 0;

  // Returns a C-String symbolic name for |event_type|.
  static const char* EventTypeToString(EventType event_type);

  // Returns a list of all the available EventTypes.
  static std::vector<EventType> GetAllEventTypes();

 private:
  DISALLOW_COPY_AND_ASSIGN(NetLog);
};

// Helper that binds a Source to a NetLog, and exposes convenience methods to
// output log messages without needing to pass in the source.
class BoundNetLog {
 public:
  BoundNetLog() : net_log_(NULL) {}

  // TODO(eroman): This is a complete hack to allow passing in NULL in
  // place of a BoundNetLog. I added this while refactoring to simplify the
  // task of updating all the callers.
  BoundNetLog(uint32) : net_log_(NULL) {}

  BoundNetLog(const NetLog::Source& source, NetLog* net_log)
      : source_(source), net_log_(net_log) {
  }

  void AddEntry(NetLog::EventType type,
                NetLog::EventPhase phase,
                NetLog::EventParameters* extra_parameters) const;

  void AddEntryWithTime(NetLog::EventType type,
                        const base::TimeTicks& time,
                        NetLog::EventPhase phase,
                        NetLog::EventParameters* extra_parameters) const;

  // Convenience methods that call through to the NetLog, passing in the
  // currently bound source.
  void AddEvent(NetLog::EventType event_type) const;
  void AddEventWithParameters(NetLog::EventType event_type,
                              NetLog::EventParameters* params) const;
  bool HasListener() const;
  void BeginEvent(NetLog::EventType event_type) const;
  void BeginEventWithParameters(NetLog::EventType event_type,
                                NetLog::EventParameters* params) const;
  void BeginEventWithString(NetLog::EventType event_type,
                            const std::string& string) const;
  void BeginEventWithInteger(NetLog::EventType event_type, int integer) const;
  void AddEventWithInteger(NetLog::EventType event_type, int integer) const;
  void EndEvent(NetLog::EventType event_type) const;
  void EndEventWithParameters(NetLog::EventType event_type,
                              NetLog::EventParameters* params) const;
  void EndEventWithInteger(NetLog::EventType event_type, int integer) const;

  // Deprecated: Don't add new dependencies that use these methods. Instead, use
  // AddEventWithParameters().
  void AddString(const std::string& string) const;
  void AddStringLiteral(const char* literal) const;

  // Helper to create a BoundNetLog given a NetLog and a SourceType. Takes care
  // of creating a unique source ID, and handles the case of NULL net_log.
  static BoundNetLog Make(NetLog* net_log, NetLog::SourceType source_type);

  const NetLog::Source& source() const { return source_; }
  NetLog* net_log() const { return net_log_; }

 private:
  NetLog::Source source_;
  NetLog* net_log_;
};

// NetLogStringParameter is a subclass of EventParameters that encapsulates a
// single std::string parameter.
class NetLogStringParameter : public NetLog::EventParameters {
 public:
  explicit NetLogStringParameter(const std::string& value);

  const std::string& value() const {
    return value_;
  }

  virtual std::string ToString() const {
    return value_;
  }

 private:
  std::string value_;
};

// NetLogIntegerParameter is a subclass of EventParameters that encapsulates a
// single integer parameter.
class NetLogIntegerParameter : public NetLog::EventParameters {
 public:
  explicit NetLogIntegerParameter(int value) : value_(value) {}

  int value() const {
    return value_;
  }

  virtual std::string ToString() const;

 private:
  const int value_;
};

// NetLogStringLiteralParameter is a subclass of EventParameters that
// encapsulates a single string literal parameter.
class NetLogStringLiteralParameter : public NetLog::EventParameters {
 public:
  explicit NetLogStringLiteralParameter(const char* value) : value_(value) {}

  const char* const value() const {
    return value_;
  }

  virtual std::string ToString() const;

 private:
  const char* const value_;
};


// CapturingNetLog is an implementation of NetLog that saves messages to a
// bounded buffer.
class CapturingNetLog : public NetLog {
 public:
  struct Entry {
    Entry(EventType type,
          const base::TimeTicks& time,
          Source source,
          EventPhase phase,
          EventParameters* extra_parameters)
        : type(type), time(time), source(source), phase(phase),
          extra_parameters(extra_parameters) {
    }

    EventType type;
    base::TimeTicks time;
    Source source;
    EventPhase phase;
    scoped_refptr<EventParameters> extra_parameters;
  };

  // Ordered set of entries that were logged.
  typedef std::vector<Entry> EntryList;

  enum { kUnbounded = -1 };

  // Creates a CapturingNetLog that logs a maximum of |max_num_entries|
  // messages.
  explicit CapturingNetLog(size_t max_num_entries)
      : next_id_(0), max_num_entries_(max_num_entries) {}

  // NetLog implementation:
  virtual void AddEntry(EventType type,
                        const base::TimeTicks& time,
                        const Source& source,
                        EventPhase phase,
                        EventParameters* extra_parameters);
  virtual uint32 NextID();
  virtual bool HasListener() const { return true; }

  // Returns the list of all entries in the log.
  const EntryList& entries() const { return entries_; }

  void Clear();

 private:
  uint32 next_id_;
  size_t max_num_entries_;
  EntryList entries_;

  DISALLOW_COPY_AND_ASSIGN(CapturingNetLog);
};

// Helper class that exposes a similar API as BoundNetLog, but uses a
// CapturingNetLog rather than the more generic NetLog.
//
// CapturingBoundNetLog can easily be converted to a BoundNetLog using the
// bound() method.
class CapturingBoundNetLog {
 public:
  CapturingBoundNetLog(const NetLog::Source& source, CapturingNetLog* net_log)
      : source_(source), capturing_net_log_(net_log) {
  }

  explicit CapturingBoundNetLog(size_t max_num_entries)
      : capturing_net_log_(new CapturingNetLog(max_num_entries)) {}

  // The returned BoundNetLog is only valid while |this| is alive.
  BoundNetLog bound() const {
    return BoundNetLog(source_, capturing_net_log_.get());
  }

  // Returns the list of all entries in the log.
  const CapturingNetLog::EntryList& entries() const {
    return capturing_net_log_->entries();
  }

  void Clear();

  // Sends all of captured messages to |net_log|, using the same source ID
  // as |net_log|.
  void AppendTo(const BoundNetLog& net_log) const;

 private:
  NetLog::Source source_;
  scoped_ptr<CapturingNetLog> capturing_net_log_;

  DISALLOW_COPY_AND_ASSIGN(CapturingBoundNetLog);
};

}  // namespace net

#endif  // NET_BASE_NET_LOG_H_
