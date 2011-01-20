// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_ABOUT_IPC_DIALOG_H_
#define CHROME_BROWSER_UI_COCOA_ABOUT_IPC_DIALOG_H_
#pragma once

#include "ipc/ipc_logging.h"
#include "ipc/ipc_message_utils.h"

#if defined(IPC_MESSAGE_LOG_ENABLED)

#ifdef __OBJC__
@class AboutIPCController;
#else
class AboutIPCController;
#endif

namespace AboutIPCDialog {
// The dialog is a singleton. If the dialog is already opened, it won't do
// anything, so you can just blindly call this function all you want.
// RunDialog() is Called from chrome/browser/browser_about_handler.cc
// in response to an about:ipc URL.
void RunDialog();
}

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

#endif  // CHROME_BROWSER_UI_COCOA_ABOUT_IPC_DIALOG_H_
