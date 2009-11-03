// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_creator.h"

#include <vector>
#include <string>

#include "base/crypto/rsa_private_key.h"
#include "base/crypto/signature_creator.h"
#include "base/file_util.h"
#include "base/scoped_handle.h"
#include "base/scoped_temp_dir.h"
#include "base/string_util.h"
#include "chrome/browser/extensions/sandboxed_extension_unpacker.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/zip.h"
#include "net/base/base64.h"

namespace {
  const int kRSAKeySize = 1024;
};

bool ExtensionCreator::InitializeInput(
    const FilePath& extension_dir,
    const FilePath& private_key_path,
    const FilePath& private_key_output_path) {
  // Validate input |extension_dir|.
  if (extension_dir.value().empty() ||
      !file_util::DirectoryExists(extension_dir)) {
    error_message_ = "Input directory must exist.";
    return false;
  }

  // Validate input |private_key| (if provided).
  if (!private_key_path.value().empty() &&
      !file_util::PathExists(private_key_path)) {
    error_message_ = "Input value for private key must be a valid path.";
    return false;
  }

  // If an |output_private_key| path is given, make sure it doesn't over-write
  // an existing private key.
  if (private_key_path.value().empty() &&
      !private_key_output_path.value().empty() &&
      file_util::PathExists(private_key_output_path)) {
      error_message_ = "A private key for specified extension already exists. "
                       "Reuse that key or delete it first.";
      return false;
  }

  return true;
}

base::RSAPrivateKey* ExtensionCreator::ReadInputKey(const FilePath&
    private_key_path) {
  if (!file_util::PathExists(private_key_path)) {
    error_message_ = "Input value for private key must exist.";
    return false;
  }

  std::string private_key_contents;
  if (!file_util::ReadFileToString(private_key_path,
      &private_key_contents)) {
    error_message_ = "Failed to read private key.";
    return false;
  }

  std::string private_key_bytes;
  if (!Extension::ParsePEMKeyBytes(private_key_contents,
       &private_key_bytes)) {
    error_message_ = "Invalid private key.";
    return false;
  }

  return base::RSAPrivateKey::CreateFromPrivateKeyInfo(
      std::vector<uint8>(private_key_bytes.begin(), private_key_bytes.end()));
}

base::RSAPrivateKey* ExtensionCreator::GenerateKey(const FilePath&
    output_private_key_path) {
  scoped_ptr<base::RSAPrivateKey> key_pair(
      base::RSAPrivateKey::Create(kRSAKeySize));
  if (!key_pair.get()) {
    error_message_ = "Yikes! Failed to generate random RSA private key.";
    return NULL;
  }

  std::vector<uint8> private_key_vector;
  if (!key_pair->ExportPrivateKey(&private_key_vector)) {
    error_message_ = "Failed to export private key.";
    return NULL;
  }
  std::string private_key_bytes(
      reinterpret_cast<char*>(&private_key_vector.front()),
      private_key_vector.size());

  std::string private_key;
  if (!Extension::ProducePEM(private_key_bytes, &private_key)) {
    error_message_ = "Failed to output private key.";
    return NULL;
  }
  std::string pem_output;
  if (!Extension::FormatPEMForFileOutput(private_key, &pem_output,
       false)) {
    error_message_ = "Failed to output private key.";
    return NULL;
  }

  if (!output_private_key_path.empty()) {
    if (-1 == file_util::WriteFile(output_private_key_path,
        pem_output.c_str(), pem_output.size())) {
      error_message_ = "Failed to write private key.";
      return NULL;
    }
  }

  return key_pair.release();
}

bool ExtensionCreator::CreateZip(const FilePath& extension_dir,
                                 const FilePath& temp_path,
                                 FilePath* zip_path) {
  *zip_path = temp_path.Append(FILE_PATH_LITERAL("extension.zip"));

  if (!Zip(extension_dir, *zip_path, false)) {  // no hidden files
    error_message_ = "Failed to create temporary zip file during packaging.";
    return false;
  }

  return true;
}

bool ExtensionCreator::SignZip(const FilePath& zip_path,
                               base::RSAPrivateKey* private_key,
                               std::vector<uint8>* signature) {
  scoped_ptr<base::SignatureCreator> signature_creator(
      base::SignatureCreator::Create(private_key));
  ScopedStdioHandle zip_handle(file_util::OpenFile(zip_path, "rb"));
  size_t buffer_size = 1 << 16;
  scoped_array<uint8> buffer(new uint8[buffer_size]);
  int bytes_read = -1;
  while ((bytes_read = fread(buffer.get(), 1, buffer_size,
       zip_handle.get())) > 0) {
    if (!signature_creator->Update(buffer.get(), bytes_read)) {
      error_message_ = "Error while signing extension.";
      return false;
    }
  }
  zip_handle.Close();

  signature_creator->Final(signature);
  return true;
}

bool ExtensionCreator::WriteCRX(const FilePath& zip_path,
                                base::RSAPrivateKey* private_key,
                                const std::vector<uint8>& signature,
                                const FilePath& crx_path) {
  if (file_util::PathExists(crx_path))
    file_util::Delete(crx_path, false);
  ScopedStdioHandle crx_handle(file_util::OpenFile(crx_path, "wb"));

  std::vector<uint8> public_key;
  if (!private_key->ExportPublicKey(&public_key)) {
    error_message_ = "Failed to export public key.";
    return false;
  }

  SandboxedExtensionUnpacker::ExtensionHeader header;
  memcpy(&header.magic, SandboxedExtensionUnpacker::kExtensionHeaderMagic,
         SandboxedExtensionUnpacker::kExtensionHeaderMagicSize);
  header.version = SandboxedExtensionUnpacker::kCurrentVersion;
  header.key_size = public_key.size();
  header.signature_size = signature.size();

  if (fwrite(&header, sizeof(SandboxedExtensionUnpacker::ExtensionHeader), 1,
             crx_handle.get()) != 1) {
    PLOG(ERROR) << "fwrite failed to write header";
  }
  if (fwrite(&public_key.front(), sizeof(uint8), public_key.size(),
             crx_handle.get()) != public_key.size()) {
    PLOG(ERROR) << "fwrite failed to write public_key.front";
  }
  if (fwrite(&signature.front(), sizeof(uint8), signature.size(),
             crx_handle.get()) != signature.size()) {
    PLOG(ERROR) << "fwrite failed to write signature.front";
  }

  size_t buffer_size = 1 << 16;
  scoped_array<uint8> buffer(new uint8[buffer_size]);
  size_t bytes_read = 0;
  ScopedStdioHandle zip_handle(file_util::OpenFile(zip_path, "rb"));
  while ((bytes_read = fread(buffer.get(), 1, buffer_size,
                             zip_handle.get())) > 0) {
    if (fwrite(buffer.get(), sizeof(char), bytes_read, crx_handle.get()) !=
        bytes_read) {
      PLOG(ERROR) << "fwrite failed to write buffer";
    }
  }

  return true;
}

bool ExtensionCreator::Run(const FilePath& extension_dir,
                           const FilePath& crx_path,
                           const FilePath& private_key_path,
                           const FilePath& output_private_key_path) {
  // Check input diretory and read manifest.
  if (!InitializeInput(extension_dir, private_key_path,
                       output_private_key_path)) {
    return false;
  }

  // Initialize Key Pair
  scoped_ptr<base::RSAPrivateKey> key_pair;
  if (!private_key_path.value().empty())
    key_pair.reset(ReadInputKey(private_key_path));
  else
    key_pair.reset(GenerateKey(output_private_key_path));
  if (!key_pair.get())
    return false;

  ScopedTempDir temp_dir;
  if (!temp_dir.CreateUniqueTempDir())
    return false;

  // Zip up the extension.
  FilePath zip_path;
  std::vector<uint8> signature;
  bool result = false;
  if (CreateZip(extension_dir, temp_dir.path(), &zip_path) &&
      SignZip(zip_path, key_pair.get(), &signature) &&
      WriteCRX(zip_path, key_pair.get(), signature, crx_path)) {
    result = true;
  }

  file_util::Delete(zip_path, false);
  return result;
}
