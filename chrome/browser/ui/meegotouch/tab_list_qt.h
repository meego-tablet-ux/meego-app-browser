/*
 * Copyright (c) 2010, Intel Corporation. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above 
 * copyright notice, this list of conditions and the following disclaimer 
 * in the documentation and/or other materials provided with the 
 * distribution.
 *     * Neither the name of Intel Corporation nor the names of its 
 * contributors may be used to endorse or promote products derived from 
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef TAB_LIST_QT_H
#define TAB_LIST_QT_H

#include "chrome/browser/history/history.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/tabs/tab_strip_model.h"

#include <QList>
#include <QAbstractListModel>
#include <QDeclarativeImageProvider>


class TabListQt;
class TabContentsWrapper;
class Browser;
class BrowserWindow;


class TabItem
{
 public:
  TabItem(TabContentsWrapper* tab_contents, TabListQt* tablist);
  ~TabItem();

 public:
  TabContentsWrapper* GetTabContents() {return tab_contents_;}

  QImage Thumbnail() {return thumbnail_;}
  QString Title() { return title_; }
  int ThumbnailId() { return id_;}

  void GetThumbnail();
  void OnThumbnailDataAvailable(
			      HistoryService::Handle request_handle,
			      scoped_refptr<RefCountedBytes> jpeg_data);
  void update();  

  void replaceTabContents(TabContentsWrapper* new_tab_contents) { tab_contents_ = new_tab_contents;}
  
 private:
  TabContentsWrapper* tab_contents_;
  TabListQt* tablist_;
  QString title_;
  QImage thumbnail_;
  int id_;

  // For history requests.
  CancelableRequestConsumer consumer_;
};


class TabListQt :  public QAbstractListModel, 
  public TabStripModelObserver,
  public QDeclarativeImageProvider
{
  Q_OBJECT

 public:
  TabListQt(Browser* browser, BrowserWindow* window);
  ~TabListQt();

  enum TabRoles {
    TitleRole = Qt::UserRole + 1,
    ThumbnailRole
  };

  Browser* browser() {return browser_;}

  void Show();
  void Hide();

  bool isVisible() {return is_shown_;}

  // interface of QAbstractListModel
  int rowCount(const QModelIndex & parent = QModelIndex()) const;
  QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

  // interface of QDeclarativeImageProvider
  QDeclarativeImageProvider::ImageType imageType() const;
  virtual QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize);

  void tabUpdated(TabItem* item);

 public Q_SLOTS:
  void newTab();
  void closeTab(int index);
  void go(int index);
  void hideSideBar();

 Q_SIGNALS:
  void show();
  void hide();
  void setNewTabEnabled(bool enabled);
  void selectTab(int index);

 protected:
  // TabStripModelObserver implementation:
  virtual void TabInsertedAt(TabContentsWrapper* contents,
                             int index,
                             bool foreground);
  virtual void TabDetachedAt(TabContentsWrapper* contents, int index);
  virtual void TabSelectedAt(TabContentsWrapper* old_contents,
                             TabContentsWrapper* contents,
                             int index,
                             bool user_gesture);
  virtual void TabMoved(TabContentsWrapper* contents,
                        int from_index,
                        int to_index);
  virtual void TabChangedAt(TabContentsWrapper* contents, int index,
                            TabChangeType change_type);
  virtual void TabReplacedAt(TabContentsWrapper* old_contents,
                             TabContentsWrapper* new_contents,
                             int index);
  virtual void TabMiniStateChanged(TabContentsWrapper* contents, int index);
  virtual void TabBlockedStateChanged(TabContentsWrapper* contents,
                                      int index);
 private:
  void CheckTabsLimit();
  void createContents();
  void ClearContents();

  void addTabItem(TabItem* item);
  //  void removeTabItem(int index);

  void insertTab(TabContentsWrapper* tab_contents);
  void removeTab(TabContentsWrapper* tab_contents);

  
 private:
  Browser* browser_;
  BrowserWindow* window_;
  bool is_shown_;

  typedef QMap<TabContentsWrapper*, TabItem*> TabContentsToItemMap;
  TabContentsToItemMap tab_item_map_;
  QList<TabItem*> tabs_;
};

#endif
