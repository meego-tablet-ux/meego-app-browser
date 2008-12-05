// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extensions_service.h"

#include "base/file_util.h"
#include "base/values.h"
#include "base/string_util.h"
#include "base/thread.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/json_value_serializer.h"

// ExtensionsService

const FilePath::CharType* ExtensionsService::kInstallDirectoryName =
    FILE_PATH_LITERAL("Extensions");

ExtensionsService::ExtensionsService(const FilePath& profile_directory)
    : message_loop_(MessageLoop::current()),
      backend_(new ExtensionsServiceBackend),
      install_directory_(profile_directory.Append(kInstallDirectoryName)) {
}

ExtensionsService::~ExtensionsService() {
  for (ExtensionList::iterator iter = extensions_.begin();
       iter != extensions_.end(); ++iter) {
    delete *iter;
  }
}

bool ExtensionsService::Init() {
  // TODO(aa): This message loop should probably come from a backend
  // interface, similar to how the message loop for the frontend comes
  // from the frontend interface.
  g_browser_process->file_thread()->message_loop()->PostTask(FROM_HERE,
      NewRunnableMethod(backend_.get(),
          &ExtensionsServiceBackend::LoadExtensionsFromDirectory,
          install_directory_,
          scoped_refptr<ExtensionsServiceFrontendInterface>(this)));
  // TODO(aa): Load extensions from other registered directories.

  return true;
}

MessageLoop* ExtensionsService::GetMessageLoop() {
  return message_loop_;
}

void ExtensionsService::OnExtensionsLoadedFromDirectory(
    ExtensionList* extensions) {
  extensions_.assign(extensions->begin(), extensions->end());
  delete extensions;

  // TODO(aa): Notify extensions are loaded.
}

void ExtensionsService::OnExtensionLoadError(const std::wstring& error) {
  // TODO(aa): Print the error message out somewhere better. Ideally we would
  // use the JavaScript console I think, but that is complicated since these
  // errors are not related to any particular page.
  LOG(WARNING) << "Error loading extension: " << error;
}


// ExtensionsServicesBackend

bool ExtensionsServiceBackend::LoadExtensionsFromDirectory(
    const FilePath& path,
    scoped_refptr<ExtensionsServiceFrontendInterface> frontend) {
  // Find all child directories in the install directory and load their
  // manifests. Post errors and results to the frontend.
  scoped_ptr<ExtensionList> extensions(new ExtensionList);
  file_util::FileEnumerator enumerator(path.ToWStringHack(),
                                       false, // not recursive
                                       file_util::FileEnumerator::DIRECTORIES);
  for (std::wstring child_path = enumerator.Next(); !child_path.empty();
       child_path = enumerator.Next()) {
     FilePath manifest_path = FilePath::FromWStringHack(child_path).Append(
         Extension::kManifestFilename);
    if (!file_util::PathExists(manifest_path)) {
      ReportExtensionLoadError(frontend.get(), 
                               Extension::kInvalidManifestError);
      continue;
    }

    JSONFileValueSerializer serializer(manifest_path.ToWStringHack());
    Value* root = NULL;
    if (!serializer.Deserialize(&root, NULL)) {
      ReportExtensionLoadError(frontend.get(), 
                               Extension::kInvalidManifestError);
      continue;
    }

    if (!root->IsType(Value::TYPE_DICTIONARY)) {
      ReportExtensionLoadError(frontend.get(), 
                               Extension::kInvalidManifestError);
      continue;
    }

    scoped_ptr<Extension> extension(new Extension());
    std::wstring error;
    if (!extension->InitFromValue(*static_cast<DictionaryValue*>(root),
                                  &error)) {
      ReportExtensionLoadError(frontend.get(), 
                               Extension::kInvalidManifestError);
      continue;
    }

    extensions->push_back(extension.release());
  }

  ReportExtensionsLoaded(frontend.get(), extensions.release());
  return true;
}

void ExtensionsServiceBackend::ReportExtensionLoadError(
    ExtensionsServiceFrontendInterface *frontend, const std::wstring &error) {
  frontend->GetMessageLoop()->PostTask(FROM_HERE, NewRunnableMethod(
      frontend, &ExtensionsServiceFrontendInterface::OnExtensionLoadError,
      error));
}

void ExtensionsServiceBackend::ReportExtensionsLoaded(
    ExtensionsServiceFrontendInterface *frontend, ExtensionList* extensions) {
  frontend->GetMessageLoop()->PostTask(FROM_HERE, NewRunnableMethod(
      frontend,
      &ExtensionsServiceFrontendInterface::OnExtensionsLoadedFromDirectory,
      extensions));
}
