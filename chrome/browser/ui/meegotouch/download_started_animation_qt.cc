// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/download/download_started_animation.h"

#include "ui/base/animation/linear_animation.h"
#include "ui/base/resource/resource_bundle.h"
#include "base/message_loop.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/common/notification_registrar.h"
#include "content/common/notification_service.h"
#include "ui/gfx/rect.h"
#include "grit/theme_resources.h"

// static
void DownloadStartedAnimation::Show(TabContents* tab_contents) {
  // The animation will delete itself.
  DNOTIMPLEMENTED();
}
