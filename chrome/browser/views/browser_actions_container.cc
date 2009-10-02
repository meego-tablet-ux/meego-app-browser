// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/views/browser_actions_container.h"

#include "base/stl_util-inl.h"
#include "chrome/browser/extensions/extension_browser_event_router.h"
#include "chrome/browser/extensions/extensions_service.h"
#include "chrome/browser/extensions/extension_tabs_module.h"
#include "chrome/browser/image_loading_tracker.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/views/extensions/extension_popup.h"
#include "chrome/browser/views/toolbar_view.h"
#include "chrome/common/extensions/extension_action.h"
#include "chrome/common/notification_source.h"
#include "chrome/common/notification_type.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "views/controls/button/menu_button.h"

// The size of the icon for page actions.
static const int kIconSize = 30;

// The padding between the browser actions and the omnibox/page menu.
static const int kHorizontalPadding = 4;

////////////////////////////////////////////////////////////////////////////////
// BrowserActionImageView

// The BrowserActionImageView is a specialization of the TextButton class.
// It acts on a ExtensionAction, in this case a BrowserAction and handles
// loading the image for the button asynchronously on the file thread to
class BrowserActionImageView : public views::MenuButton,
                               public views::ButtonListener,
                               public ImageLoadingTracker::Observer,
                               public NotificationObserver {
 public:
  BrowserActionImageView(ExtensionAction* browser_action,
                         Extension* extension,
                         BrowserActionsContainer* panel);
  ~BrowserActionImageView();

  // Overridden from views::ButtonListener:
  virtual void ButtonPressed(views::Button* sender, const views::Event& event);

  // Overridden from ImageLoadingTracker.
  virtual void OnImageLoaded(SkBitmap* image, size_t index);

  // Overridden from NotificationObserver:
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

  // MenuButton behavior overrides.  These methods all default to TextButton
  // behavior unless this button is a popup.  In that case, it uses MenuButton
  // behavior.  MenuButton has the notion of a child popup being shown where the
  // button will stay in the pushed state until the "menu" (a popup in this
  // case) is dismissed.
  virtual bool Activate();
  virtual bool OnMousePressed(const views::MouseEvent& e);
  virtual void OnMouseReleased(const views::MouseEvent& e, bool canceled);
  virtual bool OnKeyReleased(const views::KeyEvent& e);
  virtual void OnMouseExited(const views::MouseEvent& event);

  // Does this button's action have a popup?
  virtual bool IsPopup();

  // Notifications when the popup is hidden or shown by the container.
  virtual void PopupDidShow();
  virtual void PopupDidHide();

  const ExtensionAction& browser_action() const { return *browser_action_; }

 private:
  // Called to update the display to match the browser action's state.
  void OnStateUpdated();

  // The browser action this view represents. The ExtensionAction is not owned
  // by this class.
  ExtensionAction* browser_action_;

  // The state of our browser action. Not owned by this class.
  ExtensionActionState* browser_action_state_;

  // The icons representing different states for the browser action.
  std::vector<SkBitmap> browser_action_icons_;

  // The object that is waiting for the image loading to complete
  // asynchronously. This object can potentially outlive the BrowserActionView,
  // and takes care of deleting itself.
  ImageLoadingTracker* tracker_;

  // The browser action shelf.
  BrowserActionsContainer* panel_;

  NotificationRegistrar registrar_;

  DISALLOW_COPY_AND_ASSIGN(BrowserActionImageView);
};

BrowserActionImageView::BrowserActionImageView(
    ExtensionAction* browser_action, Extension* extension,
    BrowserActionsContainer* panel)
    : MenuButton(this, L"", NULL, false),
      browser_action_(browser_action),
      browser_action_state_(extension->browser_action_state()),
      tracker_(NULL),
      panel_(panel) {
  set_alignment(TextButton::ALIGN_CENTER);

  // Load the images this view needs asynchronously on the file thread. We'll
  // get a call back into OnImageLoaded if the image loads successfully. If not,
  // the ImageView will have no image and will not appear in the browser chrome.
  DCHECK(!browser_action->icon_paths().empty());
  const std::vector<std::string>& icon_paths = browser_action->icon_paths();
  browser_action_icons_.resize(icon_paths.size());
  tracker_ = new ImageLoadingTracker(this, icon_paths.size());
  for (std::vector<std::string>::const_iterator iter = icon_paths.begin();
       iter != icon_paths.end(); ++iter) {
    FilePath path = extension->GetResourcePath(*iter);
    tracker_->PostLoadImageTask(path);
  }

  registrar_.Add(this, NotificationType::EXTENSION_BROWSER_ACTION_UPDATED,
                 Source<ExtensionAction>(browser_action_));
}

BrowserActionImageView::~BrowserActionImageView() {
  if (tracker_) {
    tracker_->StopTrackingImageLoad();
    tracker_ = NULL;  // The tracker object will be deleted when we return.
  }
}

void BrowserActionImageView::ButtonPressed(
    views::Button* sender, const views::Event& event) {
  panel_->OnBrowserActionExecuted(this);
}

void BrowserActionImageView::OnImageLoaded(SkBitmap* image, size_t index) {
  DCHECK(index < browser_action_icons_.size());
  browser_action_icons_[index] = *image;
  if (index == browser_action_icons_.size() - 1) {
    OnStateUpdated();
    tracker_ = NULL;  // The tracker object will delete itself when we return.
  }
}

void BrowserActionImageView::OnStateUpdated() {
  SkBitmap* image = &browser_action_icons_[browser_action_state_->icon_index()];
  SetIcon(*image);
  SetTooltipText(ASCIIToWide(browser_action_state_->title()));
  panel_->OnBrowserActionVisibilityChanged();
}

void BrowserActionImageView::Observe(NotificationType type,
                                     const NotificationSource& source,
                                     const NotificationDetails& details) {
  if (type == NotificationType::EXTENSION_BROWSER_ACTION_UPDATED) {
    OnStateUpdated();
  } else {
    NOTREACHED() << L"Received unexpected notification";
  }
}

bool BrowserActionImageView::IsPopup() {
  return (browser_action_ && !browser_action_->popup_url().is_empty());
}

bool BrowserActionImageView::Activate() {
  if (IsPopup()) {
    panel_->OnBrowserActionExecuted(this);

    // TODO(erikkay): Run a nested modal loop while the mouse is down to
    // enable menu-like drag-select behavior.

    // The return value of this method is returned via OnMousePressed.
    // We need to return false here since we're handing off focus to another
    // widget/view, and true will grab it right back and try to send events
    // to us.
    return false;
  }
  return true;
}

bool BrowserActionImageView::OnMousePressed(const views::MouseEvent& e) {
  if (IsPopup())
    return MenuButton::OnMousePressed(e);
  return TextButton::OnMousePressed(e);
}

void BrowserActionImageView::OnMouseReleased(const views::MouseEvent& e,
                                             bool canceled) {
  if (IsPopup()) {
    // TODO(erikkay) this never actually gets called (probably because of the
    // loss of focus).
    MenuButton::OnMouseReleased(e, canceled);
  } else {
    TextButton::OnMouseReleased(e, canceled);
  }
}

bool BrowserActionImageView::OnKeyReleased(const views::KeyEvent& e) {
  if (IsPopup())
    return MenuButton::OnKeyReleased(e);
  return TextButton::OnKeyReleased(e);
}

void BrowserActionImageView::OnMouseExited(const views::MouseEvent& e) {
  if (IsPopup())
    MenuButton::OnMouseExited(e);
  else
    TextButton::OnMouseExited(e);
}

void BrowserActionImageView::PopupDidShow() {
  SetState(views::CustomButton::BS_PUSHED);
  menu_visible_ = true;
}

void BrowserActionImageView::PopupDidHide() {
  SetState(views::CustomButton::BS_NORMAL);
  menu_visible_ = false;
}

////////////////////////////////////////////////////////////////////////////////
// BrowserActionsContainer

BrowserActionsContainer::BrowserActionsContainer(
    Profile* profile, ToolbarView* toolbar)
    : profile_(profile),
      toolbar_(toolbar),
      popup_(NULL),
      popup_button_(NULL),
      ALLOW_THIS_IN_INITIALIZER_LIST(task_factory_(this)) {
  ExtensionsService* extension_service = profile->GetExtensionsService();
  registrar_.Add(this, NotificationType::EXTENSION_LOADED,
                 Source<ExtensionsService>(extension_service));
  registrar_.Add(this, NotificationType::EXTENSION_UNLOADED,
                 Source<ExtensionsService>(extension_service));
  registrar_.Add(this, NotificationType::EXTENSION_UNLOADED_DISABLED,
                 Source<ExtensionsService>(extension_service));

  RefreshBrowserActionViews();
}

BrowserActionsContainer::~BrowserActionsContainer() {
  HidePopup();
  DeleteBrowserActionViews();
}

void BrowserActionsContainer::RefreshBrowserActionViews() {
  ExtensionsService* extension_service = profile_->GetExtensionsService();
  if (!extension_service)  // The |extension_service| can be NULL in Incognito.
    return;

  std::vector<ExtensionAction*> browser_actions;
  browser_actions = extension_service->GetBrowserActions();

  DeleteBrowserActionViews();
  for (size_t i = 0; i < browser_actions.size(); ++i) {
    Extension* extension = extension_service->GetExtensionById(
        browser_actions[i]->extension_id());
    DCHECK(extension);

    // Only show browser actions that have an icon.
    if (browser_actions[i]->icon_paths().size() > 0) {
      BrowserActionImageView* view =
          new BrowserActionImageView(browser_actions[i], extension, this);
      browser_action_views_.push_back(view);
      AddChildView(view);
    }
  }
}

void BrowserActionsContainer::DeleteBrowserActionViews() {
  if (!browser_action_views_.empty()) {
    for (size_t i = 0; i < browser_action_views_.size(); ++i)
      RemoveChildView(browser_action_views_[i]);
    STLDeleteContainerPointers(browser_action_views_.begin(),
                               browser_action_views_.end());
    browser_action_views_.clear();
  }
}

void BrowserActionsContainer::OnBrowserActionVisibilityChanged() {
  toolbar_->Layout();
}

void BrowserActionsContainer::HidePopup() {
  if (popup_) {
    popup_->Hide();
    popup_->DetachFromBrowser();
    delete popup_;
    popup_ = NULL;
    popup_button_->PopupDidHide();
    popup_button_ = NULL;
    return;
  }
}

void BrowserActionsContainer::OnBrowserActionExecuted(
    BrowserActionImageView* button) {
  const ExtensionAction& browser_action = button->browser_action();

  // Popups just display.  No notification to the extension.
  // TODO(erikkay): should there be?
  if (button->IsPopup()) {
    // If we're showing the same popup, just hide it and return.
    bool same_showing = popup_ && button == popup_button_;

    // Always hide the current popup, even if it's not the same.
    // Only one popup should be visible at a time.
    HidePopup();

    if (same_showing)
      return;

    gfx::Point origin;
    View::ConvertPointToWidget(button, &origin);
    gfx::Rect rect = bounds();
    rect.set_x(origin.x());
    rect.set_y(origin.y());
    popup_ = ExtensionPopup::Show(browser_action.popup_url(),
                                  toolbar_->browser(),
                                  rect,
                                  browser_action.popup_height());
    popup_->set_delegate(this);
    popup_button_ = button;
    popup_button_->PopupDidShow();
    return;
  }

  // Otherwise, we send the action to the extension.
  int window_id = ExtensionTabUtil::GetWindowId(toolbar_->browser());
  ExtensionBrowserEventRouter::GetInstance()->BrowserActionExecuted(
      profile_, browser_action.extension_id(), window_id);
}

gfx::Size BrowserActionsContainer::GetPreferredSize() {
  if (browser_action_views_.empty())
    return gfx::Size(0, 0);
  int width = kHorizontalPadding * 2 +
      browser_action_views_.size() * kIconSize;
  return gfx::Size(width, kIconSize);
}

void BrowserActionsContainer::Layout() {
  for (size_t i = 0; i < browser_action_views_.size(); ++i) {
    views::TextButton* view = browser_action_views_[i];
    int x = kHorizontalPadding + i * kIconSize;
    view->SetBounds(x, (height() - kIconSize) / 2, kIconSize, kIconSize);
  }
}

void BrowserActionsContainer::Observe(NotificationType type,
                                      const NotificationSource& source,
                                      const NotificationDetails& details) {
  if (type == NotificationType::EXTENSION_LOADED ||
      type == NotificationType::EXTENSION_UNLOADED ||
      type == NotificationType::EXTENSION_UNLOADED_DISABLED) {
    RefreshBrowserActionViews();
  } else {
    NOTREACHED() << L"Received unexpected notification";
  }
}

void BrowserActionsContainer::BubbleBrowserWindowMoved(BrowserBubble* bubble) {
}

void BrowserActionsContainer::BubbleBrowserWindowClosing(
    BrowserBubble* bubble) {
  HidePopup();
}

void BrowserActionsContainer::BubbleGotFocus(BrowserBubble* bubble) {
}

void BrowserActionsContainer::BubbleLostFocus(BrowserBubble* bubble) {
  // This is a bit annoying.  If you click on the button that generated the
  // current popup, then we first get this lost focus message, and then
  // we get the click action.  This results in the popup being immediately
  // shown again.  To workaround this, we put in a delay.
  MessageLoop::current()->PostTask(FROM_HERE,
      task_factory_.NewRunnableMethod(&BrowserActionsContainer::HidePopup));
}
