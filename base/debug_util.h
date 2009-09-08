// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is a cross platform interface for helper functions related to debuggers.
// You should use this to test if you're running under a debugger, and if you
// would like to yield (breakpoint) into the debugger.

#ifndef BASE_DEBUG_UTIL_H_
#define BASE_DEBUG_UTIL_H_

#include <iosfwd>
#include <vector>

#include "base/basictypes.h"

#if defined(OS_WIN)
struct _EXCEPTION_POINTERS;
#endif

// A stacktrace can be helpful in debugging. For example, you can include a
// stacktrace member in a object (probably around #ifndef NDEBUG) so that you
// can later see where the given object was created from.
class StackTrace {
 public:
  // Creates a stacktrace from the current location
  StackTrace();
#if defined(OS_WIN)
  // Creates a stacktrace for an exception.
  // Note: this function will throw an import not found (StackWalk64) exception
  // on system without dbghelp 5.1.
  StackTrace(_EXCEPTION_POINTERS* exception_pointers);
#endif
  // Gets an array of instruction pointer values.
  //   count: (output) the number of elements in the returned array
  const void *const *Addresses(size_t* count);
  // Prints a backtrace to stderr
  void PrintBacktrace();

  // Resolves backtrace to symbols and write to stream.
  void OutputToStream(std::ostream* os);

 private:
  // From http://msdn.microsoft.com/en-us/library/bb204633.aspx,
  // the sum of FramesToSkip and FramesToCapture must be less than 63,
  // so set it to 62. Even if on POSIX it could be a larger value, it usually
  // doesn't give much more information.
  static const int MAX_TRACES = 62;
  void* trace_[MAX_TRACES];
  int count_;

  DISALLOW_EVIL_CONSTRUCTORS(StackTrace);
};

class DebugUtil {
 public:
  // Starts the registered system-wide JIT debugger to attach it to specified
  // process.
  static bool SpawnDebuggerOnProcess(unsigned process_id);

  // Waits wait_seconds seconds for a debugger to attach to the current process.
  // When silent is false, an exception is thrown when a debugger is detected.
  static bool WaitForDebugger(int wait_seconds, bool silent);

  // Are we running under a debugger?
  // On OS X, the underlying mechanism doesn't work when the sandbox is enabled.
  // To get around this, this function caches its value.
  // WARNING: Because of this, on OS X, a call MUST be made to this function
  // BEFORE the sandbox is enabled.
  static bool BeingDebugged();

  // Break into the debugger, assumes a debugger is present.
  static void BreakDebugger();

#if defined(OS_MACOSX)
  // On OS X, it can take a really long time for the OS Crash handler to
  // process a Chrome crash.  This translates into a long wait till the process
  // actually dies.
  // This method disables OS Crash reporting entireley.
  // TODO(playmobil): Remove this when we have Breakpad integration enabled -
  // see http://crbug.com/7652
  static void DisableOSCrashDumps();
#endif  // defined(OS_MACOSX)
};

#endif  // BASE_DEBUG_UTIL_H_
