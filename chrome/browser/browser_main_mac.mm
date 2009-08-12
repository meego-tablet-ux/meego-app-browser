// Copyright (c) 2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>
#include "app/resource_bundle.h"
#include "base/command_line.h"
#import "chrome/app/keystone_glue.h"
#include "chrome/browser/browser_main_win.h"
#include "chrome/common/main_function_params.h"
#include "chrome/common/result_codes.h"

namespace Platform {

// Perform any platform-specific work that needs to be done before the main
// message loop is created and initialized.
//
// For Mac, this involves telling Cooca to finish its initalization, which we
// want to do manually instead of calling NSApplicationMain(). The primary
// reason is that NSAM() never returns, which would leave all the objects
// currently on the stack in scoped_ptrs hanging and never cleaned up. We then
// load the main nib directly. The main event loop is run from common code using
// the MessageLoop API, which works out ok for us because it's a wrapper around
// CFRunLoop.
void WillInitializeMainMessageLoop(const MainFunctionParams& parameters) {
  [NSApplication sharedApplication];
  // Before we load the nib, we need to start up the resource bundle so we have
  // the strings avaiable for localization.
  if (!parameters.ui_task) {
    ResourceBundle::InitSharedInstance(std::wstring());
    // We only load the theme resources in the browser process, since this is
    // the browser process, load them.
    ResourceBundle::GetSharedInstance().LoadThemeResources();
  }
  // Now load the nib.
  [NSBundle loadNibNamed:@"MainMenu" owner:NSApp];

  // This is a no-op if the KeystoneRegistration framework is not present.
  // The framework is only distributed with branded Google Chrome builds.
  [[KeystoneGlue defaultKeystoneGlue] registerWithKeystone];
}

// Perform platform-specific work that needs to be done after the main event
// loop has ended. We need to send the notifications that Cooca normally would
// telling everyone the app is about to end.
void WillTerminate() {
  [[NSNotificationCenter defaultCenter]
      postNotificationName:NSApplicationWillTerminateNotification
                    object:NSApp];
}

}  // namespace Platform

// From browser_main_win.h, stubs until we figure out the right thing...

int DoUninstallTasks(bool chrome_still_running) {
  return ResultCodes::NORMAL_EXIT;
}

bool DoUpgradeTasks(const CommandLine& command_line) {
  return false;
}

bool CheckForWin2000() {
  return false;
}

int HandleIconsCommands(const CommandLine& parsed_command_line) {
  return 0;
}

bool CheckMachineLevelInstall() {
  return false;
}

void PrepareRestartOnCrashEnviroment(const CommandLine& parsed_command_line) {
}

void RecordBreakpadStatusUMA(MetricsService* metrics) {
}
