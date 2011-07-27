// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Copyright (c) 2010, Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEEGOTOUCH_BOOKMARK_QT_H_
#define CHROME_BROWSER_MEEGOTOUCH_BOOKMARK_QT_H_
#pragma once

//#include <gtk/gtk.h>

#include <vector>
#include <QSortFilterProxyModel>

//#include "ui/base/gtk/gtk_signal.h"
#include "ui/base/animation/slide_animation.h"
#include "base/gtest_prod_util.h"
#include "base/scoped_ptr.h"
#include "chrome/browser/bookmarks/bookmark_context_menu_controller.h"
#include "chrome/browser/bookmarks/bookmark_model_observer.h"
//#include "chrome/browser/ui/gtk/bookmarks/bookmark_bar_instructions_gtk.h"
//#include "chrome/browser/ui/gtk/menu_bar_helper.h"
//#include "chrome/browser/ui/gtk/owned_widget_gtk.h"
//#include "chrome/browser/ui/gtk/view_id_util.h"
//#include "chrome/browser/sync/profile_sync_service.h"
#include "content/common/notification_observer.h"
#include "content/common/notification_registrar.h"
#include "ui/gfx/point.h"
#include "ui/gfx/size.h"

#include <QDeclarativeImageProvider>
#include "chrome/browser/bookmarks/bookmark_model.h"  // for historyservice

#include "chrome/app/chrome_dll_resource.h" 
#define IDC_BOOKMARK_EDIT   IDC_CONTENT_CONTEXT_CUSTOM_FIRST+1
#define IDC_BOOKMARK_REMOVE IDC_CONTENT_CONTEXT_CUSTOM_FIRST+2
#define IDC_BOOKMARK_OPEN   IDC_CONTENT_CONTEXT_CUSTOM_FIRST+3
//#define IDC_BOOKMARK_MOVETO IDC_CONTENT_CONTEXT_CUSTOM_FIRST+3

class BookmarkMenuController;
class Browser;
class BrowserWindowQt;
//class CustomContainerButton;
//class GtkThemeProvider;
//class MenuGtk;
class PageNavigator;
class Profile;
class TabstripOriginProvider;

class BookmarkItem;
//class BookmarkGridItem;
class BookmarkQtImpl;
class BookmarkBarQtImpl;
//class BookmarkQtGridImpl;
class BookmarkQtTreeImpl;
class BookmarkQtFilterProxyModel;
class BookmarkListMenuModel; 

class BookmarkListData {
public:
  BookmarkListData() {}
  ~BookmarkListData() {}

  QStringList all_folders_title_;
  QList<int64> all_folders_id_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkListData);
};

class BookmarkImageProvider : public QDeclarativeImageProvider
{
public:
  BookmarkImageProvider()
    : QDeclarativeImageProvider(QDeclarativeImageProvider::Image)
  {}

  // clear all images in image hashmap
  void clear() {
    imageList_.clear();
  }

  // overrided function, inherited from QDeclarativeImageProvider
  virtual QImage requestImage(const QString& path,
                              QSize* size,
                              const QSize& requestedSize) {
    QImage& image = imageList_[path];
    if (!image.isNull()) {
      *size = image.size();
      return image;
    }
    DLOG(INFO) << __PRETTY_FUNCTION__ << "Failed to find image path: " << path.toStdString();
    *size = QSize(0, 0);
    return QImage();
  }

  // add a new image TODO: not adding duplicate ones
  void addImage(QString path, const QImage &image) {
    imageList_.insert(path, image);
  }

private:
  QMap<QString, QImage> imageList_;
};

class BookmarkItem {

public:
  BookmarkItem(Browser* browser, QString& title, QString& url, int64 id, BookmarkNode::Type type)
    : browser_(browser), title_(title), url_(url), id_(id), type_(type),
      folder_id_(-1), level_(0), isOpened_(false)
  {}

  ~BookmarkItem() {}

  void increaseChildrenLevels() {
    foreach(BookmarkItem *item, children_) {
      item->level_ = level_ + 1;
      item->increaseChildrenLevels();
    }
  }
  inline bool hasChildren() const { return !children_.empty(); }
  int level_;
  bool isOpened_;
  QList<BookmarkItem*> children_; // only store pointers, actually pointing to the same ones in the list

  void title(QString t) { title_ = t; }
  void url(QString u) { url_ = u; }

  QString title() const { return title_; }
  QString url()   const { return url_; }
  QString image() const { return "image://bookmark_" + root_type_ + "/" + QString::number(id_); }
   
  BookmarkItem& operator=(const BookmarkItem& bookmark) {
    if (this == &bookmark)
      return *this;
    title_ = bookmark.title();
    url_ = bookmark.url();
    id_ = bookmark.id_;
    folder_id_ = bookmark.folder_id_;
    root_type_ = bookmark.root_type_;
    level_ = bookmark.level_;
    isOpened_ = bookmark.isOpened_;
    return *this;
  }
  
  bool operator==(const BookmarkItem& bookmark) const {
    return (id_ == bookmark.id_);
  }

  BookmarkNode::Type type_;
  QString root_type_;
  QString title_;
  QString url_;
  int64 id_;
  int64 folder_id_;

protected:
  Browser* browser_;
};

//class BookmarkGridItem : public BookmarkItem {
//
//public:
//  BookmarkGridItem(Browser* browser, BookmarkQtGridImpl* model, QString& title, QString& url, int64 id, BookmarkNode::Type type)
//    : BookmarkItem(browser, title, url, id, type),
//      model_(model)
//    {}
//  ~BookmarkGridItem() {}
//
//  void RequestImg(int index);
//  void HandleThumbnailData(scoped_refptr<RefCountedBytes> jpeg_data);
//  void OnThumbnailDataAvailable(HistoryService::Handle request_handle, scoped_refptr<RefCountedBytes> jpeg_data);
//
//protected:
//  CancelableRequestConsumer consumer_;
//  BookmarkQtGridImpl* model_;
//  int index_;
//};
//
class BookmarkQt : //public AnimationDelegate,
                   //public ProfileSyncServiceObserver,
                   //public ui::AcceleratorProvider,
                   public BookmarkModelObserver
{
public:
  explicit BookmarkQt(BrowserWindowQt* window,
                      Profile* profile, Browser* browser, BookmarkListData* data,
                      const QString& anotherFolder);
  virtual ~BookmarkQt();

//  // Overridden from ui::AcceleratorProvider:
//  virtual bool GetAcceleratorForCommandId(int id,
//                                        ui::Accelerator* accelerator);
//
  // Invoked when the bookmark model has finished loading. Creates a button
  // for each of the children of the root node from the model.
  virtual void Loaded(BookmarkModel* model);

  virtual const BookmarkNode* GetParent() = 0;
  bool IsMyParent(const BookmarkNode* parent);

  // Resets the profile. This removes any buttons for the current profile and
  // recreates the models.
  void SetProfile(Profile* profile);

  // Returns the current profile.
  Profile* GetProfile() { return profile_; }

  // Returns the current browser.
  Browser* browser() const { return browser_; }

  // Sets the PageNavigator that is used when the user selects an entry on
  // the bookmark bar.
  void SetPageNavigator(PageNavigator* navigator);

  // Create the contents of the bookmark bar.
  void Init(Profile* profile);

  // Get the current height of the bookmark bar.
  int GetHeight();

  void titleChanged(QString id, QString title);
  void urlChanged(QString id, QString url);

  void GetBookmarkProperties(const BookmarkNode* node, QString& title, QString &url, int64& id, BookmarkNode::Type& type);

  virtual void BookmarkModelBeingDeleted(BookmarkModel* model);
  virtual void BookmarkNodeAdded(BookmarkModel* model,
                                 const BookmarkNode* parent, int index);
  virtual void BookmarkNodeMoved(BookmarkModel* model,
                                 const BookmarkNode* old_parent, int old_index,
                                 const BookmarkNode* new_parent, int new_index);
  virtual void BookmarkNodeRemoved(BookmarkModel* model,
                                 const BookmarkNode* parent, int old_index,
                                 const BookmarkNode* node);
  virtual void BookmarkNodeChanged(BookmarkModel* model,
                                 const BookmarkNode* node);
  virtual void BookmarkNodeChildrenReordered(BookmarkModel* model,
                                 const BookmarkNode* node);

  // Invoked when a favicon has finished loading.
  virtual void BookmarkNodeFaviconLoaded(BookmarkModel* model,
                                         const BookmarkNode* node);

  BookmarkItem* CreateBookmarkItem(const BookmarkNode* node);
//  virtual BookmarkGridItem* CreateBookmarkGridItem(const BookmarkNode* node) = 0;

  void openBookmarkItem(int index); 
  void openBookmarkItem(QString id);
  void openBookmarkItem(int64 folder_id, int index);
  void removeBookmarkInModel(int index);
  void removeBookmarkInModel(QString id);
  void removeBookmarkInModel(int64 folder_id, int index);
  void MoveToAnotherFolder(int index);

  void moveBookmarkInModel(const BookmarkNode *old_parent, 
                           const BookmarkNode *new_parent, int from, int to);
  void moveBookmarkInModel(int from, int to);
  bool moveBookmarkInList(QString from, QString to, QList<BookmarkItem*> &bookmarks, bool directed=false, bool up=false);

  void HideBookmarkManager();
  void PopupMenu(gfx::Point p); 
 
  void CreateAllBookmarkListItems();
  void RemoveAllBookmarkListItems();

  BookmarkListData* data_;

protected:
  Profile* profile_;

  // Used for opening urls.
  PageNavigator* page_navigator_;

  Browser* browser_;
  BrowserWindowQt* window_;

  // Model providing details as to the starred entries/groups that should be
  // shown. This is owned by the Profile.
  BookmarkModel* model_;

//  BookmarkQtGridImpl* grid_impl_;
  BookmarkQtTreeImpl* tree_impl_; 
//  BookmarkQtFilterProxyModel* grid_filter_;
  BookmarkQtFilterProxyModel* tree_filter_;
  BookmarkListMenuModel* bookmark_menu_;

  QString another_folder_name_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkQt);
};

class BookmarkOthersQt : public BookmarkQt
{
public:
  explicit BookmarkOthersQt(BrowserWindowQt* window,
                            Profile* profile, Browser* browser, BookmarkListData* data);
  virtual ~BookmarkOthersQt();
  virtual const BookmarkNode* GetParent();

  void Init(Profile* profile);
//  BookmarkGridItem* CreateBookmarkGridItem(const BookmarkNode* node);

  DISALLOW_COPY_AND_ASSIGN(BookmarkOthersQt);
};

class BookmarkBarQt : public BookmarkQt,
                      public NotificationObserver
{
public:
  explicit BookmarkBarQt(BrowserWindowQt* window, Profile* profile, Browser* browser, BookmarkListData* data);
  virtual ~BookmarkBarQt();

  virtual const BookmarkNode* GetParent();

  // Whether the current page is the New Tag Page (which requires different
  // rendering).
  bool OnNewTabPage();

  // Returns true if the bookmarks bar preference is set to 'always show'.
  bool IsAlwaysShown();

  // Returns false if the bookmarks bar is empty, without any bookmarkitem.
  bool IsExistBookmarks();

  //notify to show or hide bookmarkbar
  void NotifyToMayShowBookmarkBar(const bool always_show);

  void Init(Profile* profile, BookmarkOthersQt* others);

  void ShowBookmarkManager();
private:
  void ShowInstruction();
  // Helper function which generates GtkToolItems for |bookmark_toolbar_|.
  void CreateAllBookmarkButtons();

//  BookmarkGridItem* CreateBookmarkGridItem(const BookmarkNode* node);
  void CreateAllBookmarkTreeItems();
  void RemoveAllBookmarkTreeItems();

  // Helper function which destroys all the bookmark buttons in the GtkToolbar.
  void RemoveAllBookmarkButtons();

  //TODO
  //void ConfigureButtonForNode(const BookmarkNode* node, BookmarkItem* item);
  // Returns the number of buttons corresponding to starred urls/groups. This
  // is equivalent to the number of children the bookmark bar node from the
  // bookmark bar model has.
  int GetBookmarkButtonCount();

  // Set the appearance of the overflow button appropriately (either chromium
  // style or GTK style).
  //void SetOverflowButtonAppearance();
  // Finds the size of the current tab contents, if it exists and sets |size|
  // to the correct value. Returns false if there isn't a TabContents, a
  // condition that can happen during testing.
  bool GetTabContentsSize(gfx::Size* size);

  // Invoked when the bookmark model has finished loading. Creates a button
  // for each of the children of the root node from the model.
  virtual void Loaded(BookmarkModel* model);

  virtual void BookmarkNodeAdded(BookmarkModel* model,
                                 const BookmarkNode* parent,
                                 int index);
  virtual void BookmarkNodeRemoved(BookmarkModel* model,
                                   const BookmarkNode* parent,
                                   int old_index,
                                   const BookmarkNode* node);
  virtual void BookmarkNodeChanged(BookmarkModel* model,
                                   const BookmarkNode* node);
  virtual void BookmarkNodeChildrenReordered(BookmarkModel* model,
                                             const BookmarkNode* node);
//  //Overridden from NotificationObserver:
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);

  // Finds the BookmarkNode from the model associated with |button|.
//  const BookmarkNode* GetNodeForToolButton(MWidget* button);

  // ProfileSyncServiceObserver method.
  //virtual void OnStateChanged();

  BookmarkBarQtImpl* toolbar_impl_; // to display bookmarks bar in toolbar

  BookmarkQtTreeImpl*         all_trees_impl_;   // to display all bookmarks and folders as a tree
  BookmarkQtFilterProxyModel* all_trees_filter_; //

  // Using this to tell "others folder" to display on show
  BookmarkOthersQt* others_;

  void CreateTreeFolder(const BookmarkNode* node);

  // A pointer to the ProfileSyncService instance if one exists.
  //ProfileSyncService* sync_service_;

  NotificationRegistrar registrar_;
  DISALLOW_COPY_AND_ASSIGN(BookmarkBarQt);
};

class BookmarkQtImpl : public QAbstractListModel
{
  Q_OBJECT
public:
  enum BookmarkRoles {
    TitleRole = Qt::UserRole + 1,
    UrlRole,
    TypeRole,
    LengthRole,
    IdRole,
    FolderNameRole,
    LevelRole,
    IsOpenedRole,
    HasChildrenRole
  };
  BookmarkQtImpl(BookmarkQt* bookmark_qt, QObject *parent = 0);

//  void GetBookmarkProperties(const BookmarkNode* node, QString& title, QString &url, QString &id);

  bool addBookmark(BookmarkItem *bookmark);
  bool addBookmark(BookmarkItem *bookmark, int index);

  bool removeBookmark(int index);
  bool removeBookmark(BookmarkItem *bookmark);
  bool removeBookmark(const BookmarkNode *node);

  //bool updateBookmark(BookmarkItem *bookmark, int index);
  bool updateBookmark(int i, QString t, QString u, int64 is, BookmarkNode::Type type);

  void clear();

  int idx(int64 id);
  int rowCount(const QModelIndex& parent = QModelIndex()) const;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  virtual void PopupMenu(int x, int y) {}

  void Show();
  void Hide();

  QList<BookmarkItem*> bookmarks_;

Q_SIGNALS:
  void show();
  void hide();

public Q_SLOTS:
  virtual void openBookmarkItem(QString id);
  void backButtonTapped(); // back button in Bookmark Manager

  virtual void remove(QString id) {}
  virtual int64 id(int index) { return -1; }
  virtual void premove() {}
  virtual void moving(int from, int to) {}
  virtual void moveDone(int from, int to) {}
  virtual void moveDone(int f, int t, QString from, QString to) {}

  virtual void titleChanged(QString id, QString title) {}
  virtual void urlChanged(QString id, QString url) {}
  virtual void moveToAnotherFolder(int idx) {}
  int level(int index) { return bookmarks_[index]->level_; }
  virtual void expand(int idx) {}
  virtual void collapse(int idx, bool checkopen = true){} 
  virtual void folderChanged(QString id, int folder_idx){ 
    DLOG(INFO)<<__PRETTY_FUNCTION__<<" hdq move "<<id.toStdString()<<" to folder "<<bookmark_qt_->data_->all_folders_id_[folder_idx];
    bookmark_qt_->moveBookmarkInList(id, QString::number(bookmark_qt_->data_->all_folders_id_[folder_idx]), bookmarks_);
  }

protected:
  BookmarkQt* bookmark_qt_;

private:
  Q_DISABLE_COPY(BookmarkQtImpl);
};

// Showing Bookmark Bar Items in Toolbar
class BookmarkBarQtImpl : public BookmarkQtImpl
{
  Q_OBJECT
public:
  BookmarkBarQtImpl(BookmarkQt* bookmark_qt, QObject *parent = 0);
  ~BookmarkBarQtImpl();

  void addInstruction();
  void removeInstruction();

Q_SIGNALS:
  void showInstruction();
  void hideInstruction();

private:
  Q_DISABLE_COPY(BookmarkBarQtImpl);
};

//// Showing Bookmark Items in Bookmark Manager in a grid view
//class BookmarkQtGridImpl : public BookmarkQtImpl
//{
//  Q_OBJECT
//public:
//  enum GridRoles {
//    ImageRole = HasChildrenRole + 1
//  };
//  BookmarkQtGridImpl(BookmarkQt* bookmark_qt, QObject *parent = 0);
//  virtual ~BookmarkQtGridImpl();
//  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
//
////  void beginReset();
////  void endReset();
//  void clear();
//
//  BookmarkImageProvider* Provider() { return &provider_; }
//
//  //void openBookmarkItem(int index);
//  void PopupMenu(int x, int y);
//
//public Q_SLOTS:
//  void remove(QString id);
//  int64 id(int index) { return bookmarks_[index]->id_; }
//  void moving(int from, int to);
//  void moveDone(int from, int to);
//
//  void titleChanged(QString id, QString title);
//  void urlChanged(QString id, QString url);
//  void moveToAnotherFolder(int index) { bookmark_qt_->MoveToAnotherFolder(index); }
//
//private:
//  BookmarkImageProvider provider_;
//  int returnedImages_;
//private:
//  Q_DISABLE_COPY(BookmarkQtGridImpl);
//};
//
// Showing all bookmarks and folders as a tree
//   Currently implemented as a fake tree 
//   - actually storing all first level items in a list (bookmarks_)
//   - as QML is not support tree view at the moment
class BookmarkQtTreeImpl : public BookmarkQtImpl
{
  Q_OBJECT
public:
  BookmarkQtTreeImpl(BookmarkQt* bookmark_qt, QObject *parent = 0);
  virtual ~BookmarkQtTreeImpl(); 
  //int rowCount(const QModelIndex &parent = QModelIndex()) const;

  bool removeBookmark(const BookmarkNode *node);

  void PopupMenu(int x, int y);

  bool addBookmarkToFolder(BookmarkItem *bookmark, const BookmarkNode* parent, int idx);
  bool updateBookmarkById(QString title, QString url, int64 id);
  bool dragging_;

public Q_SLOTS:
  void remove(QString id);
  int64 id(int index){ return bookmarks_[index]->id_; }
  void premove();
  void moving(int from, int to);
  // void moveDone(int from, int to);
  void moveDone(int f, int t, QString from, QString to);

  void titleChanged(QString id, QString title);
  void urlChanged(QString id, QString url);
  void moveToAnotherFolder(int index) { bookmark_qt_->MoveToAnotherFolder(index); }

  void expand(int numIndex);
  void collapse(int numIndex, bool checkopen = true);

private:
  bool up_; // the last moving direction
  QList<BookmarkItem*> memento_;
  Q_DISABLE_COPY(BookmarkQtTreeImpl);
};

// Filtering to support instant search
class BookmarkQtFilterProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
public:
  BookmarkQtFilterProxyModel(BookmarkQtImpl *impl, QObject *parent = 0);
  void Show() { emit show(); }
  void Hide() { emit hide(); }
  void CloseBookmarkManager(){ emit closeBookmarkManager(); }
//  void MoveToAnother() { emit moveToAnother(); }

  void OpenItemInNewTab() { 
    DLOG(INFO)<<"hdq"<<__PRETTY_FUNCTION__<<" emitting openiteminnewtab";
    emit openItemInNewTab(); }
  void EditItem() { emit editItem(); }
  void RemoveItem() { emit removeItem(); }
  virtual QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const ;

Q_SIGNALS:
  void show();
  void hide();
  void closeBookmarkManager();
//  void moveToAnother();
//
  void openItemInNewTab();
  void editItem();
  void removeItem();

public Q_SLOTS:

  void textChanged(QString text);

  int id(int idx)        { return impl_->id(toSource(idx)); }
  void remove(QString id)       { impl_->remove(id); }
  void premove() { impl_->premove(); }
  void moving(int from, int to) { impl_->moving(toSource(from), toSource(to)); }
  void moveDone(int from, int to){impl_->moveDone(toSource(from), toSource(to)); }
  void moveDone(int f, int t, QString from, QString to){impl_->moveDone(toSource(f), toSource(t), from, to); }

  void titleChanged(QString id, QString title) { impl_->titleChanged(id, title); }
  void urlChanged(QString id, QString url)     { impl_->urlChanged(id, url); }
  void openBookmarkItem(QString id);
  void backButtonTapped();
  void popupMenu(int x, int y);
  void moveToAnotherFolder(int idx) { impl_->moveToAnotherFolder(toSource(idx)); }
  int level(int idx) { return impl_->level(toSource(idx)); }

  void expand(int idx) { impl_->expand(toSource(idx)); }
  void collapse(int idx){ impl_->collapse(toSource(idx)); } 

  void folderChanged(QString id, int folder_idx) { impl_->folderChanged(id, folder_idx); }

private:
  //BookmarkQtGridImpl *impl_;
  BookmarkQtImpl *impl_;
  QString keyWord_;

  // Helper function to convert filter's index to source's index
  int toSource(int idx) { return mapToSource(index(idx, 0)).row(); }
};

class BookmarkListMenuModel : public ui::SimpleMenuModel,
                              public ui::SimpleMenuModel::Delegate {
public:
  BookmarkListMenuModel(//BookmarkQtFilterProxyModel *gfilter,
                        BookmarkQtFilterProxyModel *tfilter); //ui::AcceleratorProvider* provider, Browser* browser);
  virtual ~BookmarkListMenuModel(){};
  void Build(const QString& anotherFolder);

  // Overridden from ui::SimpleMenuModel::Delegate:
  virtual bool IsCommandIdChecked(int command_id) const;
  virtual bool IsCommandIdEnabled(int command_id) const;
  virtual bool GetAcceleratorForCommandId(int command_id,
                                          ui::Accelerator* accelerator);
  virtual void ExecuteCommand(int command_id);

private:
  //BookmarkQtFilterProxyModel* gfilter_;
  BookmarkQtFilterProxyModel* tfilter_;
//  ui::AcceleratorProvider* provider_;  // weak
//  Browser* browser_;  // weak
//
  DISALLOW_COPY_AND_ASSIGN(BookmarkListMenuModel);
};

#endif  // CHROME_BROWSER_MEEGOTOUCH_BOOKMARK_QT_H_
