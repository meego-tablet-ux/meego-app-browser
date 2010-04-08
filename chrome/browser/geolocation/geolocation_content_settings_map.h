// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Maps [requesting_origin, embedder] to content settings.  Written on the UI
// thread and read on any thread.  One instance per profile. This is based on
// HostContentSettingsMap but differs significantly in two aspects:
// - It maps [requesting_origin.GetOrigin(), embedder.GetOrigin()] => setting
//   rather than host => setting.
// - It manages only Geolocation.

#ifndef CHROME_BROWSER_GEOLOCATION_GEOLOCATION_CONTENT_SETTINGS_MAP_H_
#define CHROME_BROWSER_GEOLOCATION_GEOLOCATION_CONTENT_SETTINGS_MAP_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/basictypes.h"
#include "base/lock.h"
#include "base/ref_counted.h"
#include "chrome/common/content_settings.h"
#include "chrome/common/notification_observer.h"
#include "googleurl/src/gurl.h"

class DictionaryValue;
class PrefService;
class Profile;

class GeolocationContentSettingsMap
    : public NotificationObserver,
      public base::RefCountedThreadSafe<GeolocationContentSettingsMap> {
 public:
  typedef std::map<GURL, ContentSetting> OneOriginSettings;
  typedef std::map<GURL, OneOriginSettings> AllOriginsSettings;

  explicit GeolocationContentSettingsMap(Profile* profile);

  static void RegisterUserPrefs(PrefService* prefs);

  // Return simplified string representing origin.  If origin is using http or
  // the standard port, those parts are not included in the output.
  static std::string OriginToString(const GURL& origin);

  // Returns the default setting.
  //
  // This may be called on any thread.
  ContentSetting GetDefaultContentSetting() const;

  // Returns a single ContentSetting which applies to the given |requesting_url|
  // when embedded in a top-level page from |embedding_url|.  To determine the
  // setting for a top-level page, as opposed to a frame embedded in a page,
  // pass the page's URL for both arguments.
  //
  // This may be called on any thread.  Both arguments should be valid GURLs.
  ContentSetting GetContentSetting(const GURL& requesting_url,
                                   const GURL& embedding_url) const;

  // Returns the settings for all origins with any non-default settings.
  //
  // This may be called on any thread.
  AllOriginsSettings GetAllOriginsSettings() const;

  // Sets the default setting.
  //
  // This should only be called on the UI thread.
  void SetDefaultContentSetting(ContentSetting setting);

  // Sets the content setting for a particular (requesting origin, embedding
  // origin) pair.  If the embedding origin is the same as the requesting
  // origin, this represents the setting used when the requesting origin is
  // itself the top-level page.  If |embedder| is the empty GURL, |setting|
  // becomes the default setting for the requesting origin when embedded on any
  // page that does not have an explicit setting.  Passing
  // CONTENT_SETTING_DEFAULT for |setting| effectively removes that setting and
  // allows future requests to return the all-embedders or global defaults (as
  // applicable).
  //
  // This should only be called on the UI thread.  |requesting_url| should be
  // a valid GURL, and |embedding_url| should be valid or empty.
  void SetContentSetting(const GURL& requesting_url,
                         const GURL& embedding_url,
                         ContentSetting setting);

  // Clears all settings for |requesting_origin|.  Note: Unlike in the functions
  // above, this is expected to be an origin, not some URL of which we'll take
  // the origin; this is to prevent ambiguity where callers could think they're
  // clearing something wider or narrower than they really are.
  //
  // This should only be called on the UI thread.  |requesting_origin| should be
  // a valid GURL.
  void ClearOneRequestingOrigin(const GURL& requesting_origin);

  // Resets all settings.
  //
  // This should only be called on the UI thread.
  void ResetToDefault();

  // NotificationObserver implementation.
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

 private:
  friend class base::RefCountedThreadSafe<GeolocationContentSettingsMap>;

  // The default setting.
  static const ContentSetting kDefaultSetting;

  ~GeolocationContentSettingsMap();

  // Reads the exceptions from the preference service.
  void ReadExceptions();

  // Sets the fields of |one_origin_settings| based on the values in
  // |dictionary|.
  static void GetOneOriginSettingsFromDictionary(
      const DictionaryValue* dictionary,
      OneOriginSettings* one_origin_settings);

  // The profile we're associated with.
  Profile* profile_;

  // Copies of the pref data, so that we can read it on the IO thread.
  ContentSetting default_content_setting_;
  AllOriginsSettings content_settings_;

  // Used around accesses to the settings objects to guarantee thread safety.
  mutable Lock lock_;

  // Whether we are currently updating preferences, this is used to ignore
  // notifications from the preference service that we triggered ourself.
  bool updating_preferences_;

  DISALLOW_COPY_AND_ASSIGN(GeolocationContentSettingsMap);
};

#endif  // CHROME_BROWSER_GEOLOCATION_GEOLOCATION_CONTENT_SETTINGS_MAP_H_
