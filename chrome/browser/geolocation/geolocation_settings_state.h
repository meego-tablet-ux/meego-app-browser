// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GEOLOCATION_GEOLOCATION_SETTINGS_STATE_H_
#define CHROME_BROWSER_GEOLOCATION_GEOLOCATION_SETTINGS_STATE_H_
#pragma once

#include <map>
#include <set>

#include "chrome/browser/tab_contents/navigation_controller.h"
#include "chrome/common/content_settings.h"

class GeolocationContentSettingsMap;

// This class manages the geolocation state per tab, and provides information
// and presentation data about the geolocation usage.
class GeolocationSettingsState {
 public:
  explicit GeolocationSettingsState(Profile* profile);
  virtual ~GeolocationSettingsState();

  typedef std::map<GURL, ContentSetting> StateMap;
  const StateMap& state_map() const {
    return state_map_;
  }

  // Sets the state for |requesting_origin|.
  void OnGeolocationPermissionSet(const GURL& requesting_origin, bool allowed);

  // Delegated by TabContents to indicate a navigation has happened and we
  // may need to clear our settings.
  void DidNavigate(const NavigationController::LoadCommittedDetails& details);

  enum TabState {
    TABSTATE_NONE = 0,
    // There's at least one entry with non-default setting.
    TABSTATE_HAS_EXCEPTION = 1 << 1,
    // There's at least one entry with a non-ASK setting.
    TABSTATE_HAS_ANY_ICON = 1 << 2,
    // There's at least one entry with ALLOWED setting.
    TABSTATE_HAS_ANY_ALLOWED = 1 << 3,
    // There's at least one entry that doesn't match the saved setting.
    TABSTATE_HAS_CHANGED = 1 << 4,
  };

  // Maps ContentSetting to a set of hosts formatted for presentation.
  typedef std::map<ContentSetting, std::set<std::string> >
      FormattedHostsPerState;

  // Returns an (optional) |formatted_hosts_per_state| and a mask of TabState.
  void GetDetailedInfo(FormattedHostsPerState* formatted_hosts_per_state,
                       unsigned int* tab_state_flags) const;

 private:
  std::string GURLToFormattedHost(const GURL& url) const;

  Profile* profile_;
  StateMap state_map_;
  GURL embedder_url_;

  DISALLOW_COPY_AND_ASSIGN(GeolocationSettingsState);
};

#endif  // CHROME_BROWSER_GEOLOCATION_GEOLOCATION_SETTINGS_STATE_H_
