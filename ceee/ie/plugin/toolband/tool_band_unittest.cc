// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// IE toolband unit tests.
#include "ceee/ie/plugin/toolband/tool_band.h"

#include <exdisp.h>
#include <shlguid.h>

#include "ceee/common/initializing_coclass.h"
#include "ceee/ie/common/mock_ceee_module_util.h"
#include "ceee/ie/testing/mock_browser_and_friends.h"
#include "ceee/testing/utils/dispex_mocks.h"
#include "ceee/testing/utils/instance_count_mixin.h"
#include "ceee/testing/utils/test_utils.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "broker_lib.h"  // NOLINT

namespace {

using testing::GetConnectionCount;
using testing::InstanceCountMixin;
using testing::MockDispatchEx;
using testing::Return;
using testing::StrictMock;
using testing::TestBrowser;
using testing::TestBrowserSite;

// Makes ToolBand testable - circumvents InitializeAndShowWindow.
class TestingToolBand
    : public ToolBand,
      public InstanceCountMixin<TestingToolBand>,
      public InitializingCoClass<TestingToolBand> {
 public:
  HRESULT Initialize(TestingToolBand** self) {
    *self = this;
    return S_OK;
  }
 private:
  virtual HRESULT InitializeAndShowWindow(IUnknown* site) {
    return S_OK;  // This aspect is not tested.
  }
};

class ToolBandTest: public testing::Test {
 public:
  ToolBandTest() : tool_band_(NULL), site_(NULL), browser_(NULL) {
  }

  ~ToolBandTest() {
  }

  virtual void SetUp() {
    // Create the instance to test.
    ASSERT_HRESULT_SUCCEEDED(
        TestingToolBand::CreateInitialized(&tool_band_, &tool_band_with_site_));
    tool_band_with_site_ = tool_band_;

    ASSERT_TRUE(tool_band_with_site_ != NULL);
  }

  virtual void TearDown() {
    tool_band_ = NULL;
    tool_band_with_site_.Release();

    site_ = NULL;
    site_keeper_.Release();

    browser_ = NULL;
    browser_keeper_.Release();

    // Everything should have been relinquished.
    ASSERT_EQ(0, testing::InstanceCountMixinBase::all_instance_count());
  }

  void CreateSite() {
    ASSERT_HRESULT_SUCCEEDED(
        TestBrowserSite::CreateInitialized(&site_, &site_keeper_));
  }

  void CreateBrowser() {
    ASSERT_HRESULT_SUCCEEDED(
        TestBrowser::CreateInitialized(&browser_, &browser_keeper_));

    if (site_)
      site_->browser_ = browser_keeper_;
  }

  bool ToolbandHasSite() {
    // Check whether ToolBand has a site set.
    CComPtr<IUnknown> site;
    if (SUCCEEDED(tool_band_with_site_->GetSite(
            IID_IUnknown, reinterpret_cast<void**>(&site)))) {
      return true;
    }

    // If GetSite failed and site != NULL, we are seeing things.
    DCHECK(site == NULL);
    return false;
  }

  static void PrepareDeskBandInfo(DESKBANDINFO* pdinfo_for_test) {
    memset(pdinfo_for_test, 0, sizeof(*pdinfo_for_test));

    // What I really care in this test is DBIM_MODEFLAGS, but if there
    // are weird interactions here, we want to be warned.
    pdinfo_for_test->dwMask = DBIM_MODEFLAGS | DBIM_MAXSIZE | DBIM_MINSIZE |
                              DBIM_TITLE | DBIM_INTEGRAL;
  }

  static const wchar_t* kUrl1;

  testing::TestBrowserSite* site_;
  CComPtr<IUnknown> site_keeper_;

  TestBrowser* browser_;
  CComPtr<IWebBrowser2> browser_keeper_;

  TestingToolBand* tool_band_;
  CComPtr<IObjectWithSite> tool_band_with_site_;

  // the purpose of this mock is to redirect registry calls
  StrictMock<testing::MockCeeeModuleUtils> ceee_module_utils_;
};

const wchar_t* ToolBandTest::kUrl1 = L"http://www.google.com";


// Setting the ToolBand site with a non-service provider fails.
TEST_F(ToolBandTest, SetSiteWithNoServiceProviderFails) {
  testing::LogDisabler no_dchecks;

  // Create an object that doesn't implement IServiceProvider.
  MockDispatchEx* site = NULL;
  CComPtr<IUnknown> site_keeper;
  ASSERT_HRESULT_SUCCEEDED(
      InitializingCoClass<MockDispatchEx>::CreateInitialized(&site,
                                                             &site_keeper));
  // Setting a site that doesn't implement IServiceProvider fails.
  ASSERT_HRESULT_FAILED(tool_band_with_site_->SetSite(site_keeper));
  ASSERT_FALSE(ToolbandHasSite());
}

// Setting the ToolBand site with no browser fails.
TEST_F(ToolBandTest, SetSiteWithNullBrowserFails) {
  testing::LogDisabler no_dchecks;

  CreateSite();
  ASSERT_HRESULT_FAILED(tool_band_with_site_->SetSite(site_keeper_));
  ASSERT_FALSE(ToolbandHasSite());
}

// Setting the ToolBand site with a non-browser fails.
TEST_F(ToolBandTest, SetSiteWithNonBrowserFails) {
  testing::LogDisabler no_dchecks;

  CreateSite();
  // Endow the site with a non-browser service.
  MockDispatchEx* mock_non_browser = NULL;
  ASSERT_HRESULT_SUCCEEDED(
      InitializingCoClass<MockDispatchEx>::CreateInitialized(&mock_non_browser,
                                                             &site_->browser_));
  ASSERT_HRESULT_FAILED(tool_band_with_site_->SetSite(site_keeper_));
  ASSERT_FALSE(ToolbandHasSite());
}

// Setting the ToolBand site with a browser that doesn't implement the
// DIID_DWebBrowserEvents2 still works.
TEST_F(ToolBandTest, SetSiteWithNoEventsWorksAnyway) {
  // We need to quash dcheck here, too (see: ToolBand::Initialize).
  testing::LogDisabler no_dchecks;
  CreateSite();
  CreateBrowser();

  // Disable the connection point.
  browser_->no_events_ = true;

  // Successful SetSite always calls GetOptionToolbandForceReposition
  EXPECT_CALL(ceee_module_utils_, GetOptionToolbandForceReposition())
      .WillOnce(Return(false));

  ASSERT_HRESULT_SUCCEEDED(tool_band_with_site_->SetSite(site_keeper_));
  ASSERT_TRUE(ToolbandHasSite());
}

TEST_F(ToolBandTest, SetSiteWithBrowserSucceeds) {
  CreateSite();
  CreateBrowser();

  size_t num_connections = 0;
  ASSERT_HRESULT_SUCCEEDED(GetConnectionCount(browser_keeper_,
                                              DIID_DWebBrowserEvents2,
                                              &num_connections));
  ASSERT_EQ(0, num_connections);

  EXPECT_CALL(ceee_module_utils_, GetOptionToolbandForceReposition())
      .WillOnce(Return(false));

  ASSERT_HRESULT_SUCCEEDED(tool_band_with_site_->SetSite(site_keeper_));

  // Check that the we have not set the connection if not strictly required.
  ASSERT_HRESULT_SUCCEEDED(GetConnectionCount(browser_keeper_,
                                              DIID_DWebBrowserEvents2,
                                              &num_connections));
  ASSERT_EQ(0, num_connections);

  // Check the site's retained.
  CComPtr<IUnknown> set_site;
  ASSERT_HRESULT_SUCCEEDED(tool_band_with_site_->GetSite(
      IID_IUnknown, reinterpret_cast<void**>(&set_site)));
  ASSERT_TRUE(set_site.IsEqualObject(site_keeper_));

  ASSERT_HRESULT_SUCCEEDED(tool_band_with_site_->SetSite(NULL));
}

TEST_F(ToolBandTest, SetSiteEstablishesConnectionWhenRequired) {
  CreateSite();
  CreateBrowser();

  size_t num_connections = 0;
  ASSERT_HRESULT_SUCCEEDED(GetConnectionCount(browser_keeper_,
                                              DIID_DWebBrowserEvents2,
                                              &num_connections));
  ASSERT_EQ(0, num_connections);

  EXPECT_CALL(ceee_module_utils_, GetOptionToolbandForceReposition())
      .WillOnce(Return(true));

  ASSERT_HRESULT_SUCCEEDED(tool_band_with_site_->SetSite(site_keeper_));

  // Check that the we have not set the connection if not strictly required.
  ASSERT_HRESULT_SUCCEEDED(GetConnectionCount(browser_keeper_,
                                              DIID_DWebBrowserEvents2,
                                              &num_connections));
  ASSERT_EQ(1, num_connections);

  // Check the site's retained.
  CComPtr<IUnknown> set_site;
  ASSERT_HRESULT_SUCCEEDED(tool_band_with_site_->GetSite(
      IID_IUnknown, reinterpret_cast<void**>(&set_site)));
  ASSERT_TRUE(set_site.IsEqualObject(site_keeper_));

  ASSERT_HRESULT_SUCCEEDED(tool_band_with_site_->SetSite(NULL));

  // And check that the connection was severed.
  ASSERT_HRESULT_SUCCEEDED(GetConnectionCount(browser_keeper_,
                                              DIID_DWebBrowserEvents2,
                                              &num_connections));
  ASSERT_EQ(0, num_connections);
}

TEST_F(ToolBandTest, NavigationCompleteResetsFlagAndUnadvises) {
  CreateSite();
  CreateBrowser();

  size_t num_connections = 0;
  ASSERT_HRESULT_SUCCEEDED(GetConnectionCount(browser_keeper_,
                                              DIID_DWebBrowserEvents2,
                                              &num_connections));
  ASSERT_EQ(0, num_connections);

  EXPECT_CALL(ceee_module_utils_, GetOptionToolbandForceReposition())
      .WillOnce(Return(true));

  ASSERT_HRESULT_SUCCEEDED(tool_band_with_site_->SetSite(site_keeper_));

  // Check that the we have not set the connection if not strictly required.
  ASSERT_HRESULT_SUCCEEDED(GetConnectionCount(browser_keeper_,
                                              DIID_DWebBrowserEvents2,
                                              &num_connections));
  ASSERT_EQ(1, num_connections);

  EXPECT_CALL(ceee_module_utils_,
              SetOptionToolbandForceReposition(false)).Times(1);

  // First navigation triggers (single) registry check and unadivising.
  // After that things stay quiet.
  browser_->FireOnNavigateComplete(browser_, &CComVariant(kUrl1));

  ASSERT_HRESULT_SUCCEEDED(GetConnectionCount(browser_keeper_,
                                              DIID_DWebBrowserEvents2,
                                              &num_connections));
  ASSERT_EQ(0, num_connections);

  browser_->FireOnNavigateComplete(browser_, &CComVariant(kUrl1));

  ASSERT_HRESULT_SUCCEEDED(tool_band_with_site_->SetSite(NULL));
}

TEST_F(ToolBandTest, NormalRunDoesntTriggerLineBreak) {
  CreateSite();
  CreateBrowser();

  // Expected sequence of actions:
  // 1) initialization will trigger registry check
  // 2) invocations if GetBandInfo do not trigger registry check
  // 3) since spoofed registry says 'do not reposition', there should be no
  //    DBIMF_BREAK flag set in the structure.
  EXPECT_CALL(ceee_module_utils_, GetOptionToolbandForceReposition())
      .WillOnce(Return(false));

  ASSERT_HRESULT_SUCCEEDED(tool_band_with_site_->SetSite(site_keeper_));

  DESKBANDINFO dinfo_for_test;
  PrepareDeskBandInfo(&dinfo_for_test);

  ASSERT_HRESULT_SUCCEEDED(tool_band_->GetBandInfo(42, DBIF_VIEWMODE_NORMAL,
                                                   &dinfo_for_test));

  ASSERT_FALSE(dinfo_for_test.dwModeFlags & DBIMF_BREAK);

  // Take another pass and result should be the same.
  PrepareDeskBandInfo(&dinfo_for_test);

  ASSERT_HRESULT_SUCCEEDED(tool_band_->GetBandInfo(42, DBIF_VIEWMODE_NORMAL,
                                                   &dinfo_for_test));

  ASSERT_FALSE(dinfo_for_test.dwModeFlags & DBIMF_BREAK);

  ASSERT_HRESULT_SUCCEEDED(tool_band_with_site_->SetSite(NULL));
}

TEST_F(ToolBandTest, NewInstallationTriggersLineBreak) {
  CreateSite();
  CreateBrowser();

  EXPECT_CALL(ceee_module_utils_, GetOptionToolbandForceReposition())
      .WillOnce(Return(true));

  ASSERT_HRESULT_SUCCEEDED(tool_band_with_site_->SetSite(site_keeper_));

  DESKBANDINFO dinfo_for_test;

  // Expected sequence of actions:
  // 1) invocation of 'GetBandInfo' will trigger registry check.
  // 2) subsequent invocations do not trigger registry check, but the answer
  //    should also be 'line break' until navigation is completed;
  //    navigation completed is emulated by a call to FireOnNavigateComplete
  // after that, the break flag is not returned.

  PrepareDeskBandInfo(&dinfo_for_test);
  ASSERT_HRESULT_SUCCEEDED(tool_band_->GetBandInfo(42, DBIF_VIEWMODE_NORMAL,
                                               &dinfo_for_test));

  EXPECT_CALL(ceee_module_utils_,
              SetOptionToolbandForceReposition(false)).Times(1);

  ASSERT_TRUE(dinfo_for_test.dwModeFlags & DBIMF_BREAK);

  browser_->FireOnNavigateComplete(browser_, &CComVariant(kUrl1));

  PrepareDeskBandInfo(&dinfo_for_test);
  ASSERT_HRESULT_SUCCEEDED(tool_band_->GetBandInfo(42, DBIF_VIEWMODE_NORMAL,
                                                   &dinfo_for_test));
  ASSERT_FALSE(dinfo_for_test.dwModeFlags & DBIMF_BREAK);

  ASSERT_HRESULT_SUCCEEDED(tool_band_with_site_->SetSite(NULL));
}

}  // namespace
