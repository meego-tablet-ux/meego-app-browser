// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/interstitial_page.h"

#include "chrome/browser/browser.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/browser_resources.h"
#include "chrome/browser/dom_operation_notification_details.h"
#include "chrome/browser/navigation_controller.h"
#include "chrome/browser/navigation_entry.h"
#include "chrome/browser/render_widget_host_view_win.h"
#include "chrome/browser/web_contents.h"
#include "chrome/browser/web_contents_view_win.h"
#include "chrome/views/window.h"
#include "chrome/views/window_delegate.h"
#include "net/base/escape.h"

// static
InterstitialPage::InterstitialPageMap*
    InterstitialPage::tab_to_interstitial_page_ =  NULL;

InterstitialPage::InterstitialPage(WebContents* tab,
                                   bool new_navigation,
                                   const GURL& url)
    : tab_(tab),
      url_(url),
      action_taken_(false),
      enabled_(true),
      new_navigation_(new_navigation),
      render_view_host_(NULL),
      should_revert_tab_title_(false) {
  InitInterstitialPageMap();
  // It would be inconsistent to create an interstitial with no new navigation
  // (which is the case when the interstitial was triggered by a sub-resource on
  // a page) when we have a pending entry (in the process of loading a new top
  // frame).
  DCHECK(new_navigation || !tab->controller()->GetPendingEntry());
}

InterstitialPage::~InterstitialPage() {
  InterstitialPageMap::iterator iter = tab_to_interstitial_page_->find(tab_);
  DCHECK(iter != tab_to_interstitial_page_->end());
  tab_to_interstitial_page_->erase(iter);
  DCHECK(!render_view_host_);
}

void InterstitialPage::Show() {
  // If an interstitial is already showing, close it before showing the new one.
  if (tab_->interstitial_page())
    tab_->interstitial_page()->DontProceed();

  // Update the tab_to_interstitial_page_ map.
  InterstitialPageMap::const_iterator iter =
      tab_to_interstitial_page_->find(tab_);
  DCHECK(iter == tab_to_interstitial_page_->end());
  (*tab_to_interstitial_page_)[tab_] = this;

  if (new_navigation_) {
    NavigationEntry* entry = new NavigationEntry(TAB_CONTENTS_WEB);
    entry->set_url(url_);
    entry->set_display_url(url_);
    entry->set_page_type(NavigationEntry::INTERSTITIAL_PAGE);

    // Give sub-classes a chance to set some states on the navigation entry.
    UpdateEntry(entry);

    tab_->controller()->AddTransientEntry(entry);
  }

  DCHECK(!render_view_host_);
  render_view_host_ = CreateRenderViewHost();

  std::string data_url = "data:text/html;charset=utf-8," +
                         EscapePath(GetHTMLContents());
  render_view_host_->NavigateToURL(GURL(data_url));

  notification_registrar_.Add(this, NOTIFY_TAB_CONTENTS_DESTROYED,
                              Source<TabContents>(tab_));
  notification_registrar_.Add(this, NOTIFY_NAV_ENTRY_COMMITTED,
                              Source<NavigationController>(tab_->controller()));
  notification_registrar_.Add(this, NOTIFY_NAV_ENTRY_PENDING,
                              Source<NavigationController>(tab_->controller()));
}

void InterstitialPage::Hide() {
  render_view_host_->Shutdown();
  render_view_host_ = NULL;
  if (tab_->interstitial_page())
    tab_->remove_interstitial_page();
  // Let's revert to the original title if necessary.
  NavigationEntry* entry = tab_->controller()->GetActiveEntry();
  if (!new_navigation_ && should_revert_tab_title_) {
    entry->set_title(original_tab_title_);
    tab_->NotifyNavigationStateChanged(TabContents::INVALIDATE_TITLE);
  }
  delete this;
}

void InterstitialPage::Observe(NotificationType type,
                               const NotificationSource& source,
                               const NotificationDetails& details) {
  if (type == NOTIFY_NAV_ENTRY_PENDING) {
    // We are navigating away from the interstitial.  Make sure clicking on the
    // interstitial will have no effect.
    Disable();
    return;
  }
  DCHECK(type == NOTIFY_TAB_CONTENTS_DESTROYED ||
         type == NOTIFY_NAV_ENTRY_COMMITTED);
  if (!action_taken_) {
    // We are navigating away from the interstitial or closing a tab with an
    // interstitial.  Default to DontProceed(). We don't just call Hide as
    // subclasses will almost certainly override DontProceed to do some work
    // (ex: close pending connections).
    DontProceed();
  } else {
    // User decided to proceed and either the navigation was committed or the
    // tab was closed before that.
    Hide();
    // WARNING: we are now deleted!
  }
}

RenderViewHost* InterstitialPage::CreateRenderViewHost() {
  RenderViewHost* render_view_host = new RenderViewHost(
      SiteInstance::CreateSiteInstance(tab()->profile()),
      this, MSG_ROUTING_NONE, NULL);
  RenderWidgetHostViewWin* view =
      new RenderWidgetHostViewWin(render_view_host);
  render_view_host->set_view(view);
  view->Create(tab_->GetContentHWND());
  view->set_parent_hwnd(tab_->GetContentHWND());
  WebContentsViewWin* web_contents_view =
      static_cast<WebContentsViewWin*>(tab_->view());
  render_view_host->CreateRenderView();
  // SetSize must be called after CreateRenderView or the HWND won't show.
  view->SetSize(web_contents_view->GetContainerSize());

  render_view_host->AllowDomAutomationBindings();
  return render_view_host;
}

void InterstitialPage::Proceed() {
  DCHECK(!action_taken_);
  Disable();
  action_taken_ = true;

  // Resumes the throbber.
  tab_->SetIsLoading(true, NULL);

  // No need to hide if we are a new navigation, we'll get hidden when the
  // navigation is committed.
  if (!new_navigation_) {
    Hide();
    // WARNING: we are now deleted!
  }
}

void InterstitialPage::DontProceed() {
  DCHECK(!action_taken_);
  Disable();
  action_taken_ = true;

  if (new_navigation_) {
    // Since no navigation happens we have to discard the transient entry
    // explicitely.  Note that by calling DiscardNonCommittedEntries() we also
    // discard the pending entry, which is what we want, since the navigation is
    // cancelled.
    tab_->controller()->DiscardNonCommittedEntries();
  }

  Hide();
  // WARNING: we are now deleted!
}

void InterstitialPage::SetSize(const gfx::Size& size) {
  render_view_host_->view()->SetSize(size);
}

Profile* InterstitialPage::GetProfile() const {
  return tab_->profile();
}

void InterstitialPage::DidNavigate(
    RenderViewHost* render_view_host,
    const ViewHostMsg_FrameNavigate_Params& params) {
  // A fast user could have navigated away from the page that triggered the
  // interstitial while the interstitial was loading, that would have disabled
  // us. In that case we can dismiss ourselves.
  if (!enabled_){
    DontProceed();
    return;
  }

  // The RenderViewHost has loaded its contents, we can show it now.
  render_view_host_->view()->Show();
  tab_->set_interstitial_page(this); 

  // Notify the tab we are not loading so the throbber is stopped. It also
  // causes a NOTIFY_LOAD_STOP notification, that the AutomationProvider (used
  // by the UI tests) expects to consider a navigation as complete. Without this,
  // navigating in a UI test to a URL that triggers an interstitial would hang.
  tab_->SetIsLoading(false, NULL);
}

void InterstitialPage::RendererGone(RenderViewHost* render_view_host) {
  // Our renderer died. This should not happen in normal cases.
  // Just dismiss the interstitial.
  DontProceed();
}

void InterstitialPage::DomOperationResponse(const std::string& json_string,
                                            int automation_id) {
  if (enabled_)
    CommandReceived(json_string);
}

void InterstitialPage::UpdateTitle(RenderViewHost* render_view_host,
                                   int32 page_id,
                                   const std::wstring& title) {
  DCHECK(render_view_host == render_view_host_);
  NavigationEntry* entry = tab_->controller()->GetActiveEntry();
  // If this interstitial is shown on an existing navigation entry, we'll need
  // to remember its title so we can revert to it when hidden.
  if (!new_navigation_ && !should_revert_tab_title_) {
    original_tab_title_ = entry->title();
    should_revert_tab_title_ = true;
  }
  entry->set_title(title);
  tab_->NotifyNavigationStateChanged(TabContents::INVALIDATE_TITLE);
}

void InterstitialPage::Disable() {
  enabled_ = false;
}

// static
void InterstitialPage::InitInterstitialPageMap() {
  if (!tab_to_interstitial_page_)
    tab_to_interstitial_page_ = new InterstitialPageMap;
}

// static
InterstitialPage* InterstitialPage::GetInterstitialPage(
    WebContents* web_contents) {
  InitInterstitialPageMap();
  InterstitialPageMap::const_iterator iter =
      tab_to_interstitial_page_->find(web_contents);
  if (iter == tab_to_interstitial_page_->end())
    return NULL;

  return iter->second;
}
