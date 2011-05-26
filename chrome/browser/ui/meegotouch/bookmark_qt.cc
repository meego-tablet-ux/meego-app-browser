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
bool started = false;
bool change_from_ui = false;
int64 other_node_id = -1;

// Find item in bookmarks by ID, returns it's index
bool index(const QList<BookmarkItem*> bookmarks, int64 id, int &index) {
  bool found = false;
  index = 0;
  foreach (BookmarkItem *item, bookmarks) {
    if (item->id_ == id) {
      DLOG(INFO)<<"hdq search found "<<item->title_.toStdString();
      found = true; break;
    }
    index++;
  }
  return found;
}

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
// BookmarkGridItem

void BookmarkGridItem::RequestImg(int index) {
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
                        NewCallback(static_cast<BookmarkGridItem*>(this),
                        &BookmarkGridItem::OnThumbnailDataAvailable));
    }
  }
  else {
    HistoryService* hs = browser_->profile()->GetHistoryService(Profile::EXPLICIT_ACCESS);
    hs->GetPageThumbnail(url, &consumer_, NewCallback(static_cast<BookmarkGridItem*>(this),
                                                 &BookmarkGridItem::OnThumbnailDataAvailable));
  }
}

void BookmarkGridItem::OnThumbnailDataAvailable(HistoryService::Handle request_handle, 
                                                scoped_refptr<RefCountedBytes> jpeg_data) {
  HandleThumbnailData(jpeg_data);
}

void BookmarkGridItem::HandleThumbnailData(scoped_refptr<RefCountedBytes> jpeg_data) {
//  model_->beginReset();
  if (jpeg_data.get()) {
    std::vector<unsigned char> thumbnail_data;
    std::copy(jpeg_data->data.begin(), jpeg_data->data.end(),
              std::back_inserter(thumbnail_data));
    QImage image = QImage::fromData(thumbnail_data.data(), thumbnail_data.size());
    model_->Provider()->addImage(QString::number(id_), image);
  }
//  model_->endReset();
  // Note that if the callback never returns, bookmark will failed to add
  model_->addBookmark(this, index_); 
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
  grid_impl_ = new BookmarkQtGridImpl(this);
  tree_impl_ = new BookmarkQtTreeImpl(this);
  grid_filter_ = new BookmarkQtFilterProxyModel(grid_impl_);
  tree_filter_ = new BookmarkQtFilterProxyModel(tree_impl_);
  bookmark_menu_ = new BookmarkListMenuModel(tree_filter_);
}

BookmarkQt::~BookmarkQt() {
  if (model_) 
    model_->RemoveObserver(this);
  RemoveAllBookmarkListItems();
  delete grid_impl_;
  delete tree_impl_;
  delete grid_filter_;
  delete tree_filter_;
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

void BookmarkQt::GetBookmarkProperties(const BookmarkNode* node, 
    QString& title, QString &url, int64 &id) {
  const std::string& t = UTF16ToUTF8(node->GetTitle());
  QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
  title = QString::fromStdString(t);

  const std::string& u = node->GetURL().spec();
  url = QString::fromStdString(u);
  id = node->id();
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
  if (!BookmarkList::started || !IsMyParent(parent)) return;
  const BookmarkNode* node = parent->GetChild(index);
  DLOG(INFO)<<"hdq"<<__PRETTY_FUNCTION__<<" node title "<<node->GetTitle()<<" of parent "<<parent->id();

  BookmarkGridItem* griditem = CreateBookmarkGridItem(node);
//  grid_impl_->addBookmark(*griditem, index);
  griditem->RequestImg(index);

  if (!BookmarkList::change_from_ui) {
    BookmarkItem* listitem = CreateBookmarkItem(node);
    listitem->folder_id_ = parent->id();
    listitem->level_ = 1;
    tree_impl_->addBookmark(listitem);
    tree_impl_->addBookmarkToFolder(listitem, parent);
  }
}

void BookmarkQt::BookmarkNodeRemoved(BookmarkModel* model,
                                     const BookmarkNode* parent,
                                     int old_index,
                                     const BookmarkNode* node) {
  // Only handles grid items of Bookmark Manager 
  if (!BookmarkList::started || !IsMyParent(parent)) return;
  DLOG(INFO)<<"hdq"<<__PRETTY_FUNCTION__;
  grid_impl_->removeBookmark(node);

  if (!BookmarkList::change_from_ui)
    tree_impl_->removeBookmark(node);
}

void BookmarkQt::BookmarkNodeMoved(BookmarkModel* model,
                                   const BookmarkNode* old_parent,
                                   int old_index,
                                   const BookmarkNode* new_parent,
                                   int new_index) {
  DLOG(INFO)<<"hdq"<<__PRETTY_FUNCTION__<<" will call noderemove and nodeadd";
  const BookmarkNode* node = new_parent->GetChild(new_index);
  BookmarkNodeRemoved(model, old_parent, old_index, node);
  BookmarkNodeAdded(model, new_parent, new_index);
}

void BookmarkQt::BookmarkNodeChanged(BookmarkModel* model,
                                     const BookmarkNode* node) {
  // Only handles bookmark manager here
  if (!BookmarkList::started || !IsMyParent(node->parent())) return;
  DLOG(INFO)<<"hdq"<<__PRETTY_FUNCTION__;
  int index = GetParent()->GetIndexOf(node);
  DCHECK(index != -1);

  QString title, url; int64 id;
  GetBookmarkProperties(node, title, url, id);
  grid_impl_->updateBookmark(index, title, url, id);
  tree_impl_->updateBookmark(index, title, url, id);
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
  QString title, url; int64 id;
  GetBookmarkProperties(node, title, url, id);
  BookmarkItem* item = new BookmarkItem(browser_, title, url, id);
  return item;
}

BookmarkGridItem* BookmarkBarQt::CreateBookmarkGridItem(const BookmarkNode* node) {
  QString title, url; int64 id;
  GetBookmarkProperties(node, title, url, id);
  BookmarkGridItem* item = new BookmarkGridItem(browser_, grid_impl_, title, url, id);
  item->root_type_ = QString("bar");
  return item;
}

BookmarkGridItem* BookmarkOthersQt::CreateBookmarkGridItem(const BookmarkNode* node) {
  QString title, url; int64 id;
  GetBookmarkProperties(node, title, url, id);
  BookmarkGridItem* item = new BookmarkGridItem(browser_, grid_impl_, title, url, id);
  item->root_type_ = QString("others");
  return item;
}

void BookmarkQt::openBookmarkItem(int64 folder_id, int index) {
  const BookmarkNode* parent = model_->GetNodeByID(folder_id);
  const BookmarkNode* node = parent->GetChild(index);
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

void BookmarkQt::openBookmarkItem(int index) { 
  openBookmarkItem(GetParent()->id(), index); 
}

void BookmarkQt::removeBookmarkInModel(int64 folder_id, int index) {
  const BookmarkNode* parent = model_->GetNodeByID(folder_id);
  const BookmarkNode* nodeFrom = parent->GetChild(index);
  model_->Remove(parent, index);
}

void BookmarkQt::removeBookmarkInModel(int index) {
  removeBookmarkInModel(GetParent()->id(), index);
}

void BookmarkQt::removeBookmarkInModel(QString id) {
  const BookmarkNode* node = model_->GetNodeByID(id.toLong());
  int idx = GetParent()->GetIndexOf(node);
  removeBookmarkInModel(idx);
}

// Moving children under different parent
void BookmarkQt::moveBookmarkInModel(const BookmarkNode *old_parent,
                                     const BookmarkNode *new_parent,
                                     int from, int to) {
  DLOG(INFO)<<"hdq 3. move folder("<<old_parent->id()<<")'s "<<from<<" to folder("<<new_parent->id()<<")'s "<<to;
  if (old_parent->id() == new_parent->id()
      && to > from) to++; //   && (from+1) == to) to++;
  DLOG(INFO)<<"hdq 3.1 ==> folder("<<old_parent->id()<<")'s "<<from<<" to folder("<<new_parent->id()<<")'s "<<to;
  model_->Move(old_parent->GetChild(from), new_parent, to);
}

// Moving children under current parent
void BookmarkQt::moveBookmarkInModel(int from, int to) {
  if (from == to) return;
  moveBookmarkInModel(GetParent(), GetParent(), from, to);
}

// Moving children based on ID, returns moving validity
bool BookmarkQt::moveBookmarkInModel(QString from, QString to, QList<BookmarkItem *>& bookmarks) {
  DLOG(INFO)<<"hdq 2.1 before moving from "<<from.toStdString()<<" to "<<to.toStdString();
  const BookmarkNode* nodeFrom = model_->GetNodeByID(from.toLong());
  const BookmarkNode* nodeTo   = model_->GetNodeByID(to.toLong());
  if (!nodeFrom || !nodeTo) return false;
  DLOG(INFO)<<"hdq 2.1.1 before moving from "<<from.toStdString()<<" to "<<to.toStdString();
  const BookmarkNode* nodef_parent = nodeFrom->parent();
  const BookmarkNode* nodet_parent = nodeTo->parent();

  if (model_->is_permanent_node(nodeFrom)) return false; // forbid moving "bar" or "others"
  DLOG(INFO)<<"hdq 2.1.2 nodefrom is not permanent node";
  if (model_->is_bookmark_bar_node(nodeTo)) return false; // forbid moving over "bar"
  DLOG(INFO)<<"hdq 2.1.3 nodeto is not bar";

  int idxfrom = nodef_parent->GetIndexOf(nodeFrom);
  int idxto   = nodet_parent->GetIndexOf(nodeTo);
  int i, j;

  const BookmarkNode* bar   = model_->GetBookmarkBarNode();
  const BookmarkNode* other = model_->other_node();

  // Moving to a folder (currently "others" only)
  // \TODO Note that for supporting multi-folder in the future, a little more to do:
  // 1. record a preceded folder node for a folder, when creating the list
  //    (the recorded one will replace bar below.
  // 2. might need to handle left-right drag
  // 3. need to consider moving subfolder(and all its items)
  if (model_->is_other_bookmarks_node(nodeTo)) {  
    BookmarkItem *item = CreateBookmarkItem(nodeFrom);
    // Moving under different folder
    DLOG(INFO)<<"hdq 2.2 nodeTo is a folder of _others_";
    if (nodef_parent == nodeTo) {
      DLOG(INFO)<<"hdq 2.2.1 move id of "<<item->id_<<" to bar";
      moveBookmarkInModel(nodef_parent, bar, idxfrom, bar->child_count()); 
      item->folder_id_ = bar->id();
      if (!BookmarkList::index(bookmarks, bar->id(), i)) return true;
      item->level_ = bookmarks[i]->level_+1;

      // adjust children of two folders: add to "bar", remove from "others"
      DLOG(INFO)<<"hdq will add "<<item->title_.toStdString()<<" in bar. Level "<<item->level_;
      bookmarks[0]->children_<<item;
      if (BookmarkList::index(bookmarks, other->id(), i)) {
        if (BookmarkList::index(bookmarks[i]->children_, item->id_, j)) {
          DLOG(INFO)<<"hdq will remove "<<i<<" in others";
          bookmarks[i]->children_.removeAt(j);
        }
      }
    }
    else {
      DLOG(INFO)<<"hdq 2.2.1 move "<<item->id_<<" to others";
      moveBookmarkInModel(nodef_parent, nodeTo, idxfrom, 0); 
      item->folder_id_ = other->id();

      if (!BookmarkList::index(bookmarks, other->id(), i)) return true;
      item->level_ = bookmarks[i]->level_+1;

      // adjust children of two folders: add to "others", remove from "bar"
      if (BookmarkList::index(bookmarks, model_->other_node()->id(), i)) {
        item->level_ = bookmarks[i]->level_+1;
        DLOG(INFO)<<"hdq will add "<<item->title_.toStdString()<<" in others idx("<<i<<"). Level "<<item->level_;
        bookmarks[i]->children_.insert(0, item);
      }
      if (BookmarkList::index(bookmarks[0]->children_, item->id_, i)) {
        DLOG(INFO)<<"hdq will remove "<<i<<" in bar";
        bookmarks[0]->children_.removeAt(i);
      }
    }
  } else {
    // Moving under the same folder
    DLOG(INFO)<<"hdq 2.2 same folder: idx from "<<idxfrom<<" to "<<idxto;
    if (idxfrom < 0 || idxto < 0) return false;
    moveBookmarkInModel(nodef_parent, nodet_parent, idxfrom, idxto);

    // adjust children of this folder
    if (!BookmarkList::index(bookmarks, nodef_parent->id(), i)) return true; // this item don't have any parent
    BookmarkItem *tmp = bookmarks[i]->children_[idxto];
    bookmarks[i]->children_[idxto] = bookmarks[i]->children_[idxfrom];
    bookmarks[i]->children_[idxfrom] = tmp;
  }
  return true;
}

// move the NO.index item to the end of another folder
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
  model_->SetTitle(model_->GetNodeByID(id.toLong()), title.utf16());
}

void BookmarkQt::urlChanged(QString id, QString url) { 
  model_->SetURL(model_->GetNodeByID(id.toLong()), GURL(WideToUTF8(url.toStdWString())));
}

void BookmarkQt::HideBookmarkManager() {
  grid_filter_->CloseBookmarkManager();
}

// Note that this function cannot get items in sub-folders
void BookmarkQt::CreateAllBookmarkListItems() {

  for (int i = 0; i < GetParent()->child_count(); ++i) {
    const BookmarkNode* node = GetParent()->GetChild(i);

    //\TODO if have sub-folder - skip folder here and bar buttons
    BookmarkGridItem* griditem = CreateBookmarkGridItem(node);
//    grid_impl_->addBookmark(*griditem);
    griditem->RequestImg(i);
    //\TODO if have sub-folder - we will have different levels here

    //\TODO if have sub-folder, do the same as above
    BookmarkItem* listitem = CreateBookmarkItem(node);
    listitem->level_ = 1; // all at first level if no sub-folder...
    tree_impl_->addBookmark(listitem);
  }
}

void BookmarkQt::RemoveAllBookmarkListItems() {
  grid_impl_->clear();
  tree_impl_->clear();
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

  toolbar_impl_ = new BookmarkBarQtImpl(this);
  QDeclarativeView *view = window_->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();
  context->setContextProperty("bookmarkBarGridModel", grid_filter_);
  context->setContextProperty("bookmarkBarListModel", tree_filter_);

  context->engine()->addImageProvider(QLatin1String("bookmark_bar"), grid_impl_->Provider());
  context->setContextProperty("bookmarkBarModel", toolbar_impl_);

  all_trees_impl_ = new BookmarkQtTreeImpl(this);
  all_trees_filter_ = new BookmarkQtFilterProxyModel(all_trees_impl_);
  context->setContextProperty("bookmarkAllTreesModel", all_trees_filter_);
  
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
  delete all_trees_impl_;
  delete all_trees_filter_;
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
  // Add to grid
  BookmarkQt::BookmarkNodeAdded(model, parent, index);
  DLOG(INFO)<<"hdq"<<__PRETTY_FUNCTION__<<" add to grid done";

  // Add to tree
  const BookmarkNode* node = parent->GetChild(index);
  DLOG(INFO)<<"hdq"<<__PRETTY_FUNCTION__<<" node title "<<node->GetTitle()<<" of parent "<<parent->id()<<" index "<<index;
  BookmarkItem* item = CreateBookmarkItem(node);
  item->folder_id_ = parent->id();
  all_trees_impl_->addBookmarkToFolder(item, parent);
  DLOG(INFO)<<"hdq"<<__PRETTY_FUNCTION__<<" add to tree done";

  // Add to toolbar
  if (!IsMyParent(parent)) return;  // ignore if it is not in "bar"
  DCHECK(index >= 0 && index <= GetBookmarkButtonCount());
  if(IsExistBookmarks()) {
    toolbar_impl_->removeInstruction();
    NotifyToMayShowBookmarkBar(true);
  }
  item = CreateBookmarkItem(node);
  toolbar_impl_->addBookmark(item, index);

//  MessageLoop::current()->PostTask(FROM_HERE,
//      method_factory_.NewRunnableMethod(
//          &BookmarkBarGtk::StartThrobbing, node));
}

void BookmarkBarQt::BookmarkNodeRemoved(BookmarkModel* model,
                                         const BookmarkNode* parent,
                                         int old_index,
                                         const BookmarkNode* node) {
  DLOG(INFO)<<"hdq"<<__PRETTY_FUNCTION__<<" node title "<<node->GetTitle()<<" of parent "<<parent->id()<<" old index "<<old_index;
  // Remove on grid
  BookmarkQt::BookmarkNodeRemoved(model, parent, old_index, node);
  DLOG(INFO)<<"hdq end of grid remove";

  // Remove on tree
  if (!BookmarkList::change_from_ui) {
    all_trees_impl_->removeBookmark(node);
  }
  DLOG(INFO)<<"hdq end of tree remove";

  // Remove on toolbar
  if (!IsMyParent(parent)) return;
  DCHECK(old_index >= 0 && old_index < GetBookmarkButtonCount());

  //int index = toolbar_impl_->index(QString::number(node->id_));
  DLOG(INFO)<<"hdq before toolbar remove"<<GetBookmarkButtonCount();
  toolbar_impl_->removeBookmark(node);
  int pos = GetBookmarkButtonCount();
  DLOG(INFO)<<"hdq after  toolbar remove"<<GetBookmarkButtonCount();
  if (pos == 0) {
    toolbar_impl_->addInstruction();
//    NotifyToMayShowBookmarkBar(false);
  }
  DLOG(INFO)<<"hdq end of toolbar remove";
}

void BookmarkBarQt::BookmarkNodeChanged(BookmarkModel* model,
                                        const BookmarkNode* node) {
  // Change on grid
  BookmarkQt::BookmarkNodeChanged(model, node);

  // Change on tree
  BookmarkItem* item = CreateBookmarkItem(node);
  all_trees_impl_->updateBookmarkById(item, node->id());

  // Change on toolbar
  if (!IsMyParent(node->parent())) return;
  int index = model_->GetBookmarkBarNode()->GetIndexOf(node);
  DCHECK(index != -1);
  if (!toolbar_impl_->updateBookmark(item, index)) 
    return;

  delete item;
}

void BookmarkBarQt::BookmarkNodeChildrenReordered(BookmarkModel* model,
                                                  const BookmarkNode* node) {
  if (!IsMyParent(node)) return;

  // Purge and rebuild the bar.
  RemoveAllBookmarkButtons();
  CreateAllBookmarkButtons();
}

void BookmarkBarQt::CreateAllBookmarkButtons() {
//  BookmarkQt::CreateAllBookmarkListItems();
  const BookmarkNode* bar = model_->GetBookmarkBarNode();
  DCHECK(bar);
  
  toolbar_impl_->clear();
  //Create a button for each of the children on the bookmark bar.
  for (int i = 0; i < bar->child_count(); ++i) {
    BookmarkItem* item = CreateBookmarkItem(bar->GetChild(i));
    toolbar_impl_->addBookmark(item);
    DLOG(INFO) << "index of bar: " << i;
    DLOG(INFO) << "title : " << item->title().toStdString();
    DLOG(INFO) << "url : " << item->url().toStdString();
    DLOG(INFO) << "id : " << item->id_;
  }
//  toolbar_impl_->Show();
}

void BookmarkBarQt::RemoveAllBookmarkButtons() {
  BookmarkQt::RemoveAllBookmarkListItems();
  toolbar_impl_->clear();
}

// Always creates first level of folders only
// - let user to open subfolders
void BookmarkBarQt::CreateAllBookmarkTreeItems() {
  CreateTreeFolder(model_->GetBookmarkBarNode()); // Bookmark Bar
  CreateTreeFolder(model_->other_node());         // Other Bookmarks
  BookmarkList::other_node_id = model_->other_node()->id();
  all_trees_impl_->openItem(1); // Open "Others" first, then "Bar".
  all_trees_impl_->openItem(0);
}

void BookmarkBarQt::CreateTreeFolder(const BookmarkNode* node) {
  BookmarkItem* folder = CreateBookmarkItem(node);

  DLOG(INFO)<<__PRETTY_FUNCTION__
            <<"hdq adding " <<node->child_count()
            <<" children to folder "<<folder->title().toStdString()
            <<" id: "<<folder->id_;

  for (int i = 0; i < node->child_count(); ++i) {
    BookmarkItem* item = CreateBookmarkItem(node->GetChild(i));
    item->folder_id_ = folder->id_;
    folder->children_ << item;
    DLOG(INFO)<<__PRETTY_FUNCTION__<<"hdq adding NO."<<i
      <<" title: "<<item->title().toStdString()
      <<" id: "<<item->id_
      <<" now child size: "<<folder->children_.size();
  }
  folder->increaseChildrenLevels();
  all_trees_impl_->addBookmark(folder);
}

void BookmarkBarQt::RemoveAllBookmarkTreeItems() {
  all_trees_impl_->clear();
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
  grid_filter_->OpenBookmarkManager();
  if (!BookmarkList::started) {
    this->CreateAllBookmarkListItems();
    others_->CreateAllBookmarkListItems();
    CreateAllBookmarkTreeItems();
    BookmarkList::started = true;
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

  QDeclarativeView *view = window_->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();
  context->setContextProperty("bookmarkOthersGridModel", grid_filter_);
  context->setContextProperty("bookmarkOthersListModel", tree_filter_);
  context->engine()->addImageProvider(QLatin1String("bookmark_others"), grid_impl_->Provider());
  context->setContextProperty("bookmarkBarOtherFolderName", TOQTSTR(IDS_BOOMARK_BAR_OTHER_FOLDER_NAME));

//  Init(profile);
//  SetProfile(profile);
}

BookmarkOthersQt::~BookmarkOthersQt() {
}

const BookmarkNode* BookmarkOthersQt::GetParent() {
  return model_->other_node();
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
  roles.insert(TitleRole, QByteArray("title"));
  roles.insert(UrlRole, QByteArray("url"));
//  roles.insert(TypeRole, QByteArray("type"));
  roles.insert(LengthRole, QByteArray("length"));
  roles.insert(IdRole, QByteArray("bookmarkId"));
  roles.insert(LevelRole, QByteArray("level"));
  roles.insert(IsOpenedRole, QByteArray("isOpened"));
  roles.insert(HasChildrenRole, QByteArray("hasChildren"));
  setRoleNames(roles);
}

QVariant BookmarkQtImpl::data(const QModelIndex& index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() > (bookmarks_.size()-1) )
    return QVariant();

  BookmarkItem *item = bookmarks_[index.row()];
  switch (role) {
    case TitleRole:       return QVariant::fromValue(item->title_);
    case UrlRole:         return QVariant::fromValue(item->url_);
//    case TypeRole:        return QVariant::fromValue(item->type());
    case LengthRole:      return QVariant::fromValue(item->title_.toUtf8().size());
    case IdRole:          return QVariant::fromValue(item->id_);
    case LevelRole:       return QVariant::fromValue(item->level_);
    case IsOpenedRole:    return QVariant::fromValue(item->isOpened_);
    case HasChildrenRole: return QVariant::fromValue(item->hasChildren());
    return QVariant();
  }
}

bool BookmarkQtImpl::addBookmark(BookmarkItem *bookmark) {
  if (bookmarks_.contains(bookmark)) 
    return false;
  beginInsertRows(QModelIndex(), rowCount(), rowCount());
  bookmarks_ << bookmark;
  endInsertRows();
  return true;
}

bool BookmarkQtImpl::addBookmark(BookmarkItem *bookmark, int index) {
  if (bookmarks_.contains(bookmark)) 
    return false;
  beginInsertRows(QModelIndex(), index, index);
  bookmarks_.insert(index, bookmark);
  endInsertRows();
  return true;
}

bool BookmarkQtImpl::removeBookmark(int index) {
  if (index < 0) 
    return false; 
  beginRemoveRows(QModelIndex(), index, index);
  bookmarks_.removeAt(index);
  endRemoveRows();
  return true;
}

bool BookmarkQtImpl::removeBookmark(BookmarkItem *bookmark) {
  return removeBookmark(bookmarks_.indexOf(bookmark));
}

bool BookmarkQtImpl::removeBookmark(const BookmarkNode *node) {
  return removeBookmark(idx(node->id()));
}

bool BookmarkQtImpl::updateBookmark(BookmarkItem *bookmark, int i) {
  if (i < 0) return false;
  //beginResetModel();
  bookmarks_[i] = bookmark;
  QModelIndex start = index(i, 0);
  QModelIndex end = index(i, 0);
  emit dataChanged(start, end);
  //endResetModel();
  return true;
}

bool BookmarkQtImpl::updateBookmark(int i, QString t, QString u, int64 is) {
  if (i < 0) return false;
  //beginResetModel();
  bookmarks_[i]->title(t);
  bookmarks_[i]->url(u);
  bookmarks_[i]->id_ = is;
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

int BookmarkQtImpl::idx(int64 id) {
  for (int i=0; i<bookmarks_.size(); i++) {
    if (id == bookmarks_[i]->id_)
      return i;
  }
  return -1;
}

int BookmarkQtImpl::rowCount(const QModelIndex& parent) const {
  return bookmarks_.count();
}

void BookmarkQtImpl::openBookmarkItem(int index) {
  bookmark_qt_->HideBookmarkManager();
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
// BookmarkQtGridImpl

BookmarkQtGridImpl::BookmarkQtGridImpl(BookmarkQt* bookmark_qt, QObject *parent)
    : BookmarkQtImpl(bookmark_qt, parent),
      returnedImages_(0) {
  QHash<int, QByteArray> roles;
  roles[TitleRole] = "title";
  roles[UrlRole] = "url";
  roles[ImageRole] = "image";
  roles[IdRole] = "gridId";

  setRoleNames(roles);
}

BookmarkQtGridImpl::~BookmarkQtGridImpl() {
  clear();
}

QVariant BookmarkQtGridImpl::data(const QModelIndex& index, int role) const {
  if(index.row() < 0 || index.row() > bookmarks_.count())
    return QVariant();
//  const BookmarkItem& bookmark = bookmarks_[index.row()];
  if (role == ImageRole)
    return bookmarks_[index.row()]->image();
  return BookmarkQtImpl::data(index, role);
}

void BookmarkQtGridImpl::remove(QString id) {
  //beginResetModel();
  bookmark_qt_->removeBookmarkInModel(id);
  //endResetModel();
}

void BookmarkQtGridImpl::moving(int from, int to) {
  if (to == from) return;
  DLOG(INFO)<<__PRETTY_FUNCTION__<<"hdq grid moving "<< from << " ---> " << to;
  beginMoveRows(QModelIndex(), from, from, QModelIndex(), to > from ? to+1 : to);
  bookmarks_.move(from, to);
  endMoveRows();
  //BookmarkItem *item = bookmarks_[from]; // not a good solution, but don't know why
                                        // beginMoveRows() not working correctly
  //removeBookmark(item);
  //addBookmark(item, to);
}

void BookmarkQtGridImpl::moveDone(int from, int to) {
  if (from == to) return;
  DLOG(INFO)<<__PRETTY_FUNCTION__<<"hdq grid movedone "<< from << " ===> " << to;
  BookmarkList::change_from_ui = true;
  bookmark_qt_->moveBookmarkInModel(from, to);
  BookmarkList::change_from_ui = false;
}

//void BookmarkQtGridImpl::beginReset() {
//  returnedImages_++;
//  DLOG(INFO) << "current returned images "<<returnedImages_<<" , we need "<<rowCount();
//  if (returnedImages_ >= rowCount()) {
//     beginResetModel();
//  }
//}
//
//void BookmarkQtGridImpl::endReset() {
//  if (returnedImages_ >= rowCount()) {
//     endResetModel();
//  }
//}
//
void BookmarkQtGridImpl::clear() {
  returnedImages_ = 0;
  provider_.clear();
  BookmarkQtImpl::clear();
}

void BookmarkQtGridImpl::titleChanged(QString id, QString title) {
  bookmark_qt_->titleChanged(id, title); 
}

void BookmarkQtGridImpl::urlChanged(QString id, QString url) {
  bookmark_qt_->urlChanged(id, url); 
}

//void BookmarkQtGridImpl::openBookmarkItem(int index) {
//  bookmark_qt_->HideBookmarkManager();
//  BookmarkQtImpl::openBookmarkItem(index);
//}
//
void BookmarkQtGridImpl::PopupMenu(int x, int y) {
  gfx::Point p(x,y);
  bookmark_qt_->PopupMenu(p);
}

////////////////////////////////////////////////////////////////////////////////
// BookmarkQtTreeImpl

BookmarkQtTreeImpl::BookmarkQtTreeImpl(BookmarkQt* bookmark_qt, QObject *parent)
  : BookmarkQtImpl(bookmark_qt, parent) {}
//  QHash<int, QByteArray> roles = roleNames();
//  roles.insert(TitleRole, QByteArray("title"));
//  roles.insert(UrlRole, QByteArray("url"));
//  roles.insert(TypeRole, QByteArray("type"));
//  roles.insert(LengthRole, QByteArray("length"));
//  roles.insert(IdRole, QByteArray("treeId"));
//  roles.insert(LevelRole, QByteArray("level"));
//  roles.insert(IsOpenedRole, QByteArray("isOpened"));
//  roles.insert(HasChildrenRole, QByteArray("hasChildren"));
//  setRoleNames(roles);
//}

//QVariant BookmarkQtTreeImpl::data(const QModelIndex &index, int role) const {
//  if (!index.isValid() || index.row() > (bookmarks_.size()-1) )
//    return QVariant();
//
//  BookmarkItem *item = bookmarks_[index.row()];
//  switch (role) {
//  case LevelRole:   return QVariant::fromValue(item->level_);
//  case IsOpenedRole:  return QVariant::fromValue(item->isOpened_);
//  case HasChildrenRole: return QVariant::fromValue(item->hasChildren());
//  default: return BookmarkQtImpl::data(index, role);
//  }
//}
//
int BookmarkQtTreeImpl::rowCount(const QModelIndex &parent) const {
  Q_UNUSED(parent)
  return bookmarks_.size();
}

void BookmarkQtTreeImpl::openItem(int idx) {
  BookmarkItem *item = bookmarks_[idx];
  DLOG(INFO)<<__PRETTY_FUNCTION__<<"hdq opening "<<item->title().toStdString()<<" of child size "<<item->children_.size()<<" isOpen "<<item->isOpened_;
  if (idx > (bookmarks_.size()-1) || item->isOpened_ )
    return;

  QModelIndex modelIndex = index(idx);
  item->isOpened_ = true;
  emit dataChanged(modelIndex, modelIndex);
  int i = idx+1;

  beginInsertRows(QModelIndex(), i, i+item->children_.size()-1);
  foreach(BookmarkItem *im, item->children_) {
    if (!bookmarks_.contains(im)) { // TODO: hdq this will make insert row a wrong count maybe
      bookmarks_.insert(i++, im);
      DLOG(INFO)<<__PRETTY_FUNCTION__<<"hdq added item "<<im->title().toStdString()<<" level "<<im->level_;
    }
  }
  endInsertRows();
}

void BookmarkQtTreeImpl::closeItem(int idx) {
  BookmarkItem *item = bookmarks_[idx];
  DLOG(INFO)<<__PRETTY_FUNCTION__<<"hdq closing "<<item->title().toStdString()<<" of child size "<<item->children_.size()<<" isOpen "<<item->isOpened_ << " level "<<item->level_;
  if (idx > (bookmarks_.size()-1) || !item->isOpened_)
    return;

  QModelIndex modelIndex = index(idx);
  item->isOpened_ = false;
  emit dataChanged(modelIndex, modelIndex);
  int i = idx+1;
  for (; i < bookmarks_.size() && (bookmarks_[i]->level_ > bookmarks_[idx]->level_); ++i) {}
  --i;
  DLOG(INFO)<<__PRETTY_FUNCTION__<<"hdq idx: "<<idx<<" i: "<<i;

  beginRemoveRows(QModelIndex(), idx+1, i);
  while (i > idx) {
    DLOG(INFO)<<__PRETTY_FUNCTION__<<"hdq removed item "<<bookmarks_[i]->title().toStdString();
    bookmarks_[i]->isOpened_ = false;
    bookmarks_.removeAt(i--);
  }
  endRemoveRows();
}

void BookmarkQtTreeImpl::remove(QString id) {
  int fpos, bpos;
  int64 bid = id.toLong();
  if (!BookmarkList::index(bookmarks_, bid, bpos)) return;
  int64 fid = bookmarks_[bpos]->folder_id_;
  // If this item has no folder
  if (-1 == fid || !BookmarkList::index(bookmarks_, fid, fpos)) {
    bookmark_qt_->removeBookmarkInModel(id);
    return;
  }

  // else find it's real index
  if (!BookmarkList::index(bookmarks_[fpos]->children_, bid, bpos)) {
    DLOG(INFO)<<"hdq"<<__PRETTY_FUNCTION__<<" bookmark item not found in children! "<<bookmarks_[fpos]->title_.toStdString();
    return;
  }

  //beginResetModel();
  bookmark_qt_->removeBookmarkInModel(fid, bpos);
  //endResetModel();
}

//int BookmarkQtTreeImpl::id(int index) {
//  QString str=bookmarks_[index]->id_;
//  int ret = str.toInt();
//  return ret;
//}
//
void BookmarkQtTreeImpl::moving(int from, int to) {
  if (to == from) return;
  DLOG(INFO)<<"hdq moving "<<from<<" --> "<<to;
  beginMoveRows(QModelIndex(), from, from, QModelIndex(), to > from ? to+1 : to);
  bookmarks_.move(from, to);
  endMoveRows();
}

//void BookmarkQtTreeImpl::moveDone(int from, int to) {
//  if (to == from) return;
//  DLOG(INFO)<<"hdq: moveDone 1. will move idx from "<<from<<" to "<<to
//            <<" ["<<bookmarks_[from]->title_.toStdString()<<"] to the place of ["<<bookmarks_[to]->title_.toStdString()<<"]";
//  bookmark_qt_->moveBookmarkInModel(bookmarks_[from]->id_, bookmarks_[to]->id_);
//}

void BookmarkQtTreeImpl::moveDone(int f, int t, QString from, QString to) {
  if (to == from) return;
  DLOG(INFO)<<"hdq: 1. will movedone "<<f<<"-->"<<t<<" id: "<<from.toStdString()<<" ==> "<<to.toStdString();
  BookmarkList::change_from_ui = true;
  bool ok = bookmark_qt_->moveBookmarkInModel(from, to, bookmarks_);
  BookmarkList::change_from_ui = false;

  if (ok) {
    beginMoveRows(QModelIndex(), f, f, QModelIndex(), t > f ? t+1 : t);
    bookmarks_.move(f, t);
    endMoveRows();
  }
  DLOG(INFO)<<"hdq: 9. done movedone "<<f<<"-->"<<t<<" id: "<<from.toStdString()<<" ==> "<<to.toStdString();
  DLOG(INFO)<<"hdq";
}

void BookmarkQtTreeImpl::titleChanged(QString id, QString title) {
  bookmark_qt_->titleChanged(id, title); 
}

void BookmarkQtTreeImpl::urlChanged(QString id, QString url) {
  bookmark_qt_->urlChanged(id, url); 
}

void BookmarkQtTreeImpl::openBookmarkItem(int index) {
  int fpos, bpos;
  int64 bid = bookmarks_[index]->id_;
  int64 fid = bookmarks_[index]->folder_id_;
  DLOG(INFO)<<"hdq"<<__PRETTY_FUNCTION__<<" fid "<<fid<<" bid "<<bid;
  // If this item has no folder
  if (-1 == fid || !BookmarkList::index(bookmarks_, fid, fpos)) {
    BookmarkQtImpl::openBookmarkItem(index);
    return;
  }

  // else find it's real index
  if (!BookmarkList::index(bookmarks_[fpos]->children_, bid, bpos)) {
    DLOG(INFO)<<"hdq"<<__PRETTY_FUNCTION__<<" bookmark item not found in children! "<<bookmarks_[index]->title_.toStdString();
    return;
  }

  // open bookmark
  DLOG(INFO)<<"hdq"<<__PRETTY_FUNCTION__<<" will open "<<bookmarks_[index]->title_.toStdString()
      << " fid "<<fid<<" real pos "<<bpos;
  bookmark_qt_->HideBookmarkManager();
  bookmark_qt_->openBookmarkItem(fid, bpos);
}

void BookmarkQtTreeImpl::PopupMenu(int x, int y) {
  gfx::Point p(x,y);
  bookmark_qt_->PopupMenu(p);
}

// Simply return false for those items  which don't have folder
bool BookmarkQtTreeImpl::addBookmarkToFolder(BookmarkItem *bookmark, const BookmarkNode* parent) {
  DLOG(INFO)<<"hdq: a. adding "<<bookmark->title().toStdString()<<" to folder "<<parent->id();
  if (BookmarkList::change_from_ui || bookmarks_.contains(bookmark)) 
    return false;

  int folder_pos = 0;
  if (!BookmarkList::index(bookmarks_, parent->id(), folder_pos)) return false;
  bookmark->level_ = bookmarks_[folder_pos]->level_+1;

  DLOG(INFO)<<"hdq: b. adding "<<bookmark->title().toStdString()<<" to folder "<<parent->id()<<" pos: "<<folder_pos<<" + "<<parent->child_count();
  bookmarks_[folder_pos]->children_ << bookmark;
  if (bookmarks_[folder_pos]->isOpened_) {
    int index = folder_pos + parent->child_count();
    beginInsertRows(QModelIndex(), index, index);
    bookmarks_.insert(index, bookmark);
    endInsertRows();
  }
  return true;
}

bool BookmarkQtTreeImpl::updateBookmarkById(BookmarkItem *bookmark, int64 id) {
  DLOG(INFO)<<"hdq: updating"<<bookmark->title().toStdString();
  int i, j;
  if (!BookmarkList::index(bookmarks_, id, i))
    return false;

  bookmarks_[i]->title_ = bookmark->title_;
  bookmarks_[i]->url_   = bookmark->url_;

  QModelIndex start = index(i, 0);
  QModelIndex end = index(i, 0);
  emit dataChanged(start, end);

  // need to update children too
  int64 fid = bookmarks_[i]->folder_id_;
  if (-1 != fid 
     && BookmarkList::index(bookmarks_, fid, i)
     && BookmarkList::index(bookmarks_[i]->children_, id, j)) {
    bookmarks_[i]->children_[j]->title_ = bookmark->title_;
    bookmarks_[i]->children_[j]->url_   = bookmark->url_;
  }

  return true;
}

bool BookmarkQtTreeImpl::removeBookmark(const BookmarkNode *node) {
  // 1. remove in children
  int i, j;
  // 1.1. remove bar's child
  if (BookmarkList::index(bookmarks_[0]->children_, node->id(), i))  
  {
    DLOG(INFO)<<"hdq removing child by tree item "<<node->GetTitle()<< " id "<<node->id();
    DLOG(INFO)<<"hdq remove in bar "<<i;
    bookmarks_[0]->children_.removeAt(i);
  }

  // 1.2. remove other's child
  if (BookmarkList::index(bookmarks_, BookmarkList::other_node_id, i))
  {
    if (BookmarkList::index(bookmarks_[i]->children_, node->id(), j))
    {
      DLOG(INFO)<<"hdq removing child by tree item "<<node->GetTitle()<< " id "<<node->id();
      DLOG(INFO)<<"hdq remove in other "<<j;
      bookmarks_[i]->children_.removeAt(j);
    }
  }

  // 2. remove in tree
  return BookmarkQtImpl::removeBookmark(node);
}

////////////////////////////////////////////////////////////////////////////////
// BookmarkQtFilterProxyModel

BookmarkQtFilterProxyModel::BookmarkQtFilterProxyModel(BookmarkQtImpl *impl, QObject *parent)
  : impl_(impl), QSortFilterProxyModel(parent)
{
  setSourceModel(impl_);
  setFilterRole(BookmarkQtImpl::TitleRole);
  setFilterCaseSensitivity(Qt::CaseInsensitive);
  setDynamicSortFilter(true);
}

void BookmarkQtFilterProxyModel::openBookmarkItem(QString id) { 
  DLOG(INFO)<<"hdq"<<__PRETTY_FUNCTION__<<" will open bookmark "<<id.toStdString()
    <<" impl idx "<<impl_->idx(id.toLong())<<" source idx "<<toSource(impl_->idx(id.toLong()));
  if (-1 == impl_->idx(id.toLong()) || 
      -1 == toSource(impl_->idx(id.toLong()))) return;
  impl_->openBookmarkItem(toSource(impl_->idx(id.toLong()))); 
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
//  AddItem(IDC_BOOKMARK_MOVETO, 
//          l10n_util::GetStringFUTF16(IDS_BOOKMARK_MANAGER_MOVETO,
//                                     anotherFolder.utf16()));
  AddItemWithStringId(IDC_BOOKMARK_OPEN,   IDS_BOOMARK_BAR_OPEN_IN_NEW_TAB);
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
//    case IDC_BOOKMARK_MOVETO: filter_->MoveToAnother(); break;
    case IDC_BOOKMARK_OPEN:   filter_->OpenItemInNewTab();   break;
    case IDC_BOOKMARK_EDIT:   filter_->EditItem();   break;
    case IDC_BOOKMARK_REMOVE: filter_->RemoveItem(); break;
    default:
      LOG(WARNING) << "Received Unimplemented Command: " << command_id;
      break;
  }
}

#include "moc_bookmark_qt.cc"
