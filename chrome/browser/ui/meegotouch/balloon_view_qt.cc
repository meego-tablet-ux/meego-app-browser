// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/animation/animation_delegate.h"
#include "ui/base/animation/slide_animation.h"
#include "base/message_loop.h"
#include "base/string_util.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/extensions/extension_host.h"
#include "chrome/browser/extensions/extension_process_manager.h"
#include "chrome/browser/notifications/balloon.h"
#include "chrome/browser/notifications/desktop_notification_service.h"
#include "chrome/browser/profiles/profile.h"
#include "content/browser/renderer_host/render_view_host.h"
#include "content/browser/renderer_host/render_widget_host_view.h"
#include "chrome/common/extensions/extension.h"
#include "content/common/notification_details.h"
#include "content/common/notification_service.h"
#include "content/common/notification_source.h"
#include "content/common/notification_type.h"

#undef signals
#include "ui/gfx/canvas.h"
#include "ui/gfx/insets.h"
#include "ui/gfx/native_widget_types.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"

  namespace {

// Margin, in pixels, between the notification frame and the contents
// of the notification.
const int kTopMargin = 0;
const int kBottomMargin = 1;
const int kLeftMargin = 1;
const int kRightMargin = 1;

// How many pixels of overlap there is between the shelf top and the
// balloon bottom.
const int kShelfBorderTopOverlap = 0;

// Properties of the dismiss button.
const int kDismissButtonWidth = 60;
const int kDismissButtonHeight = 20;

// Properties of the options menu.
const int kOptionsMenuWidth = 60;
const int kOptionsMenuHeight = 20;

// Properties of the origin label.
const int kLeftLabelMargin = 8;

// TODO(johnnyg): Add a shadow for the frame.
const int kLeftShadowWidth = 0;
const int kRightShadowWidth = 0;
const int kTopShadowWidth = 0;
const int kBottomShadowWidth = 0;

// Space in pixels between text and icon on the buttons.
const int kButtonIconSpacing = 10;

// Number of characters to show in the origin label before ellipsis.
const int kOriginLabelCharacters = 18;

// The shelf height for the system default font size.  It is scaled
// with changes in the default font size.
const int kDefaultShelfHeight = 21;
const int kShelfVerticalMargin = 3;

// The amount that the bubble collections class offsets from the side of the
// screen.
const int kScreenBorder = 5;

// Colors specified in various ways for different parts of the UI.
// These match the windows colors in balloon_view.cc
const double kShelfBackgroundColorR = 245.0 / 255.0;
const double kShelfBackgroundColorG = 245.0 / 255.0;
const double kShelfBackgroundColorB = 245.0 / 255.0;
const double kDividerLineColorR = 180.0 / 255.0;
const double kDividerLineColorG = 180.0 / 255.0;
const double kDividerLineColorB = 180.0 / 255.0;

// Makes the website label relatively smaller to the base text size.

}  // namespace

class BalloonViewImpl : public BalloonView,
                        public ui::AnimationDelegate ,
                        public NotificationObserver{
 public:
  explicit BalloonViewImpl(BalloonCollection* collection);
  ~BalloonViewImpl();

  // BalloonView interface.
  virtual void Show(Balloon* balloon);
  virtual void Update() {}
  virtual void RepositionToBalloon();
  virtual void Close(bool by_user);
  virtual gfx::Size GetSize() const;
  virtual BalloonHost* GetHost() const { return NULL;}
};

  
BalloonViewImpl::BalloonViewImpl(BalloonCollection* collection) {
  DNOTIMPLEMENTED();
}

BalloonViewImpl::~BalloonViewImpl() {
  DNOTIMPLEMENTED();
}

void BalloonViewImpl::Close(bool by_user) {
  DNOTIMPLEMENTED();
}

gfx::Size BalloonViewImpl::GetSize() const {
  return gfx::Size();
}


void BalloonViewImpl::RepositionToBalloon() {
  DNOTIMPLEMENTED();
}


void BalloonViewImpl::Show(Balloon* balloon) {
  DNOTIMPLEMENTED();
}

