// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_CONVERT_USER_SCRIPT_H_
#define CHROME_BROWSER_EXTENSIONS_CONVERT_USER_SCRIPT_H_

#include <string>

class Extension;
class FilePath;
class GURL;

// Wraps the specified user script in an extension. The extension is created
// unpacked in the system temp dir. Returns a valid extension that the caller
// should take ownership on success, or NULL and |error| on failure.
//
// NOTE: This function does file IO and should not be called on the UI thread.
Extension* ConvertUserScriptToExtension(const FilePath& user_script,
                                        const GURL& original_url,
                                        std::string* error);

#endif  // CHROME_BROWSER_EXTENSIONS_CONVERT_USER_SCRIPT_H_
