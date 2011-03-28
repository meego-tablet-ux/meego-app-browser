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
#include "base/string_util.h"
#include "base/stringprintf.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "chrome/browser/ui/tab_contents/tab_contents_wrapper.h"

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
  void AddThumbnailItemImpl(GURL url, scoped_refptr<RefCountedBytes> jpeg_data);
  void RemoveTabItemImpl(int index);
  void AddTabItemImpl(int tab_id, int win_id, std::string url, std::string title, std::string faviconUrl);
  void RemoveBookmarkItemImpl(int64 id);
  void AddBookmarkItemImpl(int64 id, std::string url,
                           std::string title, std::string faviconUrl, long long dataAdded);
  void RemoveAllURLsImpl();

 private:
  friend class base::RefCountedThreadSafe<BrowserServiceBackend>;

  ~BrowserServiceBackend() {}
  
  MeeGoPluginAPI* plugin_;
};

BrowserServiceWrapper* BrowserServiceWrapper::GetInstance() {
  return Singleton<BrowserServiceWrapper>::get();
}

BrowserServiceWrapper::BrowserServiceWrapper():
    backend_(NULL),
    browser_(NULL),
    factory_(this)
{
}

BrowserServiceWrapper::~BrowserServiceWrapper()
{
  backend_->Release();
  ///\todo: unregister observers
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

  BrowserThread::PostTask(BrowserThread::DB, FROM_HERE, NewRunnableMethod(
      backend_, &BrowserServiceBackend::InitBackend, this));
}

void BrowserServiceBackend::InitBackend(BrowserServiceWrapper* wrapper)
{
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  plugin_ = new MeeGoPluginAPI(wrapper);
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

void BrowserServiceBackend::AddBookmarkItemImpl(int64 id, std::string url,
                                                std::string title, std::string faviconUrl, long long dataAdded)
{
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  plugin_->addBookmarkItem(id, url, title, faviconUrl, dataAdded);
}

void BrowserServiceWrapper::BookmarkNodeRemoved(
    BookmarkModel* model,
    const BookmarkNode* parent,
    int index,
    const BookmarkNode* node) {
  BrowserThread::PostTask(BrowserThread::DB, FROM_HERE, NewRunnableMethod(
      backend_, &BrowserServiceBackend::RemoveBookmarkItemImpl, node->id()));
}

void BrowserServiceBackend::RemoveBookmarkItemImpl(int64 id)
{
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  plugin_->removeBookmarkItem(id);
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

void BrowserServiceWrapper::TabInsertedAt(TabContents* contents,
                                          int index,
                                          bool foreground) {
  BrowserThread::PostTask(BrowserThread::DB, FROM_HERE, NewRunnableMethod(
      backend_, &BrowserServiceBackend::AddTabItemImpl,
      index, 0, contents->GetURL().spec(), UTF16ToUTF8(contents->GetTitle()), contents->GetURL().HostNoBrackets()));  
}

void BrowserServiceBackend::AddTabItemImpl(int tab_id, int win_id, std::string url, std::string title, std::string faviconUrl)
{
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  plugin_->addTabItem(tab_id, win_id, url, title, faviconUrl);
}

void BrowserServiceWrapper::TabDetachedAt(TabContents* contents,
                                          int index) {
  BrowserThread::PostTask(BrowserThread::DB, FROM_HERE, NewRunnableMethod(
      backend_, &BrowserServiceBackend::RemoveTabItemImpl, index));  
}

void BrowserServiceBackend::RemoveTabItemImpl(int index)
{
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  plugin_->removeTabItem(index);
}

void BrowserServiceWrapper::TabClosingAt(TabContents* contents,
                                         int index) {
}

void BrowserServiceWrapper::TabSelectedAt(TabContents* old_contents,
                                                TabContents* new_contents,
                                                int index,
                                                bool user_gesture) {
}

void BrowserServiceWrapper::TabMoved(TabContents* contents,
                                           int from_index,
                                           int to_index) {
}

void BrowserServiceWrapper::OnThumbnailDataAvailable(
    HistoryService::Handle handle,
    scoped_refptr<RefCountedBytes> jpeg_data) {
  GURL* url =
      consumer_.GetClientData(
          browser_->profile()->GetHistoryService(Profile::EXPLICIT_ACCESS), handle);
  if (jpeg_data.get() && jpeg_data->data.size()) {
    BrowserThread::PostTask(BrowserThread::DB, FROM_HERE, NewRunnableMethod(
        backend_, &BrowserServiceBackend::AddThumbnailItemImpl, *url, jpeg_data));    
  }
  delete url;
}

void BrowserServiceBackend::AddThumbnailItemImpl(GURL url, scoped_refptr<RefCountedBytes> jpeg_data)
{
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  plugin_->addThumbnailItem(url.spec(), base::Time::Now().ToInternalValue(),
                            reinterpret_cast<const unsigned char*>(&jpeg_data->data[0]),
                            jpeg_data->data.size());
}

void BrowserServiceWrapper::OnFaviconDataAvailable(
    FaviconService::Handle handle,
    history::FaviconData favicon) {
  GURL* url =
      consumer_.GetClientData(
          browser_->profile()->GetFaviconService(Profile::EXPLICIT_ACCESS), handle);
/* TODO: need rework on new API
  SkBitmap fav_icon;
  if (know_favicon && png_data.get() && png_data->size()) {
    BrowserThread::PostTask(BrowserThread::DB, FROM_HERE, NewRunnableMethod(
        backend_, &BrowserServiceBackend::AddFavIconItemImpl, *url, png_data));    
  }
*/
  delete url;
}

void BrowserServiceBackend::AddFavIconItemImpl(GURL url, scoped_refptr<RefCountedMemory> png_data)
{
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::DB));
  plugin_->addFavIconItem(url.HostNoBrackets(), base::Time::Now().ToInternalValue(),
                          png_data->front(), png_data->size());
}

void BrowserServiceWrapper::GetThumbnail(GURL url)
{
  HistoryService* hs = browser_->profile()->GetHistoryService(Profile::EXPLICIT_ACCESS);
  if (hs) {
    HistoryService::Handle handle = hs->GetPageThumbnail(url, &consumer_,
                                                         NewCallback(static_cast<BrowserServiceWrapper*>(this),
                                                                     &BrowserServiceWrapper::OnThumbnailDataAvailable));
    consumer_.SetClientData(hs, handle, new GURL(url));
  }
}

void BrowserServiceWrapper::GetFavIcon(GURL url)
{
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

void BrowserServiceWrapper::TabChangedAt(TabContents* contents,
                                         int index,
                                         TabChangeType change_type) {
  if (contents->is_loading())
    return;

  // load completed
  GURL url = contents->GetURL();
  MessageLoop::current()->PostDelayedTask(FROM_HERE,
                                          factory_.NewRunnableMethod(&BrowserServiceWrapper::GetThumbnail, url),
                                          500);
  MessageLoop::current()->PostDelayedTask(FROM_HERE,
                                          factory_.NewRunnableMethod(&BrowserServiceWrapper::GetFavIcon, url),
                                          500);

  HistoryService* hs = browser_->profile()->GetHistoryService(Profile::EXPLICIT_ACCESS);
  hs->QueryURL(contents->GetURL(), true, &consumer_,
               NewCallback(this, &BrowserServiceWrapper::AddURLItem));

}

void BrowserServiceWrapper::TabReplacedAt(TabContents* old_contents,
                                          TabContents* new_contents,
                                          int index)
{
}

void BrowserServiceWrapper::TabStripEmpty()
{
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
    default:
      return;
  }
}

void BrowserServiceWrapper::HistoryUrlVisited(
    const history::URLVisitedDetails* details) {
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
      model->SelectTabContentsAt(i, true);
      return;
    }
  }

  browser_->AddSelectedTabWithURL(url, PageTransition::LINK);
}
