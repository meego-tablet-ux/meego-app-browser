// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_FILE_BROWSER_PRIVATE_API_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_FILE_BROWSER_PRIVATE_API_H_
#pragma once

#include <map>
#include <string>
#include <vector>

#include "base/platform_file.h"
#include "chrome/browser/extensions/extension_function.h"
#include "chrome/browser/ui/shell_dialogs.h"
#include "googleurl/src/url_util.h"
#include "webkit/fileapi/file_system_callback_dispatcher.h"

class GURL;

// Implements the chrome.fileBrowserPrivate.requestLocalFileSystem method.
class RequestLocalFileSystemFunction : public AsyncExtensionFunction {
 protected:
  friend class LocalFileSystemCallbackDispatcher;
  // AsyncExtensionFunction overrides.
  virtual bool RunImpl() OVERRIDE;
  void RespondSuccessOnUIThread(const std::string& name,
                                const GURL& root_path);
  void RespondFailedOnUIThread(base::PlatformFileError error_code);
  void RequestOnFileThread(const GURL& source_url);
  DECLARE_EXTENSION_FUNCTION_NAME("fileBrowserPrivate.requestLocalFileSystem");
};

// Implements the chrome.fileBrowserPrivate.getFileTasks method.
class GetFileTasksFileBrowserFunction : public AsyncExtensionFunction {
 protected:
  // AsyncExtensionFunction overrides.
  virtual bool RunImpl() OVERRIDE;

 private:
  DECLARE_EXTENSION_FUNCTION_NAME("fileBrowserPrivate.getFileTasks");
};


// Implements the chrome.fileBrowserPrivate.executeTask method.
class ExecuteTasksFileBrowserFunction : public AsyncExtensionFunction {
 protected:
  // AsyncExtensionFunction overrides.
  virtual bool RunImpl() OVERRIDE;

 private:
  struct FileDefinition {
    GURL target_file_url;
    FilePath virtual_path;
    bool is_directory;
  };
  typedef std::vector<FileDefinition> FileDefinitionList;
  friend class ExecuteTasksFileSystemCallbackDispatcher;
  // Initates execution of context menu tasks identified with |task_id| for
  // each element of |files_list|.
  bool InitiateFileTaskExecution(const std::string& task_id,
                                 ListValue* files_list);
  void RequestFileEntryOnFileThread(const GURL& source_url,
                                    const std::string& task_id,
                                    const std::vector<GURL>& file_urls);
  void RespondFailedOnUIThread(base::PlatformFileError error_code);
  void ExecuteFileActionsOnUIThread(const std::string& task_id,
                                    const std::string& file_system_name,
                                    const GURL& file_system_root,
                                    const FileDefinitionList& file_list);
  DECLARE_EXTENSION_FUNCTION_NAME("fileBrowserPrivate.executeTask");
};

// Parent class for the chromium extension APIs for the file dialog.
class FileDialogFunction
    : public AsyncExtensionFunction {
 public:
  FileDialogFunction();

  // Register/unregister callbacks.
  // When file selection events occur in a tab, we'll call back the
  // appropriate listener with the right params.
  class Callback {
   public:
    Callback(SelectFileDialog::Listener* l, void* p)
        : listener_(l),
          params_(p) {
    }
    SelectFileDialog::Listener* listener() const { return listener_; }
    void* params() const { return params_; }
    bool IsNull() const { return listener_ == NULL; }

    static void Add(int32 tab_id,
                    SelectFileDialog::Listener* listener,
                    void* params);
    static void Remove(int32 tab_id);
    static const Callback& Find(int32 tab_id);

   private:
    static const Callback& null() { return null_; }

    SelectFileDialog::Listener* listener_;
    void* params_;

    // statics.
    typedef std::map<int32, Callback> Map;
    static Map map_;
    static Callback null_;
  };

 protected:
  typedef std::vector<GURL> UrlList;
  typedef std::vector<FilePath> FilePathList;

  virtual ~FileDialogFunction();

  // Convert virtual paths to local paths on the file thread.
  void GetLocalPathsOnFileThread(const UrlList& file_urls);

  // Callback with converted local paths.
  virtual void GetLocalPathsResponseOnUIThread(const FilePathList& files) {}

  // Get the callback for the hosting tab.
  const Callback& GetCallback() const;

 private:
  // Figure out the tab_id of the hosting tab.
  int32 GetTabId() const;
};

// Select a single file.
class SelectFileFunction
    : public FileDialogFunction {
 public:
  SelectFileFunction() {}

 protected:
  virtual ~SelectFileFunction() {}

  // AsyncExtensionFunction overrides.
  virtual bool RunImpl() OVERRIDE;

  // FileDialogFunction overrides.
  virtual void GetLocalPathsResponseOnUIThread(
      const FilePathList& files) OVERRIDE;

 private:
  DECLARE_EXTENSION_FUNCTION_NAME("fileBrowserPrivate.selectFile");
};

// Views multiple selected file.
class ViewFilesFunction
    : public FileDialogFunction {
 public:
  ViewFilesFunction();

 protected:
  virtual ~ViewFilesFunction();

  // AsyncExtensionFunction overrides.
  virtual bool RunImpl() OVERRIDE;

  // FileDialogFunction overrides.
  virtual void GetLocalPathsResponseOnUIThread(
      const FilePathList& files) OVERRIDE;

 private:
  DECLARE_EXTENSION_FUNCTION_NAME("fileBrowserPrivate.viewFiles");
};

// Select multiple files.
class SelectFilesFunction
    : public FileDialogFunction {
 public:
  SelectFilesFunction();

 protected:
  virtual ~SelectFilesFunction();

  // AsyncExtensionFunction overrides.
  virtual bool RunImpl() OVERRIDE;

  // FileDialogFunction overrides.
  virtual void GetLocalPathsResponseOnUIThread(
      const FilePathList& files) OVERRIDE;

 private:
  DECLARE_EXTENSION_FUNCTION_NAME("fileBrowserPrivate.selectFiles");
};

// Cancel file selection Dialog.
class CancelFileDialogFunction
    : public FileDialogFunction {
 public:
  CancelFileDialogFunction() {}

 protected:
  virtual ~CancelFileDialogFunction() {}

  // AsyncExtensionFunction overrides.
  virtual bool RunImpl() OVERRIDE;

 private:
  DECLARE_EXTENSION_FUNCTION_NAME("fileBrowserPrivate.cancelDialog");
};

// File Dialog Strings.
class FileDialogStringsFunction
    : public FileDialogFunction {
 public:
  FileDialogStringsFunction() {}

 protected:
  virtual ~FileDialogStringsFunction() {}

  // AsyncExtensionFunction overrides.
  virtual bool RunImpl() OVERRIDE;

 private:
  DECLARE_EXTENSION_FUNCTION_NAME("fileBrowserPrivate.getStrings");
};

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_FILE_BROWSER_PRIVATE_API_H_
