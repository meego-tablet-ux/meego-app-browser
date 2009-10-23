// Copyright (c) 2006-2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/automation/automation_profile_impl.h"
#include "chrome/browser/net/chrome_url_request_context.h"
#include "chrome/test/automation/automation_messages.h"

namespace {

// A special Request context for automation. Substitute a few things
// like cookie store, proxy settings etc to handle them differently
// for automation.
class AutomationURLRequestContext : public ChromeURLRequestContext {
 public:
  AutomationURLRequestContext(ChromeURLRequestContext* original_context,
                              net::CookieStore* automation_cookie_store)
      : ChromeURLRequestContext(original_context),
        // We must hold a reference to |original_context|, since many
        // of the dependencies that ChromeURLRequestContext(original_context)
        // copied are scoped to |original_context|.
        original_context_(original_context) {
    cookie_store_ = automation_cookie_store;
  }

  virtual ~AutomationURLRequestContext() {
    // Clear out members before calling base class dtor since we don't
    // own any of them.

    // Clear URLRequestContext members.
    host_resolver_ = NULL;
    proxy_service_ = NULL;
    http_transaction_factory_ = NULL;
    ftp_transaction_factory_ = NULL;
    cookie_store_ = NULL;
    strict_transport_security_state_ = NULL;

    // Clear ChromeURLRequestContext members.
    blacklist_ = NULL;
  }

 private:
  scoped_refptr<ChromeURLRequestContext> original_context_;
  DISALLOW_COPY_AND_ASSIGN(AutomationURLRequestContext);
};

// CookieStore specialization to have automation specific
// behavior for cookies.
class AutomationCookieStore : public net::CookieStore {
 public:
  AutomationCookieStore(AutomationProfileImpl* profile,
                        net::CookieStore* original_cookie_store,
                        IPC::Message::Sender* automation_client)
      : profile_(profile),
        original_cookie_store_(original_cookie_store),
        automation_client_(automation_client) {
  }

  // CookieStore implementation.
  virtual bool SetCookie(const GURL& url, const std::string& cookie_line) {
    bool cookie_set = original_cookie_store_->SetCookie(url, cookie_line);
    if (cookie_set) {
      // TODO(eroman): Should NOT be accessing the profile from here, as this
      // is running on the IO thread.
      automation_client_->Send(new AutomationMsg_SetCookieAsync(0,
          profile_->tab_handle(), url, cookie_line));
    }
    return cookie_set;
  }
  virtual bool SetCookieWithOptions(const GURL& url,
                                    const std::string& cookie_line,
                                    const net::CookieOptions& options) {
    return original_cookie_store_->SetCookieWithOptions(url, cookie_line,
                                                       options);
  }
  virtual bool SetCookieWithCreationTime(const GURL& url,
                                         const std::string& cookie_line,
                                         const base::Time& creation_time) {
    return original_cookie_store_->SetCookieWithCreationTime(url, cookie_line,
                                                            creation_time);
  }
  virtual bool SetCookieWithCreationTimeWithOptions(
                                         const GURL& url,
                                         const std::string& cookie_line,
                                         const base::Time& creation_time,
                                         const net::CookieOptions& options) {
    return original_cookie_store_->SetCookieWithCreationTimeWithOptions(url,
        cookie_line, creation_time, options);
  }
  virtual void SetCookies(const GURL& url,
                          const std::vector<std::string>& cookies) {
    original_cookie_store_->SetCookies(url, cookies);
  }
  virtual void SetCookiesWithOptions(const GURL& url,
                                     const std::vector<std::string>& cookies,
                                     const net::CookieOptions& options) {
    original_cookie_store_->SetCookiesWithOptions(url, cookies, options);
  }
  virtual std::string GetCookies(const GURL& url) {
    return original_cookie_store_->GetCookies(url);
  }
  virtual std::string GetCookiesWithOptions(const GURL& url,
                                            const net::CookieOptions& options) {
    return original_cookie_store_->GetCookiesWithOptions(url, options);
  }

 protected:
  AutomationProfileImpl* profile_;
  net::CookieStore* original_cookie_store_;
  IPC::Message::Sender* automation_client_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AutomationCookieStore);
};

class Factory : public ChromeURLRequestContextFactory {
 public:
  Factory(ChromeURLRequestContextGetter* original_context_getter,
          AutomationProfileImpl* profile,
          IPC::Message::Sender* automation_client)
      : ChromeURLRequestContextFactory(profile),
        original_context_getter_(original_context_getter),
        profile_(profile),
        automation_client_(automation_client) {
  }

  virtual ChromeURLRequestContext* Create() {
    ChromeURLRequestContext* original_context =
        original_context_getter_->GetIOContext();

    // Create an automation cookie store.
    scoped_refptr<net::CookieStore> automation_cookie_store =
        new AutomationCookieStore(profile_,
                                  original_context->cookie_store(),
                                  automation_client_);

    return new AutomationURLRequestContext(original_context,
                                           automation_cookie_store);
  }

 private:
  scoped_refptr<ChromeURLRequestContextGetter> original_context_getter_;
  AutomationProfileImpl* profile_;
  IPC::Message::Sender* automation_client_;
};

// TODO(eroman): This duplicates CleanupRequestContext() from profile.cc.
void CleanupRequestContext(ChromeURLRequestContextGetter* context) {
  context->CleanupOnUIThread();

  // Clean up request context on IO thread.
  g_browser_process->io_thread()->message_loop()->ReleaseSoon(FROM_HERE,
                                                              context);
}

}  // namespace

AutomationProfileImpl::~AutomationProfileImpl() {
  CleanupRequestContext(alternate_request_context_);
}

void AutomationProfileImpl::Initialize(Profile* original_profile,
    IPC::Message::Sender* automation_client) {
  DCHECK(original_profile);
  original_profile_ = original_profile;

  ChromeURLRequestContextGetter* original_context =
      static_cast<ChromeURLRequestContextGetter*>(
          original_profile_->GetRequestContext());
  alternate_request_context_ = new ChromeURLRequestContextGetter(
      NULL, // Don't register an observer on PrefService.
      new Factory(original_context, this, automation_client));
  alternate_request_context_->AddRef();  // Balananced in the destructor.
}

