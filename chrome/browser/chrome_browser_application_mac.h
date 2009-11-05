// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROME_BROWSER_APPLICATION_MAC_H_
#define CHROME_BROWSER_CHROME_BROWSER_APPLICATION_MAC_H_

#ifdef __OBJC__

#import "base/chrome_application_mac.h"

@interface BrowserCrApplication : CrApplication
@end

namespace chrome_browser_application_mac {

// Bin for unknown exceptions. Exposed for testing purposes.
extern const size_t kUnknownNSException;

// Returns the histogram bin for |exception| if it is one we track
// specifically, or |kUnknownNSException| if unknown.  Exposed for testing
// purposes.
size_t BinForException(NSException* exception);

// Use UMA to track exception occurance. Exposed for testing purposes.
void RecordExceptionWithUma(NSException* exception);

}  // namespace chrome_browser_application_mac

#endif  // __OBJC__

namespace chrome_browser_application_mac {

// Calls -[NSApp terminate:].
void Terminate();

}  // namespace chrome_browser_application_mac

#endif  // CHROME_BROWSER_CHROME_BROWSER_APPLICATION_MAC_H_
