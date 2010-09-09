// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/cocoa/tabpose_window.h"

#import <QuartzCore/QuartzCore.h>

#include "app/resource_bundle.h"
#include "base/mac_util.h"
#include "base/scoped_callback_factory.h"
#include "base/sys_string_conversions.h"
#include "chrome/browser/browser_process.h"
#import "chrome/browser/cocoa/bookmark_bar_constants.h"
#import "chrome/browser/cocoa/browser_window_controller.h"
#import "chrome/browser/cocoa/tab_strip_controller.h"
#import "chrome/browser/cocoa/tab_strip_model_observer_bridge.h"
#import "chrome/browser/debugger/devtools_window.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/renderer_host/backing_store_mac.h"
#include "chrome/browser/renderer_host/render_view_host.h"
#include "chrome/browser/renderer_host/render_widget_host_view_mac.h"
#include "chrome/browser/tab_contents/tab_contents.h"
#include "chrome/browser/tab_contents/thumbnail_generator.h"
#include "chrome/common/pref_names.h"
#include "grit/app_resources.h"
#include "skia/ext/skia_utils_mac.h"
#include "third_party/skia/include/utils/mac/SkCGUtils.h"

const int kTopGradientHeight  = 15;

NSString* const kAnimationIdKey = @"AnimationId";
NSString* const kAnimationIdFadeIn = @"FadeIn";
NSString* const kAnimationIdFadeOut = @"FadeOut";

const CGFloat kDefaultAnimationDuration = 0.25;  // In seconds.
const CGFloat kSlomoFactor = 4;
const CGFloat kObserverChangeAnimationDuration = 0.75;  // In seconds.

// CAGradientLayer is 10.6-only -- roll our own.
@interface DarkGradientLayer : CALayer
- (void)drawInContext:(CGContextRef)context;
@end

@implementation DarkGradientLayer
- (void)drawInContext:(CGContextRef)context {
  scoped_cftyperef<CGColorSpaceRef> grayColorSpace(
      CGColorSpaceCreateWithName(kCGColorSpaceGenericGray));
  CGFloat grays[] = { 0.277, 1.0, 0.39, 1.0 };
  CGFloat locations[] = { 0, 1 };
  scoped_cftyperef<CGGradientRef> gradient(CGGradientCreateWithColorComponents(
      grayColorSpace.get(), grays, locations, arraysize(locations)));
  CGPoint topLeft = CGPointMake(0.0, kTopGradientHeight);
  CGContextDrawLinearGradient(context, gradient.get(), topLeft, CGPointZero, 0);
}
@end

namespace tabpose {
class ThumbnailLoader;
}

// A CALayer that draws a thumbnail for a TabContents object. The layer tries
// to draw the TabContents's backing store directly if possible, and requests
// a thumbnail bitmap from the TabContents's renderer process if not.
@interface ThumbnailLayer : CALayer {
  // The TabContents the thumbnail is for.
  TabContents* contents_;  // weak

  // The size the thumbnail is drawn at when zoomed in.
  NSSize fullSize_;

  // Used to load a thumbnail, if required.
  scoped_refptr<tabpose::ThumbnailLoader> loader_;

  // If the backing store couldn't be used and a thumbnail was returned from a
  // renderer process, it's stored in |thumbnail_|.
  scoped_cftyperef<CGImageRef> thumbnail_;

  // True if the layer already sent a thumbnail request to a renderer.
  BOOL didSendLoad_;
}
- (id)initWithTabContents:(TabContents*)contents fullSize:(NSSize)fullSize;
- (void)drawInContext:(CGContextRef)context;
- (void)setThumbnail:(const SkBitmap&)bitmap;
@end

namespace tabpose {

// ThumbnailLoader talks to the renderer process to load a thumbnail of a given
// RenderWidgetHost, and sends the thumbnail back to a ThumbnailLayer once it
// comes back from the renderer.
class ThumbnailLoader : public base::RefCountedThreadSafe<ThumbnailLoader> {
 public:
  ThumbnailLoader(gfx::Size size, RenderWidgetHost* rwh, ThumbnailLayer* layer)
      : size_(size), rwh_(rwh), layer_(layer), factory_(this) {}

  // Starts the fetch.
  void LoadThumbnail();

 private:
  friend class base::RefCountedThreadSafe<ThumbnailLoader>;
  ~ThumbnailLoader() {
    ResetPaintingObserver();
  }

  void DidReceiveBitmap(const SkBitmap& bitmap) {
    DCHECK(ChromeThread::CurrentlyOn(ChromeThread::UI));
    ResetPaintingObserver();
    [layer_ setThumbnail:bitmap];
  }

  void ResetPaintingObserver() {
    if (rwh_->painting_observer() != NULL) {
      DCHECK(rwh_->painting_observer() ==
             g_browser_process->GetThumbnailGenerator());
      rwh_->set_painting_observer(NULL);
    }
  }

  gfx::Size size_;
  RenderWidgetHost* rwh_;  // weak
  ThumbnailLayer* layer_;  // weak, owns us
  base::ScopedCallbackFactory<ThumbnailLoader> factory_;

  DISALLOW_COPY_AND_ASSIGN(ThumbnailLoader);
};

void ThumbnailLoader::LoadThumbnail() {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::UI));
  ThumbnailGenerator* generator = g_browser_process->GetThumbnailGenerator();
  if (!generator)  // In unit tests.
    return;

  // As mentioned in ThumbnailLayer's -drawInContext:, it's sufficient to have
  // thumbnails at the zoomed-out pixel size for all but the thumbnail the user
  // clicks on in the end. But we don't don't which thumbnail that will be, so
  // keep it simple and request full thumbnails for everything.
  // TODO(thakis): Request smaller thumbnails for users with many tabs.
  gfx::Size page_size(size_);  // Logical size the renderer renders at.
  gfx::Size pixel_size(size_);  // Physical pixel size the image is rendered at.

  DCHECK(rwh_->painting_observer() == NULL ||
         rwh_->painting_observer() == generator);
  rwh_->set_painting_observer(generator);

  // Will send an IPC to the renderer on the IO thread.
  generator->AskForSnapshot(
      rwh_,
      /*prefer_backing_store=*/false,
      factory_.NewCallback(&ThumbnailLoader::DidReceiveBitmap),
      page_size,
      pixel_size);
}

}  // namespace tabpose

@implementation ThumbnailLayer

- (id)initWithTabContents:(TabContents*)contents fullSize:(NSSize)fullSize {
  CHECK(contents);
  if ((self = [super init])) {
    contents_ = contents;
    fullSize_ = fullSize;
  }
  return self;
}

- (void)setTabContents:(TabContents*)contents {
  contents_ = contents;
}

- (void)setThumbnail:(const SkBitmap&)bitmap {
  // SkCreateCGImageRef() holds on to |bitmaps|'s memory, so this doesn't
  // create a copy.
  thumbnail_.reset(SkCreateCGImageRef(bitmap));
  loader_ = NULL;
  [self setNeedsDisplay];
}

- (int)topOffset {
  int topOffset = 0;

  // Medium term, we want to show thumbs of the actual info bar views, which
  // means I need to create InfoBarControllers here. At that point, we can get
  // the height from that controller. Until then, hardcode. :-/
  const int kInfoBarHeight = 31;
  topOffset += contents_->infobar_delegate_count() * kInfoBarHeight;

  bool always_show_bookmark_bar =
      contents_->profile()->GetPrefs()->GetBoolean(prefs::kShowBookmarkBar);
  bool has_detached_bookmark_bar =
      contents_->ShouldShowBookmarkBar() && !always_show_bookmark_bar;
  if (has_detached_bookmark_bar)
    topOffset += bookmarks::kNTPBookmarkBarHeight;

  return topOffset;
}

- (int)bottomOffset {
  int bottomOffset = 0;
  TabContents* devToolsContents =
      DevToolsWindow::GetDevToolsContents(contents_);
  if (devToolsContents && devToolsContents->render_view_host() &&
      devToolsContents->render_view_host()->view()) {
    // The devtool's size might not be up-to-date, but since its height doesn't
    // change on window resize, and since most users don't use devtools, this is
    // good enough.
    bottomOffset +=
        devToolsContents->render_view_host()->view()->GetViewBounds().height();
    bottomOffset += 1;  // :-( Divider line between web contents and devtools.
  }
  return bottomOffset;
}

- (void)drawBackingStore:(BackingStoreMac*)backing_store
                  inRect:(CGRect)destRect
                 context:(CGContextRef)context {
  // TODO(thakis): Add a sublayer for each accelerated surface in the rwhv.
  // Until then, accelerated layers (CoreAnimation NPAPI plugins, compositor)
  // won't show up in tabpose.
  if (backing_store->cg_layer()) {
    CGContextDrawLayerInRect(context, destRect, backing_store->cg_layer());
  } else {
    scoped_cftyperef<CGImageRef> image(
        CGBitmapContextCreateImage(backing_store->cg_bitmap()));
    CGContextDrawImage(context, destRect, image);
  }
}

- (void)drawInContext:(CGContextRef)context {
  RenderWidgetHost* rwh = contents_->render_view_host();
  // NULL if renderer crashed.
  RenderWidgetHostView* rwhv = rwh ? rwh->view() : NULL;
  if (!rwhv) {
    // TODO(thakis): Maybe draw a sad tab layer?
    [super drawInContext:context];
    return;
  }

  // The size of the TabContent's RenderWidgetHost might not fit to the
  // current browser window at all, for example if the window was resized while
  // this TabContents object was not an active tab.
  // Compute the required size ourselves. Leave room for eventual infobars and
  // a detached bookmarks bar on the top, and for the devtools on the bottom.
  // Download shelf is not included in the |fullSize| rect, so no need to
  // correct for it here.
  // TODO(thakis): This is not resolution-independent.
  int topOffset = [self topOffset];
  int bottomOffset = [self bottomOffset];
  gfx::Size desiredThumbSize(fullSize_.width,
                             fullSize_.height - topOffset - bottomOffset);

  // We need to ask the renderer for a thumbnail if
  // a) there's no backing store or
  // b) the backing store's size doesn't match our required size and
  // c) we didn't already send a thumbnail request to the renderer.
  BackingStoreMac* backing_store =
      (BackingStoreMac*)rwh->GetBackingStore(/*force_create=*/false);
  bool draw_backing_store =
      backing_store && backing_store->size() == desiredThumbSize;

  // Next weirdness: The destination rect. If the layer is |fullSize_| big, the
  // destination rect is (0, bottomOffset), (fullSize_.width, topOffset). But we
  // might be amidst an animation, so interpolate that rect.
  CGRect destRect = [self bounds];
  CGFloat scale = destRect.size.width / fullSize_.width;
  destRect.origin.y += bottomOffset * scale;
  destRect.size.height -= (bottomOffset + topOffset) * scale;

  // TODO(thakis): Draw infobars, detached bookmark bar as well.

  // If we haven't already, sent a thumbnail request to the renderer.
  if (!draw_backing_store && !didSendLoad_) {
    // Either the tab was never visible, or its backing store got evicted, or
    // the size of the backing store is wrong.

    // We only need a thumbnail the size of the zoomed-out layer for all
    // layers except the one the user clicks on. But since we can't know which
    // layer that is, request full-resolution layers for all tabs. This is
    // simple and seems to work in practice.
    loader_ = new tabpose::ThumbnailLoader(desiredThumbSize, rwh, self);
    loader_->LoadThumbnail();
    didSendLoad_ = YES;

    // Fill with bg color.
    [super drawInContext:context];
  }

  if (draw_backing_store) {
    // Backing store 'cache' hit!
    [self drawBackingStore:backing_store inRect:destRect context:context];
  } else if (thumbnail_) {
    // No cache hit, but the renderer returned a thumbnail to us.
    CGContextDrawImage(context, destRect, thumbnail_.get());
  }
}

@end

namespace {

class ScopedCAActionDisabler {
 public:
  ScopedCAActionDisabler() {
    [CATransaction begin];
    [CATransaction setValue:[NSNumber numberWithBool:YES]
                     forKey:kCATransactionDisableActions];
  }

  ~ScopedCAActionDisabler() {
    [CATransaction commit];
  }
};

class ScopedCAActionSetDuration {
 public:
  explicit ScopedCAActionSetDuration(CGFloat duration) {
    [CATransaction begin];
    [CATransaction setValue:[NSNumber numberWithFloat:duration]
                     forKey:kCATransactionAnimationDuration];
  }

  ~ScopedCAActionSetDuration() {
    [CATransaction commit];
  }
};

}  // namespace

// Given the number |n| of tiles with a desired aspect ratio of |a| and a
// desired distance |dx|, |dy| between tiles, returns how many tiles fit
// vertically into a rectangle with the dimensions |w_c|, |h_c|. This returns
// an exact solution, which is usually a fractional number.
static float FitNRectsWithAspectIntoBoundingSizeWithConstantPadding(
    int n, double a, int w_c, int h_c, int dx, int dy) {
  // We want to have the small rects have the same aspect ratio a as a full
  // tab. Let w, h be the size of a small rect, and w_c, h_c the size of the
  // container. dx, dy are the distances between small rects in x, y direction.

  // Geometry yields:
  // w_c = nx * (w + dx) - dx <=> w = (w_c + d_x) / nx - d_x
  // h_c = ny * (h + dy) - dy <=> h = (h_c + d_y) / ny - d_t
  // Plugging this into
  // a := tab_width / tab_height = w / h
  // yields
  // a = ((w_c - (nx - 1)*d_x)*ny) / (nx*(h_c - (ny - 1)*d_y))
  // Plugging in nx = n/ny and pen and paper (or wolfram alpha:
  // http://www.wolframalpha.com/input/?i=(-sqrt((d+n-a+f+n)^2-4+(a+f%2Ba+h)+(-d+n-n+w))%2Ba+f+n-d+n)/(2+a+(f%2Bh)) , (solution for nx)
  // http://www.wolframalpha.com/input/?i=+(-sqrt((a+f+n-d+n)^2-4+(d%2Bw)+(-a+f+n-a+h+n))-a+f+n%2Bd+n)/(2+(d%2Bw)) , (solution for ny)
  // ) gives us nx and ny (but the wrong root -- s/-sqrt(FOO)/sqrt(FOO)/.

  // This function returns ny.
  return (sqrt(pow(n * (a * dy - dx), 2) +
               4 * n * a * (dx + w_c) * (dy + h_c)) -
          n * (a * dy - dx))
      /
         (2 * (dx + w_c));
}

namespace tabpose {

// A tile is what is shown for a single tab in tabpose mode. It consists of a
// title, favicon, thumbnail image, and pre- and postanimation rects.
class Tile {
 public:
  Tile() {}

  // Returns the rectangle this thumbnail is at at the beginning of the zoom-in
  // animation. |tile| is the rectangle that's covering the whole tab area when
  // the animation starts.
  NSRect GetStartRectRelativeTo(const Tile& tile) const;
  NSRect thumb_rect() const { return thumb_rect_; }

  NSRect favicon_rect() const { return favicon_rect_; }
  SkBitmap favicon() const;

  // This changes |title_rect| and |favicon_rect| such that the favicon is on
  // the font's baseline and that the minimum distance between thumb rect and
  // favicon and title rects doesn't change.
  // The view code
  // 1. queries desired font size by calling |title_font_size()|
  // 2. loads that font
  // 3. calls |set_font_metrics()| which updates the title rect
  // 4. receives the title rect and puts the title on it with the font from 2.
  void set_font_metrics(CGFloat ascender, CGFloat descender);
  CGFloat title_font_size() const { return title_font_size_; }

  NSRect title_rect() const { return title_rect_; }

  // Returns an unelided title. The view logic is responsible for eliding.
  const string16& title() const { return contents_->GetTitle(); }

  TabContents* tab_contents() const { return contents_; }
  void set_tab_contents(TabContents* new_contents) { contents_ = new_contents; }

 private:
  friend class TileSet;

  // The thumb rect includes infobars, detached thumbnail bar, web contents,
  // and devtools.
  NSRect thumb_rect_;
  NSRect start_thumb_rect_;

  NSRect favicon_rect_;

  CGFloat title_font_size_;
  NSRect title_rect_;

  TabContents* contents_;  // weak

  DISALLOW_COPY_AND_ASSIGN(Tile);
};

NSRect Tile::GetStartRectRelativeTo(const Tile& tile) const {
  NSRect rect = start_thumb_rect_;
  rect.origin.x -= tile.start_thumb_rect_.origin.x;
  rect.origin.y -= tile.start_thumb_rect_.origin.y;
  return rect;
}

SkBitmap Tile::favicon() const {
  if (contents_->is_app()) {
    SkBitmap* icon = contents_->GetExtensionAppIcon();
    if (icon)
      return *icon;
  }
  return contents_->GetFavIcon();
}

// Changes |title_rect| and |favicon_rect| such that the favicon is on the
// font's baseline and that the minimum distance between thumb rect and
// favicon and title rects doesn't change.
void Tile::set_font_metrics(CGFloat ascender, CGFloat descender) {
  title_rect_.origin.y -= ascender + descender - NSHeight(title_rect_);
  title_rect_.size.height = ascender + descender;

  if (NSHeight(favicon_rect_) < ascender) {
    // Move favicon down.
    favicon_rect_.origin.y = title_rect_.origin.y + descender;
  } else {
    // Move title down.
    title_rect_.origin.y = favicon_rect_.origin.y - descender;
  }
}

// A tileset is responsible for owning and laying out all |Tile|s shown in a
// tabpose window.
class TileSet {
 public:
  TileSet() {}

  // Fills in |tiles_|.
  void Build(TabStripModel* source_model);

  // Computes coordinates for |tiles_|.
  void Layout(NSRect containing_rect);

  int selected_index() const { return selected_index_; }
  void set_selected_index(int index);

  const Tile& selected_tile() const { return *tiles_[selected_index()]; }
  Tile& tile_at(int index) { return *tiles_[index]; }
  const Tile& tile_at(int index) const { return *tiles_[index]; }

  // Inserts a new Tile object containing |contents| at |index|. Does no
  // relayout.
  void InsertTileAt(int index, TabContents* contents);

  // Removes the Tile object at |index|. Does no relayout.
  void RemoveTileAt(int index);

  // Moves the Tile object at |from_index| to |to_index|. Since this doesn't
  // change the number of tiles, relayout can be done just by swapping the
  // tile rectangles in the index interval [from_index, to_index], so this does
  // layout.
  void MoveTileFromTo(int from_index, int to_index);

 private:
  ScopedVector<Tile> tiles_;
  int selected_index_;

  DISALLOW_COPY_AND_ASSIGN(TileSet);
};

void TileSet::Build(TabStripModel* source_model) {
  selected_index_ =  source_model->selected_index();
  tiles_.resize(source_model->count());
  for (size_t i = 0; i < tiles_.size(); ++i) {
    tiles_[i] = new Tile;
    tiles_[i]->contents_ = source_model->GetTabContentsAt(i);
  }
}

void TileSet::Layout(NSRect containing_rect) {
  int tile_count = tiles_.size();
  if (tile_count == 0)  // Happens e.g. during test shutdown.
    return;

  // Room around the tiles insde of |containing_rect|.
  const int kSmallPaddingTop = 30;
  const int kSmallPaddingLeft = 30;
  const int kSmallPaddingRight = 30;
  const int kSmallPaddingBottom = 30;

  // Favicon / title area.
  const int kThumbTitlePaddingY = 6;
  const int kFaviconSize = 16;
  const int kTitleHeight = 14;  // Font size.
  const int kTitleExtraHeight = kThumbTitlePaddingY + kTitleHeight;
  const int kFaviconExtraHeight = kThumbTitlePaddingY + kFaviconSize;
  const int kFaviconTitleDistanceX = 6;
  const int kFooterExtraHeight =
      std::max(kFaviconExtraHeight, kTitleExtraHeight);

  // Room between the tiles.
  const int kSmallPaddingX = 15;
  const int kSmallPaddingY = kFooterExtraHeight;

  // Aspect ratio of the containing rect.
  CGFloat aspect = NSWidth(containing_rect) / NSHeight(containing_rect);

  // Room left in container after the outer padding is removed.
  double container_width =
      NSWidth(containing_rect) - kSmallPaddingLeft - kSmallPaddingRight;
  double container_height =
      NSHeight(containing_rect) - kSmallPaddingTop - kSmallPaddingBottom;

  // The tricky part is figuring out the size of a tab thumbnail, or since the
  // size of the containing rect is known, the number of tiles in x and y
  // direction.
  // Given are the size of the containing rect, and the number of thumbnails
  // that need to fit into that rect. The aspect ratio of the thumbnails needs
  // to be the same as that of |containing_rect|, else they will look distorted.
  // The thumbnails need to be distributed such that
  // |count_x * count_y >= tile_count|, and such that wasted space is minimized.
  //  See the comments in
  // |FitNRectsWithAspectIntoBoundingSizeWithConstantPadding()| for a more
  // detailed discussion.
  // TODO(thakis): It might be good enough to choose |count_x| and |count_y|
  //   such that count_x / count_y is roughly equal to |aspect|?
  double fny = FitNRectsWithAspectIntoBoundingSizeWithConstantPadding(
      tile_count, aspect,
      container_width, container_height - kFooterExtraHeight,
      kSmallPaddingX, kSmallPaddingY + kFooterExtraHeight);
  int count_y(roundf(fny));
  int count_x(ceilf(tile_count / float(count_y)));
  int last_row_count_x = tile_count - count_x * (count_y - 1);

  // Now that |count_x| and |count_y| are known, it's straightforward to compute
  // thumbnail width/height. See comment in
  // |FitNRectsWithAspectIntoBoundingSizeWithConstantPadding| for the derivation
  // of these two formulas.
  int small_width =
      floor((container_width + kSmallPaddingX) / float(count_x) -
            kSmallPaddingX);
  int small_height =
      floor((container_height + kSmallPaddingY) / float(count_y) -
            (kSmallPaddingY + kFooterExtraHeight));

  // |small_width / small_height| has only roughly an aspect ratio of |aspect|.
  // Shrink the thumbnail rect to make the aspect ratio fit exactly, and add
  // the extra space won by shrinking to the outer padding.
  int smallExtraPaddingLeft = 0;
  int smallExtraPaddingTop = 0;
  if (aspect > small_width/float(small_height)) {
    small_height = small_width / aspect;
    CGFloat all_tiles_height =
        (small_height + kSmallPaddingY + kFooterExtraHeight) * count_y -
        (kSmallPaddingY + kFooterExtraHeight);
    smallExtraPaddingTop = (container_height - all_tiles_height)/2;
  } else {
    small_width = small_height * aspect;
    CGFloat all_tiles_width =
        (small_width + kSmallPaddingX) * count_x - kSmallPaddingX;
    smallExtraPaddingLeft = (container_width - all_tiles_width)/2;
  }

  // Compute inter-tile padding in the zoomed-out view.
  CGFloat scale_small_to_big = NSWidth(containing_rect) / float(small_width);
  CGFloat big_padding_x = kSmallPaddingX * scale_small_to_big;
  CGFloat big_padding_y =
      (kSmallPaddingY + kFooterExtraHeight) * scale_small_to_big;

  // Now all dimensions are known. Lay out all tiles on a regular grid:
  // X X X X
  // X X X X
  // X X
  for (int row = 0, i = 0; i < tile_count; ++row) {
    for (int col = 0; col < count_x && i < tile_count; ++col, ++i) {
      // Compute the smalled, zoomed-out thumbnail rect.
      tiles_[i]->thumb_rect_.size = NSMakeSize(small_width, small_height);

      int small_x = col * (small_width + kSmallPaddingX) +
                    kSmallPaddingLeft + smallExtraPaddingLeft;
      int small_y = row * (small_height + kSmallPaddingY + kFooterExtraHeight) +
                    kSmallPaddingTop + smallExtraPaddingTop;

      tiles_[i]->thumb_rect_.origin = NSMakePoint(
          small_x, NSHeight(containing_rect) - small_y - small_height);

      tiles_[i]->favicon_rect_.size = NSMakeSize(kFaviconSize, kFaviconSize);
      tiles_[i]->favicon_rect_.origin = NSMakePoint(
          small_x,
          NSHeight(containing_rect) -
              (small_y + small_height + kFaviconExtraHeight));

      // Align lower left corner of title rect with lower left corner of favicon
      // for now. The final position is computed later by
      // |Tile::set_font_metrics()|.
      tiles_[i]->title_font_size_ = kTitleHeight;
      tiles_[i]->title_rect_.origin = NSMakePoint(
          NSMaxX(tiles_[i]->favicon_rect()) + kFaviconTitleDistanceX,
          NSMinY(tiles_[i]->favicon_rect()));
      tiles_[i]->title_rect_.size = NSMakeSize(
          small_width -
              NSWidth(tiles_[i]->favicon_rect()) - kFaviconTitleDistanceX,
          kTitleHeight);

      // Compute the big, pre-zoom thumbnail rect.
      tiles_[i]->start_thumb_rect_.size = containing_rect.size;

      int big_x = col * (NSWidth(containing_rect) + big_padding_x);
      int big_y = row * (NSHeight(containing_rect) + big_padding_y);
      tiles_[i]->start_thumb_rect_.origin = NSMakePoint(big_x, -big_y);
    }
  }

  // Go through last row and center it:
  // X X X X
  // X X X X
  //   X X
  int last_row_empty_tiles_x = count_x - last_row_count_x;
  CGFloat small_last_row_shift_x =
      last_row_empty_tiles_x * (small_width + kSmallPaddingX) / 2;
  CGFloat big_last_row_shift_x =
      last_row_empty_tiles_x * (NSWidth(containing_rect) + big_padding_x) / 2;
  for (int i = tile_count - last_row_count_x; i < tile_count; ++i) {
    tiles_[i]->thumb_rect_.origin.x += small_last_row_shift_x;
    tiles_[i]->start_thumb_rect_.origin.x += big_last_row_shift_x;
    tiles_[i]->favicon_rect_.origin.x += small_last_row_shift_x;
    tiles_[i]->title_rect_.origin.x += small_last_row_shift_x;
  }
}

void TileSet::set_selected_index(int index) {
  CHECK_GE(index, 0);
  CHECK_LT(index, static_cast<int>(tiles_.size()));
  selected_index_ = index;
}

void TileSet::InsertTileAt(int index, TabContents* contents) {
  tiles_.insert(tiles_.begin() + index, new Tile);
  tiles_[index]->contents_ = contents;
}

void TileSet::RemoveTileAt(int index) {
  tiles_.erase(tiles_.begin() + index);
}

// Moves the Tile object at |from_index| to |to_index|. Also updates rectangles
// so that the tiles stay in a left-to-right, top-to-bottom layout when walked
// in sequential order.
void TileSet::MoveTileFromTo(int from_index, int to_index) {
  NSRect thumb = tiles_[from_index]->thumb_rect_;
  NSRect start_thumb = tiles_[from_index]->start_thumb_rect_;
  NSRect favicon = tiles_[from_index]->favicon_rect_;
  NSRect title = tiles_[from_index]->title_rect_;

  scoped_ptr<Tile> tile(tiles_[from_index]);
  tiles_.weak_erase(tiles_.begin() + from_index);
  tiles_.insert(tiles_.begin() + to_index, tile.release());

  int step = from_index < to_index ? -1 : 1;
  for (int i = to_index; (i - from_index) * step < 0; i += step) {
    tiles_[i]->thumb_rect_ = tiles_[i + step]->thumb_rect_;
    tiles_[i]->start_thumb_rect_ = tiles_[i + step]->start_thumb_rect_;
    tiles_[i]->favicon_rect_ = tiles_[i + step]->favicon_rect_;
    tiles_[i]->title_rect_ = tiles_[i + step]->title_rect_;
  }
  tiles_[from_index]->thumb_rect_ = thumb;
  tiles_[from_index]->start_thumb_rect_ = start_thumb;
  tiles_[from_index]->favicon_rect_ = favicon;
  tiles_[from_index]->title_rect_ = title;
}

}  // namespace tabpose

void AnimateCALayerFrameFromTo(
    CALayer* layer, const NSRect& from, const NSRect& to,
    NSTimeInterval duration, id boundsAnimationDelegate) {
  // http://developer.apple.com/mac/library/qa/qa2008/qa1620.html
  CABasicAnimation* animation;

  animation = [CABasicAnimation animationWithKeyPath:@"bounds"];
  animation.fromValue = [NSValue valueWithRect:from];
  animation.toValue = [NSValue valueWithRect:to];
  animation.duration = duration;
  animation.timingFunction =
      [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseOut];
  animation.delegate = boundsAnimationDelegate;

  // Update the layer's bounds so the layer doesn't snap back when the animation
  // completes.
  layer.bounds = NSRectToCGRect(to);

  // Add the animation, overriding the implicit animation.
  [layer addAnimation:animation forKey:@"bounds"];

  // Prepare the animation from the current position to the new position.
  NSPoint opoint = from.origin;
  NSPoint point = to.origin;

  // Adapt to anchorPoint.
  opoint.x += NSWidth(from) * layer.anchorPoint.x;
  opoint.y += NSHeight(from) * layer.anchorPoint.y;
  point.x += NSWidth(to) * layer.anchorPoint.x;
  point.y += NSHeight(to) * layer.anchorPoint.y;

  animation = [CABasicAnimation animationWithKeyPath:@"position"];
  animation.fromValue = [NSValue valueWithPoint:opoint];
  animation.toValue = [NSValue valueWithPoint:point];
  animation.duration = duration;
  animation.timingFunction =
      [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseOut];

  // Update the layer's position so that the layer doesn't snap back when the
  // animation completes.
  layer.position = NSPointToCGPoint(point);

  // Add the animation, overriding the implicit animation.
  [layer addAnimation:animation forKey:@"position"];
}

@interface TabposeWindow (Private)
- (id)initForWindow:(NSWindow*)parent
               rect:(NSRect)rect
              slomo:(BOOL)slomo
      tabStripModel:(TabStripModel*)tabStripModel;
- (void)setUpLayersInSlomo:(BOOL)slomo;
- (void)fadeAway:(BOOL)slomo;
- (void)selectTileAtIndex:(int)newIndex;
@end

@implementation TabposeWindow

+ (id)openTabposeFor:(NSWindow*)parent
                rect:(NSRect)rect
               slomo:(BOOL)slomo
       tabStripModel:(TabStripModel*)tabStripModel {
  // Releases itself when closed.
  return [[TabposeWindow alloc]
      initForWindow:parent rect:rect slomo:slomo tabStripModel:tabStripModel];
}

- (id)initForWindow:(NSWindow*)parent
               rect:(NSRect)rect
              slomo:(BOOL)slomo
      tabStripModel:(TabStripModel*)tabStripModel {
  NSRect frame = [parent frame];
  if ((self = [super initWithContentRect:frame
                               styleMask:NSBorderlessWindowMask
                                 backing:NSBackingStoreBuffered
                                   defer:NO])) {
    containingRect_ = rect;
    tabStripModel_ = tabStripModel;
    state_ = tabpose::kFadingIn;
    tileSet_.reset(new tabpose::TileSet);
    tabStripModelObserverBridge_.reset(
        new TabStripModelObserverBridge(tabStripModel_, self));
    [self setReleasedWhenClosed:YES];
    [self setOpaque:NO];
    [self setBackgroundColor:[NSColor clearColor]];
    [self setUpLayersInSlomo:slomo];
    [self setAcceptsMouseMovedEvents:YES];
    [parent addChildWindow:self ordered:NSWindowAbove];
    [self makeKeyAndOrderFront:self];
  }
  return self;
}

- (CALayer*)selectedLayer {
  return [allThumbnailLayers_ objectAtIndex:tileSet_->selected_index()];
}

- (void)selectTileAtIndex:(int)newIndex {
  const tabpose::Tile& tile = tileSet_->tile_at(newIndex);
  selectionHighlight_.frame =
      NSRectToCGRect(NSInsetRect(tile.thumb_rect(), -5, -5));
  tileSet_->set_selected_index(newIndex);
}

- (void)selectTileAtIndexWithoutAnimation:(int)newIndex {
  ScopedCAActionDisabler disabler;
  [self selectTileAtIndex:newIndex];
}

- (void)addLayersForTile:(tabpose::Tile&)tile
                showZoom:(BOOL)showZoom
                   slomo:(BOOL)slomo
       animationDelegate:(id)animationDelegate {
  scoped_nsobject<CALayer> layer([[ThumbnailLayer alloc]
      initWithTabContents:tile.tab_contents()
                 fullSize:tile.GetStartRectRelativeTo(
                     tileSet_->selected_tile()).size]);
  [layer setNeedsDisplay];

  // Background color as placeholder for now.
  layer.get().backgroundColor = CGColorGetConstantColor(kCGColorWhite);
  if (showZoom) {
    AnimateCALayerFrameFromTo(
        layer,
        tile.GetStartRectRelativeTo(tileSet_->selected_tile()),
        tile.thumb_rect(),
        kDefaultAnimationDuration * (slomo ? kSlomoFactor : 1),
        animationDelegate);
  } else {
    layer.get().frame = NSRectToCGRect(tile.thumb_rect());
  }

  layer.get().shadowRadius = 10;
  layer.get().shadowOffset = CGSizeMake(0, -10);
  if (state_ == tabpose::kFadedIn)
    layer.get().shadowOpacity = 0.5;

  [bgLayer_ addSublayer:layer];
  [allThumbnailLayers_ addObject:layer];

  // Favicon and title.
  NSFont* font = [NSFont systemFontOfSize:tile.title_font_size()];
  tile.set_font_metrics([font ascender], -[font descender]);

  NSImage* nsFavicon = gfx::SkBitmapToNSImage(tile.favicon());
  // Either we don't have a valid favicon or there was some issue converting
  // it from an SkBitmap. Either way, just show the default.
  if (!nsFavicon) {
    NSImage* defaultFavIcon =
        ResourceBundle::GetSharedInstance().GetNSImageNamed(
            IDR_DEFAULT_FAVICON);
    nsFavicon = defaultFavIcon;
  }
  scoped_cftyperef<CGImageRef> favicon(
      mac_util::CopyNSImageToCGImage(nsFavicon));

  CALayer* faviconLayer = [CALayer layer];
  faviconLayer.frame = NSRectToCGRect(tile.favicon_rect());
  faviconLayer.contents = (id)favicon.get();
  faviconLayer.zPosition = 1;  // On top of the thumb shadow.
  if (state_ == tabpose::kFadingIn)
    faviconLayer.hidden = YES;
  [bgLayer_ addSublayer:faviconLayer];
  [allFaviconLayers_ addObject:faviconLayer];

  CATextLayer* titleLayer = [CATextLayer layer];
  titleLayer.frame = NSRectToCGRect(tile.title_rect());
  titleLayer.string = base::SysUTF16ToNSString(tile.title());
  titleLayer.fontSize = [font pointSize];
  titleLayer.truncationMode = kCATruncationEnd;
  titleLayer.font = font;
  titleLayer.zPosition = 1;  // On top of the thumb shadow.
  if (state_ == tabpose::kFadingIn)
    titleLayer.hidden = YES;
  [bgLayer_ addSublayer:titleLayer];
  [allTitleLayers_ addObject:titleLayer];
}

- (void)setUpLayersInSlomo:(BOOL)slomo {
  // Root layer -- covers whole window.
  rootLayer_ = [CALayer layer];

  // In a block so that the layers don't fade in.
  {
    ScopedCAActionDisabler disabler;
    // Background layer -- the visible part of the window.
    gray_.reset(CGColorCreateGenericGray(0.39, 1.0));
    bgLayer_ = [CALayer layer];
    bgLayer_.backgroundColor = gray_;
    bgLayer_.frame = NSRectToCGRect(containingRect_);
    bgLayer_.masksToBounds = YES;
    [rootLayer_ addSublayer:bgLayer_];

    // Selection highlight layer.
    darkBlue_.reset(CGColorCreateGenericRGB(0.25, 0.34, 0.86, 1.0));
    selectionHighlight_ = [CALayer layer];
    selectionHighlight_.backgroundColor = darkBlue_;
    selectionHighlight_.cornerRadius = 5.0;
    selectionHighlight_.zPosition = -1;  // Behind other layers.
    selectionHighlight_.hidden = YES;
    [bgLayer_ addSublayer:selectionHighlight_];

    // Top gradient.
    CALayer* gradientLayer = [DarkGradientLayer layer];
    gradientLayer.frame = CGRectMake(
        0,
        NSHeight(containingRect_) - kTopGradientHeight,
        NSWidth(containingRect_),
        kTopGradientHeight);
    [gradientLayer setNeedsDisplay];  // Draw once.
    [bgLayer_ addSublayer:gradientLayer];
  }

  // Layers for the tab thumbnails.
  tileSet_->Build(tabStripModel_);
  tileSet_->Layout(containingRect_);
  allThumbnailLayers_.reset(
      [[NSMutableArray alloc] initWithCapacity:tabStripModel_->count()]);
  allFaviconLayers_.reset(
      [[NSMutableArray alloc] initWithCapacity:tabStripModel_->count()]);
  allTitleLayers_.reset(
      [[NSMutableArray alloc] initWithCapacity:tabStripModel_->count()]);

  for (int i = 0; i < tabStripModel_->count(); ++i) {
    // Add a delegate to one of the animations to get a notification once the
    // animations are done.
    [self  addLayersForTile:tileSet_->tile_at(i)
                 showZoom:YES
                    slomo:slomo
        animationDelegate:i == tileSet_->selected_index() ? self : nil];
    if (i == tileSet_->selected_index()) {
      CALayer* layer = [allThumbnailLayers_ objectAtIndex:i];
      CAAnimation* animation = [layer animationForKey:@"bounds"];
      DCHECK(animation);
      [animation setValue:kAnimationIdFadeIn forKey:kAnimationIdKey];
    }
  }
  [self selectTileAtIndexWithoutAnimation:tileSet_->selected_index()];

  // Needs to happen after all layers have been added to |rootLayer_|, else
  // there's a one frame flash of grey at the beginning of the animation
  // (|bgLayer_| showing through with none of its children visible yet).
  [[self contentView] setLayer:rootLayer_];
  [[self contentView] setWantsLayer:YES];
}

- (BOOL)canBecomeKeyWindow {
 return YES;
}

- (void)keyDown:(NSEvent*)event {
  // Overridden to prevent beeps.
}

- (void)keyUp:(NSEvent*)event {
  if (state_ == tabpose::kFadingOut)
    return;

  NSString* characters = [event characters];
  if ([characters length] < 1)
    return;

  unichar character = [characters characterAtIndex:0];
  switch (character) {
    case NSEnterCharacter:
    case NSNewlineCharacter:
    case NSCarriageReturnCharacter:
    case ' ':
      [self fadeAway:([event modifierFlags] & NSShiftKeyMask) != 0];
      break;
    case '\e':  // Escape
      tileSet_->set_selected_index(tabStripModel_->selected_index());
      [self fadeAway:([event modifierFlags] & NSShiftKeyMask) != 0];
      break;
    // TODO(thakis): Support moving the selection via arrow keys.
  }
}

- (void)mouseMoved:(NSEvent*)event {
  int newIndex = -1;
  CGPoint p = NSPointToCGPoint([event locationInWindow]);
  for (NSUInteger i = 0; i < [allThumbnailLayers_ count]; ++i) {
    CALayer* layer = [allThumbnailLayers_ objectAtIndex:i];
    CGPoint lp = [layer convertPoint:p fromLayer:rootLayer_];
    if ([static_cast<CALayer*>([layer presentationLayer]) containsPoint:lp])
      newIndex = i;
  }
  if (newIndex >= 0)
    [self selectTileAtIndexWithoutAnimation:newIndex];
}

- (void)mouseDown:(NSEvent*)event {
  [self fadeAway:([event modifierFlags] & NSShiftKeyMask) != 0];
}

- (void)swipeWithEvent:(NSEvent*)event {
  if (abs([event deltaY]) > 0.5)  // Swipe up or down.
    [self fadeAway:([event modifierFlags] & NSShiftKeyMask) != 0];
}

- (void)close {
  // Prevent parent window from disappearing.
  [[self parentWindow] removeChildWindow:self];

  // We're dealloc'd in an autorelease pool – by then the observer registry
  // might be dead, so explicitly reset the observer now.
  tabStripModelObserverBridge_.reset();

  [super close];
}

- (void)commandDispatch:(id)sender {
  // Without this, -validateUserInterfaceItem: is not called.
}

- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)item {
  // Disable all browser-related menu items.
  return NO;
}

- (void)fadeAway:(BOOL)slomo {
  if (state_ == tabpose::kFadingOut)
    return;

  state_ = tabpose::kFadingOut;
  [self setAcceptsMouseMovedEvents:NO];

  // Select chosen tab.
  if (tileSet_->selected_index() < tabStripModel_->count()) {
    tabStripModel_->SelectTabContentsAt(tileSet_->selected_index(),
                                        /*user_gesture=*/true);
  } else {
    DCHECK_EQ(tileSet_->selected_index(), 0);
  }

  {
    ScopedCAActionDisabler disableCAActions;

    // Move the selected layer on top of all other layers.
    [self selectedLayer].zPosition = 1;

    selectionHighlight_.hidden = YES;
    for (CALayer* layer in allFaviconLayers_.get())
      layer.hidden = YES;
    for (CALayer* layer in allTitleLayers_.get())
      layer.hidden = YES;

    // Running animations with shadows is slow, so turn shadows off before
    // running the exit animation.
    for (CALayer* layer in allThumbnailLayers_.get())
      layer.shadowOpacity = 0.0;
  }

  // Animate layers out, all in one transaction.
  CGFloat duration =
      1.3 * kDefaultAnimationDuration * (slomo ? kSlomoFactor : 1);
  ScopedCAActionSetDuration durationSetter(duration);
  for (NSUInteger i = 0; i < [allThumbnailLayers_ count]; ++i) {
    CALayer* layer = [allThumbnailLayers_ objectAtIndex:i];
    // |start_thumb_rect_| was relative to the initial index, now this needs to
    // be relative to |selectedIndex_| (whose start rect was relative to
    // the initial index, too).
    CGRect newFrame = NSRectToCGRect(
        tileSet_->tile_at(i).GetStartRectRelativeTo(tileSet_->selected_tile()));

    // Add a delegate to one of the implicit animations to get a notification
    // once the animations are done.
    if (static_cast<int>(i) == tileSet_->selected_index()) {
      CAAnimation* animation = [CAAnimation animation];
      animation.delegate = self;
      [animation setValue:kAnimationIdFadeOut forKey:kAnimationIdKey];
      [layer addAnimation:animation forKey:@"frame"];
    }

    layer.frame = newFrame;

    if (static_cast<int>(i) == tileSet_->selected_index()) {
      // Redraw layer at big resolution, so that zoom-in isn't blocky.
      [layer setNeedsDisplay];
    }
  }
}

- (void)animationDidStop:(CAAnimation*)animation finished:(BOOL)finished {
  NSString* animationId = [animation valueForKey:kAnimationIdKey];
  if ([animationId isEqualToString:kAnimationIdFadeIn]) {
    if (finished && state_ == tabpose::kFadingIn) {
      // If the user clicks while the fade in animation is still running,
      // |state_| is already kFadingOut. In that case, don't do anything.
      state_ = tabpose::kFadedIn;

      selectionHighlight_.hidden = NO;
      for (CALayer* layer in allFaviconLayers_.get())
        layer.hidden = NO;
      for (CALayer* layer in allTitleLayers_.get())
        layer.hidden = NO;

      // Running animations with shadows is slow, so turn shadows on only after
      // the animation is done.
      ScopedCAActionDisabler disableCAActions;
      for (CALayer* layer in allThumbnailLayers_.get())
        layer.shadowOpacity = 0.5;
    }
  } else if ([animationId isEqualToString:kAnimationIdFadeOut]) {
    DCHECK_EQ(tabpose::kFadingOut, state_);
    [self close];
  }
}

- (NSUInteger)thumbnailLayerCount {
  return [allThumbnailLayers_ count];
}

- (int)selectedIndex {
  return tileSet_->selected_index();
}

#pragma mark TabStripModelBridge

- (void)refreshLayerFramesAtIndex:(int)i {
  const tabpose::Tile& tile = tileSet_->tile_at(i);

  CALayer* faviconLayer = [allFaviconLayers_ objectAtIndex:i];
  faviconLayer.frame = NSRectToCGRect(tile.favicon_rect());
  CALayer* titleLayer = [allTitleLayers_ objectAtIndex:i];
  titleLayer.frame = NSRectToCGRect(tile.title_rect());
  CALayer* thumbLayer = [allThumbnailLayers_ objectAtIndex:i];
  thumbLayer.frame = NSRectToCGRect(tile.thumb_rect());
}

- (void)insertTabWithContents:(TabContents*)contents
                      atIndex:(NSInteger)index
                 inForeground:(bool)inForeground {
  // This happens if you cmd-click a link and then immediately open tabpose
  // on a slowish machine.
  ScopedCAActionSetDuration durationSetter(kObserverChangeAnimationDuration);

  // Insert new layer and relayout.
  tileSet_->InsertTileAt(index, contents);
  tileSet_->Layout(containingRect_);
  [self  addLayersForTile:tileSet_->tile_at(index)
                 showZoom:NO
                    slomo:NO
        animationDelegate:nil];

  // Update old layers.
  DCHECK_EQ(tabStripModel_->count(),
            static_cast<int>([allThumbnailLayers_ count]));
  DCHECK_EQ(tabStripModel_->count(),
            static_cast<int>([allTitleLayers_ count]));
  DCHECK_EQ(tabStripModel_->count(),
            static_cast<int>([allFaviconLayers_ count]));

  for (int i = 0; i < tabStripModel_->count(); ++i) {
    if (i == index)  // The new layer.
      continue;
    [self refreshLayerFramesAtIndex:i];
  }

  // Update selection.
  int selectedIndex = tileSet_->selected_index();
  if (selectedIndex >= index)
    selectedIndex++;
  [self selectTileAtIndex:selectedIndex];
}

- (void)tabClosingWithContents:(TabContents*)contents
                       atIndex:(NSInteger)index {
  // We will also get a -tabDetachedWithContents:atIndex: notification for
  // closing tabs, so do nothing here.
}

- (void)tabDetachedWithContents:(TabContents*)contents
                        atIndex:(NSInteger)index {
  ScopedCAActionSetDuration durationSetter(kObserverChangeAnimationDuration);

  // Remove layer and relayout.
  tileSet_->RemoveTileAt(index);
  tileSet_->Layout(containingRect_);

  [[allThumbnailLayers_ objectAtIndex:index] removeFromSuperlayer];
  [allThumbnailLayers_ removeObjectAtIndex:index];
  [[allTitleLayers_ objectAtIndex:index] removeFromSuperlayer];
  [allTitleLayers_ removeObjectAtIndex:index];
  [[allFaviconLayers_ objectAtIndex:index] removeFromSuperlayer];
  [allFaviconLayers_ removeObjectAtIndex:index];

  // Update old layers.
  DCHECK_EQ(tabStripModel_->count(),
            static_cast<int>([allThumbnailLayers_ count]));
  DCHECK_EQ(tabStripModel_->count(),
            static_cast<int>([allTitleLayers_ count]));
  DCHECK_EQ(tabStripModel_->count(),
            static_cast<int>([allFaviconLayers_ count]));

  if (tabStripModel_->count() == 0)
    [self close];

  for (int i = 0; i < tabStripModel_->count(); ++i)
    [self refreshLayerFramesAtIndex:i];

  // Update selection.
  int selectedIndex = tileSet_->selected_index();
  if (selectedIndex >= index)
    selectedIndex--;
  if (selectedIndex >= 0)
    [self selectTileAtIndex:selectedIndex];
}

- (void)tabMovedWithContents:(TabContents*)contents
                    fromIndex:(NSInteger)from
                      toIndex:(NSInteger)to {
  ScopedCAActionSetDuration durationSetter(kObserverChangeAnimationDuration);

  // Move tile from |from| to |to|.
  tileSet_->MoveTileFromTo(from, to);

  // Move corresponding layers from |from| to |to|.
  scoped_nsobject<CALayer> thumbLayer(
      [[allThumbnailLayers_ objectAtIndex:from] retain]);
  [allThumbnailLayers_ removeObjectAtIndex:from];
  [allThumbnailLayers_ insertObject:thumbLayer.get() atIndex:to];
  scoped_nsobject<CALayer> faviconLayer(
      [[allFaviconLayers_ objectAtIndex:from] retain]);
  [allFaviconLayers_ removeObjectAtIndex:from];
  [allFaviconLayers_ insertObject:faviconLayer.get() atIndex:to];
  scoped_nsobject<CALayer> titleLayer(
      [[allTitleLayers_ objectAtIndex:from] retain]);
  [allTitleLayers_ removeObjectAtIndex:from];
  [allTitleLayers_ insertObject:titleLayer.get() atIndex:to];

  // Update frames of the layers.
  for (int i = std::min(from, to); i <= std::max(from, to); ++i)
    [self refreshLayerFramesAtIndex:i];

  // Update selection.
  int selectedIndex = tileSet_->selected_index();
  if (from == selectedIndex)
    selectedIndex = to;
  else if (from < selectedIndex && selectedIndex <= to)
    selectedIndex--;
  else if (to <= selectedIndex && selectedIndex < from)
    selectedIndex++;
  [self selectTileAtIndex:selectedIndex];
}

- (void)tabChangedWithContents:(TabContents*)contents
                       atIndex:(NSInteger)index
                    changeType:(TabStripModelObserver::TabChangeType)change {
  // Tell the window to update text, title, and thumb layers at |index| to get
  // their data from |contents|. |contents| can be different from the old
  // contents at that index!
  // While a tab is loading, this is unfortunately called quite often for
  // both the "loading" and the "all" change types, so we don't really want to
  // send thumb requests to the corresponding renderer when this is called.
  // For now, just make sure that we don't hold on to an invalid TabContents
  // object.
  tabpose::Tile& tile = tileSet_->tile_at(index);
  if (contents == tile.tab_contents()) {
    // TODO(thakis): Install a timer to send a thumb request/update title/update
    // favicon after 20ms or so, and reset the timer every time this is called
    // to make sure we get an updated thumb, without requesting them all over.
    return;
  }

  tile.set_tab_contents(contents);
  ThumbnailLayer* thumbLayer = [allThumbnailLayers_ objectAtIndex:index];
  [thumbLayer setTabContents:contents];
}

- (void)tabStripModelDeleted {
  [self close];
}

@end
