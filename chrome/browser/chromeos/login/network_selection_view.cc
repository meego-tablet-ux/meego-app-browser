// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/network_selection_view.h"

#include <signal.h>
#include <sys/types.h>
#include <string>

#include "app/l10n_util.h"
#include "app/resource_bundle.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/chromeos/login/helper.h"
#include "chrome/browser/chromeos/login/language_switch_menu.h"
#include "chrome/browser/chromeos/login/network_screen_delegate.h"
#include "chrome/browser/chromeos/login/rounded_rect_painter.h"
#include "chrome/browser/chromeos/status/network_dropdown_button.h"
#include "gfx/size.h"
#include "grit/chromium_strings.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"
#include "views/controls/button/native_button.h"
#include "views/controls/label.h"
#include "views/controls/throbber.h"
#include "views/grid_layout.h"
#include "views/standard_layout.h"
#include "views/widget/widget.h"
#include "views/widget/widget_gtk.h"
#include "views/window/non_client_view.h"
#include "views/window/window.h"
#include "views/window/window_gtk.h"

using views::Background;
using views::GridLayout;
using views::Label;
using views::View;
using views::Widget;
using views::WidgetGtk;

namespace {

enum kLayoutColumnsets {
  STANDARD_ROW,
  THROBBER_ROW,
};

enum kContentsLayoutColumnsets {
  WELCOME_ROW,
  CONTENTS_ROW,
};

// Grid layout constants.
const int kBorderSize = 10;
const int kWelcomeTitlePadding = 10;
const int kPaddingColumnWidth = 55;
const int kMediumPaddingColumnWidth = 20;
const int kControlPaddingRow = 15;

// Fixed size for language/network controls height.
const int kSelectionBoxHeight = 29;

// Menu button is drawn using our custom icons in resources. See
// TextButtonBorder::Paint() for details. So this offset compensate
// horizontal size, eaten by those icons.
const int kMenuHorizontalOffset = -1;

// Vertical addition to the menu window to make it appear exactly below
// MenuButton.
const int kMenuVerticalOffset = 3;

// Offset that compensates menu width so that it matches
// menu button visual width when being in pushed state.
const int kMenuWidthOffset = 6;

const SkColor kWelcomeColor = 0xFFCDD3D6;

}  // namespace

namespace chromeos {

// NetworkDropdownButton with custom accelerator enabled.
class NetworkControlWithAccelerators : public NetworkDropdownButton {
 public:
  NetworkControlWithAccelerators(bool browser_mode,
                                 gfx::NativeWindow parent_window,
                                 NetworkScreenDelegate* delegate)
      : NetworkDropdownButton(browser_mode, parent_window),
        delegate_(delegate),
        accel_clear_errors_(views::Accelerator(app::VKEY_ESCAPE,
                                               false, false, false)) {
    AddAccelerator(accel_clear_errors_);
  }

  // Overridden from View:
  bool AcceleratorPressed(const views::Accelerator& accel) {
    if (accel == accel_clear_errors_) {
      delegate_->ClearErrors();
      return true;
    }
    return false;
  }

  // Overridden from MenuButton:
  virtual bool Activate() {
    delegate_->ClearErrors();
    return MenuButton::Activate();
  }

 private:
  NetworkScreenDelegate* delegate_;

  // ESC accelerator for closing error info bubble.
  views::Accelerator accel_clear_errors_;

  DISALLOW_COPY_AND_ASSIGN(NetworkControlWithAccelerators);
};

// MenuButton with custom processing on focus events.
class NotifyingMenuButton : public views::MenuButton {
 public:
  NotifyingMenuButton(views::ButtonListener* listener,
                      const std::wstring& text,
                      views::ViewMenuDelegate* menu_delegate,
                      bool show_menu_marker,
                      NetworkScreenDelegate* delegate)
      : MenuButton(listener, text, menu_delegate, show_menu_marker),
        delegate_(delegate) {}

  // Overridden from View:
  virtual void DidGainFocus() {
    delegate_->ClearErrors();
  }

 private:
  NetworkScreenDelegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(NotifyingMenuButton);
};

NetworkSelectionView::NetworkSelectionView(NetworkScreenDelegate* delegate)
    : contents_view_(NULL),
      languages_menubutton_(NULL),
      welcome_label_(NULL),
      select_language_label_(NULL),
      select_network_label_(NULL),
      connecting_network_label_(NULL),
      network_dropdown_(NULL),
      continue_button_(NULL),
      throbber_(CreateDefaultSmoothedThrobber()),
      proxy_settings_link_(NULL),
      delegate_(delegate) {
}

NetworkSelectionView::~NetworkSelectionView() {
  throbber_->Stop();
  throbber_ = NULL;
}

void NetworkSelectionView::AddControlsToLayout(const gfx::Size& size,
    views::GridLayout* contents_layout) {
  if (IsConnecting()) {
    const int v_padding = (size.height() -
        throbber_->GetPreferredSize().height()) / 2;
    contents_layout->AddPaddingRow(0, v_padding);
    contents_layout->StartRow(0, THROBBER_ROW);
    contents_layout->AddView(connecting_network_label_);
    contents_layout->AddView(throbber_);
    contents_layout->AddPaddingRow(0, v_padding);
  } else {
    const int v_padding = (size.height() -
        3 * kControlPaddingRow - 2 * kSelectionBoxHeight -
        proxy_settings_link_->GetPreferredSize().height() -
        continue_button_->GetPreferredSize().height()) / 2;
    contents_layout->AddPaddingRow(0, v_padding);
    contents_layout->StartRow(0, STANDARD_ROW);
    contents_layout->AddView(select_language_label_);
    contents_layout->AddView(languages_menubutton_, 1, 1,
                    GridLayout::FILL, GridLayout::FILL,
                    languages_menubutton_->GetPreferredSize().width(),
                    kSelectionBoxHeight);
    contents_layout->AddPaddingRow(0, kControlPaddingRow);
    contents_layout->StartRow(0, STANDARD_ROW);
    contents_layout->AddView(select_network_label_);
    contents_layout->AddView(network_dropdown_, 1, 1,
                    GridLayout::FILL, GridLayout::FILL,
                    network_dropdown_->GetPreferredSize().width(),
                    kSelectionBoxHeight);
    contents_layout->AddPaddingRow(0, kControlPaddingRow);
    contents_layout->StartRow(0, STANDARD_ROW);
    contents_layout->SkipColumns(1);
    contents_layout->AddView(proxy_settings_link_, 1, 1,
                    GridLayout::LEADING, GridLayout::CENTER);
    contents_layout->AddPaddingRow(0, kControlPaddingRow);
    contents_layout->StartRow(0, STANDARD_ROW);
    contents_layout->SkipColumns(1);
    contents_layout->AddView(continue_button_, 1, 1,
                    GridLayout::LEADING, GridLayout::CENTER);
    contents_layout->AddPaddingRow(0, v_padding);
  }
}

void NetworkSelectionView::InitLayout() {
  gfx::Size screen_size = delegate_->size();
  const int widest_label = std::max(
      select_language_label_->GetPreferredSize().width(),
      select_network_label_->GetPreferredSize().width());
  const int dropdown_width = screen_size.width() - 2 * kBorderSize -
      2 * kPaddingColumnWidth - kMediumPaddingColumnWidth - widest_label;
  delegate_->language_switch_menu()->SetFirstLevelMenuWidth(
      dropdown_width - kMenuWidthOffset);

  // Define layout and column set for entire screen (welcome + screen).
  views::GridLayout* screen_layout = new views::GridLayout(this);
  SetLayoutManager(screen_layout);
  views::ColumnSet* column_set = screen_layout->AddColumnSet(WELCOME_ROW);
  const int welcome_width = screen_size.width() - 2 * kWelcomeTitlePadding -
      2 * kBorderSize;
  column_set->AddPaddingColumn(0, kWelcomeTitlePadding + kBorderSize);
  column_set->AddColumn(GridLayout::FILL, GridLayout::FILL, 0,
                        GridLayout::FIXED, welcome_width, welcome_width);
  column_set->AddPaddingColumn(0, kWelcomeTitlePadding + kBorderSize);
  column_set = screen_layout->AddColumnSet(CONTENTS_ROW);
  column_set->AddColumn(GridLayout::FILL, GridLayout::FILL, 0,
      GridLayout::FIXED, screen_size.width(), screen_size.width());
  screen_layout->StartRow(0, WELCOME_ROW);
  screen_layout->AddView(welcome_label_);
  screen_layout->StartRow(0, CONTENTS_ROW);
  screen_layout->AddView(contents_view_);

  // Welcome label size might have been changed after adding to grid layout.
  // Screen size includes welcome label height & border on each side.
  screen_size.set_height(screen_size.height() - 2 * kBorderSize -
                         welcome_label_->GetPreferredSize().height());

  // Define layout and column set for screen contents.
  views::GridLayout* contents_layout = new views::GridLayout(contents_view_);
  contents_view_->SetLayoutManager(contents_layout);

  column_set = contents_layout->AddColumnSet(STANDARD_ROW);
  column_set->AddPaddingColumn(0, kPaddingColumnWidth);
  column_set->AddColumn(GridLayout::LEADING, GridLayout::FILL, 0,
                        GridLayout::FIXED, widest_label, widest_label);
  column_set->AddPaddingColumn(0, kMediumPaddingColumnWidth);
  column_set->AddColumn(GridLayout::FILL, GridLayout::FILL, 0,
                        GridLayout::FIXED, dropdown_width, dropdown_width);
  column_set->AddPaddingColumn(1, kPaddingColumnWidth);

  const int h_padding = (screen_size.width() - 2 * kBorderSize -
      connecting_network_label_->GetPreferredSize().width() -
      throbber_->GetPreferredSize().width()) / 2;
  column_set = contents_layout->AddColumnSet(THROBBER_ROW);
  column_set->AddPaddingColumn(0, h_padding);
  column_set->AddColumn(GridLayout::TRAILING, GridLayout::CENTER, 0,
                        GridLayout::USE_PREF, 0, 0);
  column_set->AddPaddingColumn(0, kRelatedControlHorizontalSpacing);
  column_set->AddColumn(GridLayout::LEADING, GridLayout::CENTER, 1,
                        GridLayout::USE_PREF, 0, 0);
  column_set->AddPaddingColumn(0, h_padding);

  AddControlsToLayout(screen_size, contents_layout);
}

void NetworkSelectionView::Init() {
  contents_view_ = new views::View();

  // Use rounded rect background.
  views::Painter* painter = CreateWizardPainter(
      &BorderDefinition::kScreenBorder);
  contents_view_->set_background(
      views::Background::CreateBackgroundPainter(true, painter));

  ResourceBundle& rb = ResourceBundle::GetSharedInstance();
  gfx::Font welcome_label_font =
      rb.GetFont(ResourceBundle::LargeFont).DeriveFont(0, gfx::Font::BOLD);

  welcome_label_ = new views::Label();
  welcome_label_->SetColor(kWelcomeColor);
  welcome_label_->SetFont(welcome_label_font);
  welcome_label_->SetMultiLine(true);

  select_language_label_ = new views::Label();
  select_language_label_->SetFont(rb.GetFont(ResourceBundle::MediumFont));

  languages_menubutton_ = new NotifyingMenuButton(
      NULL, std::wstring(), delegate_->language_switch_menu(), true, delegate_);
  languages_menubutton_->SetFocusable(true);
  languages_menubutton_->SetNormalHasBorder(true);
  // Menu is positioned by bottom right corner of the MenuButton.
  delegate_->language_switch_menu()->set_menu_offset(kMenuHorizontalOffset,
                                                     kMenuVerticalOffset);

  select_network_label_ = new views::Label();
  select_network_label_->SetFont(rb.GetFont(ResourceBundle::MediumFont));

  network_dropdown_ = new NetworkControlWithAccelerators(false,
                                                         GetNativeWindow(),
                                                         delegate_);
  network_dropdown_->SetNormalHasBorder(true);
  network_dropdown_->SetFocusable(true);

  connecting_network_label_ = new views::Label();
  connecting_network_label_->SetFont(rb.GetFont(ResourceBundle::MediumFont));
  connecting_network_label_->SetVisible(false);

  proxy_settings_link_ = new views::Link();
  proxy_settings_link_->SetController(this);
  proxy_settings_link_->SetVisible(true);
  proxy_settings_link_->SetFocusable(true);

  UpdateLocalizedStrings();
}

void NetworkSelectionView::UpdateLocalizedStrings() {
  languages_menubutton_->SetText(
      delegate_->language_switch_menu()->GetCurrentLocaleName());
  welcome_label_->SetText(l10n_util::GetStringF(IDS_NETWORK_SELECTION_TITLE,
                          l10n_util::GetString(IDS_PRODUCT_OS_NAME)));
  select_language_label_->SetText(
      l10n_util::GetString(IDS_LANGUAGE_SELECTION_SELECT));
  select_network_label_->SetText(
      l10n_util::GetString(IDS_NETWORK_SELECTION_SELECT));
  proxy_settings_link_->SetText(
      l10n_util::GetString(IDS_OPTIONS_PROXIES_CONFIGURE_BUTTON));
  RecreateNativeControls();
  UpdateConnectingNetworkLabel();
  network_dropdown_->Refresh();
  InitLayout();
}

////////////////////////////////////////////////////////////////////////////////
// views::View: implementation:

void NetworkSelectionView::OnLocaleChanged() {
  UpdateLocalizedStrings();
  // Proxy settings dialog contains localized title.  Zap it.
  proxy_settings_dialog_.reset(NULL);

  Layout();
  SchedulePaint();
}

////////////////////////////////////////////////////////////////////////////////
// NetworkSelectionView, public:

gfx::NativeWindow NetworkSelectionView::GetNativeWindow() const {
  return GTK_WINDOW(static_cast<WidgetGtk*>(GetWidget())->GetNativeView());
}

views::View* NetworkSelectionView::GetNetworkControlView() const {
  return network_dropdown_;
}

void NetworkSelectionView::ShowConnectingStatus(bool connecting,
                                                const string16& network_id) {
  network_id_ = network_id;
  UpdateConnectingNetworkLabel();
  select_language_label_->SetVisible(!connecting);
  languages_menubutton_->SetVisible(!connecting);
  select_network_label_->SetVisible(!connecting);
  network_dropdown_->SetVisible(!connecting);
  continue_button_->SetVisible(!connecting);
  connecting_network_label_->SetVisible(connecting);
  InitLayout();
  Layout();
  if (connecting) {
    throbber_->Start();
    network_dropdown_->CancelMenu();
  } else {
    throbber_->Stop();
  }
}

bool NetworkSelectionView::IsConnecting() const {
  return connecting_network_label_->IsVisible();
}

void NetworkSelectionView::EnableContinue(bool enabled) {
  if (continue_button_)
    continue_button_->SetEnabled(enabled);
}

bool NetworkSelectionView::IsContinueEnabled() const {
  return continue_button_ && continue_button_->IsEnabled();
}

////////////////////////////////////////////////////////////////////////////////
// views::LinkController implementation:

void NetworkSelectionView::LinkActivated(views::Link* source, int) {
  if (source == proxy_settings_link_) {
    if (!proxy_settings_dialog_.get()) {
      static const char kProxySettingsURL[] = "chrome://options/proxy";
      proxy_settings_dialog_.reset(new LoginHtmlDialog(
          this,
          GetNativeWindow(),
          l10n_util::GetString(IDS_OPTIONS_PROXY_TAB_LABEL),
          GURL(kProxySettingsURL)));
    }
    proxy_settings_dialog_->Show();
  }
}

////////////////////////////////////////////////////////////////////////////////
// NetworkSelectionView, private:

void NetworkSelectionView::RecreateNativeControls() {
  // There is no way to get native button preferred size after the button was
  // sized so delete and recreate the button on text update.
  bool is_continue_enabled = IsContinueEnabled();
  delete continue_button_;
  continue_button_ = new views::NativeButton(
      delegate_,
      l10n_util::GetString(IDS_NETWORK_SELECTION_CONTINUE_BUTTON));
  continue_button_->SetEnabled(is_continue_enabled);
}

void NetworkSelectionView::UpdateConnectingNetworkLabel() {
  connecting_network_label_->SetText(l10n_util::GetStringF(
      IDS_NETWORK_SELECTION_CONNECTING, UTF16ToWide(network_id_)));
}

}  // namespace chromeos
