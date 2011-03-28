// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_dldm_api.h"

#include "base/callback.h"
#include "base/json/json_writer.h"
#include "base/message_loop.h"
#include "base/string_number_conversions.h"
#include "base/task.h"
#include "base/values.h"
#include "chrome/browser/extensions/extension_message_service.h"

ExtensionDldmEventRouter* ExtensionDldmEventRouter::GetInstance() {
  return Singleton<ExtensionDldmEventRouter>::get();
}

void ExtensionDldmEventRouter::ModelChanged() {

  DLOG(INFO) << "download model changed.";
  download_manager_->GetCurrentDownloads(FilePath(), &download_items_);

  for (std::vector<DownloadItem*>::iterator it = download_items_.begin();
       it != download_items_.end(); ++it) {

    DownloadItem* download = *it;
    download->RemoveObserver(this);
    download->AddObserver(this);
    DLOG(INFO) << "   ITEM " << download->url().spec();

  }

}

void ExtensionDldmEventRouter::OnDownloadFileCompleted(DownloadItem* download) {
  if (!download->NeedsRename())
    DldmFinished(profile_, download);
}

void ExtensionDldmEventRouter::DldmFinished(
    Profile* profile,
    DownloadItem* download) {
  ListValue args;
  args.Append(new StringValue(download->url().spec()));
  args.Append(Value::CreateBooleanValue(download->state() == DownloadItem::COMPLETE));
  args.Append(new StringValue(download->full_path().value()));

  std::string json_args;
  base::JSONWriter::Write(&args, false, &json_args);
  DispatchEvent(profile, "dldm.onDownload", json_args);
}

void ExtensionDldmEventRouter::DispatchEvent(Profile* profile,
                                             const char* event_name,
                                             const std::string& json_args) {
  if (profile && profile->GetExtensionMessageService()) {
    // TODO, API changed
/*
    profile->GetExtensionMessageService()->DispatchEventToRenderers(
        event_name, json_args, profile, GURL());
*/
  }
}

void DldmFunction::Run() {
  if (!RunImpl()) {
    SendResponse(false);
  }
}

bool UpdateUIDldmFunction::RunImpl() {
  string16 query;
  EXTENSION_FUNCTION_VALIDATE(args_->GetString(0, &query));
  std::vector<DownloadItem*> items;
  ExtensionDldmEventRouter* router = ExtensionDldmEventRouter::GetInstance();
  router->download_manager()->SearchDownloads(query, &items);
  if (items.size()) {
    DownloadItem* item = items[0];
    item->UpdateObservers();
  }

  SendResponse(true);
  return true;
}

