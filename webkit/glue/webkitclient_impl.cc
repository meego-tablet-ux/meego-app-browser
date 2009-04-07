// Copyright (c) 2009 The Chromium Authors. All rights reserved.  Use of this
// source code is governed by a BSD-style license that can be found in the
// LICENSE file.

#include "webkit/glue/webkitclient_impl.h"

#include "base/message_loop.h"
#include "base/stats_counters.h"
#include "base/trace_event.h"
#include "grit/webkit_resources.h"
#include "third_party/WebKit/WebKit/chromium/public/WebData.h"
#include "webkit/glue/webkit_glue.h"

using WebKit::WebData;
using WebKit::WebThemeEngine;

namespace webkit_glue {

WebKitClientImpl::WebKitClientImpl()
    : main_loop_(MessageLoop::current()),
      shared_timer_func_(NULL) {
}

WebThemeEngine* WebKitClientImpl::themeEngine() {
#if defined(OS_WIN)
  return &theme_engine_;
#else
  return NULL;
#endif
}

void WebKitClientImpl::decrementStatsCounter(const char* name) {
  StatsCounter(name).Decrement();
}

void WebKitClientImpl::incrementStatsCounter(const char* name) {
  StatsCounter(name).Increment();
}

void WebKitClientImpl::traceEventBegin(const char* name, void* id,
                                       const char* extra) {
  TRACE_EVENT_BEGIN(name, id, extra);
}

void WebKitClientImpl::traceEventEnd(const char* name, void* id,
                                     const char* extra) {
  TRACE_EVENT_END(name, id, extra);
}

WebData WebKitClientImpl::loadResource(const char* name) {
  struct {
    const char* name;
    int id;
  } resources[] = {
    { "textAreaResizeCorner", IDR_TEXTAREA_RESIZER },
    { "missingImage", IDR_BROKENIMAGE },
    { "tickmarkDash", IDR_TICKMARK_DASH },
    { "panIcon", IDR_PAN_SCROLL_ICON },
    { "searchCancel", IDR_SEARCH_CANCEL },
    { "searchCancelPressed", IDR_SEARCH_CANCEL_PRESSED },
    { "searchMagnifier", IDR_SEARCH_MAGNIFIER },
    { "searchMagnifierResults", IDR_SEARCH_MAGNIFIER_RESULTS },
#if defined(OS_LINUX)
    { "linuxCheckboxOff", IDR_LINUX_CHECKBOX_OFF },
    { "linuxCheckboxOn", IDR_LINUX_CHECKBOX_ON },
    { "linuxRadioOff", IDR_LINUX_RADIO_OFF },
    { "linuxRadioOn", IDR_LINUX_RADIO_ON },
#endif
  };
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(resources); ++i) {
    if (!strcmp(name, resources[i].name)) {
      StringPiece resource = webkit_glue::GetDataResource(resources[i].id);
      return WebData(resource.data(), resource.size());
    }
  }
  NOTREACHED() << "Unknown image resource " << name;
  return WebData();
}

double WebKitClientImpl::currentTime() {
  return base::Time::Now().ToDoubleT();
}

void WebKitClientImpl::setSharedTimerFiredFunction(void (*func)()) {
  shared_timer_func_ = func;
}

void WebKitClientImpl::setSharedTimerFireTime(double fire_time) {
  int interval = static_cast<int>((fire_time - currentTime()) * 1000);
  if (interval < 0)
    interval = 0;

  shared_timer_.Stop();
  shared_timer_.Start(base::TimeDelta::FromMilliseconds(interval), this,
                      &WebKitClientImpl::DoTimeout);
}

void WebKitClientImpl::stopSharedTimer() {
  shared_timer_.Stop();
}

void WebKitClientImpl::callOnMainThread(void (*func)()) {
  main_loop_->PostTask(FROM_HERE, NewRunnableFunction(func));
}

}  // namespace webkit_glue
