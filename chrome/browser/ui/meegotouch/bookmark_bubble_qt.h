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

#ifndef CHROME_BROWSER_QT_BOOKMARK_BUBBLE_QT_H_
#define CHROME_BROWSER_QT_BOOKMARK_BUBBLE_QT_H_
#pragma once

#include <string>
#include <vector>
#include "ui/gfx/point.h"
//#include "ui/base/gtk/gtk_signal.h"
#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "base/task.h"
//#include "chrome/browser/ui/gtk/info_bubble_gtk.h"
#include "content/common/notification_observer.h"
#include "content/common/notification_registrar.h"
#include "googleurl/src/gurl.h"
#include "chrome/browser/ui/meegotouch/browser_window_qt.h"
#include "chrome/browser/web_applications/web_app.h"
class BookmarkNode;
class BrowserWindowQt;
class Browser;
class Profile;
class RecentlyUsedFoldersComboModel;
class BookmarkBubbleQtImpl;

class BookmarkBubbleQt : public NotificationObserver {
 public:
  void PopupAt(gfx::Point point);
 
  void OnRemoveClicked();
  void OnDoneClicked();
  // Overridden from NotificationObserver:
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);
  BookmarkBubbleQt( BrowserWindowQt* window,
                    Browser* browser,
                    Profile* profile);

  BookmarkBubbleQt( BrowserWindowQt* window,
                    Browser* browser,
                    Profile* profile,
                    const GURL& url,
                    bool newly_bookmarked);
  ~BookmarkBubbleQt();

  void SetFolderIndex(int index) {folder_index_ = index;}
  void SetTitle(QString title) {name_ = title;}
  void Cancel();


  private:
  // Update the bookmark with any edits that have been made.
  void ApplyEdits();

  // Decide whether to remove or add
  void Apply();

  // Return the UTF8 encoded title for the current |url_|.
  std::string GetTitle();

  void InitFolderComboModel();

  // The URL of the bookmark.
  GURL url_;
  // Our current profile (used to access the bookmark system).
  Profile* profile_;

  scoped_ptr<RecentlyUsedFoldersComboModel> folder_combo_model_;

  // Whether the bubble is creating or editing an existing bookmark.
  bool newly_bookmarked_;
  // When closing the window, whether we should update or remove the bookmark.
  bool apply_edits_;
  bool remove_bookmark_;

  QString name_;
  int folder_index_; 
  QStringList folderList_;

  NotificationRegistrar registrar_;
  BookmarkBubbleQtImpl* impl_;
  BrowserWindowQt* window_;
  Browser* browser_;
 
  // Target shortcut info.
  ShellIntegration::ShortcutInfo shortcut_info_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkBubbleQt);
};

#endif  // CHROME_BROWSER_QT_BOOKMARK_BUBBLE_QT_H_
