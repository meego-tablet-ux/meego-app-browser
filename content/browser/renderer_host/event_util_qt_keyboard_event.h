// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RENDERER_HOST_EVENT_UTIL_QT_KEYBOARD_EVENT_H_
#define CHROME_BROWSER_RENDERER_HOST_EVENT_UTIL_QT_KEYBOARD_EVENT_H_

class QString;
QString keyIdentifierForQtKeyCode(int keyCode);
int windowsKeyCodeForQKeyEvent(unsigned int keycode, bool isKeypad = false);

#endif  // CHROME_BROWSER_RENDERER_HOST_EVENT_UTIL_QT_KEYBOARD_EVENT_H_

