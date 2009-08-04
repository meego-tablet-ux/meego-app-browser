// Copyright (c) 2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/nss_init.h"

#include <dlfcn.h>
#include <nss.h>
#include <plarena.h>
#include <prerror.h>
#include <prinit.h>

// Work around https://bugzilla.mozilla.org/show_bug.cgi?id=455424
// until NSS 3.12.2 comes out and we update to it.
#define Lock FOO_NSS_Lock
#include <pk11pub.h>
#include <secmod.h>
#include <ssl.h>
#undef Lock

#include "base/file_util.h"
#include "base/logging.h"
#include "base/singleton.h"
#include "base/string_util.h"

namespace {

std::string GetDefaultConfigDirectory() {
  const char* home = getenv("HOME");
  if (home == NULL) {
    LOG(ERROR) << "$HOME is not set.";
    return "";
  }
  FilePath dir(home);
  dir = dir.AppendASCII(".pki").AppendASCII("nssdb");
  if (!file_util::CreateDirectory(dir)) {
    LOG(ERROR) << "Failed to create ~/.pki/nssdb directory.";
    return "";
  }
  return dir.value();
}

// Load nss's built-in root certs.
SECMODModule *InitDefaultRootCerts() {
  const char* kModulePath = "libnssckbi.so";
  char modparams[1024];
  snprintf(modparams, sizeof(modparams),
          "name=\"Root Certs\" library=\"%s\"", kModulePath);
  SECMODModule *root = SECMOD_LoadUserModule(modparams, NULL, PR_FALSE);
  if (root)
    return root;

  // Aw, snap.  Can't find/load root cert shared library.
  // This will make it hard to talk to anybody via https.
  NOTREACHED();
  return NULL;
}

class NSSInitSingleton {
 public:
  NSSInitSingleton() {
    SECStatus status;
    std::string database_dir = GetDefaultConfigDirectory();
    if (!database_dir.empty()) {
      // Initialize with a persistant database (~/.pki/nssdb).
      // Use "sql:" which can be shared by multiple processes safely.
      status = NSS_InitReadWrite(
          StringPrintf("sql:%s", database_dir.c_str()).c_str());
    } else {
      LOG(WARNING) << "Initialize NSS without using a persistent database "
                   << "(~/.pki/nssdb).";
      status = NSS_NoDB_Init(".");
    }
    if (status != SECSuccess) {
      char buffer[513] = "Couldn't retrieve error";
      PRInt32 err_length = PR_GetErrorTextLength();
      if (err_length > 0 && static_cast<size_t>(err_length) < sizeof(buffer))
        PR_GetErrorText(buffer);

      NOTREACHED() << "Error initializing NSS: " << buffer;
    }

    // If we haven't initialized the password for the NSS databases,
    // initialize an empty-string password so that we don't need to
    // log in.
    PK11SlotInfo* slot = PK11_GetInternalKeySlot();
    if (slot) {
      if (PK11_NeedUserInit(slot))
        PK11_InitPin(slot, NULL, NULL);
      PK11_FreeSlot(slot);
    }

    root_ = InitDefaultRootCerts();

    NSS_SetDomesticPolicy();

    // Use late binding to avoid scary but benign warning
    // "Symbol `SSL_ImplementedCiphers' has different size in shared object,
    //  consider re-linking"
    const PRUint16* pSSL_ImplementedCiphers = static_cast<const PRUint16*>(
        dlsym(RTLD_DEFAULT, "SSL_ImplementedCiphers"));
    if (pSSL_ImplementedCiphers == NULL) {
      NOTREACHED() << "Can't get list of supported ciphers";
      return;
    }

    // Explicitly enable exactly those ciphers with keys of at least 80 bits
    for (int i = 0; i < SSL_NumImplementedCiphers; i++) {
      SSLCipherSuiteInfo info;
      if (SSL_GetCipherSuiteInfo(pSSL_ImplementedCiphers[i], &info,
                                 sizeof(info)) == SECSuccess) {
        SSL_CipherPrefSetDefault(pSSL_ImplementedCiphers[i],
                                 (info.effectiveKeyBits >= 80));
      }
    }

    // Enable SSL
    SSL_OptionSetDefault(SSL_SECURITY, PR_TRUE);

    // All other SSL options are set per-session by SSLClientSocket.
  }

  ~NSSInitSingleton() {
    if (root_) {
      SECMOD_UnloadUserModule(root_);
      SECMOD_DestroyModule(root_);
      root_ = NULL;
    }

    // Have to clear the cache, or NSS_Shutdown fails with SEC_ERROR_BUSY
    SSL_ClearSessionCache();

    SECStatus status = NSS_Shutdown();
    if (status != SECSuccess) {
      // We LOG(INFO) because this failure is relatively harmless
      // (leaking, but we're shutting down anyway).
      LOG(INFO) << "NSS_Shutdown failed; see "
                   "http://code.google.com/p/chromium/issues/detail?id=4609";
    }

    PL_ArenaFinish();

    PRStatus prstatus = PR_Cleanup();
    if (prstatus != PR_SUCCESS) {
      // We LOG(ERROR) here because this failure is bad: it indicates
      // NSPR isn't initialized and cleaned up on the same thread.
      LOG(ERROR) << "PR_Cleanup failed; see "
                    "http://code.google.com/p/chromium/issues/detail?id=18410";
    }
  }

 private:
  SECMODModule *root_;
};

}  // namespace

namespace base {

void EnsureNSSInit() {
  Singleton<NSSInitSingleton>::get();
}

}  // namespace base
