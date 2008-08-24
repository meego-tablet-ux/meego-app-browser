// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Declare a platform-neutral name for this platform's bitmap device class
// that can be used by upper-level classes that just need to pass a reference
// around.

#if defined(OS_WIN)
class BitmapPlatformDeviceWin;
typedef BitmapPlatformDeviceWin BitmapPlatformDevice;
#elif defined(OS_MACOSX)
class BitmapPlatformDeviceMac;
typedef BitmapPlatformDeviceMac BitmapPlatformDevice;
#endif

