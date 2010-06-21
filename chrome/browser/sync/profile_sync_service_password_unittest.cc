// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

#include "base/task.h"
#include "base/time.h"
#include "base/utf_string_conversions.h"
#include "base/waitable_event.h"
#include "chrome/browser/password_manager/password_store.h"
#include "chrome/browser/sync/engine/syncapi.h"
#include "chrome/browser/sync/glue/password_change_processor.h"
#include "chrome/browser/sync/glue/password_data_type_controller.h"
#include "chrome/browser/sync/glue/password_model_associator.h"
#include "chrome/browser/sync/glue/sync_backend_host_mock.h"
#include "chrome/browser/sync/profile_sync_factory.h"
#include "chrome/browser/sync/profile_sync_factory_mock.h"
#include "chrome/browser/sync/profile_sync_service.h"
#include "chrome/browser/sync/profile_sync_test_util.h"
#include "chrome/browser/sync/protocol/password_specifics.pb.h"
#include "chrome/browser/sync/syncable/directory_manager.h"
#include "chrome/browser/sync/syncable/syncable.h"
#include "chrome/browser/sync/test_profile_sync_service.h"
#include "chrome/common/notification_source.h"
#include "chrome/common/notification_type.h"
#include "chrome/test/sync/engine/test_id_factory.h"
#include "chrome/test/profile_mock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "webkit/glue/password_form.h"

using base::Time;
using base::WaitableEvent;
using browser_sync::PasswordChangeProcessor;
using browser_sync::PasswordDataTypeController;
using browser_sync::PasswordModelAssociator;
using browser_sync::SyncBackendHostMock;
using browser_sync::TestIdFactory;
using browser_sync::UnrecoverableErrorHandler;
using sync_api::SyncManager;
using sync_api::UserShare;
using syncable::BASE_VERSION;
using syncable::CREATE;
using syncable::DirectoryManager;
using syncable::ID;
using syncable::IS_DEL;
using syncable::IS_DIR;
using syncable::IS_UNAPPLIED_UPDATE;
using syncable::IS_UNSYNCED;
using syncable::MutableEntry;
using syncable::SERVER_IS_DIR;
using syncable::SERVER_VERSION;
using syncable::SPECIFICS;
using syncable::ScopedDirLookup;
using syncable::UNIQUE_SERVER_TAG;
using syncable::UNITTEST;
using syncable::WriteTransaction;
using testing::_;
using testing::DoAll;
using testing::DoDefault;
using testing::ElementsAre;
using testing::Eq;
using testing::Invoke;
using testing::Return;
using testing::SaveArg;
using testing::SetArgumentPointee;
using webkit_glue::PasswordForm;

ACTION_P3(MakePasswordSyncComponents, service, ps, dtc) {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::DB));
  PasswordModelAssociator* model_associator =
      new PasswordModelAssociator(service, ps);
  PasswordChangeProcessor* change_processor =
      new PasswordChangeProcessor(model_associator, ps, dtc);
  return ProfileSyncFactory::SyncComponents(model_associator,
                                            change_processor);
}

class MockPasswordStore : public PasswordStore {
 public:
  MOCK_METHOD1(RemoveLogin, void(const PasswordForm&));
  MOCK_METHOD2(GetLogins, int(const PasswordForm&, PasswordStoreConsumer*));
  MOCK_METHOD1(AddLogin, void(const PasswordForm&));
  MOCK_METHOD1(UpdateLogin, void(const PasswordForm&));
  MOCK_METHOD1(AddLoginImpl, void(const PasswordForm&));
  MOCK_METHOD1(UpdateLoginImpl, void(const PasswordForm&));
  MOCK_METHOD1(RemoveLoginImpl, void(const PasswordForm&));
  MOCK_METHOD2(RemoveLoginsCreatedBetweenImpl, void(const base::Time&,
               const base::Time&));
  MOCK_METHOD2(GetLoginsImpl, void(GetLoginsRequest*, const PasswordForm&));
  MOCK_METHOD1(GetAutofillableLoginsImpl, void(GetLoginsRequest*));
  MOCK_METHOD1(GetBlacklistLoginsImpl, void(GetLoginsRequest*));
  MOCK_METHOD1(FillAutofillableLogins,
      bool(std::vector<PasswordForm*>*));
  MOCK_METHOD1(FillBlacklistLogins,
      bool(std::vector<PasswordForm*>*));
};

class ProfileSyncServicePasswordTest : public testing::Test {
 protected:
  ProfileSyncServicePasswordTest()
      : ui_thread_(ChromeThread::UI, &message_loop_),
        db_thread_(ChromeThread::DB) {
  }

  virtual void SetUp() {
    password_store_ = new MockPasswordStore();
    db_thread_.Start();

    notification_service_ = new ThreadNotificationService(&db_thread_);
    notification_service_->Init();
  }

  virtual void TearDown() {
    service_.reset();
    notification_service_->TearDown();
    db_thread_.Stop();
    MessageLoop::current()->RunAllPending();
  }

  void StartSyncService(Task* task) {
    if (!service_.get()) {
      service_.reset(new TestProfileSyncService(&factory_, &profile_,
                                                false, false));
      service_->AddObserver(&observer_);
      PasswordDataTypeController* data_type_controller =
          new PasswordDataTypeController(&factory_,
                                         &profile_,
                                         service_.get());

      EXPECT_CALL(factory_, CreatePasswordSyncComponents(_, _, _)).
          WillOnce(MakePasswordSyncComponents(service_.get(),
                                              password_store_.get(),
                                              data_type_controller));
      EXPECT_CALL(factory_, CreateDataTypeManager(_, _)).
          WillOnce(MakeDataTypeManager(&backend_));

      EXPECT_CALL(profile_, GetPasswordStore(_)).
          WillOnce(Return(password_store_.get()));

      // State changes once for the backend init and once for startup done.
      EXPECT_CALL(observer_, OnStateChanged()).
          WillOnce(InvokeTask(task)).
          WillOnce(Return()).
          WillOnce(QuitUIMessageLoop());
      service_->RegisterDataTypeController(data_type_controller);
      service_->Initialize();
      MessageLoop::current()->Run();
    }
  }

  void CreatePasswordRoot() {
    UserShare* user_share = service_->backend()->GetUserShareHandle();
    DirectoryManager* dir_manager = user_share->dir_manager.get();

    ScopedDirLookup dir(dir_manager, user_share->authenticated_name);
    ASSERT_TRUE(dir.good());

    WriteTransaction wtrans(dir, UNITTEST, __FILE__, __LINE__);
    MutableEntry node(&wtrans,
                      CREATE,
                      wtrans.root_id(),
                      browser_sync::kPasswordTag);
    node.Put(UNIQUE_SERVER_TAG, browser_sync::kPasswordTag);
    node.Put(IS_DIR, true);
    node.Put(SERVER_IS_DIR, false);
    node.Put(IS_UNSYNCED, false);
    node.Put(IS_UNAPPLIED_UPDATE, false);
    node.Put(SERVER_VERSION, 20);
    node.Put(BASE_VERSION, 20);
    node.Put(IS_DEL, false);
    node.Put(ID, ids_.MakeServer(browser_sync::kPasswordTag));
    sync_pb::EntitySpecifics specifics;
    specifics.MutableExtension(sync_pb::password);
    node.Put(SPECIFICS, specifics);
  }

  void AddPasswordSyncNode(const PasswordForm& entry) {
    sync_api::WriteTransaction trans(
        service_->backend()->GetUserShareHandle());
    sync_api::ReadNode password_root(&trans);
    ASSERT_TRUE(password_root.InitByTagLookup(browser_sync::kPasswordTag));

    sync_api::WriteNode node(&trans);
    std::string tag = PasswordModelAssociator::MakeTag(entry);
    ASSERT_TRUE(node.InitUniqueByCreation(syncable::PASSWORDS,
                                          password_root,
                                          tag));
    PasswordModelAssociator::WriteToSyncNode(entry, &node);
  }

  void GetPasswordEntriesFromSyncDB(std::vector<PasswordForm>* entries) {
    sync_api::ReadTransaction trans(service_->backend()->GetUserShareHandle());
    sync_api::ReadNode password_root(&trans);
    ASSERT_TRUE(password_root.InitByTagLookup(browser_sync::kPasswordTag));

    int64 child_id = password_root.GetFirstChildId();
    while (child_id != sync_api::kInvalidId) {
      sync_api::ReadNode child_node(&trans);
      ASSERT_TRUE(child_node.InitByIdLookup(child_id));

      sync_pb::PasswordSpecificsData password;
      ASSERT_TRUE(child_node.GetPasswordSpecifics(&password));

      PasswordForm form;
      PasswordModelAssociator::CopyPassword(password, &form);

      entries->push_back(form);

      child_id = child_node.GetSuccessorId();
    }
  }

  bool ComparePasswords(const PasswordForm& lhs, const PasswordForm& rhs) {
    return lhs.scheme == rhs.scheme &&
           lhs.signon_realm == rhs.signon_realm &&
           lhs.origin == rhs.origin &&
           lhs.action == rhs.action &&
           lhs.username_element == rhs.username_element &&
           lhs.username_value == rhs.username_value &&
           lhs.password_element == rhs.password_element &&
           lhs.password_value == rhs.password_value &&
           lhs.ssl_valid == rhs.ssl_valid &&
           lhs.preferred == rhs.preferred &&
           lhs.date_created == rhs.date_created &&
           lhs.blacklisted_by_user == rhs.blacklisted_by_user;
  }

  void SetIdleChangeProcessorExpectations() {
    EXPECT_CALL(*(password_store_.get()), AddLoginImpl(_)).Times(0);
    EXPECT_CALL(*(password_store_.get()), UpdateLoginImpl(_)).Times(0);
    EXPECT_CALL(*(password_store_.get()), RemoveLoginImpl(_)).Times(0);
  }

  friend class CreatePasswordRootTask;
  friend class AddPasswordEntriesTask;

  MessageLoopForUI message_loop_;
  ChromeThread ui_thread_;
  ChromeThread db_thread_;
  scoped_refptr<ThreadNotificationService> notification_service_;

  scoped_ptr<TestProfileSyncService> service_;
  ProfileMock profile_;
  ProfileSyncFactoryMock factory_;
  ProfileSyncServiceObserverMock observer_;
  SyncBackendHostMock backend_;
  scoped_refptr<MockPasswordStore> password_store_;

  TestIdFactory ids_;
};

class CreatePasswordRootTask : public Task {
 public:
  explicit CreatePasswordRootTask(ProfileSyncServicePasswordTest* test)
      : test_(test) {
  }

  virtual void Run() {
    test_->CreatePasswordRoot();
  }

 private:
  ProfileSyncServicePasswordTest* test_;
};

class AddPasswordEntriesTask : public Task {
 public:
  AddPasswordEntriesTask(ProfileSyncServicePasswordTest* test,
                         const std::vector<PasswordForm>& entries)
      : test_(test), entries_(entries) {
  }

  virtual void Run() {
    test_->CreatePasswordRoot();
    for (size_t i = 0; i < entries_.size(); ++i) {
      test_->AddPasswordSyncNode(entries_[i]);
    }
  }

 private:
  ProfileSyncServicePasswordTest* test_;
  const std::vector<PasswordForm>& entries_;
};

TEST_F(ProfileSyncServicePasswordTest, FailModelAssociation) {
  // Backend will be paused but not resumed.
  EXPECT_CALL(backend_, RequestPause()).
      WillOnce(testing::DoAll(Notify(NotificationType::SYNC_PAUSED),
                              testing::Return(true)));
  // Don't create the root password node so startup fails.
  StartSyncService(NULL);
  EXPECT_TRUE(service_->unrecoverable_error_detected());
}

TEST_F(ProfileSyncServicePasswordTest, EmptyNativeEmptySync) {
  EXPECT_CALL(*(password_store_.get()), FillAutofillableLogins(_))
      .WillOnce(Return(true));
  EXPECT_CALL(*(password_store_.get()), FillBlacklistLogins(_))
      .WillOnce(Return(true));
  SetIdleChangeProcessorExpectations();
  CreatePasswordRootTask task(this);
  StartSyncService(&task);
  std::vector<PasswordForm> sync_entries;
  GetPasswordEntriesFromSyncDB(&sync_entries);
  EXPECT_EQ(0U, sync_entries.size());
}

TEST_F(ProfileSyncServicePasswordTest, HasNativeEntriesEmptySync) {
  std::vector<PasswordForm*> forms;
  std::vector<PasswordForm> expected_forms;
  PasswordForm* new_form = new PasswordForm;
  new_form->scheme = PasswordForm::SCHEME_HTML;
  new_form->signon_realm = "pie";
  new_form->origin = GURL("http://pie.com");
  new_form->action = GURL("http://pie.com/submit");
  new_form->username_element = UTF8ToUTF16("name");
  new_form->username_value = UTF8ToUTF16("tom");
  new_form->password_element = UTF8ToUTF16("cork");
  new_form->password_value = UTF8ToUTF16("password1");
  new_form->ssl_valid = true;
  new_form->preferred = false;
  new_form->date_created = base::Time::FromInternalValue(1234);
  new_form->blacklisted_by_user = false;
  forms.push_back(new_form);
  expected_forms.push_back(*new_form);
  EXPECT_CALL(*(password_store_.get()), FillAutofillableLogins(_))
      .WillOnce(DoAll(SetArgumentPointee<0>(forms), Return(true)));
  EXPECT_CALL(*(password_store_.get()), FillBlacklistLogins(_))
      .WillOnce(Return(true));
  SetIdleChangeProcessorExpectations();
  CreatePasswordRootTask task(this);
  StartSyncService(&task);
  std::vector<PasswordForm> sync_forms;
  GetPasswordEntriesFromSyncDB(&sync_forms);
  ASSERT_EQ(1U, sync_forms.size());
  EXPECT_TRUE(ComparePasswords(expected_forms[0], sync_forms[0]));
}

TEST_F(ProfileSyncServicePasswordTest, HasNativeEntriesEmptySyncSameUsername) {
  std::vector<PasswordForm*> forms;
  std::vector<PasswordForm> expected_forms;

  {
    PasswordForm* new_form = new PasswordForm;
    new_form->scheme = PasswordForm::SCHEME_HTML;
    new_form->signon_realm = "pie";
    new_form->origin = GURL("http://pie.com");
    new_form->action = GURL("http://pie.com/submit");
    new_form->username_element = UTF8ToUTF16("name");
    new_form->username_value = UTF8ToUTF16("tom");
    new_form->password_element = UTF8ToUTF16("cork");
    new_form->password_value = UTF8ToUTF16("password1");
    new_form->ssl_valid = true;
    new_form->preferred = false;
    new_form->date_created = base::Time::FromInternalValue(1234);
    new_form->blacklisted_by_user = false;
    forms.push_back(new_form);
    expected_forms.push_back(*new_form);
  }
  {
    PasswordForm* new_form = new PasswordForm;
    new_form->scheme = PasswordForm::SCHEME_HTML;
    new_form->signon_realm = "pie";
    new_form->origin = GURL("http://pie.com");
    new_form->action = GURL("http://pie.com/submit");
    new_form->username_element = UTF8ToUTF16("name");
    new_form->username_value = UTF8ToUTF16("pete");
    new_form->password_element = UTF8ToUTF16("cork");
    new_form->password_value = UTF8ToUTF16("password2");
    new_form->ssl_valid = true;
    new_form->preferred = false;
    new_form->date_created = base::Time::FromInternalValue(1234);
    new_form->blacklisted_by_user = false;
    forms.push_back(new_form);
    expected_forms.push_back(*new_form);
  }

  EXPECT_CALL(*(password_store_.get()), FillAutofillableLogins(_))
      .WillOnce(DoAll(SetArgumentPointee<0>(forms), Return(true)));
  EXPECT_CALL(*(password_store_.get()), FillBlacklistLogins(_))
      .WillOnce(Return(true));
  SetIdleChangeProcessorExpectations();
  CreatePasswordRootTask task(this);
  StartSyncService(&task);
  std::vector<PasswordForm> sync_forms;
  GetPasswordEntriesFromSyncDB(&sync_forms);
  ASSERT_EQ(2U, sync_forms.size());
  EXPECT_TRUE(ComparePasswords(expected_forms[0], sync_forms[1]));
  EXPECT_TRUE(ComparePasswords(expected_forms[1], sync_forms[0]));
}

TEST_F(ProfileSyncServicePasswordTest, HasNativeHasSyncNoMerge) {
  std::vector<PasswordForm*> native_forms;
  std::vector<PasswordForm> sync_forms;
  std::vector<PasswordForm> expected_forms;
  {
    PasswordForm* new_form = new PasswordForm;
    new_form->scheme = PasswordForm::SCHEME_HTML;
    new_form->signon_realm = "pie";
    new_form->origin = GURL("http://pie.com");
    new_form->action = GURL("http://pie.com/submit");
    new_form->username_element = UTF8ToUTF16("name");
    new_form->username_value = UTF8ToUTF16("tom");
    new_form->password_element = UTF8ToUTF16("cork");
    new_form->password_value = UTF8ToUTF16("password1");
    new_form->ssl_valid = true;
    new_form->preferred = false;
    new_form->date_created = base::Time::FromInternalValue(1234);
    new_form->blacklisted_by_user = false;

    native_forms.push_back(new_form);
    expected_forms.push_back(*new_form);
  }

  {
    PasswordForm new_form;
    new_form.scheme = PasswordForm::SCHEME_HTML;
    new_form.signon_realm = "pie2";
    new_form.origin = GURL("http://pie2.com");
    new_form.action = GURL("http://pie2.com/submit");
    new_form.username_element = UTF8ToUTF16("name2");
    new_form.username_value = UTF8ToUTF16("tom2");
    new_form.password_element = UTF8ToUTF16("cork2");
    new_form.password_value = UTF8ToUTF16("password12");
    new_form.ssl_valid = false;
    new_form.preferred = true;
    new_form.date_created = base::Time::FromInternalValue(12345);
    new_form.blacklisted_by_user = false;
    sync_forms.push_back(new_form);
    expected_forms.push_back(new_form);
  }

  EXPECT_CALL(*(password_store_.get()), FillAutofillableLogins(_))
      .WillOnce(DoAll(SetArgumentPointee<0>(native_forms), Return(true)));
  EXPECT_CALL(*(password_store_.get()), FillBlacklistLogins(_))
      .WillOnce(Return(true));

  AddPasswordEntriesTask task(this, sync_forms);

  EXPECT_CALL(*(password_store_.get()), AddLoginImpl(_)).Times(1);
  StartSyncService(&task);

  std::vector<PasswordForm> new_sync_forms;
  GetPasswordEntriesFromSyncDB(&new_sync_forms);

  EXPECT_EQ(2U, new_sync_forms.size());
  EXPECT_TRUE(ComparePasswords(expected_forms[0], new_sync_forms[0]));
  EXPECT_TRUE(ComparePasswords(expected_forms[1], new_sync_forms[1]));
}

TEST_F(ProfileSyncServicePasswordTest, HasNativeHasSyncMergeEntry) {
  std::vector<PasswordForm*> native_forms;
  std::vector<PasswordForm> sync_forms;
  std::vector<PasswordForm> expected_forms;
  {
    PasswordForm* new_form = new PasswordForm;
    new_form->scheme = PasswordForm::SCHEME_HTML;
    new_form->signon_realm = "pie";
    new_form->origin = GURL("http://pie.com");
    new_form->action = GURL("http://pie.com/submit");
    new_form->username_element = UTF8ToUTF16("name");
    new_form->username_value = UTF8ToUTF16("tom");
    new_form->password_element = UTF8ToUTF16("cork");
    new_form->password_value = UTF8ToUTF16("password1");
    new_form->ssl_valid = true;
    new_form->preferred = false;
    new_form->date_created = base::Time::FromInternalValue(1234);
    new_form->blacklisted_by_user = false;

    native_forms.push_back(new_form);
  }

  {
    PasswordForm new_form;
    new_form.scheme = PasswordForm::SCHEME_HTML;
    new_form.signon_realm = "pie";
    new_form.origin = GURL("http://pie.com");
    new_form.action = GURL("http://pie.com/submit");
    new_form.username_element = UTF8ToUTF16("name");
    new_form.username_value = UTF8ToUTF16("tom");
    new_form.password_element = UTF8ToUTF16("cork");
    new_form.password_value = UTF8ToUTF16("password12");
    new_form.ssl_valid = false;
    new_form.preferred = true;
    new_form.date_created = base::Time::FromInternalValue(12345);
    new_form.blacklisted_by_user = false;
    sync_forms.push_back(new_form);
  }

  {
    PasswordForm new_form;
    new_form.scheme = PasswordForm::SCHEME_HTML;
    new_form.signon_realm = "pie";
    new_form.origin = GURL("http://pie.com");
    new_form.action = GURL("http://pie.com/submit");
    new_form.username_element = UTF8ToUTF16("name");
    new_form.username_value = UTF8ToUTF16("tom");
    new_form.password_element = UTF8ToUTF16("cork");
    new_form.password_value = UTF8ToUTF16("password12");
    new_form.ssl_valid = false;
    new_form.preferred = true;
    new_form.date_created = base::Time::FromInternalValue(12345);
    new_form.blacklisted_by_user = false;
    expected_forms.push_back(new_form);
  }

  EXPECT_CALL(*(password_store_.get()), FillAutofillableLogins(_))
      .WillOnce(DoAll(SetArgumentPointee<0>(native_forms), Return(true)));
  EXPECT_CALL(*(password_store_.get()), FillBlacklistLogins(_))
      .WillOnce(Return(true));

  AddPasswordEntriesTask task(this, sync_forms);

  EXPECT_CALL(*(password_store_.get()), UpdateLoginImpl(_)).Times(1);
  StartSyncService(&task);

  std::vector<PasswordForm> new_sync_forms;
  GetPasswordEntriesFromSyncDB(&new_sync_forms);

  EXPECT_EQ(1U, new_sync_forms.size());
  EXPECT_TRUE(ComparePasswords(expected_forms[0], new_sync_forms[0]));
}
