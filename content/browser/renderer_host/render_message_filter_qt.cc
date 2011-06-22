// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/render_message_filter.h"

#include <fcntl.h>
#include <map>

#include <QRectF>
#include <QGraphicsWidget>

#include "base/file_util.h"
#include "base/lazy_instance.h"
#include "base/path_service.h"
#include "content/browser/browser_thread.h"
#include "chrome/browser/printing/print_dialog_cloud.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/render_messages.h"
#include "ui/gfx/native_widget_types.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebScreenInfo.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/x11/WebScreenInfoFactory.h"
#include "ui/base/clipboard/clipboard.h"
#include "ui/base/x/x11_util.h"

using WebKit::WebScreenInfo;
using WebKit::WebScreenInfoFactory;

namespace {

typedef std::map<int, FilePath> SequenceToPathMap;

struct PrintingSequencePathMap {
  SequenceToPathMap map;
  int sequence;
};

// No locking, only access on the FILE thread.
static base::LazyInstance<PrintingSequencePathMap>
    g_printing_file_descriptor_map(base::LINKER_INITIALIZED);

}  // namespace

// We get null window_ids passed into the two functions below; please see
// http://crbug.com/9060 for more details.

void RenderMessageFilter::OnGetScreenInfo(gfx::NativeViewId view,
                                            WebKit::WebScreenInfo* results) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::BACKGROUND_X11));
  Display* display = ui::GetSecondaryDisplay();
  int screen = ui::GetDefaultScreen(display);
  *results = WebScreenInfoFactory::screenInfo(display, screen);
}

void RenderMessageFilter::OnGetWindowRect(gfx::NativeViewId view,
                                            gfx::Rect* rect) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::BACKGROUND_X11));
  gfx::NativeView navite_view = gfx::NativeViewFromId(view);
  QRectF br = navite_view->boundingRect();
  *rect = gfx::Rect(static_cast<int>(br.x()), static_cast<int>(br.y()),
       static_cast<int>(br.width()), static_cast<int>(br.height()));

}

// Return the top-level parent of the given window. Called on the
// BACKGROUND_X11 thread.
static XID GetTopLevelWindow(XID window) {
  bool parent_is_root;
  XID parent_window;

  if (!ui::GetWindowParent(&parent_window, &parent_is_root, window))
    return 0;
  if (parent_is_root)
    return window;

  return GetTopLevelWindow(parent_window);
}

void RenderMessageFilter::OnGetRootWindowRect(gfx::NativeViewId view,
                                                gfx::Rect* rect) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::BACKGROUND_X11));
  const XID toplevel = ui::GetX11RootWindow();
  int x, y;
  unsigned width, height;

  if (ui::GetWindowGeometry(&x, &y, &width, &height, toplevel))
    *rect = gfx::Rect(x, y, width, height);
  else
    *rect = gfx::Rect();
}
