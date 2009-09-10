// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_ENGINE_SYNCER_COMMAND_H_
#define CHROME_BROWSER_SYNC_ENGINE_SYNCER_COMMAND_H_

#include "base/basictypes.h"

namespace browser_sync {

class SyncerSession;

// Implementation of a simple command pattern intended to be driven by the
// Syncer.  SyncerCommand is abstract and all subclasses must
// implement ExecuteImpl().  This is done so that chunks of syncer operation
// can be unit tested.
//
// Example Usage:
//
// SyncerSession session = ...;
// SyncerCommand *cmd = SomeCommandFactory.createCommand(...);
// cmd->Execute(session);
// delete cmd;
//

class SyncerCommand {
 public:
  SyncerCommand();
  virtual ~SyncerCommand();

  // Execute dispatches to a derived class's ExecuteImpl.
  void Execute(SyncerSession *session);

  // ExecuteImpl is where derived classes actually do work.
  virtual void ExecuteImpl(SyncerSession *session) = 0;
 private:
  void SendNotifications(SyncerSession *session);
  DISALLOW_COPY_AND_ASSIGN(SyncerCommand);
};

}  // namespace browser_sync

#endif  // CHROME_BROWSER_SYNC_ENGINE_SYNCER_COMMAND_H_
