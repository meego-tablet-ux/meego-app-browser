// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"

#include "base/file_path.h"
#include "base/file_util.h"
#include "base/path_service.h"
#include "chrome/browser/importer/firefox2_importer.h"
#include "chrome/browser/importer/firefox_importer_utils.h"
#include "chrome/browser/importer/nss_decryptor.h"
#include "chrome/common/chrome_paths.h"

using base::Time;

TEST(FirefoxImporterTest, Firefox2NSS3Decryptor) {
  std::wstring nss_path;
  ASSERT_TRUE(PathService::Get(chrome::DIR_TEST_DATA, &nss_path));
  file_util::AppendToPath(&nss_path, L"firefox2_nss");
  std::wstring db_path;
  ASSERT_TRUE(PathService::Get(chrome::DIR_TEST_DATA, &db_path));
  file_util::AppendToPath(&db_path, L"firefox2_profile");
  NSSDecryptor decryptor;
  EXPECT_TRUE(decryptor.Init(nss_path, db_path));
  EXPECT_EQ(L"hello", decryptor.Decrypt("MDIEEPgAAAAAAAAAAAAAAAAAAAE"
      "wFAYIKoZIhvcNAwcECBJM63MpT9rtBAjMCm7qo/EhlA=="));
  // Test UTF-16 encoding.
  EXPECT_EQ(L"\x4E2D", decryptor.Decrypt("MDIEEPgAAAAAAAAAAAAAAAAAAAE"
      "wFAYIKoZIhvcNAwcECN9OQ5ZFmhb8BAiFo1Z+fUvaIQ=="));
}

TEST(FirefoxImporterTest, Firefox3NSS3Decryptor) {
  std::wstring nss_path;
  ASSERT_TRUE(PathService::Get(chrome::DIR_TEST_DATA, &nss_path));
  file_util::AppendToPath(&nss_path, L"firefox3_nss");
  std::wstring db_path;
  ASSERT_TRUE(PathService::Get(chrome::DIR_TEST_DATA, &db_path));
  file_util::AppendToPath(&db_path, L"firefox3_profile");
  NSSDecryptor decryptor;
  EXPECT_TRUE(decryptor.Init(nss_path, db_path));
  EXPECT_EQ(L"hello", decryptor.Decrypt("MDIEEPgAAAAAAAAAAAAAAAAAAAE"
      "wFAYIKoZIhvcNAwcECKajtRg4qFSHBAhv9luFkXgDJA=="));
  // Test UTF-16 encoding.
  EXPECT_EQ(L"\x4E2D", decryptor.Decrypt("MDIEEPgAAAAAAAAAAAAAAAAAAAE"
      "wFAYIKoZIhvcNAwcECLWqqiccfQHWBAie74hxnULxlw=="));
}

TEST(FirefoxImporterTest, Firefox2BookmarkParse) {
  bool result;

  // Tests charset.
  std::string charset;
  result = Firefox2Importer::ParseCharsetFromLine(
      "<META HTTP-EQUIV=\"Content-Type\" "
      "CONTENT=\"text/html; charset=UTF-8\">",
      &charset);
  EXPECT_TRUE(result);
  EXPECT_EQ("UTF-8", charset);

  // Escaped characters in name.
  std::wstring folder_name;
  bool is_toolbar_folder;
  result = Firefox2Importer::ParseFolderNameFromLine(
      "<DT><H3 ADD_DATE=\"1207558707\" >&lt; &gt;"
      " &amp; &quot; &#39; \\ /</H3>",
      charset, &folder_name, &is_toolbar_folder);
  EXPECT_TRUE(result);
  EXPECT_EQ(L"< > & \" ' \\ /", folder_name);
  EXPECT_EQ(false, is_toolbar_folder);

  // Empty name and toolbar folder attribute.
  result = Firefox2Importer::ParseFolderNameFromLine(
      "<DT><H3 PERSONAL_TOOLBAR_FOLDER=\"true\"></H3>",
      charset, &folder_name, &is_toolbar_folder);
  EXPECT_TRUE(result);
  EXPECT_EQ(L"", folder_name);
  EXPECT_EQ(true, is_toolbar_folder);

  // Unicode characters in title and shortcut.
  std::wstring title;
  GURL url, favicon;
  std::wstring shortcut;
  std::wstring post_data;
  Time add_date;
  result = Firefox2Importer::ParseBookmarkFromLine(
      "<DT><A HREF=\"http://chinese.site.cn/path?query=1#ref\" "
      "SHORTCUTURL=\"\xE4\xB8\xAD\">\xE4\xB8\xAD\xE6\x96\x87</A>",
      charset, &title, &url, &favicon, &shortcut, &add_date, &post_data);
  EXPECT_TRUE(result);
  EXPECT_EQ(L"\x4E2D\x6587", title);
  EXPECT_EQ("http://chinese.site.cn/path?query=1#ref", url.spec());
  EXPECT_EQ(L"\x4E2D", shortcut);
  EXPECT_EQ(L"", post_data);
  EXPECT_TRUE(Time() == add_date);

  // No shortcut, and url contains %22 ('"' character).
  result = Firefox2Importer::ParseBookmarkFromLine(
      "<DT><A HREF=\"http://domain.com/?q=%22<>%22\">name</A>",
      charset, &title, &url, &favicon, &shortcut, &add_date, &post_data);
  EXPECT_TRUE(result);
  EXPECT_EQ(L"name", title);
  EXPECT_EQ("http://domain.com/?q=%22%3C%3E%22", url.spec());
  EXPECT_EQ(L"", shortcut);
  EXPECT_EQ(L"", post_data);
  EXPECT_TRUE(Time() == add_date);

  result = Firefox2Importer::ParseBookmarkFromLine(
      "<DT><A HREF=\"http://domain.com/?g=&quot;\"\">name</A>",
      charset, &title, &url, &favicon, &shortcut, &add_date, &post_data);
  EXPECT_TRUE(result);
  EXPECT_EQ(L"name", title);
  EXPECT_EQ("http://domain.com/?g=%22", url.spec());
  EXPECT_EQ(L"", shortcut);
  EXPECT_EQ(L"", post_data);
  EXPECT_TRUE(Time() == add_date);

  // Creation date.
  result = Firefox2Importer::ParseBookmarkFromLine(
      "<DT><A HREF=\"http://site/\" ADD_DATE=\"1121301154\">name</A>",
      charset, &title, &url, &favicon, &shortcut, &add_date, &post_data);
  EXPECT_TRUE(result);
  EXPECT_EQ(L"name", title);
  EXPECT_EQ(GURL("http://site/"), url);
  EXPECT_EQ(L"", shortcut);
  EXPECT_EQ(L"", post_data);
  EXPECT_TRUE(Time::FromTimeT(1121301154) == add_date);

  // Post-data
  result = Firefox2Importer::ParseBookmarkFromLine(
      "<DT><A HREF=\"http://localhost:8080/test/hello.html\" ADD_DATE=\""
      "1212447159\" LAST_VISIT=\"1212447251\" LAST_MODIFIED=\"1212447248\""
      "SHORTCUTURL=\"post\" ICON=\"data:\" POST_DATA=\"lname%3D%25s\""
      "LAST_CHARSET=\"UTF-8\" ID=\"rdf:#$weKaR3\">Test Post keyword</A>",
      charset, &title, &url, &favicon, &shortcut, &add_date, &post_data);
  EXPECT_TRUE(result);
  EXPECT_EQ(L"Test Post keyword", title);
  EXPECT_EQ("http://localhost:8080/test/hello.html", url.spec());
  EXPECT_EQ(L"post", shortcut);
  EXPECT_EQ(L"lname%3D%25s", post_data);
  EXPECT_TRUE(Time::FromTimeT(1212447159) == add_date);

  // Invalid case.
  result = Firefox2Importer::ParseBookmarkFromLine(
      "<DT><A HREF=\"http://domain.com/?q=%22",
      charset, &title, &url, &favicon, &shortcut, &add_date, &post_data);
  EXPECT_FALSE(result);
  EXPECT_EQ(L"", title);
  EXPECT_EQ("", url.spec());
  EXPECT_EQ(L"", shortcut);
  EXPECT_EQ(L"", post_data);
  EXPECT_TRUE(Time() == add_date);
}
