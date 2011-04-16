// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_file_browser_private_api.h"

#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/task.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/extension.h"
#include "content/browser/browser_thread.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "googleurl/src/gurl.h"
#include "grit/generated_resources.h"
#include "webkit/fileapi/file_system_context.h"
#include "webkit/fileapi/file_system_operation.h"
#include "webkit/fileapi/file_system_path_manager.h"
#include "webkit/fileapi/file_system_types.h"
#include "ui/base/l10n/l10n_util.h"


class LocalFileSystemCallbackDispatcher
    : public fileapi::FileSystemCallbackDispatcher {
 public:
  explicit LocalFileSystemCallbackDispatcher(
      RequestLocalFileSystemFunction* function) : function_(function) {
    DCHECK(function_);
  }
  // fileapi::FileSystemCallbackDispatcher overrides.
  virtual void DidSucceed() OVERRIDE {
    NOTREACHED();
  }
  virtual void DidReadMetadata(const base::PlatformFileInfo& info,
                               const FilePath& unused) OVERRIDE {
    NOTREACHED();
  }
  virtual void DidReadDirectory(
      const std::vector<base::FileUtilProxy::Entry>& entries,
      bool has_more) OVERRIDE {
    NOTREACHED();
  }
  virtual void DidWrite(int64 bytes, bool complete) OVERRIDE {
    NOTREACHED();
  }
  virtual void DidOpenFileSystem(const std::string& name,
                                 const GURL& root) OVERRIDE {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        NewRunnableMethod(function_,
            &RequestLocalFileSystemFunction::RespondSuccessOnUIThread,
            name,
            root));
  }
  virtual void DidFail(base::PlatformFileError error_code) OVERRIDE {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        NewRunnableMethod(function_,
            &RequestLocalFileSystemFunction::RespondFailedOnUIThread,
            error_code));
  }
 private:
  RequestLocalFileSystemFunction* function_;
  DISALLOW_COPY_AND_ASSIGN(LocalFileSystemCallbackDispatcher);
};

RequestLocalFileSystemFunction::RequestLocalFileSystemFunction() {
}

RequestLocalFileSystemFunction::~RequestLocalFileSystemFunction() {
}

bool RequestLocalFileSystemFunction::RunImpl() {
  fileapi::FileSystemOperation* operation =
      new fileapi::FileSystemOperation(
          new LocalFileSystemCallbackDispatcher(this),
          BrowserThread::GetMessageLoopProxyForThread(BrowserThread::FILE),
          profile()->GetFileSystemContext(),
          NULL);
  GURL origin_url = source_url().GetOrigin();
  operation->OpenFileSystem(origin_url, fileapi::kFileSystemTypeExternal,
                            false);     // create
  // Will finish asynchronously.
  return true;
}

void RequestLocalFileSystemFunction::RespondSuccessOnUIThread(
    const std::string& name, const GURL& root) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  result_.reset(new DictionaryValue());
  DictionaryValue* dict = reinterpret_cast<DictionaryValue*>(result_.get());
  dict->SetString("name", name);
  dict->SetString("path", root.spec());
  dict->SetInteger("error", base::PLATFORM_FILE_OK);
  SendResponse(true);
}

void RequestLocalFileSystemFunction::RespondFailedOnUIThread(
    base::PlatformFileError error_code) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  result_.reset(new DictionaryValue());
  DictionaryValue* dict = reinterpret_cast<DictionaryValue*>(result_.get());
  dict->SetInteger("error", static_cast<int>(error_code));
  SendResponse(true);
}

FileDialogFunction::FileDialogFunction() {
}

FileDialogFunction::~FileDialogFunction() {
}

// static
FileDialogFunction::Callback
FileDialogFunction::Callback::null_(Callback(NULL, NULL));

// static
FileDialogFunction::Callback::Map FileDialogFunction::Callback::map_;

// static
void FileDialogFunction::Callback::Add(int32 tab_id,
                                     SelectFileDialog::Listener* listener,
                                     void* params) {
  if (map_.find(tab_id) == map_.end()) {
    map_.insert(std::make_pair(tab_id, Callback(listener, params)));
  } else {
    DLOG_ASSERT("FileDialogFunction::AddCallback tab_id already present");
  }
}

// static
void FileDialogFunction::Callback::Remove(int32 tab_id) {
  map_.erase(tab_id);
}

// static
const FileDialogFunction::Callback&
FileDialogFunction::Callback::Find(int32 tab_id) {
  Callback::Map::const_iterator it = map_.find(tab_id);
  return (it == map_.end()) ? null_ : it->second;
}


int32 FileDialogFunction::GetTabId() const {
  return dispatcher()->delegate()->associated_tab_contents()->
    controller().session_id().id();
}

const FileDialogFunction::Callback& FileDialogFunction::GetCallback() const {
  return Callback::Find(GetTabId());
}

// GetFileSystemRootPathOnFileThread can only be called from the file thread,
// so here we are. This function takes a vector of virtual paths, converts
// them to local paths and calls GetLocalPathsResponseOnUIThread with the
// result vector, on the UI thread.
void FileDialogFunction::GetLocalPathsOnFileThread() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
  DCHECK(selected_files_.empty());

  // FilePath(virtual_path) doesn't work on win, so limit this to ChromeOS.
#if defined(OS_CHROMEOS)
  GURL origin_url = source_url().GetOrigin();
  fileapi::FileSystemPathManager* path_manager =
      profile()->GetFileSystemContext()->path_manager();

  size_t len = virtual_paths_.size();
  selected_files_.reserve(len);
  for (size_t i = 0; i < len; ++i) {
    std::string virtual_path = virtual_paths_[i];
    FilePath root = path_manager->GetFileSystemRootPathOnFileThread(
        origin_url,
        fileapi::kFileSystemTypeExternal,
        FilePath(virtual_path),
        false);
    if (!root.empty()) {
      selected_files_.push_back(root.Append(virtual_path));
    } else {
      LOG(WARNING) << "GetLocalPathsOnFileThread failed " << virtual_path;
    }
  }
#endif

  if (!selected_files_.empty()) {
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        NewRunnableMethod(this,
            &FileDialogFunction::GetLocalPathsResponseOnUIThread));
  }
}

bool SelectFileFunction::RunImpl() {
  DCHECK_EQ(static_cast<size_t>(2), args_->GetSize());

  std::string virtual_path;
  args_->GetString(0, &virtual_path);
  virtual_paths_.push_back(virtual_path);

  BrowserThread::PostTask(
      BrowserThread::FILE, FROM_HERE,
      NewRunnableMethod(this,
          &SelectFileFunction::GetLocalPathsOnFileThread));

  return true;
}

void SelectFileFunction::GetLocalPathsResponseOnUIThread() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  DCHECK_EQ(selected_files_.size(), static_cast<size_t>(1));

  int index;
  args_->GetInteger(1, &index);
  const Callback& callback = GetCallback();
  DCHECK(!callback.IsNull());
  if (!callback.IsNull()) {
    callback.listener()->FileSelected(selected_files_[0],
                                      index,
                                      callback.params());
  }
}

SelectFilesFunction::SelectFilesFunction() {
}

SelectFilesFunction::~SelectFilesFunction() {
}

bool SelectFilesFunction::RunImpl() {
  DCHECK_EQ(static_cast<size_t>(1), args_->GetSize());

  ListValue* path_list = NULL;
  args_->GetList(0, &path_list);
  DCHECK(path_list);

  std::string virtual_path;
  size_t len = path_list->GetSize();
  virtual_paths_.reserve(len);
  for (size_t i = 0; i < len; ++i) {
    path_list->GetString(i, &virtual_path);
    virtual_paths_.push_back(virtual_path);
  }

  BrowserThread::PostTask(
      BrowserThread::FILE, FROM_HERE,
      NewRunnableMethod(this,
          &SelectFilesFunction::GetLocalPathsOnFileThread));

  return true;
}

void SelectFilesFunction::GetLocalPathsResponseOnUIThread() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));

  const Callback& callback = GetCallback();
  DCHECK(!callback.IsNull());
  if (!callback.IsNull()) {
    callback.listener()->MultiFilesSelected(selected_files_, callback.params());
  }
}

bool CancelFileDialogFunction::RunImpl() {
  const Callback& callback = GetCallback();
  DCHECK(!callback.IsNull());
  if (!callback.IsNull()) {
    callback.listener()->FileSelectionCanceled(callback.params());
  }

  return true;
}

bool FileDialogStringsFunction::RunImpl() {
  result_.reset(new DictionaryValue());
  DictionaryValue* dict = reinterpret_cast<DictionaryValue*>(result_.get());

#define SET_STRING(ns, id) \
  dict->SetString(#id, l10n_util::GetStringUTF16(ns##_##id))

  SET_STRING(IDS, LOCALE_FMT_DATE_SHORT);
  SET_STRING(IDS, LOCALE_MONTHS_SHORT);
  SET_STRING(IDS, LOCALE_DAYS_SHORT);

  SET_STRING(IDS_FILE_BROWSER, BODY_FONT_FAMILY);
  SET_STRING(IDS_FILE_BROWSER, BODY_FONT_SIZE);

  SET_STRING(IDS_FILE_BROWSER, ROOT_DIRECTORY_LABEL);
  SET_STRING(IDS_FILE_BROWSER, DOWNLOADS_DIRECTORY_LABEL);
  SET_STRING(IDS_FILE_BROWSER, MEDIA_DIRECTORY_LABEL);
  SET_STRING(IDS_FILE_BROWSER, NAME_COLUMN_LABEL);
  SET_STRING(IDS_FILE_BROWSER, SIZE_COLUMN_LABEL);
  SET_STRING(IDS_FILE_BROWSER, DATE_COLUMN_LABEL);
  SET_STRING(IDS_FILE_BROWSER, PREVIEW_COLUMN_LABEL);

  SET_STRING(IDS_FILE_BROWSER, ERROR_CREATING_FOLDER);
  SET_STRING(IDS_FILE_BROWSER, ERROR_INVALID_FOLDER_CHARACTER);
  SET_STRING(IDS_FILE_BROWSER, NEW_FOLDER_PROMPT);
  SET_STRING(IDS_FILE_BROWSER, NEW_FOLDER_BUTTON_LABEL);
  SET_STRING(IDS_FILE_BROWSER, FILENAME_LABEL);

  SET_STRING(IDS_FILE_BROWSER, CANCEL_LABEL);
  SET_STRING(IDS_FILE_BROWSER, OPEN_LABEL);
  SET_STRING(IDS_FILE_BROWSER, SAVE_LABEL);

  SET_STRING(IDS_FILE_BROWSER, SELECT_FOLDER_TITLE);
  SET_STRING(IDS_FILE_BROWSER, SELECT_OPEN_FILE_TITLE);
  SET_STRING(IDS_FILE_BROWSER, SELECT_OPEN_MULTI_FILE_TITLE);
  SET_STRING(IDS_FILE_BROWSER, SELECT_SAVEAS_FILE_TITLE);

  SET_STRING(IDS_FILE_BROWSER, COMPUTING_SELECTION);
  SET_STRING(IDS_FILE_BROWSER, NOTHING_SELECTED);
  SET_STRING(IDS_FILE_BROWSER, ONE_FILE_SELECTED);
  SET_STRING(IDS_FILE_BROWSER, MANY_FILES_SELECTED);

#undef SET_STRING

  SendResponse(true);
  return true;
}
