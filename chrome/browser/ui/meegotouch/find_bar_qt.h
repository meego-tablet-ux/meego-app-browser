// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEEGOTOUCH_FIND_BAR_QT_H_
#define CHROME_BROWSER_MEEGOTOUCH_FIND_BAR_QT_H_

#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "base/logging.h"
#include "chrome/browser/ui/find_bar/find_bar.h"
#include "content/common/notification_observer.h"
#include "content/common/notification_registrar.h"

class Browser;
class BrowserWindowQt;
class FindBarQt;
class FindBarQtImpl;

// View class that displays the GTK version of the toolbar and routes gtk
// events back to the Browser.
class FindBarQt : public FindBar,
                  public FindBarTesting {
 public:
  explicit FindBarQt(Browser* browser, BrowserWindowQt* window);
  virtual ~FindBarQt();

  // Methods from FindBar.
  virtual FindBarController* GetFindBarController() const {
    return find_bar_controller_;
  }
  virtual void SetFindBarController(FindBarController* find_bar_controller) {
    find_bar_controller_ = find_bar_controller;
  }
  virtual void Show(bool animate);
  virtual void Hide(bool animate);
  virtual void SetFocusAndSelection();
  virtual void ClearResults(const FindNotificationDetails& results);
  virtual void StopAnimation();
  virtual void MoveWindowIfNecessary(const gfx::Rect& selection_rect,
                                     bool no_redraw);
  virtual void SetFindText(const string16& find_text);
  virtual void UpdateUIForFindResult(const FindNotificationDetails& result,
                                     const string16& find_text);
  virtual void AudibleAlert();
  virtual bool IsFindBarVisible();
  virtual void RestoreSavedFocus();
  virtual FindBarTesting* GetFindBarTesting();

  // Methods from FindBarTesting.
  virtual bool GetFindBarWindowInfo(gfx::Point* position,
                                    bool* fully_visible);
  virtual string16 GetFindText();
  virtual string16 GetFindSelectedText(){DNOTIMPLEMENTED();};
  virtual string16 GetMatchCountText(){DNOTIMPLEMENTED();};

  void FindNext() {FindEntryTextInContents(true);}
  void FindPrev() {FindEntryTextInContents(false);}
  void Close();
  void OnChanged();

//  void Init(Profile* profile);

//  // Getter for the containing widget.
//  QGraphicsItem* widget() {
//    return NULL;
//  }
//
  // Getter for associated browser object.
  Browser* browser() {
    return browser_;
  }

 private:
  // Store the currently focused widget if it is not in the find bar.
  // This should always be called before we claim focus.
  void StoreOutsideFocus();

  // For certain keystrokes, such as up or down, we want to forward the event
  // to the renderer rather than handling it ourselves. Returns true if the
  // key event was forwarded.
  // See similar function in FindBarWin.
  //    bool MaybeForwardKeyEventToRenderer(GdkEventKey* event);

  // Searches for another occurrence of the entry text, moving forward if
  // |forward_search| is true.
  void FindEntryTextInContents(bool forward_search);

  void UpdateMatchLabelAppearance(bool failure);

  // Asynchronously repositions the dialog.
  void Reposition();

  // Returns the rectangle representing where to position the find bar. If
  // |avoid_overlapping_rect| is specified, the return value will be a rectangle
  // located immediately to the left of |avoid_overlapping_rect|, as long as
  // there is enough room for the dialog to draw within the bounds. If not, the
  // dialog position returned will overlap |avoid_overlapping_rect|.
  // Note: |avoid_overlapping_rect| is expected to use coordinates relative to
  // the top of the page area, (it will be converted to coordinates relative to
  // the top of the browser window, when comparing against the dialog
  // coordinates). The returned value is relative to the browser window.
  gfx::Rect GetDialogPosition(gfx::Rect avoid_overlapping_rect);

  // Adjust the text alignment according to the text direction of the widget
  // and |text_entry_|'s content, to make sure the real text alignment is
  // always in sync with the UI language direction.
  void AdjustTextAlignment();

  // Get the position of the findbar within the floating container.
  gfx::Point GetPosition();

//  FindBarDelegate* delegate_;

  Browser* browser_;
  BrowserWindowQt* window_;

  FindBarQtImpl* impl_;

//  MOverlay* overlay_;
//  MWidget* container_;

  // Cached allocation of |container_|. We keep this on hand so that we can
  // reset the widget's shape when the width/height change.
  int container_width_;
  int container_height_;

  // The widget where text is entered.
  //  GtkWidget* text_entry_;
//  MTextEdit* text_entry_;

//  scoped_ptr<MButton> find_previous_button_;
//  scoped_ptr<MButton> find_next_button_;

//  MLabel* match_count_label_;

  // Cache whether the match count label is showing failure or not so that
  // we can update its appearance without changing its semantics.
//  bool match_label_failure_;

  // The X to close the find bar.
//  scoped_ptr<MButton> close_button_;

  // The last matchcount number we reported to the user.
//  int last_reported_matchcount_;

  // Pointer back to the owning controller.
  FindBarController* find_bar_controller_;

  /* // Saves where the focus used to be whenever we get it. */
  /* FocusStoreGtk focus_store_; */

  // If true, the change signal for the text entry is ignored.
  bool ignore_changed_signal_;

  // This is the width of widget(). We cache it so we can recognize whether
  // allocate signals have changed it, and if so take appropriate actions.
//  int current_fixed_width_;

  /* scoped_ptr<NineBox> dialog_background_; */

  // The selection rect we are currently showing. We cache it to avoid covering
  // it up.
  gfx::Rect selection_rect_;
 
  DISALLOW_COPY_AND_ASSIGN(FindBarQt);
};

#endif  // CHROME_BROWSER_MEEGOTOUCH_FIND_BAR_QT_H_
