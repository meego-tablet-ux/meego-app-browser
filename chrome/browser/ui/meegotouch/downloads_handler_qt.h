// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Copyright (c) 2010, Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_QT_DOWNLOADS_QT_HANDLER_H_
#define CHROME_BROWSER_QT_DOWNLOADS_QT_HANDLER_H_
#pragma once

#include <vector>
#include <QString>
#include "chrome/browser/download/download_item.h"
#include "chrome/browser/download/download_manager.h"
class BrowserWindowQt;
class ListValue;
class DownloadViewItem;
class DownloadsQtImpl;
class FilePath;
// The handler for Javascript messages related to the "downloads" view,
// also observes changes to the download manager.
class DownloadsQtHandler : public DownloadManager::Observer,
                            public DownloadItem::Observer {
 public:
  explicit DownloadsQtHandler(BrowserWindowQt* window, DownloadManager* dlm);
  virtual ~DownloadsQtHandler();

  void Init();

  // DownloadItem::Observer interface
  virtual void OnDownloadUpdated(DownloadItem* download);
  virtual void OnDownloadFileCompleted(DownloadItem* download) { }
  virtual void OnDownloadOpened(DownloadItem* download) { }

  // DownloadManager::Observer interface
  virtual void ModelChanged();

  // Callback for the "getDownloads" message.
  void HandleGetDownloads(const QString& args);

  // Callback for the "openFile" message - opens the file in the shell.
  void HandleOpenFile(const int args);

  // Callback for the "drag" message - initiates a file object drag.
  void HandleDrag(const ListValue* args);

  // Callback for the "saveDangerous" message - specifies that the user
  // wishes to save a dangerous file.
  void HandleSaveDangerous(const int args);

  // Callback for the "discardDangerous" message - specifies that the user
  // wishes to discard (remove) a dangerous file.
  void HandleDiscardDangerous(const int args);

  // Callback for the "show" message - shows the file in explorer.
  void HandleShow(const int args);

  // Callback for the "pause" message - pauses the file download.
  void HandlePause(const int args);

  // Callback for the "remove" message - removes the file download from shelf
  // and list.
  void HandleRemove(const int args);

  // Callback for the "cancel" message - cancels the download.
  void HandleCancel(const int args);

  // Callback for the "clearAll" message - clears all the downloads.
  void HandleClearAll();

  void Show();
  void Hide();
 private:

  DownloadViewItem* CreateDownloadViewItem(DownloadItem* download);

  void UpdateCurrentDownload(DownloadItem* download, const int id);

  int FetchMimetypeIconID(const std::string& path); 

  // Send the current list of downloads to the page.
  void SendCurrentDownloads();

  // Clear all download items and their observers.
  void ClearDownloadItems();

  // Return the download that corresponds to a given id.
  DownloadItem* GetDownloadById(int id);

  // Return the download that is referred to in a given value.
  DownloadItem* GetDownloadByValue(const ListValue* args);

  bool ExtractIntegerValue(const ListValue* value, int* out_int);

  std::wstring ExtractStringValue(const ListValue* value);
  // Current search text.
  std::wstring search_text_;

  // Our model
  DownloadManager* download_manager_;

  // The current set of visible DownloadItems for this view received from the
  // DownloadManager. DownloadManager owns the DownloadItems. The vector is
  // kept in order, sorted by ascending start time.
  typedef std::vector<DownloadItem*> OrderedDownloads;
  OrderedDownloads download_items_;

  BrowserWindowQt* window_;
  DownloadsQtImpl* impl_;
  DISALLOW_COPY_AND_ASSIGN(DownloadsQtHandler);
};

#endif  // CHROME_BROWSER_QT_DOWNLOADS_QT_HANDLER_H_
