// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/dom_ui/dom_ui_theme_source.h"

#include "app/l10n_util.h"
#include "app/resource_bundle.h"
#include "app/theme_provider.h"
#include "base/gfx/png_encoder.h"
#include "base/message_loop.h"
#include "base/string_util.h"
#include "base/time.h"
#include "chrome/browser/browser_theme_provider.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/theme_resources_util.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "googleurl/src/gurl.h"
#include "grit/browser_resources.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"

#if defined(OS_WIN)
#include "chrome/browser/views/bookmark_bar_view.h"
#endif

// Path for the New Tab CSS. When we get more than a few of these, we should
// use a resource map rather than hard-coded strings.
static const char* kNewTabCSSPath = "css/newtab.css";
static const char* kNewIncognitoTabCSSPath = "css/newincognitotab.css";

static string16 SkColorToRGBAString(SkColor color) {
  return WideToUTF16(l10n_util::GetStringF(IDS_RGBA_CSS_FORMAT_STRING,
      IntToWString(SkColorGetR(color)),
      IntToWString(SkColorGetG(color)),
      IntToWString(SkColorGetB(color)),
      DoubleToWString(SkColorGetA(color) / 255.0)));
}

static std::string StripQueryParams(const std::string& path) {
  GURL path_url = GURL(std::string(chrome::kChromeUIScheme) + "://" +
                       std::string(chrome::kChromeUIThemePath) + "/" + path);
  return path_url.path().substr(1);  // path() always includes a leading '/'.
}

////////////////////////////////////////////////////////////////////////////////
// DOMUIThemeSource, public:

DOMUIThemeSource::DOMUIThemeSource(Profile* profile)
    : DataSource(chrome::kChromeUIThemePath, MessageLoop::current()),
      profile_(profile) {
  InitNewTabCSS();
  InitNewIncognitoTabCSS();
}

void DOMUIThemeSource::StartDataRequest(const std::string& path,
                                        int request_id) {
  // Our path may include cachebuster arguments, so trim them off.
  std::string uncached_path = StripQueryParams(path);

  if (uncached_path == kNewTabCSSPath) {
    SendNewTabCSS(request_id, new_tab_css_);
    return;
  } else if (uncached_path == kNewIncognitoTabCSSPath) {
    SendNewTabCSS(request_id, new_incognito_tab_css_);
    return;
  } else {
    int resource_id = ThemeResourcesUtil::GetId(uncached_path);
    if (resource_id != -1) {
      SendThemeBitmap(request_id, resource_id);
      return;
    }
  }
  // We don't have any data to send back.
  SendResponse(request_id, NULL);
}

std::string DOMUIThemeSource::GetMimeType(const std::string& path) const {
  std::string uncached_path = StripQueryParams(path);

  if (uncached_path == kNewTabCSSPath ||
      uncached_path == kNewIncognitoTabCSSPath) {
    return "text/css";
  }

  return "image/png";
}

void DOMUIThemeSource::SendResponse(int request_id, RefCountedBytes* data) {
  ChromeURLDataManager::DataSource::SendResponse(request_id, data);
}

MessageLoop* DOMUIThemeSource::MessageLoopForRequestPath(
    const std::string& path) const {
  std::string uncached_path = StripQueryParams(path);

  if (uncached_path == kNewTabCSSPath ||
      uncached_path == kNewIncognitoTabCSSPath) {
    // All of the operations that need to be on the UI thread for these
    // requests are performed in InitNewTabCSS and InitNewIncognitoTabCSS,
    // called by the constructor.  It is safe to call StartDataRequest for
    // these resources from any thread, so return NULL.
    return NULL;
  }

  // Superclass
  return DataSource::MessageLoopForRequestPath(path);
}

////////////////////////////////////////////////////////////////////////////////
// DOMUIThemeSource, private:

void DOMUIThemeSource::InitNewTabCSS() {
  ThemeProvider* tp = profile_->GetThemeProvider();
  DCHECK(tp);

  // Get our theme colors
  SkColor color_background =
      tp->GetColor(BrowserThemeProvider::COLOR_NTP_BACKGROUND);
  SkColor color_text = tp->GetColor(BrowserThemeProvider::COLOR_NTP_TEXT);
  SkColor color_link = tp->GetColor(BrowserThemeProvider::COLOR_NTP_LINK);
  SkColor color_link_underline =
      tp->GetColor(BrowserThemeProvider::COLOR_NTP_LINK_UNDERLINE);

  SkColor color_section =
      tp->GetColor(BrowserThemeProvider::COLOR_NTP_SECTION);
  SkColor color_section_text =
      tp->GetColor(BrowserThemeProvider::COLOR_NTP_SECTION_TEXT);
  SkColor color_section_link =
      tp->GetColor(BrowserThemeProvider::COLOR_NTP_SECTION_LINK);
  SkColor color_section_link_underline =
      tp->GetColor(BrowserThemeProvider::COLOR_NTP_SECTION_LINK_UNDERLINE);

  SkColor color_header =
      tp->GetColor(BrowserThemeProvider::COLOR_NTP_HEADER);
  // Generate a lighter color for the header gradients.
  skia::HSL header_lighter;
  skia::SkColorToHSL(color_header, header_lighter);
  header_lighter.l += (1 - header_lighter.l) * 0.33;
  SkColor color_header_gradient_light =
      skia::HSLToSkColor(SkColorGetA(color_header), header_lighter);

  // Generate section border color from the header color. See
  // BookmarkBarView::Paint for how we do this for the bookmark bar
  // borders.
  SkColor color_section_border =
      SkColorSetARGB(80,
                     SkColorGetR(color_header),
                     SkColorGetG(color_header),
                     SkColorGetB(color_header));

  // Generate the replacements.
  std::vector<string16> subst;
  // A second list of replacements, each of which must be in $$x format,
  // where x is a digit from 1-9.
  std::vector<string16> subst2;

  // Cache-buster for background.
  subst.push_back(WideToUTF16(
      profile_->GetPrefs()->GetString(prefs::kCurrentThemeID)));  // $1

  // Colors.
  subst.push_back(SkColorToRGBAString(color_background));  // $2
  subst.push_back(UTF8ToUTF16(GetNewTabBackgroundCSS(false)));  // $3
  subst.push_back(UTF8ToUTF16(GetNewTabBackgroundCSS(true)));  // $4
  subst.push_back(UTF8ToUTF16(GetNewTabBackgroundTilingCSS()));  // $5
  subst.push_back(SkColorToRGBAString(color_header));  // $6
  subst.push_back(SkColorToRGBAString(color_header_gradient_light));  // $7
  subst.push_back(SkColorToRGBAString(color_text));  // $8
  subst.push_back(SkColorToRGBAString(color_link));  // $9

  subst2.push_back(SkColorToRGBAString(color_section));  // $$1
  subst2.push_back(SkColorToRGBAString(color_section_border));  // $$2
  subst2.push_back(SkColorToRGBAString(color_section_text));  // $$3
  subst2.push_back(SkColorToRGBAString(color_section_link));  // $$4
  subst2.push_back(
      UTF8ToUTF16(tp->HasCustomImage(IDR_THEME_NTP_ATTRIBUTION) ?
          "block" : "none"));  // $$5
  subst2.push_back(SkColorToRGBAString(color_link_underline));  // $$6
  subst2.push_back(SkColorToRGBAString(color_section_link_underline));  // $$7

  // Get our template.
  static const base::StringPiece new_tab_theme_css(
      ResourceBundle::GetSharedInstance().GetRawDataResource(
      IDR_NEW_TAB_THEME_CSS));

  // Create the string from our template and the replacements.
  string16 format_string = ASCIIToUTF16(new_tab_theme_css.as_string());
  const std::string css_string = UTF16ToASCII(ReplaceStringPlaceholders(
      format_string, subst, NULL));
  new_tab_css_ = UTF16ToASCII(ReplaceStringPlaceholders(
      ASCIIToUTF16(css_string), subst2, NULL));
}

void DOMUIThemeSource::InitNewIncognitoTabCSS() {
  ThemeProvider* tp = profile_->GetThemeProvider();
  DCHECK(tp);

  // Get our theme colors
  SkColor color_background =
      tp->GetColor(BrowserThemeProvider::COLOR_NTP_BACKGROUND);

  // Generate the replacements.
  std::vector<string16> subst;

  // Cache-buster for background.
  subst.push_back(WideToUTF16(
      profile_->GetPrefs()->GetString(prefs::kCurrentThemeID)));  // $1

  // Colors.
  subst.push_back(SkColorToRGBAString(color_background));  // $2
  subst.push_back(UTF8ToUTF16(GetNewTabBackgroundCSS(false)));  // $3
  subst.push_back(UTF8ToUTF16(GetNewTabBackgroundCSS(true)));  // $4
  subst.push_back(UTF8ToUTF16(GetNewTabBackgroundTilingCSS()));  // $5

  // Get our template.
  static const base::StringPiece new_tab_theme_css(
      ResourceBundle::GetSharedInstance().GetRawDataResource(
      IDR_NEW_INCOGNITO_TAB_THEME_CSS));

  // Create the string from our template and the replacements.
  string16 format_string = ASCIIToUTF16(new_tab_theme_css.as_string());
  new_incognito_tab_css_ = UTF16ToASCII(ReplaceStringPlaceholders(
      format_string, subst, NULL));
}

void DOMUIThemeSource::SendNewTabCSS(int request_id,
                                     const std::string& css_string) {
  // Convert to a format appropriate for sending.
  scoped_refptr<RefCountedBytes> css_bytes(new RefCountedBytes);
  css_bytes->data.resize(css_string.size());
  std::copy(css_string.begin(), css_string.end(), css_bytes->data.begin());

  // Send.
  SendResponse(request_id, css_bytes);
}

void DOMUIThemeSource::SendThemeBitmap(int request_id, int resource_id) {
  ThemeProvider* tp = profile_->GetThemeProvider();
  DCHECK(tp);

  std::vector<unsigned char> png_bytes;
  if (tp->GetRawData(resource_id, &png_bytes)) {
    scoped_refptr<RefCountedBytes> image_data =
        new RefCountedBytes(png_bytes);
    SendResponse(request_id, image_data);
  } else {
    SendResponse(request_id, NULL);
  }
}

std::string DOMUIThemeSource::GetNewTabBackgroundCSS(bool bar_attached) {
  int alignment;
  profile_->GetThemeProvider()->GetDisplayProperty(
      BrowserThemeProvider::NTP_BACKGROUND_ALIGNMENT, &alignment);

  // TODO(glen): This is a quick workaround to hide the notused.png image when
  // no image is provided - we don't have time right now to figure out why
  // this is painting as white.
  // http://crbug.com/17593
  if (!profile_->GetThemeProvider()->HasCustomImage(IDR_THEME_NTP_BACKGROUND)) {
    return "-64px";
  }

  if (bar_attached)
    return BrowserThemeProvider::AlignmentToString(alignment);

  // The bar is detached, so we must offset the background by the bar size
  // if it's a top-aligned bar.
#if defined(OS_WIN)
  int offset = BookmarkBarView::kNewtabBarHeight;
#else
  int offset = 0;
#endif

  if (alignment & BrowserThemeProvider::ALIGN_TOP) {
    if (alignment & BrowserThemeProvider::ALIGN_LEFT)
      return "0% " + IntToString(-offset) + "px";
    else if (alignment & BrowserThemeProvider::ALIGN_RIGHT)
      return "100% " + IntToString(-offset) + "px";
    return "center " + IntToString(-offset) + "px";
  }
  return BrowserThemeProvider::AlignmentToString(alignment);
}

std::string DOMUIThemeSource::GetNewTabBackgroundTilingCSS() {
  int repeat_mode;
  profile_->GetThemeProvider()->GetDisplayProperty(
      BrowserThemeProvider::NTP_BACKGROUND_TILING, &repeat_mode);
  return BrowserThemeProvider::TilingToString(repeat_mode);
}

