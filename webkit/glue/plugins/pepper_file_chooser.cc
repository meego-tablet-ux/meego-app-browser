// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/glue/plugins/pepper_file_chooser.h"

#include "base/logging.h"
#include "third_party/ppapi/c/pp_completion_callback.h"
#include "third_party/ppapi/c/pp_errors.h"
#include "webkit/glue/plugins/pepper_file_ref.h"
#include "webkit/glue/plugins/pepper_plugin_instance.h"
#include "webkit/glue/plugins/pepper_resource_tracker.h"

namespace pepper {

namespace {

PP_Resource Create(PP_Instance instance_id,
                   const PP_FileChooserOptions* options) {
  PluginInstance* instance = PluginInstance::FromPPInstance(instance_id);
  if (!instance)
    return 0;

  FileChooser* chooser = new FileChooser(instance, options);
  chooser->AddRef();  // AddRef for the caller.
  return chooser->GetResource();
}

bool IsFileChooser(PP_Resource resource) {
  return !!ResourceTracker::Get()->GetAsFileChooser(resource).get();
}

int32_t Show(PP_Resource chooser_id, PP_CompletionCallback callback) {
  scoped_refptr<FileChooser> chooser(
      ResourceTracker::Get()->GetAsFileChooser(chooser_id).get());
  if (!chooser.get())
    return PP_Error_BadResource;

  return chooser->Show(callback);
}

PP_Resource GetNextChosenFile(PP_Resource chooser_id) {
  scoped_refptr<FileChooser> chooser(
      ResourceTracker::Get()->GetAsFileChooser(chooser_id).get());
  if (!chooser.get())
    return 0;

  scoped_refptr<FileRef> file_ref(chooser->GetNextChosenFile());
  if (!file_ref.get())
    return 0;
  file_ref->AddRef();  // AddRef for the caller.

  return file_ref->GetResource();
}

const PPB_FileChooser ppb_filechooser = {
  &Create,
  &IsFileChooser,
  &Show,
  &GetNextChosenFile
};

}  // namespace

FileChooser::FileChooser(PluginInstance* instance,
                         const PP_FileChooserOptions* options)
    : Resource(instance->module()),
      mode_(options->mode),
      accept_mime_types_(options->accept_mime_types) {
}

FileChooser::~FileChooser() {
}

// static
const PPB_FileChooser* FileChooser::GetInterface() {
  return &ppb_filechooser;
}

int32_t FileChooser::Show(PP_CompletionCallback callback) {
  NOTIMPLEMENTED();  // TODO(darin): Implement me!
  return PP_Error_Failed;
}

scoped_refptr<FileRef> FileChooser::GetNextChosenFile() {
  NOTIMPLEMENTED();  // TODO(darin): Implement me!
  return NULL;
}

}  // namespace pepper
