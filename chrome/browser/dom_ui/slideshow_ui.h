// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOM_UI_SLIDESHOW_UI_H_
#define CHROME_BROWSER_DOM_UI_SLIDESHOW_UI_H_
#pragma once

#include <vector>

#include "base/file_path.h"
#include "base/scoped_ptr.h"
#include "base/values.h"
#include "chrome/browser/dom_ui/dom_ui.h"

class SlideshowUI : public DOMUI {
 public:
  explicit SlideshowUI(TabContents* contents);

 private:
  DISALLOW_COPY_AND_ASSIGN(SlideshowUI);
};

#endif  // CHROME_BROWSER_DOM_UI_SLIDESHOW_UI_H_
