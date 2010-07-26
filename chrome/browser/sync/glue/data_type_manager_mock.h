// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_GLUE_DATA_TYPE_MANAGER_MOCK_H__
#define CHROME_BROWSER_SYNC_GLUE_DATA_TYPE_MANAGER_MOCK_H__
#pragma once

#include "chrome/browser/sync/glue/data_type_manager.h"
#include "chrome/browser/sync/profile_sync_test_util.h"
#include "chrome/common/notification_details.h"
#include "chrome/common/notification_service.h"
#include "chrome/common/notification_type.h"
#include "testing/gmock/include/gmock/gmock.h"

ACTION_P2(NotifyWithResult, type, result) {
  NotificationService::current()->Notify(
      type,
      NotificationService::AllSources(),
      Details<browser_sync::DataTypeManager::ConfigureResult>(result));
}

namespace browser_sync {

class DataTypeManagerMock : public DataTypeManager {
 public:
  DataTypeManagerMock() : result_(OK) {
    // By default, calling Configure will send a SYNC_CONFIGURE_START
    // and SYNC_CONFIGURE_DONE notification with a DataTypeManager::OK
    // detail.
    ON_CALL(*this, Configure(testing::_)).
        WillByDefault(testing::DoAll(
            Notify(NotificationType::SYNC_CONFIGURE_START),
            NotifyWithResult(NotificationType::SYNC_CONFIGURE_DONE,
                             &result_)));
  }

  MOCK_METHOD1(Configure, void(const TypeSet&));
  MOCK_METHOD0(Stop, void());
  MOCK_METHOD0(controllers, const DataTypeController::TypeMap&());
  MOCK_METHOD0(state, State());

 private:
  DataTypeManager::ConfigureResult result_;
};

}  // namespace browser_sync

#endif  // CHROME_BROWSER_SYNC_GLUE_DATA_TYPE_MANAGER_MOCK_H__
