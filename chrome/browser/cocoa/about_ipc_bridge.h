// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_COCOA_ABOUT_IPC_BRIDGE_H_
#define CHROME_BROWSER_COCOA_ABOUT_IPC_BRIDGE_H_

#include "ipc/ipc_logging.h"
#include "ipc/ipc_message_utils.h"

#if defined(IPC_MESSAGE_LOG_ENABLED)

@class AboutIPCController;

// On Windows, the AboutIPCDialog is a views::View.  On Mac we have a
// Cocoa dialog.  This class bridges from C++ to ObjC.
class AboutIPCBridge : public IPC::Logging::Consumer {
 public:
  AboutIPCBridge(AboutIPCController* controller) : controller_(controller) { }
  virtual ~AboutIPCBridge() { }

  // IPC::Logging::Consumer implementation.
  virtual void Log(const IPC::LogData& data);

 private:
  AboutIPCController* controller_;  // weak; owns me
  DISALLOW_COPY_AND_ASSIGN(AboutIPCBridge);
};

#endif  // IPC_MESSAGE_LOG_ENABLED

#endif  // CHROME_BROWSER_COCOA_ABOUT_IPC_BRIDGE_H_
