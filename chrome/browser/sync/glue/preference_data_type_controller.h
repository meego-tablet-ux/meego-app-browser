// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_GLUE_PREFERENCE_DATA_TYPE_CONTROLLER_H__
#define CHROME_BROWSER_SYNC_GLUE_PREFERENCE_DATA_TYPE_CONTROLLER_H__
#pragma once

#include <string>

#include "chrome/browser/sync/glue/frontend_data_type_controller.h"

namespace browser_sync {

class PreferenceDataTypeController : public FrontendDataTypeController {
 public:
  PreferenceDataTypeController(
      ProfileSyncFactory* profile_sync_factory,
      Profile* profile,
      ProfileSyncService* sync_service);
  virtual ~PreferenceDataTypeController();

  // FrontendDataTypeController implementation.
  virtual syncable::ModelType type() const;

 private:
  // FrontendDataTypeController implementations.
  virtual void CreateSyncComponents();
  virtual void RecordUnrecoverableError(
      const tracked_objects::Location& from_here,
      const std::string& message);
  virtual void RecordAssociationTime(base::TimeDelta time);
  virtual void RecordStartFailure(StartResult result);

  DISALLOW_COPY_AND_ASSIGN(PreferenceDataTypeController);
};

}  // namespace browser_sync

#endif  // CHROME_BROWSER_SYNC_GLUE_PREFERENCE_DATA_TYPE_CONTROLLER_H__
