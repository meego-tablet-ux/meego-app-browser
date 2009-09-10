// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/engine/download_updates_command.h"

#include <string>

#include "chrome/browser/sync/engine/syncer.h"
#include "chrome/browser/sync/engine/syncer_proto_util.h"
#include "chrome/browser/sync/engine/syncproto.h"
#include "chrome/browser/sync/syncable/directory_manager.h"
#include "chrome/browser/sync/util/character_set_converters.h"
#include "chrome/browser/sync/util/sync_types.h"

using syncable::ScopedDirLookup;

namespace browser_sync {

using std::string;

DownloadUpdatesCommand::DownloadUpdatesCommand() {}
DownloadUpdatesCommand::~DownloadUpdatesCommand() {}

void DownloadUpdatesCommand::ExecuteImpl(SyncerSession *session) {
  ClientToServerMessage client_to_server_message;
  ClientToServerResponse update_response;

  client_to_server_message.set_share(
      static_cast<const string&>(ToUTF8(session->account_name())));
  client_to_server_message.set_message_contents(
      ClientToServerMessage::GET_UPDATES);
  GetUpdatesMessage* get_updates =
      client_to_server_message.mutable_get_updates();

  ScopedDirLookup dir(session->dirman(), session->account_name());
  if (!dir.good()) {
    LOG(ERROR) << "Scoped dir lookup failed!";
    return;
  }
  LOG(INFO) << "Getting updates from ts " << dir->last_sync_timestamp();
  get_updates->set_from_timestamp(dir->last_sync_timestamp());

  // Set GetUpdatesMessage.GetUpdatesCallerInfo information.
  get_updates->mutable_caller_info()->set_source(session->TestAndSetSource());
  get_updates->mutable_caller_info()->set_notifications_enabled(
      session->notifications_enabled());

  bool ok = SyncerProtoUtil::PostClientToServerMessage(
      &client_to_server_message,
      &update_response,
      session);

  if (!ok) {
    SyncerStatus status(session);
    status.increment_consecutive_problem_get_updates();
    status.increment_consecutive_errors();
    LOG(ERROR) << "PostClientToServerMessage() failed";
    return;
  }
  session->set_update_response(update_response);
}

}  // namespace browser_sync
