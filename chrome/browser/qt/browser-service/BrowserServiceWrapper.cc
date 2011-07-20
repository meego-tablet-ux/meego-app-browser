#include "ui/gfx/codec/png_codec.h"
#include "skia/ext/image_operations.h"
#include "skia/ext/platform_canvas.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "base/stl_util-inl.h"
#include "base/string16.h"
#include "base/string_number_conversions.h"
#include "base/utf_string_conversions.h"
#include "content/common/notification_type.h"
#include "content/common/notification_service.h"
#include "base/base64.h"
#include "base/file_util.h"
#include "base/string_util.h"
#include "base/stringprintf.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "chrome/browser/ui/tab_contents/tab_contents_wrapper.h"
#include "chrome/browser/history/history_types.h"
#include "chrome/browser/history/recent_and_bookmark_thumbnails_qt.h"
#include "chrome/browser/history/top_sites.h"
#include "content/browser/renderer_host/render_process_host.h"
#include "content/browser/renderer_host/render_view_host.h"
#include "chrome/browser/tab_contents/thumbnail_generator.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"
#include "chrome/browser/autocomplete/autocomplete.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/search_engines/template_url.h"
#include "chrome/browser/search_engines/template_url_model.h"
#include "chrome/common/url_constants.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "base/string16.h"

#include "BrowserServiceWrapper.h"
#include "MeeGoPluginAPI.h"

class BrowserServiceBackend : public base::RefCountedThreadSafe<BrowserServiceBackend>
{
 public:
  BrowserServiceBackend():
      plugin_(NULL)
  {}

  void InitBackend(BrowserServiceWrapper* wrapper);
  void RemoveURLItemImpl(std::string id);
  void AddURLItemImpl(int64 id, std::string url, std::string title, std::string favicon_url, 
                       int visit_count, int typed_count, long long last_visit_time);
  void AddFavIconItemImpl(GURL url, scoped_refptr<RefCountedMemory> png_data);
  void AddThumbnailItemImpl(int index, GURL url, scoped_refptr<RefCountedBytes> jpeg_data);
  void AddTabItemImpl(int tab_id, int win_id, std::string url, std::string title, std::string faviconUrl);
  void RemoveTabItemImpl(int index);
  void UpdateTabItemImpl(int tab_id, int win_id, std::string url, std::string title, std::string faviconUrl);
  void RemoveBookmarkItemImpl(int64 id);
  void AddBookmarkItemImpl(int64 id, std::string url,
                           std::string title, std::string faviconUrl, long long dataAdded);
  void RemoveAllURLsImpl();

  void OnBrowserClosingImpl();

  void PrepareReloadTabList();

  void FinishReloadTabList();

 private:
  friend class base::RefCountedThreadSafe<BrowserServiceBackend>;

  ~BrowserServiceBackend() {
    if (plugin_)
      delete plugin_;
  }
  
  MeeGoPluginAPI* plugin_;
  //int current_tab_id_;
};

class SnapshotTaker {
 public:
  SnapshotTaker(BrowserServiceBackend* backend, GURL url, int index)
        :backend_(backend), url_(url), tab_index_(index) {
  }

  void SnapshotOnContents(TabContents* contents) {
      Browser* browser = BrowserList::GetLastActive();
      BrowserWindowQt* browser_window = (BrowserWindowQt*)browser->window();
      int width = browser_window->window()->width();
      int height = browser_window->window()->height();
      gfx::Size page_size = gfx::Size(width, height);
      gfx::Size snapshot_size = gfx::Size(512, 320);
 
      RenderViewHost* renderer = contents->render_view_host();
      if (!renderer)
        return;

      if(renderer) {
        ThumbnailGenerator* generator =  g_browser_process->GetThumbnailGenerator();
        ThumbnailGenerator::ThumbnailReadyCallback* callback =
          NewCallback(this, &SnapshotTaker::OnSnapshotTaken);
        generator->MonitorRenderer(renderer, true);
        generator->AskForSnapshot(renderer, false, callback,page_size, snapshot_size);
      }
  }
 

  void OnSnapshotTaken(const SkBitmap& bitmap) {
/*
      // Debug use
      base::ThreadRestrictions::ScopedAllowIO allow_io;
      std::vector<unsigned char> png_data;
      gfx::PNGCodec::EncodeBGRASkBitmap(bitmap, true, &png_data);
      int bytes_written = file_util::WriteFile(FilePath("/root/.config/chromium/a.png"),
                                reinterpret_cast<char*>(&png_data[0]), png_data.size());
*/
      scoped_refptr<RefCountedBytes> jpeg_data;
      if (!EncodeBitmap(bitmap, &jpeg_data))
	return;

      if (jpeg_data.get() && jpeg_data->data.size()) {
	BrowserThread::PostTask(BrowserThread::DB, FROM_HERE, NewRunnableMethod(
            backend_, &BrowserServiceBackend::AddThumbnailItemImpl, tab_index_, url_, jpeg_data));
      }
  }
  
// static
  static bool EncodeBitmap(const SkBitmap& bitmap,
                            scoped_refptr<RefCountedBytes>* bytes) {
      *bytes = new RefCountedBytes();
      SkAutoLockPixels bitmap_lock(bitmap);
      std::vector<unsigned char> data;
      if (!gfx::JPEGCodec::Encode(
	      reinterpret_cast<unsigned char*>(bitmap.getAddr32(0, 0)),
	      gfx::JPEGCodec::FORMAT_BGRA, bitmap.width(),
	      bitmap.height(),
	      static_cast<int>(bitmap.rowBytes()), 90,
	      &data)) {
	return false;
      }
      // As we're going to cache this data, make sure the vector is only as big as
      // it needs to be.
      (*bytes)->data = data;
      return true;
  }

 private:
  GURL url_;
  int tab_index_;
  BrowserServiceBackend* backend_;
};


void BrowserServiceBackend::InitBackend(BrowserServiceWrapper* wrapper)
{
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  plugin_ = new MeeGoPluginAPI(wrapper);
}

void BrowserServiceBackend::PrepareReloadTabList()
{
  if(plugin_) {
    plugin_->clearAllTabItems();
  }
}

void BrowserServiceBackend::FinishReloadTabList()
{
  if(plugin_) 
    plugin_->emitTabListUpdatedSignal();
}

void BrowserServiceBackend::AddTabItemImpl(int tab_id, int win_id, std::string url, std::string title, std::string faviconUrl)
{
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  plugin_->addTabItem(tab_id, win_id, url, title, faviconUrl);
}
void BrowserServiceBackend::RemoveTabItemImpl(int index)
{
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  plugin_->removeTabItem(index);
}
void BrowserServiceBackend::UpdateTabItemImpl(int tab_id, int win_id, std::string url, std::string title, std::string faviconUrl)
{
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  plugin_->updateTabItem(tab_id, win_id, url, title, faviconUrl);
}

void BrowserServiceBackend::AddBookmarkItemImpl(int64 id, std::string url,
                                                std::string title, std::string faviconUrl, long long dataAdded)
{
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  plugin_->addBookmarkItem(id, url, title, faviconUrl, dataAdded);
}

void BrowserServiceBackend::RemoveBookmarkItemImpl(int64 id)
{
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  plugin_->removeBookmarkItem(id);
}

void BrowserServiceBackend::AddThumbnailItemImpl(int index, GURL url, scoped_refptr<RefCountedBytes> jpeg_data)
{
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  plugin_->addThumbnailItem(index, url.spec(), base::Time::Now().ToInternalValue(),
                            reinterpret_cast<const unsigned char*>(&jpeg_data->data[0]),
                            jpeg_data->data.size());
}

void BrowserServiceBackend::AddFavIconItemImpl(GURL url, scoped_refptr<RefCountedMemory> png_data)
{
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  plugin_->addFavIconItem(url.HostNoBrackets(), base::Time::Now().ToInternalValue(),
                          png_data->front(), png_data->size());
}

///////////////////////////////////////////
// BrowserServiceWrapper Implementation
////////////////////////////////////////// 
BrowserServiceWrapper* BrowserServiceWrapper::GetInstance() {
  return Singleton<BrowserServiceWrapper>::get();
}

BrowserServiceWrapper::BrowserServiceWrapper():
    backend_(NULL),
    browser_(NULL),
    factory_(this),
    onBrowserClosingCalled_(false)
{
}

BrowserServiceWrapper::~BrowserServiceWrapper()
{
  backend_->Release();
  ClearSnapshotList();
  ///\todo: unregister observers
  registrar_.Remove(this,
      NotificationType::HISTORY_URL_VISITED,
      NotificationService::AllSources());
  
  registrar_.Remove(this,
      NotificationType::HISTORY_URLS_DELETED,
      NotificationService::AllSources());
  registrar_.Remove(this,
      NotificationType::BROWSER_CLOSING,
      NotificationService::AllSources());

  registrar_.Remove(this,
      NotificationType::TAB_CLOSED,
      NotificationService::AllSources());

  registrar_.Remove(this,
      NotificationType::TAB_CONTENTS_TITLE_UPDATED,
      NotificationService::AllSources());

  /*registrar_.Remove(this,
      NotificationType::BROWSER_WINDOW_MINIMIZED,
      NotificationService::AllSources());*/
}

void BrowserServiceWrapper::ClearSnapshotList() {
  int count = snapshotList_.count();
  for(int i=0; i<count; i++) {
    delete snapshotList_[i];
  }
  snapshotList_.clear();
}

void BrowserServiceWrapper::Init(Browser* browser)
{
  browser_ = browser;

  backend_ = new BrowserServiceBackend();

  backend_->AddRef();

  MessageLoop::current()->PostDelayedTask(FROM_HERE,
                                          factory_.NewRunnableMethod(&BrowserServiceWrapper::InitBottomHalf),
                                          2000);
}

void BrowserServiceWrapper::InitBottomHalf()
{
  browser_->tabstrip_model()->AddObserver(this);

  browser_->profile()->GetBookmarkModel()->AddObserver(this);

  registrar_.Add(this,
                 NotificationType::HISTORY_URL_VISITED,
                 NotificationService::AllSources());
  registrar_.Add(this,
                 NotificationType::HISTORY_URLS_DELETED,
                 NotificationService::AllSources());
  registrar_.Add(this,
                 NotificationType::BROWSER_CLOSING,
                 NotificationService::AllSources());

  registrar_.Add(this,
                 NotificationType::TAB_CLOSED,
                 NotificationService::AllSources());

  registrar_.Add(this,
                 NotificationType::TAB_CONTENTS_TITLE_UPDATED,
                 NotificationService::AllSources());

  /*registrar_.Add(this,
                 NotificationType::BROWSER_WINDOW_MINIMIZED,
                 NotificationService::AllSources());*/

  BrowserThread::PostTask(BrowserThread::DB, FROM_HERE, NewRunnableMethod(
      backend_, &BrowserServiceBackend::InitBackend, this));

  MessageLoop::current()->PostDelayedTask(FROM_HERE,
                                          factory_.NewRunnableMethod(&BrowserServiceWrapper::ReloadTabList),
                                          1500);
}

void BrowserServiceWrapper::Loaded(BookmarkModel* model) {
}

void BrowserServiceWrapper::BookmarkNodeMoved(
    BookmarkModel* model,
    const BookmarkNode* old_parent,
    int old_index,
    const BookmarkNode* new_parent,
    int new_index) {
}

void BrowserServiceWrapper::BookmarkNodeAdded(BookmarkModel* model,
                                              const BookmarkNode* parent,
                                              int index) {
  const BookmarkNode* node = parent->GetChild(index);
  BrowserThread::PostTask(BrowserThread::DB, FROM_HERE, NewRunnableMethod(
  backend_, &BrowserServiceBackend::AddBookmarkItemImpl, node->id(), node->GetURL().spec(),
                      UTF16ToUTF8(node->GetTitle()), node->GetURL().HostNoBrackets(),
                      node->date_added().ToInternalValue()));
}


void BrowserServiceWrapper::BookmarkNodeRemoved(
    BookmarkModel* model,
    const BookmarkNode* parent,
    int index,
    const BookmarkNode* node) {
  BrowserThread::PostTask(BrowserThread::DB, FROM_HERE, NewRunnableMethod(
      backend_, &BrowserServiceBackend::RemoveBookmarkItemImpl, node->id()));
}

void BrowserServiceWrapper::BookmarkNodeChanged(
    BookmarkModel* model, const BookmarkNode* node) {
  ///\todo: should implement
}

void BrowserServiceWrapper::BookmarkNodeFaviconLoaded(
    BookmarkModel* model, const BookmarkNode* node) {
}

void BrowserServiceWrapper::BookmarkNodeChildrenReordered(
    BookmarkModel* model, const BookmarkNode* node) {
}

void BrowserServiceWrapper::BookmarkImportBeginning(BookmarkModel* model) {
}

void BrowserServiceWrapper::BookmarkImportEnding(BookmarkModel* model) {
}

void BrowserServiceWrapper::TabInsertedAt(TabContentsWrapper* contents,
                                          int index,
                                          bool foreground) {
  const GURL& url = contents->tab_contents()->GetURL();
  if(!url.SchemeIs("chrome") || url.HostNoBrackets() == "newtab") {
    ReloadTabList();
  }
}

void BrowserServiceWrapper::TabDetachedAt(TabContentsWrapper* contents,
    int index) {
}

void BrowserServiceWrapper::TabClosingAt(TabStripModel* tab_strip_model,  
                                         TabContentsWrapper* contents,
                                         int index) {
    urlcaptured_.removeOne(contents->tab_contents()->GetURL());
}

void BrowserServiceWrapper::TabDeselected(TabContents* content)
{
}

void BrowserServiceWrapper::TabSelectedAt(TabContentsWrapper* old_contents,
                                                TabContentsWrapper* new_contents,
                                                int index,
                                                bool user_gesture) {
  if (! urlcaptured_.contains(old_contents->tab_contents()->GetURL()))
    UpdateTabInfo(old_contents->tab_contents(), true);
}

void BrowserServiceWrapper::TabMoved(TabContentsWrapper* contents,
                                           int from_index,
                                           int to_index) {
}

void BrowserServiceWrapper::OnThumbnailDataAvailable(
    HistoryService::Handle handle,
    scoped_refptr<RefCountedBytes> jpeg_data) {
    GURL* url =
        consumer_.GetClientData(
              browser_->profile()->GetTopSites()->GetRecentAndBookmarkThumbnails(), handle);
            //browser_->profile()->GetHistoryService(Profile::EXPLICIT_ACCESS), handle);
    if (jpeg_data.get() && jpeg_data->data.size()) {
      BrowserThread::PostTask(BrowserThread::DB, FROM_HERE, NewRunnableMethod(
          backend_, &BrowserServiceBackend::AddThumbnailItemImpl, 0, *url, jpeg_data));
    }
    delete url;
}

void BrowserServiceWrapper::OnFaviconDataAvailable(
    FaviconService::Handle handle,
    history::FaviconData favicon) {
  GURL* url =
      consumer_.GetClientData(
          browser_->profile()->GetFaviconService(Profile::EXPLICIT_ACCESS), handle);
  bool known_favicon = favicon.known_icon;
  scoped_refptr<RefCountedMemory> image_data = favicon.image_data.get(); 
  if (known_favicon && image_data.get() && image_data->size()) {
    BrowserThread::PostTask(BrowserThread::DB, FROM_HERE, NewRunnableMethod(
        backend_, &BrowserServiceBackend::AddFavIconItemImpl, *url, image_data));    
  }
  delete url;
}

void BrowserServiceWrapper::GetThumbnail(TabContents* contents, const GURL& url, int index)
{
  // If a snapshot has aleardy been taken when the tab is loaded, do not take thumbnail any more.
  if (urlcaptured_.contains(url))
    return;

  // Make sure the thumbail capturing is not so frequent
  // We set time interval between two capturing action is 3s
  qint64 new_timestamp = QDateTime::currentMSecsSinceEpoch();

  if(url2timestamp_.contains(url)) {
    qint64 old_timestamp = url2timestamp_[url];
    // every  3seconds to update thumbnail
    if(new_timestamp - old_timestamp < 3000) return;
  }
  
  url2timestamp_[url] = new_timestamp;

  // make sure the contents is valid when taking snapshot
  TabStripModel* model = browser_->tabstrip_model();

  bool alive_ = false;
  
  for (int i = 0; i < model->count(); i++)
  {
    TabContents* contents_ = model->GetTabContentsAt(i)->tab_contents();
    if (contents_ && (contents_ == contents))
    {
        alive_ = true;
        break;
    }
  }
  
  if (alive_)
  {    
      // We use new method to get thumbnail here for higher qulity for web panel.
      SnapshotTaker* taker = new SnapshotTaker(backend_, url, index);
      taker->SnapshotOnContents(contents);
      snapshotList_.push_back(taker);
      if (!contents->is_loading())
        urlcaptured_.append(url);
  }  
}

void BrowserServiceWrapper::GetFavIcon(const GURL& url)
{
  // Get favicon for the given url
  FaviconService* favicon_service =
      browser_->profile()->GetFaviconService(Profile::EXPLICIT_ACCESS);
  if (favicon_service)
  {
    FaviconService::Handle handle = favicon_service->GetFaviconForURL(
        url, history::FAVICON, &consumer_,
        NewCallback(this, &BrowserServiceWrapper::OnFaviconDataAvailable));
    consumer_.SetClientData(favicon_service, handle, new GURL(url));
  }
}

void BrowserServiceWrapper::AddOpenedTab()
{
  int tabCount = browser_->tab_count();
  for (int index = 0; index < tabCount; index++)
  {
    TabInsertedAt(browser_->GetTabContentsWrapperAt(index), index, false);
  }
}

void BrowserServiceWrapper::TabChangedAt(TabContentsWrapper* contents,
                                         int index,
                                         TabChangeType change_type) {
}

void BrowserServiceWrapper::TabReplacedAt(TabStripModel* tab_strip_model,
					  TabContentsWrapper* old_contents,
                                          TabContentsWrapper* new_contents,
                                          int index)
{
}

void BrowserServiceWrapper::TabStripEmpty()
{
}

void BrowserServiceBackend::OnBrowserClosingImpl()
{
  plugin_->emitBrowserCloseSignal();
}

void BrowserServiceWrapper::Observe(NotificationType type,
                                    const NotificationSource& source,
                                    const NotificationDetails& details) {
  switch (type.value) {
    case NotificationType::HISTORY_URL_VISITED:
      HistoryUrlVisited(
          Details<const history::URLVisitedDetails>(details).ptr());
      break;

    case NotificationType::HISTORY_URLS_DELETED:
      HistoryUrlsRemoved(
          Details<const history::URLsDeletedDetails>(details).ptr());
      break;

    case NotificationType::BROWSER_CLOSING:
      if (!onBrowserClosingCalled_) {
        OnBrowserClosing();
        onBrowserClosingCalled_ = true;
      }
      break;

    /*case NotificationType::BROWSER_WINDOW_MINIMIZED:
      {
        BrowserWindow* window = Source<BrowserWindow>(source).ptr();
        OnBrowserWindowMinimized(window);
        break;
      }*/

    case NotificationType::TAB_CLOSED:
      {
        if(!browser_->IsAttemptingToCloseBrowser())
          ReloadTabList();
        break;
      }

    case NotificationType::TAB_CONTENTS_TITLE_UPDATED:
      {
        TabContents* contents = Source<TabContents>(source).ptr();
        UpdateTabInfo(contents);

        // Record history url
        HistoryService* hs = browser_->profile()->GetHistoryService(Profile::EXPLICIT_ACCESS);
        hs->QueryURL(contents->GetURL(), true, &consumer_,
            NewCallback(this, &BrowserServiceWrapper::AddURLItem));
        break;
      }

    default:
      break;
  }
}

void BrowserServiceWrapper::OnBrowserClosing()
{
  UpdateTabListAndThumbnails(false);
  backend_->OnBrowserClosingImpl();
}

void BrowserServiceWrapper::OnBrowserWindowMinimized(BrowserWindow* window)
{
  if(window != browser_->window()) return;
  UpdateTabListAndThumbnails(false);
  UpdateTabInfo(browser_->GetSelectedTabContentsWrapper()->tab_contents() , true);
}

void BrowserServiceWrapper::UpdateTabInfo(TabContents* contents, bool update_thumbnail)
{
  const GURL& url = contents->GetURL();

  if(url.SchemeIs("chrome") && url.HostNoBrackets() != "newtab")
    return;

  int index = browser_->tabstrip_model()->GetWrapperIndex(contents);
  int win_id = contents->controller().window_id().id();

  if(update_thumbnail && url.HostNoBrackets() != "newtab") {
    MessageLoop::current()->PostTask(FROM_HERE,
        factory_.NewRunnableMethod(
          &BrowserServiceWrapper::GetThumbnail,
          contents, url, index));

    MessageLoop::current()->PostTask(FROM_HERE,
        factory_.NewRunnableMethod(
          &BrowserServiceWrapper::GetFavIcon, url));
  }
  BrowserThread::PostTask(BrowserThread::DB, FROM_HERE, NewRunnableMethod(
                              backend_, &BrowserServiceBackend::UpdateTabItemImpl,
                              index, win_id, url.spec(),
                              UTF16ToUTF8(contents->GetTitle()),url.HostNoBrackets()));
}

void BrowserServiceWrapper::UpdateTabListAndThumbnails(bool update_thumbnail)
{
  TabStripModel* model = browser_->tabstrip_model();
  for (int index = 0; index < model->count(); index++)
  {
    TabContents* contents = model->GetTabContentsAt(index)->tab_contents();
    if(!contents->GetURL().SchemeIs("chrome"))
      UpdateTabInfo(contents, update_thumbnail);
  }
  BrowserThread::PostTask(BrowserThread::DB, FROM_HERE, NewRunnableMethod(
        backend_, &BrowserServiceBackend::FinishReloadTabList));
}

void BrowserServiceWrapper::ReloadTabList()
{
  BrowserThread::PostTask(BrowserThread::DB, FROM_HERE, NewRunnableMethod(
        backend_, &BrowserServiceBackend::PrepareReloadTabList));

  TabStripModel* model = browser_->tabstrip_model();
  for (int index = 0; index < model->count(); index++)
  {
    TabContents* contents = model->GetTabContentsAt(index)->tab_contents();
    const GURL& url = contents->GetURL();
    if (!url.SchemeIs("chrome") || url.HostNoBrackets() == "newtab")
    {
      int win_id = contents->controller().window_id().id();
      BrowserThread::PostTask(BrowserThread::DB, FROM_HERE, NewRunnableMethod(
            backend_, &BrowserServiceBackend::AddTabItemImpl,
            index, win_id, url.spec(),
            UTF16ToUTF8(contents->GetTitle()),url.HostNoBrackets()));
    }
  }
  BrowserThread::PostTask(BrowserThread::DB, FROM_HERE, NewRunnableMethod(
        backend_, &BrowserServiceBackend::FinishReloadTabList));
}



void BrowserServiceWrapper::HistoryUrlVisited(
    const history::URLVisitedDetails* details) {
  history::URLRow row = details->row;
  BrowserThread::PostTask(BrowserThread::DB, FROM_HERE, NewRunnableMethod(
        backend_, &BrowserServiceBackend::AddURLItemImpl, row.id(), row.url().spec(), 
        UTF16ToUTF8(row.title()), row.url().HostNoBrackets(), row.visit_count(),
        row.typed_count(), row.last_visit().ToInternalValue()));
}

void BrowserServiceWrapper::AddURLItem(HistoryService::Handle handle,
                                          bool success,
                                          const history::URLRow* row,
                                          history::VisitVector* visit_vector) {
  BrowserThread::PostTask(BrowserThread::DB, FROM_HERE, NewRunnableMethod(
      backend_, &BrowserServiceBackend::AddURLItemImpl, row->id(), row->url().spec(),
                      UTF16ToUTF8(row->title()), row->url().HostNoBrackets(), row->visit_count(),
                      row->typed_count(), row->last_visit().ToInternalValue()));  
}

void BrowserServiceBackend::AddURLItemImpl(int64 id, std::string url, std::string title, std::string favicon_url, 
                    int visit_count, int typed_count, long long last_visit_time)
{
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  plugin_->addURLItem(id, url, title, favicon_url, visit_count, typed_count, last_visit_time);
}

void BrowserServiceWrapper::RemoveURLItem(HistoryService::Handle handle,
                                          bool success,
                                          const history::URLRow* row,
                                          history::VisitVector* visit_vector) {

  BrowserThread::PostTask(BrowserThread::DB, FROM_HERE, NewRunnableMethod(
      backend_, &BrowserServiceBackend::RemoveURLItemImpl, row->url().spec()));  
}

void BrowserServiceBackend::RemoveURLItemImpl(std::string id)
{
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  plugin_->removeURLItem(id);
}

void BrowserServiceBackend::RemoveAllURLsImpl()
{
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  plugin_->clearAllURLs();
}
void BrowserServiceWrapper::HistoryUrlsRemoved(
    const history::URLsDeletedDetails* details) {
  
  if (details->all_history)
  {
    BrowserThread::PostTask(BrowserThread::DB, FROM_HERE, NewRunnableMethod(
        backend_, &BrowserServiceBackend::RemoveAllURLsImpl));  
  }
  else
  {
    for (std::set<GURL>::const_iterator iterator = details->urls.begin();
         iterator != details->urls.end();
         ++iterator) {
      //query URL for id
      HistoryService* hs = browser_->profile()->GetHistoryService(Profile::EXPLICIT_ACCESS);
      HistoryService::Handle handle = hs->QueryURL(*iterator, true, &consumer_,
                                                   NewCallback(this, &BrowserServiceWrapper::RemoveURLItem));
    }
  }
}

void BrowserServiceWrapper::RemoveUrl(std::string url_string)
{
  GURL url(url_string);

  HistoryService* hs = browser_->profile()->GetHistoryService(Profile::EXPLICIT_ACCESS);
  hs->DeleteURL(url);
}

void BrowserServiceWrapper::RemoveBookmark(std::string id_string)
{
  int64 id;
  
  base::StringToInt64(id_string, &id);
  BookmarkModel* model = browser_->profile()->GetBookmarkModel();

  const BookmarkNode* node = model->GetNodeByID(id);
  if (!node) {
    return;
  }
  if (node == model->root_node() ||
      node == model->other_node() ||
      node == model->GetBookmarkBarNode()) {
    return;
  }

  const BookmarkNode* parent = node->parent();
  int index = parent->GetIndexOf(node);
  model->Remove(parent, index);
}

void BrowserServiceWrapper::SelectTabByUrl(std::string url_string)
{
  TabStripModel* model = browser_->tabstrip_model();
  GURL url(url_string);

  for (int i = 0; i < model->count(); i++)
  {
    TabContents* contents = model->GetTabContentsAt(i)->tab_contents();
    if (contents && contents->GetURL() == url)
    {
      model->ActivateTabAt(i, true);
      return;
    }
  }

  browser_->AddSelectedTabWithURL(url, PageTransition::LINK);
}

void BrowserServiceWrapper::updateCurrentTab()
{
    int index = browser_->active_index();
    TabContents* contents = browser_->GetTabContentsAt(index);
    UpdateTabInfo(contents);
}

void BrowserServiceWrapper::showBrowser(const char *mode, const char *target)
{
    if(mode == NULL || target == NULL) return;

    // Show browser window
    BrowserWindow* window = browser_->window();
    if(window) window->Show();
    
    string16 search_term;
    UTF8ToUTF16(target, strlen(target), &search_term);
    GURL url;
    Profile * profile = browser_->profile();

    if(!strcmp(mode, "selecttab")) {
      int index = atoi(target);
      if(index >= 0 && index < browser_->tab_count()) 
      {
        browser_->ActivateTabAt(index, true);
      }
      return;
    }

    if (!strcmp(mode, "gotourl")) {
        scoped_ptr<AutocompleteController> controller( new AutocompleteController(profile, NULL) );
        controller->Start(search_term, string16(), false, false, false, AutocompleteInput::SYNCHRONOUS_MATCHES);
        const AutocompleteResult * result = &controller->result();
        //url = result->match_at(0).destination_url;
        AutocompleteResult::const_iterator itr = result->begin();
        for (; itr != result->end(); itr ++) {
            if (itr->type == AutocompleteMatch::URL_WHAT_YOU_TYPED) {
                url = itr->destination_url;
                break;
            }
        }
        if (itr == result->end())
            url = result->default_match()->destination_url;
    }else if (!strcmp(mode, "search")) {
        const TemplateURL* default_provider = profile->GetTemplateURLModel()->GetDefaultSearchProvider();
        if (!default_provider || !default_provider->url()) {
            return;
        }
        const TemplateURLRef* search_url = default_provider->url();
        DCHECK(search_url->SupportsReplacement());
        url = GURL(search_url->ReplaceSearchTerms(*default_provider, search_term,
                TemplateURLRef::NO_SUGGESTIONS_AVAILABLE, string16()));
    }
    
    int tab_count = browser_->tab_count();
    if(url.HostNoBrackets() == "newtab") {
      browser_->ShowSingletonTab(GURL(chrome::kChromeUINewTabURL));
    } else {
      browser_->OpenURL(url, GURL(""), NEW_FOREGROUND_TAB, PageTransition::TYPED);
    }
}

void BrowserServiceWrapper::closeTab(int index)
{
  if (browser_->CanCloseTab() && browser_->tab_count() > index) {

    TabStripModel* model = browser_->tabstrip_model();
    if (model->count() == 1) {
      // the last one
      model->delegate()->AddBlankTab(true);
    }
    model->CloseTabContentsAt(index, TabStripModel::CLOSE_CREATE_HISTORICAL_TAB);
  }
}

int BrowserServiceWrapper::getCurrentTabIndex()
{
    return browser_->active_index();
}
