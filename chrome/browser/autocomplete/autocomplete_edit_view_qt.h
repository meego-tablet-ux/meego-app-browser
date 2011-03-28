// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_AUTOCOMPLETE_AUTOCOMPLETE_EDIT_VIEW_QT_H_
#define CHROME_BROWSER_AUTOCOMPLETE_AUTOCOMPLETE_EDIT_VIEW_QT_H_

#include <algorithm>
#include <string>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "base/string_util.h"
#include "chrome/browser/autocomplete/autocomplete_edit_view.h"
#include "chrome/browser/ui/toolbar/toolbar_model.h"
#include "content/common/notification_observer.h"
#include "content/common/notification_registrar.h"
#include "content/common/page_transition_types.h"
#include "ui/gfx/rect.h"
#include "webkit/glue/window_open_disposition.h"


class AutocompleteEditController;
class AutocompleteEditModel;
class AutocompletePopupView;
class Profile;
class TabContents;
class BrowserWindowQt;

namespace gfx {
class Font;
}

namespace views {
class View;
}

class AutocompleteEditViewQtImpl;

class AutocompleteEditViewQt : public AutocompleteEditView,
                               public NotificationObserver {
 public:
  // Modeled like the Windows CHARRANGE.  Represent a pair of cursor position
  // offsets.  Since GtkTextIters are invalid after the buffer is changed, we
  // work in character offsets (not bytes).
  struct CharRange {
    CharRange() : cp_min(0), cp_max(0) { }
    CharRange(int n, int x) : cp_min(n), cp_max(x) { }

    // Returns the start of the selection.
    int selection_min() const { return std::min(cp_min, cp_max); }

    // Work in integers to match the gint GTK APIs.
    int cp_min;  // For a selection: Represents the start.
    int cp_max;  // For a selection: Represents the end (insert position).
  };

  AutocompleteEditViewQt(AutocompleteEditController* controller,
                          ToolbarModel* toolbar_model,
                          Profile* profile,
                          CommandUpdater* command_updater,
                          bool popup_window_mode,
                          BrowserWindowQt* window);

  ~AutocompleteEditViewQt();

  // Initialize, create the underlying widgets, etc.
  void Init();

  // Implement the AutocompleteEditView interface.
  virtual AutocompleteEditModel* model() { return model_.get(); }
  virtual const AutocompleteEditModel* model() const { return model_.get(); }

  virtual void SaveStateToTab(TabContents* tab);

  virtual void Update(const TabContents* tab_for_state_restoring);

  virtual void OpenURL(const GURL& url,
                       WindowOpenDisposition disposition,
                       PageTransition::Type transition,
                       const GURL& alternate_nav_url,
                       size_t selected_line,
                       const string16& keyword);

  virtual string16 GetText() const;

  virtual bool IsEditingOrEmpty() const;
  virtual int GetIcon() const;

  virtual void SetUserText(const string16& text) {
    SetUserText(text, text, true);
  }
  virtual void SetUserText(const string16& text,
                           const string16& display_text,
                           bool update_popup);

  virtual void SetWindowTextAndCaretPos(const string16& text,
                                        size_t caret_pos);

  virtual void SetForcedQuery();

  virtual bool IsSelectAll();
  virtual bool DeleteAtEndPressed() {DNOTIMPLEMENTED();};
  virtual void SelectAll(bool reversed);
  virtual void RevertAll();

  virtual void UpdatePopup();
  virtual void ClosePopup();

  virtual void SetFocus();

  virtual void OnTemporaryTextMaybeChanged(const string16& display_text,
                                           bool save_original_selection);
  virtual bool OnInlineAutocompleteTextMaybeChanged(
      const string16& display_text, size_t user_text_length);
  virtual void OnRevertTemporaryText();
  virtual void OnBeforePossibleChange();
  virtual bool OnAfterPossibleChange();
  virtual gfx::NativeView GetNativeView() const;
  virtual CommandUpdater* GetCommandUpdater();
  virtual void SetInstantSuggestion(const string16& suggestion,
                                    bool animate_to_complete) {DNOTIMPLEMENTED();};
  virtual string16 GetInstantSuggestion() const {DNOTIMPLEMENTED();};
  virtual int TextWidth() const {DNOTIMPLEMENTED();};
  virtual bool IsImeComposing() const {DNOTIMPLEMENTED();};

  // Overridden from NotificationObserver:
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

  void SetBaseColor();

  // Used by LocationBarViewGtk to inform AutocompleteEditViewGtk if the tab to
  // search should be enabled or not. See the comment of |enable_tab_to_search_|
  // for details.
  void set_enable_tab_to_search(bool enable) {
    enable_tab_to_search_ = enable;
  }
  void HandleFocusIn();
  void HandleFocusOut();
  void HandleEnterPressed();

  bool IsUserInput() { return user_input_;};
  virtual void GetSelectionBounds(string16::size_type* start,
                                  string16::size_type* end) {DNOTIMPLEMENTED(); return;};

 private:
  BrowserWindowQt* window_;
  AutocompleteEditViewQtImpl* impl_;

  // Get ready to update |text_buffer_|'s highlighting without making changes to
  // the PRIMARY selection.  Removes the clipboard from |text_buffer_| and
  // blocks the "mark-set" signal handler.
  void StartUpdatingHighlightedText();

  // Finish updating |text_buffer_|'s highlighting such that future changes will
  // automatically update the PRIMARY selection.  Undoes
  // StartUpdatingHighlightedText()'s changes.
  void FinishUpdatingHighlightedText();

  // Get the character indices of the current selection.  This honors
  // direction, cp_max is the insertion point, and cp_min is the bound.
  CharRange GetSelection();

  /* // Translate from character positions to iterators for the current buffer. */
  /* void ItersFromCharRange(const CharRange& range, */
  /*                         GtkTextIter* iter_min, */
  /*                         GtkTextIter* iter_max); */

  // Return the number of characers in the current buffer.
  int GetTextLength();

  // Try to parse the current text as a URL and colorize the components.
  void EmphasizeURLComponents();

  // Internally invoked whenever the text changes in some way.
  void TextChanged();

  // Returns the font used in |text_view_|.
  gfx::Font GetFont();

  // Save |selected_text| as the PRIMARY X selection. Unlike
  // OwnPrimarySelection(), this won't set an owner or use callbacks.
  void SavePrimarySelection(const std::string& selected_text);

  // Update the field with |text| and set the selection.
  void SetTextAndSelectedRange(const string16& text,
                               const CharRange& range);

  // Set the selection to |range|.
  void SetSelectedRange(const CharRange& range);

  // Adjust the text justification according to the text direction of the widget
  // and |text_buffer_|'s content, to make sure the real text justification is
  // always in sync with the UI language direction.
  void AdjustTextJustification();

  // indicate whether it is caused by user input or autocomplete
  bool user_input_;

  scoped_ptr<AutocompleteEditModel> model_;
  scoped_ptr<AutocompletePopupView> popup_view_;
  AutocompleteEditController* controller_;
  ToolbarModel* toolbar_model_;

  // The object that handles additional command functionality exposed on the
  // edit, such as invoking the keyword editor.
  CommandUpdater* command_updater_;

  // When true, the location bar view is read only and also is has a slightly
  // different presentation (smaller font size). This is used for popups.
  bool popup_window_mode_;

  ToolbarModel::SecurityLevel security_level_;

  // Selection at the point where the user started using the
  // arrows to move around in the popup.
  CharRange saved_temporary_selection_;

  // Tracking state before and after a possible change.
  string16 text_before_change_;
  CharRange sel_before_change_;

  // The most-recently-selected text from the entry that was copied to the
  // clipboard.  This is updated on-the-fly as the user selects text. This may
  // differ from the actual selected text, such as when 'http://' is prefixed to
  // the text.  It is used in cases where we need to make the PRIMARY selection
  // persist even after the user has unhighlighted the text in the view
  // (e.g. when they highlight some text and then click to unhighlight it, we
  // pass this string to SavePrimarySelection()).
  std::string selected_text_;

  // When we own the X clipboard, this is the text for it.
  std::string primary_selection_text_;

  // IDs of the signal handlers for "mark-set" on |text_buffer_|.
  /* gulong mark_set_handler_id_; */
  /* gulong mark_set_handler_id2_; */


  // Indicates if Enter key was pressed.
  //
  // It's used in the key press handler to detect an Enter key press event
  // during sync dispatch of "end-user-action" signal so that an unexpected
  // change caused by the event can be ignored in OnAfterPossibleChange().
  bool enter_was_pressed_;

  // Indicates if Tab key was pressed.
  //
  // It's only used in the key press handler to detect a Tab key press event
  // during sync dispatch of "move-focus" signal.
  bool tab_was_pressed_;

  // Indicates that user requested to paste clipboard.
  // The actual paste clipboard action might be performed later if the
  // clipboard is not empty.
  bool paste_clipboard_requested_;

  // Indicates if an Enter key press is inserted as text.
  // It's used in the key press handler to determine if an Enter key event is
  // handled by IME or not.
  bool enter_was_inserted_;

  // Indicates whether the IME changed the text.  It's possible for the IME to
  // handle a key event but not change the text contents (e.g., when pressing
  // shift+del with no selection).
  bool text_changed_;

  // Contains the character range that should have a strikethrough (used for
  // insecure schemes). If the range is size one or less, no strikethrough
  // is needed.
  CharRange strikethrough_;

  // Indicate if the tab to search should be enabled or not. It's true by
  // default and will only be set to false if the location bar view is not able
  // to show the tab to search hint.
  bool enable_tab_to_search_;

  DISALLOW_COPY_AND_ASSIGN(AutocompleteEditViewQt);
};

#endif  // CHROME_BROWSER_AUTOCOMPLETE_AUTOCOMPLETE_EDIT_VIEW_QT_H_
