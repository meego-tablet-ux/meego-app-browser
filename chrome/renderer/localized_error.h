// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_LOCALIZED_ERROR_H_
#define CHROME_RENDERER_LOCALIZED_ERROR_H_

class DictionaryValue;
class GURL;

namespace WebKit {
struct WebURLError;
}

// Fills |error_strings| with values to be used to build an error page used
// on HTTP errors, like 404 or connection reset.
void GetLocalizedErrorValues(const WebKit::WebURLError& error,
                             DictionaryValue* error_strings);

// Fills |error_strings| with values to be used to build an error page which
// warns against reposting form data. This is special cased because the form
// repost "error page" has no real error associated with it, and doesn't have
// enough strings localized to meaningfully fill the net error template.
void GetFormRepostErrorValues(const GURL& display_url,
                              DictionaryValue* error_strings);

#endif  // CHROME_RENDERER_LOCALIZED_ERROR_H_
