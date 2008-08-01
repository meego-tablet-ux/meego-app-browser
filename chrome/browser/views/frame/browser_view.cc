// Copyright 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "chrome/browser/views/frame/browser_view.h"

#include "chrome/browser/browser.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/tab_contents.h"
#include "chrome/browser/tabs/tab_strip.h"
#include "chrome/browser/view_ids.h"
#include "chrome/browser/views/bookmark_bar_view.h"
#include "chrome/browser/views/go_button.h"
#include "chrome/browser/views/location_bar_view.h"
#include "chrome/browser/views/status_bubble.h"
#include "chrome/browser/views/toolbar_star_toggle.h"
#include "chrome/browser/views/toolbar_view.h"
#include "chrome/common/l10n_util.h"
#include "generated_resources.h"

// Status Bubble metrics.
static const int kStatusBubbleHeight = 20;
static const int kStatusBubbleOffset = 2;

///////////////////////////////////////////////////////////////////////////////
// BrowserView, public:

BrowserView::BrowserView(BrowserWindow* frame,
                         Browser* browser,
                         ChromeViews::Window* window,
                         ChromeViews::View* contents_view)
    : frame_(frame),
      browser_(browser),
      initialized_(false)
/*                     ,
      ClientView(window, contents_view) */ {
}

BrowserView::~BrowserView() {
}

void BrowserView::LayoutStatusBubble(int status_bubble_y) {
  status_bubble_->SetBounds(kStatusBubbleOffset,
                            status_bubble_y - kStatusBubbleHeight +
                            kStatusBubbleOffset,
                            GetWidth() / 3,
                            kStatusBubbleHeight);
}

///////////////////////////////////////////////////////////////////////////////
// BrowserView, BrowserWindow implementation:

void BrowserView::Init() {
  SetAccessibleName(l10n_util::GetString(IDS_PRODUCT_NAME));

  toolbar_ = new BrowserToolbarView(browser_->controller(), browser_);
  AddChildView(toolbar_);
  toolbar_->SetID(VIEW_ID_TOOLBAR);
  toolbar_->Init(browser_->profile());
  toolbar_->SetAccessibleName(l10n_util::GetString(IDS_ACCNAME_TOOLBAR));

  status_bubble_.reset(new StatusBubble(GetViewContainer()));
}

void BrowserView::Show(int command, bool adjust_to_fit) {
  frame_->Show(command, adjust_to_fit);
}

void BrowserView::BrowserDidPaint(HRGN region) {
  frame_->BrowserDidPaint(region);
}

void BrowserView::Close() {
  frame_->Close();
}

void* BrowserView::GetPlatformID() {
  return frame_->GetPlatformID();
}

TabStrip* BrowserView::GetTabStrip() const {
  return frame_->GetTabStrip();
}

StatusBubble* BrowserView::GetStatusBubble() {
  return status_bubble_.get();
}

ChromeViews::RootView* BrowserView::GetRootView() {
  return frame_->GetRootView();
}

void BrowserView::ShelfVisibilityChanged() {
  frame_->ShelfVisibilityChanged();
}

void BrowserView::SelectedTabToolbarSizeChanged(bool is_animating) {
  frame_->SelectedTabToolbarSizeChanged(is_animating);
}

void BrowserView::UpdateTitleBar() {
  frame_->UpdateTitleBar();
}

void BrowserView::SetWindowTitle(const std::wstring& title) {
  frame_->SetWindowTitle(title);
}

void BrowserView::Activate() {
  frame_->Activate();
}

void BrowserView::FlashFrame() {
  frame_->FlashFrame();
}

void BrowserView::ShowTabContents(TabContents* contents) {
  frame_->ShowTabContents(contents);
}

void BrowserView::ContinueDetachConstrainedWindowDrag(
    const gfx::Point& mouse_pt,
    int frame_component) {
  frame_->ContinueDetachConstrainedWindowDrag(mouse_pt, frame_component);
}

void BrowserView::SizeToContents(const gfx::Rect& contents_bounds) {
  frame_->SizeToContents(contents_bounds);
}

void BrowserView::SetAcceleratorTable(
    std::map<ChromeViews::Accelerator, int>* accelerator_table) {
  frame_->SetAcceleratorTable(accelerator_table);
}

void BrowserView::ValidateThrobber() {
  frame_->ValidateThrobber();
}

gfx::Rect BrowserView::GetNormalBounds() {
  return frame_->GetNormalBounds();
}

bool BrowserView::IsMaximized() {
  return frame_->IsMaximized();
}

gfx::Rect BrowserView::GetBoundsForContentBounds(const gfx::Rect content_rect) {
  return frame_->GetBoundsForContentBounds(content_rect);
}

void BrowserView::DetachFromBrowser() {
  frame_->DetachFromBrowser();
}

void BrowserView::InfoBubbleShowing() {
  frame_->InfoBubbleShowing();
}

void BrowserView::InfoBubbleClosing() {
  frame_->InfoBubbleClosing();
}

ToolbarStarToggle* BrowserView::GetStarButton() const {
  return toolbar_->star_button();
}

LocationBarView* BrowserView::GetLocationBarView() const {
  return toolbar_->GetLocationBarView();
}

GoButton* BrowserView::GetGoButton() const {
  return toolbar_->GetGoButton();
}

BookmarkBarView* BrowserView::GetBookmarkBarView() {
  return frame_->GetBookmarkBarView();
}

BrowserView* BrowserView::GetBrowserView() const {
  return NULL;
}

void BrowserView::Update(TabContents* contents, bool should_restore_state) {
  toolbar_->Update(contents, should_restore_state);
}

void BrowserView::ProfileChanged(Profile* profile) {
  toolbar_->SetProfile(profile);
}

void BrowserView::FocusToolbar() {
  toolbar_->RequestFocus();
}

void BrowserView::DestroyBrowser() {
  frame_->DestroyBrowser();
}

///////////////////////////////////////////////////////////////////////////////
// BrowserView, ChromeViews::ClientView overrides:

/*
bool BrowserView::CanClose() const {
  return true;
}

int BrowserView::NonClientHitTest(const gfx::Point& point) {
  return HTCLIENT;
}
*/

///////////////////////////////////////////////////////////////////////////////
// BrowserView, ChromeViews::View overrides:

void BrowserView::Layout() {
  toolbar_->SetBounds(0, 0, GetWidth(), GetHeight());
}

void BrowserView::DidChangeBounds(const CRect& previous,
                                  const CRect& current) {
  Layout();
}

void BrowserView::ViewHierarchyChanged(bool is_add,
                                       ChromeViews::View* parent,
                                       ChromeViews::View* child) {
  if (is_add && child == this && GetViewContainer() && !initialized_) {
    Init();
    // Make sure not to call Init() twice if we get inserted into a different
    // ViewContainer.
    initialized_ = true;
  }
}
