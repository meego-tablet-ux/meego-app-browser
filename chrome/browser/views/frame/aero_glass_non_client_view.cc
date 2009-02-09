// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/views/frame/aero_glass_non_client_view.h"

#include "chrome/app/theme/theme_resources.h"
#include "chrome/browser/views/frame/browser_view.h"
#include "chrome/browser/views/tabs/tab_strip.h"
#include "chrome/common/resource_bundle.h"
#include "chrome/views/client_view.h"
#include "chrome/views/window_resources.h"

// An enumeration of bitmap resources used by this window.
enum {
  FRAME_PART_BITMAP_FIRST = 0,  // must be first.

  // Client Edge Border.
  FRAME_CLIENT_EDGE_TOP_LEFT,
  FRAME_CLIENT_EDGE_TOP,
  FRAME_CLIENT_EDGE_TOP_RIGHT,
  FRAME_CLIENT_EDGE_RIGHT,
  FRAME_CLIENT_EDGE_BOTTOM_RIGHT,
  FRAME_CLIENT_EDGE_BOTTOM,
  FRAME_CLIENT_EDGE_BOTTOM_LEFT,
  FRAME_CLIENT_EDGE_LEFT,

  FRAME_PART_BITMAP_COUNT  // Must be last.
};

class AeroGlassWindowResources {
 public:
  AeroGlassWindowResources() { InitClass(); }
  virtual ~AeroGlassWindowResources() { }

  virtual SkBitmap* GetPartBitmap(views::FramePartBitmap part) const {
    return standard_frame_bitmaps_[part];
  }

  SkBitmap app_top_left() const { return app_top_left_; }
  SkBitmap app_top_center() const { return app_top_center_; }
  SkBitmap app_top_right() const { return app_top_right_; }

 private:
  static void InitClass() {
    static bool initialized = false;
    if (!initialized) {
      static const int kFramePartBitmapIds[] = {
        0,
        IDR_CONTENT_TOP_LEFT_CORNER, IDR_CONTENT_TOP_CENTER,
            IDR_CONTENT_TOP_RIGHT_CORNER, IDR_CONTENT_RIGHT_SIDE,
            IDR_CONTENT_BOTTOM_RIGHT_CORNER, IDR_CONTENT_BOTTOM_CENTER,
            IDR_CONTENT_BOTTOM_LEFT_CORNER, IDR_CONTENT_LEFT_SIDE,
        0
      };

      ResourceBundle& rb = ResourceBundle::GetSharedInstance();
      for (int i = 0; i < FRAME_PART_BITMAP_COUNT; ++i) {
        int id = kFramePartBitmapIds[i];
        if (id != 0)
          standard_frame_bitmaps_[i] = rb.GetBitmapNamed(id);
      }
      app_top_left_ = *rb.GetBitmapNamed(IDR_APP_TOP_LEFT);
      app_top_center_ = *rb.GetBitmapNamed(IDR_APP_TOP_CENTER);
      app_top_right_ = *rb.GetBitmapNamed(IDR_APP_TOP_RIGHT);      

      initialized = true;
    }
  }

  static SkBitmap* standard_frame_bitmaps_[FRAME_PART_BITMAP_COUNT];
  static SkBitmap app_top_left_;
  static SkBitmap app_top_center_;
  static SkBitmap app_top_right_;

  DISALLOW_EVIL_CONSTRUCTORS(AeroGlassWindowResources);
};

// static
SkBitmap* AeroGlassWindowResources::standard_frame_bitmaps_[];
SkBitmap AeroGlassWindowResources::app_top_left_;
SkBitmap AeroGlassWindowResources::app_top_center_;
SkBitmap AeroGlassWindowResources::app_top_right_;

AeroGlassWindowResources* AeroGlassNonClientView::resources_ = NULL;
SkBitmap AeroGlassNonClientView::distributor_logo_;

namespace {
// There are 3 px of client edge drawn inside the outer frame borders.
const int kNonClientBorderThickness = 3;
// Besides the frame border, there's another 11 px of empty space atop the
// window in restored mode, to use to drag the window around.
const int kNonClientRestoredExtraThickness = 11;
// In the window corners, the resize areas don't actually expand bigger, but the
// 16 px at the end of the top and bottom edges triggers diagonal resizing.
const int kResizeAreaCornerSize = 16;
// The distributor logo is drawn 3 px from the top of the window.
static const int kLogoTopSpacing = 3;
// In maximized mode, the OTR avatar starts 2 px below the top of the screen, so
// that it doesn't extend into the "3D edge" portion of the titlebar.
const int kOTRMaximizedTopSpacing = 2;
// The OTR avatar ends 2 px above the bottom of the tabstrip (which, given the
// way the tabstrip draws its bottom edge, will appear like a 1 px gap to the
// user).
const int kOTRBottomSpacing = 2;
// There are 2 px on each side of the OTR avatar (between the frame border and
// it on the left, and between it and the tabstrip on the right).
const int kOTRSideSpacing = 2;
// In restored mode, the New Tab button isn't at the same height as the caption
// buttons, but the space will look cluttered if it actually slides under them,
// so we stop it when the gap between the two is down to 5 px.
const int kNewTabCaptionRestoredSpacing = 5;
// In maximized mode, where the New Tab button and the caption buttons are at
// similar vertical coordinates, we need to reserve a larger, 16 px gap to avoid
// looking too cluttered.
const int kNewTabCaptionMaximizedSpacing = 16;
// When there's a distributor logo, we leave a 7 px gap between it and the
// caption buttons.
const int kLogoCaptionSpacing = 7;
}

///////////////////////////////////////////////////////////////////////////////
// AeroGlassNonClientView, public:

AeroGlassNonClientView::AeroGlassNonClientView(AeroGlassFrame* frame,
                                               BrowserView* browser_view)
    : frame_(frame),
      browser_view_(browser_view) {
  InitClass();
}

AeroGlassNonClientView::~AeroGlassNonClientView() {
}

gfx::Rect AeroGlassNonClientView::GetBoundsForTabStrip(TabStrip* tabstrip) {
  int tabstrip_x = browser_view_->ShouldShowOffTheRecordAvatar() ?
      (otr_avatar_bounds_.right() + kOTRSideSpacing) :
      NonClientBorderThickness();
  int tabstrip_width = frame_->GetMinimizeButtonOffset() - tabstrip_x -
      (frame_->IsMaximized() ?
      kNewTabCaptionMaximizedSpacing : kNewTabCaptionRestoredSpacing);
  return gfx::Rect(tabstrip_x, NonClientTopBorderHeight(),
                   std::max(0, tabstrip_width), tabstrip->GetPreferredHeight());
}

///////////////////////////////////////////////////////////////////////////////
// AeroGlassNonClientView, views::NonClientView implementation:

gfx::Rect AeroGlassNonClientView::CalculateClientAreaBounds(int width,
                                                            int height) const {
  if (!browser_view_->IsTabStripVisible())
    return gfx::Rect(0, 0, this->width(), this->height());

  int top_height = NonClientTopBorderHeight();
  int border_thickness = NonClientBorderThickness();
  return gfx::Rect(border_thickness, top_height,
                   std::max(0, width - (2 * border_thickness)),
                   std::max(0, height - top_height - border_thickness));
}

CPoint AeroGlassNonClientView::GetSystemMenuPoint() const {
  CPoint offset;
  MapWindowPoints(GetWidget()->GetHWND(), HWND_DESKTOP, &offset, 1);
  return offset;
}

int AeroGlassNonClientView::NonClientHitTest(const gfx::Point& point) {
  // If we don't have a tabstrip, we haven't customized the frame, so Windows
  // can figure this out.  If the point isn't within our bounds, then it's in
  // the native portion of the frame, so again Windows can figure it out.
  if (!browser_view_->IsTabStripVisible() || !bounds().Contains(point))
    return HTNOWHERE;

  int frame_component = frame_->client_view()->NonClientHitTest(point);
  if (frame_component != HTNOWHERE)
    return frame_component;

  int border_thickness = FrameBorderThickness();
  int window_component = GetHTComponentForFrame(point, border_thickness,
      NonClientBorderThickness(), border_thickness,
      kResizeAreaCornerSize - border_thickness,
      frame_->window_delegate()->CanResize());
  // Fall back to the caption if no other component matches.
  return (window_component == HTNOWHERE) ? HTCAPTION : window_component;
}

///////////////////////////////////////////////////////////////////////////////
// AeroGlassNonClientView, views::View overrides:

void AeroGlassNonClientView::Paint(ChromeCanvas* canvas) {
  PaintDistributorLogo(canvas);
  if (browser_view_->IsTabStripVisible())
    PaintToolbarBackground(canvas);
  PaintOTRAvatar(canvas);
  if (browser_view_->IsTabStripVisible())
    PaintClientEdge(canvas);
}

void AeroGlassNonClientView::Layout() {
  LayoutDistributorLogo();
  LayoutOTRAvatar();
  LayoutClientView();
}

void AeroGlassNonClientView::ViewHierarchyChanged(bool is_add,
                                                  views::View* parent,
                                                  views::View* child) {
  if (is_add && child == this) {
    DCHECK(GetWidget());
    DCHECK(frame_->client_view()->GetParent() != this);
    AddChildView(frame_->client_view());
  }
}

///////////////////////////////////////////////////////////////////////////////
// AeroGlassNonClientView, private:

int AeroGlassNonClientView::FrameBorderThickness() const {
  return GetSystemMetrics(SM_CXSIZEFRAME);
}

int AeroGlassNonClientView::NonClientBorderThickness() const {
  return kNonClientBorderThickness;
}

int AeroGlassNonClientView::NonClientTopBorderHeight() const {
  return FrameBorderThickness() +
      (frame_->IsMaximized() ? 0 : kNonClientRestoredExtraThickness);
}

void AeroGlassNonClientView::PaintDistributorLogo(ChromeCanvas* canvas) {
  // The distributor logo is only painted when the frame is not maximized and
  // when we actually have a logo.
  if (!frame_->IsMaximized() && !distributor_logo_.empty()) {
    // NOTE: We don't mirror the logo placement here because the outer frame
    // itself isn't mirrored in RTL.  This is a bug; if it is fixed, this should
    // be mirrored as in opaque_non_client_view.cc.
    canvas->DrawBitmapInt(distributor_logo_, logo_bounds_.x(),
                          logo_bounds_.y());
  }
}

void AeroGlassNonClientView::PaintToolbarBackground(ChromeCanvas* canvas) {
  gfx::Rect toolbar_bounds(browser_view_->GetToolbarBounds());
  gfx::Point toolbar_origin(toolbar_bounds.origin());
  View::ConvertPointToView(frame_->client_view(), this, &toolbar_origin);
  toolbar_bounds.set_origin(toolbar_origin);

  SkBitmap* toolbar_left =
      resources_->GetPartBitmap(FRAME_CLIENT_EDGE_TOP_LEFT);
  canvas->DrawBitmapInt(*toolbar_left,
                        toolbar_bounds.x() - toolbar_left->width(),
                        toolbar_bounds.y());

  SkBitmap* toolbar_center =
      resources_->GetPartBitmap(FRAME_CLIENT_EDGE_TOP);
  canvas->TileImageInt(*toolbar_center, toolbar_bounds.x(), toolbar_bounds.y(),
                       toolbar_bounds.width(), toolbar_center->height());

  canvas->DrawBitmapInt(*resources_->GetPartBitmap(FRAME_CLIENT_EDGE_TOP_RIGHT),
      toolbar_bounds.right(), toolbar_bounds.y());
}

void AeroGlassNonClientView::PaintOTRAvatar(ChromeCanvas* canvas) {
  if (!browser_view_->ShouldShowOffTheRecordAvatar())
    return;

  SkBitmap otr_avatar_icon = browser_view_->GetOTRAvatarIcon();
  canvas->DrawBitmapInt(otr_avatar_icon, 0,
      (otr_avatar_icon.height() - otr_avatar_bounds_.height()) / 2,
      otr_avatar_bounds_.width(), otr_avatar_bounds_.height(), 
      MirroredLeftPointForRect(otr_avatar_bounds_), otr_avatar_bounds_.y(),
      otr_avatar_bounds_.width(), otr_avatar_bounds_.height(), false);
}

void AeroGlassNonClientView::PaintClientEdge(ChromeCanvas* canvas) {
  int client_area_top =
      frame_->client_view()->y() + browser_view_->GetToolbarBounds().bottom();
  gfx::Rect client_area_bounds = CalculateClientAreaBounds(width(), height());
  // The toolbar draws a client edge along its own bottom edge when it's visible
  // and in normal mode.  However, it only draws this for the width of the
  // actual client area, leaving a gap at the left and right edges:
  //
  // |             Toolbar             | <-- part of toolbar
  //  ----- (toolbar client edge) -----  <-- gap
  // |           Client area           | <-- right client edge
  //
  // To address this, we extend the left and right client edges up to fill the
  // gap, by pretending the toolbar is shorter than it really is.
  client_area_top -= kClientEdgeThickness;

  int client_area_bottom =
      std::max(client_area_top, height() - NonClientBorderThickness());
  int client_area_height = client_area_bottom - client_area_top;
  SkBitmap* right = resources_->GetPartBitmap(FRAME_CLIENT_EDGE_RIGHT);
  canvas->TileImageInt(*right, client_area_bounds.right(), client_area_top,
                       right->width(), client_area_height);

  canvas->DrawBitmapInt(
      *resources_->GetPartBitmap(FRAME_CLIENT_EDGE_BOTTOM_RIGHT),
      client_area_bounds.right(), client_area_bottom);

  SkBitmap* bottom = resources_->GetPartBitmap(FRAME_CLIENT_EDGE_BOTTOM);
  canvas->TileImageInt(*bottom, client_area_bounds.x(),
      client_area_bottom, client_area_bounds.width(),
      bottom->height());

  SkBitmap* bottom_left =
      resources_->GetPartBitmap(FRAME_CLIENT_EDGE_BOTTOM_LEFT);
  canvas->DrawBitmapInt(*bottom_left,
      client_area_bounds.x() - bottom_left->width(), client_area_bottom);

  SkBitmap* left = resources_->GetPartBitmap(FRAME_CLIENT_EDGE_LEFT);
  canvas->TileImageInt(*left, client_area_bounds.x() - left->width(),
      client_area_top, left->width(), client_area_height);
}

void AeroGlassNonClientView::LayoutDistributorLogo() {
  int logo_x = frame_->GetMinimizeButtonOffset() - (distributor_logo_.empty() ?
      0 : (distributor_logo_.width() + kLogoCaptionSpacing));
  logo_bounds_.SetRect(logo_x, kLogoTopSpacing, distributor_logo_.width(),
                       distributor_logo_.height());
}

void AeroGlassNonClientView::LayoutOTRAvatar() {
  SkBitmap otr_avatar_icon = browser_view_->GetOTRAvatarIcon();
  int top_height = NonClientTopBorderHeight();
  int tabstrip_height = browser_view_->GetTabStripHeight() - kOTRBottomSpacing;
  int otr_height = frame_->IsMaximized() ?
      (tabstrip_height - kOTRMaximizedTopSpacing) :
      otr_avatar_icon.height();
  otr_avatar_bounds_.SetRect(NonClientBorderThickness() + kOTRSideSpacing,
                             top_height + tabstrip_height - otr_height,
                             otr_avatar_icon.width(), otr_height);
}

void AeroGlassNonClientView::LayoutClientView() {
  frame_->client_view()->SetBounds(CalculateClientAreaBounds(width(),
                                                             height()));
}

// static
void AeroGlassNonClientView::InitClass() {
  static bool initialized = false;
  if (!initialized) {
    resources_ = new AeroGlassWindowResources;

    ResourceBundle& rb = ResourceBundle::GetSharedInstance();
#if defined(GOOGLE_CHROME_BUILD)
    distributor_logo_ = *rb.GetBitmapNamed(IDR_DISTRIBUTOR_LOGO);
#endif

    initialized = true;
  }
}

