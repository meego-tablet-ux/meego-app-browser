// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


//------------------------------------------------------------------------------
// Description of the life cycle of a instance of MetricsService.
//
//  OVERVIEW
//
// A MetricsService instance is created at ChromeFrame startup in
// the IE process. It is the central controller for the UMA log data.
// Its major job is to manage logs, prepare them for transmission.
// Currently only histogram data is tracked in log.  When MetricsService
// prepares log for submission it snapshots the current stats of histograms,
// translates log to XML.  Transmission includes submitting a compressed log
// as data in a URL-get, and is performed using functionality provided by
// Urlmon
// The actual transmission is performed using a windows timer procedure which
// basically means that the thread on which the MetricsService object is
// instantiated needs a message pump. Also on IE7 where every tab is created
// on its own thread we would have a case where the timer procedures can
// compete for sending histograms.
//
// When preparing log for submission we acquire a list of all local histograms
// that have been flagged for upload to the UMA server.
//
// When ChromeFrame shuts down, there will typically be a fragment of an ongoing
// log that has not yet been transmitted.  Currently this data is ignored.
//
// With the above overview, we can now describe the state machine's various
// stats, based on the State enum specified in the state_ member.  Those states
// are:
//
//    INITIALIZED,      // Constructor was called.
//    ACTIVE,           // Accumalating log data.
//    STOPPED,          // Service has stopped.
//
//-----------------------------------------------------------------------------

#include "chrome_frame/metrics_service.h"

#include <windows.h>
#include <objbase.h>

#if defined(USE_SYSTEM_LIBBZ2)
#include <bzlib.h>
#else
#include "third_party/bzip2/bzlib.h"
#endif

#include "base/file_version_info.h"
#include "base/string_util.h"
#include "base/thread.h"
#include "base/string_number_conversions.h"
#include "base/utf_string_conversions.h"
#include "chrome/common/chrome_version_info.h"
#include "chrome/installer/util/browser_distribution.h"
#include "chrome/installer/util/chrome_frame_distribution.h"
#include "chrome/installer/util/google_update_settings.h"
#include "chrome_frame/bind_status_callback_impl.h"
#include "chrome_frame/crash_reporting/crash_metrics.h"
#include "chrome_frame/html_utils.h"
#include "chrome_frame/http_negotiate.h"
#include "chrome_frame/utils.h"
#include "net/base/upload_data.h"
#include "chrome_frame/urlmon_bind_status_callback.h"

using base::Time;
using base::TimeDelta;

static const char kMetricsType[] =
    "Content-Type: application/vnd.mozilla.metrics.bz2\r\n";

// The first UMA upload occurs after this interval.
static const int kInitialUMAUploadTimeoutMilliSeconds = 30000;

// Default to one UMA upload per 10 mins.
static const int kMinMilliSecondsPerUMAUpload = 600000;

base::LazyInstance<base::ThreadLocalPointer<MetricsService> >
    MetricsService::g_metrics_instance_(base::LINKER_INITIALIZED);

extern base::LazyInstance<StatisticsRecorder> g_statistics_recorder_;

// This class provides functionality to upload the ChromeFrame UMA data to the
// server. An instance of this class is created whenever we have data to be
// uploaded to the server.
class ChromeFrameMetricsDataUploader : public BSCBImpl {
 public:
  ChromeFrameMetricsDataUploader()
      : cache_stream_(NULL),
        upload_data_size_(0) {
    DLOG(INFO) << __FUNCTION__;
  }

  ~ChromeFrameMetricsDataUploader() {
    DLOG(INFO) << __FUNCTION__;
  }

  static HRESULT ChromeFrameMetricsDataUploader::UploadDataHelper(
      const std::string& upload_data) {
    CComObject<ChromeFrameMetricsDataUploader>* data_uploader = NULL;
    CComObject<ChromeFrameMetricsDataUploader>::CreateInstance(&data_uploader);
    DCHECK(data_uploader != NULL);

    data_uploader->AddRef();
    HRESULT hr = data_uploader->UploadData(upload_data);
    if (FAILED(hr)) {
      DLOG(ERROR) << "Failed to initialize ChromeFrame UMA data uploader: Err"
                  << hr;
    }
    data_uploader->Release();
    return hr;
  }

  HRESULT UploadData(const std::string& upload_data) {
    if (upload_data.empty()) {
      NOTREACHED() << "Invalid upload data";
      return E_INVALIDARG;
    }

    DCHECK(cache_stream_.get() == NULL);

    upload_data_size_ = upload_data.size() + 1;

    HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, cache_stream_.Receive());
    if (FAILED(hr)) {
      NOTREACHED() << "Failed to create stream. Error:"
                   << hr;
      return hr;
    }

    DCHECK(cache_stream_.get());

    unsigned long written = 0;
    cache_stream_->Write(upload_data.c_str(), upload_data_size_, &written);
    DCHECK(written == upload_data_size_);

    RewindStream(cache_stream_);

    BrowserDistribution* dist = ChromeFrameDistribution::GetDistribution();
    server_url_ = dist->GetStatsServerURL();
    DCHECK(!server_url_.empty());

    hr = CreateURLMoniker(NULL, server_url_.c_str(),
                          upload_moniker_.Receive());
    if (FAILED(hr)) {
      DLOG(ERROR) << "Failed to create url moniker for url:"
                  << server_url_.c_str()
                  << " Error:"
                  << hr;
    } else {
      ScopedComPtr<IBindCtx> context;
      hr = CreateAsyncBindCtx(0, this, NULL, context.Receive());
      DCHECK(SUCCEEDED(hr));
      DCHECK(context);

      ScopedComPtr<IStream> stream;
      hr = upload_moniker_->BindToStorage(
          context, NULL, IID_IStream,
          reinterpret_cast<void**>(stream.Receive()));
      if (FAILED(hr)) {
        NOTREACHED();
        DLOG(ERROR) << "Failed to bind to upload data moniker. Error:"
                    << hr;
      }
    }
    return hr;
  }

  STDMETHOD(BeginningTransaction)(LPCWSTR url, LPCWSTR headers, DWORD reserved,
                                  LPWSTR* additional_headers) {
    std::string new_headers;
    new_headers = StringPrintf("Content-Length: %s\r\n",
                               base::Int64ToString(upload_data_size_).c_str());
    new_headers += kMetricsType;

    std::string user_agent_value = http_utils::GetDefaultUserAgent();
    user_agent_value = http_utils::AddChromeFrameToUserAgentValue(
        user_agent_value);

    new_headers += "User-Agent: " + user_agent_value;
    new_headers += "\r\n";

    *additional_headers = reinterpret_cast<wchar_t*>(
        CoTaskMemAlloc((new_headers.size() + 1) * sizeof(wchar_t)));

    lstrcpynW(*additional_headers, ASCIIToWide(new_headers).c_str(),
              new_headers.size());

    return BSCBImpl::BeginningTransaction(url, headers, reserved,
                                          additional_headers);
  }

  STDMETHOD(GetBindInfo)(DWORD* bind_flags, BINDINFO* bind_info) {
    if ((bind_info == NULL) || (bind_info->cbSize == 0) ||
        (bind_flags == NULL))
      return E_INVALIDARG;

    *bind_flags = BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;
    // Bypass caching proxies on POSTs and PUTs and avoid writing responses to
    // these requests to the browser's cache
    *bind_flags |= BINDF_GETNEWESTVERSION | BINDF_PRAGMA_NO_CACHE;

    DCHECK(cache_stream_.get());

    // Initialize the STGMEDIUM.
    memset(&bind_info->stgmedData, 0, sizeof(STGMEDIUM));
    bind_info->grfBindInfoF = 0;
    bind_info->szCustomVerb = NULL;
    bind_info->dwBindVerb = BINDVERB_POST;
    bind_info->stgmedData.tymed = TYMED_ISTREAM;
    bind_info->stgmedData.pstm = cache_stream_.get();
    bind_info->stgmedData.pstm->AddRef();
    return BSCBImpl::GetBindInfo(bind_flags, bind_info);
  }

  STDMETHOD(OnResponse)(DWORD response_code, LPCWSTR response_headers,
                        LPCWSTR request_headers, LPWSTR* additional_headers) {
    DLOG(INFO) << __FUNCTION__ << " headers: \n" << response_headers;
    return BSCBImpl::OnResponse(response_code, response_headers,
                                request_headers, additional_headers);
  }

 private:
  std::wstring server_url_;
  size_t upload_data_size_;
  ScopedComPtr<IStream> cache_stream_;
  ScopedComPtr<IMoniker> upload_moniker_;
};

MetricsService* MetricsService::GetInstance() {
  if (g_metrics_instance_.Pointer()->Get())
    return g_metrics_instance_.Pointer()->Get();

  g_metrics_instance_.Pointer()->Set(new MetricsService);
  return g_metrics_instance_.Pointer()->Get();
}

MetricsService::MetricsService()
    : recording_active_(false),
      reporting_active_(false),
      user_permits_upload_(false),
      state_(INITIALIZED),
      thread_(NULL),
      initial_uma_upload_(true),
      transmission_timer_id_(0) {
}

MetricsService::~MetricsService() {
  SetRecording(false);
  if (pending_log_) {
    delete pending_log_;
    pending_log_ = NULL;
  }
  if (current_log_) {
    delete current_log_;
    current_log_ = NULL;
  }
}

void MetricsService::InitializeMetricsState() {
  DCHECK(state_ == INITIALIZED);

  thread_ = PlatformThread::CurrentId();

  user_permits_upload_ = GoogleUpdateSettings::GetCollectStatsConsent();
  // Update session ID
  session_id_ = CrashMetricsReporter::GetInstance()->IncrementMetric(
      CrashMetricsReporter::SESSION_ID);

  // Ensure that an instance of the StatisticsRecorder object is created.
  g_statistics_recorder_.Get();

  CrashMetricsReporter::GetInstance()->set_active(true);
}

// static
void MetricsService::Start() {
  if (GetInstance()->state_ == ACTIVE)
    return;

  GetInstance()->InitializeMetricsState();
  GetInstance()->SetRecording(true);
  GetInstance()->SetReporting(true);
}

// static
void MetricsService::Stop() {
  GetInstance()->SetReporting(false);
  GetInstance()->SetRecording(false);
}

void MetricsService::SetRecording(bool enabled) {
  DCHECK_EQ(thread_, PlatformThread::CurrentId());
  if (enabled == recording_active_)
    return;

  if (enabled) {
    if (client_id_.empty()) {
      client_id_ = GenerateClientID();
      // Save client id somewhere.
    }
    StartRecording();

  } else {
    state_ = STOPPED;
  }
  recording_active_ = enabled;
}

// static
std::string MetricsService::GenerateClientID() {
  const int kGUIDSize = 39;

  GUID guid;
  HRESULT guid_result = CoCreateGuid(&guid);
  DCHECK(SUCCEEDED(guid_result));

  std::wstring guid_string;
  int result = StringFromGUID2(guid,
                               WriteInto(&guid_string, kGUIDSize), kGUIDSize);
  DCHECK(result == kGUIDSize);
  return WideToUTF8(guid_string.substr(1, guid_string.length() - 2));
}

// static
void CALLBACK MetricsService::TransmissionTimerProc(HWND window,
                                                    unsigned int message,
                                                    unsigned int event_id,
                                                    unsigned int time) {
  DLOG(INFO) << "Transmission timer notified";
  DCHECK(GetInstance() != NULL);
  GetInstance()->UploadData();
  if (GetInstance()->initial_uma_upload_) {
    // If this is the first uma upload by this process then subsequent uma
    // uploads should occur once every 10 minutes(default).
    GetInstance()->initial_uma_upload_ = false;
    DCHECK(GetInstance()->transmission_timer_id_ != 0);
    SetTimer(NULL, GetInstance()->transmission_timer_id_,
             kMinMilliSecondsPerUMAUpload,
             reinterpret_cast<TIMERPROC>(TransmissionTimerProc));
  }
}

void MetricsService::SetReporting(bool enable) {
  static const int kChromeFrameMetricsTimerId = 0xFFFFFFFF;

  DCHECK_EQ(thread_, PlatformThread::CurrentId());
  if (reporting_active_ != enable) {
    reporting_active_ = enable;
    if (reporting_active_) {
      transmission_timer_id_ =
          SetTimer(NULL, kChromeFrameMetricsTimerId,
                   kInitialUMAUploadTimeoutMilliSeconds,
                   reinterpret_cast<TIMERPROC>(TransmissionTimerProc));
    }
  }
}

//------------------------------------------------------------------------------
// Recording control methods

void MetricsService::StartRecording() {
  DCHECK_EQ(thread_, PlatformThread::CurrentId());
  if (current_log_)
    return;

  current_log_ = new MetricsLogBase(client_id_, session_id_,
                                    GetVersionString());
  if (state_ == INITIALIZED)
    state_ = ACTIVE;
}

void MetricsService::StopRecording(bool save_log) {
  DCHECK_EQ(thread_, PlatformThread::CurrentId());
  if (!current_log_)
    return;

  // Put incremental histogram deltas at the end of all log transmissions.
  // Don't bother if we're going to discard current_log_.
  if (save_log) {
    CrashMetricsReporter::GetInstance()->RecordCrashMetrics();
    RecordCurrentHistograms();
  }

  if (save_log) {
    pending_log_ = current_log_;
  }
  current_log_ = NULL;
}

void MetricsService::MakePendingLog() {
  DCHECK_EQ(thread_, PlatformThread::CurrentId());
  if (pending_log())
    return;

  switch (state_) {
    case INITIALIZED:  // We should be further along by now.
      DCHECK(false);
      return;

    case ACTIVE:
      StopRecording(true);
      StartRecording();
      break;

    default:
      DCHECK(false);
      return;
  }

  DCHECK(pending_log());
}

bool MetricsService::TransmissionPermitted() const {
  // If the user forbids uploading that's their business, and we don't upload
  // anything.
  return user_permits_upload_;
}

std::string MetricsService::PrepareLogSubmissionString() {
  DCHECK_EQ(thread_, PlatformThread::CurrentId());

  MakePendingLog();
  DCHECK(pending_log());
  if (pending_log_== NULL) {
    return std::string();
  }

  pending_log_->CloseLog();
  std::string pending_log_text = pending_log_->GetEncodedLogString();
  DCHECK(!pending_log_text.empty());
  DiscardPendingLog();
  return pending_log_text;
}

bool MetricsService::UploadData() {
  DCHECK_EQ(thread_, PlatformThread::CurrentId());

  if (!GetInstance()->TransmissionPermitted())
    return false;

  static long currently_uploading = 0;
  if (InterlockedCompareExchange(&currently_uploading, 1, 0)) {
    DLOG(INFO) << "Contention for uploading metrics data. Backing off";
    return false;
  }

  std::string pending_log_text = PrepareLogSubmissionString();
  DCHECK(!pending_log_text.empty());

  // Allow security conscious users to see all metrics logs that we send.
  LOG(INFO) << "METRICS LOG: " << pending_log_text;

  bool ret = true;

  if (!Bzip2Compress(pending_log_text, &compressed_log_)) {
    NOTREACHED() << "Failed to compress log for transmission.";
    ret = false;
  } else {
    HRESULT hr = ChromeFrameMetricsDataUploader::UploadDataHelper(
        compressed_log_);
    DCHECK(SUCCEEDED(hr));
  }
  DiscardPendingLog();

  currently_uploading = 0;
  return ret;
}

// static
std::string MetricsService::GetVersionString() {
  chrome::VersionInfo version_info;
  if (version_info.is_valid()) {
    std::string version = version_info.Version();
    // Add the -F extensions to ensure that UMA data uploaded by ChromeFrame
    // lands in the ChromeFrame bucket.
    version += "-F";
    if (!version_info.IsOfficialBuild())
      version.append("-devel");
    return version;
  } else {
    NOTREACHED() << "Unable to retrieve version string.";
  }

  return std::string();
}
