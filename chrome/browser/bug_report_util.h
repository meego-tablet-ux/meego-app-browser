// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BUG_REPORT_UTIL_H_
#define CHROME_BROWSER_BUG_REPORT_UTIL_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#if defined(OS_MACOSX)
#include "base/mac_util.h"
#include "base/sys_info.h"
#endif
#include "base/scoped_ptr.h"

#include "chrome/browser/userfeedback/proto/common.pb.h"
#include "chrome/browser/userfeedback/proto/extension.pb.h"
#include "chrome/browser/userfeedback/proto/math.pb.h"
#include "gfx/rect.h"

class Profile;
class TabContents;

class BugReportUtil {
 public:
  enum BugType {
    PAGE_WONT_LOAD = 0,
    PAGE_LOOKS_ODD,
    PHISHING_PAGE,
    CANT_SIGN_IN,
    CHROME_MISBEHAVES,
    SOMETHING_MISSING,
    BROWSER_CRASH,
    OTHER_PROBLEM
  };

  // SetOSVersion copies the maj.minor.build + servicePack_string
  // into a string. We currently have:
  //   win_util::GetWinVersion returns WinVersion, which is just
  //     an enum of 2000, XP, 2003, or VISTA. Not enough detail for
  //     bug reports.
  //   base::SysInfo::OperatingSystemVersion returns an std::string
  //     but doesn't include the build or service pack. That function
  //     is probably the right one to extend, but will require changing
  //     all the call sites or making it a wrapper around another util.
  static void SetOSVersion(std::string *os_version);

  // Generates bug report data.
  static void SendReport(Profile* profile,
      const std::string& page_title_text,
      int problem_type,
      const std::string& page_url_text,
      const std::string& description,
      const char* png_data,
      int png_data_length,
      int png_width,
      int png_height);

  // Redirects the user to Google's phishing reporting page.
  static void ReportPhishing(TabContents* currentTab,
                             const std::string& phishing_url);

  class PostCleanup;

 private:
  // Add a key value pair to the feedback object
  static void AddFeedbackData(
      userfeedback::ExternalExtensionSubmit* feedback_data,
      const std::string& key, const std::string& value);

  DISALLOW_IMPLICIT_CONSTRUCTORS(BugReportUtil);
};

#endif  // CHROME_BROWSER_BUG_REPORT_UTIL_H_
