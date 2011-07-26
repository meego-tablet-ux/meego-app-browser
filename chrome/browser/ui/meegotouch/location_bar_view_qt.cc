// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/browser_window.h"
#include <string>

#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "base/basictypes.h"
#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/string_util.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/accessibility_events.h"
#include "chrome/browser/alternate_nav_url_fetcher.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/command_updater.h"
#include "chrome/browser/content_setting_bubble_model.h"
#include "chrome/browser/content_setting_image_model.h"
#include "chrome/browser/defaults.h"
#include "chrome/browser/extensions/extension_accessibility_api_constants.h"
#include "chrome/browser/extensions/extension_browser_event_router.h"
#include "chrome/browser/extensions/extension_tabs_module.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/autocomplete/autocomplete_edit_view_qt.h"
#include "chrome/browser/ui/meegotouch/location_bar_view_qt.h"

#include "chrome/browser/ui/omnibox/location_bar_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url.h"
#include "chrome/browser/search_engines/template_url_model.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/extensions/extension_action.h"
#include "chrome/common/extensions/extension_resource.h"
#include "content/common/notification_service.h"
#include "content/common/page_transition_types.h"
#include "chrome/common/pref_names.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"
#include "net/base/net_util.h"
#include "webkit/glue/window_open_disposition.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/autocomplete/autocomplete_edit_view_qt.h"


LocationBarViewQt::LocationBarViewQt(Browser* browser, BrowserWindowQt* window)
    : window_(window),
      profile_(browser->profile()),
      command_updater_(browser->command_updater()),
      toolbar_model_(browser->toolbar_model()),
      browser_(browser),
      disposition_(CURRENT_TAB),
      transition_(PageTransition::TYPED),
      popup_window_mode_(false),
      focused_(false)
{
  location_entry_.reset(new AutocompleteEditViewQt(this,
                                                   toolbar_model_,
                                                   profile_,
                                                   command_updater_,
                                                   popup_window_mode_,
                                                   window_));
}

LocationBarViewQt::~LocationBarViewQt() {
}

void LocationBarViewQt::Init(bool popup_window_mode) {
  popup_window_mode_ = popup_window_mode;

  // Create the widget first, so we can pass it to the AutocompleteEditViewGtk.
  // location_entry_ = new MLocationBar(browser_);
  location_entry_->Init();

}

void LocationBarViewQt::SetProfile(Profile* profile) {
  profile_ = profile;
}

TabContents* LocationBarViewQt::GetTabContents() const {
  return browser_->GetSelectedTabContents();
}

void LocationBarViewQt::UpdateTitle()
{
  if (focused_ ) return;  // if focused on location bar, don't change title
  if (browser_->GetSelectedTabContents() ) {
    location_entry_->SetUserText(WideToUTF16(GetInputString()), GetTitle(), false);
  }
}

void LocationBarViewQt::Update(const TabContents* contents) {
  ///\todo: implement site area, content setting etc.,
//  UpdateSiteTypeArea();
//  UpdateContentSettingsIcons();
//  UpdatePageActions();
  location_entry_->Update(contents);
  location_entry_->model()->SetInputInProgress(!IsTitleSet());
  location_entry_->SetUserText(WideToUTF16(GetInputString()), GetTitle(), false);
}


void LocationBarViewQt::OnInputInProgress(bool in_progress) {
  // This is identical to the Windows code, except that we don't proxy the call
  // back through the Toolbar, and just access the model here.
  // The edit should make sure we're only notified when something changes.
  DCHECK(toolbar_model_->input_in_progress() != in_progress);

  toolbar_model_->set_input_in_progress(in_progress);
}

void LocationBarViewQt::OnKillFocus() {
  BrowserWindowQt* browser_window = (BrowserWindowQt*)browser_->window();
  location_entry_->SetUserText(WideToUTF16(GetInputString()), GetTitle(), false);
  focused_ = false;

  gfx::NativeView widget = browser_->GetSelectedTabContents()->GetContentNativeView();
  if (widget) {
    widget->setFocusPolicy(Qt::StrongFocus);
  }
  browser_->GetSelectedTabContents()->Focus();
}

void LocationBarViewQt::OnSetFocus() {
  BrowserWindowQt* browser_window = (BrowserWindowQt*)browser_->window();
  location_entry_->model()->SetInputInProgress(true);
  if (GetTabContents()) {
    if (GetTabContents()->ShouldDisplayURL())
      location_entry_->SetUserText(ASCIIToUTF16(GetTabContents()->GetURL().spec()));
    else 
      location_entry_->SetUserText(string16());
  }

  focused_ = true;
  gfx::NativeView widget = browser_->GetSelectedTabContents()->GetContentNativeView();
  if (widget) {
    widget->setFocusPolicy(Qt::NoFocus);
  }
  // Update the keyword and search hint states.
//  OnChanged();
}

SkBitmap LocationBarViewQt::GetFavicon() const {
  return GetTabContents()->GetFavicon();
}

string16 LocationBarViewQt::GetTitle() const {
  if (!GetTabContents())
    return string16();
  return GetTabContents()->GetTitle();
}

bool LocationBarViewQt::IsTitleSet() const {
  if (!GetTabContents())
    return false;
  return GetTabContents()->IsTitleSet();
}

void LocationBarViewQt::ShowFirstRunBubble(FirstRun::BubbleType bubble_type) {
  // We need the browser window to be shown before we can show the bubble, but
  // we get called before that's happened.
  DNOTIMPLEMENTED();
}

std::wstring LocationBarViewQt::GetInputString() const {
  return location_input_;
}

WindowOpenDisposition LocationBarViewQt::GetWindowOpenDisposition() const {
  return disposition_;
}

PageTransition::Type LocationBarViewQt::GetPageTransition() const {
  return transition_;
}

void LocationBarViewQt::AcceptInput() {
  location_entry_->model()->AcceptInput(CURRENT_TAB, false);
}

void LocationBarViewQt::FocusLocation(bool select_all) {
  DNOTIMPLEMENTED();
}

void LocationBarViewQt::UpdateContentSettingsIcons() {
    DNOTIMPLEMENTED();
}

void LocationBarViewQt::UpdatePageActions() {
    DNOTIMPLEMENTED();
}

void LocationBarViewQt::OnAutocompleteAccept(const GURL& url,
    WindowOpenDisposition disposition,
    PageTransition::Type transition,
    const GURL& alternate_nav_url) {
  if (!url.is_valid())
    return;

  location_input_ = UTF8ToWide(url.spec());
  disposition_ = disposition;
  transition_ = transition;

  if (!command_updater_)
    return;

  if (!alternate_nav_url.is_valid()) {
    command_updater_->ExecuteCommand(IDC_OPEN_CURRENT_URL);
    return;
  }

  AlternateNavURLFetcher* fetcher =
      new AlternateNavURLFetcher(alternate_nav_url);
  // The AlternateNavURLFetcher will listen for the pending navigation
  // notification that will be issued as a result of the "open URL." It
  // will automatically install itself into that navigation controller.
  command_updater_->ExecuteCommand(IDC_OPEN_CURRENT_URL);
  if (fetcher->state() == AlternateNavURLFetcher::NOT_STARTED) {
    // I'm not sure this should be reachable, but I'm not also sure enough
    // that it shouldn't to stick in a NOTREACHED().  In any case, this is
    // harmless.
    delete fetcher;
  } else {
    // The navigation controller will delete the fetcher.
  }
}

//////////////////////////////////////////////////////////////////////////////////
