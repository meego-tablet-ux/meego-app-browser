// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/password_manager/password_store.h"

#include "base/scoped_ptr.h"
#include "base/task.h"

using std::vector;
using webkit_glue::PasswordForm;

PasswordStore::PasswordStore() : handle_(0) {
}

bool PasswordStore::Init() {
  thread_.reset(new base::Thread("Chrome_PasswordStore_Thread"));

  if (!thread_->Start()) {
    thread_.reset(NULL);
    return false;
  }

  return true;
}

void PasswordStore::ScheduleTask(Task* task) {
  if (thread_.get()) {
    thread_->message_loop()->PostTask(FROM_HERE, task);
  }
}

void PasswordStore::AddLogin(const PasswordForm& form) {
  ScheduleTask(NewRunnableMethod(this, &PasswordStore::AddLoginImpl, form));
}

void PasswordStore::UpdateLogin(const PasswordForm& form) {
  ScheduleTask(NewRunnableMethod(this, &PasswordStore::UpdateLoginImpl, form));
}

void PasswordStore::RemoveLogin(const PasswordForm& form) {
  ScheduleTask(NewRunnableMethod(this, &PasswordStore::RemoveLoginImpl, form));
}

void PasswordStore::RemoveLoginsCreatedBetween(const base::Time& delete_begin,
                                               const base::Time& delete_end) {
  ScheduleTask(NewRunnableMethod(this,
                                 &PasswordStore::RemoveLoginsCreatedBetweenImpl,
                                 delete_begin, delete_end));
}

int PasswordStore::GetLogins(const PasswordForm& form,
                             PasswordStoreConsumer* consumer) {
  int handle = GetNewRequestHandle();
  GetLoginsRequest* request = new GetLoginsRequest(consumer, handle);
  ScheduleTask(NewRunnableMethod(this, &PasswordStore::GetLoginsImpl, request,
                                 form));
  return handle;
}

int PasswordStore::GetAutofillableLogins(PasswordStoreConsumer* consumer) {
  int handle = GetNewRequestHandle();
  GetLoginsRequest* request = new GetLoginsRequest(consumer, handle);
  ScheduleTask(NewRunnableMethod(this,
                                 &PasswordStore::GetAutofillableLoginsImpl,
                                 request));
  return handle;
}

int PasswordStore::GetBlacklistLogins(PasswordStoreConsumer* consumer) {
  int handle = GetNewRequestHandle();
  GetLoginsRequest* request = new GetLoginsRequest(consumer, handle);
  ScheduleTask(NewRunnableMethod(this,
                                 &PasswordStore::GetBlacklistLoginsImpl,
                                 request));
  return handle;
}

void PasswordStore::NotifyConsumer(GetLoginsRequest* request,
                                   const vector<PasswordForm*> forms) {
  scoped_ptr<GetLoginsRequest> request_ptr(request);

  DCHECK(MessageLoop::current() == thread_->message_loop());
  request->message_loop->PostTask(FROM_HERE,
      NewRunnableMethod(this,
                        &PasswordStore::NotifyConsumerImpl,
                        request->consumer, request->handle, forms));
}

void PasswordStore::NotifyConsumerImpl(PasswordStoreConsumer* consumer,
                                       int handle,
                                       const vector<PasswordForm*> forms) {
  // Don't notify the consumer if the request was canceled.
  if (pending_requests_.find(handle) == pending_requests_.end())
    return;
  pending_requests_.erase(handle);

  consumer->OnPasswordStoreRequestDone(handle, forms);
}

int PasswordStore::GetNewRequestHandle() {
  int handle = handle_++;
  pending_requests_.insert(handle);
  return handle;
}

void PasswordStore::CancelLoginsQuery(int handle) {
  pending_requests_.erase(handle);
}

PasswordStore::GetLoginsRequest::GetLoginsRequest(
    PasswordStoreConsumer* consumer, int handle)
    : consumer(consumer), handle(handle), message_loop(MessageLoop::current()) {
}
