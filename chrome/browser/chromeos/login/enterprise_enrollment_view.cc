// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/enterprise_enrollment_view.h"

#include "chrome/browser/chromeos/login/enterprise_enrollment_screen.h"
#include "chrome/browser/chromeos/login/helper.h"
#include "chrome/browser/chromeos/login/rounded_rect_painter.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/webui/chromeos/enterprise_enrollment_ui.h"
#include "chrome/common/url_constants.h"
#include "content/browser/renderer_host/render_view_host.h"
#include "content/browser/site_instance.h"
#include "content/browser/tab_contents/tab_contents_delegate.h"
#include "views/border.h"
#include "views/layout/layout_constants.h"

namespace chromeos {

namespace {

// Layout constants.
const int kBorderSize = 30;

// Renders the registration page.
class EnrollmentDomView : public WebPageDomView,
                          public TabContentsDelegate {
 public:
  EnrollmentDomView() {}
  virtual ~EnrollmentDomView() {}

 protected:
  // DomView imlementation:
  virtual TabContents* CreateTabContents(Profile* profile,
                                         SiteInstance* instance) {
    TabContents* contents = new WizardWebPageViewTabContents(profile,
                                                             instance,
                                                             page_delegate_);
    contents->set_delegate(this);
    return contents;
  }

  // TabContentsDelegate implementation:
  virtual void OpenURLFromTab(TabContents* source,
                              const GURL& url, const GURL& referrer,
                              WindowOpenDisposition disposition,
                              PageTransition::Type transition) {}
  virtual void NavigationStateChanged(const TabContents* source,
                                      unsigned changed_flags) {}
  virtual void AddNewContents(TabContents* source,
                              TabContents* new_contents,
                              WindowOpenDisposition disposition,
                              const gfx::Rect& initial_pos,
                              bool user_gesture) {}
  virtual void ActivateContents(TabContents* contents) {}
  virtual void DeactivateContents(TabContents* contents) {}
  virtual void LoadingStateChanged(TabContents* source) {}
  virtual void CloseContents(TabContents* source) {}
  virtual bool IsPopup(TabContents* source) { return false; }
  virtual void UpdateTargetURL(TabContents* source, const GURL& url) {}
  virtual bool ShouldAddNavigationToHistory(
      const history::HistoryAddPageArgs& add_page_args,
      NavigationType::Type navigation_type) {
    return false;
  }
  virtual void MoveContents(TabContents* source, const gfx::Rect& pos) {}
  virtual void ToolbarSizeChanged(TabContents* source, bool is_animating) {}
  virtual bool HandleContextMenu(const ContextMenuParams& params) {
    return true;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(EnrollmentDomView);
};

}  // namespace

EnterpriseEnrollmentView::EnterpriseEnrollmentView(
    EnterpriseEnrollmentController* controller)
    : controller_(controller) {}

EnterpriseEnrollmentView::~EnterpriseEnrollmentView() {}

void EnterpriseEnrollmentView::Init() {
  // Use rounded rect background.
  views::Painter* painter =
      CreateWizardPainter(&BorderDefinition::kScreenBorder);
  set_background(views::Background::CreateBackgroundPainter(true, painter));

  // Create the view that hosts the enrollment page.
  enrollment_page_view_ = new EnrollmentDomView();
  enrollment_page_view_->set_border(
      views::Border::CreateEmptyBorder(kBorderSize, kBorderSize,
                                       kBorderSize, kBorderSize));

  AddChildView(enrollment_page_view_);

  // Load the enrollment page.
  Profile* profile = ProfileManager::GetDefaultProfile();
  GURL url(chrome::kChromeUIEnterpriseEnrollmentURL);
  enrollment_page_view_->Init(
      profile, SiteInstance::CreateSiteInstanceForURL(profile, url));
  EnterpriseEnrollmentUI::SetController(enrollment_page_view_->tab_contents(),
                                        this);
  enrollment_page_view_->LoadURL(url);
}

void EnterpriseEnrollmentView::ShowConfirmationScreen() {
  RenderViewHost* render_view_host =
      enrollment_page_view_->tab_contents()->render_view_host();
  render_view_host->ExecuteJavascriptInWebFrame(
      string16(),
      UTF8ToUTF16("enterpriseEnrollment.showScreen('confirmation-screen');"));
}

void EnterpriseEnrollmentView::OnAuthSubmitted(const std::string& user,
                                               const std::string& password,
                                               const std::string& captcha,
                                               const std::string& access_code) {
  controller_->Authenticate(user, password, captcha, access_code);
}

void EnterpriseEnrollmentView::OnAuthCancelled() {
  controller_->CancelEnrollment();
}

void EnterpriseEnrollmentView::OnConfirmationClosed() {
  controller_->CloseConfirmation();
}

void EnterpriseEnrollmentView::Layout() {
  enrollment_page_view_->SetBoundsRect(GetContentsBounds());
}

}  // namespace chromeos
