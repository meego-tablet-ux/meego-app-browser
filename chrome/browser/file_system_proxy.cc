// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/file_system_proxy.h"

#include "base/file_util.h"
#include "chrome/browser/chrome_thread_relay.h"

namespace {

class RelayCreateOrOpen : public ChromeThreadRelay {
 public:
  RelayCreateOrOpen(
      const FilePath& file_path,
      int file_flags,
      FileSystemProxy::CreateOrOpenCallback* callback)
      : file_path_(file_path),
        file_flags_(file_flags),
        callback_(callback),
        file_handle_(base::kInvalidPlatformFileValue),
        created_(false) {
    DCHECK(callback);
  }

 protected:
  virtual ~RelayCreateOrOpen() {
    if (file_handle_ != base::kInvalidPlatformFileValue)
      FileSystemProxy::Close(file_handle_, NULL);
  }

  virtual void RunWork() {
    file_handle_ = base::CreatePlatformFile(file_path_, file_flags_, &created_);
  }

  virtual void RunCallback() {
    callback_->Run(base::PassPlatformFile(&file_handle_), created_);
    delete callback_;
  }

 private:
  FilePath file_path_;
  int file_flags_;
  FileSystemProxy::CreateOrOpenCallback* callback_;
  base::PlatformFile file_handle_;
  bool created_;
};

class RelayCreateTemporary : public ChromeThreadRelay {
 public:
  explicit RelayCreateTemporary(
      FileSystemProxy::CreateTemporaryCallback* callback)
      : callback_(callback),
        file_handle_(base::kInvalidPlatformFileValue) {
    DCHECK(callback);
  }

 protected:
  virtual ~RelayCreateTemporary() {
    if (file_handle_ != base::kInvalidPlatformFileValue)
      FileSystemProxy::Close(file_handle_, NULL);
  }

  virtual void RunWork() {
    // TODO(darin): file_util should have a variant of CreateTemporaryFile
    // that returns a FilePath and a PlatformFile.
    file_util::CreateTemporaryFile(&file_path_);

    // Use a fixed set of flags that are appropriate for writing to a temporary
    // file from the IO thread using a net::FileStream.
    int file_flags =
        base::PLATFORM_FILE_CREATE_ALWAYS |
        base::PLATFORM_FILE_WRITE |
        base::PLATFORM_FILE_ASYNC |
        base::PLATFORM_FILE_TEMPORARY;
    file_handle_ = base::CreatePlatformFile(file_path_, file_flags, NULL);
  }

  virtual void RunCallback() {
    callback_->Run(base::PassPlatformFile(&file_handle_), file_path_);
    delete callback_;
  }

 private:
  FileSystemProxy::CreateTemporaryCallback* callback_;
  base::PlatformFile file_handle_;
  FilePath file_path_;
};

class RelayWithStatusCallback : public ChromeThreadRelay {
 public:
  explicit RelayWithStatusCallback(FileSystemProxy::StatusCallback* callback)
      : callback_(callback),
        succeeded_(false) {
    // It is OK for callback to be NULL.
  }

 protected:
  virtual void RunCallback() {
    // The caller may not have been interested in the result.
    if (callback_) {
      callback_->Run(succeeded_);
      delete callback_;
    }
  }

  void SetStatus(bool succeeded) { succeeded_ = succeeded; }

 private:
  FileSystemProxy::StatusCallback* callback_;
  bool succeeded_;
};

class RelayClose : public RelayWithStatusCallback {
 public:
  RelayClose(base::PlatformFile file_handle,
             FileSystemProxy::StatusCallback* callback)
      : RelayWithStatusCallback(callback),
        file_handle_(file_handle) {
  }

 protected:
  virtual void RunWork() {
    SetStatus(base::ClosePlatformFile(file_handle_));
  }

 private:
  base::PlatformFile file_handle_;
};

class RelayDelete : public RelayWithStatusCallback {
 public:
  RelayDelete(const FilePath& file_path,
              bool recursive,
              FileSystemProxy::StatusCallback* callback)
      : RelayWithStatusCallback(callback),
        file_path_(file_path),
        recursive_(recursive) {
  }

 protected:
  virtual void RunWork() {
    SetStatus(file_util::Delete(file_path_, recursive_));
  }

 private:
  FilePath file_path_;
  bool recursive_;
};

void Start(const tracked_objects::Location& from_here,
           scoped_refptr<ChromeThreadRelay> relay) {
  relay->Start(ChromeThread::FILE, from_here);
}

}  // namespace

void FileSystemProxy::CreateOrOpen(const FilePath& file_path, int file_flags,
                                   CreateOrOpenCallback* callback) {
  Start(FROM_HERE, new RelayCreateOrOpen(file_path, file_flags, callback));
}

void FileSystemProxy::CreateTemporary(CreateTemporaryCallback* callback) {
  Start(FROM_HERE, new RelayCreateTemporary(callback));
}

void FileSystemProxy::Close(base::PlatformFile file_handle,
                            StatusCallback* callback) {
  Start(FROM_HERE, new RelayClose(file_handle, callback));
}

void FileSystemProxy::Delete(const FilePath& file_path,
                             StatusCallback* callback) {
  Start(FROM_HERE, new RelayDelete(file_path, false, callback));
}

void FileSystemProxy::RecursiveDelete(const FilePath& file_path,
                                      StatusCallback* callback) {
  Start(FROM_HERE, new RelayDelete(file_path, true, callback));
}
