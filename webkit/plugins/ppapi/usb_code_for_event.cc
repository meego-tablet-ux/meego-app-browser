// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/plugins/ppapi/usb_code_for_event.h"

#include "third_party/WebKit/Source/WebKit/chromium/public/WebInputEvent.h"

using WebKit::WebKeyboardEvent;

namespace webkit {
namespace ppapi {

namespace {

// TODO(wez): This table covers only USB HID Boot Protocol.  It should be
// extended (e.g. with "media keys"), or derived automatically from the
// evdev USB-to-linux keycode mapping.
uint32_t linux_key_code_to_usb[256] = {
  // 0x00-0x0f
  0x000000, 0x070029, 0x07001e, 0x07001f,
  0x070020, 0x070021, 0x070022, 0x070023,
  0x070024, 0x070025, 0x070026, 0x070027,
  0x07002d, 0x07002e, 0x07002a, 0x07002b,
  // 0x10-0x1f
  0x070014, 0x07001a, 0x070008, 0x070015,
  0x070017, 0x07001c, 0x070018, 0x07000c,
  0x070012, 0x070013, 0x07002f, 0x070030,
  0x070028, 0x0700e0, 0x070004, 0x070016,
  // 0x20-0x2f
  0x070007, 0x070009, 0x07000a, 0x07000b,
  0x07000d, 0x07000e, 0x07000f, 0x070033,
  0x070034, 0x070035, 0x0700e1, 0x070032,
  0x07001d, 0x07001b, 0x070006, 0x070019,
  // 0x30-0x3f
  0x070005, 0x070011, 0x070010, 0x070036,
  0x070037, 0x070038, 0x0700e5, 0x070055,
  0x0700e2, 0x07002c, 0x070039, 0x07003a,
  0x07003b, 0x07003c, 0x07003d, 0x07003e,
  // 0x40-0x4f
  0x07003f, 0x070040, 0x070041, 0x070042,
  0x070043, 0x070053, 0x070047, 0x07005f,
  0x070060, 0x070061, 0x070056, 0x07005c,
  0x07005d, 0x07005e, 0x070057, 0x070059,
  // 0x50-0x5f
  0x07005a, 0x07005b, 0x070062, 0x070063,
  0x000000, 0x070094, 0x070064, 0x070044,
  0x070045, 0x070087, 0x070092, 0x070093,
  0x07008a, 0x070088, 0x07008b, 0x07008c,
  // 0x60-0x6f
  0x070058, 0x0700e4, 0x070054, 0x070046,
  0x0700e6, 0x000000, 0x07004a, 0x070052,
  0x07004b, 0x070050, 0x07004f, 0x07004d,
  0x070051, 0x07004e, 0x070049, 0x07004c,
  // 0x70-0x7f
  0x000000, 0x0700ef, 0x0700ee, 0x0700ed,
  0x070066, 0x070067, 0x000000, 0x070048,
  0x000000, 0x070085, 0x070090, 0x070091,
  0x070089, 0x0700e3, 0x0700e7, 0x070065,
  // 0x80-0x8f
  0x0700f3, 0x070079, 0x070076, 0x07007a,
  0x070077, 0x07007c, 0x070074, 0x07007d,
  0x0700f4, 0x07007b, 0x070075, 0x000000,
  0x0700fb, 0x000000, 0x0700f8, 0x000000,
  // 0x90-0x9f
  0x000000, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x0700f0, 0x000000,
  0x0700f9, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x0700f1, 0x0700f2,
  // 0xa0-0xaf
  0x000000, 0x0700ec, 0x000000, 0x0700eb,
  0x0700e8, 0x0700ea, 0x0700e9, 0x000000,
  0x000000, 0x000000, 0x000000, 0x000000,
  0x000000, 0x0700fa, 0x000000, 0x000000,
  // 0xb0-0xbf
  0x0700f7, 0x0700f5, 0x0700f6, 0x000000,
  0x000000, 0x000000, 0x000000, 0x070068,
  0x070069, 0x07006a, 0x07006b, 0x07006c,
  0x07006d, 0x07006e, 0x07006f, 0x070070,
  // 0xc0-0xcf
  0x070071, 0x070072, 0x070073, 0x000000,
  0x000000, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x000000, 0x000000,
  // 0xd0-0xdf
  0x000000, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x000000, 0x000000,
  // 0xe0-0xef
  0x000000, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x000000, 0x000000,
  // 0xf0-0xff
  0x000000, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x000000, 0x000000,
  0x000000, 0x000000, 0x000000, 0x000000,
};

uint32_t UsbCodeForLinuxKeyboardEvent(const WebKeyboardEvent& key_event) {
  if (key_event.nativeKeyCode < 0 || key_event.nativeKeyCode > 255)
    return 0;
  return linux_key_code_to_usb[key_event.nativeKeyCode];
}

uint32_t UsbCodeForX11EvdevKeyboardEvent(const WebKeyboardEvent& key_event) {
  return UsbCodeForLinuxKeyboardEvent(key_event.nativeKeyCode - 8);
}

} // anonymous namespace

uint32_t UsbCodeForKeyboardEvent(const WebKeyboardEvent& key_event) {
#if defined(OS_LINUX)
  // TODO(wez): This code assumes that on Linux we're receiving events via
  // the Xorg "evdev" driver.  We should detect "XKB" or "kbd" at run-time and
  // re-map accordingly, but that's not possible here, inside the sandbox.
  return UsbCodeForX11EvdevKeyboardEvent(key_event);
#else
  return 0;
#endif
}

}  // namespace ppapi
}  // namespace webkit
