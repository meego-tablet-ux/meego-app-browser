// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_TEST_EXTENSION_LOADER_H_
#define CHROME_BROWSER_EXTENSIONS_TEST_EXTENSION_LOADER_H_

#include "chrome/common/notification_observer.h"
#include "chrome/common/notification_registrar.h"

class Extension;
class FilePath;
class Profile;

class TestExtensionLoader : public NotificationObserver {
 public:
  explicit TestExtensionLoader(Profile* profile);

  // Tells the extension service to load the extension at the given path.  It
  // waits for the extension with the expected ID to load, then returns a
  // handle to it.
  Extension* Load(const char* extension_id, const FilePath& path);

  // Same as above, but installs from a CRX first.
  Extension* Install(const char* extension_id, const FilePath& path);

  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

 private:
  Profile* profile_;
  Extension* extension_;
  NotificationRegistrar registrar_;
  std::string loading_extension_id_;

  DISALLOW_COPY_AND_ASSIGN(TestExtensionLoader);
};

#endif  // CHROME_BROWSER_EXTENSIONS_TEST_EXTENSION_LOADER_H_
