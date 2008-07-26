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

#ifndef CHROME_TEST_AUTOMATION_AUTOMATION_MESSAGES_H__
#define CHROME_TEST_AUTOMATION_AUTOMATION_MESSAGES_H__

#include <string>

#include "base/basictypes.h"
#include "chrome/common/ipc_message.h"
#include "chrome/common/ipc_message_utils.h"

enum AutomationMsg_NavigationResponseValues {
  AUTOMATION_MSG_NAVIGATION_ERROR = 0,
  AUTOMATION_MSG_NAVIGATION_SUCCESS,
  AUTOMATION_MSG_NAVIGATION_AUTH_NEEDED,
};

// Two-pass include of render_messages_internal.  Preprocessor magic allows
// us to use 1 header to define the enums and classes for our render messages.
#define IPC_MESSAGE_MACROS_ENUMS
#include "chrome/test/automation/automation_messages_internal.h"

#ifdef IPC_MESSAGE_MACROS_LOG_ENABLED
#  undef IPC_MESSAGE_MACROS_LOG
#  define IPC_MESSAGE_MACROS_CLASSES
#include "chrome/test/automation/automation_messages_internal.h"

#  undef IPC_MESSAGE_MACROS_CLASSES
#  define IPC_MESSAGE_MACROS_LOG
#include "chrome/test/automation/automation_messages_internal.h"
#else
// No debug strings requested, just define the classes
#  define IPC_MESSAGE_MACROS_CLASSES
#include "chrome/test/automation/automation_messages_internal.h"
#endif


#endif  // CHROME_TEST_AUTOMATION_AUTOMATION_MESSAGES_H__
