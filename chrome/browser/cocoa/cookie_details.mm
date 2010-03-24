// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/cocoa/cookie_details.h"

#include "app/l10n_util_mac.h"
#import "base/i18n/time_formatting.h"
#include "base/sys_string_conversions.h"
#include "grit/generated_resources.h"
#include "chrome/browser/cookie_modal_dialog.h"
#include "chrome/browser/cookies_tree_model.h"

@implementation CocoaCookieDetails

@synthesize type = type_;

- (BOOL)shouldHideCookieDetailsView {
  return type_ != kCocoaCookieDetailsTypeFolder &&
      type_ != kCocoaCookieDetailsTypeCookie;
}

- (BOOL)shouldShowLocalStorageTreeDetailsView {
  return type_ == kCocoaCookieDetailsTypeTreeLocalStorage;
}

- (BOOL)shouldShowDatabaseTreeDetailsView {
  return type_ == kCocoaCookieDetailsTypeTreeDatabase;
}

- (BOOL)shouldShowDatabasePromptDetailsView {
  return type_ == kCocoaCookieDetailsTypePromptDatabase;
}

- (BOOL)shouldShowLocalStoragePromptDetailsView {
  return type_ == kCocoaCookieDetailsTypePromptLocalStorage;
}

- (NSString*)name {
  return name_.get();
}

- (NSString*)content {
  return content_.get();
}

- (NSString*)domain {
  return domain_.get();
}

- (NSString*)path {
  return path_.get();
}

- (NSString*)sendFor {
  return sendFor_.get();
}

- (NSString*)created {
  return created_.get();
}

- (NSString*)expires {
  return expires_.get();
}

- (NSString*)fileSize {
  return fileSize_.get();
}

- (NSString*)lastModified {
  return lastModified_.get();
}

- (NSString*)databaseDescription {
  return databaseDescription_.get();
}

- (NSString*)localStorageKey {
  return localStorageKey_.get();
}

- (NSString*)localStorageValue {
  return localStorageValue_.get();
}

- (id)initAsFolder {
  if ((self = [super init])) {
    type_ = kCocoaCookieDetailsTypeFolder;
  }
  return self;
}

- (id)initWithCookie:(const net::CookieMonster::CanonicalCookie*)cookie
              origin:(NSString*)origin {
  if ((self = [super init])) {
    type_ = kCocoaCookieDetailsTypeCookie;
    name_.reset([base::SysUTF8ToNSString(cookie->Name()) retain]);
    content_.reset([base::SysUTF8ToNSString(cookie->Value()) retain]);
    path_.reset([base::SysUTF8ToNSString(cookie->Path()) retain]);
    domain_.reset([origin retain]);

    if (cookie->DoesExpire()) {
      expires_.reset([base::SysWideToNSString(
          base::TimeFormatFriendlyDateAndTime(cookie->ExpiryDate())) retain]);
    } else {
      expires_.reset([l10n_util::GetNSStringWithFixup(
          IDS_COOKIES_COOKIE_EXPIRES_SESSION) retain]);
    }

    created_.reset([base::SysWideToNSString(
        base::TimeFormatFriendlyDateAndTime(cookie->CreationDate())) retain]);

    if (cookie->IsSecure()) {
      sendFor_.reset([l10n_util::GetNSStringWithFixup(
          IDS_COOKIES_COOKIE_SENDFOR_SECURE) retain]);
    } else {
      sendFor_.reset([l10n_util::GetNSStringWithFixup(
          IDS_COOKIES_COOKIE_SENDFOR_ANY) retain]);
    }
  }
  return self;
}

- (id)initWithDatabase:(const BrowsingDataDatabaseHelper::DatabaseInfo*)
    databaseInfo {
  if ((self = [super init])) {
    type_ = kCocoaCookieDetailsTypeTreeDatabase;
    databaseDescription_.reset([base::SysUTF8ToNSString(
        databaseInfo->description) retain]);
    fileSize_.reset([base::SysWideToNSString(FormatBytes(databaseInfo->size,
        GetByteDisplayUnits(databaseInfo->size), true)) retain]);
    lastModified_.reset([base::SysWideToNSString(
        base::TimeFormatFriendlyDateAndTime(
            databaseInfo->last_modified)) retain]);
  }
  return self;
}

- (id)initWithLocalStorage:(
    const BrowsingDataLocalStorageHelper::LocalStorageInfo*)storageInfo {
  if ((self = [super init])) {
    type_ = kCocoaCookieDetailsTypeTreeLocalStorage;
    domain_.reset([base::SysUTF8ToNSString(storageInfo->origin) retain]);
    fileSize_.reset([base::SysWideToNSString(FormatBytes(storageInfo->size,
        GetByteDisplayUnits(storageInfo->size), true)) retain]);
    lastModified_.reset([base::SysWideToNSString(
        base::TimeFormatFriendlyDateAndTime(
            storageInfo->last_modified)) retain]);
  }
  return self;
}

- (id)initWithDatabase:(const std::string&)domain
                  name:(const string16&)name {
  if ((self = [super init])) {
    type_ = kCocoaCookieDetailsTypePromptDatabase;
    name_.reset([base::SysUTF16ToNSString(name) retain]);
    domain_.reset([base::SysUTF8ToNSString(domain) retain]);
  }
  return self;
}

- (id)initWithLocalStorage:(const std::string&)domain
                       key:(const string16&)key
                     value:(const string16&)value {
  if ((self = [super init])) {
    type_ = kCocoaCookieDetailsTypePromptLocalStorage;
    domain_.reset([base::SysUTF8ToNSString(domain) retain]);
    localStorageKey_.reset([base::SysUTF16ToNSString(key) retain]);
    localStorageValue_.reset([base::SysUTF16ToNSString(value) retain]);
  }
  return self;
}

+ (CocoaCookieDetails*)createFromCookieTreeNode:(CookieTreeNode*)treeNode {
  CookieTreeNode::DetailedInfo info = treeNode->GetDetailedInfo();
  CookieTreeNode::DetailedInfo::NodeType nodeType = info.node_type;
  if (nodeType == CookieTreeNode::DetailedInfo::TYPE_COOKIE) {
    NSString* origin = base::SysWideToNSString(info.origin.c_str());
    return [[[CocoaCookieDetails alloc] initWithCookie:&(info.cookie->second)
                                                origin:origin] autorelease];
  } else if (nodeType == CookieTreeNode::DetailedInfo::TYPE_DATABASE) {
    return [[[CocoaCookieDetails alloc]
        initWithDatabase:info.database_info] autorelease];
  } else if (nodeType == CookieTreeNode::DetailedInfo::TYPE_LOCAL_STORAGE) {
    return [[[CocoaCookieDetails alloc]
        initWithLocalStorage:info.local_storage_info] autorelease];
  } else {
    return [[[CocoaCookieDetails alloc] initAsFolder] autorelease];
  }
}

+ (CocoaCookieDetails*)createFromPromptModalDialog:(CookiePromptModalDialog*)
    dialog {
  CookiePromptModalDialog::DialogType type(dialog->dialog_type());
  CocoaCookieDetails* details = nil;
  if (type == CookiePromptModalDialog::DIALOG_TYPE_COOKIE) {
    net::CookieMonster::ParsedCookie pc(dialog->cookie_line());
    net::CookieMonster::CanonicalCookie cookie(dialog->origin(), pc);
    const std::string& domain(pc.HasDomain() ? pc.Domain() :
        dialog->origin().host());
    NSString* domainString = base::SysUTF8ToNSString(domain);
    details = [[CocoaCookieDetails alloc] initWithCookie:&cookie
                                                  origin:domainString];
  } else if (type == CookiePromptModalDialog::DIALOG_TYPE_LOCAL_STORAGE) {
    details = [[CocoaCookieDetails alloc]
        initWithLocalStorage:dialog->origin().host()
                         key:dialog->local_storage_key()
                       value:dialog->local_storage_value()];
  } else if (type == CookiePromptModalDialog::DIALOG_TYPE_DATABASE) {
    details = [[CocoaCookieDetails alloc]
        initWithDatabase:dialog->origin().host()
                    name:dialog->database_name()];
  } else if (type == CookiePromptModalDialog::DIALOG_TYPE_APPCACHE) {
    // TODO(michaeln): Show an appropiate details view, for now we
    // overload the database details view.
    details = [[CocoaCookieDetails alloc]
        initWithDatabase:dialog->origin().host()
                    name:UTF8ToUTF16(dialog->appcache_manifest_url().spec())];
  } else {
    NOTIMPLEMENTED();
  }
  return [details autorelease];
}

@end
