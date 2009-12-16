// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file defines the interface class AutocompletePopupView.  Each toolkit
// will implement the popup view differently, so that code is inheriently
// platform specific.  However, the AutocompletePopupModel needs to do some
// communication with the view.  Since the model is shared between platforms,
// we need to define an interface that all view implementations will share.

#ifndef CHROME_BROWSER_AUTOCOMPLETE_AUTOCOMPLETE_POPUP_VIEW_H_
#define CHROME_BROWSER_AUTOCOMPLETE_AUTOCOMPLETE_POPUP_VIEW_H_

#include "build/build_config.h"

class AutocompleteEditView;
class AutocompletePopupModel;
class BubblePositioner;
namespace gfx {
class Font;
}
#if defined(OS_WIN) || defined(OS_LINUX)
class AutocompleteEditViewWin;
class AutocompleteEditModel;
class Profile;
#endif

class AutocompletePopupView {
 public:
  virtual ~AutocompletePopupView() {}

  // Returns true if the popup is currently open.
  virtual bool IsOpen() const = 0;

  // Invalidates one line of the autocomplete popup.
  virtual void InvalidateLine(size_t line) = 0;

  // Redraws the popup window to match any changes in the result set; this may
  // mean opening or closing the window.
  virtual void UpdatePopupAppearance() = 0;

  // Paint any pending updates.
  virtual void PaintUpdatesNow() = 0;

  // Returns the popup's model.
  virtual AutocompletePopupModel* GetModel() = 0;

#if defined(OS_WIN) || defined(OS_LINUX)
  // Create a popup view implementation. It may make sense for this to become
  // platform independent eventually.
  static AutocompletePopupView* CreatePopupView(
      const gfx::Font& font,
      AutocompleteEditView* edit_view,
      AutocompleteEditModel* edit_model,
      Profile* profile,
      const BubblePositioner* bubble_positioner);
#endif
};

#endif  // CHROME_BROWSER_AUTOCOMPLETE_AUTOCOMPLETE_POPUP_VIEW_H_
