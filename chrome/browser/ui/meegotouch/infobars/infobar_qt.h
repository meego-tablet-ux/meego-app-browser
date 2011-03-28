// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GTK_INFOBAR_QT_H_
#define CHROME_BROWSER_GTK_INFOBAR_QT_H_
#pragma once

#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "chrome/browser/tab_contents/infobar_delegate.h"
#include "content/common/notification_observer.h"
#include "content/common/notification_registrar.h"

class InfoBarContainerQt;
class InfoBarDelegate;
class ConfirmInfoBar;
class QString;

class InfoBar : public NotificationObserver {
 public:

  enum ButtonType {
    ButtonNone = 0,
    ButtonAccept = 1 << 0,
    ButtonCancel = 1 << 1,
    ButtonOKDefault = 1 << 2,
    ButtonClose = 1 << 3
  };

  explicit InfoBar(InfoBarDelegate* delegate);
  virtual ~InfoBar();

  InfoBarDelegate* delegate() const { return delegate_; }

  // Set a link to the parent InfoBarContainer. This must be set before the
  // InfoBar is added to the view hierarchy.
  void set_container(InfoBarContainerQt* container) { container_ = container; }

  // Starts animating the InfoBar open.
  void AnimateOpen();

  // Opens the InfoBar immediately.
  void Open();

  // Starts animating the InfoBar closed. It will not be closed until the
  // animation has completed, when |Close| will be called.
  void AnimateClose();

  // Closes the InfoBar immediately and removes it from its container. Notifies
  // the delegate that it has closed. The InfoBar is deleted after this function
  // is called.
  void Close();

  // Returns true if the infobar is showing the its open or close animation.
  bool IsAnimating();

  // NotificationOPbserver implementation.
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);


  virtual bool ProcessButtonEvent(ButtonType button);
  virtual void OnCloseButton();
  virtual int type() const { return type_; }
  virtual int buttons() const { return buttons_; }
  virtual QString text() const { return text_; }
  virtual QString acceptLabel() const { return accept_label_; }
  virtual QString cancelLabel() const { return cancel_label_; }

 protected:
  // Removes our associated InfoBarDelegate from the associated TabContents.
  // (Will lead to this InfoBar being closed).
  void RemoveInfoBar() const;

  void AddLabel(const string16& display_text);


  // The InfoBar's container
  InfoBarContainerQt* container_;

  // The InfoBar's delegate.
  InfoBarDelegate* delegate_;

  NotificationRegistrar registrar_;

  int type_;
  int buttons_;
  QString text_;
  QString accept_label_;
  QString cancel_label_;

 private:
  DISALLOW_COPY_AND_ASSIGN(InfoBar);
};

#endif  // CHROME_BROWSER_GTK_INFOBAR_QT_H_
