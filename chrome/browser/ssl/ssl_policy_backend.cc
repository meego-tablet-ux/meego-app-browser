// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/ssl_policy_backend.h"

#include "app/resource_bundle.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/ssl/ssl_host_state.h"
#include "chrome/browser/tab_contents/infobar_delegate.h"
#include "chrome/browser/tab_contents/navigation_controller.h"
#include "chrome/browser/tab_contents/navigation_entry.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"

class SSLInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  SSLInfoBarDelegate(TabContents* contents,
                     const string16& message,
                     const string16& button_label,
                     Task* task)
    : ConfirmInfoBarDelegate(contents),
      message_(message),
      button_label_(button_label),
      task_(task) {
  }
  virtual ~SSLInfoBarDelegate() {}

  // Overridden from ConfirmInfoBarDelegate:
  virtual void InfoBarClosed() {
    delete this;
  }
  virtual string16 GetMessageText() const {
    return message_;
  }
  virtual SkBitmap* GetIcon() const {
    return ResourceBundle::GetSharedInstance().GetBitmapNamed(
        IDR_INFOBAR_SSL_WARNING);
  }
  virtual int GetButtons() const {
    return !button_label_.empty() ? BUTTON_OK : BUTTON_NONE;
  }
  virtual string16 GetButtonLabel(InfoBarButton button) const {
    return button_label_;
  }
  virtual bool Accept() {
    if (task_.get()) {
      task_->Run();
      task_.reset();  // Ensures we won't run the task again.
    }
    return true;
  }

 private:
  // Labels for the InfoBar's message and button.
  string16 message_;
  string16 button_label_;

  // A task to run when the InfoBar is accepted.
  scoped_ptr<Task> task_;

  DISALLOW_COPY_AND_ASSIGN(SSLInfoBarDelegate);
};

SSLPolicyBackend::SSLPolicyBackend(NavigationController* controller)
    : controller_(controller),
      ssl_host_state_(controller->profile()->GetSSLHostState()) {
  DCHECK(controller_);
}

void SSLPolicyBackend::ShowMessage(const std::wstring& msg) {
  ShowMessageWithLink(msg, std::wstring(), NULL);
}

void SSLPolicyBackend::ShowMessageWithLink(const std::wstring& msg,
                                           const std::wstring& link_text,
                                           Task* task) {
  if (controller_->pending_entry()) {
    // The main frame is currently loading, wait until the load is committed so
    // to show the error on the right page (once the location bar shows the
    // correct url).
    if (std::find(pending_messages_.begin(), pending_messages_.end(), msg) ==
        pending_messages_.end())
      pending_messages_.push_back(SSLMessageInfo(msg, link_text, task));

    return;
  }

  NavigationEntry* entry = controller_->GetActiveEntry();
  if (!entry)
    return;

  // Don't show the message if the user doesn't expect an authenticated session.
  if (entry->ssl().security_style() <= SECURITY_STYLE_UNAUTHENTICATED)
    return;

  if (controller_->tab_contents()) {
    // TODO(evanm): remove base/utf_string_conversions.h include after fixing
    // below conversions.
    controller_->tab_contents()->AddInfoBar(
        new SSLInfoBarDelegate(controller_->tab_contents(),
                               WideToUTF16Hack(msg),
                               WideToUTF16Hack(link_text),
                               task));
  }
}

void SSLPolicyBackend::HostRanInsecureContent(const std::string& host,
                                              int id) {
  ssl_host_state_->HostRanInsecureContent(host, id);
  SSLManager::NotifySSLInternalStateChanged();
}

bool SSLPolicyBackend::DidHostRunInsecureContent(const std::string& host,
                                                 int pid) const {
  return ssl_host_state_->DidHostRunInsecureContent(host, pid);
}

void SSLPolicyBackend::DenyCertForHost(net::X509Certificate* cert,
                                       const std::string& host) {
  ssl_host_state_->DenyCertForHost(cert, host);
}

void SSLPolicyBackend::AllowCertForHost(net::X509Certificate* cert,
                                        const std::string& host) {
  ssl_host_state_->AllowCertForHost(cert, host);
}

net::CertPolicy::Judgment SSLPolicyBackend::QueryPolicy(
    net::X509Certificate* cert, const std::string& host) {
  return ssl_host_state_->QueryPolicy(cert, host);
}

void SSLPolicyBackend::ShowPendingMessages() {
  std::vector<SSLMessageInfo>::const_iterator iter;
  for (iter = pending_messages_.begin();
       iter != pending_messages_.end(); ++iter) {
    ShowMessageWithLink(iter->message, iter->link_text, iter->action);
  }
  ClearPendingMessages();
}

void SSLPolicyBackend::ClearPendingMessages() {
  pending_messages_.clear();
}
