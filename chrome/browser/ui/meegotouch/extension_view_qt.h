// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_QT_EXTENSION_VIEW_QT_H_
#define CHROME_BROWSER_QT_EXTENSION_VIEW_QT_H_

#include "base/basictypes.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/size.h"
#include "third_party/skia/include/core/SkBitmap.h"

class Browser;
class ExtensionHost;
class RenderViewHost;
class RenderWidgetHostViewQt;
class SkBitmap;

class ExtensionViewQt {
 public:
  ExtensionViewQt(ExtensionHost* extension_host, Browser* browser);

  void Init();

  gfx::NativeView native_view() { return NULL;}
  
  Browser* browser() const { return browser_; }

  void SetBackground(const SkBitmap& background);

  // Method for the ExtensionHost to notify us about the correct size for
  // extension contents.
  void UpdatePreferredSize(const gfx::Size& new_size) {}

  // Method for the ExtensionHost to notify us when the RenderViewHost has a
  // connection.
  void RenderViewCreated() {}

  RenderViewHost* render_view_host() const { return NULL;}

  // Declared here for testing.
  static const int kMinWidth;
  static const int kMinHeight;
  static const int kMaxWidth;
  static const int kMaxHeight;

 private:
  void CreateWidgetHostView();

  Browser* browser_;

  ExtensionHost* extension_host_;

  RenderWidgetHostViewQt* render_widget_host_view_;

  // The background the view should have once it is initialized. This is set
  // when the view has a custom background, but hasn't been initialized yet.
  SkBitmap pending_background_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionViewQt);
};

#endif  // CHROME_BROWSER_QT_EXTENSION_VIEW_QT_H_
