// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_LOGGING_WIN_H_
#define BASE_LOGGING_WIN_H_
#pragma once

#include <string>
#include "base/basictypes.h"
#include "base/event_trace_provider_win.h"
#include "base/logging.h"

namespace logging {

// Event ID for the log messages we generate.
extern const GUID kLogEventId;

// Feature enable mask for LogEventProvider.
enum LogEnableMask {
  // If this bit is set in our provider enable mask, we will include
  // a stack trace with every log message.
  ENABLE_STACK_TRACE_CAPTURE = 0x0001,
  // If this bit is set in our provider enable mask, the provider will log
  // a LOG message with only the textual content of the message, and no
  // stack trace.
  ENABLE_LOG_MESSAGE_ONLY = 0x0002,
};

// The message types our log event provider generates.
// ETW likes user message types to start at 10.
enum LogMessageTypes {
  // A textual only log message, contains a zero-terminated string.
  LOG_MESSAGE = 10,
  // A message with a stack trace, followed by the zero-terminated
  // message text.
  LOG_MESSAGE_WITH_STACKTRACE = 11,
  // A message with:
  //  a stack trace,
  //  the line number as a four byte integer,
  //  the file as a zero terminated UTF8 string,
  //  the zero-terminated UTF8 message text.
  LOG_MESSAGE_FULL = 12,
};

// Trace provider class to drive log control and transport
// with Event Tracing for Windows.
class LogEventProvider : public EtwTraceProvider {
 public:
  LogEventProvider();

  static bool LogMessage(logging::LogSeverity severity, const char* file,
      int line, size_t message_start, const std::string& str);

  static void Initialize(const GUID& provider_name);
  static void Uninitialize();

 protected:
  // Overridden to manipulate the log level on ETW control callbacks.
  virtual void OnEventsEnabled();
  virtual void OnEventsDisabled();

 private:
  // The log severity prior to OnEventsEnabled,
  // restored in OnEventsDisabled.
  logging::LogSeverity old_log_level_;

  DISALLOW_COPY_AND_ASSIGN(LogEventProvider);
};

}  // namespace logging

#endif  // BASE_LOGGING_WIN_H_
