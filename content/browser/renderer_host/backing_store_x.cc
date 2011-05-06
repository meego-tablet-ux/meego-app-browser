// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_widget_host.h"
#include "content/browser/renderer_host/render_widget_host_view.h"
#include "content/browser/renderer_host/backing_store_x.h"

#include <cairo-xlib.h>

#if !defined(TOOLKIT_MEEGOTOUCH)
#include <gtk/gtk.h>
#endif

#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#if defined(OS_OPENBSD) || defined(OS_FREEBSD)
#include <sys/endian.h>
#endif

#include <algorithm>
#include <utility>
#include <limits>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/metrics/histogram.h"
#include "base/time.h"
#include "content/browser/renderer_host/render_process_host.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/x/x11_util.h"
#include "ui/base/x/x11_util_internal.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/surface/transport_dib.h"

#if defined(TILED_BACKING_STORE)
#include "chrome/common/render_tiling.h"
#endif

// Assume that somewhere along the line, someone will do width * height * 4
// with signed numbers. If the maximum value is 2**31, then 2**31 / 4 =
// 2**29 and floor(sqrt(2**29)) = 23170.

// Max height and width for layers
static const int kMaxVideoLayerSize = 23170;

// X Backing Stores:
//
// Unlike Windows, where the backing store is kept in heap memory, we keep our
// backing store in the X server, as a pixmap. Thus expose events just require
// instructing the X server to copy from the backing store to the window.
//
// The backing store is in the same format as the visual which our main window
// is using. Bitmaps from the renderer are uploaded to the X server, either via
// shared memory or over the wire, and XRENDER is used to convert them to the
// correct format for the backing store.

// Destroys the image and the associated shared memory structures. This is a
// helper function for code using shared memory.
static void DestroySharedImage(Display* display,
                               XImage* image,
                               XShmSegmentInfo* shminfo) {
  XShmDetach(display, shminfo);
  XDestroyImage(image);
  shmdt(shminfo->shmaddr);
}

BackingStoreX::BackingStoreX(RenderWidgetHost* widget,
                            const gfx::Size& size,
                            void* visual,
                            int depth)
    : BackingStore(widget, size),
      display_(ui::GetXDisplay()),
      shared_memory_support_(ui::QuerySharedMemorySupport(display_)),
      use_render_(ui::QueryRenderSupport(display_)),
      visual_(visual),
      visual_depth_(depth),
      root_window_(ui::GetX11RootWindow()) {
#if defined(OS_OPENBSD) || defined(OS_FREEBSD)
  COMPILE_ASSERT(_BYTE_ORDER == _LITTLE_ENDIAN, assumes_little_endian);
#else
  COMPILE_ASSERT(__BYTE_ORDER == __LITTLE_ENDIAN, assumes_little_endian);
#endif

#if !defined(TILED_BACKING_STORE)
  pixmap_ = XCreatePixmap(display_, root_window_,
                          size.width(), size.height(), depth);

  if (use_render_) {
    picture_ = XRenderCreatePicture(
        display_, pixmap_,
        ui::GetRenderVisualFormat(display_,
                                  static_cast<Visual*>(visual)),
                                  0, NULL);
    pixmap_bpp_ = 0;
  } else {
    picture_ = 0;
    pixmap_bpp_ = ui::BitsPerPixelForPixmapDepth(display_, depth);
  }

  pixmap_gc_ = XCreateGC(display_, pixmap_, 0, NULL);
#else //TILED_BACKING_STORE
  tiles_map_seq_ = 1;
  tiles_paint_tag_ = 1;
  contents_scale_ = 1.0;
  pending_scaling_ = false;
  frozen_ = false;
#endif
}

BackingStoreX::BackingStoreX(RenderWidgetHost* widget, const gfx::Size& size)
    : BackingStore(widget, size),
      display_(NULL),
      shared_memory_support_(ui::SHARED_MEMORY_NONE),
      use_render_(false),
      pixmap_bpp_(0),
      visual_(NULL),
      visual_depth_(-1),
      root_window_(0),
      pixmap_(0),
      picture_(0),
      pixmap_gc_(NULL) {
}

BackingStoreX::~BackingStoreX() {
  // In unit tests, display_ may be NULL.
  if (!display_)
    return;

#if !defined(TILED_BACKING_STORE)
  XRenderFreePicture(display_, picture_);
  XFreePixmap(display_, pixmap_);
  XFreeGC(display_, static_cast<GC>(pixmap_gc_));
#else
  
#endif
}

size_t BackingStoreX::MemorySize() {
  if (!use_render_)
    return size().GetArea() * (pixmap_bpp_ / 8);
  else
    return size().GetArea() * 4;
}

void BackingStoreX::PaintRectWithoutXrender(
    TransportDIB* bitmap,
    const gfx::Rect& bitmap_rect,
    const std::vector<gfx::Rect>& copy_rects) {
#if !defined(TILED_BACKING_STORE)
  const int width = bitmap_rect.width();
  const int height = bitmap_rect.height();
  Pixmap pixmap = XCreatePixmap(display_, root_window_, width, height,
                                visual_depth_);

  // Draw ARGB transport DIB onto our pixmap.
  ui::PutARGBImage(display_, visual_, visual_depth_, pixmap,
                   pixmap_gc_, static_cast<uint8*>(bitmap->memory()),
                   width, height);

  for (size_t i = 0; i < copy_rects.size(); i++) {
    const gfx::Rect& copy_rect = copy_rects[i];
    XCopyArea(display_,
              pixmap,                           // src
              pixmap_,                          // dest
              static_cast<GC>(pixmap_gc_),      // gc
              copy_rect.x() - bitmap_rect.x(),  // src_x
              copy_rect.y() - bitmap_rect.y(),  // src_y
              copy_rect.width(),                // width
              copy_rect.height(),               // height
              copy_rect.x(),                    // dest_x
              copy_rect.y());                   // dest_y
  }

  XFreePixmap(display_, pixmap);
#else
  NOTIMPLEMENTED();
#endif
}

void BackingStoreX::PaintToBackingStore(
    RenderProcessHost* process,
    TransportDIB::Id bitmap,
    const gfx::Rect& bitmap_rect,
    const std::vector<gfx::Rect>& copy_rects,
    unsigned int seq) {
  if (!display_)
    return;

  if (bitmap_rect.IsEmpty())
    return;

  DLOG(INFO) << "BackingStoreX::PaintToBackingStore " << bitmap_rect.width() << " " << bitmap_rect.height();
  
  const int width = bitmap_rect.width();
  const int height = bitmap_rect.height();

  if (width <= 0 || width > kMaxVideoLayerSize ||
      height <= 0 || height > kMaxVideoLayerSize)
    return;

  TransportDIB* dib = process->GetTransportDIB(bitmap);
  if (!dib)
    return;

  if (!use_render_)
    return PaintRectWithoutXrender(dib, bitmap_rect, copy_rects);

  Picture picture;
  Pixmap pixmap;

  if (shared_memory_support_ == ui::SHARED_MEMORY_PIXMAP) {
    XShmSegmentInfo shminfo = {0};
    shminfo.shmseg = dib->MapToX(display_);

    // The NULL in the following is the |data| pointer: this is an artifact of
    // Xlib trying to be helpful, rather than just exposing the X protocol. It
    // assumes that we have the shared memory segment mapped into our memory,
    // which we don't, and it's trying to calculate an offset by taking the
    // difference between the |data| pointer and the address of the mapping in
    // |shminfo|. Since both are NULL, the offset will be calculated to be 0,
    // which is correct for us.
    pixmap = XShmCreatePixmap(display_, root_window_, NULL, &shminfo,
                              width, height, 32);
  } else {
    // We don't have shared memory pixmaps.  Fall back to creating a pixmap
    // ourselves and putting an image on it.
    pixmap = XCreatePixmap(display_, root_window_, width, height, 32);
    GC gc = XCreateGC(display_, pixmap, 0, NULL);

    if (shared_memory_support_ == ui::SHARED_MEMORY_PUTIMAGE) {
      const XID shmseg = dib->MapToX(display_);

      XShmSegmentInfo shminfo;
      memset(&shminfo, 0, sizeof(shminfo));
      shminfo.shmseg = shmseg;
      shminfo.shmaddr = static_cast<char*>(dib->memory());

      XImage* image = XShmCreateImage(display_, static_cast<Visual*>(visual_),
                                      32, ZPixmap,
                                      shminfo.shmaddr, &shminfo,
                                      width, height);

      // This code path is important for performance and we have found that
      // different techniques work better on different platforms. See
      // http://code.google.com/p/chromium/issues/detail?id=44124.
      //
      // Checking for ARM is an approximation, but it seems to be a good one so
      // far.
#if defined(ARCH_CPU_ARM_FAMILY)
      for (size_t i = 0; i < copy_rects.size(); i++) {
        const gfx::Rect& copy_rect = copy_rects[i];
        XShmPutImage(display_, pixmap, gc, image,
                     copy_rect.x() - bitmap_rect.x(), /* source x */
                     copy_rect.y() - bitmap_rect.y(), /* source y */
                     copy_rect.x() - bitmap_rect.x(), /* dest x */
                     copy_rect.y() - bitmap_rect.y(), /* dest y */
                     copy_rect.width(), copy_rect.height(),
                     False /* send_event */);
      }
#else
      XShmPutImage(display_, pixmap, gc, image,
                   0, 0 /* source x, y */, 0, 0 /* dest x, y */,
                   width, height, False /* send_event */);
#endif
      XDestroyImage(image);
    } else {  // case SHARED_MEMORY_NONE
      // No shared memory support, we have to copy the bitmap contents
      // to the X server. Xlib wraps the underlying PutImage call
      // behind several layers of functions which try to convert the
      // image into the format which the X server expects. The
      // following values hopefully disable all conversions.
      XImage image;
      memset(&image, 0, sizeof(image));

      image.width = width;
      image.height = height;
      image.depth = 32;
      image.bits_per_pixel = 32;
      image.format = ZPixmap;
      image.byte_order = LSBFirst;
      image.bitmap_unit = 8;
      image.bitmap_bit_order = LSBFirst;
      image.bytes_per_line = width * 4;
      image.red_mask = 0xff;
      image.green_mask = 0xff00;
      image.blue_mask = 0xff0000;
      image.data = static_cast<char*>(dib->memory());

      XPutImage(display_, pixmap, gc, &image,
                0, 0 /* source x, y */, 0, 0 /* dest x, y */,
                width, height);
    }
    XFreeGC(display_, gc);
  }

#if !defined(TILED_BACKING_STORE)
  picture = ui::CreatePictureFromSkiaPixmap(display_, pixmap);

  for (size_t i = 0; i < copy_rects.size(); i++) {
    const gfx::Rect& copy_rect = copy_rects[i];
    XRenderComposite(display_,
                     PictOpSrc,                        // op
                     picture,                          // src
                     0,                                // mask
                     picture_,                         // dest
                     copy_rect.x() - bitmap_rect.x(),  // src_x
                     copy_rect.y() - bitmap_rect.y(),  // src_y
                     0,                                // mask_x
                     0,                                // mask_y
                     copy_rect.x(),                    // dest_x
                     copy_rect.y(),                    // dest_y
                     copy_rect.width(),                // width
                     copy_rect.height());              // height
  }
#else
  QPixmap qpixmap = QPixmap::fromX11Pixmap(pixmap);
  
#if !defined(NDEBUG)
  static int counter = 0;
  QString file_name = QString("/home/meego/tmp/update") + QString::number(counter)
                      + QString(".png");
  //qpixmap.toImage().save(file_name);
  if (counter >= 10)
    counter = 0;
  else
    counter++;
#endif
    
  for (size_t i = 0; i < copy_rects.size(); i++) {
    QRect copy_rect(copy_rects[i].x(),
                    copy_rects[i].y(),
                    copy_rects[i].width(),
                    copy_rects[i].height());
    QRect bitmap_qrect(bitmap_rect.x(), bitmap_rect.y(),
                       bitmap_rect.width(), bitmap_rect.height());
  
    if (tiles_map_seq_ != seq)
    {
      return;
    }
    
    QRect dirty_rect(MapFromContents(copy_rect));

    TileIndex first = GetTileIndexFrom(dirty_rect.topLeft());
    TileIndex last = GetTileIndexFrom(dirty_rect.bottomRight());

    for (int x = first.x(); x <= last.x(); x++)
    {
      for (int y = first.y(); y <= last.y(); y++)
      {
        TileIndex index(x, y);
        scoped_refptr<Tile> tile = GetTileAt(index);
        if (tile.get())
        {
          tile->PaintToBackingStore(qpixmap, bitmap_qrect, dirty_rect);
        }
      }
    }

    // Schedule update to Qt
    gfx::Rect updated_rect(dirty_rect.x(), dirty_rect.y(), dirty_rect.width(), dirty_rect.height());
    render_widget_host_->view()->DidBackingStorePaint(updated_rect);
  }
#endif

  // In the case of shared memory, we wait for the composite to complete so that
  // we are sure that the X server has finished reading from the shared memory
  // segment.
  if (shared_memory_support_ != ui::SHARED_MEMORY_NONE)
    XSync(display_, False);

#if !defined(TILED_BACKING_STORE)
  XRenderFreePicture(display_, picture);
#endif
  XFreePixmap(display_, pixmap);
}

bool BackingStoreX::CopyFromBackingStore(const gfx::Rect& rect,
                                         skia::PlatformCanvas* output) {
#if !defined(TILED_BACKING_STORE)
  base::TimeTicks begin_time = base::TimeTicks::Now();

  if (visual_depth_ < 24) {
    // CopyFromBackingStore() copies pixels out of the XImage
    // in a way that assumes that each component (red, green,
    // blue) is a byte.  This doesn't work on visuals which
    // encode a pixel color with less than a byte per color.
    return false;
  }

  const int width = std::min(size().width(), rect.width());
  const int height = std::min(size().height(), rect.height());

  XImage* image;
  XShmSegmentInfo shminfo;  // Used only when shared memory is enabled.
  if (shared_memory_support_ != ui::SHARED_MEMORY_NONE) {
    // Use shared memory for faster copies when it's available.
    Visual* visual = static_cast<Visual*>(visual_);
    memset(&shminfo, 0, sizeof(shminfo));
    image = XShmCreateImage(display_, visual, 32,
                            ZPixmap, NULL, &shminfo, width, height);
    if (!image) {
      return false;
    }
    // Create the shared memory segment for the image and map it.
    if (image->bytes_per_line == 0 || image->height == 0 ||
        static_cast<size_t>(image->height) >
        (std::numeric_limits<size_t>::max() / image->bytes_per_line)) {
      XDestroyImage(image);
      return false;
    }
    shminfo.shmid = shmget(IPC_PRIVATE, image->bytes_per_line * image->height,
                           IPC_CREAT|0666);
    if (shminfo.shmid == -1) {
      XDestroyImage(image);
      return false;
    }

    void* mapped_memory = shmat(shminfo.shmid, NULL, SHM_RDONLY);
    shmctl(shminfo.shmid, IPC_RMID, 0);
    if (mapped_memory == (void*)-1) {
      XDestroyImage(image);
      return false;
    }
    shminfo.shmaddr = image->data = static_cast<char*>(mapped_memory);

    if (!XShmAttach(display_, &shminfo) ||
        !XShmGetImage(display_, pixmap_, image, rect.x(), rect.y(),
                      AllPlanes)) {
      DestroySharedImage(display_, image, &shminfo);
      return false;
    }
  } else {
    // Non-shared memory case just copy the image from the server.
    image = XGetImage(display_, pixmap_,
                      rect.x(), rect.y(), width, height,
                      AllPlanes, ZPixmap);
  }

  // TODO(jhawkins): Need to convert the image data if the image bits per pixel
  // is not 32.
  // Note that this also initializes the output bitmap as opaque.
  if (!output->initialize(width, height, true) ||
      image->bits_per_pixel != 32) {
    if (shared_memory_support_ != ui::SHARED_MEMORY_NONE)
      DestroySharedImage(display_, image, &shminfo);
    else
      XDestroyImage(image);
    return false;
  }

  // The X image might have a different row stride, so iterate through
  // it and copy each row out, only up to the pixels we're actually
  // using.  This code assumes a visual mode where a pixel is
  // represented using a 32-bit unsigned int, with a byte per component.
  SkBitmap bitmap = output->getTopPlatformDevice().accessBitmap(true);
  for (int y = 0; y < height; y++) {
    const uint32* src_row = reinterpret_cast<uint32*>(
        &image->data[image->bytes_per_line * y]);
    uint32* dest_row = bitmap.getAddr32(0, y);
    for (int x = 0; x < width; ++x, ++dest_row) {
      // Force alpha to be 0xff, because otherwise it causes rendering problems.
      *dest_row = src_row[x] | 0xff000000;
    }
  }

  if (shared_memory_support_ != ui::SHARED_MEMORY_NONE)
    DestroySharedImage(display_, image, &shminfo);
  else
    XDestroyImage(image);

  HISTOGRAM_TIMES("BackingStore.RetrievalFromX",
                  base::TimeTicks::Now() - begin_time);

  return true;
#else
  NOTIMPLEMENTED();
  return false;
#endif
}

void BackingStoreX::ScrollBackingStore(int dx, int dy,
                                       const gfx::Rect& clip_rect,
                                       const gfx::Size& view_size) {
#if !defined(TILED_BACKING_STORE)
  if (!display_)
    return;

  // We only support scrolling in one direction at a time.
  DCHECK(dx == 0 || dy == 0);

  if (dy) {
    // Positive values of |dy| scroll up
    if (abs(dy) < clip_rect.height()) {
      XCopyArea(display_, pixmap_, pixmap_, static_cast<GC>(pixmap_gc_),
                clip_rect.x() /* source x */,
                std::max(clip_rect.y(), clip_rect.y() - dy),
                clip_rect.width(),
                clip_rect.height() - abs(dy),
                clip_rect.x() /* dest x */,
                std::max(clip_rect.y(), clip_rect.y() + dy) /* dest y */);
    }
  } else if (dx) {
    // Positive values of |dx| scroll right
    if (abs(dx) < clip_rect.width()) {
      XCopyArea(display_, pixmap_, pixmap_, static_cast<GC>(pixmap_gc_),
                std::max(clip_rect.x(), clip_rect.x() - dx),
                clip_rect.y() /* source y */,
                clip_rect.width() - abs(dx),
                clip_rect.height(),
                std::max(clip_rect.x(), clip_rect.x() + dx) /* dest x */,
                clip_rect.y() /* dest x */);
    }
  }
#else
  DLOG(INFO) << "BackingStoreX::ScrollBackingStore "
             << dx << " " << dy
             << " " << clip_rect.x() << " " << clip_rect.y()
             << " " << clip_rect.width() << " " << clip_rect.height();

  QRect dirty_rect(clip_rect.x(), clip_rect.y(), clip_rect.width(), clip_rect.height());
  dirty_rect = MapFromContents(dirty_rect);
  dx *= flatScaleByStep(contents_scale_);
  dy *= flatScaleByStep(contents_scale_);

  TileIndex first = GetTileIndexFrom(dirty_rect.topLeft());
  TileIndex last = GetTileIndexFrom(dirty_rect.bottomRight());
  int x_begin = first.x();
  int x_end = last.x();
  int x_step = 1;
  int y_begin = first.y();
  int y_end = last.y();
  int y_step = 1;

  if (dx > 0)
  {
    x_begin = last.x();
    x_end = first.x();
    x_step = -1;
  }

  if (dy > 0)
  {
    y_begin = last.y();
    y_end = first.y();
    y_step = -1;
  }
  
  for (int x = x_begin; x != x_end + x_step; x += x_step)
  {
    for (int y = y_begin; y != y_end + y_step; y += y_step)
    {
      DLOG(INFO) << "Scroll Tile " << x << " " << y;
      
      TileIndex index(x, y);
      // Always use front tiles map
      scoped_refptr<Tile> tile = tiles_map_.value(index);
      if (tile.get())
      {                
        tile->ScrollBackingStore(dx, dy, dirty_rect);

        if (dx > 0)
        {
          TileIndex next(x - 1, y);
          scoped_refptr<Tile> next_tile = tiles_map_.value(next);
          if (next_tile.get())
          {
            QPainter painter(tile->Pixmap());
            QRect target(tile->Pixmap()->rect().x(),
                         tile->Pixmap()->rect().y(),
                         dx,
                         tile->Pixmap()->rect().height());
            QRect source(next_tile->Pixmap()->rect().x() + next_tile->Pixmap()->rect().width() - dx,
                         next_tile->Pixmap()->rect().y(),
                         dx,
                         next_tile->Pixmap()->rect().height());
            painter.drawPixmap(target, *(next_tile->Pixmap()), source);
          }
        } else if (dx < 0)
        {
          TileIndex next(x + 1, y);
          scoped_refptr<Tile> next_tile = tiles_map_.value(next);
          if (next_tile.get())
          {
            QPainter painter(tile->Pixmap());
            QRect target(tile->Pixmap()->rect().x() + tile->Pixmap()->rect().width() + dx,
                         tile->Pixmap()->rect().y(),
                         -dx,
                         tile->Pixmap()->rect().height());
            QRect source(next_tile->Pixmap()->rect().x(),
                         next_tile->Pixmap()->rect().y(),
                         -dx,
                         next_tile->Pixmap()->rect().height());
            painter.drawPixmap(target, *(next_tile->Pixmap()), source);
          }
        }

        if (dy > 0)
        {
          TileIndex next(x, y - 1);
          scoped_refptr<Tile> next_tile = tiles_map_.value(next);
          if (next_tile.get())
          {
            QPainter painter(tile->Pixmap());
            QRect target(tile->Pixmap()->rect().x(),
                         tile->Pixmap()->rect().y(),
                         tile->Pixmap()->rect().width(),
                         dy);
            QRect source(next_tile->Pixmap()->rect().x(),
                         next_tile->Pixmap()->rect().y() + next_tile->Pixmap()->rect().height() - dy,
                         next_tile->Pixmap()->rect().width(),
                         dy);
            painter.drawPixmap(target, *(next_tile->Pixmap()), source);
          }
        } else if (dy < 0)
        {
          TileIndex next(x, y + 1);
          scoped_refptr<Tile> next_tile = tiles_map_.value(next);
          if (next_tile.get())
          {
            QPainter painter(tile->Pixmap());
            QRect target(tile->Pixmap()->rect().x(),
                         tile->Pixmap()->rect().y() + tile->Pixmap()->rect().height() + dy,
                         tile->Pixmap()->rect().width(),
                         -dy);
            QRect source(next_tile->Pixmap()->rect().x(),
                         next_tile->Pixmap()->rect().y(),
                         next_tile->Pixmap()->rect().width(),
                         -dy);
            painter.drawPixmap(target, *(next_tile->Pixmap()), source);
          }
        }
      }
    }
  }

  gfx::Rect grect(dirty_rect.x(), dirty_rect.y(), dirty_rect.width(), dirty_rect.height());
  render_widget_host_->view()->DidBackingStorePaint(grect);
#endif
}

void BackingStoreX::XShowRect(const gfx::Point &origin,
                              const gfx::Rect& rect, XID target) {
#if !defined(TILED_BACKING_STORE)
  XCopyArea(display_, pixmap_, target, static_cast<GC>(pixmap_gc_),
            rect.x(), rect.y(), rect.width(), rect.height(),
            rect.x() + origin.x(), rect.y() + origin.y());
#else
  NOTIMPLEMENTED();
#endif
}

#if !defined(TOOLKIT_MEEGOTOUCH)
void BackingStoreX::CairoShowRect(const gfx::Rect& rect,
                                  GdkDrawable* drawable) {
  cairo_surface_t* surface = cairo_xlib_surface_create(
      display_, pixmap_, static_cast<Visual*>(visual_),
      size().width(), size().height());
  cairo_t* cr = gdk_cairo_create(drawable);
  cairo_set_source_surface(cr, surface, 0, 0);

  cairo_rectangle(cr, rect.x(), rect.y(), rect.width(), rect.height());
  cairo_fill(cr);
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
}
#endif


#if defined(TILED_BACKING_STORE)
// Tile size
const static QSize kTileSize = QSize(512, 512);
// Tile cache multipler 
const static QSizeF kTileCacheMultipler = QSizeF(1.5, 2.5);

static scoped_ptr<QPixmap> g_background_pixmap;

// paint a dummy element when a tile is not ready
static void paintTileBackground(QPainter *painter, QRect &tile, QRect &dirty)
{
  const static int cell = 16;

  if (g_background_pixmap.get() == NULL)
  {
    g_background_pixmap.reset(new QPixmap(cell * 2, cell * 2));

    QPainter pixmap_painter(g_background_pixmap.get());
    for(int i = 0; i * cell < g_background_pixmap->rect().width(); i++) 
      for (int j = 0; j * cell < g_background_pixmap->rect().height(); j++) {
        QRect one(cell * i, cell * j, cell, cell);
        QRect target = one.intersected(g_background_pixmap->rect());
        if ((i + j) % 2) {
          //QBrush brush(Qt::black);
          QBrush brush(Qt::gray);
          pixmap_painter.fillRect(target, brush);
        } else {
          QBrush brush(Qt::white);
          pixmap_painter.fillRect(target, brush);
        }
      }
  }

  QRect target = tile.intersected(dirty);
  painter->drawTiledPixmap(target, *(g_background_pixmap.get()));
  // paint checkers when a tile is not ready
}
#endif

#if defined(TOOLKIT_MEEGOTOUCH)
void BackingStoreX::QPainterShowRect(QPainter *painter, QRectF &rect) {
#if defined(TILED_BACKING_STORE)
  QRect dirty_rect(rect.x(), rect.y(), rect.width(), rect.height());

  DLOG(INFO) << "BackingStoreX::QPainterShowRect "
             << dirty_rect.x() << " " << dirty_rect.y()
             << " " << dirty_rect.width() << " " << dirty_rect.height();
  
  TileIndex first = GetTileIndexFrom(dirty_rect.topLeft());
  TileIndex last = GetTileIndexFrom(dirty_rect.bottomRight());

  for (int x = first.x(); x <= last.x(); x++)
  {
    for (int y = first.y(); y <= last.y(); y++)
    {
      TileIndex index(x, y);
      // Always use front tiles map
      scoped_refptr<Tile> tile = tiles_map_.value(index);
      if (tile.get() && tile->IsReady())
      {
        DLOG(INFO) << "Paint Tile " << x << " " << y;
        tile->QPainterShowRect(painter, dirty_rect);
      }
      else
      {
        DLOG(INFO) << "Paint Checker " << x << " " << y;
        QRect rect(x * kTileSize.width(), y * kTileSize.height(),
                   kTileSize.width(), kTileSize.height());
        paintTileBackground(painter, rect, dirty_rect);
      }
    }
  }

#else
  painter->drawPixmap(rect, QPixmap::fromX11Pixmap(pixmap_), rect);
#endif
}


void BackingStoreX::QPainterShowRect(QPainter *painter, QRectF &paint_rect,
				     QRectF &source) {
#if !defined(TILED_BACKING_STORE)
  painter->drawPixmap(paint_rect, QPixmap::fromX11Pixmap(pixmap_), source);
#else
  NOTIMPLEMENTED();
#endif
}

#endif

#if defined(TOOLKIT_GTK)
void BackingStoreX::PaintToRect(const gfx::Rect& rect, GdkDrawable* target) {
  cairo_surface_t* surface = cairo_xlib_surface_create(
      display_, pixmap_, static_cast<Visual*>(visual_),
      size().width(), size().height());
  cairo_t* cr = gdk_cairo_create(target);

  cairo_translate(cr, rect.x(), rect.y());
  double x_scale = static_cast<double>(rect.width()) / size().width();
  double y_scale = static_cast<double>(rect.height()) / size().height();
  cairo_scale(cr, x_scale, y_scale);

  cairo_pattern_t* pattern = cairo_pattern_create_for_surface(surface);
  cairo_pattern_set_filter(pattern, CAIRO_FILTER_BEST);
  cairo_set_source(cr, pattern);
  cairo_pattern_destroy(pattern);

  cairo_identity_matrix(cr);

  cairo_rectangle(cr, rect.x(), rect.y(), rect.width(), rect.height());
  cairo_fill(cr);
  cairo_destroy(cr);
}
#endif

#if defined(TILED_BACKING_STORE)

BackingStoreX::Tile::Tile(const TileIndex& index, const QRect& rect):
    index_(index),
    rect_(rect),
    ready_(false),
    pixmap_(NULL)
{
  DLOG(INFO) << "Tile create for " << this << " " << "[" << index_.x() << ", " << index_.y() << "]";
  pixmap_ = new QPixmap(kTileSize.width(), kTileSize.height());
}

BackingStoreX::Tile::~Tile()
{
  DLOG(INFO) << "Tile delete for " << this << " " << "[" << index_.x() << ", " << index_.y() << "]";
  delete pixmap_;
}

void BackingStoreX::Tile::QPainterShowRect(QPainter* painter,
                                           const QRect& rect)
{
  DCHECK(pixmap_);

  QRect target = rect.intersected(rect_);
  QRect source((target.x() - rect_.x()),
               (target.y() - rect_.y()),
               target.width(),
               target.height());
    
  painter->drawPixmap(target, *pixmap_, source);
#if defined(TILED_BACKING_STORE_DEBUG)
  QPen pen(QColor("red"));
  painter->save();
  painter->setPen(pen);
  painter->drawRect(rect);
  painter->restore();
  painter->drawRect(rect_);
  QString index_str = QString("(") + QString::number(index_.x()) + QString(", ")
                       + QString::number(index_.y()) + QString(")");
  painter->drawText(rect_, Qt::AlignTop, index_str);
#endif
}
    
void BackingStoreX::Tile::PaintToBackingStore(QPixmap& bitmap,
                                              const QRect& bitmap_rect,
                                              const QRect& rect)
{
  if (!ready_)
  {
    ready_ = true;
  }
  
  DLOG(INFO) << "Tile::update dirty rect " << rect.x()
             << " " << rect.y()
             << " " << rect.width()
             << " " << rect.height();
  
  QRect updated = rect.intersected(rect_);
  QRect source((updated.x() - bitmap_rect.x()),
               (updated.y() - bitmap_rect.y()),
               updated.width(),
               updated.height());
  QRect target((updated.x() - rect_.x()),
               (updated.y() - rect_.y()),
               updated.width(),
               updated.height());
  QPainter painter(pixmap_);
  painter.drawPixmap(target, bitmap, source);
  
  DLOG(INFO) << "Tile::update (" << rect_.x() << "," << rect_.y() << ","
             << rect_.width() << "," << rect_.height() << ")"
             << " target (" << target.x() << "," << target.y() << ","
             << target.width() << "," << target.height() << ")"
             << " source (" << source.x() << "," << source.y() << ","
             << source.width() << "," << source.height() << ")";
}

void BackingStoreX::Tile::ScrollBackingStore(int dx, int dy,
                                             const QRect& clip_rect)
{
  QRect rect = clip_rect & rect_;
  rect = QRect(rect.x() - rect_.x(),
               rect.y() - rect_.y(),
               rect.width(),
               rect.height());
  DLOG(INFO) << "BackingStoreX::Tile::ScrollBackingStore "
             << pixmap_->rect().x() << " " << pixmap_->rect().y() << " " << pixmap_->rect().width() << " " << pixmap_->rect().height()
             << " " << rect.x() << " " << rect.y() << " " << rect.width() << " " << rect.height();
  pixmap_->scroll(dx, dy, rect);
}

QRect BackingStoreX::GetCachedRect()
{
  gfx::Rect grect = render_widget_host_->view()->GetVisibleRect();
  int dx = static_cast<int>(grect.width() * (kTileCacheMultipler.width() - 1.0));
  int dy = static_cast<int>(grect.height() * (kTileCacheMultipler.height() - 1.0));
  QRect cached_rect(grect.x() - dx,
                    grect.y() - dy,
                    grect.width() + 2 * dx,
                    grect.height() + 2 * dy);
  return cached_rect.intersected(ContentsRect());
}

void BackingStoreX::AdjustTiles(bool recreatedAll)
{
  if (frozen_)
    return;

  QRect cached_rect = GetCachedRect();

  DLOG(INFO) << "TiledBackingStore::AdjustTiles cached_rect"
             << " " << cached_rect.x()
             << " " << cached_rect.y()
             << " " << cached_rect.width()
             << " " << cached_rect.height();

  // Drop tiles:
  // 1. out of cached rect due to visble rect changed
  // 2. rect changes due to contents rect changed
  TilesMap::iterator itr = GetWorkingTilesMap().begin();
  while (itr != GetWorkingTilesMap().end())
  {
    if ((!(cached_rect.intersects(itr.value()->Rect())))
        || GetTileRectAt(itr.value()->Index()) != itr.value()->Rect())
    {
      itr = GetWorkingTilesMap().erase(itr);
      continue;
    }
    if (recreatedAll) {
      (*itr)->reset();
    }
    itr++;
  }

  TileIndex first = GetTileIndexFrom(cached_rect.topLeft());
  TileIndex last = GetTileIndexFrom(cached_rect.bottomRight());

  // Notify cached tiles rect to render
  QRect first_tile_rect = GetTileRectAt(first);
  QRect last_tile_rect = GetTileRectAt(last);
  QRect cached_tiles_rect = QRect(first_tile_rect.topLeft(),
                                  last_tile_rect.bottomRight());

  DLOG(INFO) << "TiledBackingStore::AdjustTiles cached_tiles_rect"
             << " " << cached_tiles_rect.x()
             << " " << cached_tiles_rect.y()
             << " " << cached_tiles_rect.width()
             << " " << cached_tiles_rect.height();

  if (cached_tiles_rect_ != cached_tiles_rect)
  {
    cached_tiles_rect_ = cached_tiles_rect;

    QRect mapped = MapToContents(cached_tiles_rect);
    gfx::Rect mapped_tiles_rect(mapped.x(),
                                mapped.y(),
                                mapped.width(),
                                mapped.height());
    gfx::Rect visible_rect = render_widget_host_->view()->GetVisibleRect();
    QRect qrect(visible_rect.x(),
                visible_rect.y(),
                visible_rect.width(),
                visible_rect.height());
    mapped = MapToContents(qrect);
    gfx::Rect mapped_contents_rect(mapped.x(),
                                mapped.y(),
                                mapped.width(),
                                mapped.height());
    
    render_widget_host_->SetVisibleRect(mapped_tiles_rect,
                                        mapped_contents_rect);
  }

  // Create tiles
  QVector<scoped_refptr<Tile> > visible_tiles;
  QVector<scoped_refptr<Tile> > other_tiles;
  DLOG(INFO) << "Cached tiles index " << first.x() << " " << first.y()
             << " " << last.x() << " " << last.y();
  for (int x = first.x(); x <= last.x(); x++)
  {
    for (int y = first.y(); y <= last.y(); y++)
    {
      TileIndex index(x, y);
      scoped_refptr<Tile> tile = GetTileAt(index);
      if (!tile.get())
      {
        tile = CreateTileAt(index);

        gfx::Rect grect = render_widget_host_->view()->GetVisibleRect();
        QRect visible_rect(grect.x(), grect.y(), grect.width(), grect.height());

        if (visible_rect.intersects(tile->Rect()))
        {
          visible_tiles.append(tile);
        }
        else
        {
          other_tiles.append(tile);
        }
      }
    }
  }

  if(!visible_tiles.isEmpty())
  {
    // request to Paint visible tiles first at once
    PaintTilesRequest(visible_tiles);
  }

  if (other_tiles.isEmpty())
    return;
    
  // paint other tiles one by one
  unsigned size = other_tiles.size();
  for (unsigned n = 0; n < size; ++n)
  {
    QVector<scoped_refptr<Tile> > tile;
    tile.append(other_tiles[n]);
    PaintTilesRequest(tile);
  }
}

void BackingStoreX::PaintTilesAck(unsigned int seq,
                                  unsigned int tag,
                                  const QRect& rect,
                                  const QRect& pixmap_rect)
{
  if (pending_scaling_ && seq == tiles_map_seq_)
  {
    //This is the first update for new tiles
    //swap to front
    tiles_map_.clear();
    tiles_map_ = scaling_tiles_map_;
    scaling_tiles_map_.clear();
    pending_scaling_ = false;
    render_widget_host_->view()->DidBackingStoreScale();
  }

  if (seq < tiles_map_seq_)
  {
    //discard old tile paint
    TilePaintRequest request = tiles_paint_map_.value(tag);
    if (request.tiles.size() == 0)
      return;
    delete tiles_paint_map_[tag].dib;
    tiles_paint_map_.remove(tag);
    return;
  }
  
  LOG(INFO) << "TiledBackingStore::paintTileAck " << tag
            << " " << rect.x() << " " << rect.y() << " " << rect.width() << " " << rect.height()
            << " " << pixmap_rect.x() << " " << pixmap_rect.y()
            << " " << pixmap_rect.width() << " " << pixmap_rect.height();

  const TilePaintRequest request = tiles_paint_map_.value(tag);
  if (request.tiles.size() == 0)
    return;

  Pixmap pixmap;

  const int width = rect.width();
  const int height = rect.height();

  TransportDIB* dib = request.dib;
  if (!dib)
    return;


  if (shared_memory_support_ == ui::SHARED_MEMORY_PIXMAP) {
    XShmSegmentInfo shminfo = {0};
    shminfo.shmseg = dib->MapToX(display_);

    // The NULL in the following is the |data| pointer: this is an artifact of
    // Xlib trying to be helpful, rather than just exposing the X protocol. It
    // assumes that we have the shared memory segment mapped into our memory,
    // which we don't, and it's trying to calculate an offset by taking the
    // difference between the |data| pointer and the address of the mapping in
    // |shminfo|. Since both are NULL, the offset will be calculated to be 0,
    // which is correct for us.
    pixmap = XShmCreatePixmap(display_, root_window_, NULL, &shminfo,
                              width, height, 32);
  } else {
    // We don't have shared memory pixmaps.  Fall back to creating a pixmap
    // ourselves and putting an image on it.
    pixmap = XCreatePixmap(display_, root_window_, width, height, 32);
    GC gc = XCreateGC(display_, pixmap, 0, NULL);

    if (shared_memory_support_ == ui::SHARED_MEMORY_PUTIMAGE) {
      const XID shmseg = dib->MapToX(display_);

      XShmSegmentInfo shminfo;
      memset(&shminfo, 0, sizeof(shminfo));
      shminfo.shmseg = shmseg;
      shminfo.shmaddr = static_cast<char*>(dib->memory());

      XImage* image = XShmCreateImage(display_, static_cast<Visual*>(visual_),
                                      32, ZPixmap,
                                      shminfo.shmaddr, &shminfo,
                                      width, height);

      // This code path is important for performance and we have found that
      // different techniques work better on different platforms. See
      // http://code.google.com/p/chromium/issues/detail?id=44124.
      //
      // Checking for ARM is an approximation, but it seems to be a good one so
      // far.
#if defined(ARCH_CPU_ARM_FAMILY)
      for (size_t i = 0; i < copy_rects.size(); i++) {
        const gfx::Rect& copy_rect = copy_rects[i];
        XShmPutImage(display_, pixmap, gc, image,
                     copy_rect.x() - bitmap_rect.x(), /* source x */
                     copy_rect.y() - bitmap_rect.y(), /* source y */
                     copy_rect.x() - bitmap_rect.x(), /* dest x */
                     copy_rect.y() - bitmap_rect.y(), /* dest y */
                     copy_rect.width(), copy_rect.height(),
                     False /* send_event */);
      }
#else
      XShmPutImage(display_, pixmap, gc, image,
                   0, 0 /* source x, y */, 0, 0 /* dest x, y */,
                   width, height, False /* send_event */);
#endif
      XDestroyImage(image);
    } else {  // case SHARED_MEMORY_NONE
      // No shared memory support, we have to copy the bitmap contents
      // to the X server. Xlib wraps the underlying PutImage call
      // behind several layers of functions which try to convert the
      // image into the format which the X server expects. The
      // following values hopefully disable all conversions.
      XImage image;
      memset(&image, 0, sizeof(image));

      image.width = width;
      image.height = height;
      image.depth = 32;
      image.bits_per_pixel = 32;
      image.format = ZPixmap;
      image.byte_order = LSBFirst;
      image.bitmap_unit = 8;
      image.bitmap_bit_order = LSBFirst;
      image.bytes_per_line = width * 4;
      image.red_mask = 0xff;
      image.green_mask = 0xff00;
      image.blue_mask = 0xff0000;
      image.data = static_cast<char*>(dib->memory());

      XPutImage(display_, pixmap, gc, &image,
                0, 0 /* source x, y */, 0, 0 /* dest x, y */,
                width, height);
    }
    XFreeGC(display_, gc);
  }

  QPixmap qpixmap = QPixmap::fromX11Pixmap(pixmap);
  
#if !defined(NDEBUG)
  static int counter = 0;
  QString file_name = QString("/home/meego/tmp/update") + QString::number(counter)
                      + QString(".png");
  //qpixmap.toImage().save(file_name);
  if (counter >= 100)
    counter = 0;
  else
    counter++;
#endif

  for (int i = 0; i < request.tiles.size(); i++)
  {
    request.tiles[i]->PaintToBackingStore(qpixmap, pixmap_rect, rect);
  }
  
  // In the case of shared memory, we wait for the composite to complete so that
  // we are sure that the X server has finished reading from the shared memory
  // segment.
  if (shared_memory_support_ != ui::SHARED_MEMORY_NONE)
    XSync(display_, False);

  delete tiles_paint_map_[tag].dib;
  tiles_paint_map_.remove(tag);
  XFreePixmap(display_, pixmap);

  gfx::Rect grect(rect.x(), rect.y(), rect.width(), rect.height());
  render_widget_host_->view()->DidBackingStorePaint(grect);
}

void BackingStoreX::SetContentsScale(float scale)
{
  contents_scale_ = scale;
  pending_scaling_ = true;
  tiles_map_seq_++;
  AdjustTiles(true);
}

void BackingStoreX::SetFrozen(bool frozen)
{
  if (frozen_ == false && frozen == true)
  {
    AdjustTiles();
  }
  frozen_ = frozen;
}

void BackingStoreX::PaintTilesRequest(QVector<scoped_refptr<Tile> >& tiles)
{
  QRect rect;
  for (unsigned i = 0; i < tiles.size(); i++)
  {
    rect = rect.united(tiles[i]->Rect());
    DLOG(INFO) << "PaintTilesRequest for " << tiles[i]->Index().x() << " " << tiles[i]->Index().y();
  }


  //the top left point of pixmap_rect is in the content coordinate system
  //but the width and height of pixmap_rect is scaled so it's in the browser UI
  //scaled coordinate system. When returning pixmaps from render, the top left 
  //point of pixmap_rect is actually in the browser UI scaled coordinate system.
  //but width and height remain unchanged.
  gfx::Rect pixmap_rect;

  float scale = flatScaleByStep(contents_scale_);
  int floorX = floorByStep((int)(rect.x() / scale));
  int floorY = floorByStep((int)(rect.y() / scale));
  int incX = rect.x()  - floorX * scale;
  int incY = rect.y()  - floorY * scale;
  pixmap_rect.set_x(floorX);
  pixmap_rect.set_y(floorY);
  pixmap_rect.set_width(rect.width() + incX + 2);
  pixmap_rect.set_height(rect.height() + incY + 2);

  TilePaintRequest request;
  request.dib = TransportDIB::Create(pixmap_rect.width() * pixmap_rect.height() * 4, tiles_paint_tag_);
  DCHECK(request.dib);
  
  request.tiles = tiles;

  tiles_paint_map_.insert(tiles_paint_tag_, request);
  gfx::Rect grect(rect.x(), rect.y(), rect.width(), rect.height());

  render_widget_host_->PaintTile(request.dib->handle(),
                                 tiles_map_seq_,
                                 tiles_paint_tag_,
                                 grect,
                                 pixmap_rect);

  tiles_paint_tag_++;
}

BackingStoreX::TilesMap& BackingStoreX::GetWorkingTilesMap()
{
  if (pending_scaling_)
  {
    return scaling_tiles_map_;
  }
  else
  {
    return tiles_map_;
  }
}

scoped_refptr<BackingStoreX::Tile> BackingStoreX::GetTileAt(const TileIndex& index)
{
  return GetWorkingTilesMap().value(index);
}

scoped_refptr<BackingStoreX::Tile> BackingStoreX::CreateTileAt(const TileIndex& index)
{
  GetWorkingTilesMap().insert(index, new Tile(index, GetTileRectAt(index)));
  return GetTileAt(index);
}

void BackingStoreX::DeleteTileAt(const TileIndex& index)
{
  GetWorkingTilesMap().remove(index);
}

QRect BackingStoreX::MapToContents(const QRect& rect)
{
  float flat = flatScaleByStep(contents_scale_);
  return QRectF(rect.x() / flat,
                rect.y() / flat,
                rect.width() / flat,
                rect.height() / flat).toAlignedRect();
}

QRect BackingStoreX::MapFromContents(const QRect& rect)
{
  float flat = flatScaleByStep(contents_scale_);
  return QRectF(rect.x() * flat,
                rect.y() * flat,
                rect.width() * flat,
                rect.height() * flat).toAlignedRect();
}
    
QRect BackingStoreX::ContentsRect()
{
  gfx::Rect grect = gfx::Rect(gfx::Point(), render_widget_host_->view()->GetContentsSize());
  QRect qrect(grect.x(), grect.y(), grect.width(), grect.height());
  return MapFromContents(qrect);

}

QRect BackingStoreX::GetTileRectAt(const BackingStoreX::TileIndex& index)
{
  QRect rect = QRect(index.x() * kTileSize.width(),
                     index.y() * kTileSize.height(),
                     kTileSize.width(),
                     kTileSize.height());
  return rect.intersected(ContentsRect());
}

BackingStoreX::TileIndex BackingStoreX::GetTileIndexFrom(const QPoint& point)
{
  int x = static_cast<int>(point.x() / kTileSize.width());
  int y = static_cast<int>(point.y() / kTileSize.height());
  return TileIndex(x >= 0 ? x : 0,
                   y >= 0 ? y : 0);
}

#endif
