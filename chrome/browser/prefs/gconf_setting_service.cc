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
// GConfSettingService Impl
//

#include "base/logging.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "content/common/notification_service.h"
#include "chrome/common/pref_names.h"
#include "chrome/browser/prefs/session_startup_pref.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/prefs/pref_notifier.h"
#include "chrome/browser/prefs/pref_notifier_impl.h"
#include "chrome/browser/prefs/gconf_setting_service.h"
#include "chrome/browser/content_settings/host_content_settings_map.h"
#include "chrome/browser/content_settings/content_settings_details.h"
#include "chrome/browser/net/url_fixer_upper.h"
#include "chrome/browser/sync/profile_sync_service.h"
#include "chrome/browser/sync/sync_ui_util.h"
#include "chrome/browser/search_engines/template_url.h"
#include "chrome/browser/search_engines/util.h"

namespace {
	// GConf directory for browser
	const char* kBrowserGConfSettingDir = "/apps/browser";
	// Indicate whether browser is running, bool
	const char* kBrowserIsRunning = "/apps/browser/BrowserRunning";
	// Page being opened on startup, integer value
	// 1 for default, 2 for open last session opening pages
	const char* kPageOpenedOnStartup = "/apps/browser/PageOpenedOnStartup";
	// New tab page as home page
	const char* kNewTabIsHomePage = "/apps/browser/NewTabIsHomePage";
	// Home page in string
	const char* kHomePage = "/apps/browser/HomePage";
	// Show Bookmarkbar on chromium browser, bool value
	const char* kShowBookmarkBar = "/apps/browser/ShowBookmarkBar";
	// Search engine provider, a pair of provider name and its URL
	const char* kDefaultSearchEngine = "/apps/browser/DefaultSearchEngine";
	// Search engine list updated?
	const char* kSearchEngineListUpdated = "/apps/browser/SearchEngineListUpdated";
	// Search engine list
	const char* kSearchEngineList = "/apps/browser/SearchEngineList";
	// Clear Data Items ?
	const char* kNeedClear = "/apps/browser/NeedClearBrowsingData";
	// History, Passwords, FormData, Cookies and Cache
	const char* kClearDataItems = "/apps/browser/ClearDataItems";
	// Passwords saving
	const char* kSavePassword = "/apps/browser/SavePassword";
	// Clear history ?
	const char* kClearBrowsingHistory = "History";
	// Clear passwords ?
	const char* kClearPasswords = "Passwords";
	// Clear form data ?
	const char* kClearFormData = "FormData";
	// Clear Cookies ?
	const char* kClearCookies = "Cookies";
	// Clear Downloads ?
	const char* kClearDownloads = "Downloads";
	// Clear Cache ?
	const char* kClearCache = "Cache";
	// Allow to run javascript, bool value
	const char* kAllowJavascript = "/apps/browser/AllowJavascript";
	// Allow to pop up window, bool value
	const char* kAllowPopup = "/apps/browser/AllowPopup";
	// Allow to enable cookie, bool value
	const char* kAllowCookie = "/apps/browser/AllowCookies";
	// Allow to images
	const char* kAllowImages = "/apps/browser/AllowImages";
		// Sync Setting Dir
	const char* kSyncSettingDir = "/apps/browser/sync";
	// GMail username
	const char* kUsername = "/apps/browser/sync/Username";
	// Password
	const char* kPassword = "/apps/browser/sync/Password";
	// Last Synced Time
	const char* kLastSyncedTime = "/apps/browser/sync/LastSyncedTime";
	// Sync State
	// 0 - Not Setup Yet
	// 1 - Request Chromium to Setup Sync
	// 2 - Response Authentication Error
	// 3 - Response Sync Error
	// 4 - Response Syncing now
	// 5 - Response Sync OK
	// 6 - Request Stop Sync Perminently
	const char* kSyncStatus = "/apps/browser/sync/Status";

	const char* kNewTabURL = "chrome://newtab";

	// Page Opened on Startup
	const int	PAGE_OPENED_TYPE_DEFAULT = 1;  // Default opening home page
	const int	PAGE_OPENED_TYPE_LAST_SESSION = 2;  // Open pages opened in last session
	const int	PAGE_OPENED_TYPE_URLS = 3; // Open URLs

	// Sync State
	const int	SYNC_STATE_UNSETUP = 0;
	const int	SYNC_STATE_REQUEST_SETUP = 1; // Indicate standalone setting app wants to request sync set up to browser
	const int	SYNC_STATE_RESPONSE_AUTH_ERROR = 2;
	const int	SYNC_STATE_RESPONSE_SYNCING = 3;
	const int	SYNC_STATE_RESPONSE_DONE = 4;
	const int   SYNC_STATE_REQUEST_STOP = 5;

	// Remove Items
	const int	REMOVE_HISTORY = 0x0001;
	const int	REMOVE_DOWNLOADS = 0x0002;
	const int	REMOVE_PASSWORDS = 0x0004;
	const int	REMOVE_COOKIES = 0x0008;
	const int	REMOVE_FORM_DATA = 0x0010;
	const int	REMOVE_CACHE     = 0x0020;
}; // namespace

static int
GetFromSessionStartupPrefType(SessionStartupPref::Type type)
{
	switch(type)
	{
		case SessionStartupPref::LAST:
			return PAGE_OPENED_TYPE_LAST_SESSION;
		case SessionStartupPref::URLS:
			return PAGE_OPENED_TYPE_URLS;
		default:
			return PAGE_OPENED_TYPE_DEFAULT;
	}
}

static SessionStartupPref::Type GetFromPageOpenedType(int type)
{
	switch(type)
	{
		case PAGE_OPENED_TYPE_LAST_SESSION:
			return SessionStartupPref::LAST;
		case PAGE_OPENED_TYPE_URLS:
			return SessionStartupPref::URLS;
		default:
			return SessionStartupPref::DEFAULT;
	}
}


GConfSettingService::GConfSettingService() :
	gconf_setting_id_(-1),
	client_(NULL)
{
	g_type_init();
	client_ = gconf_client_get_default();
}

//
void GConfSettingService::Initialize(Profile* profile)
{

	if(!client_) return;

	DLOG(INFO) << "Initialize GConfSettingService ..." ;

	profile_ = profile;
	SyncPreferenceWithGConf(profile_);
	RegisterGConfNotifyFuncs();
	RegisterProfileObservers(profile_);
}

GConfSettingService::~GConfSettingService()
{
	if(client_)
	{
		gconf_client_set_bool(client_, kBrowserIsRunning, false, NULL);

		// Remove notify callback and directories watched
		DLOG(INFO) << "Removing notify funcs on gconf ...";
		gconf_client_notify_remove(client_, gconf_setting_id_);
		gconf_client_remove_dir(client_, kBrowserGConfSettingDir, NULL);
		gconf_client_remove_dir(client_, kSyncSettingDir, NULL);
	}

	ProfileSyncService* sync_service = profile()->GetProfileSyncService();

	// Remove observer on ProfileSyncService
	if(sync_service)
		sync_service->RemoveObserver(this);

	// Remove observer on TemplateURLModel
	TemplateURLModel* template_url_model = profile()->GetTemplateURLModel();
	if(template_url_model)
		template_url_model->RemoveObserver(this);

	DLOG(INFO) << "GConfSettingService destroyed";
}

// static member function for handling notify for gconf change
void GConfSettingService::OnGConfSettingChanged(GConfClient* client,
		guint cnxn_id, GConfEntry* entry, void* userdata)
{
	GConfSettingService* gconf_service =
		static_cast<GConfSettingService*>(userdata);
	if(!gconf_service) return;

	// TODO: Refactor to function table!!!
	if(!g_strcmp0(entry->key, kPageOpenedOnStartup)){ // page opened on startup

		gconf_service->UpdatePageOpenedOnStartup(gconf_service->profile());

	} else if (!g_strcmp0(entry->key, kNewTabIsHomePage)) { // new tab is home page

		gconf_service->UpdateNewTabIsHomePage(gconf_service->profile());

	} else if (!g_strcmp0(entry->key, kHomePage)) { // home page

		gconf_service->UpdateHomePage(gconf_service->profile());

	} else if (!g_strcmp0(entry->key, kShowBookmarkBar)) { // show bookmark bar ?

		gconf_service->UpdateShowBookmarkBar(gconf_service->profile());

	} else if (!g_strcmp0(entry->key, kDefaultSearchEngine)) { // default search

		gconf_service->UpdateDefaultSearchEngine(gconf_service->profile());

	} else if(!g_strcmp0(entry->key, kSavePassword)) {

			gconf_service->UpdateSavePassword(gconf_service->profile());

	} else if (!g_strcmp0(entry->key, kNeedClear)) { // Need to clear browsing data?

		gconf_service->ClearBrowsingData(gconf_service->profile());

	} else if (!g_strcmp0(entry->key, kSyncStatus)) { // Sync status
		int status = gconf_client_get_int(gconf_service->client(), kSyncStatus, NULL);
		if(status == SYNC_STATE_REQUEST_SETUP || status == SYNC_STATE_REQUEST_STOP)
				gconf_service->UpdateSyncSetting(gconf_service->profile());

	} else if (!g_strcmp0(entry->key, kAllowJavascript)) { // Allow Javascript ?

		gconf_service->UpdateAllowJavascript(gconf_service->profile());

	} else if (!g_strcmp0(entry->key, kAllowPopup)) {  // Allow Popup ?

		gconf_service->UpdateAllowPopup(gconf_service->profile());

	} else if (!g_strcmp0(entry->key, kAllowCookie)) { // Allow Cookie ?

		gconf_service->UpdateAllowCookies(gconf_service->profile());

	} else if (!g_strcmp0(entry->key, kAllowImages)) { // Allow Images ?

		gconf_service->UpdateAllowImages(gconf_service->profile());

	} else {

		DLOG(WARNING) <<" No handler for GConf key " << entry->key << " changed!!!";
	}
}

void GConfSettingService::RegisterProfileObservers(Profile* profile)
{
  PrefService* prefs = profile->GetPrefs();
  PrefChangeRegistrar registrar;
  registrar.Init(prefs);

	ProfileSyncService* sync_service = profile->GetProfileSyncService();
	HostContentSettingsMap* content_setting =
		profile->GetHostContentSettingsMap();

	registrar.Add(prefs::kRestoreOnStartup, this);

	registrar.Add(prefs::kURLsToRestoreOnStartup, this);

	registrar.Add(prefs::kHomePageIsNewTabPage, this);

	registrar.Add(prefs::kHomePage, this);

	registrar.Add(prefs::kShowBookmarkBar, this);

	registrar.Add(prefs::kDefaultSearchProviderName, this);

	registrar.Add(prefs::kSyncHasSetupCompleted, this);

//  registrar.Add(prefs::kDeleteBrowsingHistory, this);
//
//  registrar.Add(prefs::kDeletePasswords, this);
//
//  registrar.Add(prefs::kDeleteCookies, this);
//
//  registrar.Add(prefs::kDeleteCache, this);
//
//  registrar.Add(prefs::kDeleteFormData, this);
//
//  registrar.Add(prefs::kDeleteDownloadHistory, this);
	
	registrar.Add(prefs::kPasswordManagerEnabled, this);

	// Observe ContentSettings changed  on Observe method
	registrar_.Add(this, NotificationType::CONTENT_SETTINGS_CHANGED,
			Source<const HostContentSettingsMap>(content_setting));

	// Observe ProfileSyncService on OnStateChanged method
	if(sync_service)
		sync_service->AddObserver(this);
}

void GConfSettingService::OnStateChanged()
{
	ProfileSyncService* sync_service = profile()->GetProfileSyncService();
	bool completed = sync_service->HasSyncSetupCompleted();
	//int sync_status = gconf_client_get_int(client_, kSyncStatus, NULL);
	if(completed)
	{
		DLOG(INFO) << "### Sync completed...";
		string16 username = sync_service->GetAuthenticatedUsername();

		string16 last_synced_time = sync_service->GetLastSyncedTimeString();

		std::string username_utf8 = UTF16ToUTF8(username);

		std::string last_synced_time_utf8 = UTF16ToUTF8(last_synced_time);

		gconf_client_set_string(client_, kUsername, username_utf8.c_str(), NULL);

		// Unset /apps/browser/sync/Password once sync has setup successfully
		gconf_client_set_string(client_, kPassword, "", NULL);

		gconf_client_set_string(client_, kLastSyncedTime, last_synced_time_utf8.c_str(), NULL);

		gconf_client_set_int(client_, kSyncStatus, SYNC_STATE_RESPONSE_DONE, NULL);

	} else if(sync_service->SetupInProgress()) {

		gconf_client_set_int(client_, kSyncStatus, SYNC_STATE_RESPONSE_SYNCING, NULL);

	} else {
		DLOG(INFO) << "### Sync uncompleted ...";
		sync_ui_util::MessageType t = sync_ui_util::GetStatus(sync_service);
		//if(t == sync_ui_util::PRE_SYNCED)
		//	gconf_client_set_int(client_, kSyncStatus, SYNC_STATE_UNSETUP, NULL);
		if(t == sync_ui_util::SYNC_ERROR)
			gconf_client_set_int(client_, kSyncStatus, SYNC_STATE_RESPONSE_AUTH_ERROR, NULL);
	}
	return;
}

void GConfSettingService::OnTemplateURLModelChanged()
{
	SetSearchEngineList();
	UpdateDefaultSearchEngine(profile());
}

// Called when Preference changed by browser (via tab-based setting)
void GConfSettingService::Observe(NotificationType type,
		const NotificationSource& source, const NotificationDetails& details)
{
	if(type == NotificationType::PREF_CHANGED)
	{
		std::string* pref_name = Details<std::string>(details).ptr();
		if(!pref_name) return;

		DLOG(INFO) << "Received pref changed notification:" << *pref_name;

		if(*pref_name == prefs::kRestoreOnStartup ||
				*pref_name == prefs::kURLsToRestoreOnStartup)
		{
			SessionStartupPref startup_pref = SessionStartupPref::GetStartupPref(profile());
			int startup_type = GetFromSessionStartupPrefType(startup_pref.type);

			int gconf_startup_type = gconf_client_get_int(client_, kPageOpenedOnStartup, NULL);
			if(gconf_startup_type != startup_type)
			{
				gconf_client_set_int(client_, kPageOpenedOnStartup, startup_type, NULL);
			}
		} else if(*pref_name == prefs::kHomePageIsNewTabPage) {
			gboolean new_tab_is_home_page = gconf_client_get_bool(client_, kNewTabIsHomePage, NULL);
			bool b = profile()->GetPrefs()->GetBoolean(prefs::kHomePageIsNewTabPage);
			if(b != new_tab_is_home_page){
					gconf_client_set_bool(client_, kNewTabIsHomePage, b, NULL);
			}

		}	else if (*pref_name == prefs::kHomePage) {
			std::string homepage = profile()->GetPrefs()->GetString(prefs::kHomePage);
			char* gconf_homepage = gconf_client_get_string(client_, kHomePage, NULL);
			if(g_strcmp0(homepage.c_str(), gconf_homepage))
			{
				gconf_client_set_string(client_, kHomePage, homepage.c_str(), NULL);
			}
			g_free(gconf_homepage);

		} else if (*pref_name == prefs::kShowBookmarkBar) {
			bool pref_showbookmarkbar = profile()->GetPrefs()->GetBoolean(prefs::kShowBookmarkBar);
			gboolean gconf_showbookmarkbar = gconf_client_get_bool(client_, kShowBookmarkBar, NULL);
			if(pref_showbookmarkbar != gconf_showbookmarkbar)
			{
				gconf_client_set_bool(client_, kShowBookmarkBar, pref_showbookmarkbar, NULL);
			}
		} else if (*pref_name == prefs::kDefaultSearchProviderName) {
			std::string pref_search_engine = profile()->GetPrefs()->GetString(prefs::kDefaultSearchProviderName);
			char* gconf_search_engine = gconf_client_get_string(client_, kDefaultSearchEngine, NULL);
			DLOG(INFO) << pref_search_engine << " VS. " << gconf_search_engine;

			// Only change gconf when different
			if(g_strcmp0(pref_search_engine.c_str(), gconf_search_engine))
			{
				gconf_client_set_string(client_, kDefaultSearchEngine, pref_search_engine.c_str(), NULL);
			}
			g_free(gconf_search_engine);
		} else if (*pref_name == prefs::kSyncHasSetupCompleted) {
			bool sync_completed = profile()->GetPrefs()->GetBoolean(prefs::kSyncHasSetupCompleted);
			//int sync_status = gconf_client_get_int(client_, kSyncStatus, NULL);

			// Browser has completed sync process
			// Update username and last sync time to GConf
			if(sync_completed)
			{
				ProfileSyncService* sync_service = profile()->GetProfileSyncService();
				if(sync_service)
				{
					string16 username = sync_service->GetAuthenticatedUsername();
					string16 last_synced_time = sync_service->GetLastSyncedTimeString();
					std::string username_utf8 = UTF16ToUTF8(username);
					std::string last_synced_time_utf8 = UTF16ToUTF8(last_synced_time);

					gconf_client_set_string(client_, kUsername, username_utf8.c_str(), NULL);
					gconf_client_set_string(client_, kLastSyncedTime, last_synced_time_utf8.c_str(), NULL);
				}
				gconf_client_set_int(client_, kSyncStatus, SYNC_STATE_RESPONSE_DONE, NULL);
			}

			if(!sync_completed)
			{
				gconf_client_set_int(client_, kSyncStatus, SYNC_STATE_UNSETUP, NULL);
				gconf_client_set_string(client_, kUsername, "", NULL);
				gconf_client_set_string(client_, kLastSyncedTime, "", NULL);
			}

		} else if(*pref_name == prefs::kPasswordManagerEnabled) {
				bool enabled = profile()->GetPrefs()->GetBoolean(prefs::kPasswordManagerEnabled);

				bool gconf_enabled = gconf_client_get_bool(client_, kSavePassword, NULL);
				if(enabled != gconf_enabled)
						gconf_client_set_bool(client_, kSavePassword, enabled, NULL);
		} else {
			// Ignore ...
			DLOG(INFO) << "Ignore " << *pref_name << " changed";
		}

	}
	if(type == NotificationType::CONTENT_SETTINGS_CHANGED)
	{
		HostContentSettingsMap* setting_map =
			profile()->GetHostContentSettingsMap();

		const ContentSettingsDetails* setting_details =
			static_cast<Details<const ContentSettingsDetails> >(details).ptr();

		DLOG(INFO) << "Received ContentSetting changed notification: " <<
			setting_details->type();

		ContentSetting setting = setting_map->GetDefaultContentSetting(
				setting_details->type());
		gboolean val = (setting == CONTENT_SETTING_ALLOW ? TRUE : FALSE);

		switch(setting_details->type())
		{
			case CONTENT_SETTINGS_TYPE_JAVASCRIPT:
				if(gconf_client_get_bool(client_, kAllowJavascript, NULL) != val)
					gconf_client_set_bool(client_, kAllowJavascript, val, NULL);
				break;
			case CONTENT_SETTINGS_TYPE_POPUPS:
				if(gconf_client_get_bool(client_, kAllowPopup, NULL) != val)
					gconf_client_set_bool(client_, kAllowPopup, val, NULL);
				break;
			case CONTENT_SETTINGS_TYPE_IMAGES:
				if(gconf_client_get_bool(client_, kAllowImages, NULL) != val)
					gconf_client_set_bool(client_, kAllowImages, val, NULL);
				break;
			case CONTENT_SETTINGS_TYPE_COOKIES:
				if(gconf_client_get_bool(client_, kAllowCookie, NULL) != val)
					gconf_client_set_bool(client_, kAllowCookie, val, NULL);
				break;
			default:
				break;

		};
	}
}

void GConfSettingService::AddDir(const char* dirname)
{
	if(!dirname || !client_) return;
	GError* error = NULL;
	gconf_client_add_dir(client_, dirname,
			GCONF_CLIENT_PRELOAD_NONE, &error);

	if(error)
	{
		LOG(WARNING) << "failed to add " << kBrowserGConfSettingDir
			<< ":" << error->message;
		g_error_free(error);
		error = NULL;
	}
}

// Initialize the gconf client and add the notify callback for
// handle gconf change notification
void GConfSettingService::RegisterGConfNotifyFuncs()
{
	AddDir(kBrowserGConfSettingDir);
	AddDir(kSyncSettingDir);

	DLOG(INFO) << "Register gconf notification callback";
	gconf_setting_id_ = gconf_client_notify_add(client_,
			kBrowserGConfSettingDir,
			static_cast<GConfClientNotifyFunc>(GConfSettingService::OnGConfSettingChanged),
			this, // user data
			NULL, // free func
			NULL);

	// Set BrowserRunning to be true
	gconf_client_set_bool(client_, kBrowserIsRunning, true, NULL);
}

static void free_data(void* data, void* userdata)
{
	g_free(data);
}

void GConfSettingService::SetSearchEngineList()
{
	std::vector<const TemplateURL*> model_urls;
	// Set search engine list in GConf
	TemplateURLModel* template_url_model = profile()->GetTemplateURLModel();
	if(template_url_model)
	{
		model_urls = template_url_model->GetTemplateURLs();
	}

	GSList* search_provider_list = NULL;
	for(unsigned int i = 0; i < model_urls.size(); i++)
	{
		std::string utf8name;
		utf8name = UTF16ToUTF8(model_urls[i]->short_name());
		search_provider_list = g_slist_append(search_provider_list,
				(gpointer)g_strdup(utf8name.c_str()));
	}
	if(search_provider_list)
	{
		gconf_client_set_list(client_, kSearchEngineList,
				GCONF_VALUE_STRING, search_provider_list, NULL);
		g_slist_foreach(search_provider_list, (GFunc)free_data, NULL);
		g_slist_free(search_provider_list);
		gconf_client_set_bool(client_, kSearchEngineListUpdated, TRUE, NULL);
	}
}

void GConfSettingService::SetDefaultGConfValue()
{
	DCHECK(client_);

	// Ignore if the setting dir exists
	if(gconf_client_dir_exists(client_, kBrowserGConfSettingDir, NULL))
		return;

	PrefService* prefs = profile()->GetPrefs();
	// Page opened on startup
	gconf_client_set_int(client_, kPageOpenedOnStartup, PAGE_OPENED_TYPE_DEFAULT, NULL);

	// home page group
	bool new_tab_is_home_page = prefs->GetBoolean(prefs::kHomePageIsNewTabPage);
	gconf_client_set_bool(client_, kNewTabIsHomePage, new_tab_is_home_page, NULL);

	std::string homepage = prefs->GetString(prefs::kHomePage);
	gconf_client_set_string(client_, kHomePage, homepage.c_str(), NULL);

	// Default showing bookmark
	gconf_client_set_bool(client_, kShowBookmarkBar,
			prefs->GetBoolean(prefs::kShowBookmarkBar), NULL);

	// Save passwords ?
	gconf_client_set_bool(client_, kSavePassword,
					prefs->GetBoolean(prefs::kPasswordManagerEnabled), NULL);

	// Get default search engine from chromium preference
	// and set the key value in gconf
	std::string search_engine =
		profile()->GetPrefs()->GetString(prefs::kDefaultSearchProviderName);
	gconf_client_set_string(client_, kDefaultSearchEngine,
			search_engine.c_str(), NULL);

	SetSearchEngineList();

	// Set default value for ContentSettings
	HostContentSettingsMap* settings = profile()->GetHostContentSettingsMap();
	gconf_client_set_bool(client_, kAllowJavascript,
			settings->GetDefaultContentSetting(CONTENT_SETTINGS_TYPE_JAVASCRIPT) == CONTENT_SETTING_ALLOW ? TRUE : FALSE,
			NULL);
	gconf_client_set_bool(client_, kAllowCookie,
			settings->GetDefaultContentSetting(CONTENT_SETTINGS_TYPE_COOKIES) == CONTENT_SETTING_ALLOW ? TRUE : FALSE,
			NULL);
	gconf_client_set_bool(client_, kAllowImages,
			settings->GetDefaultContentSetting(CONTENT_SETTINGS_TYPE_IMAGES) == CONTENT_SETTING_ALLOW ? TRUE : FALSE,
			NULL);
	gconf_client_set_bool(client_, kAllowPopup,
			settings->GetDefaultContentSetting(CONTENT_SETTINGS_TYPE_POPUPS) == CONTENT_SETTING_ALLOW ? TRUE : FALSE,
			NULL);

	// Clear Data ?
	gconf_client_set_bool(client_, kNeedClear, FALSE, NULL);
	
	// Set Sync state as intial status
	gconf_client_set_int(client_, kSyncStatus, SYNC_STATE_UNSETUP, NULL);
	return;
}

void GConfSettingService::SetClearBrowsingDataItems()
{
	PrefService* prefs = profile()->GetPrefs();
	GSList* data_items = NULL;
	if(prefs->GetBoolean(prefs::kDeleteBrowsingHistory))
		data_items = g_slist_prepend(data_items, (void*)kClearBrowsingHistory);
	if(prefs->GetBoolean(prefs::kDeleteDownloadHistory))
		data_items = g_slist_prepend(data_items, (void*)kClearDownloads);
	if(prefs->GetBoolean(prefs::kDeletePasswords))
		data_items = g_slist_prepend(data_items, (void*)kClearPasswords);
	if(prefs->GetBoolean(prefs::kDeleteFormData))
		data_items = g_slist_prepend(data_items, (void*)kClearFormData);
	if(prefs->GetBoolean(prefs::kDeleteCookies))
		data_items = g_slist_prepend(data_items, (void*)kClearCookies);
	if(prefs->GetBoolean(prefs::kDeleteCache))
		data_items = g_slist_prepend(data_items, (void*)kClearCache);

	gconf_client_set_list(client_, kClearDataItems, GCONF_VALUE_STRING,
			data_items, NULL);

	g_slist_free(data_items);

}
void GConfSettingService::SyncPreferenceWithGConf(Profile* profile)
{
	if(!profile) return;

	Profile* prof = profile;

	TemplateURLModel* template_url_model = profile->GetTemplateURLModel();
	if(template_url_model) {
		template_url_model->Load();
		template_url_model->AddObserver(this);
	}

	// No gconf dir for browser found due to first usage of browser and
	// standalone setting app has not been launched. In this case, we
	// set the default value for each key in gconf setting according to
	// chromium's preference
	if(!gconf_client_dir_exists(client_, kBrowserGConfSettingDir, NULL))
	{
		SetDefaultGConfValue();
		return;
	}

	// GConf setting for browser is ready. We check gconf setting
	// item by item and update chromium preference to gconf setting
	// for different ones.
	UpdatePageOpenedOnStartup(prof);
	UpdateNewTabIsHomePage(prof);
	UpdateHomePage(prof);
	UpdateShowBookmarkBar(prof);
	UpdateDefaultSearchEngine(prof);

	// Update Content Settings
	UpdateAllowJavascript(prof);
	UpdateAllowImages(prof);
	UpdateAllowCookies(prof);
	UpdateAllowPopup(prof);

	UpdateSavePassword(prof);

	// Clear data?
	ClearBrowsingData(prof);

	SetSearchEngineList();
	UpdateSyncSetting(prof);
	// Save preference to persistent storage
	profile->GetPrefs()->ScheduleSavePersistentPrefs();
}

void GConfSettingService::UpdatePageOpenedOnStartup(Profile* profile)
{
	DLOG(INFO) << "[GConfSettingService]UpdatePageOpenedOnStartup";
	if(!profile) return;

	PrefService* prefs = profile->GetPrefs();
	if(!prefs) return;

	// Sync Page Opened Setting on startup
	int kPageOpenedOnStartupValue = (gconf_client_get_int(client_,
			kPageOpenedOnStartup, NULL));

	const SessionStartupPref startup_pref =
			SessionStartupPref::GetStartupPref(prefs);

	// Actually, GConf definitely has the value for kPageOpenedOnStartup
	// due to SetDefaultGConfValue
	{
		SessionStartupPref::Type startup_type = GetFromPageOpenedType(kPageOpenedOnStartupValue);
		if(startup_pref.type != startup_type)
		{
			SessionStartupPref pref;
			pref.type = startup_type;
			SessionStartupPref::SetStartupPref(prefs, pref);
		}
	}
}

void GConfSettingService::UpdateNewTabIsHomePage(Profile* profile)
{
		if(profile == NULL) return;

		DLOG(INFO) << "[GConfSettingService] UpdateNewTabIsHomePage ";

		bool new_tab_is_home_page = gconf_client_get_bool(client_, kNewTabIsHomePage, NULL);

		// Same value, just return
		if(new_tab_is_home_page == profile->GetPrefs()->GetBoolean(prefs::kHomePageIsNewTabPage))
				return;

		// Set new value
		profile->GetPrefs()->SetBoolean(prefs::kHomePageIsNewTabPage, new_tab_is_home_page);

		// Also update home page value
		if(!new_tab_is_home_page) {
				char* homepage = gconf_client_get_string(client_, kHomePage, NULL);
				if(homepage) {
						GURL homepage_url = URLFixerUpper::FixupURL(homepage, std::string());
						profile->GetPrefs()->SetString(prefs::kHomePage, homepage_url.spec());
				}
		}
		return;
}

void GConfSettingService::UpdateHomePage(Profile* profile)
{
	if(profile == NULL) return;

	DLOG(INFO) << "[GConfSettingService] UpdateHomePage";

	PrefService* prefs = profile->GetPrefs();

	// Read Value from GConf
	bool new_tab_is_home_page = gconf_client_get_bool(client_, kNewTabIsHomePage, NULL);

	// No update to home page if new tabe is home page
	if(new_tab_is_home_page) return;

	char* gconf_homepage = gconf_client_get_string(client_, kHomePage, NULL);

	if(gconf_homepage == NULL) return;

	std::string homepage(gconf_homepage);

	// If home page is set to "chrome://newtab"
	if(!new_tab_is_home_page && !g_ascii_strncasecmp(gconf_homepage, kNewTabURL, sizeof(kNewTabURL))) {
			prefs->SetBoolean(prefs::kHomePageIsNewTabPage, true);
			prefs->SetString(prefs::kHomePage, std::string());
			return;
	}

	std::string pref_homepage = prefs->GetString(prefs::kHomePage);
	if(pref_homepage == gconf_homepage) return;

	// Check home page url. If it is invalid, use new tab as home page again
	if(gconf_homepage) {
		 GURL homepage_url = URLFixerUpper::FixupURL(gconf_homepage, std::string());
		 prefs->SetBoolean(prefs::kHomePageIsNewTabPage, false);
		 prefs->SetString(prefs::kHomePage, homepage_url.spec());
	}
	g_free(gconf_homepage);
}

void GConfSettingService::UpdateShowBookmarkBar(Profile* profile)
{
	if(!profile) return;

	// Due to gconf_client_get_bool return FALSE if the key is unset
	// How to distingish the unset key and the FALSE key??
	gboolean gconf_showbookmarkbar = gconf_client_get_bool(client_, kShowBookmarkBar, NULL);

	bool showbookmarkbar = profile->GetPrefs()->GetBoolean(prefs::kShowBookmarkBar);
	if(showbookmarkbar != gconf_showbookmarkbar)
	{
		profile->GetPrefs()->SetBoolean(prefs::kShowBookmarkBar, gconf_showbookmarkbar);
		// Notify the notification service
		Source<Profile> source(profile);
		NotificationService::current()->Notify(
				NotificationType::BOOKMARK_BAR_VISIBILITY_PREF_CHANGED,
				source,
				NotificationService::NoDetails());
	}

	return;
}

void GConfSettingService::UpdateDefaultSearchEngine(Profile* profile)
{
	if(!profile) return;

	std::vector<const TemplateURL*> model_urls;

	TemplateURLModel* template_url_model = profile->GetTemplateURLModel();

	if(template_url_model)
	{
		model_urls =  template_url_model->GetTemplateURLs();
	}

	// Check Default Search Engine
	char* gconf_shortname = NULL;
	gconf_shortname = gconf_client_get_string(client_, kDefaultSearchEngine, NULL);

	if(gconf_shortname){
		string16 default_search_engine = GetDefaultSearchEngineName(profile);
		std::string default_search_engine_utf8 = UTF16ToUTF8(default_search_engine);

		if(g_strcmp0(gconf_shortname, default_search_engine_utf8.c_str()) != 0)
		{
				// Have different search engine in gconf and preference
				unsigned int i = 0 ;
				for(i = 0; i < model_urls.size(); i++)
				{
						std::string utf8name;
						utf8name = UTF16ToUTF8(model_urls[i]->short_name());

						if(!g_strcmp0(utf8name.c_str(), gconf_shortname)){
								template_url_model->SetDefaultSearchProvider(model_urls[i]);
								break;
						}
				}
				// In case of that gconf specifies a non-existing search provider
				// Correct it here
				if(i >= model_urls.size() && model_urls.size() > 0)
				{
						gconf_client_set_string(client_, kDefaultSearchEngine,
										default_search_engine_utf8.c_str(), NULL);
				}
		}
		if(gconf_shortname) g_free(gconf_shortname);
	}
}

// Update AllowJavacript setting
void GConfSettingService::UpdateAllowJavascript(Profile* profile)
{
	if(!profile) return;

	HostContentSettingsMap* map = profile->GetHostContentSettingsMap();

	gboolean allow_javascript = gconf_client_get_bool(client_,
			kAllowJavascript, NULL);
	if(!(allow_javascript == TRUE &&
				map->GetDefaultContentSetting(CONTENT_SETTINGS_TYPE_JAVASCRIPT) == CONTENT_SETTING_ALLOW)
			|| !(allow_javascript == FALSE &&
				map->GetDefaultContentSetting(CONTENT_SETTINGS_TYPE_JAVASCRIPT) == CONTENT_SETTING_BLOCK))
	{
		map->SetDefaultContentSetting(
				CONTENT_SETTINGS_TYPE_JAVASCRIPT,
				allow_javascript ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK);
	}

}

// Update AllowPopup setting in profile
void GConfSettingService::UpdateAllowPopup(Profile* profile)
{
	if(!profile) return;

	HostContentSettingsMap* map = profile->GetHostContentSettingsMap();

	// Allow popup?
	gboolean allow_popup = gconf_client_get_bool(client_,
			kAllowPopup, NULL);
	if(!(allow_popup == TRUE &&
				map->GetDefaultContentSetting(CONTENT_SETTINGS_TYPE_POPUPS) == CONTENT_SETTING_ALLOW)
			|| !(allow_popup == FALSE &&
				map->GetDefaultContentSetting(CONTENT_SETTINGS_TYPE_POPUPS) == CONTENT_SETTING_BLOCK))
	{
		map->SetDefaultContentSetting(
				CONTENT_SETTINGS_TYPE_POPUPS,
				allow_popup ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK);
	}
}

// Update AllowCookies
void GConfSettingService::UpdateAllowCookies(Profile* profile)
{
	if(!profile) return;

	HostContentSettingsMap* map = profile->GetHostContentSettingsMap();

	// Allow Cookie?
	gboolean allow_cookie = gconf_client_get_bool(client_,
			kAllowCookie, NULL);
	if(!(allow_cookie == TRUE &&
				map->GetDefaultContentSetting(CONTENT_SETTINGS_TYPE_COOKIES) == CONTENT_SETTING_ALLOW)
			|| !(allow_cookie == FALSE &&
				map->GetDefaultContentSetting(CONTENT_SETTINGS_TYPE_COOKIES) == CONTENT_SETTING_BLOCK))
	{
		map->SetDefaultContentSetting(
				CONTENT_SETTINGS_TYPE_COOKIES,
				allow_cookie ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK);
	}

}

// Update AllowImages
void GConfSettingService::UpdateAllowImages(Profile* profile)
{
	if(!profile) return;

	HostContentSettingsMap* map = profile->GetHostContentSettingsMap();


	// Allow Image?
	gboolean allow_image = gconf_client_get_bool(client_,
			kAllowImages, NULL);
	if(!(allow_image == TRUE &&
				map->GetDefaultContentSetting(CONTENT_SETTINGS_TYPE_IMAGES) == CONTENT_SETTING_ALLOW)
			|| !(allow_image == FALSE &&
				map->GetDefaultContentSetting(CONTENT_SETTINGS_TYPE_IMAGES) == CONTENT_SETTING_BLOCK))
	{
		map->SetDefaultContentSetting(
				CONTENT_SETTINGS_TYPE_IMAGES,
				allow_image ? CONTENT_SETTING_ALLOW : CONTENT_SETTING_BLOCK);
	}
	return;
}

// Save password?
void GConfSettingService::UpdateSavePassword(Profile* profile)
{
		if(!profile) return;
		PrefService* prefs = profile->GetPrefs();

		gboolean save_password = gconf_client_get_bool(client_, kSavePassword, NULL);

		bool pref_save_password = prefs->GetBoolean(prefs::kPasswordManagerEnabled);
		if(pref_save_password != save_password)
		{
				prefs->SetBoolean(prefs::kPasswordManagerEnabled, save_password);
		}
}

void GConfSettingService::OnBrowsingDataRemoverDone()
{
		bool need_clear = gconf_client_get_bool(client_, kNeedClear, NULL);
		if(need_clear) {
				ClearBrowsingData(profile());
		}
}

// Clear browsing data
void GConfSettingService::ClearBrowsingData(Profile* profile)
{
	if(!profile) return;
	PrefService* prefs = profile->GetPrefs();
	int remove_items = 0;

	bool need_clear = gconf_client_get_bool(client_, kNeedClear, NULL);
	if(!need_clear) return;

	GSList* data_items = gconf_client_get_list(client_, kClearDataItems,
			GCONF_VALUE_STRING, NULL);
	GSList* item = data_items;
	for(item = data_items;item != NULL;item = g_slist_next(item))
	{
		char* ss = static_cast<char*>(item->data);
		if(g_strcmp0(ss, kClearBrowsingHistory) == 0){
			remove_items |= BrowsingDataRemover::REMOVE_HISTORY;
			prefs->SetBoolean(prefs::kDeleteBrowsingHistory, true);
		}
		if(g_strcmp0(ss, kClearPasswords) == 0) {
			remove_items |= BrowsingDataRemover::REMOVE_PASSWORDS;
			prefs->SetBoolean(prefs::kDeletePasswords, true);
		}
		if(g_strcmp0(ss, kClearDownloads) == 0) {
			remove_items |= BrowsingDataRemover::REMOVE_DOWNLOADS;
			prefs->SetBoolean(prefs::kDeleteDownloadHistory, true);
		}
		if(g_strcmp0(ss, kClearCookies) == 0){
			remove_items |= BrowsingDataRemover::REMOVE_COOKIES;
			prefs->SetBoolean(prefs::kDeleteCookies, true);
		}
		if(g_strcmp0(ss, kClearCache) == 0) {
			remove_items |= BrowsingDataRemover::REMOVE_CACHE;
			prefs->SetBoolean(prefs::kDeleteCache, true);
		}
		if(g_strcmp0(ss, kClearFormData) == 0) {
			remove_items |= BrowsingDataRemover::REMOVE_FORM_DATA;
			prefs->SetBoolean(prefs::kDeleteFormData, true);
		}
	}
	g_slist_foreach(data_items, free_data, NULL);
	g_slist_free(data_items);
	// remover deletes itself when done
	DLOG(INFO) <<" Start to clear browsing history ....";

	// Avoid duplicate removing worker running at the same time
	if(BrowsingDataRemover::is_removing()) return;
	// Set NeedClearBrowsing data to false
	gconf_client_set_bool(client_, kNeedClear, FALSE, NULL);
	
	BrowsingDataRemover* remover =
		new BrowsingDataRemover(profile, BrowsingDataRemover::EVERYTHING, base::Time());
	remover->AddObserver(this);
	remover->Remove(remove_items);
}

// If kSyncStatus is set to SYNC_STATE_REQUEST_SETUP,
// This function will retrieve account info from gconf and try
// to setup sync process
void GConfSettingService::UpdateSyncSetting(Profile* profile)
{
	if(!profile) return;
	ProfileSyncService* sync_service = profile->GetProfileSyncService();

	if(sync_service)
	{
		bool completed = sync_service->HasSyncSetupCompleted();
		int sync_status = gconf_client_get_int(client_, kSyncStatus, NULL);

		// If sync completed and standalone setting app request stop perminently
		if(completed && sync_status == SYNC_STATE_REQUEST_STOP) {
			gconf_client_set_int(client_, kSyncStatus, SYNC_STATE_UNSETUP, NULL);
			sync_service->DisableForUser();
			ProfileSyncService::SyncEvent(ProfileSyncService::STOP_FROM_OPTIONS);
			//sync_service->OnStopSyncingPermanently();
			return;
		}

		if(sync_status == SYNC_STATE_REQUEST_SETUP)
		{
				gsize len = 0;

				char* username = gconf_client_get_string(client_, kUsername, NULL);
				if(username == NULL) return;

				char* base64_password = gconf_client_get_string(client_, kPassword, NULL);
				if(base64_password == NULL) {
						g_free(username);
						return;
				}

				gchar* password = (gchar*)g_base64_decode(base64_password, &len);

				// If sync completed and standalone setting app wants to setup with another account
				// disable the old one and setup sync with new account
				if(completed) {
						string16 auth_username = sync_service->GetAuthenticatedUsername();
						std::string username_utf8 = UTF16ToUTF8(auth_username);
						if(g_strcmp0(username, username_utf8.c_str()) != 0 && password) // ignore same username
						{
								sync_service->DisableForUser();
								ProfileSyncService::SyncEvent(ProfileSyncService::STOP_FROM_OPTIONS);
						}
				}

				// try to sync now
				gconf_client_set_int(client_, kSyncStatus, SYNC_STATE_RESPONSE_SYNCING, NULL);
				//sync_service->EnableForUser(NULL);
				ProfileSyncService::SyncEvent(ProfileSyncService::START_FROM_OPTIONS);
				sync_service->OnUserSubmittedAuth(username, password, "", "");

				g_free(username);
				g_free(base64_password);
				g_free(password);
		}
	}
}

bool GConfSettingService::GetNewTabIsHomePage(bool* new_tab_is_homepage)
{
		if(new_tab_is_homepage == NULL) return false;

		GError* error = NULL;

		*new_tab_is_homepage =  gconf_client_get_bool(client_, kNewTabIsHomePage, &error);
		if(error){
				g_error_free(error);
				return false;
		}
		else return true;
}

GURL GConfSettingService::GetHomePage()
{
		char* s = gconf_client_get_string(client_, kHomePage, NULL);
		std::string homepage;
		if(s) homepage = s;
		else homepage = kNewTabURL;
		GURL url(homepage);
		g_free(s);
		return url;

}
