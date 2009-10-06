// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_function.h"

#include "base/json_writer.h"
#include "base/logging.h"
#include "chrome/browser/extensions/extension_function_dispatcher.h"

void AsyncExtensionFunction::SetArgs(const Value* args) {
  DCHECK(!args_);  // Should only be called once.
  args_ = args->DeepCopy();
}

const std::string AsyncExtensionFunction::GetResult() {
  std::string json;
  // Some functions might not need to return any results.
  if (result_.get())
    JSONWriter::Write(result_.get(), false, &json);
  return json;
}

void AsyncExtensionFunction::SendResponse(bool success) {
  if (!dispatcher())
    return;
  if (bad_message_) {
    dispatcher()->HandleBadMessage(this);
  } else {
    dispatcher()->SendResponse(this, success);
  }
}

std::string AsyncExtensionFunction::extension_id() {
  DCHECK(dispatcher());
  return dispatcher()->extension_id();
}

Profile* AsyncExtensionFunction::profile() {
  DCHECK(dispatcher());
  return dispatcher()->profile();
}
