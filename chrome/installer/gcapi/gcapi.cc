// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/gcapi/gcapi.h"

#include <windows.h>

DLLEXPORT BOOL __stdcall GoogleChromeCompatibilityCheck(DWORD *reasons) {
  BOOL result = TRUE;
  DWORD local_reasons = 0;

  // TODO(rahulk): Add all the checks we need before offering Google Chrome as
  // part of bundle downloads.

  // OS requirements

  // Check if it is already installed?

  // Privileges?

  if (reasons != NULL) {
    *reasons = local_reasons;
  }
  return result;
}
