// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_ENGINE_BUILD_AND_PROCESS_CONFLICT_SETS_COMMAND_H_
#define CHROME_BROWSER_SYNC_ENGINE_BUILD_AND_PROCESS_CONFLICT_SETS_COMMAND_H_

#include <vector>

#include "base/basictypes.h"
#include "chrome/browser/sync/engine/model_changing_syncer_command.h"

namespace syncable {
class BaseTransaction;
class Entry;
class Id;
class MutableEntry;
class WriteTransaction;
}  // namespace syncable

namespace browser_sync {

class ConflictResolver;

namespace sessions {
class ConflictProgress;
class StatusController;
}

class BuildAndProcessConflictSetsCommand : public ModelChangingSyncerCommand {
 public:
  BuildAndProcessConflictSetsCommand();
  virtual ~BuildAndProcessConflictSetsCommand();

  // ModelChangingSyncerCommand implementation.
  virtual void ModelChangingExecuteImpl(sessions::SyncSession* session);

 private:
  bool BuildAndProcessConflictSets(sessions::SyncSession* session);

  bool ProcessSingleDirectionConflictSets(
      syncable::WriteTransaction* trans, ConflictResolver* resolver,
      sessions::StatusController* status);
  bool ApplyUpdatesTransactionally(
      syncable::WriteTransaction* trans,
      const std::vector<syncable::Id>* const update_set,
      ConflictResolver* resolver,
      sessions::StatusController* status);
  void BuildConflictSets(syncable::BaseTransaction* trans,
                         sessions::ConflictProgress* conflict_progress);

  void MergeSetsForNameClash(syncable::BaseTransaction* trans,
                             syncable::Entry* entry,
                             sessions::ConflictProgress* conflict_progress);
  void MergeSetsForIntroducedLoops(syncable::BaseTransaction* trans,
      syncable::Entry* entry,
      sessions::ConflictProgress* conflict_progress);
  void MergeSetsForNonEmptyDirectories(syncable::BaseTransaction* trans,
      syncable::Entry* entry,
      sessions::ConflictProgress* conflict_progress);
  void MergeSetsForPositionUpdate(syncable::BaseTransaction* trans,
      syncable::Entry* entry,
      sessions::ConflictProgress* conflict_progress);

  DISALLOW_COPY_AND_ASSIGN(BuildAndProcessConflictSetsCommand);
};

}  // namespace browser_sync

#endif  // CHROME_BROWSER_SYNC_ENGINE_BUILD_AND_PROCESS_CONFLICT_SETS_COMMAND_H_
