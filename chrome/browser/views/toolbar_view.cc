// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/views/toolbar_view.h"

#include <string>

#include "app/drag_drop_types.h"
#include "app/gfx/canvas.h"
#include "app/l10n_util.h"
#include "app/os_exchange_data.h"
#include "app/resource_bundle.h"
#include "base/command_line.h"
#include "base/keyboard_codes.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "chrome/app/chrome_dll_resource.h"
#include "chrome/browser/back_forward_menu_model_views.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browser_window.h"
#include "chrome/browser/character_encoding.h"
#include "chrome/browser/encoding_menu_controller.h"
#include "chrome/browser/metrics/user_metrics.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/tab_contents/navigation_controller.h"
#include "chrome/browser/tab_contents/navigation_entry.h"
#include "chrome/browser/browser_theme_provider.h"
#include "chrome/browser/user_data_manager.h"
#include "chrome/browser/views/bookmark_menu_button.h"
#include "chrome/browser/views/browser_actions_container.h"
#include "chrome/browser/views/event_utils.h"
#include "chrome/browser/views/go_button.h"
#include "chrome/browser/views/location_bar_view.h"
#include "chrome/browser/views/toolbar_star_toggle.h"
#include "chrome/browser/view_ids.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/notification_service.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/pref_service.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"
#include "net/base/net_util.h"
#include "views/background.h"
#include "views/controls/button/button_dropdown.h"
#include "views/controls/label.h"
#include "views/drag_utils.h"
#include "views/widget/tooltip_manager.h"
#include "views/window/non_client_view.h"
#include "views/window/window.h"

static const int kControlHorizOffset = 4;
static const int kControlVertOffset = 6;
static const int kControlIndent = 3;
static const int kStatusBubbleWidth = 480;

// Separation between the location bar and the menus.
static const int kMenuButtonOffset = 3;

// Padding to the right of the location bar
static const int kPaddingRight = 2;

static const int kPopupTopSpacingNonGlass = 3;
static const int kPopupBottomSpacingNonGlass = 2;
static const int kPopupBottomSpacingGlass = 1;

static SkBitmap* kPopupBackgroundEdge = NULL;

////////////////////////////////////////////////////////////////////////////////
// EncodingMenuModel

EncodingMenuModel::EncodingMenuModel(Browser* browser)
    : SimpleMenuModel(this),
      browser_(browser) {
  Build();
}

void EncodingMenuModel::Build() {
  EncodingMenuController::EncodingMenuItemList encoding_menu_items;
  EncodingMenuController encoding_menu_controller;
  encoding_menu_controller.GetEncodingMenuItems(browser_->profile(),
                                                &encoding_menu_items);

  int group_id = 0;
  EncodingMenuController::EncodingMenuItemList::iterator it =
      encoding_menu_items.begin();
  for (; it != encoding_menu_items.end(); ++it) {
    int id = it->first;
    string16& label = it->second;
    if (id == 0) {
      AddSeparator();
    } else {
      if (id == IDC_ENCODING_AUTO_DETECT) {
        AddCheckItem(id, label);
      } else {
        // Use the id of the first radio command as the id of the group.
        if (group_id <= 0)
          group_id = id;
        AddRadioItem(id, label, group_id);
      }
    }
  }
}

bool EncodingMenuModel::IsCommandIdChecked(int command_id) const {
  TabContents* current_tab = browser_->GetSelectedTabContents();
  EncodingMenuController controller;
  return controller.IsItemChecked(browser_->profile(),
                                  current_tab->encoding(), command_id);
}

bool EncodingMenuModel::IsCommandIdEnabled(int command_id) const {
  return browser_->command_updater()->IsCommandEnabled(command_id);
}

bool EncodingMenuModel::GetAcceleratorForCommandId(
    int command_id,
    views::Accelerator* accelerator) {
  return false;
}

void EncodingMenuModel::ExecuteCommand(int command_id) {
  browser_->ExecuteCommand(command_id);
}

////////////////////////////////////////////////////////////////////////////////
// EncodingMenuModel

ZoomMenuModel::ZoomMenuModel(views::SimpleMenuModel::Delegate* delegate)
    : SimpleMenuModel(delegate) {
  Build();
}

void ZoomMenuModel::Build() {
  AddItemWithStringId(IDC_ZOOM_PLUS, IDS_ZOOM_PLUS);
  AddItemWithStringId(IDC_ZOOM_NORMAL, IDS_ZOOM_NORMAL);
  AddItemWithStringId(IDC_ZOOM_MINUS, IDS_ZOOM_MINUS);
}

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, public:

ToolbarView::ToolbarView(Browser* browser)
    : model_(browser->toolbar_model()),
      acc_focused_view_(NULL),
      back_(NULL),
      forward_(NULL),
      reload_(NULL),
      home_(NULL),
      star_(NULL),
      location_bar_(NULL),
      go_(NULL),
      browser_actions_(NULL),
      page_menu_(NULL),
      app_menu_(NULL),
      bookmark_menu_(NULL),
      profile_(NULL),
      browser_(browser),
      profiles_menu_contents_(NULL),
      ALLOW_THIS_IN_INITIALIZER_LIST(
          profiles_helper_(new GetProfilesHelper(this))) {
  browser_->command_updater()->AddCommandObserver(IDC_BACK, this);
  browser_->command_updater()->AddCommandObserver(IDC_FORWARD, this);
  browser_->command_updater()->AddCommandObserver(IDC_RELOAD, this);
  browser_->command_updater()->AddCommandObserver(IDC_HOME, this);
  browser_->command_updater()->AddCommandObserver(IDC_STAR, this);
  if (browser->type() == Browser::TYPE_NORMAL)
    display_mode_ = DISPLAYMODE_NORMAL;
  else
    display_mode_ = DISPLAYMODE_LOCATION;

  if (!kPopupBackgroundEdge) {
    kPopupBackgroundEdge = ResourceBundle::GetSharedInstance().GetBitmapNamed(
        IDR_LOCATIONBG_POPUPMODE_EDGE);
  }
}

ToolbarView::~ToolbarView() {
  profiles_helper_->OnDelegateDeleted();
}

void ToolbarView::Init(Profile* profile) {
  back_menu_model_.reset(new BackForwardMenuModelViews(
      browser_, BackForwardMenuModel::BACKWARD_MENU, GetWidget()));
  forward_menu_model_.reset(new BackForwardMenuModelViews(
      browser_, BackForwardMenuModel::FORWARD_MENU, GetWidget()));

  // Create all the individual Views in the Toolbar.
  CreateLeftSideControls();
  CreateCenterStack(profile);
  CreateRightSideControls(profile);

  show_home_button_.Init(prefs::kShowHomeButton, profile->GetPrefs(), this);

  SetProfile(profile);
}

void ToolbarView::SetProfile(Profile* profile) {
  if (profile == profile_)
    return;

  profile_ = profile;
  location_bar_->SetProfile(profile);
}

void ToolbarView::Update(TabContents* tab, bool should_restore_state) {
  if (location_bar_)
    location_bar_->Update(should_restore_state ? tab : NULL);
}

int ToolbarView::GetNextAccessibleViewIndex(int view_index, bool nav_left) {
  int modifier = 1;

  if (nav_left)
    modifier = -1;

  int current_view_index = view_index + modifier;

  while ((current_view_index >= 0) &&
         (current_view_index < GetChildViewCount())) {
    // Skip the location bar, as it has its own keyboard navigation. Also skip
    // any views that cannot be interacted with.
    if (current_view_index == GetChildIndex(location_bar_) ||
        !GetChildViewAt(current_view_index)->IsEnabled() ||
        !GetChildViewAt(current_view_index)->IsVisible()) {
      current_view_index += modifier;
      continue;
    }
    // Update view_index with the available button index found.
    view_index = current_view_index;
    break;
  }
  // Returns the next available button index, or if no button is available in
  // the specified direction, remains where it was.
  return view_index;
}

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, Menu::BaseControllerDelegate overrides:

bool ToolbarView::GetAcceleratorInfo(int id, views::Accelerator* accel) {
  return GetWidget()->GetAccelerator(id, accel);
}

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, views::MenuDelegate implementation:

void ToolbarView::RunMenu(views::View* source, const gfx::Point& pt,
                          gfx::NativeView parent) {
  switch (source->GetID()) {
    case VIEW_ID_PAGE_MENU:
      RunPageMenu(pt, parent);
      break;
    case VIEW_ID_APP_MENU:
      RunAppMenu(pt, parent);
      break;
    default:
      NOTREACHED() << "Invalid source menu.";
  }
}

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, GetProfilesHelper::Delegate implementation:

void ToolbarView::OnGetProfilesDone(
    const std::vector<std::wstring>& profiles) {
  // Nothing to do if the menu has gone away.
  if (!profiles_menu_contents_.get())
    return;

  // Store the latest list of profiles in the browser.
  browser_->set_user_data_dir_profiles(profiles);

  // Add direct sub menu items for profiles.
  std::vector<std::wstring>::const_iterator iter = profiles.begin();
  for (int i = IDC_NEW_WINDOW_PROFILE_0;
       (i <= IDC_NEW_WINDOW_PROFILE_LAST) && (iter != profiles.end());
       ++i, ++iter)
    profiles_menu_contents_->AddItem(i, WideToUTF16Hack(*iter));

  // If there are more profiles then show "Other" link.
  if (iter != profiles.end()) {
    profiles_menu_contents_->AddSeparator();
    profiles_menu_contents_->AddItemWithStringId(IDC_SELECT_PROFILE,
                                                 IDS_SELECT_PROFILE);
  }

  // Always show a link to select a new profile.
  profiles_menu_contents_->AddSeparator();
  profiles_menu_contents_->AddItemWithStringId(
      IDC_NEW_PROFILE,
      IDS_SELECT_PROFILE_DIALOG_NEW_PROFILE_ENTRY);
}

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, LocationBarView::Delegate implementation:

TabContents* ToolbarView::GetTabContents() {
  return browser_->GetSelectedTabContents();
}

void ToolbarView::OnInputInProgress(bool in_progress) {
  // The edit should make sure we're only notified when something changes.
  DCHECK(model_->input_in_progress() != in_progress);

  model_->set_input_in_progress(in_progress);
  location_bar_->Update(NULL);
}

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, CommandUpdater::CommandObserver implementation:

void ToolbarView::EnabledStateChangedForCommand(int id, bool enabled) {
  views::Button* button = NULL;
  switch (id) {
    case IDC_BACK:
      button = back_;
      break;
    case IDC_FORWARD:
      button = forward_;
      break;
    case IDC_RELOAD:
      button = reload_;
      break;
    case IDC_HOME:
      button = home_;
      break;
    case IDC_STAR:
      button = star_;
      break;
  }
  if (button)
    button->SetEnabled(enabled);
}

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, views::Button::ButtonListener implementation:

void ToolbarView::ButtonPressed(
    views::Button* sender, const views::Event& event) {
  int id = sender->tag();
  switch (id) {
    case IDC_BACK:
    case IDC_FORWARD:
    case IDC_RELOAD:
      // Forcibly reset the location bar, since otherwise it won't discard any
      // ongoing user edits, since it doesn't realize this is a user-initiated
      // action.
      location_bar_->Revert();
      break;
  }
  browser_->ExecuteCommandWithDisposition(
      id, event_utils::DispositionFromEventFlags(sender->mouse_event_flags()));
}

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, BubblePositioner implementation:

gfx::Rect ToolbarView::GetLocationStackBounds() const {
  // The number of pixels from the left or right edges of the location stack to
  // "just inside the visible borders".  When the omnibox bubble contents are
  // aligned with this, the visible borders tacked on to the outsides will line
  // up with the visible borders on the location stack.
  const int kLocationStackEdgeWidth = 2;

  gfx::Point origin;
  views::View::ConvertPointToScreen(star_, &origin);
  gfx::Rect stack_bounds(origin.x(), origin.y(),
                         star_->width() + location_bar_->width() + go_->width(),
                         location_bar_->height());
  if (UILayoutIsRightToLeft()) {
    stack_bounds.set_x(
        stack_bounds.x() - location_bar_->width() - go_->width());
  }
  // Inset the bounds to just inside the visible edges (see comment above).
  stack_bounds.Inset(kLocationStackEdgeWidth, 0);
  return stack_bounds;
}

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, NotificationObserver implementation:

void ToolbarView::Observe(NotificationType type,
                          const NotificationSource& source,
                          const NotificationDetails& details) {
  if (type == NotificationType::PREF_CHANGED) {
    std::wstring* pref_name = Details<std::wstring>(details).ptr();
    if (*pref_name == prefs::kShowHomeButton) {
      Layout();
      SchedulePaint();
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, views::SimpleMenuModel::Delegate implementation:

bool ToolbarView::IsCommandIdChecked(int command_id) const {
  if (command_id == IDC_SHOW_BOOKMARK_BAR)
    return profile_->GetPrefs()->GetBoolean(prefs::kShowBookmarkBar);
  return false;
}

bool ToolbarView::IsCommandIdEnabled(int command_id) const {
  return browser_->command_updater()->IsCommandEnabled(command_id);
}

bool ToolbarView::GetAcceleratorForCommandId(int command_id,
                                             views::Accelerator* accelerator) {
  // The standard Ctrl-X, Ctrl-V and Ctrl-C are not defined as accelerators
  // anywhere so we need to check for them explicitly here.
  // TODO(cpu) Bug 1109102. Query WebKit land for the actual bindings.
  switch (command_id) {
    case IDC_CUT:
      *accelerator = views::Accelerator(base::VKEY_X, false, true, false);
      return true;
    case IDC_COPY:
      *accelerator = views::Accelerator(base::VKEY_C, false, true, false);
      return true;
    case IDC_PASTE:
      *accelerator = views::Accelerator(base::VKEY_V, false, true, false);
      return true;
  }
  // Else, we retrieve the accelerator information from the frame.
  return GetWidget()->GetAccelerator(command_id, accelerator);
}

void ToolbarView::ExecuteCommand(int command_id) {
  browser_->ExecuteCommand(command_id);
}

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, views::View overrides:

gfx::Size ToolbarView::GetPreferredSize() {
  if (IsDisplayModeNormal()) {
    int min_width = kControlIndent + back_->GetPreferredSize().width() +
        forward_->GetPreferredSize().width() + kControlHorizOffset +
        reload_->GetPreferredSize().width() + (show_home_button_.GetValue() ?
        (home_->GetPreferredSize().width() + kControlHorizOffset) : 0) +
        star_->GetPreferredSize().width() + go_->GetPreferredSize().width() +
        kMenuButtonOffset +
        (bookmark_menu_ ? bookmark_menu_->GetPreferredSize().width() : 0) +
        page_menu_->GetPreferredSize().width() +
        app_menu_->GetPreferredSize().width() + kPaddingRight;

    static SkBitmap normal_background;
    if (normal_background.isNull()) {
      ResourceBundle& rb = ResourceBundle::GetSharedInstance();
      normal_background = *rb.GetBitmapNamed(IDR_CONTENT_TOP_CENTER);
    }

    return gfx::Size(min_width, normal_background.height());
  }

  int vertical_spacing = PopupTopSpacing() +
      (GetWindow()->GetNonClientView()->UseNativeFrame() ?
          kPopupBottomSpacingGlass : kPopupBottomSpacingNonGlass);
  return gfx::Size(0, location_bar_->GetPreferredSize().height() +
      vertical_spacing);
}

void ToolbarView::Layout() {
  // If we have not been initialized yet just do nothing.
  if (back_ == NULL)
    return;

  if (!IsDisplayModeNormal()) {
    int edge_width = (browser_->window() && browser_->window()->IsMaximized()) ?
        0 : kPopupBackgroundEdge->width();  // See Paint().
    location_bar_->SetBounds(edge_width, PopupTopSpacing(),
        width() - (edge_width * 2), location_bar_->GetPreferredSize().height());
    return;
  }

  int child_y = std::min(kControlVertOffset, height());
  // We assume all child elements are the same height.
  int child_height =
      std::min(go_->GetPreferredSize().height(), height() - child_y);

  // If the window is maximized, we extend the back button to the left so that
  // clicking on the left-most pixel will activate the back button.
  // TODO(abarth):  If the window becomes maximized but is not resized,
  //                then Layout() might not be called and the back button
  //                will be slightly the wrong size.  We should force a
  //                Layout() in this case.
  //                http://crbug.com/5540
  int back_width = back_->GetPreferredSize().width();
  if (browser_->window() && browser_->window()->IsMaximized())
    back_->SetBounds(0, child_y, back_width + kControlIndent, child_height);
  else
    back_->SetBounds(kControlIndent, child_y, back_width, child_height);

  forward_->SetBounds(back_->x() + back_->width(), child_y,
                      forward_->GetPreferredSize().width(), child_height);

  reload_->SetBounds(forward_->x() + forward_->width() + kControlHorizOffset,
                     child_y, reload_->GetPreferredSize().width(),
                     child_height);

  if (show_home_button_.GetValue()) {
    home_->SetVisible(true);
    home_->SetBounds(reload_->x() + reload_->width() + kControlHorizOffset,
                     child_y, home_->GetPreferredSize().width(), child_height);
  } else {
    home_->SetVisible(false);
    home_->SetBounds(reload_->x() + reload_->width(), child_y, 0, child_height);
  }

  star_->SetBounds(home_->x() + home_->width() + kControlHorizOffset,
                   child_y, star_->GetPreferredSize().width(), child_height);

  int go_button_width = go_->GetPreferredSize().width();
  int browser_actions_width = browser_actions_->GetPreferredSize().width();
  int page_menu_width = page_menu_->GetPreferredSize().width();
  int app_menu_width = app_menu_->GetPreferredSize().width();
  int bookmark_menu_width = bookmark_menu_ ?
      bookmark_menu_->GetPreferredSize().width() : 0;
  int location_x = star_->x() + star_->width();
  int available_width = width() - kPaddingRight - bookmark_menu_width -
      app_menu_width - page_menu_width - browser_actions_width -
      kMenuButtonOffset - go_button_width - location_x;
  location_bar_->SetBounds(location_x, child_y, std::max(available_width, 0),
                           child_height);

  go_->SetBounds(location_bar_->x() + location_bar_->width(), child_y,
                 go_button_width, child_height);

  int next_menu_x = go_->x() + go_->width() + kMenuButtonOffset;

  if (bookmark_menu_) {
    bookmark_menu_->SetBounds(next_menu_x, child_y, bookmark_menu_width,
                              child_height);
    next_menu_x += bookmark_menu_width;
  }

  browser_actions_->SetBounds(
      next_menu_x, 0, browser_actions_width, height());
  next_menu_x += browser_actions_width;

  page_menu_->SetBounds(next_menu_x, child_y, page_menu_width, child_height);
  next_menu_x += page_menu_width;

  app_menu_->SetBounds(next_menu_x, child_y, app_menu_width, child_height);
}

void ToolbarView::Paint(gfx::Canvas* canvas) {
  View::Paint(canvas);

  if (IsDisplayModeNormal())
    return;

  // In maximized mode, we don't draw the endcaps on the location bar, because
  // when they're flush against the edge of the screen they just look glitchy.
  if (!browser_->window() || !browser_->window()->IsMaximized()) {
    int top_spacing = PopupTopSpacing();
    canvas->DrawBitmapInt(*kPopupBackgroundEdge, 0, top_spacing);
    canvas->DrawBitmapInt(*kPopupBackgroundEdge,
                          width() - kPopupBackgroundEdge->width(), top_spacing);
  }

  // For glass, we need to draw a black line below the location bar to separate
  // it from the content area.  For non-glass, the NonClientView draws the
  // toolbar background below the location bar for us.
  if (GetWindow()->GetNonClientView()->UseNativeFrame())
    canvas->FillRectInt(SK_ColorBLACK, 0, height() - 1, width(), 1);
}

void ToolbarView::ThemeChanged() {
  LoadLeftSideControlsImages();
  LoadCenterStackImages();
  LoadRightSideControlsImages();
}

void ToolbarView::ShowContextMenu(int x, int y, bool is_mouse_gesture) {
  if (acc_focused_view_)
    acc_focused_view_->ShowContextMenu(x, y, is_mouse_gesture);
}

void ToolbarView::DidGainFocus() {
  // Check to see if MSAA focus should be restored to previously focused button,
  // and if button is an enabled, visibled child of toolbar.
  if (!acc_focused_view_ ||
      (acc_focused_view_->GetParent()->GetID() != VIEW_ID_TOOLBAR) ||
      !acc_focused_view_->IsEnabled() ||
      !acc_focused_view_->IsVisible()) {
    // Find first accessible child (-1 to start search at parent).
    int first_acc_child = GetNextAccessibleViewIndex(-1, false);

    // No buttons enabled or visible.
    if (first_acc_child == -1)
      return;

    set_acc_focused_view(GetChildViewAt(first_acc_child));
  }

  // Default focus is on the toolbar.
  int view_index = VIEW_ID_TOOLBAR;

  // Set hot-tracking for child, and update focused_view for MSAA focus event.
  if (acc_focused_view_) {
    acc_focused_view_->SetHotTracked(true);

    // Show the tooltip for the view that got the focus.
    if (GetWidget()->GetTooltipManager())
      GetWidget()->GetTooltipManager()->ShowKeyboardTooltip(acc_focused_view_);

    // Update focused_view with MSAA-adjusted child id.
    view_index = acc_focused_view_->GetID();
  }

#if defined(OS_WIN)
  gfx::NativeView wnd = GetWidget()->GetNativeView();

  // Notify Access Technology that there was a change in keyboard focus.
  ::NotifyWinEvent(EVENT_OBJECT_FOCUS, wnd, OBJID_CLIENT,
                   static_cast<LONG>(view_index));
#else
  // TODO(port): deal with toolbar a11y focus.
  NOTIMPLEMENTED();
#endif
}

void ToolbarView::WillLoseFocus() {
  // Any tooltips that are active should be hidden when toolbar loses focus.
  if (GetWidget() && GetWidget()->GetTooltipManager())
    GetWidget()->GetTooltipManager()->HideKeyboardTooltip();
}

bool ToolbarView::OnKeyPressed(const views::KeyEvent& e) {
  // Paranoia check, button should be initialized upon toolbar gaining focus.
  if (!acc_focused_view_)
    return false;

  int focused_view = GetChildIndex(acc_focused_view_);
  int next_view = focused_view;

  switch (e.GetKeyCode()) {
    case base::VKEY_LEFT:
      next_view = GetNextAccessibleViewIndex(focused_view, true);
      break;
    case base::VKEY_RIGHT:
      next_view = GetNextAccessibleViewIndex(focused_view, false);
      break;
    case base::VKEY_DOWN:
    case base::VKEY_RETURN:
      // VKEY_SPACE is already handled by the default case.
      if (acc_focused_view_->GetID() == VIEW_ID_PAGE_MENU ||
          acc_focused_view_->GetID() == VIEW_ID_APP_MENU) {
        // If a menu button in toolbar is activated and its menu is displayed,
        // then active tooltip should be hidden.
        if (GetWidget()->GetTooltipManager())
          GetWidget()->GetTooltipManager()->HideKeyboardTooltip();
        // Safe to cast, given to above view id check.
        static_cast<views::MenuButton*>(acc_focused_view_)->Activate();
        if (!acc_focused_view_) {
          // Activate triggered a focus change, don't try to change focus.
          return true;
        }
        // Re-enable hot-tracking, as Activate() will disable it.
        acc_focused_view_->SetHotTracked(true);
        break;
      }
    default:
      // If key is not handled explicitly, pass it on to view.
      return acc_focused_view_->OnKeyPressed(e);
  }

  // No buttons enabled or visible.
  if (next_view == -1)
    return false;

  // Only send an event if focus moved.
  if (next_view != focused_view) {
    // Remove hot-tracking from old focused button.
    acc_focused_view_->SetHotTracked(false);

    // All is well, update the focused child member variable.
    acc_focused_view_ = GetChildViewAt(next_view);

    // Hot-track new focused button.
    acc_focused_view_->SetHotTracked(true);

    // Show the tooltip for the view that got the focus.
    if (GetWidget()->GetTooltipManager()) {
      GetWidget()->GetTooltipManager()->
          ShowKeyboardTooltip(GetChildViewAt(next_view));
    }
#if defined(OS_WIN)
    // Retrieve information to generate an MSAA focus event.
    gfx::NativeView wnd = GetWidget()->GetNativeView();
    int view_id = acc_focused_view_->GetID();
    // Notify Access Technology that there was a change in keyboard focus.
    ::NotifyWinEvent(EVENT_OBJECT_FOCUS, wnd, OBJID_CLIENT,
                     static_cast<LONG>(view_id));
#else
    NOTIMPLEMENTED();
#endif
    return true;
  }
  return false;
}

bool ToolbarView::OnKeyReleased(const views::KeyEvent& e) {
  // Paranoia check, button should be initialized upon toolbar gaining focus.
  if (!acc_focused_view_)
    return false;

  // Have keys be handled by the views themselves.
  return acc_focused_view_->OnKeyReleased(e);
}

bool ToolbarView::GetAccessibleName(std::wstring* name) {
  if (!accessible_name_.empty()) {
    (*name).assign(accessible_name_);
    return true;
  }
  return false;
}

bool ToolbarView::GetAccessibleRole(AccessibilityTypes::Role* role) {
  DCHECK(role);

  *role = AccessibilityTypes::ROLE_TOOLBAR;
  return true;
}

void ToolbarView::SetAccessibleName(const std::wstring& name) {
  accessible_name_.assign(name);
}

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, views::DragController implementation:

void ToolbarView::WriteDragData(views::View* sender,
                                int press_x,
                                int press_y,
                                OSExchangeData* data) {
  DCHECK(
      GetDragOperations(sender, press_x, press_y) != DragDropTypes::DRAG_NONE);

  UserMetrics::RecordAction(L"Toolbar_DragStar", profile_);

  // If there is a bookmark for the URL, add the bookmark drag data for it. We
  // do this to ensure the bookmark is moved, rather than creating an new
  // bookmark.
  TabContents* tab = browser_->GetSelectedTabContents();
  if (tab) {
    if (profile_ && profile_->GetBookmarkModel()) {
      const BookmarkNode* node = profile_->GetBookmarkModel()->
          GetMostRecentlyAddedNodeForURL(tab->GetURL());
      if (node) {
        BookmarkDragData bookmark_data(node);
        bookmark_data.Write(profile_, data);
      }
    }

    drag_utils::SetURLAndDragImage(tab->GetURL(),
                                   UTF16ToWideHack(tab->GetTitle()),
                                   tab->GetFavIcon(),
                                   data);
  }
}

int ToolbarView::GetDragOperations(views::View* sender, int x, int y) {
  DCHECK(sender == star_);
  TabContents* tab = browser_->GetSelectedTabContents();
  if (!tab || !tab->ShouldDisplayURL() || !tab->GetURL().is_valid()) {
    return DragDropTypes::DRAG_NONE;
  }
  if (profile_ && profile_->GetBookmarkModel() &&
      profile_->GetBookmarkModel()->IsBookmarked(tab->GetURL())) {
    return DragDropTypes::DRAG_MOVE | DragDropTypes::DRAG_COPY |
           DragDropTypes::DRAG_LINK;
  }
  return DragDropTypes::DRAG_COPY | DragDropTypes::DRAG_LINK;
}

////////////////////////////////////////////////////////////////////////////////
// ToolbarView, private:

int ToolbarView::PopupTopSpacing() const {
  return GetWindow()->GetNonClientView()->UseNativeFrame() ?
      0 : kPopupTopSpacingNonGlass;
}

void ToolbarView::CreateLeftSideControls() {
  back_ = new views::ButtonDropDown(this, back_menu_model_.get());
  back_->set_triggerable_event_flags(views::Event::EF_LEFT_BUTTON_DOWN |
                                  views::Event::EF_MIDDLE_BUTTON_DOWN);
  back_->set_tag(IDC_BACK);
  back_->SetImageAlignment(views::ImageButton::ALIGN_RIGHT,
                           views::ImageButton::ALIGN_TOP);
  back_->SetTooltipText(l10n_util::GetString(IDS_TOOLTIP_BACK));
  back_->SetAccessibleName(l10n_util::GetString(IDS_ACCNAME_BACK));
  back_->SetID(VIEW_ID_BACK_BUTTON);

  forward_ = new views::ButtonDropDown(this, forward_menu_model_.get());
  forward_->set_triggerable_event_flags(views::Event::EF_LEFT_BUTTON_DOWN |
                                        views::Event::EF_MIDDLE_BUTTON_DOWN);
  forward_->set_tag(IDC_FORWARD);
  forward_->SetTooltipText(l10n_util::GetString(IDS_TOOLTIP_FORWARD));
  forward_->SetAccessibleName(l10n_util::GetString(IDS_ACCNAME_FORWARD));
  forward_->SetID(VIEW_ID_FORWARD_BUTTON);

  reload_ = new views::ImageButton(this);
  reload_->set_tag(IDC_RELOAD);
  reload_->SetTooltipText(l10n_util::GetString(IDS_TOOLTIP_RELOAD));
  reload_->SetAccessibleName(l10n_util::GetString(IDS_ACCNAME_RELOAD));
  reload_->SetID(VIEW_ID_RELOAD_BUTTON);

  home_ = new views::ImageButton(this);
  home_->set_triggerable_event_flags(views::Event::EF_LEFT_BUTTON_DOWN |
                                  views::Event::EF_MIDDLE_BUTTON_DOWN);
  home_->set_tag(IDC_HOME);
  home_->SetTooltipText(l10n_util::GetString(IDS_TOOLTIP_HOME));
  home_->SetAccessibleName(l10n_util::GetString(IDS_ACCNAME_HOME));
  home_->SetID(VIEW_ID_HOME_BUTTON);

  LoadLeftSideControlsImages();

  AddChildView(back_);
  AddChildView(forward_);
  AddChildView(reload_);
  AddChildView(home_);
}

void ToolbarView::CreateCenterStack(Profile *profile) {
  star_ = new ToolbarStarToggle(this, this);
  star_->set_tag(IDC_STAR);
  star_->SetDragController(this);
  star_->SetTooltipText(l10n_util::GetString(IDS_TOOLTIP_STAR));
  star_->SetToggledTooltipText(l10n_util::GetString(IDS_TOOLTIP_STARRED));
  star_->SetAccessibleName(l10n_util::GetString(IDS_ACCNAME_STAR));
  star_->SetID(VIEW_ID_STAR_BUTTON);
  AddChildView(star_);

  location_bar_ = new LocationBarView(profile, browser_->command_updater(),
                                      model_, this,
                                      display_mode_ == DISPLAYMODE_LOCATION,
                                      this);

  // The Go button.
  go_ = new GoButton(location_bar_, browser_);
  go_->SetAccessibleName(l10n_util::GetString(IDS_ACCNAME_GO));
  go_->SetID(VIEW_ID_GO_BUTTON);

  LoadCenterStackImages();

  location_bar_->SetAccessibleName(l10n_util::GetString(IDS_ACCNAME_LOCATION));
  AddChildView(location_bar_);
  location_bar_->Init();
  AddChildView(go_);
}

void ToolbarView::CreateRightSideControls(Profile* profile) {
  browser_actions_ = new BrowserActionsContainer(profile, this);

  page_menu_ = new views::MenuButton(NULL, std::wstring(), this, false);
  page_menu_->SetAccessibleName(l10n_util::GetString(IDS_ACCNAME_PAGE));
  page_menu_->SetTooltipText(l10n_util::GetString(IDS_PAGEMENU_TOOLTIP));
  page_menu_->SetID(VIEW_ID_PAGE_MENU);

  app_menu_ = new views::MenuButton(NULL, std::wstring(), this, false);
  app_menu_->SetAccessibleName(l10n_util::GetString(IDS_ACCNAME_APP));
  app_menu_->SetTooltipText(l10n_util::GetStringF(IDS_APPMENU_TOOLTIP,
      l10n_util::GetString(IDS_PRODUCT_NAME)));
  app_menu_->SetID(VIEW_ID_APP_MENU);

  if (CommandLine::ForCurrentProcess()->HasSwitch(switches::kBookmarkMenu)) {
    bookmark_menu_ = new BookmarkMenuButton(browser_);
    AddChildView(bookmark_menu_);
  } else {
    bookmark_menu_ = NULL;
  }

  LoadRightSideControlsImages();

  AddChildView(browser_actions_);
  AddChildView(page_menu_);
  AddChildView(app_menu_);
}

void ToolbarView::LoadLeftSideControlsImages() {
  ThemeProvider* tp = GetThemeProvider();

  SkColor color = tp->GetColor(BrowserThemeProvider::COLOR_BUTTON_BACKGROUND);
  SkBitmap* background = tp->GetBitmapNamed(IDR_THEME_BUTTON_BACKGROUND);

  back_->SetImage(views::CustomButton::BS_NORMAL, tp->GetBitmapNamed(IDR_BACK));
  back_->SetImage(views::CustomButton::BS_HOT, tp->GetBitmapNamed(IDR_BACK_H));
  back_->SetImage(views::CustomButton::BS_PUSHED,
      tp->GetBitmapNamed(IDR_BACK_P));
  back_->SetImage(views::CustomButton::BS_DISABLED,
      tp->GetBitmapNamed(IDR_BACK_D));
  back_->SetBackground(color, background,
      tp->GetBitmapNamed(IDR_BACK_MASK));

  forward_->SetImage(views::CustomButton::BS_NORMAL,
      tp->GetBitmapNamed(IDR_FORWARD));
  forward_->SetImage(views::CustomButton::BS_HOT,
      tp->GetBitmapNamed(IDR_FORWARD_H));
  forward_->SetImage(views::CustomButton::BS_PUSHED,
      tp->GetBitmapNamed(IDR_FORWARD_P));
  forward_->SetImage(views::CustomButton::BS_DISABLED,
      tp->GetBitmapNamed(IDR_FORWARD_D));
  forward_->SetBackground(color, background,
      tp->GetBitmapNamed(IDR_FORWARD_MASK));

  reload_->SetImage(views::CustomButton::BS_NORMAL,
      tp->GetBitmapNamed(IDR_RELOAD));
  reload_->SetImage(views::CustomButton::BS_HOT,
      tp->GetBitmapNamed(IDR_RELOAD_H));
  reload_->SetImage(views::CustomButton::BS_PUSHED,
      tp->GetBitmapNamed(IDR_RELOAD_P));
  reload_->SetBackground(color, background,
      tp->GetBitmapNamed(IDR_BUTTON_MASK));

  home_->SetImage(views::CustomButton::BS_NORMAL, tp->GetBitmapNamed(IDR_HOME));
  home_->SetImage(views::CustomButton::BS_HOT, tp->GetBitmapNamed(IDR_HOME_H));
  home_->SetImage(views::CustomButton::BS_PUSHED,
      tp->GetBitmapNamed(IDR_HOME_P));
  home_->SetBackground(color, background,
      tp->GetBitmapNamed(IDR_BUTTON_MASK));
}

void ToolbarView::LoadCenterStackImages() {
  ThemeProvider* tp = GetThemeProvider();

  SkColor color = tp->GetColor(BrowserThemeProvider::COLOR_BUTTON_BACKGROUND);
  SkBitmap* background = tp->GetBitmapNamed(IDR_THEME_BUTTON_BACKGROUND);

  star_->SetImage(views::CustomButton::BS_NORMAL, tp->GetBitmapNamed(IDR_STAR));
  star_->SetImage(views::CustomButton::BS_HOT, tp->GetBitmapNamed(IDR_STAR_H));
  star_->SetImage(views::CustomButton::BS_PUSHED,
      tp->GetBitmapNamed(IDR_STAR_P));
  star_->SetImage(views::CustomButton::BS_DISABLED,
      tp->GetBitmapNamed(IDR_STAR_D));
  star_->SetToggledImage(views::CustomButton::BS_NORMAL,
      tp->GetBitmapNamed(IDR_STARRED));
  star_->SetToggledImage(views::CustomButton::BS_HOT,
      tp->GetBitmapNamed(IDR_STARRED_H));
  star_->SetToggledImage(views::CustomButton::BS_PUSHED,
      tp->GetBitmapNamed(IDR_STARRED_P));
  star_->SetBackground(color, background,
      tp->GetBitmapNamed(IDR_STAR_MASK));

  go_->SetImage(views::CustomButton::BS_NORMAL, tp->GetBitmapNamed(IDR_GO));
  go_->SetImage(views::CustomButton::BS_HOT, tp->GetBitmapNamed(IDR_GO_H));
  go_->SetImage(views::CustomButton::BS_PUSHED, tp->GetBitmapNamed(IDR_GO_P));
  go_->SetToggledImage(views::CustomButton::BS_NORMAL,
      tp->GetBitmapNamed(IDR_STOP));
  go_->SetToggledImage(views::CustomButton::BS_HOT,
      tp->GetBitmapNamed(IDR_STOP_H));
  go_->SetToggledImage(views::CustomButton::BS_PUSHED,
      tp->GetBitmapNamed(IDR_STOP_P));
  go_->SetBackground(color, background,
      tp->GetBitmapNamed(IDR_GO_MASK));
}

void ToolbarView::LoadRightSideControlsImages() {
  ThemeProvider* tp = GetThemeProvider();

  // We use different menu button images if the locale is right-to-left.
  if (UILayoutIsRightToLeft())
    page_menu_->SetIcon(*tp->GetBitmapNamed(IDR_MENU_PAGE_RTL));
  else
    page_menu_->SetIcon(*tp->GetBitmapNamed(IDR_MENU_PAGE));
  if (UILayoutIsRightToLeft())
    app_menu_->SetIcon(*tp->GetBitmapNamed(IDR_MENU_CHROME_RTL));
  else
    app_menu_->SetIcon(*tp->GetBitmapNamed(IDR_MENU_CHROME));

  if (bookmark_menu_ != NULL)
    bookmark_menu_->SetIcon(*tp->GetBitmapNamed(IDR_MENU_BOOKMARK));
}

void ToolbarView::RunPageMenu(const gfx::Point& pt, gfx::NativeView parent) {
  CreatePageMenu();
  page_menu_menu_->RunMenuAt(pt, views::Menu2::ALIGN_TOPRIGHT);
}

void ToolbarView::RunAppMenu(const gfx::Point& pt, gfx::NativeView parent) {
  CreateAppMenu();
  app_menu_menu_->RunMenuAt(pt, views::Menu2::ALIGN_TOPRIGHT);
}

void ToolbarView::CreatePageMenu() {
  if (page_menu_contents_.get())
    return;

  page_menu_contents_.reset(new views::SimpleMenuModel(this));
  page_menu_contents_->AddItemWithStringId(IDC_CREATE_SHORTCUTS,
                                           IDS_CREATE_SHORTCUTS);
  page_menu_contents_->AddSeparator();
  page_menu_contents_->AddItemWithStringId(IDC_CUT, IDS_CUT);
  page_menu_contents_->AddItemWithStringId(IDC_COPY, IDS_COPY);
  page_menu_contents_->AddItemWithStringId(IDC_PASTE, IDS_PASTE);
  page_menu_contents_->AddSeparator();
  page_menu_contents_->AddItemWithStringId(IDC_FIND, IDS_FIND);
  page_menu_contents_->AddItemWithStringId(IDC_SAVE_PAGE, IDS_SAVE_PAGE);
  page_menu_contents_->AddItemWithStringId(IDC_PRINT, IDS_PRINT);
  page_menu_contents_->AddSeparator();

  zoom_menu_contents_.reset(new ZoomMenuModel(this));
  page_menu_contents_->AddSubMenuWithStringId(
      IDS_ZOOM_MENU, zoom_menu_contents_.get());

  encoding_menu_contents_.reset(new EncodingMenuModel(browser_));
  page_menu_contents_->AddSubMenuWithStringId(
      IDS_ENCODING_MENU, encoding_menu_contents_.get());

#if defined(OS_WIN)
  CreateDevToolsMenuContents();
  page_menu_contents_->AddSeparator();
  page_menu_contents_->AddSubMenuWithStringId(
      IDS_DEVELOPER_MENU, devtools_menu_contents_.get());

  page_menu_contents_->AddSeparator();
  page_menu_contents_->AddItemWithStringId(IDC_REPORT_BUG, IDS_REPORT_BUG);
#else
  NOTIMPLEMENTED();
#endif

  page_menu_menu_.reset(new views::Menu2(page_menu_contents_.get()));
}

#if defined(OS_WIN)
void ToolbarView::CreateDevToolsMenuContents() {
  devtools_menu_contents_.reset(new views::SimpleMenuModel(this));
  devtools_menu_contents_->AddItem(IDC_VIEW_SOURCE,
                                   l10n_util::GetString(IDS_VIEW_SOURCE));
  if (g_browser_process->have_inspector_files()) {
    devtools_menu_contents_->AddItem(IDC_DEV_TOOLS,
                                     l10n_util::GetString(IDS_DEV_TOOLS));
  }
  devtools_menu_contents_->AddItem(IDC_TASK_MANAGER,
                                   l10n_util::GetString(IDS_TASK_MANAGER));
}
#endif

void ToolbarView::CreateAppMenu() {
  // We always rebuild the app menu so that we can get the current state of the
  // extension system.

  app_menu_contents_.reset(new views::SimpleMenuModel(this));
  app_menu_contents_->AddItemWithStringId(IDC_NEW_TAB, IDS_NEW_TAB);
  app_menu_contents_->AddItemWithStringId(IDC_NEW_WINDOW, IDS_NEW_WINDOW);
  app_menu_contents_->AddItemWithStringId(IDC_NEW_INCOGNITO_WINDOW,
                                          IDS_NEW_INCOGNITO_WINDOW);
  // Enumerate profiles asynchronously and then create the parent menu item.
  // We will create the child menu items for this once the asynchronous call is
  // done.  See OnGetProfilesDone().
  const CommandLine& command_line = *CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kEnableUserDataDirProfiles) &&
      !profiles_menu_contents_.get()) {
    profiles_helper_->GetProfiles(NULL);
    profiles_menu_contents_.reset(new views::SimpleMenuModel(this));
    app_menu_contents_->AddSubMenuWithStringId(IDS_PROFILE_MENU,
                                               profiles_menu_contents_.get());
  }

  app_menu_contents_->AddSeparator();
  app_menu_contents_->AddCheckItemWithStringId(IDC_SHOW_BOOKMARK_BAR,
                                               IDS_SHOW_BOOKMARK_BAR);
  app_menu_contents_->AddItemWithStringId(IDC_FULLSCREEN, IDS_FULLSCREEN);
  app_menu_contents_->AddSeparator();
  app_menu_contents_->AddItemWithStringId(IDC_SHOW_HISTORY, IDS_SHOW_HISTORY);
  app_menu_contents_->AddItemWithStringId(IDC_SHOW_BOOKMARK_MANAGER,
                                          IDS_BOOKMARK_MANAGER);
  app_menu_contents_->AddItemWithStringId(IDC_SHOW_DOWNLOADS,
                                          IDS_SHOW_DOWNLOADS);

  // Create the extensions item or submenu.
  // If there are any browser actions, we create an "Extensions" submenu, of
  // which "Manage extensions" is the first entry. If there are no browser
  // actions, we just create an "Extensions" menu item which does the same thing
  // as "Manage extensions".
  ExtensionsService* extensions_service =
      browser_->profile()->GetExtensionsService();
  if (extensions_service && extensions_service->extensions_enabled()) {
    // Get a count of all non-popup browser actions to decide how to layout
    // the Extensions menu.
    std::vector<ExtensionAction*> browser_actions =
        browser_->profile()->GetExtensionsService()->GetBrowserActions(false);
    if (browser_actions.size() == 0) {
      app_menu_contents_->AddItemWithStringId(IDC_MANAGE_EXTENSIONS,
                                              IDS_SHOW_EXTENSIONS);
    } else {
      extension_menu_contents_.reset(new views::SimpleMenuModel(this));
      app_menu_contents_->AddSubMenuWithStringId(
          IDS_SHOW_EXTENSIONS, extension_menu_contents_.get());

      extension_menu_contents_->AddItemWithStringId(IDC_MANAGE_EXTENSIONS,
                                                    IDS_MANAGE_EXTENSIONS);

      // TODO(erikkay) Even though we just got the list of all browser actions,
      // we have to enumerate the list of extensions in order to get the action
      // state.  It seems like we should find a way to combine these.
      const ExtensionList* extensions = extensions_service->extensions();
      for (size_t i = 0; i < extensions->size(); ++i) {
        Extension* extension = extensions->at(i);
        if (!extension->browser_action()) {
          continue;
        } else if (extension->browser_action()->command_id() >
                   IDC_BROWSER_ACTION_LAST) {
          NOTREACHED() << "Too many browser actions.";
        } else if (!extension->browser_action()->is_popup()) {
          extension_menu_contents_->AddItem(
              extension->browser_action()->command_id(),
              UTF8ToUTF16(extension->browser_action_state()->title()));
        }
      }
    }
  }

  app_menu_contents_->AddSeparator();
#ifdef CHROME_PERSONALIZATION
  if (!CommandLine::ForCurrentProcess()->HasSwitch(switches::kDisableSync)) {
    app_menu_contents_->AddItem(IDC_SYNC_BOOKMARKS,
        l10n_util::GetString(IDS_SYNC_MY_BOOKMARKS_LABEL) + L"...");
    app_menu_contents_->AddSeparator();
  }
#endif
  app_menu_contents_->AddItem(IDC_OPTIONS,
                              l10n_util::GetStringFUTF16(
                                  IDS_OPTIONS,
                                  l10n_util::GetStringUTF16(IDS_PRODUCT_NAME)));
  app_menu_contents_->AddItem(IDC_ABOUT,
                              l10n_util::GetStringFUTF16(
                                  IDS_ABOUT,
                                  l10n_util::GetStringUTF16(IDS_PRODUCT_NAME)));
  app_menu_contents_->AddItemWithStringId(IDC_HELP_PAGE, IDS_HELP_PAGE);
  app_menu_contents_->AddSeparator();
  app_menu_contents_->AddItemWithStringId(IDC_EXIT, IDS_EXIT);

  app_menu_menu_.reset(new views::Menu2(app_menu_contents_.get()));
}
