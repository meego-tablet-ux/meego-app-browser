// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_KEYCODES_KEYBOARD_CODE_CONVERSION_WIN_H_
#define UI_BASE_KEYCODES_KEYBOARD_CODE_CONVERSION_WIN_H_
#pragma once

#include "ui/base/keycodes/keyboard_codes.h"

namespace ui {

// Methods to convert ui::KeyboardCode/Windows virtual key type methods.
WORD WindowsKeyCodeForKeyboardCode(KeyboardCode keycode);
KeyboardCode KeyboardCodeForWindowsKeyCode(WORD keycode);

}  // namespace ui

#endif  // UI_BASE_KEYCODES_KEYBOARD_CODE_CONVERSION_WIN_H_
