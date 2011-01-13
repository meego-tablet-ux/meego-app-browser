// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "views/event.h"

#include <gdk/gdk.h>

#include "ui/base/keycodes/keyboard_code_conversion_gtk.h"

namespace views {

KeyEvent::KeyEvent(const GdkEventKey* event)
    : Event(event->type == GDK_KEY_PRESS ?
            Event::ET_KEY_PRESSED : Event::ET_KEY_RELEASED,
            GetFlagsFromGdkState(event->state)),
      // TODO(erg): All these values are iffy.
      key_code_(ui::WindowsKeyCodeForGdkKeyCode(event->keyval)),
      repeat_count_(0),
      message_flags_(0)
#if !defined(TOUCH_UI)
      , native_event_(event)
#endif
{
}

// static
int Event::GetFlagsFromGdkState(int state) {
  int flags = 0;
  if (state & GDK_LOCK_MASK)
    flags |= Event::EF_CAPS_LOCK_DOWN;
  if (state & GDK_CONTROL_MASK)
    flags |= Event::EF_CONTROL_DOWN;
  if (state & GDK_SHIFT_MASK)
    flags |= Event::EF_SHIFT_DOWN;
  if (state & GDK_MOD1_MASK)
    flags |= Event::EF_ALT_DOWN;
  if (state & GDK_BUTTON1_MASK)
    flags |= Event::EF_LEFT_BUTTON_DOWN;
  if (state & GDK_BUTTON2_MASK)
    flags |= Event::EF_MIDDLE_BUTTON_DOWN;
  if (state & GDK_BUTTON3_MASK)
    flags |= Event::EF_RIGHT_BUTTON_DOWN;
  return flags;
}

}  // namespace views
