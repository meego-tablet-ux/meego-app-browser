// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/themes/browser_theme_provider.h"

//#include <gdk-pixbuf/gdk-pixbuf.h>

#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "third_party/skia/include/core/SkBitmap.h"

QImage* BrowserThemeProvider::GetPixbufNamed(int id) const {
  return GetPixbufImpl(id, false);
}

QImage* BrowserThemeProvider::GetRTLEnabledPixbufNamed(int id) const {
  return GetPixbufImpl(id, true);
}

QImage* BrowserThemeProvider::GetPixbufImpl(int id, bool rtl_enabled) const {
  DCHECK(CalledOnValidThread());
  DNOTIMPLEMENTED();
  return NULL;
}

void BrowserThemeProvider::FreePlatformCaches() {
  DCHECK(CalledOnValidThread());
  DNOTIMPLEMENTED();
}
