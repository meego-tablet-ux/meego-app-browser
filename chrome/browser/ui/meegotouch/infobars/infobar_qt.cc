// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <QString>

#include "chrome/browser/ui/meegotouch/infobars/infobar_qt.h"

#include "base/utf_string_conversions.h"
#include "chrome/browser/tab_contents/infobar_delegate.h"
#include "chrome/browser/ui/meegotouch/infobars/infobar_container_qt.h"
#include "content/common/notification_service.h"
#include "chrome/browser/tab_contents/link_infobar_delegate.h"
#include "chrome/browser/translate/translate_infobar_delegate.h"

InfoBar::InfoBar(InfoBarDelegate* delegate)
    : container_(NULL),
      delegate_(delegate) {
  buttons_ = ButtonNone;
  text_ = QString();
  accept_label_ = QString();
  cancel_label_ = QString();
}

InfoBar::~InfoBar() {
  //DNOTIMPLEMENTED();
}

void InfoBar::AnimateOpen() {
  //DNOTIMPLEMENTED();
}

void InfoBar::Open() {
  //DNOTIMPLEMENTED();
}

void InfoBar::AnimateClose() {
  //DNOTIMPLEMENTED();
}

void InfoBar::Close() {
  if (delegate_) {
    delegate_->InfoBarClosed();
    delegate_ = NULL;
  }
  delete this;
}

bool InfoBar::ProcessButtonEvent(ButtonType button) {

  switch (button) {
    case ButtonClose:
      OnCloseButton();
      return true;
    default:
      return false;
  }
}

void InfoBar::OnCloseButton() {
  if (delegate_)
    delegate_->InfoBarDismissed();
  RemoveInfoBar();
}

bool InfoBar::IsAnimating() {
  //DNOTIMPLEMENTED();
}

void InfoBar::RemoveInfoBar() const {
  container_->RemoveDelegate(delegate_);
}

void InfoBar::Observe(NotificationType type,
                      const NotificationSource& source,
                      const NotificationDetails& details) {
  //  UpdateBorderColor();
}

void InfoBar::AddLabel(const string16& display_text) {
  text_ = QString::fromStdWString(UTF16ToWide(display_text));
}

// LinkInfoBar ----------------------------------------------------------------

class LinkInfoBar : public InfoBar {
 public:
  explicit LinkInfoBar(LinkInfoBarDelegate* delegate)
      : InfoBar(delegate) {
    size_t link_offset;
    string16 display_text = delegate->GetMessageTextWithOffset(&link_offset);
    AddLabel(display_text);
  }
};

// ConfirmInfoBar --------------------------------------------------------------

class ConfirmInfoBar : public InfoBar {
 public:
  explicit ConfirmInfoBar(ConfirmInfoBarDelegate* delegate)
      : InfoBar(delegate) {
    AddConfirmButton(ConfirmInfoBarDelegate::BUTTON_CANCEL);
    AddConfirmButton(ConfirmInfoBarDelegate::BUTTON_OK);
    string16 display_text = delegate->GetMessageText();
    //string16 link_text = delegate->GetLinkText();
    AddLabel(display_text);
  }

  ~ConfirmInfoBar() {}

  virtual bool ProcessButtonEvent(ButtonType button);

  void OnCancelButton() {
    if (delegate_->AsConfirmInfoBarDelegate()->Cancel())
      RemoveInfoBar();
  }

  void OnOkButton() {
    if (delegate_->AsConfirmInfoBarDelegate()->Accept())
      RemoveInfoBar();
  }

 private:
  void AddConfirmButton(ConfirmInfoBarDelegate::InfoBarButton type);

};

bool ConfirmInfoBar::ProcessButtonEvent(ButtonType button) {

  switch (button) {
    case ButtonAccept:
      OnOkButton();
      return true;
    case ButtonCancel:
      OnCancelButton();
      return true;
    default:
      return this->InfoBar::ProcessButtonEvent(button);
  }
}

void ConfirmInfoBar::AddConfirmButton(ConfirmInfoBarDelegate::InfoBarButton type) {
  // Adds a button to the info bar by type. It will do nothing if the delegate
  // doesn't specify a button of the given type.
  if (delegate_->AsConfirmInfoBarDelegate()->GetButtons() & type) {
    QString label = QString::fromStdWString(UTF16ToWide(
      delegate_->AsConfirmInfoBarDelegate()->GetButtonLabel(type)).c_str());

    if (type == ConfirmInfoBarDelegate::BUTTON_OK) {
      accept_label_ = label;
      buttons_ |= ButtonAccept;
    } else if ( type == ConfirmInfoBarDelegate::BUTTON_CANCEL) {
      buttons_ |= ButtonCancel;
      cancel_label_ = label;
    }
  }
}

// LinkInfoBarDelegate, InfoBarDelegate overrides: ----------------------------
InfoBar* LinkInfoBarDelegate::CreateInfoBar() {
  return new LinkInfoBar(this);
}

InfoBar* TranslateInfoBarDelegate::CreateInfoBar() {
  DNOTIMPLEMENTED();
  return NULL;
}

// ConfirmInfoBarDelegate, InfoBarDelegate overrides: --------------------------

InfoBar* ConfirmInfoBarDelegate::CreateInfoBar() {
  return new ConfirmInfoBar(this);
}
