// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NOTIFICATIONS_DESKTOP_NOTIFICATION_SERVICE_H_
#define CHROME_BROWSER_NOTIFICATIONS_DESKTOP_NOTIFICATION_SERVICE_H_
#pragma once

#include <vector>

#include "base/basictypes.h"
#include "base/string16.h"
#include "chrome/browser/notifications/notification.h"
#include "chrome/common/content_settings.h"
#include "chrome/common/notification_observer.h"
#include "chrome/common/notification_registrar.h"
#include "chrome/common/notification_service.h"
#include "googleurl/src/gurl.h"
#include "third_party/WebKit/WebKit/chromium/public/WebTextDirection.h"

class NotificationUIManager;
class NotificationsPrefsCache;
class PrefService;
class Profile;
class Task;
class TabContents;
struct ViewHostMsg_ShowNotification_Params;

// The DesktopNotificationService is an object, owned by the Profile,
// which provides the creation of desktop "toasts" to web pages and workers.
class DesktopNotificationService : public NotificationObserver {
 public:
  enum DesktopNotificationSource {
    PageNotification,
    WorkerNotification
  };

  DesktopNotificationService(Profile* profile,
                             NotificationUIManager* ui_manager);
  virtual ~DesktopNotificationService();

  // Requests permission (using an info-bar) for a given origin.
  // |callback_context| contains an opaque value to pass back to the
  // requesting process when the info-bar finishes.
  void RequestPermission(const GURL& origin,
                         int process_id,
                         int route_id,
                         int callback_context,
                         TabContents* tab);

  // ShowNotification is called on the UI thread handling IPCs from a child
  // process, identified by |process_id| and |route_id|.  |source| indicates
  // whether the script is in a worker or page. |params| contains all the
  // other parameters supplied by the worker or page.
  bool ShowDesktopNotification(
      const ViewHostMsg_ShowNotification_Params& params,
      int process_id, int route_id, DesktopNotificationSource source);

  // Cancels a notification.  If it has already been shown, it will be
  // removed from the screen.  If it hasn't been shown yet, it won't be
  // shown.
  bool CancelDesktopNotification(int process_id,
                                 int route_id,
                                 int notification_id);

  // Methods to setup and modify permission preferences.
  void GrantPermission(const GURL& origin);
  void DenyPermission(const GURL& origin);

  // NotificationObserver implementation.
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

  NotificationsPrefsCache* prefs_cache() { return prefs_cache_; }

  // Creates a data:xxxx URL which contains the full HTML for a notification
  // using supplied icon, title, and text, run through a template which contains
  // the standard formatting for notifications.
  static string16 CreateDataUrl(const GURL& icon_url,
                                const string16& title,
                                const string16& body,
                                WebKit::WebTextDirection dir);

  // The default content setting determines how to handle origins that haven't
  // been allowed or denied yet.
  ContentSetting GetDefaultContentSetting();
  void SetDefaultContentSetting(ContentSetting setting);

  // Returns all origins that explicitly have been allowed.
  std::vector<GURL> GetAllowedOrigins();

  // Returns all origins that explicitly have been denied.
  std::vector<GURL> GetBlockedOrigins();

  // Removes an origin from the "explicitly allowed" set.
  void ResetAllowedOrigin(const GURL& origin);

  // Removes an origin from the "explicitly denied" set.
  void ResetBlockedOrigin(const GURL& origin);

  // Clears the sets of explicitly allowed and denied origins.
  void ResetAllOrigins();

  static void RegisterUserPrefs(PrefService* user_prefs);

 private:
  void InitPrefs();
  void StartObserving();
  void StopObserving();

  // Takes a notification object and shows it in the UI.
  void ShowNotification(const Notification& notification);

  // Save a permission change to the profile.
  void PersistPermissionChange(const GURL& origin, bool is_allowed);

  // Returns a display name for an origin, to be used in permission infobar
  // or on the frame of the notification toast.  Different from the origin
  // itself when dealing with extensions.
  string16 DisplayNameForOrigin(const GURL& origin);

  ContentSetting GetContentSetting(const GURL& origin);

  // The profile which owns this object.
  Profile* profile_;

  // A cache of preferences which is accessible only on the IO thread
  // to service synchronous IPCs.
  scoped_refptr<NotificationsPrefsCache> prefs_cache_;

  // Non-owned pointer to the notification manager which manages the
  // UI for desktop toasts.
  NotificationUIManager* ui_manager_;

  DISALLOW_COPY_AND_ASSIGN(DesktopNotificationService);
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_DESKTOP_NOTIFICATION_SERVICE_H_
