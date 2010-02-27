// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_PLATFORM_UTIL_H_
#define CHROME_COMMON_PLATFORM_UTIL_H_

#include "app/gfx/native_widget_types.h"
#include "base/string16.h"

class FilePath;
class GURL;

namespace platform_util {

// Show the given file in a file manager. If possible, select the file.
void ShowItemInFolder(const FilePath& full_path);

// Open the given file in the desktop's default manner.
void OpenItem(const FilePath& full_path);

// Open the given external protocol URL in the desktop's default manner.
// (For example, mailto: URLs in the default mail user agent.)
void OpenExternal(const GURL& url);

// Get the top level window for the native view. This can return NULL.
gfx::NativeWindow GetTopLevel(gfx::NativeView view);

// Returns true if |window| is the foreground top level window.
bool IsWindowActive(gfx::NativeWindow window);

// Returns true if the view is visible. The exact definition of this is
// platform-specific, but it is generally not "visible to the user", rather
// whether the view has the visible attribute set.
bool IsVisible(gfx::NativeView view);

// Pops up an error box with an OK button. If |parent| is non-null, the box
// will be modal on it. (On Mac, it is always app-modal.) Generally speaking,
// this class should not be used for much. Infobars are preferred.
void SimpleErrorBox(gfx::NativeWindow parent,
                    const string16& title,
                    const string16& message);

// Return a human readable modifier for the version string.  For a
// branded Chrome (not Chromium), this modifier is the channel (dev,
// beta, stable).
string16 GetVersionStringModifier();

}

#endif  // CHROME_COMMON_PLATFORM_UTIL_H_
