// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/notifications/balloon.h"

#include "base/logging.h"
#include "chrome/browser/notifications/balloon_collection.h"
#include "chrome/browser/notifications/notification.h"
#include "chrome/browser/renderer_host/site_instance.h"
#include "gfx/rect.h"
#include "gfx/size.h"

Balloon::Balloon(const Notification& notification, Profile* profile,
                 BalloonCollection* collection)
    : profile_(profile),
      notification_(new Notification(notification)),
      collection_(collection) {
}

Balloon::~Balloon() {
}

void Balloon::SetPosition(const gfx::Point& upper_left, bool reposition) {
  position_ = upper_left;
  if (reposition && balloon_view_.get())
    balloon_view_->RepositionToBalloon();
}

void Balloon::SetContentPreferredSize(const gfx::Size& size) {
  gfx::Size new_size(size);
#if defined(OS_MACOSX)
  // TODO(levin): Make all of the code that went in with this change to be
  // cross-platform. See http://crbug.com/64720
  // Only allow the size of notifications to grow.  This stops the balloon
  // from jumping between sizes due to dynamic content.  For example, the
  // balloon's contents may adjust due to changes in
  // document.body.clientHeight.
  new_size.set_height(std::max(new_size.height(), content_size_.height()));

  if (content_size_ == new_size)
    return;
#endif
  collection_->ResizeBalloon(this, new_size);
}

void Balloon::set_view(BalloonView* balloon_view) {
  balloon_view_.reset(balloon_view);
}

void Balloon::Show() {
  notification_->Display();
  if (balloon_view_.get()) {
    balloon_view_->Show(this);
    balloon_view_->RepositionToBalloon();
  }
}

void Balloon::Update(const Notification& notification) {
  notification_->Close(false);
  notification_.reset(new Notification(notification));
  notification_->Display();
  if (balloon_view_.get()) {
    balloon_view_->Update();
  }
}

void Balloon::OnClick() {
  notification_->Click();
}

void Balloon::OnClose(bool by_user) {
  notification_->Close(by_user);
  collection_->OnBalloonClosed(this);
}

void Balloon::CloseByScript() {
  // A user-initiated close begins with the view and then closes this object;
  // we simulate that with a script-initiated close but pass |by_user|=false.
  DCHECK(balloon_view_.get());
  balloon_view_->Close(false);
}
