// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Copyright (c) 2010, Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <string>
#include "chrome/browser/ui/meegotouch/downloads_handler_qt.h"
#include "ui/base/l10n/l10n_util.h"
#include "base/i18n/rtl.h"
#include "base/basictypes.h"
#include "base/callback.h"
#include "base/file_path.h"
#include "base/mime_util.h"
#include "base/singleton.h"
#include "base/string_piece.h"
#include "base/string16.h"
#include "base/threading/thread.h"
#include "base/utf_string_conversions.h"
#include "base/string_number_conversions.h"
#include "base/values.h"
#include "base/i18n/time_formatting.h"
#include "chrome/browser/browser_process.h"
#include "content/browser/browser_thread.h"
#include "chrome/browser/ui/webui/chrome_url_data_manager.h"
#include "chrome/browser/ui/webui/fileicon_source.h"
#include "chrome/browser/download/download_history.h"
#include "chrome/browser/download/download_item.h"
#include "chrome/browser/download/download_util.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"
#include "chrome/browser/metrics/user_metrics.h"
#include "chrome/browser/profiles/profile.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "chrome/common/jstemplate_builder.h"
#include "chrome/common/url_constants.h"
#include "grit/browser_resources.h"
#include "grit/generated_resources.h"
#include "net/base/escape.h"
#include <QTextCodec>
#include <QString>
#include <QObject>
#include <QDeclarativeEngine>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDeclarativeItem>
#include <QGraphicsLineItem>
#include <QVariant>
#include <QAbstractListModel>
#include <QList>

namespace {

// Maximum number of downloads to show. TODO(glen): Remove this and instead
// stuff the downloads down the pipe slowly.
static const int kMaxDownloads = 150;

// Sort DownloadItems into descending order by their start time.
class DownloadItemSorter : public std::binary_function<DownloadItem*,
                                                       DownloadItem*,
                                                       bool> {
 public:
  bool operator()(const DownloadItem* lhs, const DownloadItem* rhs) {
    return lhs->start_time() > rhs->start_time();
  }
};

} // namespace

class DownloadViewItem
{

public:
  DownloadViewItem(QString& title, QString& url, int status, QString& progress, QString& date, int type):
    title_(title),
    url_(url),
    status_(status),
    progress_(progress),
    date_(date),
    type_(type)
    {
    }

  QString title() const
  {
    return title_;
  }

  QString url() const
  {
    return url_;
  }

  int status() const
  {
    return status_;
  }

  QString progress() const
  {
    return progress_;
  }

  QString date() const
  {
    return date_;
  }

  int type() const
  {
    return type_;
  }
  
  int show_date() const
  {
    return show_date_;
  }

  void set_show_date(const int show)
  {
    show_date_ = show;
  }

  DownloadViewItem& operator=(const DownloadViewItem& download)
  {
    if (this == &download)
      return *this;
    title_ = download.title();
    url_ = download.url();
    status_ = download.status();
    progress_ = download.progress();
    date_ = download.date();
    type_ = download.type();
    return *this;
  }

  bool operator==(const DownloadViewItem& download) const
  {
    return (title_ == download.title() && url_ == download.url());
  }
private:
  QString title_;
  QString url_;
  int status_;
  QString progress_;
  QString date_;
  int type_;
  int show_date_;
};
class DownloadsQtImpl: public QAbstractListModel
{
  Q_OBJECT
public:
  enum DownloadRoles {
    TitleRole = Qt::UserRole + 1,
    UrlRole,
    StatusRole,
    ProgressRole,
    ShowDateRole,
    DateRole,
    TypeRole
  };
  DownloadsQtImpl(DownloadsQtHandler* downloads_handler, QObject *parent = 0):
    QAbstractListModel(parent),
    downloads_handler_(downloads_handler)
  {
    QHash<int, QByteArray> roles;
    roles[TitleRole] = "title";
    roles[UrlRole] = "url";
    roles[StatusRole] = "s";
    roles[ProgressRole] = "progress";
    roles[ShowDateRole]= "show_date";
    roles[DateRole] = "downloadDate";
    roles[TypeRole] = "type";
    setRoleNames(roles);
  }

  bool updateDownloads(const QList<DownloadViewItem> m_list)
  {
    beginResetModel();
    m_downloadList.clear();
    m_downloadList = m_list;  
    endResetModel();
  }

  bool downloadItemUpdated(DownloadViewItem item)
  {
    int item_index = m_downloadList.indexOf(item);
    m_downloadList[item_index] = item;
    if (item_index >=0 && item_index < m_downloadList.size())
    {
      QModelIndex start = index(item_index, 0);
      QModelIndex end = index(item_index, 0);
      emit dataChanged(start, end);
    }
  }
  
  int rowCount(const QModelIndex& parent = QModelIndex()) const
  {
    return m_downloadList.count();
  }

  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const
  {
    if(index.row() < 0 || index.row() > m_downloadList.count())
      return QVariant();
    const DownloadViewItem& download = m_downloadList[index.row()];
    if (role == TitleRole)
      return download.title();
    else if (role == UrlRole)
      return download.url();
    else if (role == StatusRole)
      return download.status();
    else if (role == ProgressRole)
      return download.progress();
    else if (role == ShowDateRole)
      return download.show_date();
    else if (role ==  DateRole)
      return download.date();
    else if (role == TypeRole)
      return download.type();
    else return QVariant();
  }

  void Show()
  {
    emit show();
  }

  void Hide()
  {
    emit hide();
  }

Q_SIGNALS:
  void show();
  void hide();

public Q_SLOTS:
  void openDownloadItem(const int index)
  {
    downloads_handler_->HandleOpenFile(index);
  }
  void pauseDownloadItem(const int index)
  {
    downloads_handler_->HandlePause(index);
  }
  void resumeDownloadItem(const int index)
  {
    downloads_handler_->HandlePause(index);
  }
  void cancelDownloadItem(const int index)
  {
    downloads_handler_->HandleCancel(index);
  }
  void retryDownloadItem(const int index)
  {
    downloads_handler_->HandleRetry(index);
  }
  void removeDownloadItem(const int index)
  {
    downloads_handler_->HandleRemove(index);
  }
  void deleteDownloadItem(const int index)
  {
    downloads_handler_->HandleDelete(index);
  }
  void saveDownloadItem(const int index)
  {
    downloads_handler_->HandleSaveDangerous(index);
  }
  void discardDownloadItem(const int index)
  {
    downloads_handler_->HandleDiscardDangerous(index);
  }

  void textChanged(const QString search)
  {
    downloads_handler_->HandleGetDownloads(search);
  }
  void clearAllItem()
  {
    downloads_handler_->HandleClearAll();
  }
private:
  QList<DownloadViewItem> m_downloadList;
  DownloadsQtHandler* downloads_handler_;
};


DownloadsQtHandler::DownloadsQtHandler(BrowserWindowQt* window,
                                       Browser* browser,
                                       DownloadManager* dlm)
    : search_text_(),
      window_(window),
      browser_(browser),
      download_manager_(dlm){

  impl_ = new DownloadsQtImpl(this);
  QDeclarativeView *view = window_->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();
  context->setContextProperty("downloadsObject", impl_);
  
  QString q_title = QString::fromUtf8(l10n_util::GetStringUTF8(IDS_DOWNLOAD_TITLE).c_str());
  context->setContextProperty("downloadTitle", q_title);
  QString q_search = QString::fromUtf8(l10n_util::GetStringUTF8(IDS_DOWNLOAD_SEARCH_BUTTON).c_str());
  context->setContextProperty("downloadSearch", q_search);
  QString q_clear_all = QString::fromUtf8(l10n_util::GetStringUTF8(IDS_DOWNLOAD_LINK_CLEAR_ALL).c_str());
  context->setContextProperty("downloadClearAll", q_clear_all);
  QString q_danger_desc = QString::fromUtf8(l10n_util::GetStringUTF8(IDS_PROMPT_DANGEROUS_DOWNLOAD).c_str());
  QStringList q_list = q_danger_desc.split("$1");
  context->setContextProperty("downloadDangerDescPre", q_list[0]);
  context->setContextProperty("downloadDangerDescPos", q_list[1]);

  QString q_control_pause = QString::fromUtf8(l10n_util::GetStringUTF8(IDS_DOWNLOAD_LINK_PAUSE).c_str());
  context->setContextProperty("downloadControlPause", q_control_pause);
  QString q_control_cancel = QString::fromUtf8(l10n_util::GetStringUTF8(IDS_DOWNLOAD_LINK_CANCEL).c_str());
  context->setContextProperty("downloadControlCancel", q_control_cancel);
  QString q_control_resume = QString::fromUtf8(l10n_util::GetStringUTF8(IDS_DOWNLOAD_LINK_RESUME).c_str());
  context->setContextProperty("downloadControlResume", q_control_resume);
  QString q_control_remove = QString::fromUtf8(l10n_util::GetStringUTF8(IDS_DOWNLOAD_LINK_REMOVE).c_str());
  context->setContextProperty("downloadControlRemove", q_control_remove);
  QString q_control_retry = QString::fromUtf8(l10n_util::GetStringUTF8(IDS_DOWNLOAD_LINK_RETRY).c_str());
  context->setContextProperty("downloadControlRetry", q_control_retry);
  QString q_control_save = QString::fromUtf8(l10n_util::GetStringUTF8(IDS_SAVE_DOWNLOAD).c_str());
  context->setContextProperty("downloadControlSave", q_control_save);
  QString q_control_discard = QString::fromUtf8(l10n_util::GetStringUTF8(IDS_DISCARD_DOWNLOAD).c_str());
  context->setContextProperty("downloadControlDiscard", q_control_discard);
   
}

DownloadsQtHandler::~DownloadsQtHandler() {
  ClearDownloadItems();
  download_manager_->RemoveObserver(this);
}

// DownloadsQtHandler, public: -----------------------------------------------

void DownloadsQtHandler::Init() {
  download_manager_->AddObserver(this);
}

//TODO double check
void DownloadsQtHandler::OnDownloadUpdated(DownloadItem* download) {
  // Get the id for the download. Our downloads are sorted latest to first,
  // and the id is the index into that list. We should be careful of sync
  // errors between the UI and the download_items_ list (we may wish to use
  // something other than 'id').
  OrderedDownloads::iterator it = find(download_items_.begin(),
                                       download_items_.end(),
                                       download);
  if (it == download_items_.end())
    return;
  const int id = static_cast<int>(it - download_items_.begin());

//ListValue results_value;
//results_value.Append(download_util::CreateDownloadItemValue(download, id));
  UpdateCurrentDownload(download, id);
}

// A download has started or been deleted. Query our DownloadManager for the
// current set of downloads.
void DownloadsQtHandler::ModelChanged() {
  ClearDownloadItems();
  download_manager_->SearchDownloads(WideToUTF16(search_text_),
                                     &download_items_);
  sort(download_items_.begin(), download_items_.end(), DownloadItemSorter());

  // Scan for any in progress downloads and add ourself to them as an observer.
  for (OrderedDownloads::iterator it = download_items_.begin();
       it != download_items_.end(); ++it) {
    if (static_cast<int>(it - download_items_.begin()) > kMaxDownloads)
      break;

    DownloadItem* download = *it;
    if (download->state() == DownloadItem::IN_PROGRESS) {
      // We want to know what happens as the download progresses.
      download->AddObserver(this);
    } else if (download->safety_state() == DownloadItem::DANGEROUS) {
      // We need to be notified when the user validates the dangerous download.
      download->AddObserver(this);
    }
  }

  SendCurrentDownloads();
}

void DownloadsQtHandler::HandleGetDownloads(const QString& args) {
  std::wstring new_search = args.toStdWString();
  if (search_text_.compare(new_search) != 0) {
    search_text_ = new_search;
    ModelChanged();
  } else {
    SendCurrentDownloads();
  }
}

void DownloadsQtHandler::HandleOpenFile(const int args) {
  DownloadItem* file = GetDownloadById(args);
  if (file)
    file->OpenDownload();
}

void DownloadsQtHandler::HandleDrag(const ListValue* args) {
/*  DownloadItem* file = GetDownloadByValue(args);
  if (file) {
    IconManager* im = g_browser_process->icon_manager();
    SkBitmap* icon = im->LookupIcon(file->full_path(), IconLoader::NORMAL);
    gfx::NativeView view = dom_ui_->tab_contents()->GetNativeView();
    download_util::DragDownload(file, icon, view);
  }*/
}

void DownloadsQtHandler::HandleSaveDangerous(const int args) {
  DownloadItem* file = GetDownloadById(args);
  if (file)
    download_manager_->DangerousDownloadValidated(file);
}

void DownloadsQtHandler::HandleDiscardDangerous(const int args) {
  DownloadItem* file = GetDownloadById(args);
  if (file)
    file->Remove(true);
}

void DownloadsQtHandler::HandleShow(const int args) {
  DownloadItem* file = GetDownloadById(args);
  if (file)
    file->ShowDownloadInShell();
}

void DownloadsQtHandler::HandlePause(const int args) {
  DownloadItem* file = GetDownloadById(args);
  if (file)
    file->TogglePause();
}

void DownloadsQtHandler::HandleRemove(const int args) {
  DownloadItem* file = GetDownloadById(args);
  if (file)
    file->Remove(false);
}

void DownloadsQtHandler::HandleDelete(const int args) {
  DownloadItem* file = GetDownloadById(args);
  if (file)
    file->Remove(true);
}


void DownloadsQtHandler::HandleCancel(const int args) {
  DownloadItem* file = GetDownloadById(args);
  if (file)
    file->Cancel(true);
}

void DownloadsQtHandler::HandleRetry(const int args) {
  DownloadItem* file = GetDownloadById(args);
  if (file) {
    PageNavigator* page_navigator_ = browser_->GetSelectedTabContents();
    //DCHECK(file->url());
    DCHECK(page_navigator_);

    page_navigator_->OpenURL(
      file->url(), GURL(),
      NEW_FOREGROUND_TAB,
      PageTransition::LINK);
  }
}

void DownloadsQtHandler::HandleClearAll() {
  download_manager_->RemoveAllDownloads();
}

void DownloadsQtHandler::Show() {
  impl_->Show();
}

void DownloadsQtHandler::Hide() {
  impl_->Hide();
}
// DownloadsQtHandler, private: ----------------------------------------------
DownloadViewItem* DownloadsQtHandler::CreateDownloadViewItem(DownloadItem* download) {
  // Keep file names as LTR.
  string16 file_name = download->GetFileNameToReportUser().LossyDisplayName();
  file_name = base::i18n::GetDisplayStringInLTRDirectionality(file_name);
  const std::string& title = UTF16ToUTF8(file_name);
  QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
  QString q_title = QString::fromStdString(title);

  QString q_url = QString::fromStdString(download->url().spec());
  QString q_status_canceled = QString::fromUtf8(l10n_util::GetStringUTF8(IDS_DOWNLOAD_TAB_CANCELED).c_str());
  QString q_status_paused = QString::fromUtf8(l10n_util::GetStringUTF8(IDS_DOWNLOAD_PROGRESS_PAUSED).c_str());

  int i_status;
  QString q_progress;
  if (download->state() == DownloadItem::IN_PROGRESS) {
    if (download->safety_state() == DownloadItem::DANGEROUS) {
      i_status = 0; //("state", "DANGEROUS");
    } else if (download->is_paused()) {
      i_status = 1; //("state", "PAUSED");
      q_progress = q_status_paused;
    } else {
      i_status = 2; //("state", "IN_PROGRESS");
      q_progress = QString::fromStdString(UTF16ToASCII(download_util::GetProgressStatusText(download)));
    }
  } else if (download->state() == DownloadItem::CANCELLED) {
    i_status = 3; //("state", "CANCELLED");
    q_progress = q_status_canceled;
  } else if (download->state() == DownloadItem::COMPLETE) {
    if (download->safety_state() == DownloadItem::DANGEROUS) {
      i_status = 0; //("state", "DANGEROUS");
    } else {
      i_status = 4; //("state", "COMPLETE");
    }
  }
  const std::string& date = UTF16ToUTF8(base::TimeFormatShortDate(download->start_time()));
  QString q_date = QString::fromStdString(date); 

  //add type feature
  string16 file_path = download->full_path().LossyDisplayName();
  const std::string& path = UTF16ToUTF8(file_path);
  int i_type = FetchMimetypeIconID(path);
  DownloadViewItem* Item = new DownloadViewItem(q_title, q_url, i_status, q_progress, q_date, i_type);
  return Item;
}

int DownloadsQtHandler::FetchMimetypeIconID(const std::string& path) {

  std::string escaped_path = UnescapeURLComponent(path, UnescapeRule::SPACES);
#if defined(OS_WIN)
  // The path we receive has the wrong slashes and escaping for what we need;
  // this only appears to matter for getting icons from .exe files.
  std::replace(escaped_path.begin(), escaped_path.end(), '/', '\\');
  FilePath escaped_filepath(UTF8ToWide(escaped_path));
#elif defined(OS_POSIX)
  // The correct encoding on Linux may not actually be UTF8.
  FilePath escaped_filepath(escaped_path);
#endif
  std::string type = mime_util::GetFileMimeType(escaped_filepath);
  int i_type;
  if (type.find("text") != std::string::npos) 
    i_type = 1;
  else if (type.find("video") != std::string::npos)
    i_type = 2;
  else if (type.find("image") != std::string::npos)
    i_type = 3;
  else if (type.find("audio") != std::string::npos)
    i_type = 4;
  else 
    i_type = 5;
  return i_type;
}

void DownloadsQtHandler::UpdateCurrentDownload(DownloadItem* download, const int id) {
      DownloadViewItem* item =  CreateDownloadViewItem(download);
      impl_->downloadItemUpdated(*item);
}

void DownloadsQtHandler::SendCurrentDownloads() {
//  ListValue results_value;
  QList<DownloadViewItem> m_list;
  for (OrderedDownloads::iterator it = download_items_.begin();
      it != download_items_.end(); ++it) {
    int index = static_cast<int>(it - download_items_.begin());
    if (index > kMaxDownloads)
      break;
    DownloadViewItem* item =  CreateDownloadViewItem(*it);
    if (it == download_items_.begin())
      item->set_show_date(1);
    else if (item->date() != m_list.last().date())
      item->set_show_date(1);
    else item->set_show_date(0);
    m_list << *item;
  }
  impl_->updateDownloads(m_list);
}

void DownloadsQtHandler::ClearDownloadItems() {
  // Clear out old state and remove self as observer for each download.
  for (OrderedDownloads::iterator it = download_items_.begin();
      it != download_items_.end(); ++it) {
    (*it)->RemoveObserver(this);
  }
  download_items_.clear();
}

DownloadItem* DownloadsQtHandler::GetDownloadById(int id) {
  for (OrderedDownloads::iterator it = download_items_.begin();
      it != download_items_.end(); ++it) {
    if (static_cast<int>(it - download_items_.begin() == id)) {
      return (*it);
    }
  }

  return NULL;
}

DownloadItem* DownloadsQtHandler::GetDownloadByValue(const ListValue* args) {
  int id;
  if (ExtractIntegerValue(args, &id)) {
    return GetDownloadById(id);
  }
  return NULL;
}

bool DownloadsQtHandler::ExtractIntegerValue(const ListValue* value,
                                            int* out_int) {
  std::string string_value;
  if (value->GetString(0, &string_value))
    return base::StringToInt(string_value, out_int);
  NOTREACHED();
  return false;
}

std::wstring DownloadsQtHandler::ExtractStringValue(const ListValue* value) {
  string16 string16_value;
  if (value->GetString(0, &string16_value))
    return UTF16ToWideHack(string16_value);
  NOTREACHED();
  return std::wstring();
}

#include "moc_downloads_handler_qt.cc"
