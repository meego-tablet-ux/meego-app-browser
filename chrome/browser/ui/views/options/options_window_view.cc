// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/options/options_window.h"
#include "chrome/browser/ui/views/options/advanced_page_view.h"
#include "chrome/browser/ui/views/options/content_page_view.h"
#include "chrome/browser/ui/views/options/general_page_view.h"
#include "chrome/browser/ui/window_sizer.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/pref_names.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "grit/locale_settings.h"
#include "ui/base/accessibility/accessible_view_state.h"
#include "ui/base/l10n/l10n_util.h"
#include "views/controls/tabbed_pane/tabbed_pane.h"
#include "views/widget/root_view.h"
#include "views/window/dialog_delegate.h"
#include "views/window/window.h"

///////////////////////////////////////////////////////////////////////////////
// OptionsWindowView
//
//  The contents of the Options dialog window.
//
class OptionsWindowView : public views::View,
                          public views::DialogDelegate,
                          public views::TabbedPane::Listener {
 public:
  explicit OptionsWindowView(Profile* profile);
  virtual ~OptionsWindowView();

  // Shows the Tab corresponding to the specified OptionsPage.
  void ShowOptionsPage(OptionsPage page, OptionsGroup highlight_group);

  // views::DialogDelegate implementation:
  virtual int GetDialogButtons() const OVERRIDE {
    return MessageBoxFlags::DIALOGBUTTON_CANCEL;
  }
  virtual std::wstring GetWindowTitle() const OVERRIDE;
  virtual std::wstring GetWindowName() const OVERRIDE;
  virtual void WindowClosing() OVERRIDE;
  virtual bool Cancel() OVERRIDE;
  virtual views::View* GetContentsView() OVERRIDE;
  virtual bool ShouldRestoreWindowSize() const OVERRIDE;

  // views::TabbedPane::Listener implementation:
  virtual void TabSelectedAt(int index) OVERRIDE;

  // views::View overrides:
  virtual void GetAccessibleState(ui::AccessibleViewState* state) OVERRIDE;
  virtual void Layout() OVERRIDE;
  virtual gfx::Size GetPreferredSize() OVERRIDE;

 protected:
  // views::View overrides:
  virtual void ViewHierarchyChanged(bool is_add,
                                    views::View* parent,
                                    views::View* child) OVERRIDE;
 private:
  // Init the assorted Tabbed pages
  void Init();

  // Returns the currently selected OptionsPageView.
  OptionsPageView* GetCurrentOptionsPageView() const;

  // The Tab view that contains all of the options pages.
  views::TabbedPane* tabs_;

  // The Profile associated with these options.
  Profile* profile_;

  // The last page the user was on when they opened the Options window.
  IntegerPrefMember last_selected_page_;

  DISALLOW_COPY_AND_ASSIGN(OptionsWindowView);
};

// static
static OptionsWindowView* instance_ = NULL;
static const int kDialogPadding = 7;

///////////////////////////////////////////////////////////////////////////////
// OptionsWindowView, public:

OptionsWindowView::OptionsWindowView(Profile* profile)
      // Always show preferences for the original profile. Most state when off
      // the record comes from the original profile, but we explicitly use
      // the original profile to avoid potential problems.
    : profile_(profile->GetOriginalProfile()) {
  // We don't need to observe changes in this value.
  last_selected_page_.Init(prefs::kOptionsWindowLastTabIndex,
                           g_browser_process->local_state(), NULL);
}

OptionsWindowView::~OptionsWindowView() {
}

void OptionsWindowView::ShowOptionsPage(OptionsPage page,
                                        OptionsGroup highlight_group) {
  // Positioning is handled by window_delegate. we just need to show the window.
  // This will show invisible windows and bring visible windows to the front.
  window()->Show();

  if (page == OPTIONS_PAGE_DEFAULT) {
    // Remember the last visited page from local state.
    page = static_cast<OptionsPage>(last_selected_page_.GetValue());
    if (page == OPTIONS_PAGE_DEFAULT)
      page = OPTIONS_PAGE_GENERAL;
  }
  // If the page number is out of bounds, reset to the first tab.
  if (page < 0 || page >= tabs_->GetTabCount())
    page = OPTIONS_PAGE_GENERAL;

  tabs_->SelectTabAt(static_cast<int>(page));

  GetCurrentOptionsPageView()->HighlightGroup(highlight_group);
}

///////////////////////////////////////////////////////////////////////////////
// OptionsWindowView, views::DialogDelegate implementation:

std::wstring OptionsWindowView::GetWindowTitle() const {
  return UTF16ToWide(
      l10n_util::GetStringFUTF16(IDS_OPTIONS_DIALOG_TITLE,
                                 l10n_util::GetStringUTF16(IDS_PRODUCT_NAME)));
}

std::wstring OptionsWindowView::GetWindowName() const {
  return UTF8ToWide(prefs::kPreferencesWindowPlacement);
}

void OptionsWindowView::WindowClosing() {
  // Clear the static instance so that the next time ShowOptionsWindow() is
  // called a new window is opened.
  instance_ = NULL;
}

bool OptionsWindowView::Cancel() {
  return GetCurrentOptionsPageView()->CanClose();
}

views::View* OptionsWindowView::GetContentsView() {
  return this;
}

bool OptionsWindowView::ShouldRestoreWindowSize() const {
  // By returning false the options window is always sized to its preferred
  // size.
  return false;
}

///////////////////////////////////////////////////////////////////////////////
// OptionsWindowView, views::TabbedPane::Listener implementation:

void OptionsWindowView::TabSelectedAt(int index) {
  DCHECK(index > OPTIONS_PAGE_DEFAULT && index < OPTIONS_PAGE_COUNT);
  last_selected_page_.SetValue(index);
}

///////////////////////////////////////////////////////////////////////////////
// OptionsWindowView, views::View overrides:

void OptionsWindowView::GetAccessibleState(ui::AccessibleViewState* state) {
  state->role = ui::AccessibilityTypes::ROLE_CLIENT;
}

void OptionsWindowView::Layout() {
  tabs_->SetBounds(kDialogPadding, kDialogPadding,
                   width() - (2 * kDialogPadding),
                   height() - (2 * kDialogPadding));
}

gfx::Size OptionsWindowView::GetPreferredSize() {
  gfx::Size size(tabs_->GetPreferredSize());
  size.Enlarge(2 * kDialogPadding, 2 * kDialogPadding);
  return size;
}

void OptionsWindowView::ViewHierarchyChanged(bool is_add,
                                             views::View* parent,
                                             views::View* child) {
  // Can't init before we're inserted into a Container, because we require a
  // HWND to parent native child controls to.
  if (is_add && child == this)
    Init();
}

///////////////////////////////////////////////////////////////////////////////
// OptionsWindowView, private:

void OptionsWindowView::Init() {
  tabs_ = new views::TabbedPane;
  tabs_->SetAccessibleName(l10n_util::GetStringFUTF16(
      IDS_OPTIONS_DIALOG_TITLE,
      l10n_util::GetStringUTF16(IDS_PRODUCT_NAME)));
  tabs_->SetListener(this);
  AddChildView(tabs_);

  int tab_index = 0;
  GeneralPageView* general_page = new GeneralPageView(profile_);
  tabs_->AddTabAtIndex(
      tab_index++,
      UTF16ToWide(l10n_util::GetStringUTF16(IDS_OPTIONS_GENERAL_TAB_LABEL)),
      general_page, false);

  ContentPageView* content_page = new ContentPageView(profile_);
  tabs_->AddTabAtIndex(
      tab_index++,
      UTF16ToWide(l10n_util::GetStringUTF16(IDS_OPTIONS_CONTENT_TAB_LABEL)),
      content_page, false);

  AdvancedPageView* advanced_page = new AdvancedPageView(profile_);
  tabs_->AddTabAtIndex(
      tab_index++,
      UTF16ToWide(l10n_util::GetStringUTF16(IDS_OPTIONS_ADVANCED_TAB_LABEL)),
      advanced_page, false);

  // Bind the profile to the window so that the ChromeViewsDelegate can find
  // the user preferences to store and retrieve window placement settings.
  window()->SetNativeWindowProperty(Profile::kProfileKey, profile_);

  DCHECK(tabs_->GetTabCount() == OPTIONS_PAGE_COUNT);
}

OptionsPageView* OptionsWindowView::GetCurrentOptionsPageView() const {
  return static_cast<OptionsPageView*>(tabs_->GetSelectedTab());
}

///////////////////////////////////////////////////////////////////////////////
// Factory/finder method:

void ShowOptionsWindow(OptionsPage page,
                       OptionsGroup highlight_group,
                       Profile* profile) {
  DCHECK(profile);
  // If there's already an existing options window, activate it and switch to
  // the specified page.
  // TODO(beng): note this is not multi-simultaneous-profile-safe. When we care
  //             about this case this will have to be fixed.
  if (!instance_) {
    instance_ = new OptionsWindowView(profile);
    views::Window::CreateChromeWindow(NULL, gfx::Rect(), instance_);
  }
  instance_->ShowOptionsPage(page, highlight_group);
}
