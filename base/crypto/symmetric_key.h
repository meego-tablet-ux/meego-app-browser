// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_CRYPTO_SYMMETRIC_KEY_H_
#define BASE_CRYPTO_SYMMETRIC_KEY_H_

#include <string>

#include "base/basictypes.h"

#if defined(USE_NSS)
#include "base/crypto/scoped_nss_types.h"
#elif defined(OS_MACOSX)
#include <Security/cssmtype.h>
#endif

namespace base {

// Wraps a platform-specific symmetric key and allows it to be held in a
// scoped_ptr.
class SymmetricKey {
 public:
  enum Algorithm {
    AES,
    HMAC_SHA1,
  };

  virtual ~SymmetricKey() {}

  // Generates a random key suitable to be used with |cipher| and of
  // |key_size_in_bits| bits.
  // The caller is responsible for deleting the returned SymmetricKey.
  static SymmetricKey* GenerateRandomKey(Algorithm algorithm,
                                         size_t key_size_in_bits);

  // Derives a key from the supplied password and salt using PBKDF2. The caller
  // is responsible for deleting the returned SymmetricKey.
  static SymmetricKey* DeriveKeyFromPassword(Algorithm algorithm,
                                             const std::string& password,
                                             const std::string& salt,
                                             size_t iterations,
                                             size_t key_size_in_bits);

#if defined(USE_NSS)
  PK11SymKey* key() const { return key_.get(); }
#elif defined(OS_MACOSX)
  CSSM_DATA cssm_data() const;
#endif

  // Extracts the raw key from the platform specific data. This should only be
  // done in unit tests to verify that keys are generated correctly.
  bool GetRawKey(std::string* raw_key);

 private:
#if defined(USE_NSS)
  explicit SymmetricKey(PK11SymKey* key) : key_(key) {}
  ScopedPK11SymKey key_;
#elif defined(OS_MACOSX)
  SymmetricKey(const void* key_data, size_t key_size_in_bits);
  std::string key_;
#endif

  DISALLOW_COPY_AND_ASSIGN(SymmetricKey);
};

}  // namespace base

#endif   // BASE_CRYPTO_SYMMETRIC_KEY_H_
