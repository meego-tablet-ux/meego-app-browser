// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/crypto/rsa_private_key.h"

#include <openssl/evp.h>
#include <openssl/pkcs12.h>
#include <openssl/rsa.h>

#include "base/logging.h"
#include "base/openssl_util.h"
#include "base/scoped_ptr.h"
#include "base/stl_util-inl.h"

namespace base {

namespace {

// Function pointer definition, for injecting the required key export function
// into ExportKey, below. The supplied function should export EVP_PKEY into
// the supplied BIO, returning 1 on success or 0 on failure.
typedef int (ExportFunction)(BIO*, EVP_PKEY*);

// Helper to export |key| into |output| via the specified ExportFunction.
bool ExportKey(EVP_PKEY* key,
               ExportFunction export_fn,
               std::vector<uint8>* output) {
  if (!key)
    return false;

  ScopedOpenSSL<BIO, BIO_free_all> bio(BIO_new(BIO_s_mem()));

  int res = export_fn(bio.get(), key);
  ClearOpenSSLERRStack();
  if (!res)
    return false;

  char* data = NULL;
  long len = BIO_get_mem_data(bio.get(), &data);
  if (!data || len < 0)
    return false;

  STLAssignToVector(output, reinterpret_cast<const uint8*>(data), len);
  return true;
}

}  // namespace

// static
RSAPrivateKey* RSAPrivateKey::Create(uint16 num_bits) {
  EnsureOpenSSLInit();

  ScopedOpenSSL<RSA, RSA_free> rsa_key(RSA_generate_key(num_bits, 65537L,
                                                        NULL, NULL));
  ClearOpenSSLERRStack();
  if (!rsa_key.get())
    return NULL;

  scoped_ptr<RSAPrivateKey> result(new RSAPrivateKey);
  result->key_ = EVP_PKEY_new();
  if (!result->key_ || !EVP_PKEY_set1_RSA(result->key_, rsa_key.get()))
    return NULL;

  return result.release();
}

// static
RSAPrivateKey* RSAPrivateKey::CreateSensitive(uint16 num_bits) {
  NOTIMPLEMENTED();
  return NULL;
}

// static
RSAPrivateKey* RSAPrivateKey::CreateFromPrivateKeyInfo(
    const std::vector<uint8>& input) {
  EnsureOpenSSLInit();

  // BIO_new_mem_buf is not const aware, but it does not modify the buffer.
  char* data = reinterpret_cast<char*>(const_cast<uint8*>(input.data()));
  ScopedOpenSSL<BIO, BIO_free_all> bio(BIO_new_mem_buf(data, input.size()));
  if (!bio.get())
    return NULL;

  // Importing is a little more involved than exporting, as we must first
  // PKCS#8 decode the input, and then import the EVP_PKEY from Private Key
  // Info structure returned.
  ScopedOpenSSL<PKCS8_PRIV_KEY_INFO, PKCS8_PRIV_KEY_INFO_free> p8inf(
      d2i_PKCS8_PRIV_KEY_INFO_bio(bio.get(), NULL));
  ClearOpenSSLERRStack();
  if (!p8inf.get())
    return NULL;

  scoped_ptr<RSAPrivateKey> result(new RSAPrivateKey);
  result->key_ = EVP_PKCS82PKEY(p8inf.get());
  ClearOpenSSLERRStack();
  if (!result->key_)
    return NULL;

  return result.release();
}

// static
RSAPrivateKey* RSAPrivateKey::CreateSensitiveFromPrivateKeyInfo(
    const std::vector<uint8>& input) {
  NOTIMPLEMENTED();
  return NULL;
}

// static
RSAPrivateKey* RSAPrivateKey::FindFromPublicKeyInfo(
    const std::vector<uint8>& input) {
  NOTIMPLEMENTED();
  return NULL;
}

RSAPrivateKey::RSAPrivateKey()
    : key_(NULL) {
}

RSAPrivateKey::~RSAPrivateKey() {
  if (key_)
    EVP_PKEY_free(key_);
}

bool RSAPrivateKey::ExportPrivateKey(std::vector<uint8>* output) {
  return ExportKey(key_, i2d_PKCS8PrivateKeyInfo_bio, output);
}

bool RSAPrivateKey::ExportPublicKey(std::vector<uint8>* output) {
  return ExportKey(key_, i2d_PUBKEY_bio, output);
}

}  // namespace base
