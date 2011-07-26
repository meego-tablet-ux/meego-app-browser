// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_QT_LOCATION_BAR_VIEW_QT_H_
#define CHROME_BROWSER_QT_LOCATION_BAR_VIEW_QT_H_

//#include <gtk/gtk.h>

#include <map>
#include <string>

//#include "ui/base/gtk/gtk_signal.h"
#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "chrome/browser/autocomplete/autocomplete_edit.h"
#include "chrome/browser/autocomplete/autocomplete_edit_view_qt.h"
#include "chrome/browser/extensions/extension_context_menu_model.h"
#include "chrome/browser/extensions/image_loading_tracker.h"
#include "chrome/browser/first_run/first_run.h"
//#include "chrome/browser/ui/gtk/info_bubble_gtk.h"
//#include "chrome/browser/ui/gtk/menu_gtk.h"
#include "chrome/browser/ui/omnibox/location_bar.h"
#include "chrome/common/content_settings_types.h"
#include "content/common/notification_observer.h"
#include "content/common/notification_registrar.h"
//#include "chrome/common/owned_widget_gtk.h"
#include "content/common/page_transition_types.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "webkit/glue/window_open_disposition.h"
#include <QGraphicsWidget>
//class AutocompleteEditViewGtk;
class Browser;
class CommandUpdater;
class ContentSettingImageModel;
//class ContentSettingBubbleGtk;
class ExtensionAction;
//class GtkThemeProvider;
class Profile;
class SkBitmap;
class TabContents;
class ToolbarModel;

class NotificationType;

class LocationBarViewQt : public AutocompleteEditController,
                           public LocationBar,
                           public LocationBarTesting
{
 public:
  explicit LocationBarViewQt(Browser* browser, BrowserWindowQt* window) ;
  virtual ~LocationBarViewQt() ;

  virtual void OnAutocompleteWillClosePopup(){DNOTIMPLEMENTED();};
  virtual void OnAutocompleteLosingFocus(
      gfx::NativeView view_gaining_focus){DNOTIMPLEMENTED();};
  virtual void OnAutocompleteWillAccept(){DNOTIMPLEMENTED();};
  virtual bool OnCommitSuggestedText(bool skip_inline_autocomplete){DNOTIMPLEMENTED();};
  virtual bool AcceptCurrentInstantPreview(){DNOTIMPLEMENTED();};
  virtual void OnPopupBoundsChanged(const gfx::Rect& bounds){DNOTIMPLEMENTED();};
  virtual bool OnCommitSuggestedText(const std::wstring& typed_text){DNOTIMPLEMENTED();};

  void Init(bool popup_window_mode) ;

  void SetProfile(Profile* profile) ;

  // Returns the current TabContents.
  TabContents* GetTabContents() const ;

  // Sets |preview_enabled| for the PageActionViewGtk associated with this
  // |page_action|. If |preview_enabled| is true, the view will display the
  // page action's icon even though it has not been activated by the extension.
  // This is used by the ExtensionInstalledBubbleGtk to preview what the icon
  // will look like for the user upon installation of the extension.
  void SetPreviewEnabledPageAction(ExtensionAction *page_action,
                                   bool preview_enabled) {};

  // Updates the location bar.  We also reset the bar's permanent text and
  // security style, and, if |tab_for_state_restoring| is non-NULL, also
  // restore saved state that the tab holds.
  void Update(const TabContents* tab_for_state_restoring) ;

  // Show the bookmark bubble.
  void ShowStarBubble(const GURL& url, bool newly_boomkarked) {};

  // Implement the AutocompleteEditController interface.
  virtual void OnAutocompleteAccept(const GURL& url,
      WindowOpenDisposition disposition,
      PageTransition::Type transition,
      const GURL& alternate_nav_url) OVERRIDE;
  virtual void OnChanged() OVERRIDE {};
  virtual void OnSelectionBoundsChanged() OVERRIDE {DNOTIMPLEMENTED();};
  virtual void OnKillFocus() OVERRIDE;
  virtual void OnSetFocus() OVERRIDE;
  virtual void OnInputInProgress(bool in_progress) OVERRIDE;
  virtual SkBitmap GetFavicon() const OVERRIDE;
  virtual string16 GetTitle() const OVERRIDE;
  virtual bool IsTitleSet() const OVERRIDE;
  virtual InstantController* GetInstant() OVERRIDE {return NULL;};
  virtual TabContentsWrapper* GetTabContentsWrapper() const OVERRIDE {DNOTIMPLEMENTED();};

  // Implement the LocationBar interface.
  virtual void ShowFirstRunBubble(FirstRun::BubbleType bubble_type) ;
  virtual void SetSuggestedText(const string16& text,
                                InstantCompleteBehavior behavior)
                                {DNOTIMPLEMENTED();};
  virtual std::wstring GetInputString() const ;
  virtual WindowOpenDisposition GetWindowOpenDisposition() const ;
  virtual PageTransition::Type GetPageTransition() const ;
  virtual void AcceptInput() ;
  virtual void FocusLocation(bool select_all) ;
  virtual void FocusSearch() {};
  virtual void UpdateContentSettingsIcons() ;
  virtual void UpdatePageActions() ;
  virtual void InvalidatePageActions() {};
  virtual void SaveStateToContents(TabContents* contents) {};
  virtual void Revert() {};
  virtual const AutocompleteEditView* location_entry() const {
    return location_entry_.get();
  }
  virtual AutocompleteEditView* location_entry() {
    return location_entry_.get();
  }
  virtual void PushForceHidden() {}
  virtual void PopForceHidden() {}
  virtual LocationBarTesting* GetLocationBarForTesting() { return this; }

  // Implement the LocationBarTesting interface.
  virtual int PageActionCount() { return 0; }
  virtual int PageActionVisibleCount() {return 0;};
  virtual ExtensionAction* GetPageAction(size_t index) {return NULL;};
  virtual ExtensionAction* GetVisiblePageAction(size_t index) {return NULL;};
  virtual void TestPageActionPressed(size_t index) {};
  virtual void SetSuggestedText(const string16& text){DNOTIMPLEMENTED();};
//  // Implement the NotificationObserver interface.
//  virtual void Observe(NotificationType type,
//                       const NotificationSource& source,
//                       const NotificationDetails& details) {};
//
 public:
  gfx::NativeView widget() { return location_entry_->GetNativeView();};
  void UpdateTitle();
  bool focused_;

 private:
  scoped_ptr<AutocompleteEditViewQt> location_entry_;

  Browser* browser_;
  Profile* profile_;
  CommandUpdater* command_updater_;
  BrowserWindowQt* window_;
  
  // When we get an OnAutocompleteAccept notification from the autocomplete
  // edit, we save the input string so we can give it back to the browser on
  // the LocationBar interface via GetInputString().
  std::wstring location_input_;

  // The user's desired disposition for how their input should be opened.
  WindowOpenDisposition disposition_;

  // The transition type to use for the navigation.
  PageTransition::Type transition_;


//  MWidget* star_image_;
  ToolbarModel* toolbar_model_;
  
  // When true, the location bar view is read only and also is has a slightly
  // different presentation (font size / color). This is used for popups.
  bool popup_window_mode_;

  DISALLOW_COPY_AND_ASSIGN(LocationBarViewQt);
};

#endif  // CHROME_BROWSER_QT_LOCATION_BAR_VIEW_QT_H_
