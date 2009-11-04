// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_FLIP_FLIP_SESSION_POOL_H_
#define NET_FLIP_FLIP_SESSION_POOL_H_

#include <map>
#include <list>
#include <string>

#include "base/ref_counted.h"
#include "base/scoped_ptr.h"
#include "net/base/host_resolver.h"

namespace net {

class ClientSocket;
class FlipSession;
class HttpNetworkSession;

// This is a very simple pool for open FlipSessions.
// TODO(mbelshe): Make this production ready.
class FlipSessionPool : public base::RefCounted<FlipSessionPool> {
 public:
  FlipSessionPool();
  virtual ~FlipSessionPool();

  // Either returns an existing FlipSession or creates a new FlipSession for
  // use.
  FlipSession* Get(const HostResolver::RequestInfo& info,
                   HttpNetworkSession* session);

  // Builds a FlipSession from an existing socket.
  FlipSession* GetFlipSessionFromSocket(
      const HostResolver::RequestInfo& info,
      HttpNetworkSession* session,
      ClientSocket* socket) {
    // TODO(willchan): Implement this to allow a HttpNetworkTransaction to
    // upgrade a TCP connection from HTTP to FLIP.
    return NULL;
  }

  // TODO(willchan): Consider renaming to HasReusableSession, since perhaps we
  // should be creating a new session.
  bool HasSession(const HostResolver::RequestInfo& info) const;

  // Close all Flip Sessions; used for debugging.
  void CloseAllSessions();

 private:
  friend class FlipSession;  // Needed for Remove().

  typedef std::list<FlipSession*> FlipSessionList;
  typedef std::map<std::string, FlipSessionList*> FlipSessionsMap;

  // Return a FlipSession to the pool.
  void Remove(FlipSession* session);

  // Helper functions for manipulating the lists.
  FlipSessionList* AddSessionList(const std::string& domain);
  FlipSessionList* GetSessionList(const std::string& domain);
  const FlipSessionList* GetSessionList(const std::string& domain) const;
  void RemoveSessionList(const std::string& domain);

  // This is our weak session pool - one session per domain.
  FlipSessionsMap sessions_;

  DISALLOW_COPY_AND_ASSIGN(FlipSessionPool);
};

}  // namespace net

#endif  // NET_FLIP_FLIP_SESSION_POOL_H_
