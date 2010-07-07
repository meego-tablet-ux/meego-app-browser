// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//
// PersonalOptions class
// Encapsulated handling of personal options page.
//
function PersonalOptions(model) {
  OptionsPage.call(this, 'personal', templateData.personalPage, 'personalPage');
}

PersonalOptions.getInstance = function() {
  if (!PersonalOptions.instance_) {
    PersonalOptions.instance_ = new PersonalOptions(null);
  }
  return PersonalOptions.instance_;
}

PersonalOptions.prototype = {
  // Inherit PersonalOptions from OptionsPage.
  __proto__: OptionsPage.prototype,

  // Initialize PersonalOptions page.
  initializePage: function() {
    // Call base class implementation to starts preference initialization.
    OptionsPage.prototype.initializePage.call(this);

    // TODO(csilv): add any needed initialization here or delete this method.
  },
};

