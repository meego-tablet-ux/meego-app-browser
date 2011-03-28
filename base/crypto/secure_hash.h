// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_CRYPTO_SECURE_HASH_H_
#define BASE_CRYPTO_SECURE_HASH_H_
#pragma once

#include "base/base_api.h"
#include "base/basictypes.h"

namespace base {

// A wrapper to calculate secure hashes incrementally, allowing to
// be used when the full input is not known in advance.
class BASE_API SecureHash {
 public:
  enum Algorithm {
    SHA256,
  };
  virtual ~SecureHash() {}

  static SecureHash* Create(Algorithm type);

  virtual void Update(const void* input, size_t len) = 0;
  virtual void Finish(void* output, size_t len) = 0;

 protected:
  SecureHash() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(SecureHash);
};

}  // namespace base

#endif  // BASE_CRYPTO_SECURE_HASH_H_
