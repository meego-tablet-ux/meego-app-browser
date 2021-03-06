#ifndef BROWSER_SERVICE_WRAPPER_H
#define BROWSER_SERVICE_WRAPPER_H

#include <map>

#include "base/memory/singleton.h"
#include "chrome/browser/bookmarks/bookmark_model_observer.h"
#include "content/common/notification_observer.h"
#include "content/common/notification_registrar.h"
#include "chrome/browser/favicon_service.h"
#include "chrome/browser/history/history.h"
#include "chrome/browser/history/history_types.h"
#include "chrome/browser/history/history_notifications.h"
#include "chrome/browser/tabs/tab_strip_model_observer.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window.h"
#include "content/browser/tab_contents/navigation_entry.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "chrome/browser/tabs/tab_strip_model.h"
#include "chrome/browser/bookmarks/bookmark_codec.h"
#include "chrome/browser/bookmarks/bookmark_html_writer.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "chrome/browser/bookmarks/bookmark_utils.h"
#include "base/memory/ref_counted.h"
#include "content/browser/cancelable_request.h"
#include "content/browser/browser_thread.h"
#include "googleurl/src/gurl.h"
#include <QList>
#include <QMap>

class BrowserServiceBackend;
class SnapshotTaker;

class BrowserServiceWrapper :
         public BookmarkModelObserver,
         public NotificationObserver,
         public TabStripModelObserver
{
public:
  virtual ~BrowserServiceWrapper();
  // Singleton
  static BrowserServiceWrapper* GetInstance();

  // Must be called once. Subsequent calls have no effect.
  void Init(Browser* browser);

  
  // TabStripModelObserver
  virtual void TabInsertedAt(TabContentsWrapper* contents, int index, bool foreground);
  virtual void TabClosingAt(TabStripModel* tab_strip_model,
			    TabContentsWrapper* contents, int index);
  virtual void TabDetachedAt(TabContentsWrapper* contents, int index);
  virtual void TabDeselected(TabContents* content);
  virtual void TabSelectedAt(TabContentsWrapper* old_contents,
                             TabContentsWrapper* new_contents,
                             int index,
                             bool user_gesture);
  virtual void TabMoved(TabContentsWrapper* contents, int from_index, int to_index);
  virtual void TabChangedAt(TabContentsWrapper* contents, int index,
                            TabChangeType change_type);
  virtual void TabReplacedAt(TabStripModel* tab_strip_model,
			     TabContentsWrapper* old_contents,
                             TabContentsWrapper* new_contents,
                             int index);
  virtual void TabStripEmpty();

  // NotificationObserver.
  void Observe(NotificationType type,
               const NotificationSource& source,
               const NotificationDetails& details);

    // BookmarkModelObserver
  virtual void Loaded(BookmarkModel* model);
  virtual void BookmarkModelBeingDeleted(BookmarkModel* model) { }
  virtual void BookmarkNodeMoved(BookmarkModel* model,
                                 const BookmarkNode* old_parent,
                                 int old_index,
                                 const BookmarkNode* new_parent,
                                 int new_index);
  virtual void BookmarkNodeAdded(BookmarkModel* model,
                                 const BookmarkNode* parent,
                                 int index);
  virtual void BookmarkNodeRemoved(BookmarkModel* model,
                                   const BookmarkNode* parent,
                                   int old_index,
                                   const BookmarkNode* node);
  virtual void BookmarkNodeChanged(BookmarkModel* model,
                                   const BookmarkNode* node);
  virtual void BookmarkNodeFaviconLoaded(BookmarkModel* model,
                                         const BookmarkNode* node);
  virtual void BookmarkNodeChildrenReordered(BookmarkModel* model,
                                             const BookmarkNode* node);
  virtual void BookmarkImportBeginning(BookmarkModel* model);
  virtual void BookmarkImportEnding(BookmarkModel* model);

  // for plugin
  void RemoveUrl(std::string url);
  void RemoveBookmark(std::string id);
  void SelectTabByUrl(std::string url);

  void updateCurrentTab();
  void showBrowser(const char * mode, const char *target);
  void closeTab(int index);
  int getCurrentTabIndex();

  void AddOpenedTab();

  // internal
  void HistoryUrlVisited(
      const history::URLVisitedDetails* details);
  void HistoryUrlsRemoved(
    const history::URLsDeletedDetails* details);
  void RemoveURLItem(HistoryService::Handle handle,
                     bool success,
                     const history::URLRow* row,
                     history::VisitVector* visit_vector);
  void AddURLItem(HistoryService::Handle handle,
                     bool success,
                     const history::URLRow* row,
                     history::VisitVector* visit_vector);
  void OnThumbnailDataAvailable(
    HistoryService::Handle request_handle,
    scoped_refptr<RefCountedBytes> jpeg_data);
  void OnFaviconDataAvailable(
    FaviconService::Handle handle,
    history::FaviconData favicon);

  void InitBottomHalf();
  void GetThumbnail(TabContents* contents, const GURL& url, int index);
  void GetFavIcon(const GURL &url);
  void OnBrowserClosing();
  void OnBrowserWindowMinimized(BrowserWindow* window);
  void ReloadTabList();
  void UpdateTabListAndThumbnails(bool update_thumbnail = true);
  void UpdateTabInfo(TabContents* contents, bool update_thumbnail = true);

private:
  void ClearSnapshotList();

  ScopedRunnableMethodFactory<BrowserServiceWrapper> factory_;
  BrowserServiceWrapper();
  friend struct DefaultSingletonTraits<BrowserServiceWrapper>;

  BrowserServiceBackend* backend_;
  Browser* browser_;

  NotificationRegistrar registrar_;

  CancelableRequestConsumerTSimple<GURL*> consumer_;

  QList<SnapshotTaker*> snapshotList_;

  QMap<GURL, qint64> url2timestamp_; 
  QList<GURL> urlcaptured_;
  bool onBrowserClosingCalled_;
};
#endif
