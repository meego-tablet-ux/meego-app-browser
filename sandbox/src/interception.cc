// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// For information about interceptions as a whole see
// http://dev.chromium.org/developers/design-documents/sandbox .

#include <set>

#include "sandbox/src/interception.h"

#include "base/logging.h"
#include "base/pe_image.h"
#include "base/scoped_ptr.h"
#include "sandbox/src/interception_internal.h"
#include "sandbox/src/sandbox.h"
#include "sandbox/src/sandbox_utils.h"
#include "sandbox/src/service_resolver.h"
#include "sandbox/src/target_interceptions.h"
#include "sandbox/src/target_process.h"
#include "sandbox/src/wow64.h"

namespace {

const char kMapViewOfSectionName[] = "NtMapViewOfSection";
const char kUnmapViewOfSectionName[] = "NtUnmapViewOfSection";

}  // namespace

namespace sandbox {

SANDBOX_INTERCEPT SharedMemory* g_interceptions;

// Magic constant that identifies that this function is not to be patched.
const char kUnloadDLLDummyFunction[] = "@";

InterceptionManager::InterceptionManager(TargetProcess* child_process,
                                         bool relaxed)
    : child_(child_process), names_used_(false), relaxed_(relaxed) {
  child_->AddRef();
}
InterceptionManager::~InterceptionManager() {
  child_->Release();
}

bool InterceptionManager::AddToPatchedFunctions(
    const wchar_t* dll_name, const char* function_name,
    InterceptionType interception_type, const void* replacement_code_address) {
  InterceptionData function;
  function.type = interception_type;
  function.dll = dll_name;
  function.function = function_name;
  function.interceptor_address = replacement_code_address;

  interceptions_.push_back(function);
  return true;
}

bool InterceptionManager::AddToPatchedFunctions(
    const wchar_t* dll_name, const char* function_name,
    InterceptionType interception_type, const char* replacement_function_name) {
  InterceptionData function;
  function.type = interception_type;
  function.dll = dll_name;
  function.function = function_name;
  function.interceptor = replacement_function_name;
  function.interceptor_address = NULL;

  interceptions_.push_back(function);
  names_used_ = true;
  return true;
}

bool InterceptionManager::AddToUnloadModules(const wchar_t* dll_name) {
  InterceptionData module_to_unload;
  module_to_unload.type = INTERCEPTION_UNLOAD_MODULE;
  module_to_unload.dll = dll_name;
  // The next two are dummy values that make the structures regular, instead
  // of having special cases. They should not be used.
  module_to_unload.function = kUnloadDLLDummyFunction;
  module_to_unload.interceptor_address = reinterpret_cast<void*>(1);

  interceptions_.push_back(module_to_unload);
  return true;
}

bool InterceptionManager::InitializeInterceptions() {
  if (interceptions_.empty())
    return true;  // Nothing to do here

  size_t buffer_bytes = GetBufferSize();
  scoped_array<char> local_buffer(new char[buffer_bytes]);

  if (!SetupConfigBuffer(local_buffer.get(), buffer_bytes))
    return false;

  void* remote_buffer;
  if (!CopyDataToChild(local_buffer.get(), buffer_bytes, &remote_buffer))
    return false;

  bool hot_patch_needed = (0 != buffer_bytes);
  if (!PatchNtdll(hot_patch_needed))
    return false;

  g_interceptions = reinterpret_cast<SharedMemory*>(remote_buffer);
  child_->TransferVariable("sandbox::g_interceptions", &g_interceptions,
                           sizeof(g_interceptions));
  return true;
}

size_t InterceptionManager::GetBufferSize() const {
  std::set<std::wstring> dlls;
  size_t buffer_bytes = 0;

  std::list<InterceptionData>::const_iterator it = interceptions_.begin();
  for (; it != interceptions_.end(); ++it) {
    // skip interceptions that are performed from the parent
    if (!IsInterceptionPerformedByChild(*it))
      continue;

    if (!dlls.count(it->dll)) {
      // NULL terminate the dll name on the structure
      size_t dll_name_bytes = (it->dll.size() + 1) * sizeof(wchar_t);

      // include the dll related size
      buffer_bytes += RoundUpToMultiple(offsetof(DllPatchInfo, dll_name) +
                                            dll_name_bytes, sizeof(size_t));
      dlls.insert(it->dll);
    }

    // we have to NULL terminate the strings on the structure
    size_t strings_chars = it->function.size() + it->interceptor.size() + 2;

    // a new FunctionInfo is required per function
    size_t record_bytes = offsetof(FunctionInfo, function) + strings_chars;
    record_bytes = RoundUpToMultiple(record_bytes, sizeof(size_t));
    buffer_bytes += record_bytes;
  }

  if (0 != buffer_bytes)
    // add the part of SharedMemory that we have not counted yet
    buffer_bytes += offsetof(SharedMemory, dll_list);

  return buffer_bytes;
}

// Basically, walk the list of interceptions moving them to the config buffer,
// but keeping together all interceptions that belong to the same dll.
// The config buffer is a local buffer, not the one allocated on the child.
bool InterceptionManager::SetupConfigBuffer(void* buffer, size_t buffer_bytes) {
  if (0 == buffer_bytes)
    return true;

  DCHECK(buffer_bytes > sizeof(SharedMemory));

  SharedMemory* shared_memory = reinterpret_cast<SharedMemory*>(buffer);
  DllPatchInfo* dll_info = shared_memory->dll_list;
  int num_dlls = 0;

  shared_memory->interceptor_base = names_used_ ? child_->MainModule() : NULL;

  buffer_bytes -= offsetof(SharedMemory, dll_list);
  buffer = dll_info;

  std::list<InterceptionData>::iterator it = interceptions_.begin();
  for (; it != interceptions_.end();) {
    // skip interceptions that are performed from the parent
    if (!IsInterceptionPerformedByChild(*it)) {
      ++it;
      continue;
    }

    const std::wstring dll = it->dll;
    if (!SetupDllInfo(*it, &buffer, &buffer_bytes))
      return false;

    // walk the interceptions from this point, saving the ones that are
    // performed on this dll, and removing the entry from the list.
    // advance the iterator before removing the element from the list
    std::list<InterceptionData>::iterator rest = it;
    for (; rest != interceptions_.end();) {
      if (rest->dll == dll) {
        if (!SetupInterceptionInfo(*rest, &buffer, &buffer_bytes, dll_info))
          return false;
        if (it == rest)
          ++it;
        rest = interceptions_.erase(rest);
      } else {
        ++rest;
      }
    }
    dll_info = reinterpret_cast<DllPatchInfo*>(buffer);
    ++num_dlls;
  }

  shared_memory->num_intercepted_dlls = num_dlls;
  return true;
}

// Fills up just the part that depends on the dll, not the info that depends on
// the actual interception.
bool InterceptionManager::SetupDllInfo(const InterceptionData& data,
                                       void** buffer,
                                       size_t* buffer_bytes) const {
  DCHECK(buffer_bytes);
  DCHECK(buffer);
  DCHECK(*buffer);

  DllPatchInfo* dll_info = reinterpret_cast<DllPatchInfo*>(*buffer);

  // the strings have to be zero terminated
  size_t required = offsetof(DllPatchInfo, dll_name) +
                    (data.dll.size() + 1) * sizeof(wchar_t);
  required = RoundUpToMultiple(required, sizeof(size_t));
  if (*buffer_bytes < required)
    return false;

  *buffer_bytes -= required;
  *buffer = reinterpret_cast<char*>(*buffer) + required;

  // set up the dll info to be what we know about it at this time
  dll_info->unload_module = (data.type == INTERCEPTION_UNLOAD_MODULE);
  dll_info->record_bytes = required;
  dll_info->offset_to_functions = required;
  dll_info->num_functions = 0;
  data.dll._Copy_s(dll_info->dll_name, data.dll.size(), data.dll.size());
  dll_info->dll_name[data.dll.size()] = L'\0';

  return true;
}

bool InterceptionManager::SetupInterceptionInfo(const InterceptionData& data,
                                                void** buffer,
                                                size_t* buffer_bytes,
                                                DllPatchInfo* dll_info) const {
  DCHECK(buffer_bytes);
  DCHECK(buffer);
  DCHECK(*buffer);

  if ((dll_info->unload_module) &&
      (data.function != kUnloadDLLDummyFunction)) {
    // Can't specify a dll for both patch and unload.
    NOTREACHED();
  }

  FunctionInfo* function = reinterpret_cast<FunctionInfo*>(*buffer);

  size_t name_bytes = data.function.size();
  size_t interceptor_bytes = data.interceptor.size();

  // the strings at the end of the structure are zero terminated
  size_t required = offsetof(FunctionInfo, function) +
                    name_bytes + interceptor_bytes + 2;
  required = RoundUpToMultiple(required, sizeof(size_t));
  if (*buffer_bytes < required)
    return false;

  // update the caller's values
  *buffer_bytes -= required;
  *buffer = reinterpret_cast<char*>(*buffer) + required;

  function->record_bytes = required;
  function->type = data.type;
  function->interceptor_address = data.interceptor_address;
  char* names = function->function;

  data.function._Copy_s(names, name_bytes, name_bytes);
  names += name_bytes;
  *names++ = '\0';

  // interceptor follows the function_name
  data.interceptor._Copy_s(names, interceptor_bytes, interceptor_bytes);
  names += interceptor_bytes;
  *names++ = '\0';

  // update the dll table
  dll_info->num_functions++;
  dll_info->record_bytes += required;

  return true;
}

bool InterceptionManager::CopyDataToChild(const void* local_buffer,
                                          size_t buffer_bytes,
                                          void** remote_buffer) const {
  DCHECK(NULL != remote_buffer);
  if (0 == buffer_bytes) {
    *remote_buffer = NULL;
    return true;
  }

  HANDLE child = child_->Process();

  // Allocate memory on the target process without specifying the address
  void* remote_data = ::VirtualAllocEx(child, NULL, buffer_bytes,
                                       MEM_COMMIT, PAGE_READWRITE);
  if (NULL == remote_data)
    return false;

  SIZE_T bytes_written;
  BOOL success = ::WriteProcessMemory(child, remote_data, local_buffer,
                                      buffer_bytes, &bytes_written);
  if (FALSE == success || bytes_written != buffer_bytes) {
    ::VirtualFreeEx(child, remote_data, 0, MEM_RELEASE);
    return false;
  }

  *remote_buffer = remote_data;

  return true;
}

// Only return true if the child should be able to perform this interception.
bool InterceptionManager::IsInterceptionPerformedByChild(
    const InterceptionData& data) const {
  if (INTERCEPTION_INVALID == data.type)
    return false;

  if (INTERCEPTION_SERVICE_CALL == data.type)
    return false;

  if (data.type >= INTERCEPTION_LAST)
    return false;

  std::wstring ntdll(kNtdllName);
  if (ntdll == data.dll)
    return false;  // ntdll has to be intercepted from the parent

  return true;
}

bool InterceptionManager::PatchNtdll(bool hot_patch_needed) {
  // Maybe there is nothing to do
  if (!hot_patch_needed && interceptions_.empty())
    return true;

  if (hot_patch_needed) {
#if SANDBOX_EXPORTS
    // Make sure the functions are not excluded by the linker.
    #pragma comment(linker, "/include:_TargetNtMapViewOfSection@44")
    #pragma comment(linker, "/include:_TargetNtUnmapViewOfSection@12")

    AddToPatchedFunctions(kNtdllName, kMapViewOfSectionName,
                          INTERCEPTION_SERVICE_CALL,
                          "_TargetNtMapViewOfSection@44");
    AddToPatchedFunctions(kNtdllName, kUnmapViewOfSectionName,
                          INTERCEPTION_SERVICE_CALL,
                          "_TargetNtUnmapViewOfSection@12");
#else
    AddToPatchedFunctions(kNtdllName, kMapViewOfSectionName,
                          INTERCEPTION_SERVICE_CALL, &TargetNtMapViewOfSection);
    AddToPatchedFunctions(kNtdllName, kUnmapViewOfSectionName,
                          INTERCEPTION_SERVICE_CALL,
                          &TargetNtUnmapViewOfSection);
#endif
  }

  size_t thunk_bytes = interceptions_.size() * sizeof(ThunkData) +
                       sizeof(DllInterceptionData);

  // Allocate memory on the child, without specifying the desired address
  HANDLE child = child_->Process();
  DllInterceptionData* thunks = reinterpret_cast<DllInterceptionData*>(
                                    ::VirtualAllocEx(child, NULL, thunk_bytes,
                                                     MEM_COMMIT,
                                                     PAGE_EXECUTE_READWRITE));

  DllInterceptionData dll_data;
  dll_data.data_bytes = thunk_bytes;
  dll_data.num_thunks = 0;
  dll_data.used_bytes = offsetof(DllInterceptionData, thunks);

  // this should write all the individual thunks to the child's memory
  if (!PatchClientFunctions(thunks, thunk_bytes, &dll_data))
    return false;

  // and now write the first part of the table to the child's memory
  DWORD written;
  bool ok = FALSE != ::WriteProcessMemory(child, thunks, &dll_data,
                                          offsetof(DllInterceptionData, thunks),
                                          &written);

  if (!ok || (offsetof(DllInterceptionData, thunks) != written))
    return false;

  // Attempt to protect all the thunks, but ignore failure
  DWORD old_protection;
  ::VirtualProtectEx(child, thunks, thunk_bytes,
                     PAGE_EXECUTE_READ, &old_protection);

  return true;
}

bool InterceptionManager::PatchClientFunctions(DllInterceptionData* thunks,
                                               size_t thunk_bytes,
                                               DllInterceptionData* dll_data) {
  DCHECK(NULL != thunks);
  DCHECK(NULL != dll_data);

  HMODULE ntdll_base = ::GetModuleHandle(kNtdllName);
  if (!ntdll_base)
    return false;

  PEImage ntdll_image(ntdll_base);

  // Bypass purify's interception.
  wchar_t* loader_get = reinterpret_cast<wchar_t*>(
                            ntdll_image.GetProcAddress("LdrGetDllHandle"));
  if (loader_get) {
    if (!GetModuleHandleHelper(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                   GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               loader_get, &ntdll_base))
      return false;
  }

  Wow64 WowHelper(child_, ntdll_base);
  if (!WowHelper.WaitForNtdll(INFINITE))
    return false;

  char* interceptor_base = NULL;

#if SANDBOX_EXPORTS
  interceptor_base = reinterpret_cast<char*>(child_->MainModule());
  HMODULE local_interceptor = ::LoadLibrary(child_->Name());
#endif

  ServiceResolverThunk* thunk;
  if (WowHelper.IsWow64())
    thunk = new Wow64ResolverThunk(child_->Process(), relaxed_);
  else if (!IsXPSP2OrLater())
    thunk = new Win2kResolverThunk(child_->Process(), relaxed_);
  else
    thunk = new ServiceResolverThunk(child_->Process(), relaxed_);

  std::list<InterceptionData>::iterator it = interceptions_.begin();
  for (; it != interceptions_.end(); ++it) {
    const std::wstring ntdll(kNtdllName);
    if (it->dll != ntdll)
      break;

    if (INTERCEPTION_SERVICE_CALL != it->type)
      break;

#if SANDBOX_EXPORTS
    // We may be trying to patch by function name.
    if (NULL == it->interceptor_address) {
      const char* address;
      NTSTATUS ret = thunk->ResolveInterceptor(local_interceptor,
                                               it->interceptor.c_str(),
                                               reinterpret_cast<const void**>(
                                               &address));
      if (!NT_SUCCESS(ret))
        break;

      // Translate the local address to an address on the child.
      it->interceptor_address = interceptor_base + (address -
                                    reinterpret_cast<char*>(local_interceptor));
    }
#endif
    NTSTATUS ret = thunk->Setup(ntdll_base,
                                interceptor_base,
                                it->function.c_str(),
                                it->interceptor.c_str(),
                                it->interceptor_address,
                                &thunks->thunks[dll_data->num_thunks],
                                thunk_bytes - dll_data->used_bytes,
                                NULL);
    if (!NT_SUCCESS(ret))
      break;

    dll_data->num_thunks++;
    dll_data->used_bytes += sizeof(ThunkData);
  }

  delete(thunk);

#if SANDBOX_EXPORTS
  if (NULL != local_interceptor)
    ::FreeLibrary(local_interceptor);
#endif

  if (it != interceptions_.end())
    return false;

  return true;
}

}  // namespace sandbox
