// Copyright 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "chrome/browser/sandbox_policy.h"

#include "base/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/registry.h"
#include "base/string_util.h"
#include "base/win_util.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/ipc_logging.h"
#include "chrome/common/win_util.h"
#include "webkit/glue/plugins/plugin_list.h"

PluginPolicyCategory GetPolicyCategoryForPlugin(
    const std::wstring& dll,
    const std::wstring& clsid,
    const std::wstring& list) {
  std::wstring filename = file_util::GetFilenameFromPath(dll);
  std::wstring plugin_dll = StringToLowerASCII(filename);
  std::wstring trusted_plugins = StringToLowerASCII(list);
  std::wstring activex_clsid = StringToLowerASCII(clsid);

  size_t pos = 0;
  size_t end_item = 0;
  while(end_item != std::wstring::npos) {
    end_item = list.find(L",", pos);

    size_t size_item = (end_item == std::wstring::npos) ? end_item :
                                                          end_item - pos;
    std::wstring item = list.substr(pos, size_item);
    if (!item.empty()) {
      if (item == activex_clsid || item == plugin_dll)
        return PLUGIN_GROUP_TRUSTED;
    }

    pos = end_item + 1;
  }

  return PLUGIN_GROUP_UNTRUSTED;
}

// Adds the policy rules for the path and path\* with the semantic |access|.
// We need to add the wildcard rules to also apply the rule to the subfiles
// and subfolders.
bool AddDirectoryAndChildren(int path, const wchar_t* sub_dir,
                             sandbox::TargetPolicy::Semantics access,
                             sandbox::TargetPolicy* policy) {
  std::wstring directory;
  if (!PathService::Get(path, &directory))
    return false;

  if (sub_dir)
    file_util::AppendToPath(&directory, sub_dir);

  sandbox::ResultCode result;
  result = policy->AddRule(sandbox::TargetPolicy::SUBSYS_FILES, access,
                           directory.c_str());
  if (result != sandbox::SBOX_ALL_OK)
    return false;

  file_util::AppendToPath(&directory, L"*");
  result = policy->AddRule(sandbox::TargetPolicy::SUBSYS_FILES, access,
                           directory.c_str());
  if (result != sandbox::SBOX_ALL_OK)
    return false;

  return true;
}

// Adds the policy rules for the path and path\* with the semantic |access|.
// We need to add the wildcard rules to also apply the rule to the subkeys.
bool AddKeyAndSubkeys(std::wstring key,
                      sandbox::TargetPolicy::Semantics access,
                      sandbox::TargetPolicy* policy) {
  sandbox::ResultCode result;
  result = policy->AddRule(sandbox::TargetPolicy::SUBSYS_REGISTRY, access,
                           key.c_str());
  if (result != sandbox::SBOX_ALL_OK)
    return false;

  key += L"\\*";
  result = policy->AddRule(sandbox::TargetPolicy::SUBSYS_REGISTRY, access,
                           key.c_str());
  if (result != sandbox::SBOX_ALL_OK)
    return false;

  return true;
}

bool AddGenericPolicy(sandbox::TargetPolicy* policy) {
  sandbox::ResultCode result;

  // Add the policy for the pipes
  result = policy->AddRule(sandbox::TargetPolicy::SUBSYS_FILES,
                           sandbox::TargetPolicy::FILES_ALLOW_ANY,
                           L"\\??\\pipe\\chrome.*");
  if (result != sandbox::SBOX_ALL_OK)
    return false;

#ifdef IPC_MESSAGE_LOG_ENABLED
  // Add the policy for the IPC logging events.
  result = policy->AddRule(sandbox::TargetPolicy::SUBSYS_SYNC,
                           sandbox::TargetPolicy::EVENTS_ALLOW_ANY,
                           IPC::Logging::GetEventName(true).c_str());
  if (result != sandbox::SBOX_ALL_OK)
    return false;

  result = policy->AddRule(sandbox::TargetPolicy::SUBSYS_SYNC,
                           sandbox::TargetPolicy::EVENTS_ALLOW_ANY,
                           IPC::Logging::GetEventName(false).c_str());
  if (result != sandbox::SBOX_ALL_OK)
    return false;
#endif

  // Add the policy for debug message only in debug
#ifndef NDEBUG
  std::wstring debug_message;
  if (!PathService::Get(chrome::DIR_APP, &debug_message))
    return false;
  if (!win_util::ConvertToLongPath(debug_message, &debug_message))
    return false;
  file_util::AppendToPath(&debug_message, L"debug_message.exe");
  result = policy->AddRule(sandbox::TargetPolicy::SUBSYS_PROCESS,
                           sandbox::TargetPolicy::PROCESS_MIN_EXEC,
                           debug_message.c_str());
  if (result != sandbox::SBOX_ALL_OK)
    return false;
#endif  // NDEBUG

  return true;
}

bool ApplyPolicyForTrustedPlugin(sandbox::TargetPolicy* policy) {
  policy->SetJobLevel(sandbox::JOB_UNPROTECTED, 0);
  policy->SetTokenLevel(sandbox::USER_UNPROTECTED, sandbox::USER_UNPROTECTED);
  return true;
}

bool ApplyPolicyForUntrustedPlugin(sandbox::TargetPolicy* policy) {
  policy->SetJobLevel(sandbox::JOB_UNPROTECTED, 0);

  sandbox::TokenLevel initial_token = sandbox::USER_UNPROTECTED;
  if (win_util::GetWinVersion() > win_util::WINVERSION_XP) {
    // On 2003/Vista the initial token has to be restricted if the main token
    // is restricted.
    initial_token = sandbox::USER_RESTRICTED_SAME_ACCESS;
  }
  policy->SetTokenLevel(initial_token, sandbox::USER_LIMITED);
  policy->SetDelayedIntegrityLevel(sandbox::INTEGRITY_LEVEL_LOW);

  if (!AddDirectoryAndChildren(base::DIR_TEMP, NULL,
                               sandbox::TargetPolicy::FILES_ALLOW_ANY, policy))
    return false;

  if (!AddDirectoryAndChildren(base::DIR_IE_INTERNET_CACHE, NULL,
                               sandbox::TargetPolicy::FILES_ALLOW_ANY, policy))
    return false;


  if (!AddDirectoryAndChildren(base::DIR_APP_DATA, NULL,
                               sandbox::TargetPolicy::FILES_ALLOW_READONLY,
                               policy))
    return false;

  if (!AddDirectoryAndChildren(base::DIR_APP_DATA, L"Macromedia",
                               sandbox::TargetPolicy::FILES_ALLOW_ANY,
                               policy))
    return false;

  if (!AddDirectoryAndChildren(base::DIR_LOCAL_APP_DATA, NULL,
                               sandbox::TargetPolicy::FILES_ALLOW_READONLY,
                               policy))
    return false;

  if (!AddKeyAndSubkeys(L"HKEY_CURRENT_USER\\SOFTWARE\\MACROMEDIA",
                        sandbox::TargetPolicy::REG_ALLOW_ANY,
                        policy))
    return false;

  if (win_util::GetWinVersion() == win_util::WINVERSION_VISTA) {
    if (!AddKeyAndSubkeys(L"HKEY_CURRENT_USER\\SOFTWARE\\AppDataLow",
                          sandbox::TargetPolicy::REG_ALLOW_ANY,
                          policy))
      return false;

    if (!AddDirectoryAndChildren(base::DIR_LOCAL_APP_DATA_LOW, NULL,
                                 sandbox::TargetPolicy::FILES_ALLOW_ANY,
                                 policy))
      return false;
  }

  return true;
}

bool AddPolicyForPlugin(const std::wstring &plugin_dll,
                        const std::string &activex_clsid,
                        const std::wstring &trusted_plugins,
                        sandbox::TargetPolicy* policy) {
  // Add the policy for the pipes.
  sandbox::ResultCode result = sandbox::SBOX_ALL_OK;
  result = policy->AddRule(sandbox::TargetPolicy::SUBSYS_NAMED_PIPES,
                           sandbox::TargetPolicy::NAMEDPIPES_ALLOW_ANY,
                           L"\\\\.\\pipe\\chrome.*");
  if (result != sandbox::SBOX_ALL_OK)
    return false;

  std::wstring clsid = UTF8ToWide(activex_clsid);

  PluginPolicyCategory policy_category =
      GetPolicyCategoryForPlugin(plugin_dll, clsid, trusted_plugins);

  switch (policy_category) {
    case PLUGIN_GROUP_TRUSTED:
      return ApplyPolicyForTrustedPlugin(policy);
    case PLUGIN_GROUP_UNTRUSTED:
      return ApplyPolicyForUntrustedPlugin(policy);
    default:
      NOTREACHED();
      break;
  }

  return false;
}