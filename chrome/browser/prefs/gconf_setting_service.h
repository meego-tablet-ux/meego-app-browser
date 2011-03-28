/*
 * Copyright (c) 2010, Intel Corporation. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above 
 * copyright notice, this list of conditions and the following disclaimer 
 * in the documentation and/or other materials provided with the 
 * distribution.
 *     * Neither the name of Intel Corporation nor the names of its 
 * contributors may be used to endorse or promote products derived from 
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//
// gconf_setting_service.h
//
#ifndef CHROME_BROWSER_PREFS_GCONF_SETTING_SERVICE_H_
#define CHROME_BROWSER_PREFS_GCONF_SETTING_SERVICE_H_
#include <glib.h>
#include <gconf/gconf-client.h>
#include <gnome-keyring.h>
#include <gnome-keyring-result.h>

#include "chrome/common/content_settings.h"
#include "chrome/common/content_settings_types.h"
#include "content/common/notification_registrar.h"
#include "content/common/notification_details.h"
#include "content/common/notification_source.h"
#include "content/common/notification_type.h"
#include "content/common/notification_observer.h"
#include "chrome/browser/browsing_data_remover.h"
#include "chrome/browser/metrics/user_metrics.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/profile_sync_service_observer.h"
#include "chrome/browser/search_engines/template_url_model_observer.h"
#include "chrome/browser/search_engines/template_url_model.h"
#include "googleurl/src/gurl.h"

//
// GConfSettingService is used to sync browser private setting with gconf setting.
//
// It plays three observer roles:
// o Observe /app/browser gconf dir to check whether the gconf setting is
//   changed by other /applications, such as browser standalone setting.
//   Once setting is updated, GConfSettingService will also update chromium
//   internal setting.
// o Observe chromium's Preference service to check whether the setting
//   is changed by chromium itself (tab-based setting page). Once the setting
//   is updated by chromium itself, the new setting will be updated to gconf
//   setting
// o Observe profile sync service to determine the sync state. Once the sync
//   state is changed, the state code is also updated to gconf setting
//

class GConfSettingService : public NotificationObserver,
	public ProfileSyncServiceObserver, public TemplateURLModelObserver,
		public BrowsingDataRemover::Observer
{
public:
	explicit GConfSettingService();
	virtual ~GConfSettingService();

	void Initialize(Profile* profile);

	Profile* profile() { return profile_; }

	// NotificationObserver interface
	virtual void Observe(NotificationType type,
			const NotificationSource& source,
			const NotificationDetails& details);

	// ProfileSyncServiceObserver interface
	virtual void OnStateChanged();

	// TemplateURLModelObserver method
	virtual void OnTemplateURLModelChanged();

	// Call back on completed to remove browsing data
	virtual void OnBrowsingDataRemoverDone();

	// GConf callback for kBrowserGConfSettingDir
	static void OnGConfSettingChanged(GConfClient* client, guint cnxn_id,
			GConfEntry* entry, void* userdata);

	// New tab is home page ?
	bool GetNewTabIsHomePage(bool* new_tab_is_home_page);
	// Get home page URL according to gconf value
	GURL GetHomePage();

	GConfClient* client() const { return client_; }

protected:
	// Register different observer roles to chromium's profile system
	void RegisterProfileObservers(Profile* profile);

	// Register gconf notify func for observing changed occured on gconf
	void RegisterGConfNotifyFuncs();

	// Set default value for gconf setting at the first time when
	// the browser gconf setting directory is not exisiting
	void SetDefaultGConfValue();

	// Add a directory into gconf
	void AddDir(const char* dirname);

	// Sync chromium profile value with gconf setting
	// On browser startup, it should sync its own preference with gconf
	// setting by invoking this method
	void SyncPreferenceWithGConf(Profile* profile);

	//----------
	// Update* and Clear* methods are used to update chromium profile
	// according to the value retrieved from gconf. If the value
	// is not set in gconf, they also set gconf setting according
	// to chromium's default preference.
	void UpdatePageOpenedOnStartup(Profile* profile);

	void UpdateNewTabIsHomePage(Profile* profile);

	void UpdateHomePage(Profile* profile);

	void UpdateShowBookmarkBar(Profile* profile);

	void UpdateDefaultSearchEngine(Profile* profile);

	void UpdateAllowJavascript(Profile* profile);

	void UpdateAllowPopup(Profile* profile);

	void UpdateAllowCookies(Profile* profile);

	void UpdateAllowImages(Profile* profile);

	void UpdateSavePassword(Profile* profile);

	// Update sync setting and make sure the chromium itself preference
	// and gconf setting are consistent. In case of:
	// 1) Chromium has setup sync service via tab-based setting page and
	//    SyncStatus in gconf is SYNC_STATE_UNSETUP. In this situation,
	//    SyncStatus will be set to SYNC_STATE_RESPONSE_DONE, also, the
	//    user and last synced time will be updated.
	// 2) Chromium hasn't setup sync service yet and SyncStatus in gconf
	//    is set to SYNC_STATE_REQUEST_SETUP. It means standalone setting
	//    app request to setup a new sync service with given user name and
	//    password info. Chromium reads these info and try to setup.
	// 3) SyncStatus is set to SYNC_STATE_REQUEST_STOP in gconf. It means
	//    that sync service will be stopped perminently.
	void UpdateSyncSetting(Profile* profile);

	// Clear browsing data if gconf is set
	void ClearBrowsingData(Profile* profile);
	//-----

	// Set search provider list in gconf
	void SetSearchEngineList();

	// Set clearing data items in gconf according to chromium's profile
	void SetClearBrowsingDataItems();

	//
	// TODO: use gnome-keyring to store password for syncing
	//
	// Callback when sync password found from gnome keyring
	// static void OnSyncPasswordFound(GnomeKeyringResult res,
	//		const char* password, gpointer data);

protected:
	int gconf_setting_id_;
	int sync_setting_id_;

	GConfClient* client_;
	Profile*     profile_;

	// A notification register
	NotificationRegistrar registrar_;

	DISALLOW_COPY_AND_ASSIGN(GConfSettingService);
};

#endif
