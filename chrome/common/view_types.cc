// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/view_types.h"

const char* ViewType::kTabContents = "TAB";
const char* ViewType::kToolstrip = "TOOLSTRIP";
const char* ViewType::kMole = "MOLE";
const char* ViewType::kBackgroundPage = "BACKGROUND";
const char* ViewType::kPopup = "POPUP";
const char* ViewType::kAll = "ALL";

bool ViewType::ShouldAutoResize(ViewType::Type view_type) {
  return (view_type == EXTENSION_TOOLSTRIP ||
          view_type == EXTENSION_MOLE ||
          view_type == EXTENSION_POPUP);
}
