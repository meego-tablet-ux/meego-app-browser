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
#define IDC_BOOKMARK_MOVETO IDC_CONTENT_CONTEXT_CUSTOM_FIRST+3

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
class BookmarkListItem;
class BookmarkQtImpl;
class BookmarkBarQtImpl;
class BookmarkQtListImpl;
class BookmarkQtFilterProxyModel;
class BookmarkListMenuModel; 

class BookmarkImageProvider : public QDeclarativeImageProvider
{
public:
  BookmarkImageProvider()
    : QDeclarativeImageProvider(QDeclarativeImageProvider::Image)
  {}

  // clear all images in image hashmap
  void clear()
  {
    imageList_.clear();
  }

  // overrided function, inherited from QDeclarativeImageProvider
  virtual QImage requestImage(const QString& id,
                              QSize* size,
                              const QSize& requestedSize)
  {
    QImage& image = imageList_[id];
    if (!image.isNull()) {
      *size = image.size();
      return image;
    }
    DLOG(INFO) << __PRETTY_FUNCTION__ << "Failed to find image id: " << id.toStdString();
    *size = QSize(0, 0);
    return QImage();
  }

  // add a new image TODO: not adding duplicate ones
  void addImage(const QString& id, const QImage &image)
  {
    imageList_.insert(id, image);
  }

private:
  QMap<QString, QImage> imageList_;
};

class BookmarkItem {

public:
  BookmarkItem(Browser* browser, QString& title, QString& url, QString& id)
    : browser_(browser), 
      title_(title), url_(url), id_(id) 
  {}

  ~BookmarkItem() {}

  void type(QString t) { type_ = t; }
  void title(QString t) { title_ = t; }
  void url(QString u) { url_ = u; }
  void id(QString i) { id_ = i; }

  QString type()  const { return type_; }
  QString title() const { return title_; }
  QString url()   const { return url_; }
  QString id()    const { return id_; }
  QString image() const { return "image://bookmark_" + type_ + "/" + id(); }
   
  BookmarkItem& operator=(const BookmarkItem& bookmark) {
    if (this == &bookmark)
      return *this;
    title_ = bookmark.title();
    url_ = bookmark.url();
    id_ = bookmark.id();
    return *this;
  }
  
  bool operator==(const BookmarkItem& bookmark) const {
    return (title_ == bookmark.title() && url_ == bookmark.url());
  }

protected:
  Browser* browser_;
  QString type_;
  QString title_;
  QString url_;
  QString id_;
};

class BookmarkListItem : public BookmarkItem {

public:
  BookmarkListItem(Browser* browser, BookmarkQtListImpl* model, QString& title, QString& url, QString& id)
    : BookmarkItem(browser, title, url, id),
      model_(model)
    {}

  ~BookmarkListItem() {}

  void RequestImg(int index);
  void OnThumbnailDataAvailable(HistoryService::Handle request_handle, scoped_refptr<RefCountedBytes> jpeg_data);

protected:
  CancelableRequestConsumer consumer_;
  BookmarkQtListImpl* model_;
  int index_;
};

class BookmarkQt : //public AnimationDelegate,
                   //public ProfileSyncServiceObserver,
                   //public ui::AcceleratorProvider,
                   public BookmarkModelObserver
{
public:
  explicit BookmarkQt(BrowserWindowQt* window,
                      Profile* profile, Browser* browser,
                      const QString& anotherFolder);
  virtual ~BookmarkQt();

//  // Overridden from ui::AcceleratorProvider:
//  virtual bool GetAcceleratorForCommandId(int id,
//                                        ui::Accelerator* accelerator);
//
  // Invoked when the bookmark model has finished loading. Creates a button
  // for each of the children of the root node from the model.
  virtual void Loaded(BookmarkModel* model);

  void ExposeModel(QString expose_name);

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

  void GetBookmarkProperties(const BookmarkNode* node, QString& title, QString &url, QString &id);

  virtual void BookmarkModelBeingDeleted(BookmarkModel* model);
  virtual void BookmarkNodeAdded(BookmarkModel* model,
                                 const BookmarkNode* parent,
                                 int index);
  virtual void BookmarkNodeRemoved(BookmarkModel* model,
                                   const BookmarkNode* parent,
                                   int old_index,
                                   const BookmarkNode* node);
  virtual void BookmarkNodeMoved(BookmarkModel* model,
                                 const BookmarkNode* old_parent,
                                 int old_index,
                                 const BookmarkNode* new_parent,
                                 int new_index);
  virtual void BookmarkNodeChanged(BookmarkModel* model,
                                   const BookmarkNode* node);
  virtual void BookmarkNodeChildrenReordered(BookmarkModel* model,
                                             const BookmarkNode* node);

  // Invoked when a favicon has finished loading.
  virtual void BookmarkNodeFaviconLoaded(BookmarkModel* model,
                                         const BookmarkNode* node);

  BookmarkItem* CreateBookmarkItem(const BookmarkNode* node);
  virtual BookmarkListItem* CreateBookmarkListItem(const BookmarkNode* node) = 0;

  void openBookmarkItem(const int index); 
  void removeBookmarkInModel(const int index);
  void moveBookmarkInModel(int from, int to);
  void MoveToAnotherFolder(const int index);

  // Display or hide Bookmark Manager.
  void ShowBookmarkManager(const bool show);

  void PopupMenu(gfx::Point p); 

protected:
  Profile* profile_;

  // Used for opening urls.
  PageNavigator* page_navigator_;

  Browser* browser_;
  BrowserWindowQt* window_;

  // Model providing details as to the starred entries/groups that should be
  // shown. This is owned by the Profile.
  BookmarkModel* model_;

  BookmarkQtListImpl* list_impl_;
  BookmarkQtFilterProxyModel* filter_;

  void CreateAllBookmarkListItems();
  void RemoveAllBookmarkListItems();

  QString another_folder_name_;
  BookmarkListMenuModel* bookmark_menu_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkQt);
};

class BookmarkBarQt : public BookmarkQt,
                      public NotificationObserver
{
public:
  explicit BookmarkBarQt(BrowserWindowQt* window, Profile* profile, Browser* browser);
  virtual ~BookmarkBarQt();

  virtual const BookmarkNode* GetParent();
  virtual BookmarkListItem* CreateBookmarkListItem(const BookmarkNode* node);

  // Whether the current page is the New Tag Page (which requires different
  // rendering).
  bool OnNewTabPage();

  // Returns true if the bookmarks bar preference is set to 'always show'.
  bool IsAlwaysShown();

  // Returns false if the bookmarks bar is empty, without any bookmarkitem.
  bool IsExistBookmarks();

  //notify to show or hide bookmarkbar
  void NotifyToMayShowBookmarkBar(const bool always_show);

  void Init(Profile* profile);

private:
  void ShowInstruction();
  // Helper function which generates GtkToolItems for |bookmark_toolbar_|.
  void CreateAllBookmarkButtons();

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

  // to display bookmarks bar in toolbar
  BookmarkBarQtImpl* toolbar_impl_;

  // A pointer to the ProfileSyncService instance if one exists.
  //ProfileSyncService* sync_service_;

  NotificationRegistrar registrar_;
  DISALLOW_COPY_AND_ASSIGN(BookmarkBarQt);
};

class BookmarkOthersQt : public BookmarkQt
{
public:
  explicit BookmarkOthersQt(BrowserWindowQt* window,
                            Profile* profile, Browser* browser);
  virtual ~BookmarkOthersQt();
  virtual const BookmarkNode* GetParent();
  virtual BookmarkListItem* CreateBookmarkListItem(const BookmarkNode* node);

  void Init(Profile* profile);

  DISALLOW_COPY_AND_ASSIGN(BookmarkOthersQt);
};

class BookmarkQtImpl : public QAbstractListModel
{
  Q_OBJECT
public:
  enum BookmarkRoles {
    TitleRole = Qt::UserRole + 1,
    UrlRole
  };
  BookmarkQtImpl(BookmarkQt* bookmark_qt, QObject *parent = 0);

  void GetBookmarkProperties(const BookmarkNode* node, QString& title, QString &url, QString &id);

  bool addBookmark(const BookmarkItem &bookmark);
  bool addBookmark(const BookmarkItem &bookmark, int index);

  bool removeBookmark(const BookmarkItem &bookmark);
  bool removeBookmark(int index);
  bool removeBookmark(const BookmarkNode *node);

  bool updateBookmark(const BookmarkItem &bookmark, const int index);
  bool updateBookmark(const int index, QString title, QString url, QString id);

  void clear();

  int idx(QString id);
  int rowCount(const QModelIndex& parent = QModelIndex()) const;
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

  void Show();
  void Hide();

Q_SIGNALS:
  void show();
  void hide();

public Q_SLOTS:
  void openBookmarkItem(const int index);
  void backButtonTapped(); // back button in Bookmark Manager

protected:
  BookmarkQt* bookmark_qt_;
  QList<BookmarkItem> bookmarks_;
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
};

// Showing Bookmark Items in Bookmark Manager
class BookmarkQtListImpl : public BookmarkQtImpl
{
  Q_OBJECT
public:
  enum ListRoles {
    ImageRole = UrlRole + 1,
    IdRole
  };
  BookmarkQtListImpl(BookmarkQt* bookmark_qt, QObject *parent = 0);
  virtual ~BookmarkQtListImpl();
  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

  void openBookmarkItem(const int index);
  void MoveToAnotherFolder(const int index) { bookmark_qt_->MoveToAnotherFolder(index); }

  void beginReset();
  void endReset();
  void clear();

  BookmarkImageProvider* Provider() { return &provider_; }

  void PopupMenu(int x, int y);

public Q_SLOTS:
  void remove(int index);
  int id(int index);
  void moving(int from, int to);
  void moveDone(int from, int to);

  void titleChanged(QString id, QString title);
  void urlChanged(QString id, QString url);
  void clicked(QString id) { openBookmarkItem(idx(id)); }

private:
  BookmarkImageProvider provider_;
  int returnedImages_;
};

class BookmarkQtFilterProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT
public:
  BookmarkQtFilterProxyModel(BookmarkQtListImpl *impl, QObject *parent = 0);
  void Show() { emit show(); }
  void Hide() { emit hide(); }
  void OpenBookmarkManager() { emit openBookmarkManager(); }
  void CloseBookmarkManager(){ emit closeBookmarkManager(); }
  void MoveToAnother() { emit moveToAnother(); }

  void EditItem() { emit editItem(); }
  void RemoveItem() { emit removeItem(); }

Q_SIGNALS:
  void show();
  void hide();
  void openBookmarkManager();
  void closeBookmarkManager();
  void moveToAnother();

  void editItem();
  void removeItem();

public Q_SLOTS:

  void textChanged(QString text);

  int  id(int idx)       { return impl_->id(toSource(idx)); }
  void remove(int idx)          { impl_->remove(toSource(idx)); }
  void moving(int from, int to) { impl_->moving(toSource(from), toSource(to)); }
  void moveDone(int from, int to){impl_->moveDone(toSource(from), toSource(to)); }

  void titleChanged(QString id, QString title) { impl_->titleChanged(id, title); }
  void urlChanged(QString id, QString url)     { impl_->urlChanged(id, url); }
  void openBookmarkItem(QString id);
  void backButtonTapped();
  void popupMenu(int x, int y);
  void moveToAnotherFolder(int idx) { impl_->MoveToAnotherFolder(idx); }

private:
  BookmarkQtListImpl *impl_;

  // Helper function to convert filter's index to source's index
  int toSource(int idx) { return mapToSource(index(idx, 0)).row(); }
};

class BookmarkListMenuModel : public ui::SimpleMenuModel,
                              public ui::SimpleMenuModel::Delegate {
 public:
  BookmarkListMenuModel(BookmarkQtFilterProxyModel *filter); //ui::AcceleratorProvider* provider, Browser* browser);
  virtual ~BookmarkListMenuModel(){};
  void Build(const QString& anotherFolder);

  // Overridden from ui::SimpleMenuModel::Delegate:
  virtual bool IsCommandIdChecked(int command_id) const;
  virtual bool IsCommandIdEnabled(int command_id) const;
  virtual bool GetAcceleratorForCommandId(int command_id,
                                          ui::Accelerator* accelerator);
  virtual void ExecuteCommand(int command_id);

 private:
  BookmarkQtFilterProxyModel* filter_;
//  ui::AcceleratorProvider* provider_;  // weak
//  Browser* browser_;  // weak
//
  DISALLOW_COPY_AND_ASSIGN(BookmarkListMenuModel);
};

#endif  // CHROME_BROWSER_MEEGOTOUCH_BOOKMARK_QT_H_
