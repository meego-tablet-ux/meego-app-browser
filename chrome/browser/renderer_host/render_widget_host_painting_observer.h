// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_PAINTING_OBSERVER_H_
#define CHROME_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_PAINTING_OBSERVER_H_
#pragma once

class BackingStore;
class RenderWidgetHost;

namespace gfx {
class Size;
}

// This class can be used to observe painting events for a RenderWidgetHost.
// Its primary goal in Chrome is to allow thumbnails to be generated.
class RenderWidgetHostPaintingObserver {
 public:
  // Indicates the RenderWidgetHost is about to destroy the backing store. The
  // backing store will still be valid when this call is made.
  virtual void WidgetWillDestroyBackingStore(RenderWidgetHost* widget,
                                             BackingStore* backing_store) = 0;

  // Indicates that the RenderWidgetHost just updated the backing store.
  virtual void WidgetDidUpdateBackingStore(RenderWidgetHost* widget) = 0;

  // This notifies the painting observer that a PaintAtSizeACK was
  // received.
  virtual void WidgetDidReceivePaintAtSizeAck(
      RenderWidgetHost* widget,
      int tag,
      const gfx::Size& size) = 0;

 protected:
  virtual ~RenderWidgetHostPaintingObserver() {}
};

#endif  // CHROME_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_PAINTING_OBSERVER_H_
