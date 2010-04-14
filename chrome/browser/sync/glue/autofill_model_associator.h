// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SYNC_GLUE_AUTOFILL_MODEL_ASSOCIATOR_H_
#define CHROME_BROWSER_SYNC_GLUE_AUTOFILL_MODEL_ASSOCIATOR_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/scoped_ptr.h"
#include "chrome/browser/autofill/personal_data_manager.h"
#include "chrome/browser/chrome_thread.h"
#include "chrome/browser/sync/engine/syncapi.h"
#include "chrome/browser/sync/glue/model_associator.h"
#include "chrome/browser/sync/protocol/autofill_specifics.pb.h"
#include "chrome/browser/webdata/autofill_entry.h"

class AutoFillProfile;

class ProfileSyncService;
class WebDatabase;

namespace sync_api {
class WriteTransaction;
}

namespace browser_sync {

class AutofillChangeProcessor;
class UnrecoverableErrorHandler;

extern const char kAutofillTag[];
extern const char kAutofillProfileNamespaceTag[];
extern const char kAutofillEntryNamespaceTag[];

// Contains all model association related logic:
// * Algorithm to associate autofill model and sync model.
// We do not check if we have local data before this run; we always
// merge and sync.
class AutofillModelAssociator
    : public PerDataTypeAssociatorInterface<std::string, std::string> {
 public:
  static syncable::ModelType model_type() { return syncable::AUTOFILL; }
  AutofillModelAssociator(ProfileSyncService* sync_service,
                          WebDatabase* web_database,
                          PersonalDataManager* data_manager,
                          UnrecoverableErrorHandler* error_handler);
  virtual ~AutofillModelAssociator();

  // A task used by this class and the change processor to inform the
  // PersonalDataManager living on the UI thread that it needs to refresh.
  class DoOptimisticRefreshTask : public Task {
   public:
    explicit DoOptimisticRefreshTask(PersonalDataManager* pdm) : pdm_(pdm) {}
    virtual void Run() {
      DCHECK(ChromeThread::CurrentlyOn(ChromeThread::UI));
      pdm_->Refresh();
    }
   private:
    PersonalDataManager* pdm_;
  };

  // PerDataTypeAssociatorInterface implementation.
  //
  // Iterates through the sync model looking for matched pairs of items.
  virtual bool AssociateModels();

  // Clears all associations.
  virtual bool DisassociateModels();

  // The has_nodes out param is true if the sync model has nodes other
  // than the permanent tagged nodes.
  virtual bool SyncModelHasUserCreatedNodes(bool* has_nodes);

  // The has_nodes out param is true if the autofill model has any
  // user-defined autofill entries.
  virtual bool ChromeModelHasUserCreatedNodes(bool* has_nodes);

  // Not implemented.
  virtual const std::string* GetChromeNodeFromSyncId(int64 sync_id) {
    return NULL;
  }

  // Not implemented.
  virtual bool InitSyncNodeFromChromeId(std::string node_id,
                                        sync_api::BaseNode* sync_node) {
    return false;
  }

  // Returns the sync id for the given autofill name, or sync_api::kInvalidId
  // if the autofill name is not associated to any sync id.
  virtual int64 GetSyncIdFromChromeId(std::string node_id);

  // Associates the given autofill name with the given sync id.
  virtual void Associate(const std::string* node, int64 sync_id);

  // Remove the association that corresponds to the given sync id.
  virtual void Disassociate(int64 sync_id);

  // Returns whether a node with the given permanent tag was found and update
  // |sync_id| with that node's id.
  virtual bool GetSyncIdForTaggedNode(const std::string& tag, int64* sync_id);

  static std::string KeyToTag(const string16& name, const string16& value);
  static std::string ProfileLabelToTag(const string16& label);

  static bool MergeTimestamps(const sync_pb::AutofillSpecifics& autofill,
                              const std::vector<base::Time>& timestamps,
                              std::vector<base::Time>* new_timestamps);
  static bool OverwriteProfileWithServerData(
      AutoFillProfile* merge_into,
      const sync_pb::AutofillProfileSpecifics& specifics);

  // Returns sync service instance.
  ProfileSyncService* sync_service() { return sync_service_; }

  static string16 MakeUniqueLabel(const string16& non_unique_label,
                                  sync_api::BaseTransaction* trans);

 private:
  typedef std::map<std::string, int64> AutofillToSyncIdMap;
  typedef std::map<int64, std::string> SyncIdToAutofillMap;

  // A convenience wrapper of a bunch of state we pass around while associating
  // models, and send to the WebDatabase for persistence.
  struct DataBundle {
    std::set<AutofillKey> current_entries;
    std::vector<AutofillEntry> new_entries;
    std::set<string16> current_profiles;
    std::vector<AutoFillProfile*> updated_profiles;
    std::vector<AutoFillProfile*> new_profiles;  // We own these pointers.
    ~DataBundle() { STLDeleteElements(&new_profiles); }
  };

  // Helper to query WebDatabase for the current autofill state.
  bool LoadAutofillData(std::vector<AutofillEntry>* entries,
                        std::vector<AutoFillProfile*>* profiles);

  // We split up model association first by autofill sub-type (entries, and
  // profiles.  There is a Traverse* method for each of these.
  bool TraverseAndAssociateChromeAutofillEntries(
      sync_api::WriteTransaction* write_trans,
      const sync_api::ReadNode& autofill_root,
      const std::vector<AutofillEntry>& all_entries_from_db,
      std::set<AutofillKey>* current_entries,
      std::vector<AutofillEntry>* new_entries);
  bool TraverseAndAssociateChromeAutoFillProfiles(
      sync_api::WriteTransaction* write_trans,
      const sync_api::ReadNode& autofill_root,
      const std::vector<AutoFillProfile*>& all_profiles_from_db,
      std::set<string16>* current_profiles,
      std::vector<AutoFillProfile*>* updated_profiles);

  // Once the above traversals are complete, we traverse the sync model to
  // associate all remaining nodes.
  bool TraverseAndAssociateAllSyncNodes(
      sync_api::WriteTransaction* write_trans,
      const sync_api::ReadNode& autofill_root,
      DataBundle* bundle);

  // Helper to persist any changes that occured during model association to
  // the WebDatabase.
  bool SaveChangesToWebData(const DataBundle& bundle);

  // Helper to insert an AutofillEntry into the WebDatabase (e.g. in response
  // to encountering a sync node that doesn't exist yet locally).
  void AddNativeEntryIfNeeded(const sync_pb::AutofillSpecifics& autofill,
                              DataBundle* bundle,
                              const sync_api::ReadNode& node);

  // Helper to insert an AutoFillProfile into the WebDatabase (e.g. in response
  // to encountering a sync node that doesn't exist yet locally).
  void AddNativeProfileIfNeeded(
      const sync_pb::AutofillProfileSpecifics& profile,
      DataBundle* bundle,
      const sync_api::ReadNode& node);

  // Helper to insert a sync node for the given AutoFillProfile (e.g. in
  // response to encountering a native profile that doesn't exist yet in the
  // cloud).
  bool MakeNewAutofillProfileSyncNode(
      sync_api::WriteTransaction* trans,
      const sync_api::BaseNode& autofill_root,
      const std::string& tag,
      const AutoFillProfile& profile,
      int64* sync_id);

  ProfileSyncService* sync_service_;
  WebDatabase* web_database_;
  PersonalDataManager* personal_data_;
  UnrecoverableErrorHandler* error_handler_;
  int64 autofill_node_id_;

  AutofillToSyncIdMap id_map_;
  SyncIdToAutofillMap id_map_inverse_;

  DISALLOW_COPY_AND_ASSIGN(AutofillModelAssociator);
};

}  // namespace browser_sync

#endif  // CHROME_BROWSER_SYNC_GLUE_AUTOFILL_MODEL_ASSOCIATOR_H_
