// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Copyright (c) 2010, Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/meegotouch/bookmark_bubble_qt.h"


#include "ui/base/l10n/l10n_util.h"
#include "base/basictypes.h"
#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/string16.h"
#include "base/utf_string_conversions.h"
#include "chrome/browser/bookmarks/bookmark_editor.h"
#include "chrome/browser/bookmarks/bookmark_model.h"
#include "chrome/browser/bookmarks/bookmark_utils.h"
#include "chrome/browser/bookmarks/recently_used_folders_combo_model.h"
#include "chrome/browser/metrics/user_metrics.h"
#include "chrome/browser/profiles/profile.h"
#include "content/browser/tab_contents/tab_contents.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tab_contents/tab_contents_wrapper.h"
#include "content/common/notification_service.h"
#include "grit/generated_resources.h"
#include <QTextCodec>
#include <QString>
#include <QObject>
#include <QDeclarativeEngine>
#include <QDeclarativeView>
#include <QDeclarativeContext>
#include <QDeclarativeItem>
#include <QGraphicsLineItem>
#include <QVariant>
#include <QAbstractListModel>
#include <QList>

namespace {

// Padding between content and edge of info bubble.
const int kContentBorder = 7;


}  // namespace
class BookmarkBubbleQtImpl : public QObject
{
  Q_OBJECT
  public:
    BookmarkBubbleQtImpl(BookmarkBubbleQt* bubble):
        QObject(NULL),
        bubble_(bubble)
    {
    }
    void PopupAt(int x, int y)
    {
       emit popupAt(x, y);
    }   
  public slots:
    void doneButtonClicked()
    {
       bubble_->OnDoneClicked();
       emit close();
    }
   
    void removeButtonClicked()
    {
       bubble_->OnRemoveClicked();
       emit close();
    }

    void folderSelectedIndex(int index)
    {
       bubble_->SetFolderIndex(index);
    }
  
    void setTitle(QString title)
    {
       bubble_->SetTitle(title);
    }
  
    void cancel()
    {
       bubble_->Cancel();
    } 
  Q_SIGNALS:
    void popupAt(int x, int y);
    void close();
  private:
    BookmarkBubbleQt* bubble_;
};
/*void BookmarkBubbleQt::InfoBubbleClosing(InfoBubbleGtk* info_bubble,
                                          bool closed_by_escape) {
  if (closed_by_escape) {
    remove_bookmark_ = newly_bookmarked_;
    apply_edits_ = false;
  }

  NotificationService::current()->Notify(
      NotificationType::BOOKMARK_BUBBLE_HIDDEN,
      Source<Profile>(profile_->GetOriginalProfile()),
      NotificationService::NoDetails());
}*/

void BookmarkBubbleQt::Observe(NotificationType type,
                                const NotificationSource& source,
                                const NotificationDetails& details) {

}

BookmarkBubbleQt::BookmarkBubbleQt( BrowserWindowQt* window,
                                    Browser* browser,
                                     Profile* profile)
    : browser_(browser),
      profile_(profile),
      window_(window){
  impl_ = new BookmarkBubbleQtImpl(this);
  QDeclarativeView *view = window_->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();
  context->setContextProperty("bookmarkBubbleObject", impl_);

}

BookmarkBubbleQt::BookmarkBubbleQt( BrowserWindowQt* window,
                                    Browser* browser,
                                     Profile* profile,
                                     const GURL& url,
                                     bool already_bookmarked)
    : url_(url),
      browser_(browser),
      profile_(profile),
      window_(window),
      newly_bookmarked_(!already_bookmarked),
      apply_edits_(true),
      remove_bookmark_(false){

  impl_ = new BookmarkBubbleQtImpl(this);
  InitFolderComboModel();


  QDeclarativeView *view = window_->DeclarativeView();
  QDeclarativeContext *context = view->rootContext();
  context->setContextProperty("bookmarkBubbleObject", impl_);

  QString q_title;
  if (newly_bookmarked_) {
    q_title = QString::fromUtf8(l10n_util::GetStringUTF8(IDS_BOOMARK_BUBBLE_PAGE_BOOKMARKED).c_str());
  } else {
    q_title = QString::fromUtf8(l10n_util::GetStringUTF8(IDS_BOOMARK_BUBBLE_PAGE_BOOKMARK).c_str());
  }
  context->setContextProperty("bubbleTitle", q_title);
  
  QString q_name = QString::fromUtf8(l10n_util::GetStringUTF8(IDS_BOOMARK_BUBBLE_TITLE_TEXT).c_str());
  context->setContextProperty("bubbleName", q_name);
  QString q_folder = QString::fromUtf8(l10n_util::GetStringUTF8(IDS_BOOMARK_BUBBLE_FOLDER_TEXT).c_str());
  context->setContextProperty("bubbleFolder", q_folder);
  QString q_done = QString::fromUtf8(l10n_util::GetStringUTF8(IDS_DONE).c_str());
  context->setContextProperty("bubbleDone", q_done);
  QString q_remove = QString::fromUtf8(l10n_util::GetStringUTF8(IDS_BOOMARK_BUBBLE_REMOVE_BOOKMARK).c_str());
  context->setContextProperty("bubbleRemove", q_remove);

//  context->setContextProperty("bubbleModel", impl_);
  context->setContextProperty("bubbleFolderModel", QVariant::fromValue(folderList_));

  BookmarkModel* model = profile_->GetBookmarkModel();
  const BookmarkNode* node = model->GetMostRecentlyAddedNodeForURL(url_);
  const std::string& title = UTF16ToUTF8(node->GetTitle());
  QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
  QString q_node_title = QString::fromStdString(title);
  context->setContextProperty("bubbleNameInput", q_node_title);
  
  const BookmarkNode* parent = node->parent();
  const std::string& folder = UTF16ToUTF8(parent->GetTitle());
  QString q_node_folder = QString::fromStdString(folder);
  context->setContextProperty("bubbleFolderInput", q_node_folder);
}

BookmarkBubbleQt::~BookmarkBubbleQt() {
  delete impl_;
}

void BookmarkBubbleQt::Apply() {
  if (apply_edits_) {
    ApplyEdits();
  } else if (remove_bookmark_){
    BookmarkModel* model = profile_->GetBookmarkModel();
    const BookmarkNode* node = model->GetMostRecentlyAddedNodeForURL(url_);
    if (node)
      model->Remove(node->parent(), node->parent()->GetIndexOf(node));
  }
}

void BookmarkBubbleQt::Cancel(){
  BookmarkModel* model = profile_->GetBookmarkModel();
  const BookmarkNode* node = model->GetMostRecentlyAddedNodeForURL(url_);
  if (node && newly_bookmarked_)
    model->Remove(node->GetParent(), node->GetParent()->IndexOfChild(node));
}

void BookmarkBubbleQt::OnRemoveClicked() {
//  UserMetrics::RecordAction(UserMetricsAction("BookmarkBubble_Unstar"),
//                            profile_);

  apply_edits_ = false;
  remove_bookmark_ = true;
  Apply();
}

void BookmarkBubbleQt::ApplyEdits() {
  // Set this to make sure we don't attempt to apply edits again.
  apply_edits_ = false;

  BookmarkModel* model = profile_->GetBookmarkModel();
  const BookmarkNode* node = model->GetMostRecentlyAddedNodeForURL(url_);
  if (node) {
    const string16 new_title = UTF8ToUTF16(name_.toUtf8().data());
    if (new_title != node->GetTitle()) {
      model->SetTitle(node, new_title);
      UserMetrics::RecordAction(
          UserMetricsAction("BookmarkBubble_ChangeTitleInBubble"),
          profile_);
    }

    // Last index means 'Create Application Bookmark...'
    if (folder_index_ < folder_combo_model_->GetItemCount() - 1) {
      const BookmarkNode* new_parent = 
               folder_combo_model_->GetNodeAt(folder_index_);
      if (new_parent != node->parent()) {
        UserMetrics::RecordAction(
            UserMetricsAction("BookmarkBubble_ChangeParent"), profile_);
       if (newly_bookmarked_) {
          model->Move(node, new_parent, new_parent->child_count());
        } else {
          model->Copy(node, new_parent, new_parent->child_count());
        }
      }
    }
   
    if (folder_index_ == folder_combo_model_->GetItemCount() - 1) {
       //TODO ./brower/browser.cc RegisterAppPrefs, ConvertContentsToApplication
      if(newly_bookmarked_) {
        model->Remove(node->parent(), node->parent()->GetIndexOf(node));
      }
      TabContents* current_tab_contents =
                         browser_->tabstrip_model()->GetSelectedTabContents()->tab_contents();

       // Prepare data
      web_app::GetShortcutInfoForTab(current_tab_contents, &shortcut_info_);
      shortcut_info_.create_on_desktop = true;
      shortcut_info_.create_in_quick_launch_bar = false;
      shortcut_info_.create_in_applications_menu = false;

      web_app::CreateShortcut(current_tab_contents->profile()->GetPath(),
                          shortcut_info_,
                          NULL);

    //  current_tab_contents->SetAppIcon(shortcut_info_.favicon);
   //   if (current_tab_contents->delegate())
   //      current_tab_contents->delegate()->ConvertContentsToApplication(current_tab_contents);

    }
  }
}

std::string BookmarkBubbleQt::GetTitle() {
  BookmarkModel* bookmark_model= profile_->GetBookmarkModel();
  const BookmarkNode* node =
      bookmark_model->GetMostRecentlyAddedNodeForURL(url_);
  if (!node) {
    NOTREACHED();
    return std::string();
  }

  return UTF16ToUTF8(node->GetTitle());
}

void BookmarkBubbleQt::OnDoneClicked() {
  apply_edits_ = true;
  remove_bookmark_ = false;
  Apply();
}


void BookmarkBubbleQt::InitFolderComboModel() {
  folder_combo_model_.reset(new RecentlyUsedFoldersComboModel(
      profile_->GetBookmarkModel(),
      profile_->GetBookmarkModel()->GetMostRecentlyAddedNodeForURL(url_)));

  for (int index = 0; index < folder_combo_model_->GetItemCount(); index ++) {
     QString folderName = 
           QString::fromStdString(UTF16ToUTF8(folder_combo_model_->GetItemAt(index)));
     folderList_.append(folderName);
  }
}

// Displays the bubble at the given coords. |point| is intentionally not const.
void BookmarkBubbleQt::PopupAt(gfx::Point point)
{
  impl_->PopupAt(point.x(), point.y());
}

#include "moc_bookmark_bubble_qt.cc"
