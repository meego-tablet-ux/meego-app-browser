// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_DLDM_API_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_DLDM_API_H_
#pragma once

#include <map>
#include <string>

#include "base/singleton.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/download/download_item.h"
#include "chrome/browser/download/download_manager.h"
#include "chrome/browser/extensions/extension_function.h"
//#include "content/common/notification_registrar.h"

// Observes History service and routes the notifications as events to the
// extension system.
class ExtensionDldmEventRouter : public DownloadManager::Observer,
  public DownloadItem::Observer {
 public:
  // Single instance of the event router.
  static ExtensionDldmEventRouter* GetInstance();

  // Safe to call multiple times.
  void ObserveProfile(Profile* profile) {
    profile_ = profile;
    download_manager_ = profile->GetDownloadManager();
    download_manager_->AddObserver(this);
  }

  DownloadManager* download_manager() { return download_manager_; }
 private:
  friend struct DefaultSingletonTraits<ExtensionDldmEventRouter>;

  ExtensionDldmEventRouter() {}
  virtual ~ExtensionDldmEventRouter() {}

 public:

  // DownloadItem::Observer interface
  virtual void OnDownloadUpdated(DownloadItem* download) {}
  virtual void OnDownloadFileCompleted(DownloadItem* download);
  virtual void OnDownloadOpened(DownloadItem* download) { }

  // DownloadManager::Observer interface
  virtual void ModelChanged();

 private:
  std::vector<DownloadItem*> download_items_;
  Profile* profile_;
  DownloadManager* download_manager_;
  void DldmFinished(
		    Profile* profile,
		    DownloadItem* download);
  void DispatchEvent(Profile* profile,
                     const char* event_name,
                     const std::string& json_args);

  DISALLOW_COPY_AND_ASSIGN(ExtensionDldmEventRouter);
};


// Base class for dldm function APIs.
class DldmFunction : public AsyncExtensionFunction {
 public:
  virtual void Run();
  virtual bool RunImpl() = 0;

};

class UpdateUIDldmFunction : public DldmFunction {
 public:
  virtual bool RunImpl();
  DECLARE_EXTENSION_FUNCTION_NAME("dldm.updateUI");
};

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_DLDM_API_H_
