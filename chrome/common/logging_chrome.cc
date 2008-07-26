// Copyright 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <windows.h>

#include <iostream>
#include <fstream>

#include "chrome/common/logging_chrome.h"

#include "base/command_line.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/string_util.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/env_util.h"
#include "chrome/common/env_vars.h"

// When true, this means that error dialogs should not be shown.
static bool dialogs_are_suppressed_ = false;

// This should be true for exactly the period between the end of
// InitChromeLogging() and the beginning of CleanupChromeLogging().
static bool chrome_logging_initialized_ = false;

// Assertion handler for logging errors that occur when dialogs are
// silenced.  To record a new error, pass the log string associated
// with that error in the str parameter.
#pragma optimize("", off)
static void SilentRuntimeAssertHandler(const std::string& str) {
  __debugbreak();
}
#pragma optimize("", on)

// Suppresses error/assertion dialogs and enables the logging of
// those errors into silenced_errors_.
static void SuppressDialogs() {
  if (dialogs_are_suppressed_)
    return;

  logging::SetLogAssertHandler(SilentRuntimeAssertHandler);

  UINT new_flags = SEM_FAILCRITICALERRORS |
                   SEM_NOGPFAULTERRORBOX |
                   SEM_NOOPENFILEERRORBOX;

  // Preserve existing error mode, as discussed at http://t/dmea
  UINT existing_flags = SetErrorMode(new_flags);
  SetErrorMode(existing_flags | new_flags);

  dialogs_are_suppressed_ = true;
}

namespace logging {

void InitChromeLogging(const CommandLine& command_line,
                       OldFileDeletionState delete_old_log_file) {
  DCHECK(!chrome_logging_initialized_) <<
    "Attempted to initialize logging when it was already initialized.";

  // only use OutputDebugString in debug mode
#ifdef NDEBUG
  bool enable_logging = false;
  const wchar_t *kInvertLoggingSwitch = switches::kEnableLogging;
  const logging::LoggingDestination kDefaultLoggingMode =
      logging::LOG_ONLY_TO_FILE;
#else
  bool enable_logging = true;
  const wchar_t *kInvertLoggingSwitch = switches::kDisableLogging;
  const logging::LoggingDestination kDefaultLoggingMode =
      logging::LOG_TO_BOTH_FILE_AND_SYSTEM_DEBUG_LOG;
#endif

  if (command_line.HasSwitch(kInvertLoggingSwitch))
    enable_logging = !enable_logging;

  logging::LoggingDestination log_mode;
  if (enable_logging) {
    log_mode = kDefaultLoggingMode;
  } else {
    log_mode = logging::LOG_NONE;
  }

  logging::InitLogging(GetLogFileName().c_str(),
                       log_mode,
                       logging::LOCK_LOG_FILE,
                       delete_old_log_file);

  // we want process and thread IDs because we have a lot of things running
  logging::SetLogItems(true, true, false, true);

  // We call running in unattended mode "headless", and allow
  // headless mode to be configured either by the Environment
  // Variable or by the Command Line Switch.  This is for
  // automated test purposes.
  if (env_util::HasEnvironmentVariable(env_vars::kHeadless) ||
      command_line.HasSwitch(switches::kNoErrorDialogs))
    SuppressDialogs();

  std::wstring log_filter_prefix =
      command_line.GetSwitchValue(switches::kLogFilterPrefix);
  logging::SetLogFilterPrefix(WideToUTF8(log_filter_prefix).c_str());

  chrome_logging_initialized_ = true;
}

// This is a no-op, but we'll keep it around in case
// we need to do more cleanup in the future.
void CleanupChromeLogging() {
  DCHECK(chrome_logging_initialized_) <<
    "Attempted to clean up logging when it wasn't initialized.";

  CloseLogFile();

  chrome_logging_initialized_ = false;
}

std::wstring GetLogFileName() {
  wchar_t filename[MAX_PATH];
  unsigned status = GetEnvironmentVariable(env_vars::kLogFileName,
                                           filename, MAX_PATH);
  if (status && (status <= MAX_PATH))
    return std::wstring(filename);

  const std::wstring log_filename(L"chrome_debug.log");
  std::wstring log_path;

  if (PathService::Get(chrome::DIR_LOGS, &log_path)) {
    file_util::AppendToPath(&log_path, log_filename);
    return log_path;
  } else {
    // error with path service, just use some default file somewhere
    return log_filename;
  }
}

bool DialogsAreSuppressed() {
  return dialogs_are_suppressed_;
}

size_t GetFatalAssertions(AssertionList* assertions) {
  // In this function, we don't assume that assertions is non-null, so
  // that if you just want an assertion count, you can pass in NULL.
  if (assertions)
    assertions->clear();
  size_t assertion_count = 0;

  std::ifstream log_file;
  log_file.open(GetLogFileName().c_str());
  if (!log_file.is_open())
    return 0;

  std::string utf8_line;
  std::wstring wide_line;
  while(!log_file.eof()) {
    getline(log_file, utf8_line);
    if (utf8_line.find(":FATAL:") != std::string::npos) {
      wide_line = UTF8ToWide(utf8_line);
      if (assertions)
        assertions->push_back(wide_line);
      ++assertion_count;
    }
  }
  log_file.close();

  return assertion_count;
}

} // namespace logging
