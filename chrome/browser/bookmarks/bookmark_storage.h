// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_BOOKMARKS_BOOKMARK_STORAGE_H_
#define CHROME_BROWSER_BOOKMARKS_BOOKMARK_STORAGE_H_

#include "base/file_path.h"
#include "base/ref_counted.h"
#include "chrome/common/important_file_writer.h"
#include "chrome/common/notification_observer.h"
#include "chrome/common/notification_registrar.h"

class BookmarkModel;
class BookmarkNode;
class Profile;
class Task;
class Value;

namespace base {
class Thread;
}

// BookmarkStorage handles reading/write the bookmark bar model. The
// BookmarkModel uses the BookmarkStorage to load bookmarks from disk, as well
// as notifying the BookmarkStorage every time the model changes.
//
// Internally BookmarkStorage uses BookmarkCodec to do the actual read/write.
class BookmarkStorage : public NotificationObserver,
                        public ImportantFileWriter::DataSerializer,
                        public base::RefCountedThreadSafe<BookmarkStorage> {
 public:
  // Creates a BookmarkStorage for the specified model
  BookmarkStorage(Profile* profile, BookmarkModel* model);
  ~BookmarkStorage();

  // Loads the bookmarks into the model, notifying the model when done.
  void LoadBookmarks();

  // Schedules saving the bookmark bar model to disk.
  void ScheduleSave();

  // Notification the bookmark bar model is going to be deleted. If there is
  // a pending save, it is saved immediately.
  void BookmarkModelDeleted();

  // ImportantFileWriter::DataSerializer
  virtual bool SerializeData(std::string* output);

 private:
  class LoadTask;

  // Callback from backend with the results of the bookmark file.
  // This may be called multiple times, with different paths. This happens when
  // we migrate bookmark data from database.
  void OnLoadFinished(bool file_exists, const FilePath& path,
                      Value* root_value);

  // Loads bookmark data from |file| and notifies the model when finished.
  void DoLoadBookmarks(const FilePath& file);

  // Load bookmarks data from the file written by history (StarredURLDatabase).
  void MigrateFromHistory();

  // Called when history has written the file with bookmarks data. Loads data
  // from that file.
  void OnHistoryFinishedWriting();

  // Called after we loaded file generated by history. Saves the data, deletes
  // the temporary file, and notifies the model.
  void FinishHistoryMigration();

  // NotificationObserver
  void Observe(NotificationType type, const NotificationSource& source,
               const NotificationDetails& details);

  // Serializes the data and schedules save using ImportantFileWriter.
  // Returns true on successful serialization.
  bool SaveNow();

  // Runs task on backend thread (or on current thread if backend thread
  // is NULL). Takes ownership of |task|.
  void RunTaskOnBackendThread(Task* task) const;

  // Returns the thread the backend is run on.
  const base::Thread* backend_thread() const { return backend_thread_; }

  // Adds node to the model's index, recursing through all children as well.
  void AddBookmarksToIndex(BookmarkNode* node);

  // Keep the pointer to profile, we may need it for migration from history.
  Profile* profile_;

  // The model. The model is NULL once BookmarkModelDeleted has been invoked.
  BookmarkModel* model_;

  // Thread read/writing is run on. This comes from the profile, and is null
  // during testing.
  const base::Thread* backend_thread_;

  // Helper to write bookmark data safely.
  ImportantFileWriter writer_;

  // Helper to ensure that we unregister from notifications on destruction.
  NotificationRegistrar notification_registrar_;

  // Path to temporary file created during migrating bookmarks from history.
  const FilePath tmp_history_path_;

  DISALLOW_COPY_AND_ASSIGN(BookmarkStorage);
};

#endif  // CHROME_BROWSER_BOOKMARKS_BOOKMARK_STORAGE_H_
