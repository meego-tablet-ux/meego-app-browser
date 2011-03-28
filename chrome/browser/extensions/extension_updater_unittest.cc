// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>

#include "base/file_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/stl_util-inl.h"
#include "base/string_number_conversions.h"
#include "base/string_split.h"
#include "base/string_util.h"
#include "base/stringprintf.h"
#include "base/threading/thread.h"
#include "base/version.h"
#include "chrome/browser/extensions/extension_error_reporter.h"
#include "chrome/browser/extensions/extension_updater.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/test_extension_prefs.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/net/test_url_fetcher_factory.h"
#include "chrome/test/testing_profile.h"
#include "content/browser/browser_thread.h"
#include "net/base/escape.h"
#include "net/base/load_flags.h"
#include "net/url_request/url_request_status.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "libxml/globals.h"

using base::Time;
using base::TimeDelta;

namespace {

const char kEmptyUpdateUrlData[] = "";

int expected_load_flags =
    net::LOAD_DO_NOT_SEND_COOKIES |
    net::LOAD_DO_NOT_SAVE_COOKIES |
    net::LOAD_DISABLE_CACHE;

const ManifestFetchData::PingData kNeverPingedData(
    ManifestFetchData::kNeverPinged, ManifestFetchData::kNeverPinged);

}  // namespace

// Base class for further specialized test classes.
class MockService : public ExtensionUpdateService {
 public:
  MockService()
      : pending_extension_manager_(ALLOW_THIS_IN_INITIALIZER_LIST(*this)) {}
  virtual ~MockService() {}

  virtual const ExtensionList* extensions() const {
    ADD_FAILURE();
    return NULL;
  }

  virtual PendingExtensionManager* pending_extension_manager() {
    ADD_FAILURE() << "Subclass should override this if it will "
                  << "be accessed by a test.";
    return &pending_extension_manager_;
  }

  virtual void UpdateExtension(const std::string& id,
                               const FilePath& path,
                               const GURL& download_url) {
    FAIL();
  }

  virtual const Extension* GetExtensionById(const std::string& id,
                                            bool include_disabled) const {
    ADD_FAILURE();
    return NULL;
  }

  virtual void UpdateExtensionBlacklist(
      const std::vector<std::string>& blacklist) {
    FAIL();
  }

  virtual void CheckAdminBlacklist() {
    FAIL();
  }

  virtual bool HasInstalledExtensions() {
    ADD_FAILURE();
    return false;
  }

  virtual ExtensionPrefs* extension_prefs() { return prefs_.prefs(); }
  virtual const ExtensionPrefs& const_extension_prefs() const {
    return prefs_.const_prefs();
  }

  virtual Profile* profile() { return &profile_; }

  PrefService* pref_service() { return prefs_.pref_service(); }

  // Creates test extensions and inserts them into list. The name and
  // version are all based on their index. If |update_url| is non-null, it
  // will be used as the update_url for each extension.
  // The |id| is used to distinguish extension names and make sure that
  // no two extensions share the same name.
  void CreateTestExtensions(int id, int count, ExtensionList *list,
                            const std::string* update_url,
                            Extension::Location location) {
    for (int i = 1; i <= count; i++) {
      DictionaryValue manifest;
      manifest.SetString(extension_manifest_keys::kVersion,
                         base::StringPrintf("%d.0.0.0", i));
      manifest.SetString(extension_manifest_keys::kName,
                         base::StringPrintf("Extension %d.%d", id, i));
      if (update_url)
        manifest.SetString(extension_manifest_keys::kUpdateURL, *update_url);
      scoped_refptr<Extension> e =
          prefs_.AddExtensionWithManifest(manifest, location);
      ASSERT_TRUE(e != NULL);
      list->push_back(e);
    }
  }

 protected:
  PendingExtensionManager pending_extension_manager_;
  TestExtensionPrefs prefs_;
  TestingProfile profile_;

 private:
  DISALLOW_COPY_AND_ASSIGN(MockService);
};


std::string GenerateId(std::string input) {
  std::string result;
  EXPECT_TRUE(Extension::GenerateId(input, &result));
  return result;
}

bool ShouldInstallExtensionsOnly(const Extension& extension) {
  return extension.GetType() == Extension::TYPE_EXTENSION;
}

bool ShouldInstallThemesOnly(const Extension& extension) {
  return extension.is_theme();
}

bool ShouldAlwaysInstall(const Extension& extension) {
  return true;
}

// Loads some pending extension records into a pending extension manager.
void SetupPendingExtensionManagerForTest(
    int count,
    const GURL& update_url,
    PendingExtensionManager* pending_extension_manager) {
  for (int i = 1; i <= count; i++) {
    PendingExtensionInfo::ShouldAllowInstallPredicate should_allow_install =
        (i % 2 == 0) ? &ShouldInstallThemesOnly : &ShouldInstallExtensionsOnly;
    const bool kIsFromSync = true;
    const bool kInstallSilently = true;
    const Extension::State kInitialState = Extension::ENABLED;
    const bool kInitialIncognitoEnabled = false;
    std::string id = GenerateId(base::StringPrintf("extension%i", i));

    pending_extension_manager->AddForTesting(
        id,
        PendingExtensionInfo(update_url,
                             should_allow_install,
                             kIsFromSync,
                             kInstallSilently,
                             kInitialState,
                             kInitialIncognitoEnabled,
                             Extension::INTERNAL));
  }
}

class ServiceForManifestTests : public MockService {
 public:
  ServiceForManifestTests() : has_installed_extensions_(false) {}

  virtual ~ServiceForManifestTests() {}

  virtual const Extension* GetExtensionById(const std::string& id,
                                            bool include_disabled) const {
    for (ExtensionList::const_iterator iter = extensions_.begin();
        iter != extensions_.end(); ++iter) {
      if ((*iter)->id() == id) {
        return *iter;
      }
    }
    return NULL;
  }

  virtual const ExtensionList* extensions() const { return &extensions_; }

  virtual PendingExtensionManager* pending_extension_manager() {
    return &pending_extension_manager_;
  }

  void set_extensions(ExtensionList extensions) {
    extensions_ = extensions;
  }

  virtual bool HasInstalledExtensions() {
    return has_installed_extensions_;
  }

  void set_has_installed_extensions(bool value) {
    has_installed_extensions_ = value;
  }

 private:
  ExtensionList extensions_;
  bool has_installed_extensions_;
};

class ServiceForDownloadTests : public MockService {
 public:
  virtual void UpdateExtension(const std::string& id,
                               const FilePath& extension_path,
                               const GURL& download_url) {
    extension_id_ = id;
    install_path_ = extension_path;
    download_url_ = download_url;
  }

  virtual PendingExtensionManager* pending_extension_manager() {
    return &pending_extension_manager_;
  }

  virtual const Extension* GetExtensionById(const std::string& id, bool) const {
    last_inquired_extension_id_ = id;
    return NULL;
  }

  const std::string& extension_id() const { return extension_id_; }
  const FilePath& install_path() const { return install_path_; }
  const GURL& download_url() const { return download_url_; }
  const std::string& last_inquired_extension_id() const {
    return last_inquired_extension_id_;
  }

 private:
  std::string extension_id_;
  FilePath install_path_;
  GURL download_url_;

  // The last extension ID that GetExtensionById was called with.
  // Mutable because the method that sets it (GetExtensionById) is const
  // in the actual extension service, but must record the last extension
  // ID in this test class.
  mutable std::string last_inquired_extension_id_;
};

class ServiceForBlacklistTests : public MockService {
 public:
  ServiceForBlacklistTests()
     : MockService(),
       processed_blacklist_(false) {
  }
  virtual void UpdateExtensionBlacklist(
    const std::vector<std::string>& blacklist) {
    processed_blacklist_ = true;
    return;
  }
  bool processed_blacklist() { return processed_blacklist_; }
  const std::string& extension_id() { return extension_id_; }

 private:
  bool processed_blacklist_;
  std::string extension_id_;
  FilePath install_path_;
};

static const int kUpdateFrequencySecs = 15;

// Takes a string with KEY=VALUE parameters separated by '&' in |params| and
// puts the key/value pairs into |result|. For keys with no value, the empty
// string is used. So for "a=1&b=foo&c", result would map "a" to "1", "b" to
// "foo", and "c" to "".
static void ExtractParameters(const std::string& params,
                              std::map<std::string, std::string>* result) {
  std::vector<std::string> pairs;
  base::SplitString(params, '&', &pairs);
  for (size_t i = 0; i < pairs.size(); i++) {
    std::vector<std::string> key_val;
    base::SplitString(pairs[i], '=', &key_val);
    if (!key_val.empty()) {
      std::string key = key_val[0];
      EXPECT_TRUE(result->find(key) == result->end());
      (*result)[key] = (key_val.size() == 2) ? key_val[1] : "";
    } else {
      NOTREACHED();
    }
  }
}

// All of our tests that need to use private APIs of ExtensionUpdater live
// inside this class (which is a friend to ExtensionUpdater).
class ExtensionUpdaterTest : public testing::Test {
 public:
  static void SimulateTimerFired(ExtensionUpdater* updater) {
    EXPECT_TRUE(updater->timer_.IsRunning());
    updater->timer_.Stop();
    updater->TimerFired();
  }

  // Adds a Result with the given data to results.
  static void AddParseResult(
      const std::string& id,
      const std::string& version,
      const std::string& url,
      UpdateManifest::Results* results) {
    UpdateManifest::Result result;
    result.extension_id = id;
    result.version = version;
    result.crx_url = GURL(url);
    results->list.push_back(result);
  }

  static void TestExtensionUpdateCheckRequests(bool pending) {
    MessageLoop message_loop;
    BrowserThread file_thread(BrowserThread::FILE, &message_loop);
    BrowserThread io_thread(BrowserThread::IO);
    io_thread.Start();

    // Create an extension with an update_url.
    ServiceForManifestTests service;
    std::string update_url("http://foo.com/bar");
    ExtensionList extensions;
    PendingExtensionManager* pending_extension_manager =
        service.pending_extension_manager();
    if (pending) {
      SetupPendingExtensionManagerForTest(1, GURL(update_url),
                                          pending_extension_manager);
    } else {
      service.CreateTestExtensions(1, 1, &extensions, &update_url,
                                   Extension::INTERNAL);
      service.set_extensions(extensions);
    }

    // Set up and start the updater.
    TestURLFetcherFactory factory;
    URLFetcher::set_factory(&factory);
    scoped_refptr<ExtensionUpdater> updater(
        new ExtensionUpdater(&service, service.pref_service(), 60*60*24));
    updater->Start();

    // Tell the update that it's time to do update checks.
    SimulateTimerFired(updater.get());

    // Get the url our mock fetcher was asked to fetch.
    TestURLFetcher* fetcher =
        factory.GetFetcherByID(ExtensionUpdater::kManifestFetcherId);
    const GURL& url = fetcher->original_url();
    EXPECT_FALSE(url.is_empty());
    EXPECT_TRUE(url.is_valid());
    EXPECT_TRUE(url.SchemeIs("http"));
    EXPECT_EQ("foo.com", url.host());
    EXPECT_EQ("/bar", url.path());

    // Validate the extension request parameters in the query. It should
    // look something like "?x=id%3D<id>%26v%3D<version>%26uc".
    EXPECT_TRUE(url.has_query());
    std::vector<std::string> parts;
    base::SplitString(url.query(), '=', &parts);
    EXPECT_EQ(2u, parts.size());
    EXPECT_EQ("x", parts[0]);
    std::string decoded = UnescapeURLComponent(parts[1],
                                               UnescapeRule::URL_SPECIAL_CHARS);
    std::map<std::string, std::string> params;
    ExtractParameters(decoded, &params);
    if (pending) {
      EXPECT_EQ(pending_extension_manager->begin()->first, params["id"]);
      EXPECT_EQ("0.0.0.0", params["v"]);
    } else {
      EXPECT_EQ(extensions[0]->id(), params["id"]);
      EXPECT_EQ(extensions[0]->VersionString(), params["v"]);
    }
    EXPECT_EQ("", params["uc"]);
  }

  static void TestBlacklistUpdateCheckRequests() {
    ServiceForManifestTests service;

    // Setup and start the updater.
    MessageLoop message_loop;
    BrowserThread io_thread(BrowserThread::IO);
    io_thread.Start();

    TestURLFetcherFactory factory;
    URLFetcher::set_factory(&factory);
    scoped_refptr<ExtensionUpdater> updater(
        new ExtensionUpdater(&service, service.pref_service(), 60*60*24));
    updater->Start();

    // Tell the updater that it's time to do update checks.
    SimulateTimerFired(updater.get());

    // No extensions installed, so nothing should have been fetched.
    TestURLFetcher* fetcher =
        factory.GetFetcherByID(ExtensionUpdater::kManifestFetcherId);
    EXPECT_TRUE(fetcher == NULL);

    // Try again with an extension installed.
    service.set_has_installed_extensions(true);
    SimulateTimerFired(updater.get());

    // Get the url our mock fetcher was asked to fetch.
    fetcher = factory.GetFetcherByID(ExtensionUpdater::kManifestFetcherId);
    ASSERT_FALSE(fetcher == NULL);
    const GURL& url = fetcher->original_url();

    EXPECT_FALSE(url.is_empty());
    EXPECT_TRUE(url.is_valid());
    EXPECT_TRUE(url.SchemeIs("https"));
    EXPECT_EQ("clients2.google.com", url.host());
    EXPECT_EQ("/service/update2/crx", url.path());

    // Validate the extension request parameters in the query. It should
    // look something like "?x=id%3D<id>%26v%3D<version>%26uc".
    EXPECT_TRUE(url.has_query());
    std::vector<std::string> parts;
    base::SplitString(url.query(), '=', &parts);
    EXPECT_EQ(2u, parts.size());
    EXPECT_EQ("x", parts[0]);
    std::string decoded = UnescapeURLComponent(parts[1],
                                               UnescapeRule::URL_SPECIAL_CHARS);
    std::map<std::string, std::string> params;
    ExtractParameters(decoded, &params);
    EXPECT_EQ("com.google.crx.blacklist", params["id"]);
    EXPECT_EQ("0", params["v"]);
    EXPECT_EQ("", params["uc"]);
    EXPECT_TRUE(ContainsKey(params, "ping"));
  }

  static void TestUpdateUrlDataEmpty() {
    const std::string id = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    const std::string version = "1.0";

    // Make sure that an empty update URL data string does not cause a ap=
    // option to appear in the x= parameter.
    ManifestFetchData fetch_data(GURL("http://localhost/foo"));
    fetch_data.AddExtension(id, version,
                            kNeverPingedData, "");
    EXPECT_EQ("http://localhost/foo\?x=id%3Daaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
              "%26v%3D1.0%26uc",
              fetch_data.full_url().spec());
  }

  static void TestUpdateUrlDataSimple() {
    const std::string id = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    const std::string version = "1.0";

    // Make sure that an update URL data string causes an appropriate ap=
    // option to appear in the x= parameter.
    ManifestFetchData fetch_data(GURL("http://localhost/foo"));
    fetch_data.AddExtension(id, version,
                            kNeverPingedData, "bar");
    EXPECT_EQ("http://localhost/foo\?x=id%3Daaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
              "%26v%3D1.0%26uc%26ap%3Dbar",
              fetch_data.full_url().spec());
  }

  static void TestUpdateUrlDataCompound() {
    const std::string id = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    const std::string version = "1.0";

    // Make sure that an update URL data string causes an appropriate ap=
    // option to appear in the x= parameter.
    ManifestFetchData fetch_data(GURL("http://localhost/foo"));
    fetch_data.AddExtension(id, version,
                            kNeverPingedData, "a=1&b=2&c");
    EXPECT_EQ("http://localhost/foo\?x=id%3Daaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
              "%26v%3D1.0%26uc%26ap%3Da%253D1%2526b%253D2%2526c",
              fetch_data.full_url().spec());
  }

  static void TestUpdateUrlDataFromGallery(const std::string& gallery_url) {
    MockService service;
    ManifestFetchesBuilder builder(&service);
    ExtensionList extensions;
    std::string url(gallery_url);

    service.CreateTestExtensions(1, 1, &extensions, &url, Extension::INTERNAL);
    builder.AddExtension(*extensions[0]);
    std::vector<ManifestFetchData*> fetches = builder.GetFetches();
    EXPECT_EQ(1u, fetches.size());
    scoped_ptr<ManifestFetchData> fetch(fetches[0]);
    fetches.clear();

    // Make sure that extensions that update from the gallery ignore any
    // update URL data.
    const std::string& update_url = fetch->full_url().spec();
    std::string::size_type x = update_url.find("x=");
    EXPECT_NE(std::string::npos, x);
    std::string::size_type ap = update_url.find("ap%3D", x);
    EXPECT_EQ(std::string::npos, ap);
  }

  static void TestDetermineUpdates() {
    MessageLoop message_loop;
    BrowserThread file_thread(BrowserThread::FILE, &message_loop);

    // Create a set of test extensions
    ServiceForManifestTests service;
    ExtensionList tmp;
    service.CreateTestExtensions(1, 3, &tmp, NULL, Extension::INTERNAL);
    service.set_extensions(tmp);

    scoped_refptr<ExtensionUpdater> updater(
        new ExtensionUpdater(&service, service.pref_service(),
                             kUpdateFrequencySecs));
    updater->Start();

    // Check passing an empty list of parse results to DetermineUpdates
    ManifestFetchData fetch_data(GURL("http://localhost/foo"));
    UpdateManifest::Results updates;
    std::vector<int> updateable = updater->DetermineUpdates(fetch_data,
                                                            updates);
    EXPECT_TRUE(updateable.empty());

    // Create two updates - expect that DetermineUpdates will return the first
    // one (v1.0 installed, v1.1 available) but not the second one (both
    // installed and available at v2.0).
    scoped_ptr<Version> one(Version::GetVersionFromString("1.0"));
    EXPECT_TRUE(tmp[0]->version()->Equals(*one));
    fetch_data.AddExtension(tmp[0]->id(), tmp[0]->VersionString(),
                            kNeverPingedData,
                            kEmptyUpdateUrlData);
    AddParseResult(tmp[0]->id(),
        "1.1", "http://localhost/e1_1.1.crx", &updates);
    fetch_data.AddExtension(tmp[1]->id(), tmp[1]->VersionString(),
                            kNeverPingedData,
                            kEmptyUpdateUrlData);
    AddParseResult(tmp[1]->id(),
        tmp[1]->VersionString(), "http://localhost/e2_2.0.crx", &updates);
    updateable = updater->DetermineUpdates(fetch_data, updates);
    EXPECT_EQ(1u, updateable.size());
    EXPECT_EQ(0, updateable[0]);
  }

  static void TestDetermineUpdatesPending() {
    // Create a set of test extensions
    ServiceForManifestTests service;
    PendingExtensionManager* pending_extension_manager =
        service.pending_extension_manager();
    SetupPendingExtensionManagerForTest(3, GURL(), pending_extension_manager);

    MessageLoop message_loop;
    scoped_refptr<ExtensionUpdater> updater(
        new ExtensionUpdater(&service, service.pref_service(),
                             kUpdateFrequencySecs));
    updater->Start();

    ManifestFetchData fetch_data(GURL("http://localhost/foo"));
    UpdateManifest::Results updates;
    PendingExtensionManager::const_iterator it;
    for (it = pending_extension_manager->begin();
         it != pending_extension_manager->end(); ++it) {
      fetch_data.AddExtension(it->first, "1.0.0.0",
                              kNeverPingedData,
                              kEmptyUpdateUrlData);
      AddParseResult(it->first,
                     "1.1", "http://localhost/e1_1.1.crx", &updates);
    }
    std::vector<int> updateable =
        updater->DetermineUpdates(fetch_data, updates);
    // All the apps should be updateable.
    EXPECT_EQ(3u, updateable.size());
    for (std::vector<int>::size_type i = 0; i < updateable.size(); ++i) {
      EXPECT_EQ(static_cast<int>(i), updateable[i]);
    }
  }

  static void TestMultipleManifestDownloading() {
    MessageLoop ui_loop;
    BrowserThread ui_thread(BrowserThread::UI, &ui_loop);
    BrowserThread file_thread(BrowserThread::FILE);
    file_thread.Start();
    BrowserThread io_thread(BrowserThread::IO);
    io_thread.Start();

    TestURLFetcherFactory factory;
    TestURLFetcher* fetcher = NULL;
    URLFetcher::set_factory(&factory);
    scoped_ptr<ServiceForDownloadTests> service(new ServiceForDownloadTests);
    scoped_refptr<ExtensionUpdater> updater(
        new ExtensionUpdater(service.get(), service->pref_service(),
                             kUpdateFrequencySecs));
    updater->Start();

    GURL url1("http://localhost/manifest1");
    GURL url2("http://localhost/manifest2");

    // Request 2 update checks - the first should begin immediately and the
    // second one should be queued up.
    ManifestFetchData* fetch1 = new ManifestFetchData(url1);
    ManifestFetchData* fetch2 = new ManifestFetchData(url2);
    ManifestFetchData::PingData zeroDays(0, 0);
    fetch1->AddExtension("1111", "1.0", zeroDays, kEmptyUpdateUrlData);
    fetch2->AddExtension("12345", "2.0", kNeverPingedData,
                         kEmptyUpdateUrlData);
    updater->StartUpdateCheck(fetch1);
    updater->StartUpdateCheck(fetch2);

    std::string invalid_xml = "invalid xml";
    fetcher = factory.GetFetcherByID(ExtensionUpdater::kManifestFetcherId);
    EXPECT_TRUE(fetcher != NULL && fetcher->delegate() != NULL);
    EXPECT_TRUE(fetcher->load_flags() == expected_load_flags);
    fetcher->delegate()->OnURLFetchComplete(
        fetcher, url1, net::URLRequestStatus(), 200, ResponseCookies(),
        invalid_xml);

    // Now that the first request is complete, make sure the second one has
    // been started.
    const std::string kValidXml =
        "<?xml version='1.0' encoding='UTF-8'?>"
        "<gupdate xmlns='http://www.google.com/update2/response'"
        "                protocol='2.0'>"
        " <app appid='12345'>"
        "  <updatecheck codebase='http://example.com/extension_1.2.3.4.crx'"
        "               version='1.2.3.4' prodversionmin='2.0.143.0' />"
        " </app>"
        "</gupdate>";
    fetcher = factory.GetFetcherByID(ExtensionUpdater::kManifestFetcherId);
    EXPECT_TRUE(fetcher != NULL && fetcher->delegate() != NULL);
    EXPECT_TRUE(fetcher->load_flags() == expected_load_flags);
    fetcher->delegate()->OnURLFetchComplete(
        fetcher, url2, net::URLRequestStatus(), 200, ResponseCookies(),
        kValidXml);

    // This should run the manifest parsing, then we want to make sure that our
    // service was called with GetExtensionById with the matching id from
    // kValidXml.
    file_thread.Stop();
    io_thread.Stop();
    ui_loop.RunAllPending();
    EXPECT_EQ("12345", service->last_inquired_extension_id());
    xmlCleanupGlobals();

    // The FILE thread is needed for |service|'s cleanup,
    // because of ImportantFileWriter.
    file_thread.Start();
    service.reset();
  }

  static void TestSingleExtensionDownloading(bool pending) {
    MessageLoop ui_loop;
    BrowserThread ui_thread(BrowserThread::UI, &ui_loop);
    BrowserThread file_thread(BrowserThread::FILE);
    file_thread.Start();
    BrowserThread io_thread(BrowserThread::IO);
    io_thread.Start();

    TestURLFetcherFactory factory;
    TestURLFetcher* fetcher = NULL;
    URLFetcher::set_factory(&factory);
    scoped_ptr<ServiceForDownloadTests> service(new ServiceForDownloadTests);
    scoped_refptr<ExtensionUpdater> updater(
        new ExtensionUpdater(service.get(), service->pref_service(),
                             kUpdateFrequencySecs));
    updater->Start();

    GURL test_url("http://localhost/extension.crx");

    std::string id = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    std::string hash = "";
    scoped_ptr<Version> version(Version::GetVersionFromString("0.0.1"));
    ASSERT_TRUE(version.get());
    updater->FetchUpdatedExtension(id, test_url, hash, version->GetString());

    if (pending) {
      const bool kIsFromSync = true;
      const bool kInstallSilently = true;
      const Extension::State kInitialState = Extension::ENABLED;
      const bool kInitialIncognitoEnabled = false;
      PendingExtensionManager* pending_extension_manager =
          service->pending_extension_manager();
      pending_extension_manager->AddForTesting(
          id,
          PendingExtensionInfo(test_url, &ShouldAlwaysInstall, kIsFromSync,
                               kInstallSilently, kInitialState,
                               kInitialIncognitoEnabled, Extension::INTERNAL));
    }

    // Call back the ExtensionUpdater with a 200 response and some test data
    std::string extension_data("whatever");
    fetcher = factory.GetFetcherByID(ExtensionUpdater::kExtensionFetcherId);
    EXPECT_TRUE(fetcher != NULL && fetcher->delegate() != NULL);
    EXPECT_TRUE(fetcher->load_flags() == expected_load_flags);
    fetcher->delegate()->OnURLFetchComplete(
        fetcher, test_url, net::URLRequestStatus(), 200, ResponseCookies(),
        extension_data);

    file_thread.Stop();
    ui_loop.RunAllPending();

    // Expect that ExtensionUpdater asked the mock extensions service to install
    // a file with the test data for the right id.
    EXPECT_EQ(id, service->extension_id());
    FilePath tmpfile_path = service->install_path();
    EXPECT_FALSE(tmpfile_path.empty());
    EXPECT_EQ(test_url, service->download_url());
    std::string file_contents;
    EXPECT_TRUE(file_util::ReadFileToString(tmpfile_path, &file_contents));
    EXPECT_TRUE(extension_data == file_contents);

    // The FILE thread is needed for |service|'s cleanup,
    // because of ImportantFileWriter.
    file_thread.Start();
    service.reset();

    file_util::Delete(tmpfile_path, false);
    URLFetcher::set_factory(NULL);
  }

  static void TestBlacklistDownloading() {
    MessageLoop message_loop;
    BrowserThread ui_thread(BrowserThread::UI, &message_loop);
    BrowserThread file_thread(BrowserThread::FILE, &message_loop);
    BrowserThread io_thread(BrowserThread::IO);
    io_thread.Start();

    TestURLFetcherFactory factory;
    TestURLFetcher* fetcher = NULL;
    URLFetcher::set_factory(&factory);
    ServiceForBlacklistTests service;
    scoped_refptr<ExtensionUpdater> updater(
        new ExtensionUpdater(&service, service.pref_service(),
                             kUpdateFrequencySecs));
    updater->Start();
    GURL test_url("http://localhost/extension.crx");

    std::string id = "com.google.crx.blacklist";

    std::string hash =
      "2CE109E9D0FAF820B2434E166297934E6177B65AB9951DBC3E204CAD4689B39C";

    std::string version = "0.0.1";
    updater->FetchUpdatedExtension(id, test_url, hash, version);

    // Call back the ExtensionUpdater with a 200 response and some test data
    std::string extension_data("aaabbb");
    fetcher = factory.GetFetcherByID(ExtensionUpdater::kExtensionFetcherId);
    EXPECT_TRUE(fetcher != NULL && fetcher->delegate() != NULL);
    EXPECT_TRUE(fetcher->load_flags() == expected_load_flags);
    fetcher->delegate()->OnURLFetchComplete(
        fetcher, test_url, net::URLRequestStatus(), 200, ResponseCookies(),
        extension_data);

    message_loop.RunAllPending();

    // The updater should have called extension service to process the
    // blacklist.
    EXPECT_TRUE(service.processed_blacklist());

    EXPECT_EQ(version, service.pref_service()->
      GetString(prefs::kExtensionBlacklistUpdateVersion));

    URLFetcher::set_factory(NULL);
  }

  static void TestMultipleExtensionDownloading() {
    MessageLoopForUI message_loop;
    BrowserThread ui_thread(BrowserThread::UI, &message_loop);
    BrowserThread file_thread(BrowserThread::FILE, &message_loop);
    BrowserThread io_thread(BrowserThread::IO);
    io_thread.Start();

    TestURLFetcherFactory factory;
    TestURLFetcher* fetcher = NULL;
    URLFetcher::set_factory(&factory);
    ServiceForDownloadTests service;
    scoped_refptr<ExtensionUpdater> updater(
        new ExtensionUpdater(&service, service.pref_service(),
                             kUpdateFrequencySecs));
    updater->Start();

    GURL url1("http://localhost/extension1.crx");
    GURL url2("http://localhost/extension2.crx");

    std::string id1 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    std::string id2 = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";

    std::string hash1 = "";
    std::string hash2 = "";

    std::string version1 = "0.1";
    std::string version2 = "0.1";
    // Start two fetches
    updater->FetchUpdatedExtension(id1, url1, hash1, version1);
    updater->FetchUpdatedExtension(id2, url2, hash2, version2);

    // Make the first fetch complete.
    std::string extension_data1("whatever");
    fetcher = factory.GetFetcherByID(ExtensionUpdater::kExtensionFetcherId);
    EXPECT_TRUE(fetcher != NULL && fetcher->delegate() != NULL);
    EXPECT_TRUE(fetcher->load_flags() == expected_load_flags);
    fetcher->delegate()->OnURLFetchComplete(
        fetcher, url1, net::URLRequestStatus(), 200, ResponseCookies(),
        extension_data1);
    message_loop.RunAllPending();

    // Expect that the service was asked to do an install with the right data.
    FilePath tmpfile_path = service.install_path();
    EXPECT_FALSE(tmpfile_path.empty());
    EXPECT_EQ(id1, service.extension_id());
    EXPECT_EQ(url1, service.download_url());
    message_loop.RunAllPending();
    file_util::Delete(tmpfile_path, false);

    // Make sure the second fetch finished and asked the service to do an
    // update.
    std::string extension_data2("whatever2");
    fetcher = factory.GetFetcherByID(ExtensionUpdater::kExtensionFetcherId);
    EXPECT_TRUE(fetcher != NULL && fetcher->delegate() != NULL);
    EXPECT_TRUE(fetcher->load_flags() == expected_load_flags);
    fetcher->delegate()->OnURLFetchComplete(
        fetcher, url2, net::URLRequestStatus(), 200, ResponseCookies(),
        extension_data2);
    message_loop.RunAllPending();
    EXPECT_EQ(id2, service.extension_id());
    EXPECT_EQ(url2, service.download_url());
    EXPECT_FALSE(service.install_path().empty());

    // Make sure the correct crx contents were passed for the update call.
    std::string file_contents;
    EXPECT_TRUE(file_util::ReadFileToString(service.install_path(),
                                            &file_contents));
    EXPECT_TRUE(extension_data2 == file_contents);
    file_util::Delete(service.install_path(), false);
  }

  // Test requests to both a Google server and a non-google server. This allows
  // us to test various combinations of installed (ie roll call) and active
  // (ie app launch) ping scenarios. The invariant is that each type of ping
  // value should be present at most once per day, and can be calculated based
  // on the delta between now and the last ping time (or in the case of active
  // pings, that delta plus whether the app has been active).
  static void TestGalleryRequests(int rollcall_ping_days,
                                  int active_ping_days,
                                  bool active_bit) {
    MessageLoop message_loop;
    BrowserThread file_thread(BrowserThread::FILE, &message_loop);

    TestURLFetcherFactory factory;
    URLFetcher::set_factory(&factory);

    // Set up 2 mock extensions, one with a google.com update url and one
    // without.
    ServiceForManifestTests service;
    ExtensionList tmp;
    GURL url1("http://clients2.google.com/service/update2/crx");
    GURL url2("http://www.somewebsite.com");
    service.CreateTestExtensions(1, 1, &tmp, &url1.possibly_invalid_spec(),
                                 Extension::INTERNAL);
    service.CreateTestExtensions(2, 1, &tmp, &url2.possibly_invalid_spec(),
                                 Extension::INTERNAL);
    EXPECT_EQ(2u, tmp.size());
    service.set_extensions(tmp);

    ExtensionPrefs* prefs = service.extension_prefs();
    const std::string& id = tmp[0]->id();
    Time now = Time::Now();
    if (rollcall_ping_days == 0) {
      prefs->SetLastPingDay(id, now - TimeDelta::FromSeconds(15));
    } else if (rollcall_ping_days > 0) {
      Time last_ping_day = now -
                           TimeDelta::FromDays(rollcall_ping_days) -
                           TimeDelta::FromSeconds(15);
      prefs->SetLastPingDay(id, last_ping_day);
    }

    // Store a value for the last day we sent an active ping.
    if (active_ping_days == 0) {
      prefs->SetLastActivePingDay(id, now - TimeDelta::FromSeconds(15));
    } else if (active_ping_days > 0) {
      Time last_active_ping_day = now -
                                  TimeDelta::FromDays(active_ping_days) -
                                  TimeDelta::FromSeconds(15);
      prefs->SetLastActivePingDay(id, last_active_ping_day);
    }
    if (active_bit)
      prefs->SetActiveBit(id, true);

    scoped_refptr<ExtensionUpdater> updater(
      new ExtensionUpdater(&service, service.pref_service(),
                           kUpdateFrequencySecs));
    updater->Start();
    updater->set_blacklist_checks_enabled(false);

    // Make the updater do manifest fetching, and note the urls it tries to
    // fetch.
    std::vector<GURL> fetched_urls;
    updater->CheckNow();
    TestURLFetcher* fetcher =
      factory.GetFetcherByID(ExtensionUpdater::kManifestFetcherId);
    EXPECT_TRUE(fetcher != NULL && fetcher->delegate() != NULL);
    fetched_urls.push_back(fetcher->original_url());
    fetcher->delegate()->OnURLFetchComplete(
      fetcher, fetched_urls[0], net::URLRequestStatus(), 500,
      ResponseCookies(), "");
    fetcher =
      factory.GetFetcherByID(ExtensionUpdater::kManifestFetcherId);
    fetched_urls.push_back(fetcher->original_url());

    // The urls could have been fetched in either order, so use the host to
    // tell them apart and note the query each used.
    std::string url1_query;
    std::string url2_query;
    if (fetched_urls[0].host() == url1.host()) {
      url1_query = fetched_urls[0].query();
      url2_query = fetched_urls[1].query();
    } else if (fetched_urls[0].host() == url2.host()) {
      url1_query = fetched_urls[1].query();
      url2_query = fetched_urls[0].query();
    } else {
      NOTREACHED();
    }

    // First make sure the non-google query had no ping parameter.
    std::string search_string = "ping%3D";
    EXPECT_TRUE(url2_query.find(search_string) == std::string::npos);

    // Now make sure the google query had the correct ping parameter.
    bool ping_expected = false;
    bool did_rollcall = false;
    if (rollcall_ping_days != 0) {
      search_string += "r%253D" + base::IntToString(rollcall_ping_days);
      did_rollcall = true;
      ping_expected = true;
    }
    if (active_bit && active_ping_days != 0) {
      if (did_rollcall)
        search_string += "%2526";
      search_string += "a%253D" + base::IntToString(active_ping_days);
      ping_expected = true;
    }
    bool ping_found = url1_query.find(search_string) != std::string::npos;
    EXPECT_EQ(ping_expected, ping_found) << "query was: " << url1_query
        << " was looking for " << search_string;
  }

  // This makes sure that the extension updater properly stores the results
  // of a <daystart> tag from a manifest fetch in one of two cases: 1) This is
  // the first time we fetched the extension, or 2) We sent a ping value of
  // >= 1 day for the extension.
  static void TestHandleManifestResults() {
    ServiceForManifestTests service;
    MessageLoop message_loop;
    scoped_refptr<ExtensionUpdater> updater(
        new ExtensionUpdater(&service, service.pref_service(),
                             kUpdateFrequencySecs));
    updater->Start();

    GURL update_url("http://www.google.com/manifest");
    ExtensionList tmp;
    service.CreateTestExtensions(1, 1, &tmp, &update_url.spec(),
                                 Extension::INTERNAL);
    service.set_extensions(tmp);

    ManifestFetchData fetch_data(update_url);
    const Extension* extension = tmp[0];
    fetch_data.AddExtension(extension->id(), extension->VersionString(),
                            kNeverPingedData,
                            kEmptyUpdateUrlData);
    UpdateManifest::Results results;
    results.daystart_elapsed_seconds = 750;

    updater->HandleManifestResults(fetch_data, &results);
    Time last_ping_day =
        service.extension_prefs()->LastPingDay(extension->id());
    EXPECT_FALSE(last_ping_day.is_null());
    int64 seconds_diff = (Time::Now() - last_ping_day).InSeconds();
    EXPECT_LT(seconds_diff - results.daystart_elapsed_seconds, 5);
  }
};

// Because we test some private methods of ExtensionUpdater, it's easer for the
// actual test code to live in ExtenionUpdaterTest methods instead of TEST_F
// subclasses where friendship with ExtenionUpdater is not inherited.

TEST(ExtensionUpdaterTest, TestExtensionUpdateCheckRequests) {
  ExtensionUpdaterTest::TestExtensionUpdateCheckRequests(false);
}

TEST(ExtensionUpdaterTest, TestExtensionUpdateCheckRequestsPending) {
  ExtensionUpdaterTest::TestExtensionUpdateCheckRequests(true);
}

// This test is disabled on Mac, see http://crbug.com/26035.
TEST(ExtensionUpdaterTest, TestBlacklistUpdateCheckRequests) {
  ExtensionUpdaterTest::TestBlacklistUpdateCheckRequests();
}

TEST(ExtensionUpdaterTest, TestUpdateUrlData) {
  MessageLoop message_loop;
  BrowserThread file_thread(BrowserThread::FILE, &message_loop);

  ExtensionUpdaterTest::TestUpdateUrlDataEmpty();
  ExtensionUpdaterTest::TestUpdateUrlDataSimple();
  ExtensionUpdaterTest::TestUpdateUrlDataCompound();
  ExtensionUpdaterTest::TestUpdateUrlDataFromGallery(
      Extension::GalleryUpdateUrl(false).spec());
  ExtensionUpdaterTest::TestUpdateUrlDataFromGallery(
      Extension::GalleryUpdateUrl(true).spec());
}

TEST(ExtensionUpdaterTest, TestDetermineUpdates) {
  ExtensionUpdaterTest::TestDetermineUpdates();
}

TEST(ExtensionUpdaterTest, TestDetermineUpdatesPending) {
  ExtensionUpdaterTest::TestDetermineUpdatesPending();
}

TEST(ExtensionUpdaterTest, TestMultipleManifestDownloading) {
  ExtensionUpdaterTest::TestMultipleManifestDownloading();
}

TEST(ExtensionUpdaterTest, TestSingleExtensionDownloading) {
  ExtensionUpdaterTest::TestSingleExtensionDownloading(false);
}

TEST(ExtensionUpdaterTest, TestSingleExtensionDownloadingPending) {
  ExtensionUpdaterTest::TestSingleExtensionDownloading(true);
}

// This test is disabled on Mac, see http://crbug.com/26035.
TEST(ExtensionUpdaterTest, TestBlacklistDownloading) {
  ExtensionUpdaterTest::TestBlacklistDownloading();
}

TEST(ExtensionUpdaterTest, TestMultipleExtensionDownloading) {
  ExtensionUpdaterTest::TestMultipleExtensionDownloading();
}

TEST(ExtensionUpdaterTest, TestGalleryRequests) {
  // We want to test a variety of combinations of expected ping conditions for
  // rollcall and active pings.
  int ping_cases[] = { ManifestFetchData::kNeverPinged, 0, 1, 5 };

  for (size_t i = 0; i < arraysize(ping_cases); i++) {
    for (size_t j = 0; j < arraysize(ping_cases); j++) {
      for (size_t k = 0; k < 2; k++) {
        int rollcall_ping_days = ping_cases[i];
        int active_ping_days = ping_cases[j];
        // Skip cases where rollcall_ping_days == -1, but active_ping_days > 0,
        // because rollcall_ping_days == -1 means the app was just installed and
        // this is the first update check after installation.
        if (rollcall_ping_days == ManifestFetchData::kNeverPinged &&
            active_ping_days > 0)
          continue;

        bool active_bit = k > 0;
        ExtensionUpdaterTest::TestGalleryRequests(
            rollcall_ping_days, active_ping_days, active_bit);
        ASSERT_FALSE(HasFailure()) <<
          " rollcall_ping_days=" << ping_cases[i] <<
          " active_ping_days=" << ping_cases[j] <<
          " active_bit=" << active_bit;
      }
    }
  }
}

TEST(ExtensionUpdaterTest, TestHandleManifestResults) {
  ExtensionUpdaterTest::TestHandleManifestResults();
}

TEST(ExtensionUpdaterTest, TestManifestFetchesBuilderAddExtension) {
  MessageLoop message_loop;
  BrowserThread file_thread(BrowserThread::FILE, &message_loop);

  MockService service;
  ManifestFetchesBuilder builder(&service);

  // Non-internal non-external extensions should be rejected.
  {
    ExtensionList extensions;
    service.CreateTestExtensions(1, 1, &extensions, NULL, Extension::INVALID);
    ASSERT_FALSE(extensions.empty());
    builder.AddExtension(*extensions[0]);
    EXPECT_TRUE(builder.GetFetches().empty());
  }

  // Extensions with invalid update URLs should be rejected.
  builder.AddPendingExtension(
      GenerateId("foo"), PendingExtensionInfo(GURL("http:google.com:foo"),
                                              &ShouldInstallExtensionsOnly,
                                              false, false, true, false,
                                              Extension::INTERNAL));
  EXPECT_TRUE(builder.GetFetches().empty());

  // Extensions with empty IDs should be rejected.
  builder.AddPendingExtension(
      "", PendingExtensionInfo(GURL(), &ShouldInstallExtensionsOnly,
                               false, false, true, false,
                               Extension::INTERNAL));
  EXPECT_TRUE(builder.GetFetches().empty());

  // TODO(akalin): Test that extensions with empty update URLs
  // converted from user scripts are rejected.

  // Extensions with empty update URLs should have a default one
  // filled in.
  builder.AddPendingExtension(
      GenerateId("foo"), PendingExtensionInfo(GURL(),
                                              &ShouldInstallExtensionsOnly,
                                              false, false, true, false,
                                              Extension::INTERNAL));
  std::vector<ManifestFetchData*> fetches = builder.GetFetches();
  ASSERT_EQ(1u, fetches.size());
  scoped_ptr<ManifestFetchData> fetch(fetches[0]);
  fetches.clear();
  EXPECT_FALSE(fetch->base_url().is_empty());
  EXPECT_FALSE(fetch->full_url().is_empty());
}

TEST(ExtensionUpdaterTest, TestStartUpdateCheckMemory) {
    MessageLoop message_loop;
    BrowserThread file_thread(BrowserThread::FILE, &message_loop);

    ServiceForManifestTests service;
    TestURLFetcherFactory factory;
    URLFetcher::set_factory(&factory);
    scoped_refptr<ExtensionUpdater> updater(
        new ExtensionUpdater(&service, service.pref_service(),
                             kUpdateFrequencySecs));
    updater->Start();
    updater->StartUpdateCheck(new ManifestFetchData(GURL()));
    // This should delete the newly-created ManifestFetchData.
    updater->StartUpdateCheck(new ManifestFetchData(GURL()));
    // This should add into |manifests_pending_|.
    updater->StartUpdateCheck(new ManifestFetchData(
        GURL("http://www.google.com")));
    // This should clear out |manifests_pending_|.
    updater->Stop();
}

TEST(ExtensionUpdaterTest, TestAfterStopBehavior) {
    MessageLoop message_loop;
    BrowserThread file_thread(BrowserThread::FILE, &message_loop);

    ServiceForManifestTests service;
    scoped_refptr<ExtensionUpdater> updater(
        new ExtensionUpdater(&service, service.pref_service(),
                             kUpdateFrequencySecs));
    updater->Start();
    updater->Stop();
    // All the below functions should do nothing.
    updater->OnCRXFileWritten("", FilePath(), GURL());
    GURL dummy_gurl;
    ManifestFetchData dummy_manifest_fetch_data(dummy_gurl);
    UpdateManifest::Results results;
    updater->HandleManifestResults(dummy_manifest_fetch_data, &results);
    // The manifest results can be NULL if something goes wrong when parsing
    // the manifest.  HandleManifestResults should handle this gracefully.
    updater->HandleManifestResults(dummy_manifest_fetch_data, NULL);
}

// TODO(asargent) - (http://crbug.com/12780) add tests for:
// -prodversionmin (shouldn't update if browser version too old)
// -manifests & updates arriving out of order / interleaved
// -malformed update url (empty, file://, has query, has a # fragment, etc.)
// -An extension gets uninstalled while updates are in progress (so it doesn't
//  "come back from the dead")
// -An extension gets manually updated to v3 while we're downloading v2 (ie
//  you don't get downgraded accidentally)
// -An update manifest mentions multiple updates
