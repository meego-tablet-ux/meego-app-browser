// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_AUTOCOMPLETE_AUTOCOMPLETE_POPUP_VIEW_MAC_H_
#define CHROME_BROWSER_AUTOCOMPLETE_AUTOCOMPLETE_POPUP_VIEW_MAC_H_

#import <Cocoa/Cocoa.h>

#include <string>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "base/scoped_nsobject.h"
#include "chrome/browser/autocomplete/autocomplete.h"
#include "chrome/browser/autocomplete/autocomplete_popup_view.h"
#include "webkit/glue/window_open_disposition.h"

class AutocompletePopupModel;
class AutocompleteEditModel;
class AutocompleteEditViewMac;
@class AutocompleteTableTarget;
class Profile;

// Implements AutocompletePopupView using a raw NSWindow containing an
// NSTableView.

class AutocompletePopupViewMac : public AutocompletePopupView {
 public:
  AutocompletePopupViewMac(AutocompleteEditViewMac* edit_view,
                           AutocompleteEditModel* edit_model,
                           Profile* profile);
  virtual ~AutocompletePopupViewMac();

  // Implement the AutocompletePopupView interface.
  virtual bool IsOpen() const;
  virtual void InvalidateLine(size_t line) {
    // TODO(shess): Verify that there is no need to implement this.
    // This is currently used in two places in the model:
    //
    // When setting the selected line, the selected line is
    // invalidated, then the selected line is changed, then the new
    // selected line is invalidated, then PaintUpdatesNow() is called.
    // For us PaintUpdatesNow() should be sufficient.
    //
    // Same thing happens when changing the hovered line, except with
    // no call to PaintUpdatesNow().  Since this code does not
    // currently support special display of the hovered line, there's
    // nothing to do here.
    //
    // deanm indicates that this is an anti-flicker optimization,
    // which we probably cannot utilize (and may not need) so long as
    // we're using NSTableView to implement the popup contents.  We
    // may need to move away from NSTableView to implement hover,
    // though.
  }
  virtual void UpdatePopupAppearance();
  virtual void OnHoverEnabledOrDisabled(bool disabled) { NOTIMPLEMENTED(); }
 
  // This is only called by model in SetSelectedLine() after updating
  // everything.  Popup should already be visible.
  virtual void PaintUpdatesNow();

  // Returns the popup's model.
  virtual AutocompletePopupModel* GetModel();

  // Helpers which forward to model_, otherwise our Objective-C helper
  // object would need model_ to be public:.
  void StopAutocomplete();
  size_t ResultRowCount();
  const std::wstring& ResultContentsAt(size_t i);
  bool ResultStarredAt(size_t i);
  const std::wstring& ResultDescriptionAt(size_t i);
  void AcceptInput(WindowOpenDisposition disposition, bool for_drop);

  // TODO(shess): Get rid of this.  Right now it's needed because of
  // the ordering of initialization in tab_contents_controller.mm.
  void SetField(NSTextField* field) {
    field_ = field;
  }

 private:
  // Create the popup_ instance if needed.
  void CreatePopupIfNeeded();

  scoped_ptr<AutocompletePopupModel> model_;
  AutocompleteEditViewMac* edit_view_;

  NSTextField* field_;  // owned by tab controller

  scoped_nsobject<AutocompleteTableTarget> table_target_;
  // TODO(shess): Before checkin review implementation to make sure
  // that popup_'s object hierarchy doesn't keep references to
  // destructed objects.
  scoped_nsobject<NSWindow> popup_;

  DISALLOW_COPY_AND_ASSIGN(AutocompletePopupViewMac);
};

#endif  // CHROME_BROWSER_AUTOCOMPLETE_AUTOCOMPLETE_POPUP_VIEW_MAC_H_
