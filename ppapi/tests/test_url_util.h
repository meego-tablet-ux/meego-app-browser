// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_TESTS_TEST_URL_UTIL_H_
#define PPAPI_TESTS_TEST_URL_UTIL_H_

#include "ppapi/cpp/dev/url_util_dev.h"
#include "ppapi/tests/test_case.h"

class TestURLUtil : public TestCase {
 public:
  TestURLUtil(TestingInstance* instance) : TestCase(instance), util_(NULL) {}

  // TestCase implementation.
  virtual bool Init();
  virtual void RunTest();

 private:
  std::string TestCanonicalize();
  std::string TestResolveRelative();
  std::string TestIsSameSecurityOrigin();
  std::string TestDocumentCanRequest();
  std::string TestDocumentCanAccessDocument();
  std::string TestGetDocumentURL();
  std::string TestGetPluginInstanceURL();

  const pp::URLUtil_Dev* util_;
};

#endif  // PPAPI_TESTS_TEST_URL_UTIL_H_
