// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOM_UI_SHOWN_SECTIONS_HANDLER_H_
#define CHROME_BROWSER_DOM_UI_SHOWN_SECTIONS_HANDLER_H_

#include "chrome/browser/dom_ui/dom_ui.h"

class DOMUI;
class Value;
class PrefService;

// Use for the shown sections bitmask.
enum Section {
  THUMB = 1,
  LIST = 2,
  RECENT = 4,
  TIPS = 8,
  SYNC = 16,
  DEBUG = 32
};

class ShownSectionsHandler : public DOMMessageHandler {
 public:
  ShownSectionsHandler() {}
  virtual ~ShownSectionsHandler() {}

  // DOMMessageHandler implementation.
  virtual void RegisterMessages();

  // Callback for "getShownSections" message.
  void HandleGetShownSections(const Value* value);

  // Callback for "setShownSections" message.
  void HandleSetShownSections(const Value* value);

  // Sets the prefs for first run of the App Launcher.
  static void SetFirstAppLauncherRunPref(PrefService* pref_service);

  static void RegisterUserPrefs(PrefService* pref_service);

  static void MigrateUserPrefs(PrefService* pref_service,
                               int old_pref_version,
                               int new_pref_version);

 private:
  DISALLOW_COPY_AND_ASSIGN(ShownSectionsHandler);
};

#endif  // CHROME_BROWSER_DOM_UI_SHOWN_SECTIONS_HANDLER_H_
