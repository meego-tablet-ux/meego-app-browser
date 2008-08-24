// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/views/layout_manager.h"

#include "chrome/views/view.h"

namespace ChromeViews {

int LayoutManager::GetPreferredHeightForWidth(View* host, int width) {
  CSize pref;
  GetPreferredSize(host, &pref);
  return pref.cy;
}

}  // namespace

