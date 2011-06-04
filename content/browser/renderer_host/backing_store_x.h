// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_BACKING_STORE_X_H_
#define CONTENT_BROWSER_RENDERER_HOST_BACKING_STORE_X_H_
#pragma once

#if defined(TOOLKIT_MEEGOTOUCH)
#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <QHash>
#include <QPair>
#endif

#include "base/basictypes.h"
#include "build/build_config.h"
#include "base/ref_counted.h"
#include "base/scoped_ptr.h"
#include "content/browser/renderer_host/backing_store.h"
#include "ui/base/x/x11_util.h"

namespace gfx {
class Point;
class Rect;
}  // namespace gfx

typedef struct _GdkDrawable GdkDrawable;
class SkBitmap;

#define TILED_BACKING_STORE

class BackingStoreX : public BackingStore {
 public:
  // Create a backing store on the X server. The visual is an Xlib Visual
  // describing the format of the target window and the depth is the color
  // depth of the X window which will be drawn into.
  BackingStoreX(RenderWidgetHost* widget,
               const gfx::Size& size,
               void* visual,
               int depth);

  // This is for unittesting only. An object constructed using this constructor
  // will silently ignore all paints
  BackingStoreX(RenderWidgetHost* widget, const gfx::Size& size);

  virtual ~BackingStoreX();

  Display* display() const { return display_; }
  XID root_window() const { return root_window_; }

  // Copy from the server-side backing store to the target window
  //   origin: the destination rectangle origin
  //   damage: the area to copy
  //   target: the X id of the target window
  void XShowRect(const gfx::Point &origin, const gfx::Rect& damage,
                 XID target);

  // As above, but use Cairo instead of Xlib.
  void CairoShowRect(const gfx::Rect& damage, GdkDrawable* drawable);

#if defined(TOOLKIT_MEEGOTOUCH)
  void QPainterShowRect(QPainter *painter, QRectF &paint_rect);
  void QPainterShowRect(QPainter *painter, QRectF &paint_rect,
			QRectF &source);
#endif

#if defined(TOOLKIT_GTK)
  // Paint the backing store into the target's |dest_rect|.
  void PaintToRect(const gfx::Rect& dest_rect, GdkDrawable* target);
#endif

#if defined(TILED_BACKING_STORE)
  // Tiled backing store public APIs
  
  // Adjust tiles according to visible rect, contents size or scale change
  // if least_request is true, just create tiles and send request for those are
  // not all contained in update_rect.
  void AdjustTiles(bool recreate_all = false, 
                   bool least_request = false,
                   const gfx::Rect &update_rect = gfx::Rect(0, 0, 0, 0));

  // Handle tiles painting tiles ack from render
  void PaintTilesAck(unsigned int seq, unsigned int tag, const QRect& rect, const QRect& pixmap_rect);

  // Set content scale
  void SetContentsScale(float);

  // Set frozen
  void SetFrozen(bool);

  // Mapped contents rect
  QRect ContentsRect();
#endif
  
  // BackingStore implementation.
  virtual size_t MemorySize();
  virtual void PaintToBackingStore(
      RenderProcessHost* process,
      TransportDIB::Id bitmap,
      const gfx::Rect& bitmap_rect,
      const std::vector<gfx::Rect>& copy_rects,
      unsigned int seq);
  virtual bool CopyFromBackingStore(const gfx::Rect& rect,
                                    skia::PlatformCanvas* output);
  virtual void ScrollBackingStore(int dx, int dy,
                                  const gfx::Rect& clip_rect,
                                  const gfx::Size& view_size);

 private:
  // Paints the bitmap from the renderer onto the backing store without
  // using Xrender to composite the pixmaps.
  void PaintRectWithoutXrender(TransportDIB* bitmap,
                               const gfx::Rect& bitmap_rect,
                               const std::vector<gfx::Rect>& copy_rects);

  // This is the connection to the X server where this backing store will be
  // displayed.
  Display* const display_;
  // What flavor, if any, MIT-SHM (X shared memory) support we have.
  const ui::SharedMemorySupport shared_memory_support_;
  // If this is true, then we can use Xrender to composite our pixmaps.
  const bool use_render_;
  // If |use_render_| is false, this is the number of bits-per-pixel for |depth|
  int pixmap_bpp_;
  // if |use_render_| is false, we need the Visual to get the RGB masks.
  void* const visual_;
  // This is the depth of the target window.
  const int visual_depth_;
  // The parent window (probably a GtkDrawingArea) for this backing store.
  const XID root_window_;
  // This is a handle to the server side pixmap which is our backing store.
  XID pixmap_;
  // This is the RENDER picture pointing at |pixmap_|.
  XID picture_;
  // This is a default graphic context, used in XCopyArea
  void* pixmap_gc_;

#if defined(TILED_BACKING_STORE)
private:
  class TileIndex : public QPair<int, int>
  {
   public:
    TileIndex(int x, int y) : QPair<int, int>(x, y) {}
    int x() const { return first; }
    int y() const { return second; }
  };
  
  class Tile : public base::RefCounted<Tile>
  {
   public:
    Tile(const TileIndex& index, const QRect& rect);
    ~Tile();

    // service Qt paint request
    void QPainterShowRect(QPainter* painter,
                          const QRect& rect);
    
    // update tile from shared bitmap
    void PaintToBackingStore(QPixmap& bitmap,
                             const QRect& bitmap_rect,
                             const QRect& rect);

    // support fast path scroll
    void ScrollBackingStore(int dx, int dy,
                            const QRect& clip_rect);

    TileIndex Index() { return index_; }
    
    QRect Rect() { return rect_; }

    bool IsReady() { return ready_; }

    void reset() { ready_ = false; }

    QPixmap* Pixmap() {return pixmap_;}
    
   private:
    friend class base::RefCounted<Tile>;

    TileIndex index_;
    QRect rect_;
    QPixmap* pixmap_;
    bool ready_;
  };

private:
  typedef QHash<TileIndex, scoped_refptr<Tile> > TilesMap;
  TilesMap tiles_map_;
  TilesMap scaling_tiles_map_;
  unsigned int tiles_map_seq_;
  
  struct TilePaintRequest {
    TransportDIB* dib;
    QVector<scoped_refptr<Tile> > tiles;
  };
  typedef QHash<unsigned int, TilePaintRequest > TilePaintMap;
  TilePaintMap tiles_paint_map_;
  unsigned int tiles_paint_tag_;

  float contents_scale_;

  QRect cached_tiles_rect_;
  gfx::Rect visible_rect_;

  bool pending_scaling_;

  bool frozen_;
  
private:
  friend class Tile;
  
  // Send tile painting request to render
  void PaintTilesRequest(QVector<scoped_refptr<Tile> >& tiles);

  TilesMap& GetWorkingTilesMap();
  scoped_refptr<Tile> GetTileAt(const TileIndex& pos);
  scoped_refptr<Tile> CreateTileAt(const TileIndex& pos);
  void DeleteTileAt(const TileIndex& pos);

  QRect MapToContents(const QRect&);
  QRect MapFromContents(const QRect&);

  QRect GetTileRectAt(const TileIndex& index);
  TileIndex GetTileIndexFrom(const QPoint& point);

  QRect GetCachedRect();
#endif

  DISALLOW_COPY_AND_ASSIGN(BackingStoreX);
};

#endif  // CONTENT_BROWSER_RENDERER_HOST_BACKING_STORE_X_H_
