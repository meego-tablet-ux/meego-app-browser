// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TAB_CONTENTS_WEB_DRAG_UTILS_WIN_H_
#define CHROME_BROWSER_TAB_CONTENTS_WEB_DRAG_UTILS_WIN_H_

#include "third_party/WebKit/WebKit/chromium/public/WebDragOperation.h"

#include <windows.h>

namespace web_drag_utils_win {

WebKit::WebDragOperationsMask WinDragOpToWebDragOp(DWORD effect);
DWORD WebDragOpToWinDragOp(WebKit::WebDragOperationsMask op);

}  // namespace web_drag_utils_win

#endif  // CHROME_BROWSER_TAB_CONTENTS_WEB_DRAG_UTILS_WIN_H_

