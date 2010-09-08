// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/glue/webkitclient_impl.h"

#if defined(OS_LINUX)
#include <malloc.h>
#endif

#include <math.h>

#include <vector>

#include "base/lock.h"
#include "base/message_loop.h"
#include "base/process_util.h"
#include "base/platform_file.h"
#include "base/singleton.h"
#include "base/stats_counters.h"
#include "base/string_number_conversions.h"
#include "base/string_util.h"
#include "base/time.h"
#include "base/utf_string_conversions.h"
#include "base/trace_event.h"
#include "grit/webkit_resources.h"
#include "grit/webkit_strings.h"
#include "third_party/WebKit/WebKit/chromium/public/WebCookie.h"
#include "third_party/WebKit/WebKit/chromium/public/WebData.h"
#include "third_party/WebKit/WebKit/chromium/public/WebFrameClient.h"
#include "third_party/WebKit/WebKit/chromium/public/WebPluginListBuilder.h"
#include "third_party/WebKit/WebKit/chromium/public/WebScreenInfo.h"
#include "third_party/WebKit/WebKit/chromium/public/WebString.h"
#include "third_party/WebKit/WebKit/chromium/public/WebVector.h"
#include "third_party/WebKit/WebKit/chromium/public/WebURL.h"
#include "webkit/glue/plugins/plugin_instance.h"
#include "webkit/glue/plugins/webplugininfo.h"
#include "webkit/glue/webkit_glue.h"
#include "webkit/glue/websocketstreamhandle_impl.h"
#include "webkit/glue/weburlloader_impl.h"

#if defined(OS_LINUX)
#include "v8/include/v8.h"
#endif

using WebKit::WebCookie;
using WebKit::WebData;
using WebKit::WebLocalizedString;
using WebKit::WebPluginListBuilder;
using WebKit::WebString;
using WebKit::WebSocketStreamHandle;
using WebKit::WebThemeEngine;
using WebKit::WebURL;
using WebKit::WebURLLoader;
using WebKit::WebVector;

namespace {

// A simple class to cache the memory usage for a given amount of time.
class MemoryUsageCache {
 public:
  // Retrieves the Singleton.
  static MemoryUsageCache* Get() {
    return Singleton<MemoryUsageCache>::get();
  }

  MemoryUsageCache() : memory_value_(0) { Init(); }
  ~MemoryUsageCache() {}

  void Init() {
    const unsigned int kCacheSeconds = 1;
    cache_valid_time_ = base::TimeDelta::FromSeconds(kCacheSeconds);
  }

  // Returns true if the cached value is fresh.
  // Returns false if the cached value is stale, or if |cached_value| is NULL.
  bool IsCachedValueValid(size_t* cached_value) {
    AutoLock scoped_lock(lock_);
    if (!cached_value)
      return false;
    if (base::Time::Now() - last_updated_time_ > cache_valid_time_)
      return false;
    *cached_value = memory_value_;
    return true;
  };

  // Setter for |memory_value_|, refreshes |last_updated_time_|.
  void SetMemoryValue(const size_t value) {
    AutoLock scoped_lock(lock_);
    memory_value_ = value;
    last_updated_time_ = base::Time::Now();
  }

 private:
  // The cached memory value.
  size_t memory_value_;

  // How long the cached value should remain valid.
  base::TimeDelta cache_valid_time_;

  // The last time the cached value was updated.
  base::Time last_updated_time_;

  Lock lock_;
};

}  // anonymous namespace

namespace webkit_glue {

static int ToMessageID(WebLocalizedString::Name name) {
  switch (name) {
    case WebLocalizedString::SubmitButtonDefaultLabel:
      return IDS_FORM_SUBMIT_LABEL;
    case WebLocalizedString::InputElementAltText:
      return IDS_FORM_INPUT_ALT;
    case WebLocalizedString::ResetButtonDefaultLabel:
      return IDS_FORM_RESET_LABEL;
    case WebLocalizedString::FileButtonChooseFileLabel:
      return IDS_FORM_FILE_BUTTON_LABEL;
    case WebLocalizedString::FileButtonNoFileSelectedLabel:
      return IDS_FORM_FILE_NO_FILE_LABEL;
    case WebLocalizedString::MultipleFileUploadText:
      return IDS_FORM_FILE_MULTIPLE_UPLOAD;
    case WebLocalizedString::SearchableIndexIntroduction:
      return IDS_SEARCHABLE_INDEX_INTRO;
    case WebLocalizedString::SearchMenuNoRecentSearchesText:
      return IDS_RECENT_SEARCHES_NONE;
    case WebLocalizedString::SearchMenuRecentSearchesText:
      return IDS_RECENT_SEARCHES;
    case WebLocalizedString::SearchMenuClearRecentSearchesText:
      return IDS_RECENT_SEARCHES_CLEAR;
    case WebLocalizedString::AXWebAreaText:
      return IDS_AX_ROLE_WEB_AREA;
    case WebLocalizedString::AXLinkText:
      return IDS_AX_ROLE_LINK;
    case WebLocalizedString::AXListMarkerText:
      return IDS_AX_ROLE_LIST_MARKER;
    case WebLocalizedString::AXImageMapText:
      return IDS_AX_ROLE_IMAGE_MAP;
    case WebLocalizedString::AXHeadingText:
      return IDS_AX_ROLE_HEADING;
    case WebLocalizedString::AXButtonActionVerb:
      return IDS_AX_BUTTON_ACTION_VERB;
    case WebLocalizedString::AXRadioButtonActionVerb:
      return IDS_AX_RADIO_BUTTON_ACTION_VERB;
    case WebLocalizedString::AXTextFieldActionVerb:
      return IDS_AX_TEXT_FIELD_ACTION_VERB;
    case WebLocalizedString::AXCheckedCheckBoxActionVerb:
      return IDS_AX_CHECKED_CHECK_BOX_ACTION_VERB;
    case WebLocalizedString::AXUncheckedCheckBoxActionVerb:
      return IDS_AX_UNCHECKED_CHECK_BOX_ACTION_VERB;
    case WebLocalizedString::AXLinkActionVerb:
      return IDS_AX_LINK_ACTION_VERB;
    case WebLocalizedString::KeygenMenuHighGradeKeySize:
      return IDS_KEYGEN_HIGH_GRADE_KEY;
    case WebLocalizedString::KeygenMenuMediumGradeKeySize:
      return IDS_KEYGEN_MED_GRADE_KEY;
  }
  return -1;
}

WebKitClientImpl::WebKitClientImpl()
    : main_loop_(MessageLoop::current()),
      shared_timer_func_(NULL),
      shared_timer_fire_time_(0.0),
      shared_timer_suspended_(0) {
}

WebKitClientImpl::~WebKitClientImpl() {
}

WebThemeEngine* WebKitClientImpl::themeEngine() {
#if defined(OS_WIN)
  return &theme_engine_;
#else
  return NULL;
#endif
}

WebURLLoader* WebKitClientImpl::createURLLoader() {
  return new WebURLLoaderImpl();
}

WebSocketStreamHandle* WebKitClientImpl::createSocketStreamHandle() {
  return new WebSocketStreamHandleImpl();
}

WebString WebKitClientImpl::userAgent(const WebURL& url) {
  return WebString::fromUTF8(webkit_glue::GetUserAgent(url));
}

void WebKitClientImpl::getPluginList(bool refresh,
                                     WebPluginListBuilder* builder) {
  std::vector<WebPluginInfo> plugins;
  GetPlugins(refresh, &plugins);

  for (size_t i = 0; i < plugins.size(); ++i) {
    const WebPluginInfo& plugin = plugins[i];

    builder->addPlugin(
        plugin.name, plugin.desc,
        FilePathStringToWebString(plugin.path.BaseName().value()));

    for (size_t j = 0; j < plugin.mime_types.size(); ++j) {
      const WebPluginMimeType& mime_type = plugin.mime_types[j];

      builder->addMediaTypeToLastPlugin(
          WebString::fromUTF8(mime_type.mime_type), mime_type.description);

      for (size_t k = 0; k < mime_type.file_extensions.size(); ++k) {
        builder->addFileExtensionToLastMediaType(
            UTF8ToUTF16(mime_type.file_extensions[k]));
      }
    }
  }
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
    { "missingImage", IDR_BROKENIMAGE },
    { "mediaPause", IDR_MEDIA_PAUSE_BUTTON },
    { "mediaPlay", IDR_MEDIA_PLAY_BUTTON },
    { "mediaPlayDisabled", IDR_MEDIA_PLAY_BUTTON_DISABLED },
    { "mediaSoundDisabled", IDR_MEDIA_SOUND_DISABLED },
    { "mediaSoundFull", IDR_MEDIA_SOUND_FULL_BUTTON },
    { "mediaSoundNone", IDR_MEDIA_SOUND_NONE_BUTTON },
    { "mediaSliderThumb", IDR_MEDIA_SLIDER_THUMB },
    { "mediaVolumeSliderThumb", IDR_MEDIA_VOLUME_SLIDER_THUMB },
    { "panIcon", IDR_PAN_SCROLL_ICON },
    { "searchCancel", IDR_SEARCH_CANCEL },
    { "searchCancelPressed", IDR_SEARCH_CANCEL_PRESSED },
    { "searchMagnifier", IDR_SEARCH_MAGNIFIER },
    { "searchMagnifierResults", IDR_SEARCH_MAGNIFIER_RESULTS },
    { "textAreaResizeCorner", IDR_TEXTAREA_RESIZER },
    { "tickmarkDash", IDR_TICKMARK_DASH },
    { "inputSpeech", IDR_INPUT_SPEECH },
    { "inputSpeechRecording", IDR_INPUT_SPEECH_RECORDING },
    { "inputSpeechWaiting", IDR_INPUT_SPEECH_WAITING },
    { "americanExpressCC", IDR_AUTOFILL_CC_AMEX },
    { "dinersCC", IDR_AUTOFILL_CC_DINERS },
    { "discoverCC", IDR_AUTOFILL_CC_DISCOVER },
    { "genericCC", IDR_AUTOFILL_CC_GENERIC },
    { "jcbCC", IDR_AUTOFILL_CC_JCB },
    { "masterCardCC", IDR_AUTOFILL_CC_MASTERCARD },
    { "soloCC", IDR_AUTOFILL_CC_SOLO },
    { "visaCC", IDR_AUTOFILL_CC_VISA },
#if defined(OS_POSIX) && !defined(OS_MACOSX)
    // TODO(port): rename these to "skia" instead of "Linux".
    { "linuxCheckboxDisabledIndeterminate",
        IDR_LINUX_CHECKBOX_DISABLED_INDETERMINATE },
    { "linuxCheckboxDisabledOff", IDR_LINUX_CHECKBOX_DISABLED_OFF },
    { "linuxCheckboxDisabledOn", IDR_LINUX_CHECKBOX_DISABLED_ON },
    { "linuxCheckboxIndeterminate", IDR_LINUX_CHECKBOX_INDETERMINATE },
    { "linuxCheckboxOff", IDR_LINUX_CHECKBOX_OFF },
    { "linuxCheckboxOn", IDR_LINUX_CHECKBOX_ON },
    { "linuxRadioDisabledOff", IDR_LINUX_RADIO_DISABLED_OFF },
    { "linuxRadioDisabledOn", IDR_LINUX_RADIO_DISABLED_ON },
    { "linuxRadioOff", IDR_LINUX_RADIO_OFF },
    { "linuxRadioOn", IDR_LINUX_RADIO_ON },
    { "linuxProgressBar", IDR_PROGRESS_BAR },
    { "linuxProgressBorderLeft", IDR_PROGRESS_BORDER_LEFT },
    { "linuxProgressBorderRight", IDR_PROGRESS_BORDER_RIGHT },
    { "linuxProgressValue", IDR_PROGRESS_VALUE },
#endif
  };
  for (size_t i = 0; i < ARRAYSIZE_UNSAFE(resources); ++i) {
    if (!strcmp(name, resources[i].name)) {
      base::StringPiece resource = GetDataResource(resources[i].id);
      return WebData(resource.data(), resource.size());
    }
  }
  // TODO(jhawkins): Restore this NOTREACHED once WK stops sending in empty
  // strings. http://crbug.com/50675.
  //NOTREACHED() << "Unknown image resource " << name;
  return WebData();
}

WebString WebKitClientImpl::queryLocalizedString(
    WebLocalizedString::Name name) {
  int message_id = ToMessageID(name);
  if (message_id < 0)
    return WebString();
  return GetLocalizedString(message_id);
}

WebString WebKitClientImpl::queryLocalizedString(
    WebLocalizedString::Name name, int numeric_value) {
  int message_id = ToMessageID(name);
  if (message_id < 0)
    return WebString();
  return ReplaceStringPlaceholders(GetLocalizedString(message_id),
                                   base::IntToString16(numeric_value),
                                   NULL);
}

double WebKitClientImpl::currentTime() {
  return base::Time::Now().ToDoubleT();
}

void WebKitClientImpl::setSharedTimerFiredFunction(void (*func)()) {
  shared_timer_func_ = func;
}

void WebKitClientImpl::setSharedTimerFireTime(double fire_time) {
  shared_timer_fire_time_ = fire_time;
  if (shared_timer_suspended_)
    return;

  // By converting between double and int64 representation, we run the risk
  // of losing precision due to rounding errors. Performing computations in
  // microseconds reduces this risk somewhat. But there still is the potential
  // of us computing a fire time for the timer that is shorter than what we
  // need.
  // As the event loop will check event deadlines prior to actually firing
  // them, there is a risk of needlessly rescheduling events and of
  // needlessly looping if sleep times are too short even by small amounts.
  // This results in measurable performance degradation unless we use ceil() to
  // always round up the sleep times.
  int64 interval = static_cast<int64>(
      ceil((fire_time - currentTime()) * base::Time::kMicrosecondsPerSecond));
  if (interval < 0)
    interval = 0;

  shared_timer_.Stop();
  shared_timer_.Start(base::TimeDelta::FromMicroseconds(interval), this,
                      &WebKitClientImpl::DoTimeout);
}

void WebKitClientImpl::stopSharedTimer() {
  shared_timer_.Stop();
}

void WebKitClientImpl::callOnMainThread(void (*func)(void*), void* context) {
  main_loop_->PostTask(FROM_HERE, NewRunnableFunction(func, context));
}

base::PlatformFile WebKitClientImpl::databaseOpenFile(
    const WebKit::WebString& vfs_file_name, int desired_flags) {
  return base::kInvalidPlatformFileValue;
}

int WebKitClientImpl::databaseDeleteFile(
    const WebKit::WebString& vfs_file_name, bool sync_dir) {
  return -1;
}

long WebKitClientImpl::databaseGetFileAttributes(
    const WebKit::WebString& vfs_file_name) {
  return 0;
}

long long WebKitClientImpl::databaseGetFileSize(
    const WebKit::WebString& vfs_file_name) {
  return 0;
}

WebKit::WebString WebKitClientImpl::signedPublicKeyAndChallengeString(
    unsigned key_size_index,
    const WebKit::WebString& challenge,
    const WebKit::WebURL& url) {
  NOTREACHED();
  return WebKit::WebString();
}

#if defined(OS_LINUX)
static size_t memoryUsageMBLinux() {
  struct mallinfo minfo = mallinfo();
  uint64_t mem_usage =
#if defined(USE_TCMALLOC)
      minfo.uordblks
#else
      (minfo.hblkhd + minfo.arena)
#endif
      >> 20;

  v8::HeapStatistics stat;
  v8::V8::GetHeapStatistics(&stat);
  return mem_usage + (static_cast<uint64_t>(stat.total_heap_size()) >> 20);
}
#endif

#if defined(OS_MACOSX)
static size_t memoryUsageMBMac() {
  using base::ProcessMetrics;
  static ProcessMetrics* process_metrics =
      // The default port provider is sufficient to get data for the current
      // process.
      ProcessMetrics::CreateProcessMetrics(base::GetCurrentProcessHandle(),
                                           NULL);
  DCHECK(process_metrics);
  return process_metrics->GetPagefileUsage() >> 20;
}
#endif

#if !defined(OS_LINUX) && !defined(OS_MACOSX)
static size_t memoryUsageMBGeneric() {
  using base::ProcessMetrics;
  static ProcessMetrics* process_metrics =
      ProcessMetrics::CreateProcessMetrics(base::GetCurrentProcessHandle());
  DCHECK(process_metrics);
  return process_metrics->GetPagefileUsage() >> 20;
}
#endif

static size_t getMemoryUsageMB(bool bypass_cache) {
  size_t current_mem_usage = 0;
  MemoryUsageCache* mem_usage_cache_singleton = MemoryUsageCache::Get();
  if (!bypass_cache &&
      mem_usage_cache_singleton->IsCachedValueValid(&current_mem_usage))
    return current_mem_usage;

  current_mem_usage =
#if defined(OS_LINUX)
      memoryUsageMBLinux();
#elif defined(OS_MACOSX)
      memoryUsageMBMac();
#else
      memoryUsageMBGeneric();
#endif
  mem_usage_cache_singleton->SetMemoryValue(current_mem_usage);
  return current_mem_usage;
}

size_t WebKitClientImpl::memoryUsageMB() {
  return getMemoryUsageMB(false);
}

size_t WebKitClientImpl::actualMemoryUsageMB() {
  return getMemoryUsageMB(true);
}

void WebKitClientImpl::SuspendSharedTimer() {
  ++shared_timer_suspended_;
}

void WebKitClientImpl::ResumeSharedTimer() {
  // The shared timer may have fired or been adjusted while we were suspended.
  if (--shared_timer_suspended_ == 0 && !shared_timer_.IsRunning())
    setSharedTimerFireTime(shared_timer_fire_time_);
}

}  // namespace webkit_glue
