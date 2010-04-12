// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_NOTIFICATIONS_PREFS_CACHE_H_
#define CHROME_BROWSER_NOTIFICATIONS_NOTIFICATIONS_PREFS_CACHE_H_

#include <set>
#include <vector>

#include "base/ref_counted.h"
#include "googleurl/src/gurl.h"

class ListValue;

// Class which caches notification preferences.
// Construction occurs on the UI thread when the contents
// of the profile preferences are initialized.  Once is_initialized() is set,
// access can only be done from the IO thread.
class NotificationsPrefsCache
    : public base::RefCountedThreadSafe<NotificationsPrefsCache> {
 public:
  NotificationsPrefsCache();

  void set_is_initialized(bool val) { is_initialized_ = val; }
  bool is_initialized() { return is_initialized_; }

  // Checks to see if a given origin has permission to create desktop
  // notifications.  Returns a constant from WebNotificationPresenter
  // class.
  int HasPermission(const GURL& origin);

  // Updates the cache with a new origin allowed or denied.
  void CacheAllowedOrigin(const GURL& origin);
  void CacheDeniedOrigin(const GURL& origin);

  // Set the cache to the supplied values.  This clears the current
  // contents of the cache.
  void SetCacheAllowedOrigins(const std::vector<GURL>& allowed);
  void SetCacheDeniedOrigins(const std::vector<GURL>& denied);

  static void ListValueToGurlVector(const ListValue& origin_list,
                                    std::vector<GURL>* origin_vector);

 private:
  friend class base::RefCountedThreadSafe<NotificationsPrefsCache>;

  ~NotificationsPrefsCache() {}

  // Helper functions which read preferences.
  bool IsOriginAllowed(const GURL& origin);
  bool IsOriginDenied(const GURL& origin);

  // Helper that ensures we are running on the expected thread.
  void CheckThreadAccess();

  // Storage of the actual preferences.
  std::set<GURL> allowed_origins_;
  std::set<GURL> denied_origins_;

  // Set to true once the initial cached settings have been completely read.
  // Once this is done, the class can no longer be accessed on the UI thread.
  bool is_initialized_;

  DISALLOW_COPY_AND_ASSIGN(NotificationsPrefsCache);
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_NOTIFICATIONS_PREFS_CACHE_H_
