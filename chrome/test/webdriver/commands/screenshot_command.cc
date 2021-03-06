// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/webdriver/commands/screenshot_command.h"

#include <string>
#include <vector>

#include "base/base64.h"
#include "base/values.h"
#include "chrome/test/webdriver/commands/response.h"
#include "chrome/test/webdriver/error_codes.h"
#include "chrome/test/webdriver/session.h"

namespace webdriver {

ScreenshotCommand::ScreenshotCommand(const std::vector<std::string>& ps,
                                     const DictionaryValue* const parameters)
    : WebDriverCommand(ps, parameters) {}

ScreenshotCommand::~ScreenshotCommand() {}

bool ScreenshotCommand::DoesGet() {
  return true;
}

void ScreenshotCommand::ExecuteGet(Response* const response) {
  std::string raw_bytes;
  if (!session_->GetScreenShot(&raw_bytes)) {
    SET_WEBDRIVER_ERROR(response, "Screenshot of current page failed",
                        kInternalServerError);
    return;
  }

  // Convert the raw binary data to base 64 encoding for webdriver.
  std::string base64_screenshot;
  if (!base::Base64Encode(raw_bytes, &base64_screenshot)) {
    SET_WEBDRIVER_ERROR(response, "Encoding the PNG to base64 format failed",
                        kInternalServerError);
    return;
  }

  response->SetValue(new StringValue(base64_screenshot));
  response->SetStatus(kSuccess);
}

}  // namespace webdriver

