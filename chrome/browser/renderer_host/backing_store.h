// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_HOST_BACKING_STORE_H_
#define CHROME_BROWSER_RENDERER_HOST_BACKING_STORE_H_

#include "base/basictypes.h"
#include "base/gfx/rect.h"
#include "base/gfx/size.h"
#include "base/process.h"
#include "build/build_config.h"
#include "chrome/common/mru_cache.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

class RenderWidgetHost;

// BackingStore ----------------------------------------------------------------

// Represents a backing store for the pixels in a RenderWidgetHost.
class BackingStore {
 public:
  explicit BackingStore(const gfx::Size& size);
  ~BackingStore();

  const gfx::Size& size() { return size_; }

#if defined(OS_WIN)
  HDC hdc() { return hdc_; }
#endif

  // Paints the bitmap from the renderer onto the backing store.
  // TODO(port): The HANDLE is a shared section on Windows. Abstract this.
  bool PaintRect(base::ProcessHandle process,
                 HANDLE bitmap_section,
                 const gfx::Rect& bitmap_rect);

  // Scrolls the given rect in the backing store, replacing the given region
  // identified by |bitmap_rect| by the bitmap in the file identified by the
  // given file handle.
  // TODO(port): The HANDLE is a shared section on Windows. Abstract this.
  void ScrollRect(base::ProcessHandle process,
                  HANDLE bitmap, const gfx::Rect& bitmap_rect,
                  int dx, int dy,
                  const gfx::Rect& clip_rect,
                  const gfx::Size& view_size);

 private:
  // The size of the backing store.
  gfx::Size size_;

#if defined(OS_WIN)
  // Creates a dib conforming to the height/width/section parameters passed
  // in. The use_os_color_depth parameter controls whether we use the color
  // depth to create an appropriate dib or not.
  HANDLE CreateDIB(HDC dc,
                   int width, int height,
                   bool use_os_color_depth,
                   HANDLE section);

  // The backing store dc.
  HDC hdc_;

  // Handle to the backing store dib.
  HANDLE backing_store_dib_;

  // Handle to the original bitmap in the dc.
  HANDLE original_bitmap_;
#endif

  DISALLOW_COPY_AND_ASSIGN(BackingStore);
};

// BackingStoreManager ---------------------------------------------------------

// This class manages backing stores in the browsr. Every RenderWidgetHost is
// associated with a backing store which it requests from this class.  The
// hosts don't maintain any references to the backing stores.  These backing
// stores are maintained in a cache which can be trimmed as needed.
class BackingStoreManager {
 public:
  // Returns a backing store which matches the desired dimensions.
  //
  // backing_store_rect
  //   The desired backing store dimensions.
  // Returns a pointer to the backing store on success, NULL on failure.
  static BackingStore* GetBackingStore(RenderWidgetHost* host,
                                       const gfx::Size& desired_size);

  // Returns a backing store which is fully ready for consumption, i.e. the
  // bitmap from the renderer has been copied into the backing store dc, or the
  // bitmap in the backing store dc references the renderer bitmap.
  //
  // backing_store_rect
  //   The desired backing store dimensions.
  // process_handle
  //   The renderer process handle.
  // bitmap_section
  //   The bitmap section from the renderer.
  // bitmap_rect
  //   The rect to be painted into the backing store
  // needs_full_paint
  //   Set if we need to send out a request to paint the view
  //   to the renderer.
  // TODO(port): The HANDLE is a shared section on Windows. Abstract this.
  static BackingStore* PrepareBackingStore(RenderWidgetHost* host,
                                           const gfx::Rect& backing_store_rect,
                                           base::ProcessHandle process_handle,
                                           HANDLE bitmap_section,
                                           const gfx::Rect& bitmap_rect,
                                           bool* needs_full_paint);

  // Returns a matching backing store for the host.
  // Returns NULL if we fail to find one.
  static BackingStore* Lookup(RenderWidgetHost* host);

  // Removes the backing store for the host.
  static void RemoveBackingStore(RenderWidgetHost* host);

 private:
  // Not intended for instantiation.
  BackingStoreManager() {}

  DISALLOW_COPY_AND_ASSIGN(BackingStoreManager);
};

#endif  // CHROME_BROWSER_RENDERER_HOST_BACKING_STORE_H_
