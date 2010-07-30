// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/update_screen.h"

#include "base/logging.h"
#include "chrome/browser/chromeos/cros/cros_library.h"
#include "chrome/browser/chromeos/login/screen_observer.h"
#include "chrome/browser/chromeos/login/update_view.h"

namespace {

// Update window should appear for at least kMinimalUpdateTime seconds.
const int kMinimalUpdateTimeSec = 3;

// Time in seconds that we wait for the device to reboot.
// If reboot didn't happen, ask user to reboot device manually.
const int kWaitForRebootTimeSec = 3;

// Progress bar stages. Each represents progress bar value
// at the beginning of each stage.
// TODO(nkostylev): Base stage progress values on approximate time.
// TODO(nkostylev): Animate progress during each state.
const int kBeforeUpdateCheckProgress = 7;
const int kBeforeDownloadProgress = 14;
const int kBeforeVerifyingProgress = 74;
const int kBeforeFinalizingProgress = 81;
const int kProgressComplete = 100;

// Defines what part of update progress does download part takes.
const int kDownloadProgressIncrement = 60;

}  // anonymous namespace

namespace chromeos {

UpdateScreen::UpdateScreen(WizardScreenDelegate* delegate)
    : DefaultViewScreen<chromeos::UpdateView>(delegate),
      proceed_with_oobe_(false),
      checking_for_update_(true) {
}

UpdateScreen::~UpdateScreen() {
  // Remove pointer to this object from view.
  if (view())
    view()->set_controller(NULL);
  CrosLibrary::Get()->GetUpdateLibrary()->RemoveObserver(this);
}

void UpdateScreen::UpdateStatusChanged(UpdateLibrary* library) {
  UpdateStatusOperation status = library->status().status;
  LOG(INFO) << "Update status: " << status;
  if (checking_for_update_ && status > UPDATE_STATUS_CHECKING_FOR_UPDATE) {
    checking_for_update_ = false;
  }

  switch (status) {
    case UPDATE_STATUS_CHECKING_FOR_UPDATE:
      // Do nothing in these cases, we don't want to notify the user of the
      // check unless there is an update.
      break;
    case UPDATE_STATUS_UPDATE_AVAILABLE:
      view()->SetProgress(kBeforeDownloadProgress);
      LOG(INFO) << "Update available: " << library->status().new_version;
      break;
    case UPDATE_STATUS_DOWNLOADING:
      {
        int download_progress = static_cast<int>(
            library->status().download_progress * kDownloadProgressIncrement);
        view()->SetProgress(kBeforeDownloadProgress + download_progress);
      }
      break;
    case UPDATE_STATUS_VERIFYING:
      view()->SetProgress(kBeforeVerifyingProgress);
      break;
    case UPDATE_STATUS_FINALIZING:
      view()->SetProgress(kBeforeFinalizingProgress);
      break;
    case UPDATE_STATUS_UPDATED_NEED_REBOOT:
      view()->SetProgress(kProgressComplete);
      CrosLibrary::Get()->GetUpdateLibrary()->RebootAfterUpdate();
      LOG(INFO) << "Reboot API was called. Waiting for reboot.";
      reboot_timer_.Start(base::TimeDelta::FromSeconds(kWaitForRebootTimeSec),
                          this,
                          &UpdateScreen::OnWaitForRebootTimeElapsed);
      break;
    case UPDATE_STATUS_IDLE:
    case UPDATE_STATUS_ERROR:
    case UPDATE_STATUS_REPORTING_ERROR_EVENT:
      if (MinimalUpdateTimeElapsed()) {
        ExitUpdate();
      }
      proceed_with_oobe_ = true;
      break;
    default:
      NOTREACHED();
      break;
  }
}

void UpdateScreen::StartUpdate() {
  // Reset view.
  view()->Reset();
  view()->set_controller(this);

  // Start the minimal update time timer.
  minimal_update_time_timer_.Start(
      base::TimeDelta::FromSeconds(kMinimalUpdateTimeSec),
      this,
      &UpdateScreen::OnMinimalUpdateTimeElapsed);

  view()->SetProgress(kBeforeUpdateCheckProgress);

  if (!CrosLibrary::Get()->EnsureLoaded()) {
    LOG(ERROR) << "Error loading CrosLibrary";
  } else {
    CrosLibrary::Get()->GetUpdateLibrary()->AddObserver(this);
    LOG(INFO) << "Checking for update";
    if (!CrosLibrary::Get()->GetUpdateLibrary()->CheckForUpdate()) {
      ExitUpdate();
    }
  }
}

void UpdateScreen::CancelUpdate() {
#if !defined(OFFICIAL_BUILD)
  ExitUpdate();
#endif
}

void UpdateScreen::ExitUpdate() {
  minimal_update_time_timer_.Stop();
  ScreenObserver* observer = delegate()->GetObserver(this);

  if (!CrosLibrary::Get()->EnsureLoaded()) {
    observer->OnExit(ScreenObserver::UPDATE_ERROR_CHECKING_FOR_UPDATE);
  }

  UpdateLibrary* update_library = CrosLibrary::Get()->GetUpdateLibrary();
  update_library->RemoveObserver(this);
  switch (update_library->status().status) {
    case UPDATE_STATUS_IDLE:
      observer->OnExit(ScreenObserver::UPDATE_NOUPDATE);
      break;
    case UPDATE_STATUS_ERROR:
    case UPDATE_STATUS_REPORTING_ERROR_EVENT:
      observer->OnExit(checking_for_update_ ?
          ScreenObserver::UPDATE_ERROR_CHECKING_FOR_UPDATE :
          ScreenObserver::UPDATE_ERROR_UPDATING);
      break;
    default:
      NOTREACHED();
  }
}

bool UpdateScreen::MinimalUpdateTimeElapsed() {
  return !minimal_update_time_timer_.IsRunning();
}

void UpdateScreen::OnMinimalUpdateTimeElapsed() {
  if (proceed_with_oobe_)
    ExitUpdate();
}

void UpdateScreen::OnWaitForRebootTimeElapsed() {
  LOG(ERROR) << "Unable to reboot - asking user for a manual reboot.";
  view()->ShowManualRebootInfo();
}

}  // namespace chromeos
