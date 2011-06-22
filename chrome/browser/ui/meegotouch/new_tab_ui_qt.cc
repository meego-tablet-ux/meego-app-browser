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

#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "base/i18n/rtl.h"
#include "chrome/browser/history/history_types.h"
#include "chrome/browser/history/recent_and_bookmark_thumbnails_qt.h"
#include "chrome/browser/tabs/tab_strip_model.h"
#include "chrome/browser/prefs/pref_service.h"
#include "content/common/notification_observer.h"
#include "chrome/common/url_constants.h"
#include "chrome/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "grit/app_resources.h"
#include "grit/generated_resources.h"
#include "grit/locale_settings.h"
#include "new_tab_ui_qt.h"

#include <QDeclarativeImageProvider>
#include <QDeclarativeView>
#include <QUrl>

#define MOST_VISITED "mostvisited"
#define RECENTLY_CLOSED "recentlyclosed"
#define EMPTY_PAGE "emptypage"

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

class MostVisitedPage {
public:
  string16 title;

  GURL url;

  GURL thumbnail_url;

  GURL favicon_url;
};

class MaxViewImageProvider : public QDeclarativeImageProvider 
{
public:
  MaxViewImageProvider()
      : QDeclarativeImageProvider(QDeclarativeImageProvider::Image){
    blankImage_ = QImage(QSize(212,132), QImage::Format_RGB666);
    blankImage_.invertPixels(QImage::InvertRgb);
  };

  virtual ~MaxViewImageProvider() {
    clear();
  };

  void clear()
  {
    imageList_.clear();
    favList_.clear();
  }

  virtual QImage requestImage(const QString& id,
                              QSize* size,
                              const QSize& requestedSize)
  {
    bool isGetThumbnail = id.startsWith("thumbnail");
    int finded = id.indexOf("_");
    if(isGetThumbnail) {
      if (finded != -1) {
        int index = id.size() - finded - 1;
        //DLOG(INFO) <<"thumbnail query map id: " << id.right(index).toStdString();
        QImage& image = imageList_[id.right(index)];
        if (!image.isNull()) {
          *size = image.size();
          return image;
        }
      }
      *size = blankImage_.size();
      return blankImage_;
    } else {
      if (finded != -1) {
        int index = id.size() - finded - 1;
        //DLOG(INFO) <<"favicon query map id: " << id.right(index).toStdString();
        QImage& image = favList_[id.right(index)];
        if (!image.isNull()) {
          *size = image.size();
          return image;
        }
      }
      SkBitmap *default_favicon =
      ResourceBundle::GetSharedInstance().GetBitmapNamed(IDR_DEFAULT_FAVICON);
      return SkBitmap2Image(*default_favicon);
    }
  }

  void addImage(const QString& type, const QString& id, const QImage &image)
  {
    if(type.contains("thumbnail")) {
    DLOG(INFO) <<"add map id: " << id.toStdString();
      imageList_.insert(id, image);
    }else{
      favList_.insert(id,image);
    }
  }

private:
  QHash<QString, QImage> imageList_;
  QHash<QString, QImage> favList_;
  QImage blankImage_;
};

class ThumbnailEntry
{
public:
  ThumbnailEntry(MaxViewImageProvider* imageProvider,
                 GURL url, Profile* profile, MaxViewModel* model)
                 : url_(url), imageProvider_(imageProvider), model_(model) {
    if(url != GURL(EMPTY_PAGE)) {
      //DLOG(INFO)<<__FUNCTION__;
      history::TopSites* ts = profile->GetTopSites();
      if (ts) {
         scoped_refptr<RefCountedBytes> thumbnail_data;
         ts->GetPageThumbnail(url, &thumbnail_data);
    	 if (thumbnail_data.get()) {
           handleThumbnailData(thumbnail_data);
           return;
         }
 
         history::RecentAndBookmarkThumbnailsQt * recentThumbnails =
                                       ts->GetRecentAndBookmarkThumbnails();
         if(recentThumbnails) {
           recentThumbnails->GetRecentPageThumbnail(url, &consumer_,
                           NewCallback(static_cast<ThumbnailEntry*>(this),
                           &ThumbnailEntry::onThumbnailDataAvailable));
         }
      }
    } else {
      //QImage image = QImage::fromColor();
      model_->beginReset();
      imageProvider_->addImage("thumbnail", QString::fromStdString(url_.spec()), QImage());
      model_->endReset();
    }

  };


  void onThumbnailDataAvailable(HistoryService::Handle request_handle,
                                scoped_refptr<RefCountedBytes> jpeg_data) {
        DLOG(INFO)<<__FUNCTION__<<"get thumbnail for "<< url_.spec();
        handleThumbnailData(jpeg_data);
  };

  void handleThumbnailData(scoped_refptr<RefCountedBytes> jpeg_data) {
    model_->beginReset();
    if (jpeg_data.get()) {
      std::vector<unsigned char> thumbnail_data;
      std::copy(jpeg_data->data.begin(), jpeg_data->data.end(),
      std::back_inserter(thumbnail_data));
      QImage image = QImage::fromData(thumbnail_data.data(), thumbnail_data.size());
      //DLOG(INFO) <<"thumbnail:";
      imageProvider_->addImage("thumbnail", QString::fromStdString(url_.spec()), image);
    }
    model_->endReset();
  }; 
private:

  GURL url_;

  MaxViewImageProvider* imageProvider_;

  MaxViewModel* model_;

  CancelableRequestConsumer consumer_;
};

class FaviconEntry 
{
public: 
  FaviconEntry(MaxViewImageProvider* imageProvider,
               GURL url, Profile* profile, MaxViewModel* model)
               : url_(url), imageProvider_(imageProvider), model_(model) {

    FaviconService* favicon_service = profile->GetFaviconService(Profile::EXPLICIT_ACCESS);
    if(favicon_service) {
      FaviconService::Handle handle;
      if(url != GURL(EMPTY_PAGE)) {
        handle = favicon_service->GetFaviconForURL(GURL(url), history::FAVICON, &consumer_,
                 NewCallback(this, &FaviconEntry::OnFaviconDataAvailable));
      } else {
        model_->beginReset();
        imageProvider_->addImage("favicon", QString::fromStdString(url_.spec()), QImage());
        model_->endReset();
      }
    }
  }; 

  void OnFaviconDataAvailable(FaviconService::Handle handle,
                              history::FaviconData favicon) {

    model_->beginReset();
    scoped_refptr<RefCountedMemory> data = favicon.image_data;
    if (data.get() && data->size()) {
      //scoped_refptr<RefCountedBytes> data_s = static_cast<scoped_refptr<RefCountedBytes>>data;
      std::vector<unsigned char> fav_data;
      std::copy(data->front(), data->front()+data->size(),
      std::back_inserter(fav_data));
      QImage image = QImage::fromData(fav_data.data(), fav_data.size());
      //DLOG(INFO) <<"favicon:";
      imageProvider_->addImage("favicon", QString::fromStdString(url_.spec()), image);
    }
    model_->endReset();

  }

private:
  GURL url_;

  MaxViewImageProvider* imageProvider_;

  MaxViewModel* model_;

  CancelableRequestConsumer consumer_;
};


NewTabUIQt::NewTabUIQt(Browser* browser, BrowserWindowQt* window)
  : isShowing_(true),
    browser_(browser),
    window_(window),
    pinned_urls_(NULL),
    tab_restore_service_(NULL) {
    TabStripModel* tabModel = browser_->tabstrip_model();
    tabModel->AddObserver(this);

    Profile* profile = browser_->profile();
    pinned_urls_ = profile->GetPrefs()->GetDictionary(prefs::kNTPMostVisitedPinnedURLs);

    impl_ = new NewTabUIQtImpl();
    mostVisitedModel_ = new MaxViewModel(this, NULL, QString(MOST_VISITED));
    recentlyClosedModel_ = new MaxViewModel(this, NULL, QString(RECENTLY_CLOSED));
    //Expand recently closed area defaully
    recentlyClosedModel_->setCollapsedState(MaxViewModel::LayoutList);
    recentlyClosedModel_->setCloseButtonState(false);

    QDeclarativeView* view = window_->DeclarativeView();
    QDeclarativeContext *context = view->rootContext();
    context->setContextProperty("browserNewTabObject", impl_);
    context->setContextProperty("browserMostVisitModel", mostVisitedModel_);
    context->setContextProperty("browserRecentlyClosedModel", recentlyClosedModel_);

    mostVisitedImageProvider_ = new MaxViewImageProvider();
    context->engine()->addImageProvider(QLatin1String(MOST_VISITED), mostVisitedImageProvider_);
    
    recentlyClosedImageProvider_ = new MaxViewImageProvider();
    context->engine()->addImageProvider(QLatin1String(RECENTLY_CLOSED), recentlyClosedImageProvider_);

}

NewTabUIQt:: ~NewTabUIQt() {
    TabStripModel* tabModel = browser_->tabstrip_model();
    tabModel->RemoveObserver(this);

    if (tab_restore_service_)
      tab_restore_service_->RemoveObserver(this);

    delete mostVisitedImageProvider_;
    delete recentlyClosedImageProvider_;

    delete impl_;
    delete mostVisitedModel_;
    delete recentlyClosedModel_;
}

void NewTabUIQt::AboutToShow() {
    DLOG(INFO)<<__FUNCTION__;
    //impl_->show();
    isAboutToShow_ = true;

    RegisterGetRecentlyClosedTab();
    //Call this function will trigger the show event after got the mostvisited data
    StartQueryForMostVisited();

    //Asumming is showing from now
    isShowing_ = true;
}

void NewTabUIQt::Hide() {
    DLOG(INFO)<<__FUNCTION__;

    isAboutToShow_ = false;
    impl_->hide();

    //TODO: Enable update new tab when browser is in using!
    //mostVisitedModel_->clear();
    //mostVisitedImageProvider_->clear();

    isShowing_ = false;
}

void NewTabUIQt::updateDataModel() {
    QDeclarativeView* view = window_->DeclarativeView();
    QDeclarativeContext *context = view->rootContext();
    context->setContextProperty("browserMostVisitModel", mostVisitedModel_);
    context->setContextProperty("browserRecentlyClosedModel", recentlyClosedModel_);

}

bool NewTabUIQt::isShowing() {
    return isShowing_;
}

void NewTabUIQt::TabInsertedAt(TabContentsWrapper* contents,
                             int index,
                             bool foreground) {
    handleTabStatusChanged();

}

void NewTabUIQt::TabSelectedAt(TabContentsWrapper* old_contents,
                             TabContentsWrapper* new_contents,
                             int index,
                             bool user_gesture) {
    handleTabStatusChanged();

}

void NewTabUIQt::TabChangedAt(TabContentsWrapper* contents, int index,
                            TabChangeType change_type) {
    handleTabStatusChanged();

}

void NewTabUIQt::handleTabStatusChanged() {
    if(shouldDisplay()) {
        if(!isShowing())
            AboutToShow();
    }else{ 
        if(isShowing())
            Hide();
    }
}

bool NewTabUIQt::shouldDisplay() {
    TabContents* contents = browser_->GetSelectedTabContents();
    if(contents == NULL)
        return false;
    if(contents->GetURL() == GURL(chrome::kChromeUINewTabURL)) {//.HostNoBrackets() == "newtab") {
        return true;
    }
    return false;
}

void NewTabUIQt::OnMostVisitedURLsAvailable(
                         const history::MostVisitedURLList &data) {
    //DLOG(INFO)<<__FUNCTION__;
    std::vector<MostVisitedPage*> pData;
    int size = data.size();
    for (size_t i = 0; i < size; i++) {
	const history::MostVisitedURL& mvp = data[i];	
        MostVisitedPage* page = new MostVisitedPage();
      	page->title = mvp.title;
      	page->url = mvp.url;
        pData.push_back(page);
    	//DLOG(INFO)<<__FUNCTION__<<page->title;
    }

    handleMostVisitedPageData(&pData);

    int count = pData.size();
    for(int i=0; i<count; i++) {
	MostVisitedPage* item = pData[i];
	delete item;
    }
    pData.clear();
    
}

void NewTabUIQt::handleMostVisitedPageData(std::vector<MostVisitedPage*>* data) {
    if(data==NULL)
        return;
    //DLOG(INFO)<<__FUNCTION__<<data->size();
/*
    int count = data->size();
    for(int i=count; i<8; i++) {
        DLOG(INFO)<<__FUNCTION__<<"compensation:" << i << "with null";
        PageUsageData* page = new PageUsageData(0);
        page->SetURL(GURL(EMPTY_PAGE));
        page->SetTitle(UTF8ToUTF16(""));
        data->push_back(page);
    }

    //Most Visited Area should not be null
    int q_count = data->size(); 
    if(q_count == 0) {
        PageUsageData* page = new PageUsageData(0);
        page->SetURL(GURL(EMPTY_PAGE));
        page->SetTitle(UTF8ToUTF16("Example"));
        data->push_back(page);
    }
*/

/*  
    // The sync work will be handled inside TopSites.
    std::vector<MostVisitedPage*> newData;
    syncWithPinnedPage(data, &newData);
*/

/*
    // Checking whether refresh display
    bool changed = false;
    int oldCount = mostVisitedModel_->rowCount();
    if(oldCount == 8) {
        for(int j=0; j<8; j++) {
            GURL new_url = newData[j]->url;
            GURL old_url = mostVisitedModel_->getItemURL(j);
            if(new_url != old_url) {
                changed = true;
                break;
            }
        }
    }else if( oldCount == 1 || oldCount != newData.size() ) {
        changed = true;
    }

    if(changed)
*/
        mostVisitedModel_->updateContent(data);

    if(isAboutToShow_) {
        impl_->show();
    }

    //updateDataModel();
/*
    //TODO: delete *data here!!!
    int count = newData.size();
    for(int i=0; i<count; i++) {
        MostVisitedPage* page = newData[i];
        delete page;
    }
    newData.clear();
*/
}

void NewTabUIQt::syncWithPinnedPage(std::vector<MostVisitedPage*>* data,
							    std::vector<MostVisitedPage*>* newData) {
  int data_index = 0;
  int output_index = 0;

  //Clean old data from pined list.
  for (DictionaryValue::key_iterator it = pinned_urls_->begin_keys();
      it != pinned_urls_->end_keys(); ++it) {
    Value* value;
    if (pinned_urls_->GetWithoutPathExpansion(*it, &value)) {
      if (!value->IsType(DictionaryValue::TYPE_DICTIONARY)) {
        // Moved on to TopSites and now going back.
//        pinned_urls_->Clear();
        DLOG(INFO)<<__FUNCTION__<<"Critical error!";
        return;
      }

      DictionaryValue* dict = static_cast<DictionaryValue*>(value);

      std::string tmp_string;
      GURL pinnedUrl;
      dict->GetString("url", &tmp_string);
      pinnedUrl = GURL(tmp_string);
      int find = false;
      int count = data->size();
      for(int i=0; i<count; i++) {
        const MostVisitedPage& page = *(*data)[i];
        if( pinnedUrl == page.url ) {
          find = true;
          break;
        }
      }
      if(!find) {
        DLOG(INFO)<<"ungot!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!";
        RemovePinnedURL(pinnedUrl);
      }
    }
  }

    
  //sync the query data with pinned list.
  while (output_index < kMostVisitedPages) {
    bool found = false;
    MostVisitedPage *mvp = new MostVisitedPage();
    
    //Find data from pinned list firstly
    if (GetPinnedURLAtIndex(output_index, mvp)) {
      DLOG(INFO)<<"got "<< mvp->title<<"from pinned list";
      found = true;
    }

    //Find data from query data secondly
    while (!found && data_index < data->size()) {
      const MostVisitedPage& page = *(*data)[data_index];
      data_index++;
      // filt out pinned URLs which will be check in first step
      std::string key = GetDictionaryKeyForURL(page.url.spec());
      if (pinned_urls_->HasKey(key))
        continue;

      mvp->url = page.url;
      mvp->title = page.title;
      //DLOG(INFO)<<"got "<< mvp->title<<"from top sites list";
      found = true;
    }

    //save the new data.
    if (found) {
      //if(pinned)
      //    newData.push_back(new PageUsageData((*data)[data_index]));
      //else {
      newData->push_back(mvp);
      //}
    }
    output_index++;
  }
}

void NewTabUIQt::HandleAddPinnedURL(MostVisitedPage* data, int index) {

  AddPinnedURL(*data, index);
}

void NewTabUIQt::AddPinnedURL(const MostVisitedPage& page, int index) {

  history::TopSites* ts = browser_->GetProfile()->GetTopSites();
  if (ts){
    ts->AddPinnedURL(page.url, index);
    return;
  }

/*
  // Remove any pinned URL at the given index.
  MostVisitedPage old_page;
  if (GetPinnedURLAtIndex(index, &old_page)) {
    RemovePinnedURL(old_page.url);
  }

  DictionaryValue* new_value = new DictionaryValue();
  new_value->SetString("title", page.title);
  new_value->SetString("url", page.url.spec());
  new_value->SetInteger("index", index);
  pinned_urls_->Set(GetDictionaryKeyForURL(page.url.spec()), new_value);
*/
}

bool NewTabUIQt::GetPinnedURLAtIndex(int index,
                                             MostVisitedPage* page) {
  // This iterates over all the pinned URLs. It might seem like it is worth
  // having a map from the index to the item but the number of items is limited
  // to the number of items the most visited section is showing on the NTP so
  // this will be fast enough for now.
  for (DictionaryValue::key_iterator it = pinned_urls_->begin_keys();
      it != pinned_urls_->end_keys(); ++it) {
    Value* value;
    if (pinned_urls_->GetWithoutPathExpansion(*it, &value)) {
      if (!value->IsType(DictionaryValue::TYPE_DICTIONARY)) {
        // Moved on to TopSites and now going back.
//        pinned_urls_->Clear();
        return false;
      }

      int dict_index;
      DictionaryValue* dict = static_cast<DictionaryValue*>(value);
      if (dict->GetInteger("index", &dict_index) && dict_index == index) {
        // The favicon and thumbnail URLs may be empty.
        std::string tmp_string;
        string16 tmp_string16;
        int id;
        dict->GetString("url", &tmp_string);
        page->url = GURL(tmp_string);
        dict->GetString("title", &tmp_string16);
        page->title = tmp_string16;
        return true;
      }
    } else {
      NOTREACHED() << "DictionaryValue iterators are filthy liars.";
    }
  }
  return false;
}

void NewTabUIQt::RemovePinnedURL(const GURL& url) {

  history::TopSites* ts = browser_->GetProfile()->GetTopSites();
  if (ts) {
    ts->RemovePinnedURL(url);
    return;
  }

/*
  const std::string key = GetDictionaryKeyForURL(url.spec());
  if (pinned_urls_->HasKey(key))
    pinned_urls_->Remove(key, NULL);
*/
}

std::string NewTabUIQt::GetDictionaryKeyForURL(const std::string& url) {
  return MD5String(url);
}

void NewTabUIQt::RegisterGetRecentlyClosedTab() {
  if (!tab_restore_service_) {
    tab_restore_service_ = browser_->profile()->GetTabRestoreService();
    if (tab_restore_service_) {
      tab_restore_service_->LoadTabsFromLastSession();

      tab_restore_service_->AddObserver(this);
    }
  }

  if (tab_restore_service_)
    TabRestoreServiceChanged(tab_restore_service_);
}

bool NewTabUIQt::TabToValue(
    const TabRestoreService::Tab& tab,
    MostVisitedPage* value) {
  if (tab.navigations.empty())
    return false;

  const TabNavigation& current_navigation = tab.navigations.at(tab.current_navigation_index);
  
  GURL gurl = current_navigation.virtual_url();
  if (gurl == GURL(chrome::kChromeUINewTabURL))
    return false;
  value->url = gurl;
  bool using_url_as_the_title = false;
  string16 title_to_set = current_navigation.title();
  if (title_to_set.empty()) {
    using_url_as_the_title = true;
    title_to_set = UTF8ToUTF16(gurl.spec());
  }
  value->title = title_to_set;

  return true;
}

bool NewTabUIQt::EnsureTabIsUnique(
    const MostVisitedPage* value,
    std::set<string16>* unique_items) {
    DCHECK(unique_items);
    string16 title = value->title;
    string16 url = UTF8ToUTF16(value->url.spec());
    string16 unique_key = title + url;
    if (unique_items->find(unique_key) != unique_items->end())
      return false;
    else
      unique_items->insert(unique_key);

    return true;
}

void NewTabUIQt::TabRestoreServiceChanged(TabRestoreService* service) {
  const TabRestoreService::Entries& entries = service->entries();
  std::vector<MostVisitedPage*> list_value;
  std::set<string16> unique_items;
  int added_count = 0;
  const int max_count = 10;
  for (TabRestoreService::Entries::const_iterator it = entries.begin();
       it != entries.end() && added_count < max_count; ++it) {
    TabRestoreService::Entry* entry = *it;
    MostVisitedPage* value = new MostVisitedPage();
    if ((entry->type == TabRestoreService::TAB &&
         TabToValue(*static_cast<TabRestoreService::Tab*>(entry), value) &&
         EnsureTabIsUnique(value, &unique_items))) {
      list_value.push_back(value);
      added_count++;
    } else {
      delete value;
    }
  }
  
  //notify QML update recent closed here!
  recentlyClosedModel_->updateContent(&list_value);

  for(int i=0; i<added_count; i++) {
    delete list_value[i];
  }
}

void NewTabUIQt::TabRestoreServiceDestroyed(TabRestoreService* service) {
  tab_restore_service_ = NULL;
}

//TODO: Call this function when new Tab is showed.
void NewTabUIQt::StartQueryForMostVisited() {
  history::TopSites* ts = browser_->GetProfile()->GetTopSites();
  if (ts) {
    ts->GetMostVisitedURLs(
         &topsites_consumer_,
         NewCallback(this, &NewTabUIQt::OnMostVisitedURLsAvailable));
    return;
  }
}

void NewTabUIQt::AddBlacklistURL(const GURL& url) {
  history::TopSites* ts = browser_->GetProfile()->GetTopSites();
  if (ts)
    ts->AddBlacklistedURL(url);
}

// This method is added for close thumbanails function.
void NewTabUIQt::RefreshMostVisitedArea() {
  StartQueryForMostVisited();
}

MaxViewImageProvider* NewTabUIQt::getImageProviderByName(QString name) {
    if(name == MOST_VISITED)
        return mostVisitedImageProvider_;
    else 
        return recentlyClosedImageProvider_;

}

MaxViewModel::MaxViewModel(NewTabUIQt* tab, std::vector<MostVisitedPage*>* data, QString name)
        : returnedImages_(0),
        new_tab_(tab),
        updateTimes_(0),
        name_(name),
        collapsedState_(LayoutThumbnails),
        closeButtonState_(true) {
  QHash<int, QByteArray> roles;
  roles[UrlRule] = "url";
  roles[TitleRule] = "title";
  roles[ThumbnailRule] = "thumbnail";
  roles[FaviconRule] = "favicon";
  roles[IndexRule] = "grid_id";

  setRoleNames(roles);
  clear();
  if(data) {
    MaxViewImageProvider* provider = new_tab_->getImageProviderByName(name_);
    provider->clear();
    int count = data->size();
    for(int i=0; i<count; i++) {
      const MostVisitedPage& page = *(*data)[i];
      siteInfoList_.push_back(new MostVisitedPage(page));
      ThumbnailEntry *thumbnail = new ThumbnailEntry(provider, page.url, 
          new_tab_->getProfile(), this);
      thumbnailList_.push_back(thumbnail);
      FaviconEntry *favicon = new FaviconEntry(provider, page.url, 
          new_tab_->getProfile(), this);
      faviconList_.push_back(favicon);
    }
  }
}

MaxViewModel::~MaxViewModel() {
    clear();
}

void MaxViewModel::updateContent(std::vector<MostVisitedPage*>* data) {
    if(data) {
        clear();
        MaxViewImageProvider* provider = new_tab_->getImageProviderByName(name_);
        provider->clear();
        int count = data->size();
        for(int i=0; i<count; i++) {
            const MostVisitedPage& page = *(*data)[i];
            siteInfoList_.push_back(new MostVisitedPage(page));
            ThumbnailEntry *thumbnail = new ThumbnailEntry(provider, page.url, 
              new_tab_->getProfile(), this);
            thumbnailList_.push_back(thumbnail);
            FaviconEntry *favicon = new FaviconEntry(provider, page.url, 
              new_tab_->getProfile(), this);
            faviconList_.push_back(favicon);
        }
        // collpased view when empty
        if(count == 0 && name_==QString(MOST_VISITED)) {
            setCollapsedState(LayoutCollapsed);
        }
    }
}

void MaxViewModel::beginReset()
{
    returnedImages_++;

    if (returnedImages_ == rowCount()*2) {
        DLOG(INFO) << "begin reset" << name_.toStdString() <<" model";
        beginResetModel();
    }
}

void MaxViewModel::endReset()
{
    if (returnedImages_ == rowCount()*2) {
        DLOG(INFO) << "end reset" << name_.toStdString() <<" model";
        updateTimes_++;
        endResetModel();
    }
}

void MaxViewModel::clear() {
    returnedImages_ = 0;
    beginResetModel();
    for(int i = 0; i < siteInfoList_.count(); i++) {
        delete siteInfoList_[i];
    }
    siteInfoList_.clear();
    for(int i = 0; i < thumbnailList_.count(); i++) {
        delete thumbnailList_[i];
    }
    thumbnailList_.clear();
    for(int i = 0; i < faviconList_.count(); i++) {
        delete faviconList_[i];
    }
    faviconList_.clear();

    endResetModel();
}

GURL MaxViewModel::getItemURL(int index) {
    if(index >= 0 && index < rowCount())
        return siteInfoList_[index]->url;
    else
        return GURL(EMPTY_PAGE);
}

int MaxViewModel::rowCount(const QModelIndex &parent) const
{
  int count = siteInfoList_.count();

  return count;
}

QVariant MaxViewModel::data(const QModelIndex & index, int role) const
{
  if (index.row() < 0 || index.row() > siteInfoList_.count())
    return QVariant();

  MostVisitedPage* siteInfoItem = siteInfoList_[(index.row()+1)*(index.column()+1) - 1];
  if (role == UrlRule) {
    //TODO: handle GURL here. return QUrl(siteInfoItem->GetURL());
    return QVariant();
  } else if (role == TitleRule) {
    std::wstring title_str = UTF16ToWide(siteInfoItem->title);
    return QString::fromStdWString(title_str);
  } else if (role == ThumbnailRule) {
    //DLOG(INFO)<<"query "<<(QString("image://") + name_ + QString("/thumbnail") + QString::number(updateTimes_) + QString("_") + QString::number((int64)siteInfoItem->GetID(), 16)).toStdString();
    return QString("image://") + name_ + QString("/thumbnail") + QString::number(updateTimes_) + QString("_") + QString::fromStdString(siteInfoItem->url.spec());
  } else if (role == FaviconRule) {
    //DLOG(INFO)<<"query "<<(QString("image://") + name_ + QString("/favicon") + QString::number(updateTimes_) + QString("_") + QString::number((int64)siteInfoItem->GetID(), 16)).toStdString();
    return QString("image://") + name_ + QString("/favicon") + QString::number(updateTimes_) + QString("_") + QString::fromStdString(siteInfoItem->url.spec());
  } else if (role == IndexRule) {
    //return index.row();
    return QString::fromStdString(siteInfoItem->url.spec());
    //return (int)siteInfoItem->GetID();
  }

  return QVariant();
}

QString MaxViewModel::GetCategoryName() {
  if(name_ == MOST_VISITED) {
    return QString::fromUtf8(l10n_util::GetStringUTF8(IDS_NEW_TAB_MOST_VISITED).c_str());
  } else if(name_ == RECENTLY_CLOSED) {
    return QString::fromUtf8(l10n_util::GetStringUTF8(IDS_NEW_TAB_RECENTLY_CLOSED).c_str());
  }
     
}

QString MaxViewModel::getId(int index) {
  //DLOG(INFO) <<"get index id: " << index;
  if(index >= 0 && index < rowCount()) {
    return QString::fromStdString(siteInfoList_[index]->url.spec());
  }
}

void MaxViewModel::openWebPage(int index) {
    if(getItemURL(index) != GURL(EMPTY_PAGE)) {
        new_tab_->getBrowser()->OpenURL(getItemURL(index), GURL(),
                CURRENT_TAB, PageTransition::LINK);
    }
}

void MaxViewModel::removeWebPage(int index) {
    GURL url = getItemURL(index);
    if(url != GURL(EMPTY_PAGE)) {
	new_tab_->AddBlacklistURL(url);
	new_tab_->RefreshMostVisitedArea();
    }
}
void MaxViewModel::bringToFront(int i) {
    MostVisitedPage* siteInfoItem = siteInfoList_[i];
    beginRemoveRows(QModelIndex(), i, i);
    siteInfoList_.removeAt(i);
    endRemoveRows();

    beginInsertRows(QModelIndex(), i, i);
    siteInfoList_.insert(i, siteInfoItem);
    endInsertRows();
}

void MaxViewModel::swap(int from, int to) {
    if(from<0 || from>rowCount() || to<0 || to>rowCount() || from==to) {
        return;
    }
    //beginResetModel();
    //update list
/*
    DLOG(INFO) <<"move from: " << from <<" to: " <<to;
    if(!beginMoveRows(QModelIndex(),from,from,QModelIndex(),to)) {
        return;
    }

    DLOG(INFO) <<"beginMoveRows";
    siteInfoList_.move(from,to);
    endMoveRows();
*/
    int big = from>to?from:to;
    int small = from>to?to:from;
    MostVisitedPage* siteInfoBigItem = siteInfoList_[big];
    MostVisitedPage* siteInfoSmallItem = siteInfoList_[small];
    beginRemoveRows(QModelIndex(), big, big);
    siteInfoList_.removeAt(big);
    endRemoveRows();

    beginInsertRows(QModelIndex(), big, big);
    siteInfoList_.insert(big, siteInfoSmallItem);
    endInsertRows();

    beginRemoveRows(QModelIndex(), small, small);
    siteInfoList_.removeAt(small);
    endRemoveRows();

    beginInsertRows(QModelIndex(), small, small);
    siteInfoList_.insert(small, siteInfoBigItem);
    endInsertRows();

    if(from == big) 
        new_tab_->HandleAddPinnedURL(siteInfoBigItem, small);
    else
        new_tab_->HandleAddPinnedURL(siteInfoSmallItem, big);

    //update thumbnail list

    //update favicon list

    //update mostvisited database
    
    //endResetModel();

}

#include "moc_new_tab_ui_qt.cc"
