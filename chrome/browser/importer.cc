// Copyright 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "chrome/browser/importer.h"

#include <map>

#include "base/file_util.h"
#include "base/gfx/image_operations.h"
#include "base/gfx/png_encoder.h"
#include "base/string_util.h"
#include "chrome/browser/bookmark_bar_model.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/firefox2_importer.h"
#include "chrome/browser/firefox3_importer.h"
#include "chrome/browser/firefox_importer_utils.h"
#include "chrome/browser/firefox_profile_lock.h"
#include "chrome/browser/ie_importer.h"
#include "chrome/browser/template_url_model.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/webdata/web_data_service.h"
#include "chrome/common/gfx/favicon_size.h"
#include "chrome/common/l10n_util.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/pref_service.h"
#include "chrome/views/window.h"
#include "webkit/glue/image_decoder.h"

#include "generated_resources.h"

// ProfileWriter.

bool ProfileWriter::BookmarkBarModelIsLoaded() const {
  return profile_->GetBookmarkBarModel()->IsLoaded();
}

void ProfileWriter::AddBookmarkBarModelObserver(
    BookmarkBarModelObserver* observer) {
  profile_->GetBookmarkBarModel()->AddObserver(observer);
}

bool ProfileWriter::TemplateURLModelIsLoaded() const {
  return profile_->GetTemplateURLModel()->loaded();
}

void ProfileWriter::AddTemplateURLModelObserver(
    NotificationObserver* observer) {
  TemplateURLModel* model = profile_->GetTemplateURLModel();
  NotificationService::current()->AddObserver(
      observer, TEMPLATE_URL_MODEL_LOADED,
      Source<TemplateURLModel>(model));
  model->Load();
}

void ProfileWriter::AddPasswordForm(const PasswordForm& form) {
  profile_->GetWebDataService(Profile::EXPLICIT_ACCESS)->AddLogin(form);
}

void ProfileWriter::AddIE7PasswordInfo(const IE7PasswordInfo& info) {
  profile_->GetWebDataService(Profile::EXPLICIT_ACCESS)->AddIE7Login(info);
}

void ProfileWriter::AddHistoryPage(const std::vector<history::URLRow>& page) {
  profile_->GetHistoryService(Profile::EXPLICIT_ACCESS)->
      AddPagesWithDetails(page);
}

void ProfileWriter::AddHomepage(const GURL& home_page) {
  DCHECK(profile_);

  PrefService* prefs = profile_->GetPrefs();
  // NOTE: We set the kHomePage value, but keep the NewTab page as the homepage.
  prefs->SetString(prefs::kHomePage, ASCIIToWide(home_page.spec()));
  prefs->ScheduleSavePersistentPrefs(g_browser_process->file_thread());
}

void ProfileWriter::AddBookmarkEntry(
    const std::vector<BookmarkEntry>& bookmark) {
  BookmarkBarModel* model = profile_->GetBookmarkBarModel();
  DCHECK(model->IsLoaded());

  bool show_bookmark_toolbar = false;
  std::set<BookmarkBarNode*> groups_added_to;
  for (std::vector<BookmarkEntry>::const_iterator it = bookmark.begin();
       it != bookmark.end(); ++it) {
    // Don't insert this url if it exists in model or url is not valid.
    if (model->GetNodeByURL(it->url) != NULL || !it->url.is_valid())
      continue;

    // Set up groups in BookmarkBarModel in such a way that path[i] is
    // the subgroup of path[i-1]. Finally they construct a path in the
    // model:
    //   path[0] \ path[1] \ ... \ path[size() - 1]
    BookmarkBarNode* parent =
        (it->in_toolbar ? model->GetBookmarkBarNode() : model->other_node());
    for (std::vector<std::wstring>::const_iterator i = it->path.begin();
         i != it->path.end(); ++i) {
      BookmarkBarNode* child = NULL;
      for (int index = 0; index < parent->GetChildCount(); ++index) {
        BookmarkBarNode* node = parent->GetChild(index);
        if ((node->GetType() == history::StarredEntry::BOOKMARK_BAR ||
             node->GetType() == history::StarredEntry::USER_GROUP) &&
            node->GetTitle() == *i) {
          child = node;
          break;
        }
      }
      if (child == NULL)
        child = model->AddGroup(parent, parent->GetChildCount(), *i);
      parent = child;
    }
    groups_added_to.insert(parent);
    model->AddURLWithCreationTime(parent, parent->GetChildCount(),
        it->title, it->url, it->creation_time);

    // If some items are put into toolbar, it looks like the user was using
    // it in their last browser. We turn on the bookmarks toolbar.
    if (it->in_toolbar)
      show_bookmark_toolbar = true;
  }

  // Reset the date modified time of the groups we added to. We do this to
  // make sure the 'recently added to' combobox in the bubble doesn't get random
  // groups.
  for (std::set<BookmarkBarNode*>::const_iterator i = groups_added_to.begin();
       i != groups_added_to.end(); ++i) {
    model->ResetDateGroupModified(*i);
  }

  if (show_bookmark_toolbar)
    ShowBookmarkBar();
}

void ProfileWriter::AddFavicons(
    const std::vector<history::ImportedFavIconUsage>& favicons) {
  profile_->GetHistoryService(Profile::EXPLICIT_ACCESS)->
      SetImportedFavicons(favicons);
}

typedef std::map<std::string, const TemplateURL*> HostPathMap;

// Builds the key to use in HostPathMap for the specified TemplateURL. Returns
// an empty string if a host+path can't be generated for the TemplateURL.
// If an empty string is returned, it should not be added to HostPathMap.
static std::string BuildHostPathKey(const TemplateURL* t_url) {
  if (t_url->url() && t_url->url()->SupportsReplacement()) {
    GURL search_url(t_url->url()->ReplaceSearchTerms(
        *t_url, L"random string", TemplateURLRef::NO_SUGGESTIONS_AVAILABLE,
        std::wstring()));
    if (search_url.is_valid())
      return search_url.host() + search_url.path();
  }
  return std::string();
}

// Builds a set that contains an entry of the host+path for each TemplateURL in
// the TemplateURLModel that has a valid search url.
static void BuildHostPathMap(const TemplateURLModel& model,
                             HostPathMap* host_path_map) {
  std::vector<const TemplateURL*> template_urls = model.GetTemplateURLs();
  for (size_t i = 0; i < template_urls.size(); ++i) {
    const std::string host_path = BuildHostPathKey(template_urls[i]);
    if (!host_path.empty()) {
      const TemplateURL* existing_turl = (*host_path_map)[host_path];
      if (!existing_turl || template_urls[i]->show_in_default_list()) {
        // If there are multiple TemplateURLs with the same host+path, favor
        // those show in the default list. This is done just in case we end
        // up using it as the default search provider.
        (*host_path_map)[host_path] = template_urls[i];
      }
    }  // else case, TemplateURL doesn't have a search url, doesn't support
       // replacement, or doesn't have valid GURL. Ignore it.
  }
}

void ProfileWriter::AddKeywords(const std::vector<TemplateURL*>& template_urls,
                                int default_keyword_index,
                                bool unique_on_host_and_path) {
  TemplateURLModel* model = profile_->GetTemplateURLModel();
  HostPathMap host_path_map;
  if (unique_on_host_and_path)
    BuildHostPathMap(*model, &host_path_map);

  for (std::vector<TemplateURL*>::const_iterator i = template_urls.begin();
       i != template_urls.end(); ++i) {
    TemplateURL* t_url = *i;
    bool default_keyword =
        default_keyword_index >= 0 &&
        (i - template_urls.begin() == default_keyword_index);

    // TemplateURLModel requires keywords to be unique. If there is already a
    // TemplateURL with this keyword, don't import it again.
    const TemplateURL* turl_with_keyword =
        model->GetTemplateURLForKeyword(t_url->keyword());
    if (turl_with_keyword != NULL) {
      if (default_keyword)
        model->SetDefaultSearchProvider(turl_with_keyword);
      delete t_url;
      continue;
    }

    // For search engines if there is already a keyword with the same
    // host+path, we don't import it. This is done to avoid both duplicate
    // search providers (such as two Googles, or two Yahoos) as well as making
    // sure the search engines we provide aren't replaced by those from the
    // imported browser.
    if (unique_on_host_and_path &&
        host_path_map.find(BuildHostPathKey(t_url)) != host_path_map.end()) {
      if (default_keyword) {
        const TemplateURL* turl_with_host_path =
            host_path_map[BuildHostPathKey(t_url)];
        if (turl_with_host_path)
          model->SetDefaultSearchProvider(turl_with_host_path);
        else
          NOTREACHED();  // BuildHostPathMap should only insert non-null values.
      }
      delete t_url;
      continue;
    }
    model->Add(t_url);
    if (default_keyword)
      model->SetDefaultSearchProvider(t_url);
  }
}

void ProfileWriter::ShowBookmarkBar() {
  DCHECK(profile_);

  PrefService* prefs = profile_->GetPrefs();
  // Check whether the bookmark bar is shown in current pref.
  if (!prefs->GetBoolean(prefs::kShowBookmarkBar)) {
    // Set the pref and notify the notification service.
    prefs->SetBoolean(prefs::kShowBookmarkBar, true);
    prefs->ScheduleSavePersistentPrefs(g_browser_process->file_thread());
    Source<Profile> source(profile_);
    NotificationService::current()->Notify(
        NOTIFY_BOOKMARK_BAR_VISIBILITY_PREF_CHANGED, source,
        NotificationService::NoDetails());
  }
}

// Importer.

// static
bool Importer::ReencodeFavicon(const unsigned char* src_data, size_t src_len,
                               std::vector<unsigned char>* png_data) {
  // Decode the favicon using WebKit's image decoder.
  webkit_glue::ImageDecoder decoder(gfx::Size(kFavIconSize, kFavIconSize));
  SkBitmap decoded = decoder.Decode(src_data, src_len);
  if (decoded.empty())
    return false;  // Unable to decode.

  if (decoded.width() != kFavIconSize || decoded.height() != kFavIconSize) {
    // The bitmap is not the correct size, re-sample.
    int new_width = decoded.width();
    int new_height = decoded.height();
    calc_favicon_target_size(&new_width, &new_height);
    decoded = gfx::ImageOperations::Resize(
        decoded, gfx::ImageOperations::RESIZE_LANCZOS3,
        gfx::Size(new_width, new_height));
  }

  // Encode our bitmap as a PNG.
  SkAutoLockPixels decoded_lock(decoded);
  PNGEncoder::Encode(reinterpret_cast<unsigned char*>(decoded.getPixels()),
                     PNGEncoder::FORMAT_BGRA, decoded.width(),
                     decoded.height(), decoded.width() * 4, false, png_data);
  return true;
}

// ImporterHost.

ImporterHost::ImporterHost()
    : observer_(NULL),
      task_(NULL),
      importer_(NULL),
      file_loop_(g_browser_process->file_thread()->message_loop()),
      waiting_for_bookmarkbar_model_(false),
      waiting_for_template_url_model_(false),
      is_source_readable_(true) {
  DetectSourceProfiles();
}

ImporterHost::ImporterHost(MessageLoop* file_loop)
    : observer_(NULL),
      task_(NULL),
      importer_(NULL),
      file_loop_(file_loop),
      waiting_for_bookmarkbar_model_(false),
      waiting_for_template_url_model_(false),
      is_source_readable_(true) {
  DetectSourceProfiles();
}

ImporterHost::~ImporterHost() {
  STLDeleteContainerPointers(source_profiles_.begin(), source_profiles_.end());
}

void ImporterHost::Loaded(BookmarkBarModel* model) {
  model->RemoveObserver(this);
  waiting_for_bookmarkbar_model_ = false;
  InvokeTaskIfDone();
}

void ImporterHost::Observe(NotificationType type,
                           const NotificationSource& source,
                           const NotificationDetails& details) {
  DCHECK(type == TEMPLATE_URL_MODEL_LOADED);
  TemplateURLModel* model = Source<TemplateURLModel>(source).ptr();
  NotificationService::current()->RemoveObserver(
      this, TEMPLATE_URL_MODEL_LOADED,
      Source<TemplateURLModel>(model));
  waiting_for_template_url_model_ = false;
  InvokeTaskIfDone();
}

void ImporterHost::ShowWarningDialog() {
  ChromeViews::Window::CreateChromeWindow(GetActiveWindow(), gfx::Rect(),
                                          new ImporterLockView(this))->Show();
}

void ImporterHost::OnLockViewEnd(bool is_continue) {
  if (is_continue) {
    // User chose to continue, then we check the lock again to make
    // sure that Firefox has been closed. Try to import the settings
    // if successful. Otherwise, show a warning dialog.
    firefox_lock_->Lock();
    if (firefox_lock_->HasAcquired()) {
      is_source_readable_ = true;
      InvokeTaskIfDone();
    } else {
      ShowWarningDialog();
    }
  } else {
    // User chose to skip the import process. We should delete
    // the task and notify the ImporterHost to finish.
    delete task_;
    task_ = NULL;
    importer_ = NULL;
    ImportEnded();
  }
}

void ImporterHost::StartImportSettings(const ProfileInfo& profile_info,
                                       uint16 items,
                                       ProfileWriter* writer,
                                       bool first_run) {
  // Preserves the observer and creates a task, since we do async import
  // so that it doesn't block the UI. When the import is complete, observer
  // will be notified.
  writer_ = writer;
  importer_ = CreateImporterByType(profile_info.browser_type);
  importer_->set_first_run(first_run);
  task_ = NewRunnableMethod(importer_, &Importer::StartImport,
      profile_info, items, writer_.get(), this);

  // We should lock the Firefox profile directory to prevent corruption.
  if (profile_info.browser_type == FIREFOX2 ||
      profile_info.browser_type == FIREFOX3) {
    firefox_lock_.reset(new FirefoxProfileLock(profile_info.source_path));
    if (!firefox_lock_->HasAcquired()) {
      // If fail to acquire the lock, we set the source unreadable and
      // show a warning dialog.
      is_source_readable_ = false;
      ShowWarningDialog();
    }
  }

  // BookmarkBarModel should be loaded before adding IE favorites. So we
  // observe the BookmarkBarModel if needed, and start the task after
  // it has been loaded.
  if ((items & FAVORITES) && !writer_->BookmarkBarModelIsLoaded()) {
    writer_->AddBookmarkBarModelObserver(this);
    waiting_for_bookmarkbar_model_ = true;
  }

  // Observes the TemplateURLModel if needed to import search engines from the
  // other browser. We also check to see if we're importing bookmarks because
  // we can import bookmark keywords from Firefox as search engines.
  if ((items & SEARCH_ENGINES) || (items & FAVORITES)) {
    if (!writer_->TemplateURLModelIsLoaded()) {
      writer_->AddTemplateURLModelObserver(this);
      waiting_for_template_url_model_ = true;
    }
  }

  AddRef();
  InvokeTaskIfDone();
}

void ImporterHost::Cancel() {
  if (importer_)
    importer_->Cancel();
}

void ImporterHost::SetObserver(Observer* observer) {
  observer_ = observer;
}

void ImporterHost::InvokeTaskIfDone() {
  if (waiting_for_bookmarkbar_model_ || waiting_for_template_url_model_ ||
      !is_source_readable_)
    return;
  file_loop_->PostTask(FROM_HERE, task_);
}

void ImporterHost::ImportItemStarted(ImportItem item) {
  if (observer_)
    observer_->ImportItemStarted(item);
}

void ImporterHost::ImportItemEnded(ImportItem item) {
  if (observer_)
    observer_->ImportItemEnded(item);
}

void ImporterHost::ImportStarted() {
  if (observer_)
    observer_->ImportStarted();
}

void ImporterHost::ImportEnded() {
  firefox_lock_.reset();  // Release the Firefox profile lock.
  if (observer_)
    observer_->ImportEnded();
  Release();
}

Importer* ImporterHost::CreateImporterByType(ProfileType type) {
  switch (type) {
    case MS_IE:
      return new IEImporter();
    case FIREFOX2:
      return new Firefox2Importer();
    case FIREFOX3:
      return new Firefox3Importer();
  }
  NOTREACHED();
  return NULL;
}

int ImporterHost::GetAvailableProfileCount() {
  return static_cast<int>(source_profiles_.size());
}

std::wstring ImporterHost::GetSourceProfileNameAt(int index) const {
  DCHECK(index < static_cast<int>(source_profiles_.size()));
  return source_profiles_[index]->description;
}

const ProfileInfo& ImporterHost::GetSourceProfileInfoAt(int index) const {
  DCHECK(index < static_cast<int>(source_profiles_.size()));
  return *source_profiles_[index];
}

void ImporterHost::DetectSourceProfiles() {
  if (ShellIntegration::IsFirefoxDefaultBrowser()) {
    DetectFirefoxProfiles();
    DetectIEProfiles();
  } else {
    DetectIEProfiles();
    DetectFirefoxProfiles();
  }
}

void ImporterHost::DetectIEProfiles() {
  // IE always exists and don't have multiple profiles.
  ProfileInfo* ie = new ProfileInfo();
  ie->description = l10n_util::GetString(IDS_IMPORT_FROM_IE);
  ie->browser_type = MS_IE;
  ie->source_path.clear();
  ie->app_path.clear();
  source_profiles_.push_back(ie);
}

void ImporterHost::DetectFirefoxProfiles() {
  // Detects which version of Firefox is installed.
  int version = GetCurrentFirefoxMajorVersion();
  ProfileType firefox_type;
  if (version == 2) {
    firefox_type = FIREFOX2;
  } else if (version == 3) {
    firefox_type = FIREFOX3;
  } else {
    // Ignores other versions of firefox.
    return;
  }

  std::wstring ini_file = GetProfilesINI();
  DictionaryValue root;
  ParseProfileINI(ini_file, &root);

  std::wstring source_path;
  for (int i = 0; ; ++i) {
    std::wstring current_profile = L"Profile" + IntToWString(i);
    if (!root.HasKey(current_profile)) {
      // Profiles are continuously numbered. So we exit when we can't
      // find the i-th one.
      break;
    }
    std::wstring is_relative, path, profile_path;
    if (root.GetString(current_profile + L".IsRelative", &is_relative) &&
        root.GetString(current_profile + L".Path", &path)) {
      ReplaceSubstringsAfterOffset(&path, 0, L"/", L"\\");

      // IsRelative=1 means the folder path would be relative to the
      // path of profiles.ini. IsRelative=0 refers to a custom profile
      // location.
      if (is_relative == L"1") {
        profile_path = file_util::GetDirectoryFromPath(ini_file);
        file_util::AppendToPath(&profile_path, path);
      } else {
        profile_path = path;
      }

      // We only import the default profile when multiple profiles exist,
      // since the other profiles are used mostly by developers for testing.
      // Otherwise, Profile0 will be imported.
      std::wstring is_default;
      if ((root.GetString(current_profile + L".Default", &is_default) &&
           is_default == L"1") || i == 0) {
        source_path = profile_path;
        // We break out of the loop when we have found the default profile.
        if (is_default == L"1")
          break;
      }
    }
  }

  if (!source_path.empty()) {
    ProfileInfo* firefox = new ProfileInfo();
    firefox->description = l10n_util::GetString(IDS_IMPORT_FROM_FIREFOX);
    firefox->browser_type = firefox_type;
    firefox->source_path = source_path;
    firefox->app_path = GetFirefoxInstallPath();
    source_profiles_.push_back(firefox);
  }
}
