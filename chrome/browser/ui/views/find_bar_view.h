// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_FIND_BAR_VIEW_H_
#define CHROME_BROWSER_UI_VIEWS_FIND_BAR_VIEW_H_
#pragma once

#include "base/string16.h"
#include "chrome/browser/ui/find_bar/find_notification_details.h"
#include "chrome/browser/views/dropdown_bar_view.h"
#include "gfx/size.h"
#include "views/controls/button/button.h"
#include "views/controls/textfield/textfield.h"

class FindBarHost;

namespace views {
class ImageButton;
class Label;
class MouseEvent;
class View;
}

////////////////////////////////////////////////////////////////////////////////
//
// The FindBarView is responsible for drawing the UI controls of the
// FindBar, the find text box, the 'Find' button and the 'Close'
// button. It communicates the user search words to the FindBarHost.
//
////////////////////////////////////////////////////////////////////////////////
class FindBarView : public DropdownBarView,
                    public views::ButtonListener,
                    public views::Textfield::Controller {
 public:
  // A tag denoting which button the user pressed.
  enum ButtonTag {
    FIND_PREVIOUS_TAG = 0,  // The Find Previous button.
    FIND_NEXT_TAG,          // The Find Next button.
    CLOSE_TAG,              // The Close button (the 'X').
  };

  explicit FindBarView(FindBarHost* host);
  virtual ~FindBarView();

  // Gets/sets the text displayed in the text box.
  string16 GetFindText() const;
  void SetFindText(const string16& find_text);

  // Gets the match count text displayed in the text box.
  string16 GetMatchCountText() const;

  // Updates the label inside the Find text box that shows the ordinal of the
  // active item and how many matches were found.
  void UpdateForResult(const FindNotificationDetails& result,
                       const string16& find_text);

  // Clears the current Match Count value in the Find text box.
  void ClearMatchCount();

  // Claims focus for the text field and selects its contents.
  virtual void SetFocusAndSelection(bool select_all);

  // Overridden from views::View:
  virtual void Paint(gfx::Canvas* canvas);
  virtual void Layout();
  virtual gfx::Size GetPreferredSize();
  virtual void ViewHierarchyChanged(bool is_add,
                                    views::View* parent,
                                    views::View* child);

  // Overridden from views::ButtonListener:
  virtual void ButtonPressed(views::Button* sender, const views::Event& event);

  // Overridden from views::Textfield::Controller:
  virtual void ContentsChanged(views::Textfield* sender,
                               const string16& new_contents);
  virtual bool HandleKeystroke(views::Textfield* sender,
                               const views::Textfield::Keystroke& key);

 private:
  // Update the appearance for the match count label.
  void UpdateMatchCountAppearance(bool no_match);

  // Overridden from views::View.
  virtual void OnThemeChanged();

  // We use a hidden view to grab mouse clicks and bring focus to the find
  // text box. This is because although the find text box may look like it
  // extends all the way to the find button, it only goes as far as to the
  // match_count label. The user, however, expects being able to click anywhere
  // inside what looks like the find text box (including on or around the
  // match_count label) and have focus brought to the find box.
  class FocusForwarderView : public views::View {
   public:
    explicit FocusForwarderView(
        views::Textfield* view_to_focus_on_mousedown)
      : view_to_focus_on_mousedown_(view_to_focus_on_mousedown) {}

   private:
    virtual bool OnMousePressed(const views::MouseEvent& event);

    views::Textfield* view_to_focus_on_mousedown_;

    DISALLOW_COPY_AND_ASSIGN(FocusForwarderView);
  };

  // Returns the OS-specific view for the find bar that acts as an intermediary
  // between us and the TabContentsView.
  FindBarHost* find_bar_host() const;

#if defined(OS_LINUX)
  // In gtk we get changed signals if we programatically set the text. If we
  // don't ignore them we run into problems. For example, switching tabs back
  // to one with the find bar visible will cause a search to the next found
  // text. Also if the find bar had been visible and then hidden and the user
  // switches back, found text will be highlighted again.
  bool ignore_contents_changed_;
#endif

  // The controls in the window.
  views::Textfield* find_text_;
  views::Label* match_count_text_;
  FocusForwarderView* focus_forwarder_view_;
  views::ImageButton* find_previous_button_;
  views::ImageButton* find_next_button_;
  views::ImageButton* close_button_;

  DISALLOW_COPY_AND_ASSIGN(FindBarView);
};

#endif  // CHROME_BROWSER_UI_VIEWS_FIND_BAR_VIEW_H_
