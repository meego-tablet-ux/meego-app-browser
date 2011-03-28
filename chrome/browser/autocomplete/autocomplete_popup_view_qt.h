// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_AUTOCOMPLETE_AUTOCOMPLETE_POPUP_VIEW_QT_H_
#define CHROME_BROWSER_AUTOCOMPLETE_AUTOCOMPLETE_POPUP_VIEW_QT_H_

#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "chrome/browser/autocomplete/autocomplete_popup_view.h"
#include "content/common/notification_observer.h"
#include "content/common/notification_registrar.h"
#include "webkit/glue/window_open_disposition.h"

class AutocompleteEditModel;
class AutocompleteEditView;
class AutocompletePopupModel;
class Profile;
class SkBitmap;

class BrowserWindowQt;
class AutocompletePopupViewQtImpl;

class AutocompletePopupViewQt : public AutocompletePopupView,
                                public NotificationObserver {
 public:
  AutocompletePopupViewQt(const gfx::Font& font,
                          AutocompleteEditView* edit_view,
                          AutocompleteEditModel* edit_model,
                          Profile* profile,
                          BrowserWindowQt* window);
  ~AutocompletePopupViewQt();

  void Init();
  
  // Overridden from AutocompletePopupView:
  virtual bool IsOpen() const { return opened_; }
  virtual void InvalidateLine(size_t line);
  virtual void UpdatePopupAppearance();
  virtual gfx::Rect GetTargetBounds();
  virtual void PaintUpdatesNow();
  virtual void OnDragCanceled();
  virtual AutocompletePopupModel* GetModel();
  virtual int GetMaxYCoordinate();
  AutocompleteEditView* GetEditView() {return edit_view_;};

  // Overridden from NotificationObserver:
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

  // Accept a line of the results, for example, when the user clicks a line.
  void AcceptLine(size_t line, WindowOpenDisposition disposition);

 private:
  void Show(size_t num_results);
  void Hide();

  scoped_ptr<AutocompletePopupModel> model_;
  AutocompleteEditView* edit_view_;
  AutocompletePopupViewQtImpl* impl_;
  BrowserWindowQt* window_;
  
  NotificationRegistrar registrar_;

  // Whether our popup is currently open / shown, or closed / hidden.
  bool opened_;

  DISALLOW_COPY_AND_ASSIGN(AutocompletePopupViewQt);
};

#endif  // CHROME_BROWSER_AUTOCOMPLETE_AUTOCOMPLETE_POPUP_VIEW_QT_H_
