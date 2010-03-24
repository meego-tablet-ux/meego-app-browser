// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_COCOA_STATUS_ICONS_STATUS_TRAY_MAC_H_
#define CHROME_BROWSER_COCOA_STATUS_ICONS_STATUS_TRAY_MAC_H_

#include "chrome/browser/status_icons/status_tray.h"

class StatusTrayMac : public StatusTray {
 public:
  StatusTrayMac();

 protected:
  // Factory method for creating a status icon.
  virtual StatusIcon* CreateStatusIcon();

 private:
  DISALLOW_COPY_AND_ASSIGN(StatusTrayMac);
};

#endif // CHROME_BROWSER_COCOA_STATUS_ICONS_STATUS_TRAY_MAC_H_

