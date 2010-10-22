// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/service/cloud_print/print_system.h"

namespace cloud_print {

PrinterBasicInfo::PrinterBasicInfo() : printer_status(0) {}

PrinterBasicInfo::~PrinterBasicInfo() {}

PrintJobDetails::PrintJobDetails()
    : status(PRINT_JOB_STATUS_INVALID),
      platform_status_flags(0),
      total_pages(0),
      pages_printed(0) {
}

void PrintJobDetails::Clear() {
  status = PRINT_JOB_STATUS_INVALID;
  platform_status_flags = 0;
  status_message.clear();
  total_pages = 0;
  pages_printed = 0;
}

PrintSystem::PrintServerWatcher::~PrintServerWatcher() {}

PrintSystem::PrinterWatcher::~PrinterWatcher() {}

PrintSystem::JobSpooler::~JobSpooler() {}

PrintSystem::~PrintSystem() {}

}  // namespace cloud_print
