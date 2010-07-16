// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DOWNLOAD_DOWNLOAD_FILE_H_
#define CHROME_BROWSER_DOWNLOAD_DOWNLOAD_FILE_H_

#include <map>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/file_path.h"
#include "base/hash_tables.h"
#include "base/linked_ptr.h"
#include "chrome/browser/download/download_types.h"
#include "chrome/browser/power_save_blocker.h"
#include "googleurl/src/gurl.h"

struct DownloadCreateInfo;

// These objects live exclusively on the download thread and handle the writing
// operations for one download. These objects live only for the duration that
// the download is 'in progress': once the download has been completed or
// cancelled, the DownloadFile is destroyed.
class DownloadFile {
 public:
  explicit DownloadFile(const DownloadCreateInfo* info);
  ~DownloadFile();

  bool Initialize();

  // Write a new chunk of data to the file. Returns true on success.
  bool AppendDataToFile(const char* data, int data_len);

  // Abort the download and automatically close the file.
  void Cancel();

  // Rename the download file. Returns 'true' if the rename was successful.
  bool Rename(const FilePath& full_path);

  // Informs the OS that this file came from the internet.
  void AnnotateWithSourceInformation();

  // Accessors.
  int64 bytes_so_far() const { return bytes_so_far_; }
  int id() const { return id_; }
  FilePath full_path() const { return full_path_; }
  int child_id() const { return child_id_; }
  int render_view_id() const { return render_view_id_; }
  int request_id() const { return request_id_; }
  bool path_renamed() const { return path_renamed_; }
  bool in_progress() const { return file_stream_ != NULL; }
  void set_in_progress(bool in_progress) { in_progress_ = in_progress; }

 private:
  // Open or Close the OS file stream. The stream is opened in the constructor
  // based on creation information passed to it, and automatically closed in
  // the destructor.
  void Close();
  bool Open();

  // OS file stream for writing
  linked_ptr<net::FileStream> file_stream_;

  // Source URL for the file being downloaded.
  GURL source_url_;

  // The URL where the download was initiated.
  GURL referrer_url_;

  // The unique identifier for this download, assigned at creation by
  // the DownloadFileManager for its internal record keeping.
  int id_;

  // IDs for looking up the tab we are associated with.
  int child_id_;
  int render_view_id_;

  // Handle for informing the ResourceDispatcherHost of a UI based cancel.
  int request_id_;

  // Amount of data received up to this point. We may not know in advance how
  // much data to expect since some servers don't provide that information.
  int64 bytes_so_far_;

  // Full path to the downloaded file.
  FilePath full_path_;

  // Whether the download is still using its initial temporary path.
  bool path_renamed_;

  // Whether the download is still receiving data.
  bool in_progress_;

  // RAII handle to keep the system from sleeping while we're downloading.
  PowerSaveBlocker dont_sleep_;

  // The provider used to save the download data.
  DownloadSaveInfo save_info_;

  DISALLOW_COPY_AND_ASSIGN(DownloadFile);
};

#endif  // CHROME_BROWSER_DOWNLOAD_DOWNLOAD_FILE_H_
