// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_FRAME_TEST_NET_FAKE_EXTERNAL_TAB_H_
#define CHROME_FRAME_TEST_NET_FAKE_EXTERNAL_TAB_H_

#include <string>

#include "base/file_path.h"
#include "base/message_loop.h"

#include "chrome/app/scoped_ole_initializer.h"
#include "chrome/browser/browser_process_impl.h"

#include "chrome_frame/test/test_server.h"
#include "chrome_frame/test/net/test_automation_provider.h"
#include "chrome_frame/test/net/process_singleton_subclass.h"

#include "net/base/net_test_suite.h"

class ProcessSingleton;

class FakeExternalTab { 
 public:
  FakeExternalTab();
  ~FakeExternalTab();

  virtual std::wstring GetProfileName();

  virtual std::wstring GetProfilePath();
  virtual void Initialize();
  virtual void Shutdown();

  const FilePath& user_data() const {
    return user_data_dir_;
  }

  MessageLoopForUI* ui_loop() {
    return &loop_;
  }

 protected:
  MessageLoopForUI loop_;
  scoped_ptr<BrowserProcess> browser_process_;
  FilePath overridden_user_dir_;
  FilePath user_data_dir_;
  ScopedOleInitializer ole_initializer_;  // For RegisterDropTarget etc to work.
  scoped_ptr<ProcessSingleton> process_singleton_;
};

// The "master class" that spins the UI and test threads.
class CFUrlRequestUnittestRunner
    : public NetTestSuite,
      public ProcessSingletonSubclassDelegate,
      public TestAutomationProviderDelegate {
 public:
  CFUrlRequestUnittestRunner(int argc, char** argv);
  ~CFUrlRequestUnittestRunner();

  virtual void StartChromeFrameInHostBrowser();

  virtual void ShutDownHostBrowser();

  // Overrides to not call icu initialize
  virtual void Initialize();
  virtual void Shutdown();

  // ProcessSingletonSubclassDelegate.
  virtual void OnConnectAutomationProviderToChannel(
      const std::string& channel_id);

  // TestAutomationProviderDelegate.
  virtual void OnInitialTabLoaded();

  void RunMainUIThread();

  void StartTests();

 protected:
  // This is the thread that runs all the UrlRequest tests.
  // Within its context, the Initialize() and Shutdown() routines above
  // will be called.
  static DWORD WINAPI RunAllUnittests(void* param);

  static void TakeDownBrowser(CFUrlRequestUnittestRunner* me);

 protected:
  // Borrowed from TestSuite::Initialize().
  void InitializeLogging();

 protected:
  ScopedHandle test_thread_;
  DWORD test_thread_id_;
  scoped_ptr<MessageLoop> test_thread_message_loop_;

  scoped_ptr<test_server::SimpleWebServer> test_http_server_;
  test_server::SimpleResponse chrome_frame_html_;

  // The fake chrome instance.  This instance owns the UI message loop
  // on the main thread.
  FakeExternalTab fake_chrome_;
  scoped_ptr<ProcessSingletonSubclass> pss_subclass_;
};

#endif  // CHROME_FRAME_TEST_NET_FAKE_EXTERNAL_TAB_H_
