// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/crypto/symmetric_key.h"

#include <nss.h>
#include <pk11pub.h>

#include "base/nss_util.h"
#include "base/logging.h"

namespace base {

// static
SymmetricKey* SymmetricKey::GenerateRandomKey(Algorithm algorithm, size_t key_size) {
  DCHECK_EQ(AES, algorithm);

  EnsureNSSInit();
  if (key_size == 0)
    return NULL;

  ScopedPK11Slot slot(PK11_GetBestSlot(CKM_AES_KEY_GEN, NULL));
  if (!slot.get())
    return NULL;

  PK11SymKey* sym_key = PK11_KeyGen(slot.get(), CKM_AES_KEY_GEN, NULL, key_size,
                                    NULL);
  if (!sym_key)
    return NULL;

  return new SymmetricKey(sym_key);
}

// static
SymmetricKey* SymmetricKey::DeriveKeyFromPassword(Algorithm algorithm,
                                                  const std::string& password,
                                                  const std::string& salt,
                                                  size_t iterations,
                                                  size_t key_size) {
  EnsureNSSInit();
  if (salt.empty() || iterations == 0 || key_size == 0)
    return NULL;

  SECItem password_item;
  password_item.type = siBuffer;
  password_item.data = reinterpret_cast<unsigned char*>(
      const_cast<char *>(password.data()));
  password_item.len = password.size();

  SECItem salt_item;
  salt_item.type = siBuffer;
  salt_item.data = reinterpret_cast<unsigned char*>(
      const_cast<char *>(salt.data()));
  salt_item.len = salt.size();


  SECOidTag cipher_algorithm =
      algorithm == AES ? SEC_OID_AES_256_CBC : SEC_OID_HMAC_SHA1;
  ScopedSECAlgorithmID alg_id(PK11_CreatePBEV2AlgorithmID(SEC_OID_PKCS5_PBKDF2,
                                                          cipher_algorithm,
                                                          SEC_OID_HMAC_SHA1,
                                                          key_size,
                                                          iterations,
                                                          &salt_item));
  if (!alg_id.get())
    return NULL;

  ScopedPK11Slot slot(PK11_GetBestSlot(SEC_OID_PKCS5_PBKDF2, NULL));
  if (!slot.get())
    return NULL;

  PK11SymKey* sym_key = PK11_PBEKeyGen(slot.get(), alg_id.get(), &password_item,
                                       PR_FALSE, NULL);
  if (!sym_key)
    return NULL;

  return new SymmetricKey(sym_key);
}

bool SymmetricKey::GetRawKey(std::string* raw_key) {
  SECStatus rv = PK11_ExtractKeyValue(key_.get());
  if (SECSuccess != rv)
    return false;

  SECItem* key_item = PK11_GetKeyData(key_.get());
  if (!key_item)
    return false;

  raw_key->assign(reinterpret_cast<char*>(key_item->data), key_item->len);
  return true;
}

}  // namespace base
