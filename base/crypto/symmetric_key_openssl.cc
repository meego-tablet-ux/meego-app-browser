// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/crypto/symmetric_key.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <algorithm>

#include "base/logging.h"
#include "base/openssl_util.h"
#include "base/scoped_ptr.h"
#include "base/string_util.h"

namespace base {

SymmetricKey::~SymmetricKey() {
  std::fill(key_.begin(), key_.end(), '\0');  // Zero out the confidential key.
}

// static
SymmetricKey* SymmetricKey::GenerateRandomKey(Algorithm algorithm,
                                              size_t key_size_in_bits) {
  DCHECK_EQ(AES, algorithm);
  int key_size_in_bytes = key_size_in_bits / 8;
  DCHECK_EQ(static_cast<int>(key_size_in_bits), key_size_in_bytes * 8);

  if (key_size_in_bits == 0)
    return NULL;

  scoped_ptr<SymmetricKey> key(new SymmetricKey);
  uint8* key_data =
      reinterpret_cast<uint8*>(WriteInto(&key->key_, key_size_in_bytes + 1));

  int res = RAND_bytes(key_data, key_size_in_bytes);
  if (res != 1) {
    DLOG(ERROR) << "RAND_bytes failed. res = " << res;
    ClearOpenSSLERRStack();
    return NULL;
  }
  return key.release();
}

// static
SymmetricKey* SymmetricKey::DeriveKeyFromPassword(Algorithm algorithm,
                                                  const std::string& password,
                                                  const std::string& salt,
                                                  size_t iterations,
                                                  size_t key_size_in_bits) {
  DCHECK(algorithm == AES || algorithm == HMAC_SHA1);
  int key_size_in_bytes = key_size_in_bits / 8;
  DCHECK_EQ(static_cast<int>(key_size_in_bits), key_size_in_bytes * 8);

  scoped_ptr<SymmetricKey> key(new SymmetricKey);
  uint8* key_data =
      reinterpret_cast<uint8*>(WriteInto(&key->key_, key_size_in_bytes + 1));
  int res = PKCS5_PBKDF2_HMAC_SHA1(password.data(), password.length(),
                                   reinterpret_cast<const uint8*>(salt.data()),
                                   salt.length(), iterations,
                                   key_size_in_bytes, key_data);
  if (res != 1) {
    DLOG(ERROR) << "HMAC SHA1 failed. res = " << res;
    ClearOpenSSLERRStack();
    return NULL;
  }
  return key.release();
}

// static
SymmetricKey* SymmetricKey::Import(Algorithm algorithm,
                                   const std::string& raw_key) {
  scoped_ptr<SymmetricKey> key(new SymmetricKey);
  key->key_ = raw_key;
  return key.release();
}

bool SymmetricKey::GetRawKey(std::string* raw_key) {
  *raw_key = key_;
  return true;
}

}  // namespace base
