// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_info_window.h"

#include <cryptuiapi.h>
#pragma comment(lib, "cryptui.lib")

#include "base/string_util.h"
#include "base/time_format.h"
#include "chrome/app/locales/locale_settings.h"
#include "chrome/app/theme/theme_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/cert_store.h"
#include "chrome/browser/history/history.h"
#include "chrome/browser/navigation_entry.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/standard_layout.h"
#include "chrome/browser/ssl_manager.h"
#include "chrome/common/l10n_util.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/pref_service.h"
#include "chrome/common/resource_bundle.h"
#include "chrome/common/win_util.h"
#include "chrome/views/background.h"
#include "chrome/views/grid_layout.h"
#include "chrome/views/image_view.h"
#include "chrome/views/label.h"
#include "chrome/views/native_button.h"
#include "chrome/views/separator.h"
#include "net/base/cert_status_flags.h"
#include "net/base/x509_certificate.h"
#include "skia/include/SkColor.h"
#include "generated_resources.h"

const int kVerticalPadding = 10;
const int kHorizontalPadding = 10;

////////////////////////////////////////////////////////////////////////////////
// SecurityTabView
class SecurityTabView : public ChromeViews::View {
 public:
  SecurityTabView(Profile* profile, NavigationEntry* navigation_entry);
  virtual ~SecurityTabView();

  virtual void Layout();

  // Add a section.
  virtual void AddSection(const std::wstring& title,
                          bool state,
                          const std::wstring& head_line,
                          const std::wstring& description);

 private:
  // A section contains an image that shows a status (good or bad), a title,
  // an optional head-line (in bold) and a description.
  class Section : public ChromeViews::View {
   public:
    Section(const std::wstring& title,
            bool state,
            const std::wstring& head_line,
            const std::wstring& description);
    virtual ~Section();

    virtual int GetHeightForWidth(int w);
    virtual void Layout();

   private:
    // The text placed on top of the section (on the left of the separator bar).
    std::wstring title_;

    // Whether to show the good/bad icon.
    bool state_;

    // The first line of the description, show in bold.
    std::wstring head_line_;

    // The description, displayed below the head line.
    std::wstring description_;

    static SkBitmap* good_state_icon_;
    static SkBitmap* bad_state_icon_;

    ChromeViews::Label* title_label_;
    ChromeViews::Separator* separator_;
    ChromeViews::ImageView* status_image_;
    ChromeViews::Label* head_line_label_;
    ChromeViews::Label* description_label_;

    DISALLOW_EVIL_CONSTRUCTORS(Section);
  };

  // Returns a name that can be used to represent the issuer.  It tries in this
  // order CN, O and OU and returns the first non-empty one found.
  static std::string GetIssuerName(
      const net::X509Certificate::Principal& issuer);

  // Callback from history service with number of visits to url.
  void OnGotVisitCountToHost(HistoryService::Handle handle,
                             bool found_visits,
                             int count,
                             Time first_visit);

  std::vector<Section*> sections_;

  // Used to request number of visits.
  CancelableRequestConsumer request_consumer_;

  DISALLOW_EVIL_CONSTRUCTORS(SecurityTabView);
};

// static
SkBitmap* SecurityTabView::Section::good_state_icon_ = NULL;
SkBitmap* SecurityTabView::Section::bad_state_icon_ = NULL;

// Layout constants.
static const int kHGapToBorder = 6;
static const int kVGapToBorder = 6;
static const int kHGapTitleToSeparator = 2;
static const int kVGapTitleToImage = 6;
static const int kHGapImageToDescription = 6;
static const int kVGapHeadLineToDescription = 2;
static const int kVGapBetweenSections = 20;
static const int kHExtraSeparatorPadding = 2;

SecurityTabView::Section::Section(const std::wstring& title, bool state,
                                  const std::wstring& head_line,
                                  const std::wstring& description)
    : title_(title),
      state_(state),
      head_line_(head_line),
      description_(description) {
  if (!good_state_icon_) {
    ResourceBundle& rb = ResourceBundle::GetSharedInstance();
    good_state_icon_ = rb.GetBitmapNamed(IDR_PAGEINFO_GOOD);
    bad_state_icon_ = rb.GetBitmapNamed(IDR_PAGEINFO_BAD);
  }
  title_label_ = new ChromeViews::Label(title);
  title_label_->SetHorizontalAlignment(ChromeViews::Label::ALIGN_LEFT);
  AddChildView(title_label_);

  separator_ = new ChromeViews::Separator();
  AddChildView(separator_);

  status_image_ = new ChromeViews::ImageView();
  status_image_->SetImage(state ? good_state_icon_ : bad_state_icon_);
  AddChildView(status_image_);

  head_line_label_ = new ChromeViews::Label(head_line);
  head_line_label_->SetFont(
      head_line_label_->GetFont().DeriveFont(0, ChromeFont::BOLD));
  head_line_label_->SetHorizontalAlignment(ChromeViews::Label::ALIGN_LEFT);
  AddChildView(head_line_label_);

  description_label_ = new ChromeViews::Label(description);
  description_label_->SetMultiLine(true);
  description_label_->SetHorizontalAlignment(ChromeViews::Label::ALIGN_LEFT);
  AddChildView(description_label_);
}

SecurityTabView::Section::~Section() {
}

int SecurityTabView::Section::GetHeightForWidth(int width) {
  // The height of the section depends on the height of the description label
  // (multi-line).  We need to know the width of the description label to know
  // its height.
  int height = 0;
  CSize size;
  title_label_->GetPreferredSize(&size);
  height += size.cy + kVGapTitleToImage;

  CSize image_size;
  status_image_->GetPreferredSize(&image_size);

  int text_height = 0;
  if (!head_line_label_->GetText().empty()) {
    head_line_label_->GetPreferredSize(&size);
    text_height = size.cy + kVGapHeadLineToDescription;
  }

  int description_width = width - image_size.cx - kHGapImageToDescription -
                          kHGapToBorder;
  text_height += description_label_->GetHeightForWidth(description_width);

  height += std::max(static_cast<int>(image_size.cy), text_height);

  return height;
}

void SecurityTabView::Section::Layout() {
  // First, layout the title and separator.
  int x = 0;
  int y = 0;
  CSize size;
  title_label_->GetPreferredSize(&size);
  title_label_->SetBounds(x, y, size.cx, size.cy);
  x += size.cx + kHGapTitleToSeparator;
  separator_->SetBounds(x + kHExtraSeparatorPadding, y,
                        GetWidth() - x - 2 * kHExtraSeparatorPadding, size.cy);

  // Then the image, head-line and description.
  x = kHGapToBorder;
  y += title_label_->GetHeight() + kVGapTitleToImage;
  status_image_->GetPreferredSize(&size);
  status_image_->SetBounds(x, y, size.cx, size.cy);
  x += size.cx + kHGapImageToDescription;
  int w = GetWidth() - x;
  if (!head_line_label_->GetText().empty()) {
    head_line_label_->GetPreferredSize(&size);
    head_line_label_->SetBounds(x, y, w > 0 ? w : 0, size.cy);
    y += size.cy + kVGapHeadLineToDescription;
  } else {
    head_line_label_->SetBounds(x, y, 0, 0);
  }
  if (w > 0) {
    description_label_->SetBounds(x, y, w,
                                  description_label_->GetHeightForWidth(w));
  } else {
    description_label_->SetBounds(x, y, 0, 0);
  }
}

SecurityTabView::SecurityTabView(Profile* profile,
                                 NavigationEntry* navigation_entry) {
  bool identity_ok = true;
  bool connection_ok = true;
  std::wstring identity_title;
  std::wstring identity_msg;
  std::wstring connection_msg;
  scoped_refptr<net::X509Certificate> cert;
  const NavigationEntry::SSLStatus& ssl = navigation_entry->ssl();

  // Identity section.
  std::wstring subject_name(UTF8ToWide(navigation_entry->url().host()));
  bool empty_subject_name = false;
  if (subject_name.empty()) {
    subject_name.assign(
        l10n_util::GetString(IDS_PAGE_INFO_SECURITY_TAB_UNKNOWN_PARTY));
    empty_subject_name = true;
  }
  if (navigation_entry->page_type() == NavigationEntry::NORMAL_PAGE &&
      ssl.cert_id() &&
      CertStore::GetSharedInstance()->RetrieveCert(ssl.cert_id(), &cert) &&
      !net::IsCertStatusError(ssl.cert_status())) {
    // OK HTTPS page.
    if ((ssl.cert_status() & net::CERT_STATUS_IS_EV) != 0) {
      DCHECK(!cert->subject().organization_names.empty());
      identity_title =
          l10n_util::GetStringF(IDS_PAGE_INFO_EV_IDENTITY_TITLE,
              UTF8ToWide(cert->subject().organization_names[0]),
              UTF8ToWide(navigation_entry->url().host()));
      // An EV Cert is required to have a city (localityName) and country but
      // state is "if any".
      DCHECK(!cert->subject().locality_name.empty());
      DCHECK(!cert->subject().country_name.empty());
      std::wstring locality;
      if (!cert->subject().state_or_province_name.empty()) {
        locality = l10n_util::GetStringF(
            IDS_PAGEINFO_ADDRESS,
            UTF8ToWide(cert->subject().locality_name),
            UTF8ToWide(cert->subject().state_or_province_name),
            UTF8ToWide(cert->subject().country_name));
      } else {
        locality = l10n_util::GetStringF(
            IDS_PAGEINFO_PARTIAL_ADDRESS,
            UTF8ToWide(cert->subject().locality_name),
            UTF8ToWide(cert->subject().country_name));
      }
      DCHECK(!cert->subject().organization_names.empty());
      identity_msg.assign(l10n_util::GetStringF(
          IDS_PAGE_INFO_SECURITY_TAB_SECURE_IDENTITY_EV,
          UTF8ToWide(cert->subject().organization_names[0]),
          locality,
          UTF8ToWide(GetIssuerName(cert->issuer()))));
    } else {
      // Non EV OK HTTPS.
      if (empty_subject_name)
        identity_title.clear();  // Don't display any title.
      else
        identity_title.assign(subject_name);
      std::wstring issuer_name(UTF8ToWide(GetIssuerName(cert->issuer())));
      if (issuer_name.empty()) {
        issuer_name.assign(
            l10n_util::GetString(IDS_PAGE_INFO_SECURITY_TAB_UNKNOWN_PARTY));
      } else {
        identity_msg.assign(
            l10n_util::GetStringF(IDS_PAGE_INFO_SECURITY_TAB_SECURE_IDENTITY,
                                  issuer_name));
      }
    }
  } else {
    // Bad HTTPS.
    identity_msg.assign(
        l10n_util::GetString(IDS_PAGE_INFO_SECURITY_TAB_INSECURE_IDENTITY));
    identity_ok = false;
  }

  // Connection section.
  // We consider anything less than 80 bits encryption to be weak encryption.
  // TODO(wtc): Bug 1198735: report mixed/unsafe content for unencrypted and
  // weakly encrypted connections.
  if (ssl.security_bits() <= 0) {
    connection_ok = false;
    connection_msg.assign(
        l10n_util::GetStringF(
            IDS_PAGE_INFO_SECURITY_TAB_NOT_ENCRYPTED_CONNECTION_TEXT,
            subject_name));
  } else if (ssl.security_bits() < 80) {
    connection_ok = false;
    connection_msg.assign(
        l10n_util::GetStringF(
            IDS_PAGE_INFO_SECURITY_TAB_WEAK_ENCRYPTION_CONNECTION_TEXT,
            subject_name));
  } else {
    connection_msg.assign(
        l10n_util::GetStringF(
            IDS_PAGE_INFO_SECURITY_TAB_ENCRYPTED_CONNECTION_TEXT,
            subject_name,
            IntToWString(ssl.security_bits())));
    if (ssl.has_mixed_content()) {
      connection_ok = false;
      connection_msg.assign(
          l10n_util::GetStringF(
              IDS_PAGE_INFO_SECURITY_TAB_ENCRYPTED_SENTENCE_LINK,
              connection_msg,
              l10n_util::GetString(
                  IDS_PAGE_INFO_SECURITY_TAB_ENCRYPTED_MIXED_CONTENT_WARNING)));
    } else if (ssl.has_unsafe_content()) {
      connection_ok = false;
      connection_msg.assign(
          l10n_util::GetStringF(
              IDS_PAGE_INFO_SECURITY_TAB_ENCRYPTED_SENTENCE_LINK,
              connection_msg,
              l10n_util::GetString(
                  IDS_PAGE_INFO_SECURITY_TAB_ENCRYPTED_BAD_HTTPS_WARNING)));
    }
  }

  // Let's add the different sections.
  AddSection(l10n_util::GetString(IDS_PAGE_INFO_SECURITY_TAB_IDENTITY_TITLE),
             identity_ok, identity_title, identity_msg);
  AddSection(l10n_util::GetString(IDS_PAGE_INFO_SECURITY_TAB_CONNECTION_TITLE),
             connection_ok, std::wstring(), connection_msg);

  // Request the number of visits.
  HistoryService* history = profile->GetHistoryService(
      Profile::EXPLICIT_ACCESS);
  if (history) {
    history->GetVisitCountToHost(
        navigation_entry->url(),
        &request_consumer_,
        NewCallback(this, &SecurityTabView::OnGotVisitCountToHost));
  }
}

SecurityTabView::~SecurityTabView() {
}

void SecurityTabView::AddSection(const std::wstring& title,
                                     bool state,
                                     const std::wstring& head_line,
                                     const std::wstring& description) {
  Section* section = new Section(title, state, head_line, description);
  sections_.push_back(section);
  AddChildView(section);
}

void SecurityTabView::Layout() {
  int width = GetWidth() - 2 * kHGapToBorder;
  int x = kHGapToBorder;
  int y = kVGapToBorder;
  for (std::vector<Section*>::const_iterator iter = sections_.begin();
       iter != sections_.end(); ++iter) {
    Section* section = *iter;
    int h = section->GetHeightForWidth(width);
    section->SetBounds(x, y, width, h);
    section->Layout();
    y += h + kVGapBetweenSections;
  }
}

// static
std::string SecurityTabView::GetIssuerName(
    const net::X509Certificate::Principal& issuer) {
  if (!issuer.common_name.empty())
    return issuer.common_name;
  if (!issuer.organization_names.empty())
    return issuer.organization_names[0];
  if (!issuer.organization_unit_names.empty())
    return issuer.organization_unit_names[0];

  return std::string();
}

void SecurityTabView::OnGotVisitCountToHost(HistoryService::Handle handle,
                                            bool found_visits,
                                            int count,
                                            Time first_visit) {
  if (!found_visits) {
    // This indicates an error, such as the page wasn't http/https; do nothing.
    return;
  }

  bool visited_before_today = false;
  if (count) {
    Time today = Time::Now().LocalMidnight();
    Time first_visit_midnight = first_visit.LocalMidnight();
    visited_before_today = (first_visit_midnight < today);
  }

  if (!visited_before_today) {
    AddSection(
        l10n_util::GetString(IDS_PAGE_INFO_SECURITY_TAB_PERSONAL_HISTORY_TITLE),
        false, std::wstring(),
        l10n_util::GetString(IDS_PAGE_INFO_SECURITY_TAB_FIRST_VISITED_TODAY));
  } else {
    AddSection(
        l10n_util::GetString(IDS_PAGE_INFO_SECURITY_TAB_PERSONAL_HISTORY_TITLE),
        true, std::wstring(),
        l10n_util::GetStringF(IDS_PAGE_INFO_SECURITY_TAB_VISITED_BEFORE_TODAY,
                              base::TimeFormatShortDate(first_visit)));
  }
  Layout();
  SchedulePaint();
}

////////////////////////////////////////////////////////////////////////////////
// PageInfoContentView
class PageInfoContentView : public ChromeViews::View {
 public:
  PageInfoContentView() : cert_viewer_button_(NULL) {}

  void set_cert_viewer_button(ChromeViews::NativeButton* cert_viewer_button) {
    cert_viewer_button_ = cert_viewer_button;
  }

  // ChromeViews::View overrides:
  virtual void GetPreferredSize(CSize *out) {
    DCHECK(out);
    *out = ChromeViews::Window::GetLocalizedContentsSize(
        IDS_PAGEINFO_DIALOG_WIDTH_CHARS,
        IDS_PAGEINFO_DIALOG_HEIGHT_LINES).ToSIZE();
  }

  virtual void Layout() {
    if (cert_viewer_button_) {
      CSize ps;
      cert_viewer_button_->GetPreferredSize(&ps);

      CRect parent_bounds;
      GetParent()->GetLocalBounds(&parent_bounds, false);
      int y_buttons = parent_bounds.bottom - ps.cy - kButtonVEdgeMargin;
      cert_viewer_button_->SetBounds(kPanelHorizMargin, y_buttons, ps.cx,
                                     ps.cy);
    }
    View::Layout();
  }

 private:
  ChromeViews::NativeButton* cert_viewer_button_;

  DISALLOW_EVIL_CONSTRUCTORS(PageInfoContentView);
};

////////////////////////////////////////////////////////////////////////////////
// PageInfoWindow

int PageInfoWindow::opened_window_count_ = 0;

// static
void PageInfoWindow::Create(Profile* profile,
                            NavigationEntry* nav_entry,
                            HWND parent_hwnd,
                            PageInfoWindow::TabID tab) {
  PageInfoWindow* window = new PageInfoWindow();
  window->Init(profile, nav_entry, parent_hwnd);
  window->Show();
}

// static
void PageInfoWindow::RegisterPrefs(PrefService* prefs) {
  prefs->RegisterDictionaryPref(prefs::kPageInfoWindowPlacement);
}

PageInfoWindow::PageInfoWindow() : cert_id_(0), contents_(NULL) {
}

PageInfoWindow::~PageInfoWindow() {
  DCHECK(opened_window_count_ > 0);
  opened_window_count_--;
}

void PageInfoWindow::Init(Profile* profile,
                          NavigationEntry* navigation_entry,
                          HWND parent) {
  cert_id_ = navigation_entry->ssl().cert_id();

  cert_info_button_ = new ChromeViews::NativeButton(
      l10n_util::GetString(IDS_PAGEINFO_CERT_INFO_BUTTON));
  cert_info_button_->SetListener(this);

  contents_ = new PageInfoContentView();
  DWORD sys_color = ::GetSysColor(COLOR_3DFACE);
  SkColor color = SkColorSetRGB(GetRValue(sys_color), GetGValue(sys_color),
                                GetBValue(sys_color));
  contents_->SetBackground(
      ChromeViews::Background::CreateSolidBackground(color));

  ChromeViews::GridLayout* layout = new ChromeViews::GridLayout(contents_);
  contents_->SetLayoutManager(layout);
  ChromeViews::ColumnSet* columns = layout->AddColumnSet(0);
  columns->AddPaddingColumn(0, kHorizontalPadding);
  columns->AddColumn(ChromeViews::GridLayout::FILL,  // Horizontal resize.
                     ChromeViews::GridLayout::FILL,  // Vertical resize.
                     1,  // Resize weight.
                     ChromeViews::GridLayout::USE_PREF,  // Size type.
                     0,  // Ignored for USE_PREF.
                     0);  // Minimum size.
  columns->AddColumn(ChromeViews::GridLayout::FILL,  // Horizontal resize.
                     ChromeViews::GridLayout::FILL,  // Vertical resize.
                     1,  // Resize weight.
                     ChromeViews::GridLayout::USE_PREF,  // Size type.
                     0,  // Ignored for USE_PREF.
                     0);  // Minimum size.
  columns->AddPaddingColumn(0, kHorizontalPadding);

  layout->AddPaddingRow(0, kHorizontalPadding);
  layout->StartRow(1, 0);
  layout->AddView(CreateSecurityTabView(profile, navigation_entry), 2, 1);

  layout->AddPaddingRow(0, kHorizontalPadding);

  if (opened_window_count_ > 0) {
    // There already is a PageInfo window opened.  Let's shift the location of
    // the new PageInfo window so they don't overlap entirely.
    // Window::Init will position the window from the stored location.
    CRect bounds;
    bool maximized, always_on_top;
    if (RestoreWindowPosition(&bounds, &maximized, &always_on_top)) {
      CalculateWindowBounds(&bounds);
      SaveWindowPosition(bounds, maximized, always_on_top);
    }
  }

  ChromeViews::Window::CreateChromeWindow(parent, gfx::Rect(), this);
  // TODO(beng): (Cleanup) - cert viewer button should use GetExtraView.
  
  if (cert_id_) {
    scoped_refptr<net::X509Certificate> cert;
    CertStore::GetSharedInstance()->RetrieveCert(cert_id_, &cert);
    // When running with Gears, we have no os certificate, so there is no cert
    // to show. Don't bother showing the cert info button in that case.
    if (cert.get() && cert->os_cert_handle()) {
      contents_->GetParent()->AddChildView(cert_info_button_);
      contents_->set_cert_viewer_button(cert_info_button_);
      contents_->Layout();
    }
  }
}

ChromeViews::View* PageInfoWindow::CreateGeneralTabView() {
  return new ChromeViews::View();
}

ChromeViews::View* PageInfoWindow::CreateSecurityTabView(
    Profile* profile,
    NavigationEntry* navigation_entry) {
  return new SecurityTabView(profile, navigation_entry);
}

void PageInfoWindow::Show() {
  window()->Show();
  opened_window_count_++;
}

int PageInfoWindow::GetDialogButtons() const {
  return DIALOGBUTTON_CANCEL;
}

std::wstring PageInfoWindow::GetWindowTitle() const {
  return l10n_util::GetString(IDS_PAGEINFO_WINDOW_TITLE);
}

void PageInfoWindow::SaveWindowPosition(const CRect& bounds,
                                        bool maximized,
                                        bool always_on_top) {
  window()->SaveWindowPositionToPrefService(g_browser_process->local_state(),
                                            prefs::kPageInfoWindowPlacement,
                                            bounds, maximized, always_on_top);
}

bool PageInfoWindow::RestoreWindowPosition(CRect* bounds,
                                           bool* maximized,
                                           bool* always_on_top) {
  return window()->RestoreWindowPositionFromPrefService(
      g_browser_process->local_state(),
      prefs::kPageInfoWindowPlacement,
      bounds, maximized, always_on_top);
}

ChromeViews::View* PageInfoWindow::GetContentsView() {
  return contents_;
}

void PageInfoWindow::ButtonPressed(ChromeViews::NativeButton* sender) {
  if (sender == cert_info_button_) {
    DCHECK(cert_id_ != 0);
    ShowCertDialog(cert_id_);
  } else {
    NOTREACHED();
  }
}

void PageInfoWindow::CalculateWindowBounds(CRect* bounds) {
  const int kDefaultOffset = 15;

  gfx::Rect window_bounds(*bounds);
  gfx::Rect monitor_bounds(win_util::GetMonitorBoundsForRect(window_bounds));

  // If necessary, move the window so it is visible on the screen.
  gfx::Rect adjusted_bounds = window_bounds.AdjustToFit(monitor_bounds);
  if (adjusted_bounds != window_bounds) {
    // The bounds have moved, we are done.
    RECT rect = adjusted_bounds.ToRECT();
    bounds->CopyRect(&rect);
    return;
  }

  // Move the window from its specified position, trying to keep it entirely
  // visible.
  int x_offset, y_offset;
  if (window_bounds.right() + kDefaultOffset >= monitor_bounds.right() &&
      abs(monitor_bounds.x() - window_bounds.x()) >= kDefaultOffset) {
    x_offset = -kDefaultOffset;
  } else {
    x_offset = kDefaultOffset;
  }

  if (window_bounds.bottom() + kDefaultOffset >= monitor_bounds.bottom() &&
      abs(monitor_bounds.y() - window_bounds.y()) >= kDefaultOffset) {
    y_offset = -kDefaultOffset;
  } else {
    y_offset = kDefaultOffset;
  }

  bounds->OffsetRect(x_offset, y_offset);
}

void PageInfoWindow::ShowCertDialog(int cert_id) {
  scoped_refptr<net::X509Certificate> cert;
  CertStore::GetSharedInstance()->RetrieveCert(cert_id, &cert);
  if (!cert.get()) {
    // The certificate was not found. Could be that the renderer crashed before
    // we displayed the page info.
    return; 		
  } 		

  CRYPTUI_VIEWCERTIFICATE_STRUCT view_info = { 0 }; 		
  view_info.dwSize = sizeof(view_info); 		
  // We set our parent to the tab window. This makes the cert dialog created 		
  // in CryptUIDlgViewCertificate modal to the browser. 		
  view_info.hwndParent = window()->owning_window();
  view_info.dwFlags = CRYPTUI_DISABLE_EDITPROPERTIES |
                      CRYPTUI_DISABLE_ADDTOSTORE;
  view_info.pCertContext = cert->os_cert_handle();
  // Search the cert store that 'cert' is in when building the cert chain.
  HCERTSTORE cert_store = view_info.pCertContext->hCertStore;
  view_info.cStores = 1;
  view_info.rghStores = &cert_store;
  BOOL properties_changed;

  // This next call blocks but keeps processing windows messages, making it
  // modal to the browser window.
  BOOL rv = ::CryptUIDlgViewCertificate(&view_info, &properties_changed);
}

