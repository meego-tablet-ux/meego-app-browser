// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/string_util.h"
#include "net/base/net_errors.h"
#include "net/http/http_auth_cache.h"
#include "net/http/http_auth_handler.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace {

class MockAuthHandler : public HttpAuthHandler {
 public:
  MockAuthHandler(const char* scheme, const std::string& realm,
                  HttpAuth::Target target) {
    // Can't use initializer list since these are members of the base class.
    scheme_ = scheme;
    realm_ = realm;
    score_ = 1;
    target_ = target;
    properties_ = 0;
  }

  virtual int GenerateAuthToken(const std::wstring&,
                                const std::wstring&,
                                const HttpRequestInfo*,
                                const ProxyInfo*,
                                std::string* auth_token) {
    *auth_token = "mock-credentials";
    return OK;
  }

  virtual int GenerateDefaultAuthToken(const HttpRequestInfo*,
                                       const ProxyInfo*,
                                       std::string* auth_token) {
    *auth_token = "mock-credentials";
    return OK;
  }

 protected:
  virtual bool Init(HttpAuth::ChallengeTokenizer* challenge) {
    return false;  // Unused.
  }

 private:
  ~MockAuthHandler() {}
};

}  // namespace

// Test adding and looking-up cache entries (both by realm and by path).
TEST(HttpAuthCacheTest, Basic) {
  GURL origin("http://www.google.com");
  HttpAuthCache cache;
  HttpAuthCache::Entry* entry;

  // Add cache entries for 3 realms: "Realm1", "Realm2", "Realm3"

  scoped_ptr<HttpAuthHandler> realm1_handler(
      new MockAuthHandler("basic", "Realm1", HttpAuth::AUTH_SERVER));
  cache.Add(origin, realm1_handler->realm(), realm1_handler->scheme(),
            "Basic realm=Realm1", L"realm1-user", L"realm1-password",
            "/foo/bar/index.html");

  scoped_ptr<HttpAuthHandler> realm2_handler(
      new MockAuthHandler("basic", "Realm2", HttpAuth::AUTH_SERVER));
  cache.Add(origin, realm2_handler->realm(), realm2_handler->scheme(),
            "Basic realm=Realm2", L"realm2-user", L"realm2-password",
            "/foo2/index.html");

  scoped_ptr<HttpAuthHandler> realm3_basic_handler(
      new MockAuthHandler("basic", "Realm3", HttpAuth::AUTH_PROXY));
  cache.Add(origin, realm3_basic_handler->realm(),
            realm3_basic_handler->scheme(), "Basic realm=Realm3",
            L"realm3-basic-user", L"realm3-basic-password", "");

  scoped_ptr<HttpAuthHandler> realm3_digest_handler(
      new MockAuthHandler("digest", "Realm3", HttpAuth::AUTH_PROXY));
  cache.Add(origin, realm3_digest_handler->realm(),
            realm3_digest_handler->scheme(), "Digest realm=Realm3",
            L"realm3-digest-user", L"realm3-digest-password",
            "/baz/index.html");

  // There is no Realm4
  entry = cache.Lookup(origin, "Realm4", "basic");
  EXPECT_TRUE(NULL == entry);

  // While Realm3 does exist, the origin scheme is wrong.
  entry = cache.Lookup(GURL("https://www.google.com"), "Realm3",
                       "basic");
  EXPECT_TRUE(NULL == entry);

  // Realm, origin scheme ok, authentication scheme wrong
  entry = cache.Lookup(GURL("http://www.google.com"), "Realm1", "digest");
  EXPECT_TRUE(NULL == entry);

  // Valid lookup by origin, realm, scheme.
  entry = cache.Lookup(GURL("http://www.google.com:80"), "Realm3", "basic");
  ASSERT_FALSE(NULL == entry);
  EXPECT_EQ("basic", entry->scheme());
  EXPECT_EQ("Realm3", entry->realm());
  EXPECT_EQ("Basic realm=Realm3", entry->auth_challenge());
  EXPECT_EQ(L"realm3-basic-user", entry->username());
  EXPECT_EQ(L"realm3-basic-password", entry->password());

  // Valid lookup by origin, realm, scheme when there's a duplicate
  // origin, realm in the cache
  entry = cache.Lookup(GURL("http://www.google.com:80"), "Realm3", "digest");
  ASSERT_FALSE(NULL == entry);
  EXPECT_EQ("digest", entry->scheme());
  EXPECT_EQ("Realm3", entry->realm());
  EXPECT_EQ("Digest realm=Realm3", entry->auth_challenge());
  EXPECT_EQ(L"realm3-digest-user", entry->username());
  EXPECT_EQ(L"realm3-digest-password", entry->password());

  // Valid lookup by realm.
  entry = cache.Lookup(origin, "Realm2", "basic");
  ASSERT_FALSE(NULL == entry);
  EXPECT_EQ("basic", entry->scheme());
  EXPECT_EQ("Realm2", entry->realm());
  EXPECT_EQ("Basic realm=Realm2", entry->auth_challenge());
  EXPECT_EQ(L"realm2-user", entry->username());
  EXPECT_EQ(L"realm2-password", entry->password());

  // Check that subpaths are recognized.
  HttpAuthCache::Entry* realm2_entry = cache.Lookup(origin, "Realm2", "basic");
  EXPECT_FALSE(NULL == realm2_entry);
  // Positive tests:
  entry = cache.LookupByPath(origin, "/foo2/index.html");
  EXPECT_TRUE(realm2_entry == entry);
  entry = cache.LookupByPath(origin, "/foo2/foobar.html");
  EXPECT_TRUE(realm2_entry == entry);
  entry = cache.LookupByPath(origin, "/foo2/bar/index.html");
  EXPECT_TRUE(realm2_entry == entry);
  entry = cache.LookupByPath(origin, "/foo2/");
  EXPECT_TRUE(realm2_entry == entry);

  // Negative tests:
  entry = cache.LookupByPath(origin, "/foo2");
  EXPECT_FALSE(realm2_entry == entry);
  entry = cache.LookupByPath(origin, "/foo3/index.html");
  EXPECT_FALSE(realm2_entry == entry);
  entry = cache.LookupByPath(origin, "");
  EXPECT_FALSE(realm2_entry == entry);
  entry = cache.LookupByPath(origin, "/");
  EXPECT_FALSE(realm2_entry == entry);

  // Confirm we find the same realm, different auth scheme by path lookup
  HttpAuthCache::Entry* realm3_digest_entry =
      cache.Lookup(origin, "Realm3", "digest");
  EXPECT_FALSE(NULL == realm3_digest_entry);
  entry = cache.LookupByPath(origin, "/baz/index.html");
  EXPECT_TRUE(realm3_digest_entry == entry);
  entry = cache.LookupByPath(origin, "/baz/");
  EXPECT_TRUE(realm3_digest_entry == entry);
  entry = cache.LookupByPath(origin, "/baz");
  EXPECT_FALSE(realm3_digest_entry == entry);

  // Confirm we find the same realm, different auth scheme by path lookup
  HttpAuthCache::Entry* realm3DigestEntry =
      cache.Lookup(origin, "Realm3", "digest");
  EXPECT_FALSE(NULL == realm3DigestEntry);
  entry = cache.LookupByPath(origin, "/baz/index.html");
  EXPECT_TRUE(realm3DigestEntry == entry);
  entry = cache.LookupByPath(origin, "/baz/");
  EXPECT_TRUE(realm3DigestEntry == entry);
  entry = cache.LookupByPath(origin, "/baz");
  EXPECT_FALSE(realm3DigestEntry == entry);

  // Lookup using empty path (may be used for proxy).
  entry = cache.LookupByPath(origin, "");
  EXPECT_FALSE(NULL == entry);
  EXPECT_EQ("basic", entry->scheme());
  EXPECT_EQ("Realm3", entry->realm());
}

TEST(HttpAuthCacheTest, AddPath) {
  HttpAuthCache::Entry entry;

  // All of these paths have a common root /1/2/2/4/5/
  entry.AddPath("/1/2/3/4/5/x.txt");
  entry.AddPath("/1/2/3/4/5/y.txt");
  entry.AddPath("/1/2/3/4/5/z.txt");

  EXPECT_EQ(1U, entry.paths_.size());
  EXPECT_EQ("/1/2/3/4/5/", entry.paths_.front());

  // Add a new entry (not a subpath).
  entry.AddPath("/1/XXX/q");
  EXPECT_EQ(2U, entry.paths_.size());
  EXPECT_EQ("/1/XXX/", entry.paths_.front());
  EXPECT_EQ("/1/2/3/4/5/", entry.paths_.back());

  // Add containing paths of /1/2/3/4/5/ -- should swallow up the deeper paths.
  entry.AddPath("/1/2/3/4/x.txt");
  EXPECT_EQ(2U, entry.paths_.size());
  EXPECT_EQ("/1/2/3/4/", entry.paths_.front());
  EXPECT_EQ("/1/XXX/", entry.paths_.back());
  entry.AddPath("/1/2/3/x");
  EXPECT_EQ(2U, entry.paths_.size());
  EXPECT_EQ("/1/2/3/", entry.paths_.front());
  EXPECT_EQ("/1/XXX/", entry.paths_.back());

  entry.AddPath("/index.html");
  EXPECT_EQ(1U, entry.paths_.size());
  EXPECT_EQ("/", entry.paths_.front());
}

// Calling Add when the realm entry already exists, should append that
// path.
TEST(HttpAuthCacheTest, AddToExistingEntry) {
  HttpAuthCache cache;
  GURL origin("http://www.foobar.com:70");
  const std::string auth_challenge = "Basic realm=MyRealm";

  scoped_ptr<HttpAuthHandler> handler(
      new MockAuthHandler("basic", "MyRealm", HttpAuth::AUTH_SERVER));

  HttpAuthCache::Entry* orig_entry = cache.Add(
      origin, handler->realm(), handler->scheme(), auth_challenge,
      L"user1", L"password1", "/x/y/z/");
  cache.Add(origin, handler->realm(), handler->scheme(), auth_challenge,
            L"user2", L"password2", "/z/y/x/");
  cache.Add(origin, handler->realm(), handler->scheme(), auth_challenge,
            L"user3", L"password3", "/z/y");

  HttpAuthCache::Entry* entry = cache.Lookup(origin, "MyRealm", "basic");

  EXPECT_TRUE(entry == orig_entry);
  EXPECT_EQ(L"user3", entry->username());
  EXPECT_EQ(L"password3", entry->password());

  EXPECT_EQ(2U, entry->paths_.size());
  EXPECT_EQ("/z/", entry->paths_.front());
  EXPECT_EQ("/x/y/z/", entry->paths_.back());
}

TEST(HttpAuthCacheTest, Remove) {
  GURL origin("http://foobar2.com");

  scoped_ptr<HttpAuthHandler> realm1_handler(
      new MockAuthHandler("basic", "Realm1", HttpAuth::AUTH_SERVER));

  scoped_ptr<HttpAuthHandler> realm2_handler(
      new MockAuthHandler("basic", "Realm2", HttpAuth::AUTH_SERVER));

  scoped_ptr<HttpAuthHandler> realm3_basic_handler(
      new MockAuthHandler("basic", "Realm3", HttpAuth::AUTH_SERVER));

  scoped_ptr<HttpAuthHandler> realm3_digest_handler(
      new MockAuthHandler("digest", "Realm3", HttpAuth::AUTH_SERVER));

  HttpAuthCache cache;
  cache.Add(origin, realm1_handler->realm(), realm1_handler->scheme(),
            "basic realm=Realm1", L"alice", L"123", "/");
  cache.Add(origin, realm2_handler->realm(), realm2_handler->scheme(),
            "basic realm=Realm2", L"bob", L"princess", "/");
  cache.Add(origin, realm3_basic_handler->realm(),
            realm3_basic_handler->scheme(), "basic realm=Realm3", L"admin",
            L"password", "/");
  cache.Add(origin, realm3_digest_handler->realm(),
            realm3_digest_handler->scheme(), "digest realm=Realm3", L"root",
            L"wilecoyote", "/");

  // Fails, because there is no realm "Realm4".
  EXPECT_FALSE(cache.Remove(origin, "Realm4", "basic", L"alice", L"123"));

  // Fails because the origin is wrong.
  EXPECT_FALSE(cache.Remove(
      GURL("http://foobar2.com:100"), "Realm1", "basic", L"alice", L"123"));

  // Fails because the username is wrong.
  EXPECT_FALSE(cache.Remove(origin, "Realm1", "basic", L"alice2", L"123"));

  // Fails because the password is wrong.
  EXPECT_FALSE(cache.Remove(origin, "Realm1", "basic", L"alice", L"1234"));

  // Fails because the authentication type is wrong.
  EXPECT_FALSE(cache.Remove(origin, "Realm1", "digest", L"alice", L"123"));

  // Succeeds.
  EXPECT_TRUE(cache.Remove(origin, "Realm1", "basic", L"alice", L"123"));

  // Fails because we just deleted the entry!
  EXPECT_FALSE(cache.Remove(origin, "Realm1", "basic", L"alice", L"123"));

  // Succeed when there are two authentication types for the same origin,realm.
  EXPECT_TRUE(cache.Remove(origin, "Realm3", "digest", L"root", L"wilecoyote"));

  // Succeed as above, but when entries were added in opposite order
  cache.Add(origin, realm3_digest_handler->realm(),
            realm3_digest_handler->scheme(), "digest realm=Realm3", L"root",
            L"wilecoyote", "/");
  EXPECT_TRUE(cache.Remove(origin, "Realm3", "basic", L"admin", L"password"));

  // Make sure that removing one entry still leaves the other available for
  // lookup.
  HttpAuthCache::Entry* entry = cache.Lookup(origin, "Realm3", "digest");
  EXPECT_FALSE(NULL == entry);
}

// Test fixture class for eviction tests (contains helpers for bulk
// insertion and existence testing).
class HttpAuthCacheEvictionTest : public testing::Test {
 protected:
  HttpAuthCacheEvictionTest() : origin_("http://www.google.com") { }

  std::string GenerateRealm(int realm_i) {
    return StringPrintf("Realm %d", realm_i);
  }

  std::string GeneratePath(int realm_i, int path_i) {
    return StringPrintf("/%d/%d/x/y", realm_i, path_i);
  }

  void AddRealm(int realm_i) {
    AddPathToRealm(realm_i, 0);
  }

  void AddPathToRealm(int realm_i, int path_i) {
    cache_.Add(origin_, GenerateRealm(realm_i), "basic", "",
               L"username", L"password", GeneratePath(realm_i, path_i));
  }

  void CheckRealmExistence(int realm_i, bool exists) {
    const HttpAuthCache::Entry* entry =
        cache_.Lookup(origin_, GenerateRealm(realm_i), "basic");
    if (exists) {
      EXPECT_FALSE(entry == NULL);
      EXPECT_EQ(GenerateRealm(realm_i), entry->realm());
    } else {
      EXPECT_TRUE(entry == NULL);
    }
  }

  void CheckPathExistence(int realm_i, int path_i, bool exists) {
    const HttpAuthCache::Entry* entry =
        cache_.LookupByPath(origin_, GeneratePath(realm_i, path_i));
    if (exists) {
      EXPECT_FALSE(entry == NULL);
      EXPECT_EQ(GenerateRealm(realm_i), entry->realm());
    } else {
      EXPECT_TRUE(entry == NULL);
    }
  }

  GURL origin_;
  HttpAuthCache cache_;

  static const int kMaxPaths = HttpAuthCache::kMaxNumPathsPerRealmEntry;
  static const int kMaxRealms = HttpAuthCache::kMaxNumRealmEntries;
};

// Add the maxinim number of realm entries to the cache. Each of these entries
// must still be retrievable. Next add three more entries -- since the cache is
// full this causes FIFO eviction of the first three entries.
TEST_F(HttpAuthCacheEvictionTest, RealmEntryEviction) {
  for (int i = 0; i < kMaxRealms; ++i)
    AddRealm(i);

  for (int i = 0; i < kMaxRealms; ++i)
    CheckRealmExistence(i, true);

  for (int i = 0; i < 3; ++i)
    AddRealm(i + kMaxRealms);

  for (int i = 0; i < 3; ++i)
    CheckRealmExistence(i, false);

  for (int i = 0; i < kMaxRealms; ++i)
    CheckRealmExistence(i + 3, true);
}

// Add the maximum number of paths to a single realm entry. Each of these
// paths should be retrievable. Next add 3 more paths -- since the cache is
// full this causes FIFO eviction of the first three paths.
TEST_F(HttpAuthCacheEvictionTest, RealmPathEviction) {
  for (int i = 0; i < kMaxPaths; ++i)
    AddPathToRealm(0, i);

  for (int i = 1; i < kMaxRealms; ++i)
    AddRealm(i);

  for (int i = 0; i < 3; ++i)
    AddPathToRealm(0, i + kMaxPaths);

  for (int i = 0; i < 3; ++i)
    CheckPathExistence(0, i, false);

  for (int i = 0; i < kMaxPaths; ++i)
    CheckPathExistence(0, i + 3, true);

  for (int i = 0; i < kMaxRealms; ++i)
    CheckRealmExistence(i, true);
}

}  // namespace net
