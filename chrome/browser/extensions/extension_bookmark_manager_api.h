// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_BOOKMARK_MANAGER_API_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_BOOKMARK_MANAGER_API_H_

#include "chrome/browser/bookmarks/bookmark_drag_data.h"
#include "chrome/browser/extensions/extension_bookmarks_module.h"
#include "chrome/browser/extensions/extension_function.h"
#include "chrome/browser/renderer_host/render_view_host_delegate.h"
#include "chrome/browser/shell_dialogs.h"

class BookmarkNode;
class ListValue;
class Profile;
class TabContents;

// Class that handles the chrome.experimental.bookmarkManager events.
class ExtensionBookmarkManagerEventRouter
    : public RenderViewHostDelegate::BookmarkDrag {
 public:
  ExtensionBookmarkManagerEventRouter(Profile* profile,
                                      TabContents* tab_contents);
  virtual ~ExtensionBookmarkManagerEventRouter();

  // RenderViewHostDelegate::BookmarkDrag interface
  virtual void OnDragEnter(const DragData* data);
  virtual void OnDragOver(const DragData* data);
  virtual void OnDragLeave(const DragData* data);
  virtual void OnDrop(const DragData* data);

  // The bookmark drag and drop data. This gets set after a drop was done on
  // the page. This returns NULL if no data is available.
  const BookmarkDragData* GetBookmarkDragData();

  // Clears the drag and drop data.
  void ClearBookmarkDragData();

 private:
  // Helper to actually dispatch an event to extension listeners.
  void DispatchEvent(const char* event_name, const ListValue* args);

  void DispatchDragEvent(const DragData* data, const char* event_name);

  Profile* profile_;
  TabContents* tab_contents_;
  BookmarkDragData bookmark_drag_data_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionBookmarkManagerEventRouter);
};

class ClipboardBookmarkManagerFunction : public BookmarksFunction {
 protected:
  bool CopyOrCut(bool cut);
};

class CopyBookmarkManagerFunction : public ClipboardBookmarkManagerFunction {
 public:
  // Override ClipboardBookmarkManagerFunction.
  virtual bool RunImpl();

 private:
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.bookmarkManager.copy");
};

class CutBookmarkManagerFunction : public ClipboardBookmarkManagerFunction {
 public:
  // Override ClipboardBookmarkManagerFunction.
  virtual bool RunImpl();

 private:
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.bookmarkManager.cut");
};

class PasteBookmarkManagerFunction : public BookmarksFunction {
 public:
  // Override BookmarksFunction.
  virtual bool RunImpl();

 private:
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.bookmarkManager.paste");
};

class CanPasteBookmarkManagerFunction
    : public BookmarksFunction {
 public:
  // Override BookmarksFunction.
  virtual bool RunImpl();

 private:
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.bookmarkManager.canPaste");
};

class BookmarkManagerIOFunction : public BookmarksFunction,
                                  public SelectFileDialog::Listener {
 public:
  // Overridden from SelectFileDialog::Listener:
  virtual void FileSelected(const FilePath& path, int index, void* params) = 0;
  void MultiFilesSelected(const std::vector<FilePath>& files, void* params);
  void FileSelectionCanceled(void* params);
  void SelectFile(SelectFileDialog::Type type);

 protected:
  scoped_refptr<SelectFileDialog> select_file_dialog_;
};

class ImportBookmarksFunction : public BookmarkManagerIOFunction {
 public:
  // Override BookmarkManagerIOFunction.
  bool RunImpl();
  void FileSelected(const FilePath& path, int index, void* params);

 private:
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.bookmarkManager.import");
};

class ExportBookmarksFunction : public BookmarkManagerIOFunction {
 public:
  // Override BookmarkManagerIOFunction.
  bool RunImpl();
  void FileSelected(const FilePath& path, int index, void* params);

 private:
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.bookmarkManager.export");
};

class SortChildrenBookmarkManagerFunction
    : public BookmarksFunction {
 public:
  // Override BookmarksFunction.
  virtual bool RunImpl();

 private:
  DECLARE_EXTENSION_FUNCTION_NAME(
      "experimental.bookmarkManager.sortChildren");
};

class BookmarkManagerGetStringsFunction : public AsyncExtensionFunction {
 public:
  // Override AsyncExtensionFunction.
  virtual bool RunImpl();

 private:
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.bookmarkManager.getStrings");
};

class StartDragBookmarkManagerFunction
    : public BookmarksFunction {
 public:
  // Override BookmarksFunction.
  virtual bool RunImpl();

 private:
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.bookmarkManager.startDrag");
};

class DropBookmarkManagerFunction : public BookmarksFunction {
 public:
  virtual bool RunImpl();

 private:
  DECLARE_EXTENSION_FUNCTION_NAME("experimental.bookmarkManager.drop");
};

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_BOOKMARK_MANAGER_API_H_
