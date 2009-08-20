// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_VIEWS_UNINSTALL_VIEW_H_
#define CHROME_BROWSER_VIEWS_UNINSTALL_VIEW_H_

#include "views/controls/combobox/combobox.h"
#include "views/window/dialog_delegate.h"

namespace views {
class Checkbox;
class Label;
}

// UninstallView implements the dialog that confirms Chrome uninstallation
// and asks whether to delete Chrome profile. Also if currently Chrome is set
// as default browser, it asks users whether to set another browser as default.
class UninstallView : public views::View,
                      public views::ButtonListener,
                      public views::DialogDelegate,
                      public views::Combobox::Model {
 public:
  explicit UninstallView(int& user_selection);
  virtual ~UninstallView();

  // Overridden from views::DialogDelegate:
  virtual bool Accept();
  virtual bool Cancel();
  virtual std::wstring GetDialogButtonLabel(
      MessageBoxFlags::DialogButton button) const;

  // Overridden form views::ButtonListener.
  virtual void ButtonPressed(views::Button* sender);

  // Overridden from views::WindowDelegate:
  virtual std::wstring GetWindowTitle() const;
  virtual views::View* GetContentsView();

  // Overridden from views::Combobox::Model.
  virtual int GetItemCount(views::Combobox* source);
  virtual std::wstring GetItemAt(views::Combobox* source, int index);

 private:
  // Initializes the controls on the dialog.
  void SetupControls();

  views::Label* confirm_label_;
  views::Checkbox* delete_profile_;
  views::Checkbox* change_default_browser_;
  views::Combobox* browsers_combo_;
  typedef std::map<std::wstring, std::wstring> BrowsersMap;
  scoped_ptr<BrowsersMap> browsers_;
  int& user_selection_;

  DISALLOW_COPY_AND_ASSIGN(UninstallView);
};

#endif  // CHROME_BROWSER_VIEWS_UNINSTALL_VIEW_H_