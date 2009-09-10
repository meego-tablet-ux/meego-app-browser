// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// An UpdateApplicator is used to iterate over a number of unapplied
// updates, applying them to the client using the given syncer session.
//
// UpdateApplicator might resemble an iterator, but it actually keeps retrying
// failed updates until no remaining updates can be successfully applied.

#ifndef CHROME_BROWSER_SYNC_ENGINE_UPDATE_APPLICATOR_H_
#define CHROME_BROWSER_SYNC_ENGINE_UPDATE_APPLICATOR_H_

#include <vector>
#include <set>

#include "base/basictypes.h"
#include "base/port.h"

namespace syncable {
class Id;
class WriteTransaction;
}  // namespace syncable

namespace browser_sync {

class SyncerSession;

class UpdateApplicator {
 public:
  typedef std::vector<int64>::iterator vi64iter;

  UpdateApplicator(SyncerSession* session, const vi64iter& begin,
                   const vi64iter& end);
  // returns true if there's more we can do.
  bool AttemptOneApplication(syncable::WriteTransaction* trans);
  // return true if we've applied all updates.
  bool AllUpdatesApplied() const;

  // This class does not automatically save its progress into the
  // SyncerSession -- to get that to happen, call this method after
  // update application is finished (i.e., when AttemptOneAllocation
  // stops returning true).
  void SaveProgressIntoSessionState();

 private:
  SyncerSession* const session_;
  vi64iter const begin_;
  vi64iter end_;
  vi64iter pointer_;
  bool progress_;

  // Track the result of the various items.
  std::vector<syncable::Id> conflicting_ids_;
  std::vector<syncable::Id> blocked_ids_;
  std::vector<syncable::Id> successful_ids_;
};

}  // namespace browser_sync

#endif  // CHROME_BROWSER_SYNC_ENGINE_UPDATE_APPLICATOR_H_
