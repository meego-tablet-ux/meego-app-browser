// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/meegotouch/find_bar_qt.h"

#include "base/i18n/rtl.h"
#include "base/string_number_conversions.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "grit/generated_resources.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/find_bar/find_bar_controller.h"
#include "chrome/browser/ui/find_bar/find_bar_state.h"
#include "chrome/browser/ui/find_bar/find_tab_helper.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"
#include "chrome/browser/ui/meegotouch/tab_contents_container_qt.h"
#include "chrome/browser/ui/tab_contents/tab_contents_wrapper.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "chrome/browser/tab_contents/tab_contents_view_qt.h"
#include "chrome/browser/upgrade_detector.h"
#include "chrome/common/chrome_switches.h"
#include "content/common/notification_details.h"
#include "content/common/notification_service.h"
#include "content/common/notification_type.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

#include <QDeclarativeEngine>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDeclarativeItem>
#include <QGraphicsLineItem>
#include <launcherwindow.h>

class FindBarQtImpl: public QObject
{
  Q_OBJECT
 public:
  FindBarQtImpl(FindBarQt* findbar):
      QObject(NULL),
      find_bar_(findbar)
  {
  }

  void Show(bool animate)  
  {
    visible_ = true; 
    emit show(animate);
  } 
  void Hide()
  {
    visible_ = false;
    emit hide();
  }

  bool isVisible() const { return visible_; } 

  QString GetSearchText() const { return search_text_; } 
  void SetSearchText  (QString text) { emit setSearchText(text); }
  void SetMatchesLabel(QString text) { emit setMatchesLabel(text); }

  void SetX(int x)  
  {
    rect_.moveTo(x, rect_.y()); 
    emit setX(x); 
  }
  QRect GetRect() const { return rect_; }

 Q_SIGNALS:
  void show(bool animate);
  void hide();
//  void close();
  void setSearchText(QString text);
  void setMatchesLabel(QString text);
  void setX(int x);

 public slots:

  void textChanged(QString text)
  {
    search_text_ = text;
    find_bar_->OnChanged();
  }
  void positionUpdated(int x, int y, int width, int height) 
  { 
    rect_.setRect(x, y, width, height); 
  }
  void visibleChanged(bool visible) { visible_ = visible; }
  void closeButtonClicked(){ find_bar_->Close(); }
  void prevButtonClicked() { find_bar_->FindPrev(); }
  void nextButtonClicked() { find_bar_->FindNext(); }

 private:
  FindBarQt* find_bar_;
  QString search_text_;
  bool visible_;
  QRect rect_;
};

FindBarQt::FindBarQt(Browser* browser, BrowserWindowQt* window)
  : browser_(browser),
    window_(window),
    impl_(new FindBarQtImpl(this)),
    ignore_changed_signal_(false) 
{
  QDeclarativeView* view = window_->DeclarativeView(); 
  QDeclarativeContext *context = view->rootContext();
  context->setContextProperty("findBarModel", impl_);
}

FindBarQt::~FindBarQt() {
  delete  impl_;
}

void FindBarQt::Show(bool animate) {
  impl_->Show(animate);
}

void FindBarQt::Hide(bool animate) {
  impl_->Hide();
}

void FindBarQt::Close() {
  find_bar_controller_->EndFindSession(
	  FindBarController::kKeepSelection);
}

void FindBarQt::SetFocusAndSelection() {
  //Show(true);   ///\todo 1. remove hard code of animate state
                ///\todo 2. Is this the proper way to simply show for this function?
  // StoreOutsideFocus();
  // gtk_widget_grab_focus(text_entry_);
  // // Select all the text.
  // gtk_entry_select_region(GTK_ENTRY(text_entry_), 0, -1);
}

void FindBarQt::ClearResults(const FindNotificationDetails& results) {
  UpdateUIForFindResult(results, string16());
}

void FindBarQt::StopAnimation() {
  DNOTIMPLEMENTED();
  // slide_widget_->End();
}

void FindBarQt::MoveWindowIfNecessary(const gfx::Rect& selection_rect,
                                       bool no_redraw) {
  DNOTIMPLEMENTED();
  // Not moving the window on demand, so do nothing.
}

void FindBarQt::SetFindText(const string16& find_text) {
  std::wstring find_text_wide = UTF16ToWide(find_text);

  // Ignore the "changed" signal handler because programatically setting the
  // text should not fire a "changed" event.
  ignore_changed_signal_ = true;
  impl_->SetSearchText(QString::fromStdWString(find_text_wide));
  //text_entry_->setText(QString::fromStdWString(find_text_wide));
  //  gtk_entry_set_text(GTK_ENTRY(text_entry_), find_text_utf8.c_str());
  ignore_changed_signal_ = false;
}

void FindBarQt::UpdateUIForFindResult(const FindNotificationDetails& result,
                                      const string16& find_text) {
  //DNOTIMPLEMENTED();
  if (!result.selection_rect().IsEmpty()) {
    selection_rect_ = result.selection_rect();
    DLOG(INFO) << "selection_rect : " << selection_rect_.x() << " , " << selection_rect_.y();
    QRect rect = impl_->GetRect();
    DLOG(INFO) << "overlay_ : " << rect.x() << " , " << rect.y();
    
    int xposition = GetDialogPosition(result.selection_rect()).x();
    DLOG(INFO) << "xposition: " << xposition;
    if (xposition != rect.x())
    {
     //       Reposition();
      impl_->SetX(xposition);
    }

  }

  // // Once we find a match we no longer want to keep track of what had
  // // focus. EndFindSession will then set the focus to the page content.
  // if (result.number_of_matches() > 0)
  //   focus_store_.Store(NULL);

  std::wstring find_text_wide = UTF16ToWide(find_text);
  bool have_valid_range =
      result.number_of_matches() != -1 && result.active_match_ordinal() != -1;

  std::wstring entry_text(impl_->GetSearchText().toStdWString());
  if (entry_text != find_text_wide) {
    SetFindText(find_text);
    //    gtk_entry_select_region(GTK_ENTRY(text_entry_), 0, -1);
    //text_entry_->setSelection(0, -1);
  }

  if (!find_text.empty() && have_valid_range) {
    impl_->SetMatchesLabel(QString::fromUtf16(l10n_util::GetStringFUTF16(IDS_FIND_IN_PAGE_COUNT,
                                                              base::IntToString16(result.active_match_ordinal()),
                                                              base::IntToString16(result.number_of_matches())).c_str()));
  } else {
    // If there was no text entered, we don't show anything in the result count
    // area.
    impl_->SetMatchesLabel(" ");
    UpdateMatchLabelAppearance(false);
  }
}

void FindBarQt::AudibleAlert() {
  DNOTIMPLEMENTED();
  // This call causes a lot of weird bugs, especially when using the custom
  // frame. TODO(estade): if people complain, re-enable it. See
  // http://crbug.com/27635 and others.
  //
  //   gtk_widget_error_bell(widget());
}

gfx::Rect FindBarQt::GetDialogPosition(gfx::Rect avoid_overlapping_rect) {
  bool ltr = !base::i18n::IsRTL();
  // 15 is the size of the scrollbar, copied from ScrollbarThemeChromium.
  // The height is not used.
  // At very low browser widths we can wind up with a negative |dialog_bounds|
  // width, so clamp it to 0.
  QRect wr = window_->window()->geometry();
  gfx::Rect dialog_bounds = gfx::Rect(ltr ? 0 : 15, 0,
     std::max(0, wr.width() - (ltr ? 15 : 0)), 0);

  // GtkRequisition req;
  // gtk_widget_size_request(container_, &req);
  QRect rect = impl_->GetRect();
  gfx::Size prefsize(rect.width(), rect.height());

  gfx::Rect view_location(
       ltr ? dialog_bounds.width() - prefsize.width() : dialog_bounds.x(),
       dialog_bounds.y(), prefsize.width(), prefsize.height());
  gfx::Rect new_pos = FindBarController::GetLocationForFindbarView(
       view_location, dialog_bounds, avoid_overlapping_rect);

  return new_pos;
}

bool FindBarQt::IsFindBarVisible() {
  return impl_->isVisible();
//  return overlay_->isOnDisplay();
}

void FindBarQt::RestoreSavedFocus() {
  DNOTIMPLEMENTED();
  // // This function sometimes gets called when we don't have focus. We should do
  // // nothing in this case.
  // if (!gtk_widget_is_focus(text_entry_))
  //   return;

  // if (focus_store_.widget())
  //   gtk_widget_grab_focus(focus_store_.widget());
  // else
  //   find_bar_controller_->tab_contents()->Focus();
}

FindBarTesting* FindBarQt::GetFindBarTesting() {
  return this;
}

bool FindBarQt::GetFindBarWindowInfo(gfx::Point* position,
                                      bool* fully_visible) {
  DNOTIMPLEMENTED();
  // if (position)
  //   *position = GetPosition();

  // if (fully_visible) {
  //   *fully_visible = !slide_widget_->IsAnimating() &&
  //                    slide_widget_->IsShowing();
  // }
  return true;
}

string16 FindBarQt::GetFindText() {
  std::wstring contents = impl_->GetSearchText().toStdWString();
  return WideToUTF16(contents);
//  std::wstring contents(text_entry_->text().toStdWString());
//  // std::string contents(gtk_entry_get_text(GTK_ENTRY(text_entry_)));
//  return WideToUTF16(contents);
}

void FindBarQt::FindEntryTextInContents(bool forward_search) {
  TabContentsWrapper* tab_contents = find_bar_controller_->tab_contents();
  if (!tab_contents)
    return;

  FindTabHelper* find_tab_helper = tab_contents->find_tab_helper();

  std::wstring new_contents = impl_->GetSearchText().toStdWString();

  if (new_contents.length() > 0) {
    find_tab_helper->StartFinding(WideToUTF16(new_contents), forward_search,
                               false);  // Not case sensitive.
  } else {
    // The textbox is empty so we reset.
    find_tab_helper->StopFinding(FindBarController::kClearSelection);
    UpdateUIForFindResult(find_tab_helper->find_result(), string16());

    // Clearing the text box should also clear the prepopulate state so that
    // when we close and reopen the Find box it doesn't show the search we
    // just deleted.
    FindBarState* find_bar_state = browser_->profile()->GetFindBarState();
    find_bar_state->set_last_prepopulate_text(string16());
  }
}

void FindBarQt::UpdateMatchLabelAppearance(bool failure) {
  DNOTIMPLEMENTED();
}

void FindBarQt::Reposition() {
  if (!IsFindBarVisible())
    return;

  DNOTIMPLEMENTED();
  // // This will trigger an allocate, which allows us to reposition.
  // if (widget()->parent)
  //   gtk_widget_queue_resize(widget()->parent);
}

void FindBarQt::StoreOutsideFocus() {
  DNOTIMPLEMENTED();
  // // |text_entry_| is the only widget in the find bar that can be focused,
  // // so it's the only one we have to check.
  // // TODO(estade): when we make the find bar buttons focusable, we'll have
  // // to change this (same above in RestoreSavedFocus).
  // if (!gtk_widget_is_focus(text_entry_))
  //   focus_store_.Store(text_entry_);
}

void FindBarQt::AdjustTextAlignment() {
  DNOTIMPLEMENTED();
}

gfx::Point FindBarQt::GetPosition() {
  DNOTIMPLEMENTED();
//  gfx::Point point;
//
//  return point;
}

// static
void FindBarQt::OnChanged() {
  AdjustTextAlignment();

  if (!ignore_changed_signal_)
    FindEntryTextInContents(true);

  return;
}

#include "moc_find_bar_qt.cc"
