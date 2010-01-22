// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/ref_counted.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/renderer_host/render_view_host.h"
#include "chrome/browser/extensions/autoupdate_interceptor.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/browser/extensions/extension_host.h"
#include "chrome/browser/extensions/extensions_service.h"
#include "chrome/browser/extensions/extension_updater.h"
#include "chrome/browser/profile.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/ui_test_utils.h"

class ExtensionManagementTest : public ExtensionBrowserTest {
 protected:
  // Helper method that returns whether the extension is at the given version.
  // This calls version(), which must be defined in the extension's bg page,
  // as well as asking the extension itself.
  bool IsExtensionAtVersion(Extension* extension,
                            const std::string& expected_version) {
    // Test that the extension's version from the manifest and reported by the
    // background page is correct.  This is to ensure that the processes are in
    // sync with the Extension.
    ExtensionProcessManager* manager = browser()->profile()->
        GetExtensionProcessManager();
    ExtensionHost* ext_host = manager->GetBackgroundHostForExtension(extension);
    EXPECT_TRUE(ext_host);
    if (!ext_host)
      return false;

    std::string version_from_bg;
    bool exec = ui_test_utils::ExecuteJavaScriptAndExtractString(
        ext_host->render_view_host(), L"", L"version()", &version_from_bg);
    EXPECT_TRUE(exec);
    if (!exec)
      return false;

    if (version_from_bg != expected_version ||
        extension->VersionString() != expected_version)
      return false;
    return true;
  }

  // Helper method that installs a low permission extension then updates
  // to the second version requiring increased permissions. Returns whether
  // the operation was completed successfully.
  bool InstallAndUpdateIncreasingPermissionsExtension() {
    ExtensionsService* service = browser()->profile()->GetExtensionsService();
    if (service->HasInstalledExtensions())
      return false;

    // Install the initial version, which should happen just fine.
    if (!InstallExtension(
        test_data_dir_.AppendASCII("permissions-low-v1.crx"), 1))
      return false;

    // Upgrade to a version that wants more permissions. We should disable the
    // extension and prompt the user to reenable.
    if (service->extensions()->size() != 1u)
      return false;
    if (!UpdateExtension(
        service->extensions()->at(0)->id(),
        test_data_dir_.AppendASCII("permissions-high-v2.crx"), -1))
      return false;
    EXPECT_EQ(0u, service->extensions()->size());
    if (service->disabled_extensions()->size() != 1u)
      return false;
    return true;
  }
};

// Tests that installing the same version does not overwrite.
IN_PROC_BROWSER_TEST_F(ExtensionManagementTest, InstallSameVersion) {
  ExtensionsService* service = browser()->profile()->GetExtensionsService();
  ASSERT_FALSE(service->HasInstalledExtensions());
  ASSERT_TRUE(InstallExtension(
      test_data_dir_.AppendASCII("install/install.crx"), 1));

  // Install an extension with the same version. The previous install should
  // be kept.
  ASSERT_TRUE(InstallExtension(
      test_data_dir_.AppendASCII("install/install_same_version.crx"), 0));
  EXPECT_TRUE(IsExtensionAtVersion(service->extensions()->at(0), "1.0"));
}

IN_PROC_BROWSER_TEST_F(ExtensionManagementTest, InstallOlderVersion) {
  ExtensionsService* service = browser()->profile()->GetExtensionsService();
  ASSERT_FALSE(service->HasInstalledExtensions());
  ASSERT_TRUE(InstallExtension(
      test_data_dir_.AppendASCII("install/install.crx"), 1));
  ASSERT_TRUE(InstallExtension(
      test_data_dir_.AppendASCII("install/install_older_version.crx"), 0));
  EXPECT_TRUE(IsExtensionAtVersion(service->extensions()->at(0), "1.0"));
}

IN_PROC_BROWSER_TEST_F(ExtensionManagementTest, InstallThenCancel) {
  ExtensionsService* service = browser()->profile()->GetExtensionsService();
  ASSERT_FALSE(service->HasInstalledExtensions());
  ASSERT_TRUE(InstallExtension(
      test_data_dir_.AppendASCII("install/install.crx"), 1));

  // Cancel this install.
  StartInstallButCancel(test_data_dir_.AppendASCII("install/install_v2.crx"));
  EXPECT_TRUE(IsExtensionAtVersion(service->extensions()->at(0), "1.0"));
}

// Tests that installing and uninstalling extensions don't crash with an
// incognito window open.
IN_PROC_BROWSER_TEST_F(ExtensionManagementTest, Incognito) {
  // Open an incognito window to the extensions management page.  We just
  // want to make sure that we don't crash while playing with extensions when
  // this guy is around.
  ui_test_utils::OpenURLOffTheRecord(browser()->profile(),
                                     GURL(chrome::kChromeUIExtensionsURL));

  ASSERT_TRUE(InstallExtension(test_data_dir_.AppendASCII("good.crx"), 1));
  UninstallExtension("ldnnhddmnhbkjipkidpdiheffobcpfmf");
}

// Tests the process of updating an extension to one that requires higher
// permissions.
IN_PROC_BROWSER_TEST_F(ExtensionManagementTest, UpdatePermissions) {
  ExtensionsService* service = browser()->profile()->GetExtensionsService();
  ASSERT_TRUE(InstallAndUpdateIncreasingPermissionsExtension());

  // Now try reenabling it.
  service->EnableExtension(service->disabled_extensions()->at(0)->id());
  EXPECT_EQ(1u, service->extensions()->size());
  EXPECT_EQ(0u, service->disabled_extensions()->size());
}

// Tests that we can uninstall a disabled extension.
IN_PROC_BROWSER_TEST_F(ExtensionManagementTest, UninstallDisabled) {
  ExtensionsService* service = browser()->profile()->GetExtensionsService();
  ASSERT_TRUE(InstallAndUpdateIncreasingPermissionsExtension());

  // Now try uninstalling it.
  UninstallExtension(service->disabled_extensions()->at(0)->id());
  EXPECT_EQ(0u, service->extensions()->size());
  EXPECT_EQ(0u, service->disabled_extensions()->size());
  ASSERT_FALSE(service->HasInstalledExtensions());
}

// Tests that disabling and re-enabling an extension works.
IN_PROC_BROWSER_TEST_F(ExtensionManagementTest, DisableEnable) {
  ExtensionProcessManager* manager = browser()->profile()->
      GetExtensionProcessManager();
  ExtensionsService* service = browser()->profile()->GetExtensionsService();

  // Load an extension, expect the background page to be available.
  ASSERT_FALSE(service->HasInstalledExtensions());
  ASSERT_TRUE(LoadExtension(
      test_data_dir_.AppendASCII("good").AppendASCII("Extensions")
                    .AppendASCII("bjafgdebaacbbbecmhlhpofkepfkgcpa")
                    .AppendASCII("1.0")));
  ASSERT_EQ(1u, service->extensions()->size());
  EXPECT_EQ(0u, service->disabled_extensions()->size());
  Extension* extension = service->extensions()->at(0);
  EXPECT_TRUE(manager->GetBackgroundHostForExtension(extension));
  ASSERT_TRUE(service->HasInstalledExtensions());

  // After disabling, the background page should go away.
  service->DisableExtension("bjafgdebaacbbbecmhlhpofkepfkgcpa");
  EXPECT_EQ(0u, service->extensions()->size());
  EXPECT_EQ(1u, service->disabled_extensions()->size());
  EXPECT_FALSE(manager->GetBackgroundHostForExtension(extension));
  ASSERT_TRUE(service->HasInstalledExtensions());

  // And bring it back.
  service->EnableExtension("bjafgdebaacbbbecmhlhpofkepfkgcpa");
  EXPECT_EQ(1u, service->extensions()->size());
  EXPECT_EQ(0u, service->disabled_extensions()->size());
  EXPECT_TRUE(manager->GetBackgroundHostForExtension(extension));
  ASSERT_TRUE(service->HasInstalledExtensions());
}

// TODO(asargent): This test seems to crash on linux buildbots.
// (http://crbug.com/31737)
#if !defined(OS_LINUX)
// Tests extension autoupdate.
IN_PROC_BROWSER_TEST_F(ExtensionManagementTest, AutoUpdate) {
  FilePath basedir = test_data_dir_.AppendASCII("autoupdate");
  // Note: This interceptor gets requests on the IO thread.
  scoped_refptr<AutoUpdateInterceptor> interceptor(new AutoUpdateInterceptor());
  URLFetcher::enable_interception_for_tests(true);

  interceptor->SetResponseOnIOThread("http://localhost/autoupdate/manifest",
                                     basedir.AppendASCII("manifest_v2.xml"));
  interceptor->SetResponseOnIOThread("http://localhost/autoupdate/v2.crx",
                                     basedir.AppendASCII("v2.crx"));

  // Install version 1 of the extension.
  ExtensionsService* service = browser()->profile()->GetExtensionsService();
  ASSERT_FALSE(service->HasInstalledExtensions());
  ASSERT_TRUE(InstallExtension(basedir.AppendASCII("v1.crx"), 1));
  const ExtensionList* extensions = service->extensions();
  ASSERT_TRUE(service->HasInstalledExtensions());
  ASSERT_EQ(1u, extensions->size());
  ASSERT_EQ("ogjcoiohnmldgjemafoockdghcjciccf", extensions->at(0)->id());
  ASSERT_EQ("1.0", extensions->at(0)->VersionString());

  // We don't want autoupdate blacklist checks.
  service->updater()->set_blacklist_checks_enabled(false);

  // Run autoupdate and make sure version 2 of the extension was installed.
  service->updater()->CheckNow();
  ASSERT_TRUE(WaitForExtensionInstall());
  extensions = service->extensions();
  ASSERT_EQ(1u, extensions->size());
  ASSERT_EQ("ogjcoiohnmldgjemafoockdghcjciccf", extensions->at(0)->id());
  ASSERT_EQ("2.0", extensions->at(0)->VersionString());

  // Now try doing an update to version 3, which has been incorrectly
  // signed. This should fail.
  interceptor->SetResponseOnIOThread("http://localhost/autoupdate/manifest",
                                     basedir.AppendASCII("manifest_v3.xml"));
  interceptor->SetResponseOnIOThread("http://localhost/autoupdate/v3.crx",
                                     basedir.AppendASCII("v3.crx"));

  service->updater()->CheckNow();
  ASSERT_TRUE(WaitForExtensionInstallError());

  // Make sure the extension state is the same as before.
  extensions = service->extensions();
  ASSERT_EQ(1u, extensions->size());
  ASSERT_EQ("ogjcoiohnmldgjemafoockdghcjciccf", extensions->at(0)->id());
  ASSERT_EQ("2.0", extensions->at(0)->VersionString());
}
#endif  // !defined(OS_LINUX)
