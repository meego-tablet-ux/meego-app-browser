// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GTK_FIND_BAR_GTK_H_
#define CHROME_BROWSER_GTK_FIND_BAR_GTK_H_

#include <gtk/gtk.h>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "chrome/browser/find_bar.h"
#include "chrome/common/owned_widget_gtk.h"

class BrowserWindowGtk;
class CustomDrawButton;
class FindBarController;
class SlideAnimatorGtk;
class TabContentsContainerGtk;

// Currently this class contains both a model and a view.  We may want to
// eventually pull out the model specific bits and share with Windows.
class FindBarGtk : public FindBar,
                   public FindBarTesting {
 public:
  explicit FindBarGtk(BrowserWindowGtk* browser);
  virtual ~FindBarGtk();

  GtkWidget* widget() const { return fixed_.get(); }

  // Methods from FindBar.
  virtual FindBarController* GetFindBarController() const {
    return find_bar_controller_;
  }
  virtual void SetFindBarController(FindBarController* find_bar_controller) {
    find_bar_controller_ = find_bar_controller;
  }
  virtual void Show();
  virtual void Hide(bool animate);
  virtual void SetFocusAndSelection();
  virtual void ClearResults(const FindNotificationDetails& results);
  virtual void StopAnimation();
  virtual void MoveWindowIfNecessary(const gfx::Rect& selection_rect,
                                     bool no_redraw);
  virtual void SetFindText(const string16& find_text);
  virtual void UpdateUIForFindResult(const FindNotificationDetails& result,
                                     const string16& find_text);
  virtual void AudibleAlertIfNotFound(const FindNotificationDetails& result);
  virtual gfx::Rect GetDialogPosition(gfx::Rect avoid_overlapping_rect);
  virtual void SetDialogPosition(const gfx::Rect& new_pos, bool no_redraw);
  virtual bool IsFindBarVisible();
  virtual void RestoreSavedFocus();
  virtual FindBarTesting* GetFindBarTesting();

  // Methods from FindBarTesting.
  virtual bool GetFindBarWindowInfo(gfx::Point* position,
                                    bool* fully_visible);

 private:
  void InitWidgets();

  // Returns the child of |fixed_| that holds what the user perceives as the
  // findbar.
  GtkWidget* slide_widget();

  // Searches for another occurrence of the entry text, moving forward if
  // |forward_search| is true.
  void FindEntryTextInContents(bool forward_search);

  // Callback when the entry text changes.
  static gboolean OnChanged(GtkWindow* window, FindBarGtk* find_bar);

  // Callback for key presses.
  static gboolean OnKeyPressEvent(GtkWindow* window, GdkEventKey* event,
                                  FindBarGtk* find_bar);

  // Callback for previous, next, and close button.
  static void OnButtonPressed(GtkWidget* button, FindBarGtk* find_bar);

  // Called when |fixed_| changes sizes. Used to position the dialog (the
  // "dialog" is the widget hierarchy rooted at |slide_widget_|).
  static void OnFixedSizeAllocate(GtkWidget* fixed,
                                  GtkAllocation* allocation,
                                  FindBarGtk* findbar);

  // Called when |container_| is allocated.
  static void OnContainerSizeAllocate(GtkWidget* container,
                                      GtkAllocation* allocation,
                                      FindBarGtk* findbar);

  // GtkFixed containing the find bar widgets.
  OwnedWidgetGtk fixed_;

  // An event box which shows the background for |fixed_|. We could just set
  // |fixed_| to have its own GdkWindow and draw the background directly, but
  // then |container_| would clip to the bounds of |fixed_|.
  GtkWidget* border_;

  // The widget that animates the slide-in and -out of the findbar.
  scoped_ptr<SlideAnimatorGtk> slide_widget_;

  // A GtkAlignment that is the child of |slide_widget_|.
  GtkWidget* container_;

  // This will be set to true after ContourWidget() has been called so we don't
  // call it twice.
  bool container_shaped_;

  // The widget where text is entered.
  GtkWidget* find_text_;

  // The next and previous match buttons.
  scoped_ptr<CustomDrawButton> find_previous_button_;
  scoped_ptr<CustomDrawButton> find_next_button_;

  // The X to close the find bar.
  scoped_ptr<CustomDrawButton> close_button_;

  // Pointer back to the owning controller.
  FindBarController* find_bar_controller_;

  DISALLOW_COPY_AND_ASSIGN(FindBarGtk);
};

#endif  // CHROME_BROWSER_GTK_FIND_BAR_GTK_H_
