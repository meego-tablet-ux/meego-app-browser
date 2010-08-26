// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/user_script_listener.h"

#include "chrome/browser/chrome_thread.h"
#include "chrome/browser/extensions/extensions_service.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/renderer_host/global_request_id.h"
#include "chrome/browser/renderer_host/resource_dispatcher_host_request_info.h"
#include "chrome/common/extensions/extension.h"
#include "chrome/common/extensions/url_pattern.h"
#include "chrome/common/notification_service.h"
#include "net/url_request/url_request.h"

UserScriptListener::UserScriptListener(ResourceQueue* resource_queue)
    : resource_queue_(resource_queue),
      user_scripts_ready_(false) {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::UI));
  DCHECK(resource_queue_);

  registrar_.Add(this, NotificationType::EXTENSION_LOADED,
                 NotificationService::AllSources());
  registrar_.Add(this, NotificationType::EXTENSION_UNLOADED,
                 NotificationService::AllSources());
  registrar_.Add(this, NotificationType::USER_SCRIPTS_UPDATED,
                 NotificationService::AllSources());
}

void UserScriptListener::ShutdownMainThread() {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::UI));
  registrar_.RemoveAll();
}

bool UserScriptListener::ShouldDelayRequest(
    URLRequest* request,
    const ResourceDispatcherHostRequestInfo& request_info,
    const GlobalRequestID& request_id) {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::IO));

  // If it's a frame load, then we need to check the URL against the list of
  // user scripts to see if we need to wait.
  if (request_info.resource_type() != ResourceType::MAIN_FRAME &&
      request_info.resource_type() != ResourceType::SUB_FRAME) {
    return false;
  }

  if (user_scripts_ready_)
    return false;

  for (URLPatterns::iterator it = url_patterns_.begin();
       it != url_patterns_.end(); ++it) {
    if ((*it).MatchesUrl(request->url())) {
      // One of the user scripts wants to inject into this request, but the
      // script isn't ready yet. Delay the request.
      delayed_request_ids_.push_front(request_id);
      return true;
    }
  }

  return false;
}

void UserScriptListener::WillShutdownResourceQueue() {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::IO));
  resource_queue_ = NULL;
}

UserScriptListener::~UserScriptListener() {
}

void UserScriptListener::StartDelayedRequests() {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::IO));

  user_scripts_ready_ = true;

  if (resource_queue_) {
    for (DelayedRequests::iterator it = delayed_request_ids_.begin();
         it != delayed_request_ids_.end(); ++it) {
      resource_queue_->StartDelayedRequest(this, *it);
    }
  }

  delayed_request_ids_.clear();
}

void UserScriptListener::AppendNewURLPatterns(const URLPatterns& new_patterns) {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::IO));

  user_scripts_ready_ = false;
  url_patterns_.insert(url_patterns_.end(),
                       new_patterns.begin(), new_patterns.end());
}

void UserScriptListener::ReplaceURLPatterns(const URLPatterns& patterns) {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::IO));
  url_patterns_ = patterns;
}

void UserScriptListener::CollectURLPatterns(Extension* extension,
                                            URLPatterns* patterns) {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::UI));

  const UserScriptList& scripts = extension->content_scripts();
  for (UserScriptList::const_iterator iter = scripts.begin();
       iter != scripts.end(); ++iter) {
    patterns->insert(patterns->end(),
                     (*iter).url_patterns().begin(),
                     (*iter).url_patterns().end());
  }
}

void UserScriptListener::Observe(NotificationType type,
                                 const NotificationSource& source,
                                 const NotificationDetails& details) {
  DCHECK(ChromeThread::CurrentlyOn(ChromeThread::UI));

  switch (type.value) {
    case NotificationType::EXTENSION_LOADED: {
      Extension* extension = Details<Extension>(details).ptr();
      if (extension->content_scripts().empty())
        return;  // no new patterns from this extension.

      URLPatterns new_patterns;
      CollectURLPatterns(Details<Extension>(details).ptr(), &new_patterns);
      if (!new_patterns.empty()) {
        ChromeThread::PostTask(
            ChromeThread::IO, FROM_HERE,
            NewRunnableMethod(
                this, &UserScriptListener::AppendNewURLPatterns, new_patterns));
      }
      break;
    }

    case NotificationType::EXTENSION_UNLOADED: {
      Extension* unloaded_extension = Details<Extension>(details).ptr();
      if (unloaded_extension->content_scripts().empty())
        return;  // no patterns to delete for this extension.

      // Clear all our patterns and reregister all the still-loaded extensions.
      URLPatterns new_patterns;
      ExtensionsService* service =
          Source<Profile>(source).ptr()->GetExtensionsService();
      for (ExtensionList::const_iterator it = service->extensions()->begin();
           it != service->extensions()->end(); ++it) {
        if (*it != unloaded_extension)
          CollectURLPatterns(*it, &new_patterns);
      }
      ChromeThread::PostTask(
          ChromeThread::IO, FROM_HERE,
          NewRunnableMethod(
              this, &UserScriptListener::ReplaceURLPatterns, new_patterns));
      break;
    }

    case NotificationType::USER_SCRIPTS_UPDATED: {
      ChromeThread::PostTask(
          ChromeThread::IO, FROM_HERE,
          NewRunnableMethod(this, &UserScriptListener::StartDelayedRequests));
      break;
    }

    default:
      NOTREACHED();
  }
}
