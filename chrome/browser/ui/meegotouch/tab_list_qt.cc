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

#include "chrome/browser/ui/meegotouch/tab_list_qt.h"

#include "base/command_line.h"
#include "base/string_number_conversions.h"
#include "base/utf_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/history/history.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"
#include "content/browser/site_instance.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "chrome/browser/tabs/tab_strip_model.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"
#include "chrome/browser/ui/tab_contents/tab_contents_wrapper.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/url_constants.h"
#include "grit/generated_resources.h"
#include "net/base/registry_controlled_domain.h"
#include "ui/base/l10n/l10n_util.h"

#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QVector>
#include <QList>
#include <QTimer>
#include <algorithm>

#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QString>
#include <QLatin1String>

static const int TABLET_TABS_LIMIT = 7;
static const int UNLIMIT_TABS = 65535;

static const int kThumbnailWidth = 212;
static const int kThumbnailHeight = 132;

static QImage SkBitmap2Image(const SkBitmap& bitmap)
{
  QImage image;
  
  QImage::Format format;
  switch (bitmap.getConfig()) {
    case SkBitmap::kARGB_8888_Config:
      format = QImage::Format_ARGB32_Premultiplied;
      break;
    default:
      format = QImage::Format_Invalid;
  }
  if (format != QImage::Format_Invalid) {
    bitmap.lockPixels();
    QImage img((const uchar*)bitmap.getPixels(),
               bitmap.width(), bitmap.height(),
               bitmap.rowBytes(), format);
    bitmap.unlockPixels();
    return img;
  }
  else
    return image;
}


TabItem::TabItem(TabContentsWrapper* tab_contents, TabListQt* tablist)
{
  tab_contents_ = tab_contents;
  tablist_ = tablist;

  QImage ret(kThumbnailWidth,kThumbnailHeight, QImage::Format_RGB32);
  ret.fill(0xFFFFFF);
  thumbnail_ = ret;

  update();
}

TabItem::~TabItem()
{
}

void TabItem::GetThumbnail()
{
  if (!tab_contents_->tab_contents()->GetURL().SchemeIs("chrome") &&
      !tab_contents_->tab_contents()->GetURL().SchemeIs("chrome-extension"))
  {
    history::TopSites* ts = tablist_->browser()->profile()->GetTopSites();
    if (ts) {
      scoped_refptr<RefCountedBytes> jpeg_data;
      ts->GetPageThumbnail(tab_contents_->tab_contents()->GetURL(), &jpeg_data);

      if (jpeg_data.get()) {
	std::vector<unsigned char> thumbnail_data;
	std::copy(jpeg_data->data.begin(), jpeg_data->data.end(),
		  std::back_inserter(thumbnail_data));
	QImage image = QImage::fromData(thumbnail_data.data(), thumbnail_data.size());
	thumbnail_ = image;
      }
    }
  }
  else
  {
    QImage image;
    if(tab_contents_->tab_contents()->is_loading())
      thumbnail_ = image;
    ThumbnailGenerator* generator = g_browser_process->GetThumbnailGenerator();
    DCHECK(generator);
    SkBitmap bitmap = generator->GetThumbnailForRenderer((RenderWidgetHost*)(\
	 tab_contents_->tab_contents()->render_view_host()));
    thumbnail_ = SkBitmap2Image(bitmap);
    //    tablist_->tabUpdated(this);
  }
  id_ = qrand();
  tablist_->tabUpdated(this);

}

void TabItem::OnThumbnailDataAvailable(
			      HistoryService::Handle request_handle,
			      scoped_refptr<RefCountedBytes> jpeg_data) {
  if (jpeg_data.get()) {
    std::vector<unsigned char> thumbnail_data;
    std::copy(jpeg_data->data.begin(), jpeg_data->data.end(),
	      std::back_inserter(thumbnail_data));
    QImage image = QImage::fromData(thumbnail_data.data(), thumbnail_data.size());
    thumbnail_ = image;
  }

  id_ = qrand();
  tablist_->tabUpdated(this);
}

void TabItem::update()
{
  std::wstring title_str = UTF16ToWide(tab_contents_->tab_contents()->GetTitle());
  title_ = QString::fromStdWString(title_str);

  GetThumbnail();
}


void TabListQt::CheckTabsLimit()
{
  TabStripModel* model = browser_->tabstrip_model();
  browser_->command_updater()->UpdateCommandEnabled(IDC_NEW_TAB, !model->IsReachTabsLimit());
  
  if (model->IsReachTabsLimit())
  {
    emit setNewTabEnabled(false);
  }
  else
  {
    emit setNewTabEnabled(true);
  }
}


TabListQt::TabListQt(Browser* browser, BrowserWindow* window):
    QDeclarativeImageProvider(QDeclarativeImageProvider::Image),
    browser_(browser),
    window_(window),
    is_shown_(false)
{
  QHash<int, QByteArray> roles;
  roles[ThumbnailRole] = "thumbnail";
  roles[TitleRole] = "title";
     
  setRoleNames(roles);

  QDeclarativeView* view = ((BrowserWindowQt*)window)->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();

  context->setContextProperty("tabSideBarModel", this);
  QString newtab = QString::fromStdWString(UTF16ToWide(l10n_util::GetStringUTF16(IDS_TAB_CXMENU_NEWTAB)));
  context->setContextProperty("newtabtitle", newtab);

  context->engine()->addImageProvider(QLatin1String("tabsidebar"), this);

  // init();
  // TabStripModel* model = browser->tabstrip_model();
  // model->AddObserver(this);
}

TabListQt::~TabListQt()
{
  browser_ = NULL;
  window_ = NULL;
}

QDeclarativeImageProvider::ImageType TabListQt::imageType() const
{
  return QDeclarativeImageProvider::Image;
}

QImage TabListQt::requestImage(const QString& id, QSize* size, const QSize& requestedSize)
{
  bool ok = false;

  QImage ret(kThumbnailWidth,kThumbnailHeight, QImage::Format_RGB32);
  ret.fill(0xFFFFFF);

  QStringList strList = id.split("_", QString::SkipEmptyParts);

  if (strList.size() < 2 ) {
    *size = ret.size();
    return ret;
  }

  int requestType = -1;   // -1 invalid, 0 for thumbnail and 1 for favicon
  if (id.startsWith("thumbnail_")) {
    requestType = 0;
  } else {
    *size = ret.size();
    return ret;
  }

  int index = strList.at(1).toInt(&ok);
  if (!ok) { 
    *size = ret.size(); 
    return ret;}

  if ( index < 0 || index > tabs_.count()) { *size = ret.size();  
    return ret;}
  
  if (requestType == 0)
    ret = tabs_[index]->Thumbnail();

  if (size) *size = ret.size();
  return ret;
}

// interface of QAbstractListModel
int TabListQt::rowCount(const QModelIndex & parent) const
{
  return tabs_.count();
}
   
QVariant TabListQt::data(const QModelIndex & index, int role) const
{
  if (index.row() < 0 || index.row() > tabs_.count())
    return QVariant();

  TabItem* item = tabs_[index.row()];
  if (role == TitleRole)
    return item->Title();
  else if (role == ThumbnailRole)
    return item->ThumbnailId();
  return QVariant();
}

void TabListQt::createContents()
{
  TabStripModel* tabs = browser_->tabstrip_model();
  int tab_count = tabs->count();

  for (int i=0; i< tab_count; i++)
  {
    TabContentsWrapper* tab_contents = tabs->GetTabContentsAt(i);
    if (tab_contents)
    {
      LOG(INFO) << "tab_contents = " << tab_contents->tab_contents()->GetTitle();
      insertTab(tab_contents);
    }
  }

  // check tabs limit
  CheckTabsLimit();

}

void TabListQt::insertTab(TabContentsWrapper* tab_contents)
{
  if(tab_contents->tab_contents()->GetURL().HostNoBrackets() != std::string("newtab"))
  {
    TabItem* item = new TabItem(tab_contents, this);
    tab_item_map_.insert(tab_contents, item);
    addTabItem(item);
  }
}

void TabListQt::addTabItem(TabItem* item)
{
  beginInsertRows(QModelIndex(), rowCount(), rowCount());
  tabs_ << item;
  endInsertRows();
}

void TabListQt::removeTab(TabContentsWrapper* tab_contents)
{
  TabItem* item = NULL;
  TabContentsToItemMap::iterator itr = tab_item_map_.find(tab_contents);
  if (itr != tab_item_map_.end())
    item = itr.value();
  if (!item)
    return;
  
  int index = tabs_.indexOf(item);
  
  tab_item_map_.remove(tab_contents);
  beginRemoveRows(QModelIndex(), index, index);
  tabs_.removeAt(index);
  endRemoveRows();
}

void TabListQt::ClearContents()
{
  if (tabs_.size() > 0)
  {
    beginRemoveRows(QModelIndex(), 0, tabs_.size()-1);
    for (int i=0; i<tabs_.size(); i++)
    {
      delete tabs_[i];
    }
    tabs_.clear();
    endRemoveRows();
  }
}

void TabListQt::Show()
{
  ClearContents();
  createContents();

  is_shown_ = true;

  TabStripModel* model = browser_->tabstrip_model();
  model->AddObserver(this);

  emit show();

  // set the selected tab
  TabItem* item = NULL;
  TabContentsWrapper* current = model->GetSelectedTabContents();
  TabContentsToItemMap::iterator itr = tab_item_map_.find(current);
  if (itr != tab_item_map_.end())
    item = itr.value();
  if (!item)
  {
    emit selectTab(-1);
    return;
  }
  
  int tab_index = tabs_.indexOf(item);
  emit selectTab(tab_index);

  CheckTabsLimit();

}

void TabListQt::Hide()
{
  ClearContents();
  is_shown_ = false;
  
  TabStripModel* model = browser_->tabstrip_model();
  model->RemoveObserver(this);

  emit hide();
}

void TabListQt::go(int index)
{
  if ((index < 0) || (index >= tabs_.size())) return;

  TabItem* item = tabs_[index];

  TabStripModel* model = browser_->tabstrip_model();
  TabContentsWrapper *tab_contents = item->GetTabContents();
  int index_ = model->GetIndexOfTabContents(tab_contents);
  LOG(INFO) << "TabListQt go " << index_;
  model->SelectTabContentsAt(index_, true);
  Hide();
}

void TabListQt::closeTab(int index)
{
  if ((index < 0) || (index >= tabs_.size())) return;

  bool _hide = true;
  TabStripModel* model = browser_->tabstrip_model();

  TabItem* item = tabs_[index];
  TabContentsWrapper *tab_contents = item->GetTabContents();
  int index_ = model->GetIndexOfTabContents(tab_contents);

  if ( model->selected_index() != index_ ) _hide = false;

  if (model->count() == 1) {
    // the last one
    model->delegate()->AddBlankTab(true);
  }

  model->CloseTabContentsAt(index_, TabStripModel::CLOSE_CREATE_HISTORICAL_TAB);

  if (_hide) Hide();
}

void TabListQt::newTab()
{
  TabStripModel* model = browser()->tabstrip_model();
  for(int i = 0; i < model->count(); i++)
  {
    if(model->ContainsIndex(i))
    {
      TabContents* contents = model->GetTabContentsAt(i)->tab_contents();
      if(contents->GetURL().HostNoBrackets() == "newtab")
      {
	model->SelectTabContentsAt(i, true);
	Hide();
	return;
      }
    }
  }
  model->delegate()->AddBlankTab(true);
  Hide();
}

void TabListQt::tabUpdated(TabItem* item)
{
    int tab_index = tabs_.indexOf(item);
    if (tab_index >= 0 && tab_index <tabs_.size())
    {
      QModelIndex start = index(tab_index,0);
      QModelIndex end = index(tab_index, 0);
      emit dataChanged(start, end);
    }
}

void TabListQt::TabInsertedAt(TabContentsWrapper* contents,
                             int index,
                             bool foreground)
{
  LOG(INFO)<<"TabInsertedAt " << contents << " " << index;
  if(contents->tab_contents()->GetURL().HostNoBrackets() != std::string("newtab"))
    insertTab(contents);

  CheckTabsLimit();
}

void TabListQt::TabDetachedAt(TabContentsWrapper* contents, int index)
{
  LOG(INFO)<<"TabDetachedAt " << contents << " " << index;
  removeTab(contents);

  CheckTabsLimit();
}

void TabListQt::TabSelectedAt(TabContentsWrapper* old_contents,
                             TabContentsWrapper* contents,
                             int index,
                             bool user_gesture)
{
  LOG(INFO)<<"TabSelectedAt " << old_contents << " " << contents << " " << index;

  TabItem* item = NULL;
  TabContentsToItemMap::iterator itr = tab_item_map_.find(contents);
  if (itr != tab_item_map_.end())
    item = itr.value();
  if (!item)
  {
    emit selectTab(-1);
    return;
  }
  
  int tab_index = tabs_.indexOf(item);
  emit selectTab(tab_index);
}

void TabListQt::TabMoved(TabContentsWrapper* contents,
                        int from_index,
                        int to_index)
{
  LOG(INFO)<<"TabMoved " << contents << " " << to_index;
}

void TabListQt::TabChangedAt(TabContentsWrapper* contents, int index,
                            TabChangeType change_type)
{
  DLOG(INFO)<<"TabChangedAt " << contents << " " << index << " " << change_type;
  if (change_type == TITLE_NOT_LOADING) {
    // We'll receive another notification of the change asynchronously.
    return;
  }

  TabItem* item = NULL;
  TabContentsToItemMap::iterator itr = tab_item_map_.find(contents);
  if (itr != tab_item_map_.end())
    item = itr.value();
  if (!item)
    return;
  
  item->update();

  CheckTabsLimit();
 
}

void TabListQt::TabReplacedAt(TabContentsWrapper* old_contents,
                             TabContentsWrapper* new_contents,
                             int index)
{
  LOG(INFO)<<"TabReplacedAt " << new_contents << " " << index;
  TabItem* item = NULL;

  TabContentsToItemMap::iterator itr = tab_item_map_.find(old_contents);
  if (itr != tab_item_map_.end())
    item = itr.value();

  if(!item)
  {
    return;
  }

  tab_item_map_.remove(old_contents);
  tab_item_map_.insert(new_contents, item);
  item->replaceTabContents(new_contents);
  
  item->update();

}

void TabListQt::TabMiniStateChanged(TabContentsWrapper* contents, int index)
{
  LOG(INFO)<<"TabMiniStateChanged " << contents << " " << index;
}

void TabListQt::TabBlockedStateChanged(TabContentsWrapper* contents,
                                      int index)
{
  LOG(INFO)<<"TabBlockedStateChanged " << contents << " " << index;
}

#include "moc_tab_list_qt.cc"
