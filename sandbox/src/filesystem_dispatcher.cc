// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/src/filesystem_dispatcher.h"

#include "base/logging.h"
#include "sandbox/src/crosscall_client.h"
#include "sandbox/src/filesystem_interception.h"
#include "sandbox/src/filesystem_policy.h"
#include "sandbox/src/interception.h"
#include "sandbox/src/ipc_tags.h"
#include "sandbox/src/policy_broker.h"
#include "sandbox/src/policy_params.h"
#include "sandbox/src/sandbox.h"
#include "sandbox/src/sandbox_nt_util.h"

namespace sandbox {

FilesystemDispatcher::FilesystemDispatcher(PolicyBase* policy_base)
    : policy_base_(policy_base) {
  static const IPCCall create_params = {
    {IPC_NTCREATEFILE_TAG, WCHAR_TYPE, ULONG_TYPE, ULONG_TYPE, ULONG_TYPE,
     ULONG_TYPE, ULONG_TYPE, ULONG_TYPE},
    reinterpret_cast<CallbackGeneric>(&FilesystemDispatcher::NtCreateFile)
  };

  static const IPCCall open_file = {
    {IPC_NTOPENFILE_TAG, WCHAR_TYPE, ULONG_TYPE, ULONG_TYPE, ULONG_TYPE,
     ULONG_TYPE},
    reinterpret_cast<CallbackGeneric>(&FilesystemDispatcher::NtOpenFile)
  };

  static const IPCCall attribs = {
    {IPC_NTQUERYATTRIBUTESFILE_TAG, WCHAR_TYPE, ULONG_TYPE, INOUTPTR_TYPE},
    reinterpret_cast<CallbackGeneric>(
        &FilesystemDispatcher::NtQueryAttributesFile)
  };

  static const IPCCall full_attribs = {
    {IPC_NTQUERYFULLATTRIBUTESFILE_TAG, WCHAR_TYPE, ULONG_TYPE, INOUTPTR_TYPE},
    reinterpret_cast<CallbackGeneric>(
          &FilesystemDispatcher::NtQueryFullAttributesFile)
  };

  static const IPCCall set_info = {
    {IPC_NTSETINFO_RENAME_TAG, ULONG_TYPE, INOUTPTR_TYPE, INOUTPTR_TYPE,
     ULONG_TYPE, ULONG_TYPE},
    reinterpret_cast<CallbackGeneric>(
        &FilesystemDispatcher::NtSetInformationFile)
  };

  ipc_calls_.push_back(create_params);
  ipc_calls_.push_back(open_file);
  ipc_calls_.push_back(attribs);
  ipc_calls_.push_back(full_attribs);
  ipc_calls_.push_back(set_info);
}

bool FilesystemDispatcher::SetupService(InterceptionManager* manager,
                                        int service) {
  switch (service) {
    case IPC_NTCREATEFILE_TAG:
      return INTERCEPT_NT(manager, NtCreateFile, "_TargetNtCreateFile@48");

    case IPC_NTOPENFILE_TAG:
      return INTERCEPT_NT(manager, NtOpenFile, "_TargetNtOpenFile@28");

    case IPC_NTQUERYATTRIBUTESFILE_TAG:
      return INTERCEPT_NT(manager, NtQueryAttributesFile,
                          "_TargetNtQueryAttributesFile@12");

    case IPC_NTQUERYFULLATTRIBUTESFILE_TAG:
        return INTERCEPT_NT(manager, NtQueryFullAttributesFile,
                            "_TargetNtQueryFullAttributesFile@12");

    case IPC_NTSETINFO_RENAME_TAG:
      return INTERCEPT_NT(manager, NtSetInformationFile,
                          "_TargetNtSetInformationFile@24");

    default:
      return false;
  }
}

bool FilesystemDispatcher::NtCreateFile(
    IPCInfo* ipc, std::wstring* name, DWORD attributes, DWORD desired_access,
    DWORD file_attributes, DWORD share_access, DWORD create_disposition,
    DWORD create_options) {
  if (!PreProcessName(*name, name)) {
    // The path requested might contain a reparse point.
    ipc->return_info.nt_status = STATUS_ACCESS_DENIED;
    return true;
  }

  const wchar_t* filename = name->c_str();

  ULONG broker = TRUE;
  CountedParameterSet<OpenFile> params;
  params[OpenFile::NAME] = ParamPickerMake(filename);
  params[OpenFile::ACCESS] = ParamPickerMake(desired_access);
  params[OpenFile::OPTIONS] = ParamPickerMake(create_options);
  params[OpenFile::BROKER] = ParamPickerMake(broker);

  // To evaluate the policy we need to call back to the policy object. We
  // are just middlemen in the operation since is the FileSystemPolicy which
  // knows what to do.
  EvalResult result = policy_base_->EvalPolicy(IPC_NTCREATEFILE_TAG,
                                               params.GetBase());
  HANDLE handle;
  ULONG_PTR io_information = 0;
  NTSTATUS nt_status;
  if (!FileSystemPolicy::CreateFileAction(result, *ipc->client_info, *name,
                                          attributes, desired_access,
                                          file_attributes, share_access,
                                          create_disposition, create_options,
                                          &handle, &nt_status,
                                          &io_information)) {
    ipc->return_info.nt_status = STATUS_ACCESS_DENIED;
    return true;
  }
  // Return operation status on the IPC.
  ipc->return_info.extended[0].ulong_ptr = io_information;
  ipc->return_info.nt_status = nt_status;
  ipc->return_info.handle = handle;
  return true;
}

bool FilesystemDispatcher::NtOpenFile(
    IPCInfo* ipc, std::wstring* name, DWORD attributes, DWORD desired_access,
    DWORD share_access, DWORD open_options) {
  if (!PreProcessName(*name, name)) {
    // The path requested might contain a reparse point.
    ipc->return_info.nt_status = STATUS_ACCESS_DENIED;
    return true;
  }

  const wchar_t* filename = name->c_str();

  ULONG broker = TRUE;
  CountedParameterSet<OpenFile> params;
  params[OpenFile::NAME] = ParamPickerMake(filename);
  params[OpenFile::ACCESS] = ParamPickerMake(desired_access);
  params[OpenFile::OPTIONS] = ParamPickerMake(open_options);
  params[OpenFile::BROKER] = ParamPickerMake(broker);

  // To evaluate the policy we need to call back to the policy object. We
  // are just middlemen in the operation since is the FileSystemPolicy which
  // knows what to do.
  EvalResult result = policy_base_->EvalPolicy(IPC_NTOPENFILE_TAG,
                                               params.GetBase());
  HANDLE handle;
  ULONG_PTR io_information = 0;
  NTSTATUS nt_status;
  if (!FileSystemPolicy::OpenFileAction(result, *ipc->client_info, *name,
                                        attributes, desired_access,
                                        share_access, open_options, &handle,
                                        &nt_status, &io_information)) {
    ipc->return_info.nt_status = STATUS_ACCESS_DENIED;
    return true;
  }
  // Return operation status on the IPC.
  ipc->return_info.extended[0].ulong_ptr = io_information;
  ipc->return_info.nt_status = nt_status;
  ipc->return_info.handle = handle;
  return true;
}

bool FilesystemDispatcher::NtQueryAttributesFile(
    IPCInfo* ipc, std::wstring* name, DWORD attributes, CountedBuffer* info) {
  if (sizeof(FILE_BASIC_INFORMATION) != info->Size())
    return false;

  if (!PreProcessName(*name, name)) {
    // The path requested might contain a reparse point.
    ipc->return_info.nt_status = STATUS_ACCESS_DENIED;
    return true;
  }

  ULONG broker = TRUE;
  const wchar_t* filename = name->c_str();
  CountedParameterSet<FileName> params;
  params[FileName::NAME] = ParamPickerMake(filename);
  params[FileName::BROKER] = ParamPickerMake(broker);

  // To evaluate the policy we need to call back to the policy object. We
  // are just middlemen in the operation since is the FileSystemPolicy which
  // knows what to do.
  EvalResult result = policy_base_->EvalPolicy(IPC_NTQUERYATTRIBUTESFILE_TAG,
                                               params.GetBase());

  FILE_BASIC_INFORMATION* information =
        reinterpret_cast<FILE_BASIC_INFORMATION*>(info->Buffer());
  NTSTATUS nt_status;
  if (!FileSystemPolicy::QueryAttributesFileAction(result, *ipc->client_info,
                                                   *name, attributes,
                                                   information, &nt_status)) {
    ipc->return_info.nt_status = STATUS_ACCESS_DENIED;
    return true;
  }

  // Return operation status on the IPC.
  ipc->return_info.nt_status = nt_status;
  return true;
}

bool FilesystemDispatcher::NtQueryFullAttributesFile(
    IPCInfo* ipc, std::wstring* name, DWORD attributes, CountedBuffer* info) {
  if (sizeof(FILE_NETWORK_OPEN_INFORMATION) != info->Size())
    return false;

  if (!PreProcessName(*name, name)) {
    // The path requested might contain a reparse point.
    ipc->return_info.nt_status = STATUS_ACCESS_DENIED;
    return true;
  }

  ULONG broker = TRUE;
  const wchar_t* filename = name->c_str();
  CountedParameterSet<FileName> params;
  params[FileName::NAME] = ParamPickerMake(filename);
  params[FileName::BROKER] = ParamPickerMake(broker);

  // To evaluate the policy we need to call back to the policy object. We
  // are just middlemen in the operation since is the FileSystemPolicy which
  // knows what to do.
  EvalResult result = policy_base_->EvalPolicy(
                          IPC_NTQUERYFULLATTRIBUTESFILE_TAG, params.GetBase());

  FILE_NETWORK_OPEN_INFORMATION* information =
        reinterpret_cast<FILE_NETWORK_OPEN_INFORMATION*>(info->Buffer());
  NTSTATUS nt_status;
  if (!FileSystemPolicy::QueryFullAttributesFileAction(result,
                                                       *ipc->client_info,
                                                       *name, attributes,
                                                       information,
                                                       &nt_status)) {
    ipc->return_info.nt_status = STATUS_ACCESS_DENIED;
    return true;
  }

  // Return operation status on the IPC.
  ipc->return_info.nt_status = nt_status;
  return true;
}

bool FilesystemDispatcher::NtSetInformationFile(
    IPCInfo* ipc, HANDLE handle, CountedBuffer* status, CountedBuffer* info,
    DWORD length, DWORD info_class) {
  if (sizeof(IO_STATUS_BLOCK) != status->Size())
    return false;
  if (length != info->Size())
    return false;

  FILE_RENAME_INFORMATION* rename_info =
      reinterpret_cast<FILE_RENAME_INFORMATION*>(info->Buffer());

  if (!IsSupportedRenameCall(rename_info, length, info_class))
    return false;

  std::wstring name;
  name.assign(rename_info->FileName, rename_info->FileNameLength /
                                     sizeof(rename_info->FileName[0]));
  if (!PreProcessName(name, &name)) {
    // The path requested might contain a reparse point.
    ipc->return_info.nt_status = STATUS_ACCESS_DENIED;
    return true;
  }

  ULONG broker = TRUE;
  const wchar_t* filename = name.c_str();
  CountedParameterSet<FileName> params;
  params[FileName::NAME] = ParamPickerMake(filename);
  params[FileName::BROKER] = ParamPickerMake(broker);

  // To evaluate the policy we need to call back to the policy object. We
  // are just middlemen in the operation since is the FileSystemPolicy which
  // knows what to do.
  EvalResult result = policy_base_->EvalPolicy(IPC_NTSETINFO_RENAME_TAG,
                                               params.GetBase());

  IO_STATUS_BLOCK* io_status =
        reinterpret_cast<IO_STATUS_BLOCK*>(status->Buffer());
  NTSTATUS nt_status;
  if (!FileSystemPolicy::SetInformationFileAction(result, *ipc->client_info,
                                                  handle, rename_info, length,
                                                  info_class, io_status,
                                                  &nt_status)) {
    ipc->return_info.nt_status = STATUS_ACCESS_DENIED;
    return true;
  }

  // Return operation status on the IPC.
  ipc->return_info.nt_status = nt_status;
  return true;
}

}  // namespace sandbox
