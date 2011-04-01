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
    int finded = id.indexOf("_");
    int index = id.size() - finded - 1;
    if(id.contains("thumbnail")) {
      if (finded != -1) {
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
    //DLOG(INFO) <<"add map id: " << id.toStdString();
    if(type.contains("thumbnail")) {
      imageList_.insert(id, image);
    }else{
      favList_.insert(id,image);
    }
  }

private:
  QMap<QString, QImage> imageList_;
  QMap<QString, QImage> favList_;
  QImage blankImage_;
};

class ThumbnailEntry
{
public:

  ThumbnailEntry(const int64 index, MaxViewImageProvider* imageProvider,
                 GURL url, Profile* profile, MaxViewModel* model)
                 : index_(index), imageProvider_(imageProvider), model_(model) {
    if(url != GURL(EMPTY_PAGE)) {
      HistoryService* hs = profile->GetHistoryService(Profile::EXPLICIT_ACCESS);
      hs->GetPageThumbnail(url, &consumer_,
                           NewCallback(static_cast<ThumbnailEntry*>(this),
                           &ThumbnailEntry::onThumbnailDataAvailable));
    } else {
      //QImage image = QImage::fromColor();
      model_->beginReset();
      imageProvider_->addImage("thumbnail", QString::number(index_, 16), QImage());
      model_->endReset();
    }

  };


  void onThumbnailDataAvailable(HistoryService::Handle request_handle,
                                scoped_refptr<RefCountedBytes> jpeg_data) {
    model_->beginReset();
    if (jpeg_data.get()) {
      //DLOG(INFO) << "get image id: " << index_;
      std::vector<unsigned char> thumbnail_data;
      std::copy(jpeg_data->data.begin(), jpeg_data->data.end(),
      std::back_inserter(thumbnail_data));
      QImage image = QImage::fromData(thumbnail_data.data(), thumbnail_data.size());
      //DLOG(INFO) <<"thumbnail:";
      imageProvider_->addImage("thumbnail", QString::number(index_, 16), image);
    }
    model_->endReset();
  }; 
private:
  int64 index_;

  MaxViewImageProvider* imageProvider_;

  MaxViewModel* model_;

  CancelableRequestConsumer consumer_;
};

class FaviconEntry 
{
public: 
  FaviconEntry(const int64 index, MaxViewImageProvider* imageProvider,
               GURL url, Profile* profile, MaxViewModel* model)
               : index_(index), imageProvider_(imageProvider), model_(model) {

    FaviconService* favicon_service = profile->GetFaviconService(Profile::EXPLICIT_ACCESS);
    if(favicon_service) {
      FaviconService::Handle handle;
      if(url != GURL(EMPTY_PAGE)) {
        handle = favicon_service->GetFaviconForURL(GURL(url), history::FAVICON, &consumer_,
                 NewCallback(this, &FaviconEntry::OnFaviconDataAvailable));
      } else {
        model_->beginReset();
        imageProvider_->addImage("favicon", QString::number(index_,16), QImage());
        model_->endReset();
      }
    }
  }; 

  void OnFaviconDataAvailable(FaviconService::Handle handle,
                              history::FaviconData favicon) {

/* TODO, need rework based on new API
    model_->beginReset();
    if (data.get() && data->size()) {
      //scoped_refptr<RefCountedBytes> data_s = static_cast<scoped_refptr<RefCountedBytes>>data;
      //DLOG(INFO) << "get image id: " << index_;
      std::vector<unsigned char> fav_data;
      std::copy(data->front(), data->front()+data->size(),
      std::back_inserter(fav_data));
      QImage image = QImage::fromData(fav_data.data(), fav_data.size());
      //DLOG(INFO) <<"favicon:";
      imageProvider_->addImage("favicon", QString::number(index_,16), image);
    }
    model_->endReset();
*/
  }

private:
  int64 index_;

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
    pinned_urls_ = profile->GetPrefs()->GetMutableDictionary(prefs::kNTPMostVisitedPinnedURLs);

    impl_ = new NewTabUIQtImpl();
    mostVisitedModel_ = new MaxViewModel(this, NULL, QString(MOST_VISITED));
    recentlyClosedModel_ = new MaxViewModel(this, NULL, QString(RECENTLY_CLOSED));
    //Expand recently closed area defaully
    //recentlyClosedModel_->setCollapsedState(true);

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
    std::vector<PageUsageData*> pData;
    int size = data.size();
    for (size_t i = 0; i < size; i++) {
	const history::MostVisitedURL& mvp = data[i];	
	//TODO: Need to create an unique id for page.
        PageUsageData* page = new PageUsageData(0);
      	page->SetTitle(mvp.title);
      	page->SetURL(mvp.url);
        pData.push_back(page);
    }

    OnSegmentUsageAvailable(NULL, &pData);

    int count = pData.size();
    for(int i=0; i<count; i++) {
        PageUsageData* page = pData[i];
        delete page;
    }
    
}

void NewTabUIQt::OnSegmentUsageAvailable(CancelableRequestProvider::Handle handle,
                               std::vector<PageUsageData*>* data) {
    if(data==NULL)
        return;
    //Make Sure we have 8 item for Most Visited Area.
/*
    int count = data->size();
    for(int i=count; i<8; i++) {
        DLOG(INFO)<<__FUNCTION__<<"compensation:" << i << "with null";
        PageUsageData* page = new PageUsageData(0);
        page->SetURL(GURL(EMPTY_PAGE));
        page->SetTitle(UTF8ToUTF16(""));
        data->push_back(page);
    }
*/
    std::vector<PageUsageData*>* newData = new std::vector<PageUsageData*>();
    syncWithPinnedPage(data, newData);

    bool changed = false;
    int oldCount = mostVisitedModel_->rowCount();
    if(oldCount == 8) {
        for(int j=0; j<8; j++) {
            GURL new_url = (*newData)[j]->GetURL();
            GURL old_url = mostVisitedModel_->getItemURL(j);
            if(new_url != old_url) {
                changed = true;
                break;
            }
        }
    }else if( oldCount == 1 || oldCount != newData->size() ) {
        changed = true;
    }

    if(changed)
        mostVisitedModel_->updateContent(newData);

    if(isAboutToShow_) {
        impl_->show();
    }

    //Debug usage
    //dump(newData);

    //updateDataModel();

    //TODO: delete *data here!!!
    int count = newData->size();
    for(int i=0; i<count; i++) {
        PageUsageData* page = (*newData)[i];
        delete page;
    }
    newData->clear();
    delete newData;
}

void NewTabUIQt::syncWithPinnedPage(std::vector<PageUsageData*>* data,
							    std::vector<PageUsageData*>* newData) {
  size_t data_index = 0;
  size_t output_index = 0;

  //Clean old data from pined list.
  for (DictionaryValue::key_iterator it = pinned_urls_->begin_keys();
      it != pinned_urls_->end_keys(); ++it) {
    Value* value;
    if (pinned_urls_->GetWithoutPathExpansion(*it, &value)) {
      if (!value->IsType(DictionaryValue::TYPE_DICTIONARY)) {
        // Moved on to TopSites and now going back.
        pinned_urls_->Clear();
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
        const PageUsageData& page = *(*data)[i];
        if( pinnedUrl == page.GetURL() ) {
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
    MostVisitedPage mvp;
    
    //Find data from pinned list firstly
    if (GetPinnedURLAtIndex(output_index, &mvp)) {
      DLOG(INFO)<<"got "<< mvp.title<<"from pinned list";
      found = true;
    }

    //Find data from query data secondly
    while (!found && data_index < data->size()) {
      const PageUsageData& page = *(*data)[data_index];
      data_index++;
      mvp.id = (int)page.GetID();
      mvp.url = page.GetURL();

      // filt out pinned URLs which will be check in first step
      std::string key = GetDictionaryKeyForURL(mvp.url.spec());
      if (pinned_urls_->HasKey(key))
        continue;

      mvp.title = page.GetTitle();
      found = true;
    }

    //save the new data.
    if (found) {
      //if(pinned)
      //    newData.push_back(new PageUsageData((*data)[data_index]));
      //else {
      PageUsageData* page = new PageUsageData(mvp.id);
      page->SetTitle(mvp.title);
      page->SetURL(mvp.url);
      newData->push_back(page);
      //}
    }
    output_index++;
  }
}

void NewTabUIQt::HandleAddPinnedURL(PageUsageData* data, int index) {
  MostVisitedPage mvp;
  mvp.id = (int)data->GetID();
  mvp.title = data->GetTitle();
  mvp.url = data->GetURL();

  AddPinnedURL(mvp, index);
}

void NewTabUIQt::AddPinnedURL(const MostVisitedPage& page, int index) {
  history::TopSites* ts = browser_->GetProfile()->GetTopSites();
  if (ts)
    ts->AddPinnedURL(page.url, index);
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
        pinned_urls_->Clear();
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
        dict->GetInteger("id", &id);
        page->id = id;
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
  if (ts)
    ts->RemovePinnedURL(url);
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
    PageUsageData* value) {
  if (tab.navigations.empty())
    return false;

  const TabNavigation& current_navigation = tab.navigations.at(tab.current_navigation_index);
  
  GURL gurl = current_navigation.virtual_url();
  if (gurl == GURL(chrome::kChromeUINewTabURL))
    return false;
  value->SetURL(gurl);
  bool using_url_as_the_title = false;
  string16 title_to_set = current_navigation.title();
  if (title_to_set.empty()) {
    using_url_as_the_title = true;
    title_to_set = UTF8ToUTF16(gurl.spec());
  }
  value->SetTitle(title_to_set);

  return true;
}

bool NewTabUIQt::EnsureTabIsUnique(
    const PageUsageData* value,
    std::set<string16>* unique_items) {
    DCHECK(unique_items);
    string16 title = value->GetTitle();
    string16 url = UTF8ToUTF16(value->GetURL().spec());
    string16 unique_key = title + url;
    if (unique_items->find(unique_key) != unique_items->end())
      return false;
    else
      unique_items->insert(unique_key);

    return true;
}

void NewTabUIQt::TabRestoreServiceChanged(TabRestoreService* service) {
  const TabRestoreService::Entries& entries = service->entries();
  std::vector<PageUsageData*> list_value;
  std::set<string16> unique_items;
  int added_count = 0;
  const int max_count = 8;
  for (TabRestoreService::Entries::const_iterator it = entries.begin();
       it != entries.end() && added_count < max_count; ++it) {
    TabRestoreService::Entry* entry = *it;
    PageUsageData* value = new PageUsageData(added_count);
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
/*
  // Use TopSites.
  history::TopSites* ts = browser_->GetProfile()->GetTopSites();
  if (ts) {
    ts->GetMostVisitedURLs(
         &topsites_consumer_,
         NewCallback(this, &NewTabUIQt::OnMostVisitedURLsAvailable));
    return;
  }
*/
  Profile* profile = browser_->profile();
  const int page_count = kMostVisitedPages;
  const int result_count = page_count;
  HistoryService* hs =
      profile->GetHistoryService(Profile::EXPLICIT_ACCESS);
  if (hs) {
    hs->QuerySegmentUsageSince(
        &cancelable_consumer_,
        base::Time::Now() - base::TimeDelta::FromDays(kMostVisitedScope),
        result_count,
        NewCallback(this, &NewTabUIQt::OnSegmentUsageAvailable));
  }
}

MaxViewImageProvider* NewTabUIQt::getImageProviderByName(QString name) {
    if(name == MOST_VISITED)
        return mostVisitedImageProvider_;
    else 
        return recentlyClosedImageProvider_;

}
void NewTabUIQt::dump(std::vector<PageUsageData*>* data) {
    int count = data->size();
    for(int i=0; i<count; i++) {
        const PageUsageData& page = *(*data)[i];
        DLOG(INFO)<<page.GetURL();
        DLOG(INFO)<<page.GetTitle();

/*
        DLOG(INFO)<<page.thumbnail_pending();
        DLOG(INFO)<<page.HasThumbnail();
        DLOG(INFO)<<page.favicon_pending();
        DLOG(INFO)<<page.HasFavIcon();
        DLOG(INFO)<< page.GetURL().spec().c_str();
        //std::wstring imageUrl = UTF8ToWide(page.GetURL().spec());
        char url[] = "chrome://thumb/";
        strcat(url, page.GetURL().spec().c_str());
        DLOG(INFO)<< url;
        QImage image = QImage(url);
        DLOG(INFO)<<"add map id: " << QString::number(i, 10).toStdString();
        imageProvider_->addImage(QString::number(i, 10), image);
        DLOG(INFO)<<"width = "<<image.width();
        DLOG(INFO)<<"height = "<<image.height();

        if(page.GetThumbnail()) {
        QImage image = SkBitmap2Image(*(page.GetThumbnail()));
        }
        else
        DLOG(INFO)<<"error";
*/
    }

}

MaxViewModel::MaxViewModel(NewTabUIQt* tab, std::vector<PageUsageData*>* data, QString name)
        : returnedImages_(0),
        new_tab_(tab),
        updateTimes_(0),
        name_(name),
        collapsedState(false) {
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
      const PageUsageData& page = *(*data)[i];
      siteInfoList_.push_back(new PageUsageData(page));
      ThumbnailEntry *thumbnail = new ThumbnailEntry((int64)page.GetID(), provider, page.GetURL(), 
          new_tab_->getProfile(), this);
      thumbnailList_.push_back(thumbnail);
      FaviconEntry *favicon = new FaviconEntry((int64)page.GetID(), provider, page.GetURL(), 
          new_tab_->getProfile(), this);
      faviconList_.push_back(favicon);
    }
  }
}

MaxViewModel::~MaxViewModel() {
    clear();
}

void MaxViewModel::updateContent(std::vector<PageUsageData*>* data) {
    if(data) {
        clear();
        MaxViewImageProvider* provider = new_tab_->getImageProviderByName(name_);
        provider->clear();
        int count = data->size();
        for(int i=0; i<count; i++) {
            const PageUsageData& page = *(*data)[i];
            siteInfoList_.push_back(new PageUsageData(page));
            ThumbnailEntry *thumbnail = new ThumbnailEntry((int64)page.GetID(), provider, page.GetURL(), 
              new_tab_->getProfile(), this);
            thumbnailList_.push_back(thumbnail);
            FaviconEntry *favicon = new FaviconEntry((int64)page.GetID(), provider, page.GetURL(), 
              new_tab_->getProfile(), this);
            faviconList_.push_back(favicon);
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
        return siteInfoList_[index]->GetURL();
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

  PageUsageData* siteInfoItem = siteInfoList_[(index.row()+1)*(index.column()+1) - 1];
  if (role == UrlRule) {
    //TODO: handle GURL here. return QUrl(siteInfoItem->GetURL());
    return QVariant();
  } else if (role == TitleRule) {
    std::wstring title_str = UTF16ToWide(siteInfoItem->GetTitle());
    return QString::fromStdWString(title_str);
  } else if (role == ThumbnailRule) {
    //DLOG(INFO)<<"query "<<(QString("image://") + name_ + QString("/thumbnail") + QString::number(updateTimes_) + QString("_") + QString::number((int64)siteInfoItem->GetID(), 16)).toStdString();
    return QString("image://") + name_ + QString("/thumbnail") + QString::number(updateTimes_) + QString("_") + QString::number((int64)siteInfoItem->GetID(), 16);
  } else if (role == FaviconRule) {
    //DLOG(INFO)<<"query "<<(QString("image://") + name_ + QString("/favicon") + QString::number(updateTimes_) + QString("_") + QString::number((int64)siteInfoItem->GetID(), 16)).toStdString();
    return QString("image://") + name_ + QString("/favicon") + QString::number(updateTimes_) + QString("_") + QString::number((int64)siteInfoItem->GetID(), 16);
  } else if (role == IndexRule) {
    //return index.row();
    return (int)siteInfoItem->GetID();
  }

  return QVariant();
}

QString MaxViewModel::GetCategoryName() {
  if(name_ == MOST_VISITED) {
    return QString::fromStdString(l10n_util::GetStringUTF8(IDS_NEW_TAB_MOST_VISITED));
  } else if(name_ == RECENTLY_CLOSED) {
    return QString::fromStdString(l10n_util::GetStringUTF8(IDS_NEW_TAB_RECENTLY_CLOSED));
  }
     
}

int MaxViewModel::getId(int index) {
  //DLOG(INFO) <<"get index id: " << index;
  if(index >= 0 && index < rowCount())
    return (int)(siteInfoList_[index]->GetID());
}

void MaxViewModel::openWebPage(int index) {
    if(getItemURL(index) != GURL(EMPTY_PAGE)) {
        new_tab_->getBrowser()->OpenURL(getItemURL(index), GURL(),
                CURRENT_TAB, PageTransition::LINK);
    }
    //TODO: UserMetrics????
    //UserMetrics::RecordAction(UserMetricsAction("ClickedBookmarkBarURLButton"),
    //                    browser_->profile());

}

void MaxViewModel::bringToFront(int i) {
    PageUsageData* siteInfoItem = siteInfoList_[i];
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
    PageUsageData* siteInfoBigItem = siteInfoList_[big];
    PageUsageData* siteInfoSmallItem = siteInfoList_[small];
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
