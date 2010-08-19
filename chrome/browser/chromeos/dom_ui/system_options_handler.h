// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DOM_UI_SYSTEM_OPTIONS_HANDLER_H_
#define CHROME_BROWSER_CHROMEOS_DOM_UI_SYSTEM_OPTIONS_HANDLER_H_
#pragma once

#include "chrome/browser/chromeos/dom_ui/cros_options_page_ui_handler.h"

class DictionaryValue;

// ChromeOS system options page UI handler.
class SystemOptionsHandler : public chromeos::CrosOptionsPageUIHandler {
 public:
  SystemOptionsHandler();
  virtual ~SystemOptionsHandler();

  // OptionsUIHandler implementation.
  virtual void GetLocalizedValues(DictionaryValue* localized_strings);

 private:
  DISALLOW_COPY_AND_ASSIGN(SystemOptionsHandler);
};

#endif  // CHROME_BROWSER_CHROMEOS_DOM_UI_SYSTEM_OPTIONS_HANDLER_H_
