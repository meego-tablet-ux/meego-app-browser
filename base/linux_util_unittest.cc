// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/linux_util.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "base/env_var.h"

using ::testing::_;
using ::testing::Return;
using ::testing::SetArgumentPointee;
using ::testing::StrEq;

class MockEnvVarGetter : public base::EnvVarGetter {
 public:
  MOCK_METHOD2(GetEnv, bool(const char*, std::string* result));
};

namespace {

const char* kGnome = "gnome";
const char* kKDE4 = "kde4";
const char* kKDE = "kde";
const char* kXFCE = "xfce";

}  // namespace

TEST(LinuxUtilTest, GetDesktopEnvironmentGnome) {
  MockEnvVarGetter getter;
  EXPECT_CALL(getter, GetEnv(_, _)).WillRepeatedly(Return(false));
  EXPECT_CALL(getter, GetEnv(StrEq("DESKTOP_SESSION"), _))
      .WillOnce(DoAll(SetArgumentPointee<1>(kGnome), Return(true)));

  EXPECT_EQ(base::DESKTOP_ENVIRONMENT_GNOME,
            base::GetDesktopEnvironment(&getter));
}

TEST(LinuxUtilTest, GetDesktopEnvironmentKDE4) {
  MockEnvVarGetter getter;
  EXPECT_CALL(getter, GetEnv(_, _)).WillRepeatedly(Return(false));
  EXPECT_CALL(getter, GetEnv(StrEq("DESKTOP_SESSION"), _))
      .WillOnce(DoAll(SetArgumentPointee<1>(kKDE4), Return(true)));

  EXPECT_EQ(base::DESKTOP_ENVIRONMENT_KDE4,
            base::GetDesktopEnvironment(&getter));
}

TEST(LinuxUtilTest, GetDesktopEnvironmentKDE3) {
  MockEnvVarGetter getter;
  EXPECT_CALL(getter, GetEnv(_, _)).WillRepeatedly(Return(false));
  EXPECT_CALL(getter, GetEnv(StrEq("DESKTOP_SESSION"), _))
      .WillOnce(DoAll(SetArgumentPointee<1>(kKDE), Return(true)));

  EXPECT_EQ(base::DESKTOP_ENVIRONMENT_KDE3,
            base::GetDesktopEnvironment(&getter));
}

TEST(LinuxUtilTest, GetDesktopEnvironmentXFCE) {
  MockEnvVarGetter getter;
  EXPECT_CALL(getter, GetEnv(_, _)).WillRepeatedly(Return(false));
  EXPECT_CALL(getter, GetEnv(StrEq("DESKTOP_SESSION"), _))
      .WillOnce(DoAll(SetArgumentPointee<1>(kXFCE), Return(true)));

  EXPECT_EQ(base::DESKTOP_ENVIRONMENT_XFCE,
            base::GetDesktopEnvironment(&getter));
}
