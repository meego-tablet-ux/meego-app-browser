// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/first_run/first_run.h"


#include "base/command_line.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/path_service.h"
#include "base/process_util.h"
#include "base/string_piece.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/ui/meegotouch/first_run_dialog.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/installer/util/google_update_settings.h"
#include "content/common/result_codes.h"
#include "googleurl/src/gurl.h"
#include "ui/base/ui_base_switches.h"

bool OpenFirstRunDialog(Profile* profile, bool homepage_defined,
                        int import_items,
                        int dont_import_items,
                        bool search_engine_experiment,
                        bool randomize_search_engine_experiment,
                        ProcessSingleton* process_singleton) {
  NOTIMPLEMENTED();
  return true;
}

FilePath GetDefaultPrefFilePath(bool create_profile_dir,
                                const FilePath& user_data_dir) {
  NOTIMPLEMENTED();
}

bool FirstRun::ImportBookmarks(const FilePath& import_bookmarks_path) {
  NOTIMPLEMENTED();
  return false;
}

// static
bool FirstRun::IsOrganicFirstRun() {
  // We treat all installs as organic.
  NOTIMPLEMENTED();
  return true;
}

// static
void FirstRun::PlatformSetup() {
  // Things that Windows does here (creating a desktop icon, for example) are
  // handled at install time on Linux.
  NOTIMPLEMENTED();
}
