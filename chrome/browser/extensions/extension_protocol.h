// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_PROTOCOL_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_PROTOCOL_H_

#include "base/file_path.h"

// Gets a FilePath for a resource inside an extension. |extension_path| is the
// full path to the extension directory. |resource_path| is the path to the
// resource from the extension root, including the leading '/'.
FilePath GetPathForExtensionResource(const FilePath& extension_path,
                                     const std::string& resource_path);

// Registers support for the extension URL scheme.
void RegisterExtensionProtocols();

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_PROTOCOL_H_
