// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_FRAME_URLMON_MONIKER_H_
#define CHROME_FRAME_URLMON_MONIKER_H_

#include <atlbase.h>
#include <atlcom.h>
#include <urlmon.h>
#include <string>

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/scoped_comptr_win.h"
#include "base/thread_local.h"
#include "googleurl/src/gurl.h"
#include "chrome_frame/utils.h"

// This file contains classes that are used to cache the contents of a top-level
// http request (not for sub frames) while that request is parsed for the
// presence of a meta tag indicating that the page should be rendered in CF.

// Here are a few scenarios we handle and how the classes come to play.

//
// Scenario 1: Non CF url navigation through address bar (www.msn.com)
// - Bho::BeforeNavigate - top level url = www.msn.com
// - MSHTML -> MonikerPatch::BindToStorage.
//   (IEFrame starts this by calling mshtml!*SuperNavigate*)
//    - check if the url is a top level url
//    - iff the url is a top level url, we switch in our own callback object
//      and hook it up to the bind context (BSCBStorageBind)
//    - otherwise just call the original
// - BSCBStorageBind::OnDataAvailable - sniffs data and determines that the
//   renderer is not chrome. Goes into pass through mode.
// -  The page loads in mshtml.
//

//
// Scenario 2: CF navigation through address bar URL
// - Bho::BeforeNavigate - top level url = http://wave.google.com/
// - MSHTML -> MonikerPatch::BindToStorage.
//   (IEFrame starts this by calling mshtml!*SuperNavigate*)
//    - request_data is NULL
//    - check if the url is a top level url
//    - iff the url is a top level url, we switch in our own callback object
//      and hook it up to the bind context (BSCBStorageBind)
// - BSCBStorageBind::OnDataAvailable - sniffs data and determines that the
//   renderer is chrome. It then registers a special bind context param and
//   sets a magic clip format in the format_etc. Then goes into pass through
//   mode.
// - mshtml looks at the clip format and re-issues the navigation with the
//   same bind context. Also returns INET_E_TERMINATED_BIND so that same
//   underlying transaction objects are used.
// - IEFrame -> MonikerPatch::BindToStorage
//    - We check for the special bind context param and instantiate and
//      return our ActiveDoc

//
// Scenario 3: CF navigation through mshtml link
//  Same as scenario #2.
//

//
// Scenario 4: CF navigation through link click in chrome loads non CF page
// - Link click comes to ChromeActiveDocument::OnOpenURL
//    - web_browser->Navigate with URL
// - [Scenario 1]
//

//
// Scenario 5: CF navigation through link click in chrome loads CF page
// - Link click comes to ChromeActiveDocument::OnOpenURL
//    - web_browser->Navigate with URL
// - [Scenario 2]
//

// This class is the link between a few static, moniker related functions to
// the bho.  The specific services needed by those functions are abstracted into
// this interface for easier testability.
class NavigationManager {
 public:
  NavigationManager() {
  }

  // Returns the Bho instance for the current thread. This is returned from
  // TLS.  Returns NULL if no instance exists on the current thread.
  static NavigationManager* GetThreadInstance();

  // Mark a bind context for navigation by storing a bind context param.
  static bool SetForSwitch(IBindCtx* bind_context);
  static bool ResetSwitch(IBindCtx* bind_context);

  void RegisterThreadInstance();
  void UnregisterThreadInstance();

  virtual ~NavigationManager() {
    DCHECK(GetThreadInstance() != this);
  }

  // Returns the url of the current top level navigation.
  const std::wstring& url() const {
    return url_;
  }

  // Called to set the current top level URL that's being navigated to.
  void set_url(const wchar_t* url) {
    DLOG(INFO) << __FUNCTION__ << " " << url;
    url_ = url;
  }

  const std::wstring& original_url_with_fragment() const {
    return original_url_with_fragment_;
  }

  void set_original_url_with_fragment(const wchar_t* url) {
    DLOG(INFO) << __FUNCTION__ << " " << url;
    original_url_with_fragment_ = url;
  }

  // Returns the referrer header value of the current top level navigation.
  const std::string& referrer() const {
    return referrer_;
  }

  void set_referrer(const std::string& referrer) {
    referrer_ = referrer;
  }

  // Return true if this is a URL that represents a top-level
  // document that might have to be rendered in CF.
  virtual bool IsTopLevelUrl(const wchar_t* url) {
    return GURL(url_) == GURL(url);
  }

  // Called from HttpNegotiatePatch::BeginningTransaction when a request is
  // being issued.  We check the url and headers and see if there is a referrer
  // header that we need to cache.
  virtual void OnBeginningTransaction(bool is_top_level, const wchar_t* url,
                                      const wchar_t* headers,
                                      const wchar_t* additional_headers);

  // Called when we've detected the http-equiv meta tag in the current page
  // and need to switch over from mshtml to CF.
  virtual HRESULT NavigateToCurrentUrlInCF(IBrowserService* browser);

 protected:
  std::string referrer_;
  std::wstring url_;

  static base::LazyInstance<base::ThreadLocalPointer<NavigationManager> >
      thread_singleton_;

  // If the url being navigated to within ChromeFrame has a fragment, this
  // member contains this URL. This member is cleared when the Chrome active
  // document is loaded.
  std::wstring original_url_with_fragment_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NavigationManager);
};

// static-only class that manages an IMoniker patch.
// We need this patch to stay in the loop when top-level HTML content is
// downloaded that might have the CF http-equiv meta tag.
// When we detect candidates for those requests, we add our own callback
// object (as explained at the top of this file) and use it to cache the
// original document contents in order to avoid multiple network trips
// if we need to switch the renderer over to CF.
class MonikerPatch {
  MonikerPatch() {} // no instances should be created of this class.
 public:
  // Patches two IMoniker methods, BindToObject and BindToStorage.
  static bool Initialize();

  // Nullifies the IMoniker patches.
  static void Uninitialize();

  // Typedefs for IMoniker methods.
  typedef HRESULT (STDMETHODCALLTYPE* IMoniker_BindToObject_Fn)(IMoniker* me,
      IBindCtx* bind_ctx, IMoniker* to_left, REFIID iid, void** obj);
  typedef HRESULT (STDMETHODCALLTYPE* IMoniker_BindToStorage_Fn)(IMoniker* me,
      IBindCtx* bind_ctx, IMoniker* to_left, REFIID iid, void** obj);

  static STDMETHODIMP BindToObject(IMoniker_BindToObject_Fn original,
                                   IMoniker* me, IBindCtx* bind_ctx,
                                   IMoniker* to_left, REFIID iid, void** obj);

  static STDMETHODIMP BindToStorage(IMoniker_BindToStorage_Fn original,
                                    IMoniker* me, IBindCtx* bind_ctx,
                                    IMoniker* to_left, REFIID iid, void** obj);

};

#endif  // CHROME_FRAME_URLMON_MONIKER_H_
