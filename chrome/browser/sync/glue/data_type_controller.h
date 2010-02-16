// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_GLUE_DATA_TYPE_CONTROLLER_H__
#define CHROME_BROWSER_SYNC_GLUE_DATA_TYPE_CONTROLLER_H__

#include "base/task.h"
#include "chrome/browser/sync/engine/model_safe_worker.h"
#include "chrome/browser/sync/syncable/model_type.h"

namespace browser_sync {

class DataTypeController {
 public:
  enum State {
    NOT_RUNNING,    // The controller has never been started or has
                    // previously been stopped.  Must be in this state to start.
    MODEL_STARTING, // The controller is waiting on dependent services
                    // that need to be available before model
                    // association.
    ASSOCIATING,    // Model association is in progress.
    RUNNING,        // The controller is running and the data type is
                    // in sync with the cloud.
    STOPPING        // The controller is in the process of stopping
                    // and is waiting for dependent services to stop.
  };

  enum StartResult {
    OK,                 // The data type has started normally.
    OK_FIRST_RUN,       // Same as OK, but sent on first successful
                        // start for this type for this user as
                        // determined by cloud state.
    BUSY,               // Start() was called while already in progress.
    NOT_ENABLED,        // This data type is not enabled for the current user.
    NEEDS_MERGE,        // Can't start without explicit permission to
                        // perform a data merge.  Re-starting with
                        // merge_allowed = true will allow this data
                        // type to start.
    ASSOCIATION_FAILED, // An error occurred during model association.
    ABORTED             // Start was aborted by calling Stop().
  };

  typedef Callback1<StartResult>::Type StartCallback;

  virtual ~DataTypeController() {}

  // Begins asynchronous start up of this data type.  Start up will
  // wait for all other dependent services to be available, then
  // proceed with model association and then change processor
  // activation.  Upon completion, the start_callback will be invoked
  // on the UI thread.  The merge_allowed parameter gives the data
  // type permission to perform a data merge at start time.  See the
  // StartResult enum above for details on the possible start results.
  virtual void Start(bool merge_allowed, StartCallback* start_callback) = 0;

  // Synchronously stops the data type.  If called after Start() is
  // called but before the start callback is called, the start is
  // aborted and the start callback is invoked with the ABORTED start
  // result.
  virtual void Stop() = 0;

  // Returns true if the user has indicated that they want this data
  // type to be enabled.
  virtual bool enabled() = 0;

  // Unique model type for this data type controller.
  virtual syncable::ModelType type() = 0;

  // The model safe group of this data type.  This should reflect the
  // thread that should be used to modify the data type's native
  // model.
  virtual browser_sync::ModelSafeGroup model_safe_group() = 0;

  // Current state of the data type controller.
  virtual State state() = 0;
};

}  // namespace browser_sync

#endif  // CHROME_BROWSER_SYNC_GLUE_DATA_TYPE_CONTROLLER_H__
