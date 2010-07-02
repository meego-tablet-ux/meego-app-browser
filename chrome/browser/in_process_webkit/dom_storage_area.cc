// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/in_process_webkit/dom_storage_area.h"

#include "base/file_path.h"
#include "base/file_util.h"
#include "base/task.h"
#include "chrome/browser/chrome_thread.h"
#include "chrome/browser/in_process_webkit/dom_storage_context.h"
#include "chrome/browser/in_process_webkit/dom_storage_dispatcher_host.h"
#include "chrome/browser/in_process_webkit/dom_storage_namespace.h"
#include "chrome/browser/in_process_webkit/dom_storage_permission_request.h"
#include "chrome/browser/host_content_settings_map.h"
#include "chrome/common/render_messages.h"
#include "third_party/WebKit/WebKit/chromium/public/WebSecurityOrigin.h"
#include "third_party/WebKit/WebKit/chromium/public/WebStorageArea.h"
#include "third_party/WebKit/WebKit/chromium/public/WebString.h"
#include "third_party/WebKit/WebKit/chromium/public/WebURL.h"
#include "webkit/glue/webkit_glue.h"

using WebKit::WebSecurityOrigin;
using WebKit::WebStorageArea;
using WebKit::WebString;
using WebKit::WebURL;

DOMStorageArea::DOMStorageArea(
    const string16& origin,
    int64 id,
    DOMStorageNamespace* owner,
    HostContentSettingsMap* host_content_settings_map)
    : origin_(origin),
      origin_url_(origin),
      id_(id),
      owner_(owner),
      host_content_settings_map_(host_content_settings_map) {
  DCHECK(owner_);
  DCHECK(host_content_settings_map_);
}

DOMStorageArea::~DOMStorageArea() {
}

unsigned DOMStorageArea::Length() {
  CreateWebStorageAreaIfNecessary();
  return storage_area_->length();
}

NullableString16 DOMStorageArea::Key(unsigned index) {
  CreateWebStorageAreaIfNecessary();
  return storage_area_->key(index);
}

NullableString16 DOMStorageArea::GetItem(const string16& key) {
  CreateWebStorageAreaIfNecessary();
  return storage_area_->getItem(key);
}

NullableString16 DOMStorageArea::SetItem(
    const string16& key, const string16& value,
    WebStorageArea::Result* result, DOMStorageDispatcherHost* sender) {
  if (!CheckContentSetting(key, value, sender)) {
    *result = WebStorageArea::ResultBlockedByPolicy;
    return NullableString16(true);  // Ignored if the content was blocked.
  }

  CreateWebStorageAreaIfNecessary();
  WebString old_value;
  storage_area_->setItem(key, value, WebURL(), *result, old_value);
  return old_value;
}

NullableString16 DOMStorageArea::RemoveItem(const string16& key) {
  CreateWebStorageAreaIfNecessary();
  WebString old_value;
  storage_area_->removeItem(key, WebURL(), old_value);
  return old_value;
}

bool DOMStorageArea::Clear() {
  CreateWebStorageAreaIfNecessary();
  bool somethingCleared;
  storage_area_->clear(WebURL(), somethingCleared);
  return somethingCleared;
}

void DOMStorageArea::PurgeMemory() {
  storage_area_.reset();
}

void DOMStorageArea::CreateWebStorageAreaIfNecessary() {
  if (!storage_area_.get())
    storage_area_.reset(owner_->CreateWebStorageArea(origin_));
}

bool DOMStorageArea::CheckContentSetting(
    const string16& key, const string16& value,
    DOMStorageDispatcherHost* sender) {
  ContentSetting content_setting =
      host_content_settings_map_->GetContentSetting(
          origin_url_, CONTENT_SETTINGS_TYPE_COOKIES);

  if (content_setting == CONTENT_SETTING_ASK) {
    DOMStoragePermissionRequest request(origin_url_, key, value,
                                        host_content_settings_map_);
    ChromeThread::PostTask(
        ChromeThread::UI, FROM_HERE,
        NewRunnableFunction(&DOMStoragePermissionRequest::PromptUser,
                            &request));
    // Tell the renderer that it needs to run a nested message loop.
    sender->Send(new ViewMsg_SignalCookiePromptEvent());
    content_setting = request.WaitOnResponse();
  }

  return (content_setting != CONTENT_SETTING_BLOCK);
}
