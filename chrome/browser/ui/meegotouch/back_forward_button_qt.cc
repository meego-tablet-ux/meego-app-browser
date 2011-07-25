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

#include "chrome/browser/ui/meegotouch/back_forward_button_qt.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"
#include "chrome/browser/ui/meegotouch/browser_toolbar_qt.h"
#include "chrome/browser/ui/meegotouch/new_tab_ui_qt.h"
#include "base/utf_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/rect.h"
#include "base/string_number_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/metrics/user_metrics.h"
#include "content/browser/tab_contents/navigation_controller.h"
#include "content/browser/tab_contents/navigation_entry.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "content/common/notification_service.h"
#include "chrome/common/url_constants.h"
#include "grit/generated_resources.h"
#include "grit/theme_resources.h"
#include "net/base/registry_controlled_domain.h"
#include "chrome/browser/history/history.h"
#include "chrome/browser/history/recent_and_bookmark_thumbnails_qt.h"
#include "chrome/browser/prefs/pref_service.h"
#include "chrome/browser/profiles/profile.h"

#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QVector>
#include <QList>
#include <QTimer>
#include <algorithm>
#include <QGraphicsSceneResizeEvent>
#include <QGraphicsWidget>
#include <QEvent>
#include <QDebug>
//#include <QTapAndHoldGesture>

#include <QDeclarativeEngine>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDeclarativeItem>
#include <QDeclarativeImageProvider>
#include <QAbstractListModel>

// Image provider, managing to provide image to QML
// for history view
// Due to qml limitation, the id of image is suffixed with
// a static increaser, which forces using new image source
// each time
class HistoryImageProvider : public QDeclarativeImageProvider 
{
public:
    HistoryImageProvider()
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
        DLOG(INFO) << "requesting image id: " << id.toStdString();
        int finded = id.indexOf("_");
        if (finded != -1) {
            QImage& image = imageList_[id.left(finded)];
            if (!image.isNull()) {
                //QImage scaled = image.scaled(requestedSize);
                if (size) {
                  *size = image.size();
                }
                return image;
            }
        }
        QImage ret(kThumbnailWidth,kThumbnailHeight, QImage::Format_RGB32);
        ret.fill(0xFFFFFF);
        if (size) {
          *size = ret.size();
        }
        return ret;
    }

    // add a new image
    void addImage(const QString& id, const QImage &image)
    {
        imageList_.insert(id, image);
    }

private:
    static const int kThumbnailWidth = 212;
    static const int kThumbnailHeight = 132;
    QMap<QString, QImage> imageList_;
};

class HistoryStackModel;

// history entry to represent one history data
class HistoryEntry 
{
public:
  
    // constructor
    // @param index the index in history stack
    // @param hiProvider image provider for history stack
    // @param entry representation of navigation entry in browser
    // @param controller current navigation controller in browser
    // @param model history stack model
    HistoryEntry(const int index, 
                 HistoryImageProvider& hiProvider,
                 NavigationEntry* entry,
                 NavigationController* controller,
                 HistoryStackModel* model)
        : index_(index), hiProvider_(hiProvider), entry_(entry), model_(model)
    {
        // get image
        getThumbnailData(controller);
        std::wstring title_str = UTF16ToWide(entry->title());
        title_ = QString::fromStdWString(title_str);
    }

    void imgURLGen() 
    {
        static QString prefix("image://historystack/");
        imageSrc_ = prefix + QString::number(index_) + "_" + QString::number(reloadNumber_);
    }

    // get thumbnail
    void getThumbnailData(NavigationController *controller);

    // callback to get the image data from browser
    void onThumbnailDataAvailable(HistoryService::Handle request_handle,
                                  scoped_refptr<RefCountedBytes> jpeg_data); 


    NavigationEntry* entry() { return entry_; }
    QString image() { return imageSrc_; }
    QString title() { return title_; }

    static void incReloadNumber() { reloadNumber_++; }
    static unsigned long reloadNumber() { return reloadNumber_; }

private:
    // static increaser to generate unique image source 
    static unsigned long reloadNumber_;

    int index_;
    HistoryImageProvider& hiProvider_;
    NavigationEntry* entry_;
    HistoryStackModel* model_;
    // image source
    QString imageSrc_;
    // history title
    QString title_;
    CancelableRequestConsumer consumer_;
};

unsigned long HistoryEntry::reloadNumber_ = 0;

// represent list model used in QML to store history data
class HistoryStackModel: public QAbstractListModel
{
    Q_OBJECT
    enum HistoryRole {
        IMAGE_ROLE = Qt::UserRole + 1,
        TITLE_ROLE
    };

public:
    HistoryStackModel(BackForwardButtonQtImpl* backForward)
        : back_forward_(backForward), returnedImages_(0)
    {
        QHash<int, QByteArray> roles;
        roles[IMAGE_ROLE] = "thumbSrc";
        roles[TITLE_ROLE] = "title";
        setRoleNames(roles);
    }

    ~HistoryStackModel()
    {
        clear();
    }

    // clear saved data
    void clear()
    {
        returnedImages_ = 0;
        beginResetModel();
        for(int i = 0; i < entryList_.count(); i++) {
            delete entryList_[i];
        }
        entryList_.clear();
        hiProvider_.clear();
        endResetModel();
    }

    // wrapper to check whether browser returns all images of history
    // so we can reset model to avoid resetting model many times
    void beginReset()
    {
        // begin reset if necessary
        returnedImages_++;
        if (returnedImages_ == rowCount()) {
            DLOG(INFO) << "begin reset history stack model";
            // generate new number to create new image url
            HistoryEntry::incReloadNumber();
            for(int i = 0; i < entryList_.count(); i++) {
                HistoryEntry* entry = entryList_[i];
                entry->imgURLGen();
            }
            beginResetModel();
        }
    }

    void endReset()
    {
        // end reset if necessary
        if (returnedImages_ == rowCount()) {
            DLOG(INFO) << "end reset history stack model";
            endResetModel();
        }
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const
    {
        return entryList_.count();
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const
    {
        DLOG(INFO) << "read list model data: row = " << index.row() << ", column = " << index.column();
        if(index.row() < 0 || index.row() > entryList_.count())
            return QVariant();
        HistoryEntry* entry = entryList_[index.row()];
        switch(role) {
            case IMAGE_ROLE:
                return entry->image();
            case TITLE_ROLE:
                return entry->title();
            default:
                return QVariant();
        }
    }

    void appendEntry(NavigationController* controller, NavigationEntry *entry)
    {
        HistoryEntry* historyEntry 
                                = new HistoryEntry(entryList_.count(),
                                                   hiProvider_,
                                                   entry, 
                                                   controller,
                                                   this);
        historyEntry->imgURLGen();
        beginInsertRows(QModelIndex(), rowCount(), rowCount());
        entryList_.push_back(historyEntry);
        endInsertRows();
    }

    // emit a showHistory signal to QML
    void show();

    // emit a hideHistory signal to QML
    void hide() { emit hideHistory(); }

    // emit a current selected(focused) history entry
    void setCurrent(int index) { emit current(index); }

    HistoryImageProvider* hiProvider() { return &hiProvider_; }

Q_SIGNALS:
    // three signals used to notify QML
    void showHistory();
    void hideHistory();
    void current(int index);

public Q_SLOTS:
    // open specific page, called by QML element
    // It's called by QML element when user clicks one history entry in QML view
    void openPage(const int index);
    // hide Overlay, called from QML
    void OnOverlayHide();

private:
    BackForwardButtonQtImpl* back_forward_;
    // list to hold entries
    QList<HistoryEntry*> entryList_;
    HistoryImageProvider hiProvider_;
    // count of returned images from browser
    int returnedImages_;
};

class BackForwardButtonQtImpl
{
public:
    enum NaviState {
        ONLY_BACK = 0,
        ONLY_FORWARD,
        BACK_FORWARD
    };

    // constructor
    BackForwardButtonQtImpl(BrowserToolbarQt* toolbar, Browser* browser, BrowserWindowQt* window)
        :toolbar_(toolbar), browser_(browser), model_(this), 
        state_(ONLY_BACK), active_(false)
    {
        QDeclarativeView* view = window->DeclarativeView();
        QDeclarativeContext *context = view->rootContext();
        context->setContextProperty("historyStackModel", &model_);
        context->engine()->addImageProvider(QLatin1String("historystack"), model_.hiProvider());
    }

    NavigationController* currentController()
    {
        return &(browser_->GetSelectedTabContents()->controller());
    }

    void openPage(NavigationEntry *entry)
    {
        currentController()->GoToIndex(currentController()->GetIndexOfEntry(entry));
        updateStatus();
        // In case of opening a new page which has no render process
        // We need to show new tab and update title bar manually due
        // to there is no title update IPC message received.
        if(entry->url() == GURL(chrome::kChromeUINewTabURL)) {
          BrowserWindowQt* window = static_cast<BrowserWindowQt*>(browser_->window());
          NewTabUIQt* new_tab = window->GetNewTabUIQt();
          new_tab->AboutToShow();
          window->UpdateTitleBar();
        }
    }


    void updateStatus()
    {
        if(currentController()->GetCurrentEntryIndex() == currentController()->entry_count() - 1)
        {
            state_ = ONLY_BACK;
            if (currentController()->entry_count() > 1) {
                active_ = true;
            } else {
                active_ = false;
            }
        } else if (currentController()->GetCurrentEntryIndex() == 0
                   && currentController()->entry_count() > 0) {
            state_ = ONLY_FORWARD;
        } else if (currentController()->CanGoBack() && currentController()->CanGoForward()) {
            state_ = BACK_FORWARD;
        } else {
            state_ = ONLY_BACK;
            active_ = false;
        }
        // update button status in qml
        updateButton();
        DLOG(INFO) << "In C++, updateStatus is invoked\n";
    }

    // update back-forward button icon
    void updateButton() { toolbar_->updateBfButton((int)state_, active_); }

    void tap()
    {
        DLOG(INFO) << "In C++, tap is invoked";
        switch(state_) {
        case ONLY_BACK:
            if (currentController()->CanGoBack()) {
                currentController()->GoBack();
            }
            break;
        case ONLY_FORWARD:
            if (currentController()->entry_count() == 2) {
                if (currentController()->CanGoForward()) {
                    currentController()->GoForward();
                }
            } else {
                prepareAndShowHistory();
            }
            break;
        case BACK_FORWARD:
            prepareAndShowHistory();
            break;
        default:
            break;
        }
    }

    void tapAndHold()
    {
        switch(state_) {
        case ONLY_BACK:
            prepareAndShowHistory();
            break;
        case ONLY_FORWARD:
            if (currentController()->entry_count() == 2) {
                if (currentController()->CanGoForward()) {
                    currentController()->GoForward();
                }
            } else {
                prepareAndShowHistory();
            }
            break;
        case BACK_FORWARD:
                prepareAndShowHistory();
            break;
        default:
            break;
        }
        updateStatus();
    }

    //prepare c++ list model and emit signal to QML to show it
    void prepareAndShowHistory()
    {
        model_.clear();
        int count = currentController()->entry_count();
        int curr = -1;
        HistoryEntry::incReloadNumber();
        for(int i = count - 1; i >= 0; i--)
        {
            DLOG(INFO) << "page index: ---" << i << "---\n";
            NavigationEntry* navEntry = currentController()->GetEntryAtIndex(i);
            // don't skip 'newtab' now, if yes, we do not only skip newtab here
            // but also skip count calculation for 'updateStatus'
            /*
            if (navEntry->url().HostNoBrackets() == "newtab") {
                // skip 'newtab'
                continue;
            }*/
            model_.appendEntry(currentController(), navEntry);
            curr++;
            if(currentController()->GetCurrentEntryIndex() == i) {
                model_.setCurrent(curr);
            }
        }
        model_.show();
        toolbar_->showHistory(count);
    }

    void ComposeEmbededFlashWindow(const gfx::Rect& rect)
    {
        BrowserWindowQt* window = static_cast<BrowserWindowQt*>(browser_->window());
        window->ComposeEmbededFlashWindow(rect);
    }

    void ReshowEmbededFlashWindow()
    {
        BrowserWindowQt* window = static_cast<BrowserWindowQt*>(browser_->window());
        window->ReshowEmbededFlashWindow();
    }

private:
    BrowserToolbarQt* toolbar_;
    Browser* browser_;
    HistoryStackModel model_;

    // current navigation state
    NaviState state_;
    // whether the backward/forward button is active
    bool active_;
};

BackForwardButtonQt::BackForwardButtonQt(BrowserToolbarQt* toolbar, Browser* browser, BrowserWindowQt* window)
{
  impl_ = new BackForwardButtonQtImpl(toolbar, browser, window);
}

BackForwardButtonQt::~BackForwardButtonQt()
{
  delete impl_;
}

void BackForwardButtonQt::tap()
{
  impl_->tap();
}

void BackForwardButtonQt::tapAndHold()
{
  impl_->tapAndHold();
}

void BackForwardButtonQt::updateStatus()
{
  impl_->updateStatus();
}

void HistoryEntry::onThumbnailDataAvailable(HistoryService::Handle request_handle,
                                            scoped_refptr<RefCountedBytes> jpeg_data) 
{
    model_->beginReset();
    if (jpeg_data.get()) {
        DLOG(INFO) << "get image id: " << index_;
        std::vector<unsigned char> thumbnail_data;
        std::copy(jpeg_data->data.begin(), jpeg_data->data.end(),
                std::back_inserter(thumbnail_data));
        QImage image = QImage::fromData(thumbnail_data.data(), thumbnail_data.size());
        hiProvider_.addImage(QString::number(index_), image);
    }
    model_->endReset();
}

void HistoryEntry::getThumbnailData(NavigationController *controller)
{
  history::TopSites* ts = controller->profile()->GetTopSites();
  if (ts) {
    scoped_refptr<RefCountedBytes> thumbnail_data;
    ts->GetPageThumbnail(entry_->url(), &thumbnail_data);
    if (thumbnail_data.get()) {
      //Also update the count for the thumbnail from TopSites
      model_->beginReset();
      std::vector <unsigned char> jpeg;
      std::copy(thumbnail_data->data.begin(), 
                thumbnail_data->data.end(),
                std::back_inserter(jpeg));
      QImage image = QImage::fromData(jpeg.data(), jpeg.size());
      DLOG(INFO) << "image size ===== " << jpeg.size();
      hiProvider_.addImage(QString::number(index_), image);
      model_->endReset();
      return;
    }

    history::RecentAndBookmarkThumbnailsQt * recentThumbnails =
                             ts->GetRecentAndBookmarkThumbnails();
    if(recentThumbnails) {
      recentThumbnails->GetRecentPageThumbnail(entry_->url(), &consumer_,
                     NewCallback(static_cast<HistoryEntry*>(this),
                     &HistoryEntry::onThumbnailDataAvailable));
    }
  } else {
     HistoryService* hs = controller->profile()->GetHistoryService(Profile::EXPLICIT_ACCESS);
     hs->GetPageThumbnail(entry_->url(), 
                          &consumer_,
                          NewCallback(static_cast<HistoryEntry*>(this),
                                      &HistoryEntry::onThumbnailDataAvailable));
  }
}

void HistoryStackModel::openPage(const int index)
{
    back_forward_->openPage(entryList_[index]->entry());
    hide();

    back_forward_->ReshowEmbededFlashWindow();
}

void HistoryStackModel::show() {
    // TODO: compose embeded flash window with correct rect
    gfx::Rect rect(0, 0, 0, 0);
    back_forward_->ComposeEmbededFlashWindow(rect);

    emit showHistory();
}

void HistoryStackModel::OnOverlayHide() {
    back_forward_->ReshowEmbededFlashWindow();
}

#include "moc_back_forward_button_qt.cc"
