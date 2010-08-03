// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/update_observer.h"

#include "app/l10n_util.h"
#include "base/string_number_conversions.h"
#include "base/utf_string_conversions.h"
#include "chrome/common/time_format.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"

namespace chromeos {

UpdateObserver::UpdateObserver(Profile* profile)
    : notification_(profile, "update.chromeos", IDR_NOTIFICATION_UPDATE,
                    l10n_util::GetStringUTF16(IDS_UPDATE_TITLE)),
      progress_(-1) {}

UpdateObserver::~UpdateObserver() {
  notification_.Hide();
}

void UpdateObserver::UpdateStatusChanged(UpdateLibrary* library) {
  switch (library->status().status) {
    case UPDATE_STATUS_IDLE:
    case UPDATE_STATUS_CHECKING_FOR_UPDATE:
      // Do nothing in these cases, we don't want to notify the user of the
      // check unless there is an update. We don't hide here because
      // we want the final state to be sticky.
      break;
    case UPDATE_STATUS_UPDATE_AVAILABLE:
      notification_.Show(l10n_util::GetStringUTF16(IDS_UPDATE_AVAILABLE),
                         false);
      break;
    case UPDATE_STATUS_DOWNLOADING:
    {
      int progress = static_cast<int>(library->status().download_progress *
          100.0);
      if (progress != progress_) {
        progress_ = progress;
        notification_.Show(l10n_util::GetStringFUTF16(IDS_UPDATE_DOWNLOADING,
            base::IntToString16(progress_)), false);
      }
    }
      break;
    case UPDATE_STATUS_VERIFYING:
      notification_.Show(l10n_util::GetStringUTF16(IDS_UPDATE_VERIFYING),
                         false);
      break;
    case UPDATE_STATUS_FINALIZING:
      notification_.Show(l10n_util::GetStringUTF16(IDS_UPDATE_FINALIZING),
                         false);
      break;
    case UPDATE_STATUS_UPDATED_NEED_REBOOT:
      notification_.Show(l10n_util::GetStringUTF16(IDS_UPDATE_COMPLETED), true);
      break;
    case UPDATE_STATUS_REPORTING_ERROR_EVENT:
      // If the update engine encounters an error and we have already
      // notified the user of the update progress, show an error
      // notification. Don't show anything otherwise -- for example,
      // in cases where the update engine encounters an error while
      // checking for an update.
      if (notification_.visible()) {
        notification_.Show(l10n_util::GetStringUTF16(IDS_UPDATE_ERROR), true);
      }
      break;
    default:
      notification_.Show(l10n_util::GetStringUTF16(IDS_UPDATE_ERROR), true);
      break;
  }
}

}  // namespace chromeos
