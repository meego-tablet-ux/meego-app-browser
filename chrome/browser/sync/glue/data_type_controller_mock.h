// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_GLUE_DATA_TYPE_CONTROLLER_MOCK_H__
#define CHROME_BROWSER_SYNC_GLUE_DATA_TYPE_CONTROLLER_MOCK_H__

#include "chrome/browser/sync/glue/data_type_controller.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace browser_sync {

class DataTypeControllerMock : public DataTypeController {
 public:
  MOCK_METHOD2(Start, void(bool merge_allowed, StartCallback* start_callback));
  MOCK_METHOD0(Stop, void());
  MOCK_METHOD0(enabled, bool());
  MOCK_METHOD0(type, syncable::ModelType());
  MOCK_METHOD0(model_safe_group, browser_sync::ModelSafeGroup());
  MOCK_METHOD0(state, State());
};

}  // namespace browser_sync

#endif  // CHROME_BROWSER_SYNC_GLUE_DATA_TYPE_CONTROLLER_MOCK_H__
