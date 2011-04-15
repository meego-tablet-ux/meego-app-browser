// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/autocomplete/autocomplete_edit_view_qt.h"

#include <algorithm>

#include "base/logging.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/autocomplete/autocomplete_edit.h"
#include "chrome/browser/autocomplete/autocomplete_match.h"
#include "chrome/browser/autocomplete/autocomplete_popup_model.h"
#include "chrome/browser/autocomplete/autocomplete_popup_view_qt.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/command_updater.h"
#include "chrome/browser/defaults.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "chrome/browser/ui/toolbar/toolbar_model.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"
#include "chrome/browser/ui/meegotouch/location_bar_view_qt.h"
#include "content/common/notification_service.h"
#include "googleurl/src/gurl.h"
#include "grit/generated_resources.h"
#include "net/base/escape.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/clipboard/scoped_clipboard_writer.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/font.h"
#include "ui/gfx/gtk_util.h"
#include "ui/gfx/skia_utils_gtk.h"

#include <QDeclarativeEngine>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDeclarativeItem>
#include <QGraphicsLineItem>
#include <QVariant>

namespace {
// Stores GTK+-specific state so it can be restored after switching tabs.
struct ViewState {
  explicit ViewState(const AutocompleteEditViewQt::CharRange& selection_range)
      : selection_range(selection_range) {
  }

  // Range of selected text.
  AutocompleteEditViewQt::CharRange selection_range;
};

struct AutocompleteEditState {
  AutocompleteEditState(const AutocompleteEditModel::State& model_state,
                        const ViewState& view_state)
      : model_state(model_state),
        view_state(view_state) {
  }

  const AutocompleteEditModel::State model_state;
  const ViewState view_state;
};

// Returns a lazily initialized property bag accessor for saving our state in a
// TabContents.
PropertyAccessor<AutocompleteEditState>* GetStateAccessor() {
  static PropertyAccessor<AutocompleteEditState> state;
  return &state;
}
} // namespace

class AutocompleteEditViewQtImpl: public QObject
{
  Q_OBJECT;
 public:
  AutocompleteEditViewQtImpl(AutocompleteEditViewQt* edit_view):
      QObject(NULL),
      edit_view_(edit_view),
      user_input_(true)
  {
  }

 public slots:
  void focusGained()
  {
    DLOG(INFO) << __PRETTY_FUNCTION__ ;
    edit_view_->HandleFocusIn();
  }

  void focusLost()
  {
    DLOG(INFO) << __PRETTY_FUNCTION__ ;
    edit_view_->HandleFocusOut();
  }
  

  void returnPressed()
  {
    DLOG(INFO) << __PRETTY_FUNCTION__ ;
    edit_view_->HandleEnterPressed();
  }

  void textChanged(QString text, bool isDelete)
  {
    DLOG(INFO) << __PRETTY_FUNCTION__ ;
    DLOG(INFO) << text.toStdString();

    is_just_delete_text_ = isDelete;
    text_ = text;
    if (user_input_)
    {
      string16 wtext = edit_view_->GetText();
      edit_view_->SetUserText(wtext, wtext, true);
    }
  }

  bool isJustDeleteText() 
  {
    return is_just_delete_text_;
  }

  signals:
  void setText(QString text);
  void setFocus();
  void setSelection(int start, int end);
  void selectAll();
  void setReadOnly(bool readonly);
  
  //call from chrome view
  public:
  QString GetText()
  {
    return text_;
  }
  
  void SetText(QString text, bool autocomplete)
  {
    user_input_ = autocomplete;
    emit setText(text);
    user_input_ = true;
  }

  void SetFocus()
  {
    DLOG(INFO) << __PRETTY_FUNCTION__ ;
    emit setFocus();
  }

  void SetSelection(int start, int end)
  {
    emit setSelection(start, end);
  }

  void SelectAll()
  {
    emit selectAll();
  }

  void SetReadOnly(bool readonly)
  {
    emit setReadOnly(readonly);
  }

 private:
  AutocompleteEditViewQt* edit_view_;
  QString text_;
  bool user_input_;
  string16 prev_text;
  bool is_just_delete_text_;
};

AutocompleteEditViewQt::AutocompleteEditViewQt(
                                               AutocompleteEditController* controller,
                                               ToolbarModel* toolbar_model,
                                               Profile* profile,
                                               CommandUpdater* command_updater,
                                               bool popup_window_mode,
                                               BrowserWindowQt* window
                                               ):
  window_(window),
  model_(new AutocompleteEditModel(this, controller, profile)),
  popup_view_(new AutocompletePopupViewQt(GetFont(), this, model_.get(), profile, window)),
  user_input_(true),
  controller_(controller),
  toolbar_model_(toolbar_model),
  command_updater_(command_updater),
  popup_window_mode_(popup_window_mode),
  security_level_(ToolbarModel::NONE),
  // mark_set_handler_id_(0),
  enter_was_pressed_(false),
  tab_was_pressed_(false),
  paste_clipboard_requested_(false),
  enter_was_inserted_(false),
  enable_tab_to_search_(true)
{
  impl_ = new AutocompleteEditViewQtImpl(this);

  QDeclarativeView* view = window->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();
  context->setContextProperty("autocompleteEditViewModel", impl_);
}

AutocompleteEditViewQt::~AutocompleteEditViewQt() {
  NotificationService::current()->Notify(
      NotificationType::AUTOCOMPLETE_EDIT_DESTROYED,
      Source<AutocompleteEditViewQt>(this),
      NotificationService::NoDetails());

  // Explicitly teardown members which have a reference to us.  Just to be safe
  // we want them to be destroyed before destroying any other internal state.
  popup_view_.reset();
  model_.reset();
  delete impl_;
}

void AutocompleteEditViewQt::Init() {
  reinterpret_cast<AutocompletePopupViewQt*>(popup_view_.get())->Init();
}

void AutocompleteEditViewQt::SetFocus() {
  impl_->SetFocus();
}

gfx::Font AutocompleteEditViewQt::GetFont() {
  return gfx::Font();
}

void AutocompleteEditViewQt::SaveStateToTab(TabContents* tab) {
   DCHECK(tab);
  // If any text has been selected, register it as the PRIMARY selection so it
  // can still be pasted via middle-click after the text view is cleared.
  if (!selected_text_.empty())
    SavePrimarySelection(selected_text_);
  // NOTE: GetStateForTabSwitch may affect GetSelection, so order is important.
  AutocompleteEditModel::State model_state = model_->GetStateForTabSwitch();
  GetStateAccessor()->SetProperty(
      tab->property_bag(),
      AutocompleteEditState(model_state, ViewState(GetSelection())));
}

void AutocompleteEditViewQt::Update(const TabContents* contents) {
  ///\todo: to implement the sophiscated state save/store
  // NOTE: We're getting the URL text here from the ToolbarModel.
  bool visibly_changed_permanent_text =
      model_->UpdatePermanentText(WideToUTF16Hack(toolbar_model_->GetText()));

  ToolbarModel::SecurityLevel security_level =
      toolbar_model_->GetSecurityLevel();
  bool changed_security_level = (security_level != security_level_);
  security_level_ = security_level;

  if (contents) {
    selected_text_.clear();
    RevertAll();
    const AutocompleteEditState* state =
        GetStateAccessor()->GetProperty(contents->property_bag());
    if (state) {
      model_->RestoreState(state->model_state);

      // Move the marks for the cursor and the other end of the selection to
      // the previously-saved offsets (but preserve PRIMARY).
      StartUpdatingHighlightedText();
      SetSelectedRange(state->view_state.selection_range);
      FinishUpdatingHighlightedText();
    }
  } else if (visibly_changed_permanent_text) {
    RevertAll();
    // TODO(deanm): There should be code to restore select all here.
  } else if (changed_security_level) {
    EmphasizeURLComponents();
  }

  // Disallow to change URL for chrome page, for tabs limit
  if (contents)
  {
    if ((contents->GetURL().SchemeIs("chrome") ||
         contents->GetURL().SchemeIs("chrome-extension")) &&
        contents->GetURL().HostNoBrackets() != "newtab")
    {
      impl_->SetReadOnly(true);
    }
    else
    {
      impl_->SetReadOnly(false);
    }
  }
}

void AutocompleteEditViewQt::OpenURL(const GURL& url,
                                      WindowOpenDisposition disposition,
                                      PageTransition::Type transition,
                                      const GURL& alternate_nav_url,
                                      size_t selected_line,
                                      const string16& keyword) {
  if (!url.is_valid())
    return;

  model_->OpenURL(url, disposition, transition, alternate_nav_url,
                  selected_line, keyword);
}

string16 AutocompleteEditViewQt::GetText() const {
  QString text = impl_->GetText();
  return WideToUTF16Hack(text.toStdWString());
}

bool AutocompleteEditViewQt::IsEditingOrEmpty() const {
  return model_->user_input_in_progress() ||
    impl_->GetText().isEmpty();  
}

int AutocompleteEditViewQt::GetIcon() const {
  return IsEditingOrEmpty() ?
      AutocompleteMatch::TypeToIcon(model_->CurrentTextType()) :
      toolbar_model_->GetIcon();
}

void AutocompleteEditViewQt::SetUserText(const string16& text,
                                          const string16& display_text,
                                          bool update_popup) {
    model_->SetUserText(text);
    // TODO(deanm): something about selection / focus change here.
    // avoid repeatedly emit textChanged signal
    QString str = QString::fromStdString(UTF16ToUTF8(display_text));
    if (str != impl_->GetText())
    {
      SetWindowTextAndCaretPos(display_text, display_text.length());
    }
    if (update_popup)
      UpdatePopup();
    TextChanged();
  }

void AutocompleteEditViewQt::SetWindowTextAndCaretPos(const string16& text,
                                                       size_t caret_pos) {
  CharRange range(static_cast<int>(caret_pos), static_cast<int>(caret_pos));
  SetTextAndSelectedRange(text, range);
}

void AutocompleteEditViewQt::SetForcedQuery() {
  const string16 current_text(GetText());
  const size_t start = current_text.find_first_not_of(kWhitespaceUTF16);
  if (start == string16::npos || (current_text[start] != '?')) {
    SetUserText(ASCIIToUTF16("?"));
  } else {
    StartUpdatingHighlightedText();
    SetSelectedRange(CharRange(current_text.size(), start + 1));
    FinishUpdatingHighlightedText();
  }
}

bool AutocompleteEditViewQt::IsSelectAll() {
  DNOTIMPLEMENTED();
  return false;
}

void AutocompleteEditViewQt::SelectAll(bool reversed) {
  // SelectAll() is invoked as a side effect of other actions (e.g.  switching
  // tabs or hitting Escape) in autocomplete_edit.cc, so we don't update the
  // PRIMARY selection here.
  DNOTIMPLEMENTED();
}

void AutocompleteEditViewQt::RevertAll() {
  ClosePopup();
  model_->Revert();
  TextChanged();
}

void AutocompleteEditViewQt::UpdatePopup() {
  model_->SetInputInProgress(true);
  if (!model_->has_focus())
  {
    DLOG(WARNING) << "no focus";
    return;
  }

  CharRange sel = GetSelection();
  bool no_inline_complete = impl_->isJustDeleteText();
  model_->StartAutocomplete(sel.cp_min != sel.cp_max, 
			    no_inline_complete || std::max(sel.cp_max, sel.cp_min) < GetTextLength());
}

void AutocompleteEditViewQt::ClosePopup() {
  model_->StopAutocomplete();
}

void AutocompleteEditViewQt::OnTemporaryTextMaybeChanged(
    const string16& display_text,
    bool save_original_selection) {
  if (save_original_selection)
    saved_temporary_selection_ = GetSelection();

  StartUpdatingHighlightedText();
  SetWindowTextAndCaretPos(display_text, display_text.length());
  FinishUpdatingHighlightedText();
  TextChanged();
}

bool AutocompleteEditViewQt::OnInlineAutocompleteTextMaybeChanged(
    const string16& display_text,
    size_t user_text_length) {
  if (display_text == GetText())
    return false;

  StartUpdatingHighlightedText();
  CharRange range(display_text.size(), user_text_length);
  SetTextAndSelectedRange(display_text, range);
  FinishUpdatingHighlightedText();
  TextChanged();
  return true;
}

void AutocompleteEditViewQt::OnRevertTemporaryText() {
  StartUpdatingHighlightedText();
  SetSelectedRange(saved_temporary_selection_);
  FinishUpdatingHighlightedText();
  TextChanged();
}

void AutocompleteEditViewQt::OnBeforePossibleChange() {
  // If this change is caused by a paste clipboard action and all text is
  // selected, then call model_->on_paste_replacing_all() to prevent inline
  // autocomplete.
  if (paste_clipboard_requested_) {
    paste_clipboard_requested_ = false;
    model_->on_paste();
  }

  // Record our state.
  text_before_change_ = GetText();
  sel_before_change_ = GetSelection();
}

// TODO(deanm): This is mostly stolen from Windows, and will need some work.
bool AutocompleteEditViewQt::OnAfterPossibleChange() {
  // If the change is caused by an Enter key press event, and the event was not
  // handled by IME, then it's an unexpected change and shall be reverted here.
  // {Start|Finish}UpdatingHighlightedText() are called here to prevent the
  // PRIMARY selection from being changed.
  if (enter_was_pressed_ && enter_was_inserted_) {
    StartUpdatingHighlightedText();
    SetTextAndSelectedRange(text_before_change_, sel_before_change_);
    FinishUpdatingHighlightedText();
    return false;
  }

  CharRange new_sel = GetSelection();
  int length = GetTextLength();
  bool selection_differs = (new_sel.cp_min != sel_before_change_.cp_min) ||
                           (new_sel.cp_max != sel_before_change_.cp_max);
  bool at_end_of_edit = (new_sel.cp_min == length && new_sel.cp_max == length);

  // See if the text or selection have changed since OnBeforePossibleChange().
  string16 new_text(GetText());
  text_changed_ = (new_text != text_before_change_);

  if (text_changed_)
    AdjustTextJustification();

  // When the user has deleted text, we don't allow inline autocomplete.  Make
  // sure to not flag cases like selecting part of the text and then pasting
  // (or typing) the prefix of that selection.  (We detect these by making
  // sure the caret, which should be after any insertion, hasn't moved
  // forward of the old selection start.)
  bool just_deleted_text =
      (text_before_change_.length() > new_text.length()) &&
      (new_sel.cp_min <= std::min(sel_before_change_.cp_min,
                                 sel_before_change_.cp_max));

  bool something_changed = model_->OnAfterPossibleChange(new_text,
      selection_differs, text_changed_, just_deleted_text, at_end_of_edit);

  // If only selection was changed, we don't need to call |controller_|'s
  // OnChanged() method, which is called in TextChanged().
  // But we still need to call EmphasizeURLComponents() to make sure the text
  // attributes are updated correctly.
  if (something_changed && text_changed_)
    TextChanged();
  else if (selection_differs)
    EmphasizeURLComponents();

  return something_changed;
}

gfx::NativeView AutocompleteEditViewQt::GetNativeView() const {
  return NULL;
}

CommandUpdater* AutocompleteEditViewQt::GetCommandUpdater() {
  return command_updater_;
}

void AutocompleteEditViewQt::Observe(NotificationType type,
                                      const NotificationSource& source,
                                      const NotificationDetails& details) {
  DNOTIMPLEMENTED();
}

void AutocompleteEditViewQt::HandleFocusIn()
{
  // assume no control key pressed
  model_->OnSetFocus(false);
  controller_->OnSetFocus();
  //
  impl_->SelectAll();
}

void AutocompleteEditViewQt::HandleFocusOut()
{
  // Close the popup.
  ClosePopup();
  // Tell the model to reset itself.
  model_->OnKillFocus();
  controller_->OnKillFocus();
}

void AutocompleteEditViewQt::HandleEnterPressed()
{
  model_->AcceptInput(CURRENT_TAB, false);
}

void AutocompleteEditViewQt::StartUpdatingHighlightedText() {
  DNOTIMPLEMENTED();
}

void AutocompleteEditViewQt::FinishUpdatingHighlightedText() {
  DNOTIMPLEMENTED();
}

AutocompleteEditViewQt::CharRange AutocompleteEditViewQt::GetSelection() {
  //  DNOTIMPLEMENTED();
  int start, end;
  QDeclarativeView* view = window_->DeclarativeView();
  QDeclarativeItem* item = view->rootObject()->findChild<QDeclarativeItem*>("urlTextInput");

  QString selectedText = item->property("selectedText").toString();
  if (!selectedText.isEmpty())
  {
    start = item->property("selectionStart").toInt();
    end = item->property("selectionEnd").toInt();
  } else {
    start = item->property("cursorPosition").toInt();
    end = start;
  }
  return CharRange(start, end);
}

int AutocompleteEditViewQt::GetTextLength() {
  return impl_->GetText().length();
}

void AutocompleteEditViewQt::EmphasizeURLComponents() {
  DNOTIMPLEMENTED();
}

void AutocompleteEditViewQt::TextChanged() {
  EmphasizeURLComponents();
  controller_->OnChanged();
}

void AutocompleteEditViewQt::SavePrimarySelection(
     const std::string& selected_text) {
  DNOTIMPLEMENTED();
 }

void AutocompleteEditViewQt::SetTextAndSelectedRange(const string16& text,
                                                     const CharRange& range) {
  if (text != GetText()) {
    impl_->SetText(QString::fromStdWString(UTF16ToWide(text)), false);
  }
  SetSelectedRange(range);
  AdjustTextJustification();
}

void AutocompleteEditViewQt::SetSelectedRange(const CharRange& range) {
  //  DNOTIMPLEMENTED();
  int start = std::min(range.cp_max, range.cp_min);
  int end = std::max(range.cp_max, range.cp_min);
  DLOG(INFO) << "SetSelectionRange : " << start << " : " << end;
  impl_->SetSelection(start,end);
}

void AutocompleteEditViewQt::AdjustTextJustification() {
  DNOTIMPLEMENTED();
}

#include "moc_autocomplete_edit_view_qt.cc"
