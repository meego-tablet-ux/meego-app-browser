// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <map>

#include <QFileDialog>
#include <QString>
#include <QSet>

#include "ui/base/l10n/l10n_util.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/mime_util.h"
#include "base/sys_string_conversions.h"
#include "base/utf_string_conversions.h"
#include "base/threading/thread.h"
#include "chrome/browser/browser_process.h"
#include "content/browser/browser_thread.h"
#include "chrome/browser/ui/shell_dialogs.h"
#include "grit/generated_resources.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"
#include "chrome/browser/ui/meegotouch/select_file_dialog_qt.h"

#include <QDeclarativeView>
#include <QDeclarativeContext>

class SelectFileDialogQtImpl;
///\todo: Need to implement select folder/file open/multi file open dialogs.

// Implementation of SelectFileDialog that shows a common dialog for
// choosing a file or folder. This acts as a modal dialog.
class SelectFileDialogImpl : public SelectFileDialog {
 public:
  explicit SelectFileDialogImpl(Listener* listener);

  // BaseShellDialog implementation.
  virtual bool IsRunning(gfx::NativeWindow parent_window) const;
  virtual void ListenerDestroyed();

  // SelectFileDialog implementation.
  // |params| is user data we pass back via the Listener interface.
  virtual void SelectFileImpl(Type type,
                          const string16& title,
                          const FilePath& default_path,
                          const FileTypeInfo* file_types,
                          int file_type_index,
                          const FilePath::StringType& default_extension,
                          gfx::NativeWindow owning_window,
                          void* params);
  void FileNotSelected();
  void FileSelected(QString uri);
  void MultiFilesSelected(const std::vector<FilePath>& files);
 private:
  virtual ~SelectFileDialogImpl();

  // QML interaction 
  SelectFileDialogQtImpl* impl_;

  // The file filters.
  FileTypeInfo file_types_;

  // The index of the default selected file filter.
  // Note: This starts from 1, not 0.
  size_t file_type_index_;

  // The type of dialog we are showing the user.
  Type type_;

  // These two variables track where the user last saved a file or opened a
  // file so that we can display future dialogs with the same starting path.
  static FilePath* last_saved_path_;
  static FilePath* last_opened_path_;

  std::map<QFileDialog*, void*> params_map_;

  // All our dialogs. -> do we have chance to open multiple dialogs??
  QSet<QFileDialog*> dialogs_;

  void* PopParamsForDialog(QFileDialog* dialog);

  void ProcessResult(QFileDialog* dialog, int result);

  void AddFilters(QFileDialog* dialog);

  QFileDialog* CreateSelectFolderDialog(const QString& title,
      const FilePath& default_path, gfx::NativeWindow parent);

  QFileDialog* CreateFileOpenDialog(const QString& title,
      const FilePath& default_path, gfx::NativeWindow parent);

  QFileDialog* CreateMultiFileOpenDialog(const QString& title,
      const FilePath& default_path, gfx::NativeWindow parent);

  QFileDialog* CreateSaveAsDialog(const QString& title,
      const FilePath& default_path, gfx::NativeWindow parent);

  DISALLOW_COPY_AND_ASSIGN(SelectFileDialogImpl);
};

FilePath* SelectFileDialogImpl::last_saved_path_ = NULL;
FilePath* SelectFileDialogImpl::last_opened_path_ = NULL;

// static
SelectFileDialog* SelectFileDialog::Create(Listener* listener) {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::FILE));
  return new SelectFileDialogImpl(listener);
}

SelectFileDialogImpl::SelectFileDialogImpl(Listener* listener)
    : SelectFileDialog(listener) {
  if (!last_saved_path_) {
    last_saved_path_ = new FilePath();
    last_opened_path_ = new FilePath();
  }

  Browser* browser = BrowserList::GetLastActive();
  BrowserWindowQt* browser_window = (BrowserWindowQt*)browser->window();
  impl_ = browser_window->GetSelectFileDialog();
  impl_->SetDialog(this);

}

SelectFileDialogImpl::~SelectFileDialogImpl() {
  QSet<QFileDialog*>::iterator iter = dialogs_.begin();
  while (iter != dialogs_.end()) {
    delete (*iter);
    iter = dialogs_.erase(iter);
  }
}

bool SelectFileDialogImpl::IsRunning(gfx::NativeWindow parent_window) const {
  DNOTIMPLEMENTED();
  return false;
}

void SelectFileDialogImpl::ListenerDestroyed() {
  listener_ = NULL;
}

void SelectFileDialogImpl::AddFilters(QFileDialog* dialog) {
  QStringList filterlists;
  QString filter;
  for (size_t i = 0; i < file_types_.extensions.size(); ++i) {

    filter.clear();

    for (size_t j = 0; j < file_types_.extensions[i].size(); ++j) {
      if (!file_types_.extensions[i][j].empty()) {
        filter.append("*.");
        filter.append(QString::fromUtf8(file_types_.extensions[i][j].c_str()));
        filter.append(" ");
      }
    }

    if (filter.isEmpty())
      continue;

    filter.prepend(" ( ");
    filter.append(")");

    if (i < file_types_.extension_description_overrides.size()) {
      filter.prepend(QString::fromStdWString(
          UTF16ToWide(file_types_.extension_description_overrides[i])));
    } else {
      // There is no system default filter description so we use
      // the MIME type itself if the description is blank.
      std::string mime_type = mime_util::GetFileMimeType(
          FilePath("name.").Append(file_types_.extensions[i][0]));
      filter.prepend(QString::fromUtf8(mime_type.c_str()));
    }

    filterlists << filter;
  }

  // Add the *.* filter, but only if we have added other filters
  if (file_types_.include_all_files && file_types_.extensions.size() > 0) {
    QString filter_all = QString::fromStdString(l10n_util::GetStringUTF8(IDS_SAVEAS_ALL_FILES));
    filter_all.append(" ( * )");
    filterlists << filter_all;
  }

  dialog->setNameFilters(filterlists);
}

QFileDialog* SelectFileDialogImpl::CreateSelectFolderDialog(const QString& title,
    const FilePath& default_path, gfx::NativeWindow parent) {
  DNOTIMPLEMENTED();
  return NULL;
}

QFileDialog* SelectFileDialogImpl::CreateFileOpenDialog(const QString& title,
    const FilePath& default_path, gfx::NativeWindow parent) {
  DNOTIMPLEMENTED();
  return NULL;
}

QFileDialog* SelectFileDialogImpl::CreateMultiFileOpenDialog(const QString& title,
    const FilePath& default_path, gfx::NativeWindow parent) {
  DNOTIMPLEMENTED();
  return NULL;
}

QFileDialog* SelectFileDialogImpl::CreateSaveAsDialog(const QString& title,
    const FilePath& default_path, gfx::NativeWindow parent) {

  QString title_string = !title.isEmpty() ? title :
      QString::fromStdString(l10n_util::GetStringUTF8(IDS_SAVE_AS_DIALOG_TITLE));

  QFileDialog* dialog = new QFileDialog();

  dialog->setFileMode(QFileDialog::AnyFile);
  dialog->setAcceptMode(QFileDialog::AcceptSave);

  AddFilters(dialog);

  if (!default_path.empty()) {
    dialog->setDirectory(QString::fromUtf8(default_path.DirName().value().c_str()));
    dialog->selectFile(QString::fromUtf8(default_path.BaseName().value().c_str()));
  } else if (!last_saved_path_->empty()) {
    dialog->setDirectory(QString::fromUtf8(last_saved_path_->value().c_str()));
  }

  return dialog;
}

void* SelectFileDialogImpl::PopParamsForDialog(QFileDialog* dialog) {
  std::map<QFileDialog*, void*>::iterator iter = params_map_.find(dialog);
  DCHECK(iter != params_map_.end());
  void* params = iter->second;
  params_map_.erase(iter);
  return params;
}

void SelectFileDialogImpl::FileNotSelected() {
  if (listener_)
    listener_->FileSelectionCanceled(NULL);
}

void SelectFileDialogImpl::FileSelected(QString uri) {
  if (listener_)
    listener_->FileSelected(FilePath(uri.toAscii().data()), 1, NULL);
}

void SelectFileDialogImpl::MultiFilesSelected(const std::vector<FilePath>& files) {
  *last_opened_path_ = files[0].DirName();

  if (listener_)
    listener_->MultiFilesSelected(files, NULL);
}

void SelectFileDialogImpl::ProcessResult(QFileDialog* dialog, int result) {

  if (result == QDialog::Rejected) {
    FileNotSelected();
    return;
  }

  char* filename = NULL;

  switch (type_) {
    case SELECT_FOLDER:
      break;
    case SELECT_OPEN_FILE:
      break;
    case SELECT_OPEN_MULTI_FILE:
      break;
    case SELECT_SAVEAS_FILE:
      filename = (dialog->selectedFiles()).value(0).toAscii().data();
      if (!filename) {
        FileNotSelected();
      } else {
        FileSelected(QString(filename));
      }

      return;

    default:
      NOTREACHED();
      return;
  }

}

// We ignore |default_extension|.
void SelectFileDialogImpl::SelectFileImpl(
    Type type,
    const string16& title,
    const FilePath& default_path,
    const FileTypeInfo* file_types,
    int file_type_index,
    const FilePath::StringType& default_extension,
    gfx::NativeWindow owning_window,
    void* params) {

  type_ = type;

  QString title_string = QString::fromStdString(UTF16ToUTF8(title));

  file_type_index_ = file_type_index;
  if (file_types)
    file_types_ = *file_types;
  else
    file_types_.include_all_files = true;
#if 0
  QFileDialog* dialog = NULL;
  switch (type) {
    case SELECT_FOLDER:
      dialog = CreateSelectFolderDialog(title_string, default_path, owning_window);
      break;
    case SELECT_OPEN_FILE:
      dialog = CreateFileOpenDialog(title_string, default_path, owning_window);
      break;
    case SELECT_OPEN_MULTI_FILE:
      dialog = CreateMultiFileOpenDialog(title_string, default_path, owning_window);
      break;
    case SELECT_SAVEAS_FILE:
      dialog = CreateSaveAsDialog(title_string, default_path, owning_window);
      break;
    default:
      NOTREACHED();
      return;
  }

  if (!dialog)
    return;

  dialogs_.insert(dialog);

  params_map_[dialog] = params;

  int result;
  result = dialog->exec();
  ProcessResult(dialog, result);
#else
  ///\todo At present, we just return a predefined absolute path
  switch (type) {
    case SELECT_FOLDER:
      break;
    case SELECT_OPEN_FILE:
      if (listener_) {
        impl_->SetMultiSelection(false);
        impl_->Popup();
      }
      break;
    case SELECT_OPEN_MULTI_FILE:
      if (listener_) {
        impl_->SetMultiSelection(true);
        impl_->Popup();
      }
      break;
    case SELECT_SAVEAS_FILE:
      if (listener_) {
        QString download_path = QDir::homePath() + QDir::separator() + QString("Downloads");
        QString full_file_path = download_path + QDir::separator() + QString::fromUtf8(default_path.BaseName().value().c_str());
        listener_->FileSelected(FilePath(full_file_path.toAscii().data()), 1, params);
      }
      break;
    default:
      NOTREACHED();
      return;
  }

#endif
}

SelectFileDialogQtImpl::SelectFileDialogQtImpl(BrowserWindowQt *window)
    : window_(window),
      dialog_(NULL),
      multi_selection_(false) {
  QDeclarativeView* view = window_->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();

  context->setContextProperty("selectFileDialogObject", this);
}

void SelectFileDialogQtImpl::SetDialog(SelectFileDialogImpl* dialog)
{
  dialog_ = dialog;
}

void SelectFileDialogQtImpl::Popup() {
  Q_EMIT popup();
}

void SelectFileDialogQtImpl::Dismiss() {
  Q_EMIT dismiss();
}

void SelectFileDialogQtImpl::OnPickerSelected(QString uri) {
  uri.replace("file://", "");
  dialog_->FileSelected(uri);
  Dismiss();
}

void SelectFileDialogQtImpl::OnPickerMultiSelected(QString uris) {
  QStringList list = uris.split(",file://", QString::SkipEmptyParts);
  list.replaceInStrings("file://", "");
  std::vector<FilePath> files;
  for (int i = 0; i < list.length(); i ++) { 
    files.push_back(FilePath(list[i].toAscii().data()));
  }
  dialog_->MultiFilesSelected(files);
  Dismiss();
}

void SelectFileDialogQtImpl::OnPickerCancelled() {
  dialog_->FileNotSelected();
  Dismiss();
}

void SelectFileDialogQtImpl::SetMultiSelection(bool multi_selection) {
  multi_selection_ = multi_selection;
}

bool SelectFileDialogQtImpl::IsMultiSelection() {
  return multi_selection_;
}
