// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Copyright (c) 2010, Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/meegotouch/bookmark_qt.h"

#include <vector>

#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "base/utf_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/bookmarks/bookmark_node_data.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "chrome/browser/bookmarks/bookmark_utils.h"
#include "chrome/browser/browser_shutdown.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"
#include "chrome/browser/history/recent_and_bookmark_thumbnails_qt.h"
#include "chrome/browser/importer/importer_data_types.h"
#include "chrome/browser/metrics/user_metrics.h"
#include "chrome/browser/ntp_background_util.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/browser/tab_contents/tab_contents_view.h"
#include "content/common/notification_service.h"
#include "chrome/common/pref_names.h"
#undef signals
#include "ui/gfx/canvas_skia_paint.h"
#include "grit/app_resources.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"
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
#include <QSortFilterProxyModel>

#define TOQTSTR(x) QString::fromUtf8(l10n_util::GetStringUTF8(x).c_str())

namespace BookmarkList {
bool bStarted = false;
}

namespace {

// The showing height of the bar.
const int kBookmarkBarHeight = 40;
const int kBookmarkBarWidth = 160;

// The height of the bar when it is "hidden". It is usually not completely
// hidden because even when it is closed it forms the bottom few pixels of
// the toolbar.
const int kBookmarkBarMinimumHeight = 3;

// Left-padding for the instructional text.
const int kInstructionsPadding = 6;

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// BookmarkListItem

void BookmarkListItem::RequestImg(int index) {
  index_=index;
  GURL url(WideToUTF8(url_.toStdWString()));
  history::TopSites* ts = browser_->profile()->GetTopSites();
  if (ts) {
    scoped_refptr<RefCountedBytes> jpeg_data;
    ts->GetPageThumbnail(url, &jpeg_data);
    if(jpeg_data.get()) {
      HandleThumbnailData(jpeg_data);
      return;
    }
    history::RecentAndBookmarkThumbnailsQt * recentThumbnails =
                            ts->GetRecentAndBookmarkThumbnails();
    if(recentThumbnails) {
       recentThumbnails->GetRecentPageThumbnail(url, &consumer_,
                        NewCallback(static_cast<BookmarkListItem*>(this),
                        &BookmarkListItem::OnThumbnailDataAvailable));
    }
  }
  else {
    HistoryService* hs = browser_->profile()->GetHistoryService(Profile::EXPLICIT_ACCESS);
    hs->GetPageThumbnail(url, &consumer_, NewCallback(static_cast<BookmarkListItem*>(this),
                                                 &BookmarkListItem::OnThumbnailDataAvailable));
  }
}

void BookmarkListItem::OnThumbnailDataAvailable(HistoryService::Handle request_handle, 
                                                scoped_refptr<RefCountedBytes> jpeg_data) {
  HandleThumbnailData(jpeg_data);
}

void BookmarkListItem::HandleThumbnailData(scoped_refptr<RefCountedBytes> jpeg_data) {
//  model_->beginReset();
  if (jpeg_data.get()) {
    std::vector<unsigned char> thumbnail_data;
    std::copy(jpeg_data->data.begin(), jpeg_data->data.end(),
              std::back_inserter(thumbnail_data));
    QImage image = QImage::fromData(thumbnail_data.data(), thumbnail_data.size());
    model_->Provider()->addImage(id(), image);
  }
//  model_->endReset();
  // Note that if the callback never returns, bookmark will failed to add
  model_->addBookmark(*this, index_); 
}

////////////////////////////////////////////////////////////////////////////////
// BookmarkQt

BookmarkQt::BookmarkQt(BrowserWindowQt* window,
                       Profile* profile, Browser* browser,
                       const QString &anotherFolder)
    : profile_(NULL),
      page_navigator_(NULL),
      browser_(browser),
      window_(window),
      model_(NULL),
      another_folder_name_(anotherFolder)
{
  list_impl_ = new BookmarkQtListImpl(this);
  filter_ = new BookmarkQtFilterProxyModel(list_impl_);
  bookmark_menu_ = new BookmarkListMenuModel(filter_);
}

BookmarkQt::~BookmarkQt() {
  if (model_) 
    model_->RemoveObserver(this);
  RemoveAllBookmarkListItems();
  delete list_impl_;
  delete filter_;
  delete bookmark_menu_;
}

//bool BookmarkQt::GetAcceleratorForCommandId(
//    int id,
//    ui::Accelerator* accelerator) {
//  return false;
//}
//
void BookmarkQt::Loaded(BookmarkModel* model) {
//  RemoveAllBookmarkListItems();
//  CreateAllBookmarkListItems();
}

void BookmarkQt::GetBookmarkProperties(const BookmarkNode* node, QString& title, QString &url, QString &id)
{
  const std::string& t = UTF16ToUTF8(node->GetTitle());
  QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
  title = QString::fromStdString(t);

  const std::string& u = node->GetURL().spec();
  url = QString::fromStdString(u);
  id = QString::number(node->id());
}

void BookmarkQt::ExposeModel(QString expose_name) {
  QDeclarativeView *view = window_->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();
  context->setContextProperty(expose_name, filter_);
}

bool BookmarkQt::IsMyParent(const BookmarkNode* parent) {
  return (parent == GetParent());
}

void BookmarkQt::SetProfile(Profile* profile) {
  CHECK(profile);
  if (profile_ == profile)
    return;

  profile_ = profile;

  if (model_)
    model_->RemoveObserver(this);

  // TODO(erg): Handle extensions

  model_ = profile_->GetBookmarkModel();
  model_->AddObserver(this);
  if (model_->IsLoaded())
    Loaded(model_);

  // else case: we'll receive notification back from the BookmarkModel when done
  // loading, then we'll populate the bar.
}

void BookmarkQt::SetPageNavigator(PageNavigator* navigator) {
  page_navigator_ = navigator;
}

void BookmarkQt::Init(Profile* profile) {
  SetProfile(profile);  
}

int BookmarkQt::GetHeight() {
  return 0; //TODO
// return event_box_->allocation.height - kBookmarkBarMinimumHeight;
}

void BookmarkQt::BookmarkModelBeingDeleted(BookmarkModel* model) {
  // The bookmark model should never be deleted before us. This code exists
  // to check for regressions in shutdown code and not crash.
  if (!browser_shutdown::ShuttingDownWithoutClosingBrowsers())
    NOTREACHED();

  // Do minimal cleanup, presumably we'll be deleted shortly.
  model_->RemoveObserver(this);
  model_ = NULL;
}

void BookmarkQt::BookmarkNodeAdded(BookmarkModel* model,
                                   const BookmarkNode* parent,
                                   int index) {
  // Only handles items of Bookmark Manager 
  if (!BookmarkList::bStarted || !IsMyParent(parent)) return;
  const BookmarkNode* node = parent->GetChild(index);
  BookmarkListItem* listitem = CreateBookmarkListItem(node);
//  list_impl_->addBookmark(*listitem, index);
  listitem->RequestImg(index);
}

void BookmarkQt::BookmarkNodeRemoved(BookmarkModel* model,
                                     const BookmarkNode* parent,
                                     int old_index,
                                     const BookmarkNode* node) {
  // Only handles items of Bookmark Manager 
  if (!BookmarkList::bStarted || !IsMyParent(parent)) return;
  list_impl_->removeBookmark(node);
}

void BookmarkQt::BookmarkNodeMoved(BookmarkModel* model,
                                   const BookmarkNode* old_parent,
                                   int old_index,
                                   const BookmarkNode* new_parent,
                                   int new_index) {
  const BookmarkNode* node = new_parent->GetChild(new_index);
  BookmarkNodeRemoved(model, old_parent, old_index, node);
  BookmarkNodeAdded(model, new_parent, new_index);
}

void BookmarkQt::BookmarkNodeChanged(BookmarkModel* model,
                                     const BookmarkNode* node) {
  // Only handles list_impl_ here
  if (!BookmarkList::bStarted || !IsMyParent(node->parent())) return;
  int index = GetParent()->GetIndexOf(node);
  DCHECK(index != -1);

  QString title, url, id;
  GetBookmarkProperties(node, title, url, id);
  list_impl_->updateBookmark(index, title, url, id);
}

void BookmarkQt::BookmarkNodeFaviconLoaded(BookmarkModel* model,
                                           const BookmarkNode* node) {
  BookmarkNodeChanged(model, node);
}

void BookmarkQt::BookmarkNodeChildrenReordered(BookmarkModel* model,
                                               const BookmarkNode* node) {
  if (!IsMyParent(node)) return;

  // Purge and rebuild the bar.
  RemoveAllBookmarkListItems();
  CreateAllBookmarkListItems();
}
BookmarkItem* BookmarkQt::CreateBookmarkItem(const BookmarkNode* node) {
  QString title, url, id;
  GetBookmarkProperties(node, title, url, id);
  BookmarkItem* item = new BookmarkItem(browser_, title, url, id);
  return item;
}

void BookmarkQt::openBookmarkItem(const int index) {
  const BookmarkNode* node = GetParent()->GetChild(index);
  page_navigator_ = browser_->GetSelectedTabContents();
  DCHECK(node);
  DCHECK(node->is_url());
  DCHECK(page_navigator_);

  page_navigator_->OpenURL(
      node->GetURL(), GURL(),
      NEW_FOREGROUND_TAB,//gtk_util::DispositionForCurrentButtonPressEvent(),
      PageTransition::AUTO_BOOKMARK);
  UserMetrics::RecordAction(UserMetricsAction("ClickedBookmarkBarURLButton"),
                            profile_);
  /*
  page_navigator_->OpenURL(
      node->GetURL(), GURL(),
      NEW_FOREGROUND_TAB,//gtk_util::DispositionForCurrentButtonPressEvent(),
      PageTransition::AUTO_BOOKMARK);
  UserMetrics::RecordAction(UserMetricsAction("ClickedBookmarkBarURLButton"),
                            profile_);
*/
}

void BookmarkQt::removeBookmarkInModel(int index) {
  const BookmarkNode* nodeFrom = GetParent()->GetChild(index);
  model_->Remove(GetParent(), index);
}

void BookmarkQt::moveBookmarkInModel(int from, int to) {
  const BookmarkNode* nodeFrom = GetParent()->GetChild(from);
  const string16 title = nodeFrom->GetTitle();
  const GURL url = nodeFrom->GetURL();

  // BookmarkModel doesn't support moving children under a same parent,
  // instead we remove then add it back.
  model_->Remove(GetParent(), from);
  model_->AddURL(GetParent(), to, title, url);
}

void BookmarkQt::MoveToAnotherFolder(int index) {
  const BookmarkNode* anotherParent = model_->GetBookmarkBarNode();
  if (anotherParent == GetParent())
    anotherParent = model_->other_node();

  const BookmarkNode* nodeFrom = GetParent()->GetChild(index);
  std::vector<const BookmarkNode*> nodes;
  model_->GetNodesByURL(nodeFrom->GetURL(), &nodes);
  if (!nodes.empty()){
    std::vector<const BookmarkNode*>::iterator it;
    for ( it=nodes.begin() ; it < nodes.end(); it++ ) {
      if ((*it)->parent() == anotherParent) {
         model_->Remove((*it)->parent(), (*it)->parent()->GetIndexOf(*it));
      }
    }
  }
 
  model_->Move(nodeFrom, anotherParent, anotherParent->child_count());
}

void BookmarkQt::titleChanged(QString id, QString title) {
  model_->SetTitle(model_->GetNodeByID(id.toInt()), title.utf16());
}

void BookmarkQt::urlChanged(QString id, QString url) { 
  model_->SetURL(model_->GetNodeByID(id.toInt()), GURL(WideToUTF8(url.toStdWString())));
}

void BookmarkQt::HideBookmarkManager() {
  filter_->CloseBookmarkManager();
}

// Note that this function cannot get items in sub-folders
void BookmarkQt::CreateAllBookmarkListItems() {
  for (int i = 0; i < GetParent()->child_count(); ++i) {
    const BookmarkNode* node = GetParent()->GetChild(i);
    BookmarkListItem* listitem = CreateBookmarkListItem(node);
//    list_impl_->addBookmark(*listitem);
    listitem->RequestImg(i);
  }
}

void BookmarkQt::RemoveAllBookmarkListItems() {
  list_impl_->clear();
}

void BookmarkQt::PopupMenu(gfx::Point p) {
  window_->ShowContextMenu(bookmark_menu_, p);
}

////////////////////////////////////////////////////////////////////////////////
// BookmarkBarQt

BookmarkBarQt::BookmarkBarQt(BrowserWindowQt* window,
                             Profile* profile, Browser* browser)
    : BookmarkQt(window, profile, browser, TOQTSTR(IDS_BOOMARK_BAR_OTHER_FOLDER_NAME))
{
/*  if (profile->GetProfileSyncService()) {
    // Obtain a pointer to the profile sync service and add our instance as an
    // observer.
    sync_service_ = profile->GetProfileSyncService();
    sync_service_->AddObserver(this);
  } */

  ExposeModel("bookmarkBarListModel");

  toolbar_impl_ = new BookmarkBarQtImpl(this);
  QDeclarativeView *view = window_->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();
  context->engine()->addImageProvider(QLatin1String("bookmark_bar"), list_impl_->Provider());
  context->setContextProperty("bookmarkBarModel", toolbar_impl_);
  
  context->setContextProperty("bookmarkInstruction", TOQTSTR(IDS_BOOKMARKS_NO_ITEMS));
  context->setContextProperty("bookmarkManagerTitle", TOQTSTR(IDS_BOOKMARK_MANAGER_TITLE));
  context->setContextProperty("bookmarkManagerSearchHolder", TOQTSTR(IDS_BOOKMARK_MANAGER_SEARCH_BUTTON));
  context->setContextProperty("bookmarkBarFolderName", TOQTSTR(IDS_BOOMARK_BAR_FOLDER_NAME));
  context->setContextProperty("bookmarkManagerMenuEdit", TOQTSTR(IDS_BOOKMARK_BAR_EDIT));

//  Init(profile);
//  SetProfile(profile);
  registrar_.Add(this, NotificationType::BROWSER_THEME_CHANGED,
                 NotificationService::AllSources());
  registrar_.Add(this, NotificationType::BOOKMARK_LIST_VISIBILITY_SHOW,
                 NotificationService::AllSources());
}

BookmarkBarQt::~BookmarkBarQt() {
  RemoveAllBookmarkButtons();
  delete toolbar_impl_;
}

void BookmarkBarQt::Loaded(BookmarkModel* model) {
  RemoveAllBookmarkButtons();
  if (!IsAlwaysShown()) {
    toolbar_impl_->Hide();
  } else {
    toolbar_impl_->Show();
  }

  if (!IsExistBookmarks()) {
    toolbar_impl_->addInstruction();   
  } else {
    CreateAllBookmarkButtons();
  }
}

const BookmarkNode* BookmarkBarQt::GetParent() {
  return model_->GetBookmarkBarNode();
}

void BookmarkBarQt::BookmarkNodeAdded(BookmarkModel* model,
                                      const BookmarkNode* parent,
                                      int index) {
  BookmarkQt::BookmarkNodeAdded(model, parent, index);
  if (!IsMyParent(parent)) return;
  DCHECK(index >= 0 && index <= GetBookmarkButtonCount());
  if(IsExistBookmarks()) {
    toolbar_impl_->removeInstruction();
    NotifyToMayShowBookmarkBar(true);
  }

  const BookmarkNode* node = parent->GetChild(index);
  BookmarkItem* item = CreateBookmarkItem(node);
  toolbar_impl_->addBookmark(*item, index);

//  MessageLoop::current()->PostTask(FROM_HERE,
//      method_factory_.NewRunnableMethod(
//          &BookmarkBarGtk::StartThrobbing, node));
}

void BookmarkBarQt::BookmarkNodeRemoved(BookmarkModel* model,
                                         const BookmarkNode* parent,
                                         int old_index,
                                         const BookmarkNode* node) {
  BookmarkQt::BookmarkNodeRemoved(model, parent, old_index, node);
  if (!IsMyParent(parent)) return;
  DCHECK(old_index >= 0 && old_index < GetBookmarkButtonCount());

  //int index = toolbar_impl_->index(QString::number(node->id()));
  toolbar_impl_->removeBookmark(node);
  int pos = GetBookmarkButtonCount();
  if (pos == 0) {
    toolbar_impl_->addInstruction();
//    NotifyToMayShowBookmarkBar(false);
  }
}

void BookmarkBarQt::BookmarkNodeChanged(BookmarkModel* model,
                                         const BookmarkNode* node) {
  BookmarkQt::BookmarkNodeChanged(model, node);
  if (!IsMyParent(node->parent())) return;
  int index = model_->GetBookmarkBarNode()->GetIndexOf(node);
  DCHECK(index != -1);

  BookmarkItem* item = CreateBookmarkItem(node);
  toolbar_impl_->updateBookmark(*item, index);
}

void BookmarkBarQt::BookmarkNodeChildrenReordered(BookmarkModel* model,
                                                  const BookmarkNode* node) {
  if (!IsMyParent(node)) return;

  // Purge and rebuild the bar.
  RemoveAllBookmarkButtons();
  CreateAllBookmarkButtons();
}

BookmarkListItem* BookmarkBarQt::CreateBookmarkListItem(const BookmarkNode* node) {
  QString title, url, id;
  GetBookmarkProperties(node, title, url, id);
  BookmarkListItem* item = new BookmarkListItem(browser_, list_impl_, title, url, id);
  item->type(QString("bar"));
  return item;
}

void BookmarkBarQt::CreateAllBookmarkButtons() {
//  BookmarkQt::CreateAllBookmarkListItems();
  const BookmarkNode* bar = model_->GetBookmarkBarNode();
  DCHECK(bar);
  
  toolbar_impl_->clear();
  //Create a button for each of the children on the bookmark bar.
  for (int i = 0; i < bar->child_count(); ++i) {
    const BookmarkNode* node = bar->GetChild(i);
    BookmarkItem* item = CreateBookmarkItem(node);
    toolbar_impl_->addBookmark(*item);
    DLOG(INFO) << "index of bar: " << i;
    DLOG(INFO) << "title : " << item->title().toStdString();
    DLOG(INFO) << "url : " << item->url().toStdString();
    DLOG(INFO) << "id : " << item->id().toStdString();
  }
//  toolbar_impl_->Show();
}

void BookmarkBarQt::RemoveAllBookmarkButtons() {
  BookmarkQt::RemoveAllBookmarkListItems();
  toolbar_impl_->clear();
}

int BookmarkBarQt::GetBookmarkButtonCount() {
  return toolbar_impl_->rowCount();
}

bool BookmarkBarQt::OnNewTabPage() {
  return (browser_ && browser_->GetSelectedTabContents() &&
          browser_->GetSelectedTabContents()->ShouldShowBookmarkBar());
}

bool BookmarkBarQt::GetTabContentsSize(gfx::Size* size) {
  Browser* browser = browser_;
  if (!browser) {
    NOTREACHED();
    return false;
  }
  TabContents* tab_contents = browser->GetSelectedTabContents();
  if (!tab_contents) {
    // It is possible to have a browser but no TabContents while under testing,
    // so don't NOTREACHED() and error the program.
    return false;
  }
  if (!tab_contents->view()) {
    NOTREACHED();
    return false;
  }
  *size = tab_contents->view()->GetContainerSize();
  return true;
}

bool BookmarkBarQt::IsAlwaysShown() {
  return profile_->GetPrefs()->GetBoolean(prefs::kShowBookmarkBar);
}

void BookmarkBarQt::ShowBookmarkManager() {
  filter_->OpenBookmarkManager();
  if (!BookmarkList::bStarted) {
    this->CreateAllBookmarkListItems();
    others_->CreateAllBookmarkListItems();
    BookmarkList::bStarted = true;
  }
}

void BookmarkBarQt::Observe(NotificationType type,
                            const NotificationSource& source,
                            const NotificationDetails& details) {
  if (type == NotificationType::BROWSER_THEME_CHANGED) {
    if (model_) {
      // Regenerate the bookmark bar with all new objects with their theme
      // properties set correctly for the new theme.
      RemoveAllBookmarkButtons();
      CreateAllBookmarkButtons();
    } else {
      DLOG(ERROR) << "Received a theme change notification while we "
                  << "don't have a BookmarkModel. Taking no action.";
    }

  } else if (type == NotificationType::BOOKMARK_LIST_VISIBILITY_SHOW) {
    ShowBookmarkManager();
  } else {
    NOTREACHED();
  }
}
/*
const BookmarkNode* BookmarkBarQt::GetNodeForToolButton(MWidget* widget) {
  // Search the contents of |bookmark_toolbar_| for the corresponding widget
  // and find its index.
  int index_to_use = -1;
  index_to_use = top_bookmarkbar_layout_->indexOf(widget);
  if (index_to_use != -1)
    return model_->GetBookmarkBarNode()->GetChild(index_to_use);

  return NULL;
}
*/
bool BookmarkBarQt::IsExistBookmarks() {
  return (model_->GetBookmarkBarNode()->child_count() != 0);
}

/*
void BookmarkBarQt::ConfigureButtonForNode(const BookmarkNode* node, BookmarkItem* item) {
  DCHECK(node);
  DCHECK(item);
  const std::string& title = UTF16ToUTF8(node->GetTitle());
  QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
  QString q_title = QString::fromStdString(title);
  button->setText(q_title);
}*/

void BookmarkBarQt::NotifyToMayShowBookmarkBar(const bool show) {
/*  PrefService* prefs = profile_->GetPrefs();

  // The user changed when the bookmark bar is shown, update the preferences.
  prefs->SetBoolean(prefs::kShowBookmarkBar, always_show);
  prefs->ScheduleSavePersistentPrefs();

  // And notify the notification service.
  Source<Profile> source(profile_);
  NotificationService::current()->Notify(
      NotificationType::BOOKMARK_BAR_VISIBILITY_PREF_CHANGED,
      source,
      NotificationService::NoDetails());*/
  if (show) {
    if (IsAlwaysShown()) {
      toolbar_impl_->Show();
    } else {
      toolbar_impl_->Hide();
    }
  } else {
    toolbar_impl_->Hide();
  }
}

void BookmarkBarQt::Init(Profile* profile, BookmarkOthersQt* others) {
  bookmark_menu_->Build(another_folder_name_);
  BookmarkQt::Init(profile);
  others_ = others;
}

////////////////////////////////////////////////////////////////////////////////
// BookmarkOthersQt

BookmarkOthersQt::BookmarkOthersQt(BrowserWindowQt* window,
                                   Profile* profile, Browser* browser)
    : BookmarkQt(window, profile, browser, TOQTSTR(IDS_BOOMARK_BAR_FOLDER_NAME))
{
/*  if (profile->GetProfileSyncService()) {
    // Obtain a pointer to the profile sync service and add our instance as an
    // observer.
    sync_service_ = profile->GetProfileSyncService();
    sync_service_->AddObserver(this);
  } */
  ExposeModel("bookmarkOthersListModel");

  QDeclarativeView *view = window_->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();
  context->engine()->addImageProvider(QLatin1String("bookmark_others"), list_impl_->Provider());
  context->setContextProperty("bookmarkBarOtherFolderName", TOQTSTR(IDS_BOOMARK_BAR_OTHER_FOLDER_NAME));

//  Init(profile);
//  SetProfile(profile);
}

BookmarkOthersQt::~BookmarkOthersQt() {
}

const BookmarkNode* BookmarkOthersQt::GetParent() {
  return model_->other_node();
}

BookmarkListItem* BookmarkOthersQt::CreateBookmarkListItem(const BookmarkNode* node) {
  QString title, url, id;
  GetBookmarkProperties(node, title, url, id);
  BookmarkListItem* item = new BookmarkListItem(browser_, list_impl_, title, url, id);
  item->type(QString("others"));
  return item;
}

void BookmarkOthersQt::Init(Profile* profile) {
  bookmark_menu_->Build(another_folder_name_);
  BookmarkQt::Init(profile);
}

////////////////////////////////////////////////////////////////////////////////
// BookmarkQtImpl

BookmarkQtImpl::BookmarkQtImpl(BookmarkQt* bookmark_qt, QObject *parent)
 : bookmark_qt_(bookmark_qt),
   QAbstractListModel(parent) 
{
  QHash<int, QByteArray> roles;
  roles[TitleRole] = "title";
  roles[UrlRole] = "url";
  roles[LengthRole] = "length";
  setRoleNames(roles);
}

bool BookmarkQtImpl::addBookmark(const BookmarkItem &bookmark) {
  if (bookmarks_.contains(bookmark)) 
    return false;
  beginInsertRows(QModelIndex(), rowCount(), rowCount());
  bookmarks_ << bookmark;
  endInsertRows();
  return true;
}

bool BookmarkQtImpl::addBookmark(const BookmarkItem &bookmark, int index) {
  if (bookmarks_.contains(bookmark)) 
    return false;
  beginInsertRows(QModelIndex(), index, index);
  bookmarks_.insert(index, bookmark);
  endInsertRows();
  return true;
}

bool BookmarkQtImpl::removeBookmark(int index) {
  if (index < 0) return false; 
  beginRemoveRows(QModelIndex(), index, index);
  bookmarks_.removeAt(index);
  endRemoveRows();
  return true;
}

bool BookmarkQtImpl::removeBookmark(const BookmarkItem &bookmark) {
  return removeBookmark(bookmarks_.indexOf(bookmark));
}

bool BookmarkQtImpl::removeBookmark(const BookmarkNode* node) {
  return removeBookmark(idx(QString::number(node->id())));
}

bool BookmarkQtImpl::updateBookmark(const BookmarkItem &bookmark, const int i) {
  if (i < 0) return false;
  //beginResetModel();
  bookmarks_[i] = bookmark;
  QModelIndex start = index(i, 0);
  QModelIndex end = index(i, 0);
  emit dataChanged(start, end);
  //endResetModel();
  return true;
}

bool BookmarkQtImpl::updateBookmark(const int i, QString t, QString u, QString is) {
  if (i < 0) return false;
  //beginResetModel();
  bookmarks_[i].title(t);
  bookmarks_[i].url(u);
  bookmarks_[i].id(is);
  //endResetModel();
  QModelIndex start = index(i, 0);
  QModelIndex end = index(i, 0);
  emit dataChanged(start, end);
  return true;
}

void BookmarkQtImpl::Show() {
  emit show();
}

void BookmarkQtImpl::Hide() {
  emit hide();
}

void BookmarkQtImpl::clear() {
  //beginResetModel();
  bookmarks_.clear();
  //endResetModel();
}

int BookmarkQtImpl::idx(QString id) {
  for (int i=0; i<bookmarks_.size(); i++)
  {
    if (id == bookmarks_[i].id())
      return i;
  }
  return -1;
}

int BookmarkQtImpl::rowCount(const QModelIndex& parent) const {
  return bookmarks_.count();
}

QVariant BookmarkQtImpl::data(const QModelIndex& index, int role) const {
  if(index.row() < 0 || index.row() > bookmarks_.count())
    return QVariant();

  if (role == TitleRole)    return bookmarks_[index.row()].title();
  else if (role == UrlRole) return bookmarks_[index.row()].url();
  else if (role == LengthRole) return bookmarks_[index.row()].title().toUtf8().size();
  return QVariant();
}

void BookmarkQtImpl::openBookmarkItem(const int index) {
  bookmark_qt_->openBookmarkItem(index);
}

void BookmarkQtImpl::backButtonTapped() {
  bookmark_qt_->HideBookmarkManager();
}

////////////////////////////////////////////////////////////////////////////////
// BookmarkBarQtImpl

BookmarkBarQtImpl::BookmarkBarQtImpl(BookmarkQt* bookmark_qt, QObject *parent)
 : BookmarkQtImpl(bookmark_qt, parent) {}

BookmarkBarQtImpl::~BookmarkBarQtImpl() {}

void BookmarkBarQtImpl::addInstruction() {
  emit showInstruction();
}

void BookmarkBarQtImpl::removeInstruction() {
  emit hideInstruction();
}

////////////////////////////////////////////////////////////////////////////////
// BookmarkQtListImpl

BookmarkQtListImpl::BookmarkQtListImpl(BookmarkQt* bookmark_qt, QObject *parent)
    : BookmarkQtImpl(bookmark_qt, parent),
      returnedImages_(0) {
  QHash<int, QByteArray> roles;
  roles[TitleRole] = "title";
  roles[UrlRole] = "url";
  roles[ImageRole] = "image";
  roles[IdRole] = "gridId";

  setRoleNames(roles);
}

BookmarkQtListImpl::~BookmarkQtListImpl() {
  clear();
}

QVariant BookmarkQtListImpl::data(const QModelIndex& index, int role) const {
  if(index.row() < 0 || index.row() > bookmarks_.count())
    return QVariant();
//  const BookmarkItem& bookmark = bookmarks_[index.row()];
  if (role == ImageRole)
    return bookmarks_[index.row()].image();
  else if (role == IdRole)
    return bookmarks_[index.row()].id();
  return BookmarkQtImpl::data(index, role);
}

void BookmarkQtListImpl::remove(int index) {
  //beginResetModel();
  bookmark_qt_->removeBookmarkInModel(index);
  //endResetModel();
}

int BookmarkQtListImpl::id(int index) {
  QString str=bookmarks_[index].id();
  int ret = str.toInt();
  return ret;
}

void BookmarkQtListImpl::moving(int from, int to) {
  if (to == from) return;
  BookmarkItem item = bookmarks_[from]; // not a good solution, but don't know why
                                        // beginMoveRows() not working correctly
  removeBookmark(item);
  addBookmark(item, to);
}

void BookmarkQtListImpl::moveDone(int from, int to) {
  if (from == to) return;
  bookmark_qt_->moveBookmarkInModel(from, to);
}

//void BookmarkQtListImpl::beginReset() {
//  returnedImages_++;
//  DLOG(INFO) << "current returned images "<<returnedImages_<<" , we need "<<rowCount();
//  if (returnedImages_ >= rowCount()) {
//     beginResetModel();
//  }
//}
//
//void BookmarkQtListImpl::endReset() {
//  if (returnedImages_ >= rowCount()) {
//     endResetModel();
//  }
//}
//
void BookmarkQtListImpl::clear() {
  returnedImages_ = 0;
  provider_.clear();
  BookmarkQtImpl::clear();
}

void BookmarkQtListImpl::titleChanged(QString id, QString title) {
  bookmark_qt_->titleChanged(id, title); 
}

void BookmarkQtListImpl::urlChanged(QString id, QString url) {
  bookmark_qt_->urlChanged(id, url); 
}

void BookmarkQtListImpl::openBookmarkItem(const int index) {
  bookmark_qt_->HideBookmarkManager();
  BookmarkQtImpl::openBookmarkItem(index);
}

void BookmarkQtListImpl::PopupMenu(int x, int y) {
  gfx::Point p(x,y);
  bookmark_qt_->PopupMenu(p);
}

////////////////////////////////////////////////////////////////////////////////
// BookmarkQtFilterProxyModel

BookmarkQtFilterProxyModel::BookmarkQtFilterProxyModel(BookmarkQtListImpl *impl, QObject *parent)
  : impl_(impl), QSortFilterProxyModel(parent)
{
  setSourceModel(impl_);
  setFilterRole(BookmarkQtListImpl::TitleRole);
  setFilterCaseSensitivity(Qt::CaseInsensitive);
  setDynamicSortFilter(true);
}

void BookmarkQtFilterProxyModel::openBookmarkItem(QString id) { 
  impl_->openBookmarkItem(toSource(impl_->idx(id))); 
}

void BookmarkQtFilterProxyModel::popupMenu(int x, int y) { 
  impl_->PopupMenu(x, y);
}

void BookmarkQtFilterProxyModel::textChanged(QString text) {
  setFilterFixedString(text); 
}

void BookmarkQtFilterProxyModel::backButtonTapped() { 
  impl_->backButtonTapped();
}

////////////////////////////////////////////////////////////////////////////////
// BookmarkListMenuModel

BookmarkListMenuModel::BookmarkListMenuModel(BookmarkQtFilterProxyModel* filter)
    : ALLOW_THIS_IN_INITIALIZER_LIST(ui::SimpleMenuModel(this)),
      filter_(filter) {
//  Build();
}

void BookmarkListMenuModel::Build(const QString& anotherFolder) {
  AddItem(IDC_BOOKMARK_MOVETO, 
          l10n_util::GetStringFUTF16(IDS_BOOKMARK_MANAGER_MOVETO,
                                     anotherFolder.utf16()));
  AddItemWithStringId(IDC_BOOKMARK_EDIT,   IDS_BOOKMARK_BAR_EDIT);
  AddItemWithStringId(IDC_BOOKMARK_REMOVE, IDS_BOOKMARK_BAR_REMOVE);
}

bool BookmarkListMenuModel::IsCommandIdChecked(int command_id) const {
  return false;
}

bool BookmarkListMenuModel::IsCommandIdEnabled(int command_id) const {
  return true;
}

bool BookmarkListMenuModel::GetAcceleratorForCommandId(
    int command_id,
    ui::Accelerator* accelerator) {
  return false;
}

void BookmarkListMenuModel::ExecuteCommand(int command_id) {
  switch (command_id) {
    case IDC_BOOKMARK_MOVETO: filter_->MoveToAnother(); break;
    case IDC_BOOKMARK_EDIT:   filter_->EditItem();   break;
    case IDC_BOOKMARK_REMOVE: filter_->RemoveItem(); break;
    default:
      LOG(WARNING) << "Received Unimplemented Command: " << command_id;
      break;
  }
}
#include "moc_bookmark_qt.cc"
