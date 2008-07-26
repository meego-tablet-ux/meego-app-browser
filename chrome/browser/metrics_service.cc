// Copyright 2008, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.



//------------------------------------------------------------------------------
// Description of the life cycle of a instance of MetricsService.
//
//  OVERVIEW
//
// A MetricsService instance is typically created at application startup.  It
// is the central controller for the acquisition of log data, and the automatic
// transmission of that log data to an external server.  Its major job is to
// manage logs, grouping them for transmission, and transmitting them.  As part
// of its grouping, MS finalizes logs by including some just-in-time gathered
// memory statistics, snapshotting the current stats of numerous histograms,
// closing the logs, translating to XML text, and compressing the results for
// transmission.  Transmission includes submitting a compressed log as data in a
// URL-get, and retransmitting (or retaining at process termination) if the
// attempted transmission failed.  Retention across process terminations is done
// using the the PrefServices facilities.  The format for the retained
// logs (ones that never got transmitted) is always the uncompressed textual
// representation.
//
// Logs fall into one of two categories: "Initial logs," and "ongoing logs."
// There is at most one initial log sent for each complete run of the chrome
// product (from startup, to browser shutdown).  An initial log is generally
// transmitted some short time (1 minute?) after startup, and includes stats
// such as recent crash info, the number and types of plugins, etc.  The
// external server's response to the initial log conceptually tells
// this MS if it should continue transmitting logs (during this session). The
// server response can actually be much more detailed, and always includes (at
// a minimum) how often additional ongoing logs should be sent.
//
// After the above initial log, a series of ongoing logs will be transmitted.
// The first ongoing log actually begins to accumulate information stating when
// the MS was first constructed.  Note that even though the initial log is
// commonly sent a full minute after startup, the initial log does not include
// much in the way of user stats.   The most common interlog period (delay)
// is 5 minutes. That time period starts when the first user action causes a
// logging event.  This means that if there is no user action, there may be long
// periods without any (ongoing) log transmissions.  Ongoing log typically
// contain very detailed records of user activities (ex: opened tab, closed
// tab, fetched URL, maximized window, etc.)  In addition, just before an
// ongoing log is closed out, a call is made to gather memory statistics.  Those
// memory statistics are deposited into a histogram, and the log finalization
// code is then called.  In the finalization, a call to a Histogram server
// acquires a list of all local histograms that have been flagged for upload
// to the UMA server.
//
// When the browser shuts down, there will typically be a fragment of an ongoing
// log that has not yet been transmitted.  At shutdown time, that fragment
// is closed (including snapshotting histograms), and converted to text.  Note
// that memory stats are not gathered during shutdown, as gathering *might* be
// too time consuming.  The textual representation of the fragment of the
// ongoing log is then stored persistently as a string in the PrefServices, for
// potential transmission during a future run of the product.
//
// There are two slightly abnormal shutdown conditions.  There is a
// "disconnected scenario," and a "really fast startup and shutdown" scenario.
// In the "never connected" situation, the user has (during the running of the
// process) never established an internet connection.  As a result, attempts to
// transmit the initial log have failed, and a lot(?) of data has accumulated in
// the ongoing log (which didn't yet get closed, because there was never even a
// contemplation of sending it).  There is also a kindred "lost connection"
// situation, where a loss of connection prevented an ongoing log from being
// transmitted, and a (still open) log was stuck accumulating a lot(?) of data,
// while the earlier log retried its transmission.  In both of these
// disconnected situations, two logs need to be, and are, persistently stored
// for future transmission.
//
// The other unusual shutdown condition, termed "really fast startup and
// shutdown," involves the deliberate user termination of the process before
// the initial log is even formed or transmitted. In that situation, no logging
// is done, but the historical crash statistics remain (unlogged) for inclusion
// in a future run's initial log.  (i.e., we don't lose crash stats).
//
// With the above overview, we can now describe the state machine's various
// stats, based on the State enum specified in the state_ member.  Those states
// are:
//
//    INITIALIZED,            // Constructor was called.
//    PLUGIN_LIST_REQUESTED,  // Waiting for DLL list to be loaded.
//    PLUGIN_LIST_ARRIVED,    // Waiting for timer to send initial log.
//    INITIAL_LOG_READY,      // Initial log generated, and waiting for reply.
//    SEND_OLD_INITIAL_LOGS,  // Sending unsent logs from previous session.
//    SENDING_OLD_LOGS,       // Sending unsent logs from previous session.
//    SENDING_CURRENT_LOGS,   // Sending standard current logs as they accrue.
//
// In more detail, we have:
//
//    INITIALIZED,            // Constructor was called.
// The MS has been constructed, but has taken no actions to compose the
// initial log.
//
//    PLUGIN_LIST_REQUESTED,  // Waiting for DLL list to be loaded.
// Typically about 30 seconds after startup, a task is sent to a second thread
// to get the list of plugins.  That task will (when complete) make an async
// callback (via a Task) to indicate the completion.
//
//    PLUGIN_LIST_ARRIVED,    // Waiting for timer to send initial log.
// The callback has arrived, and it is now possible for an initial log to be
// created.  This callback typically arrives back less than one second after
// the task is dispatched.
//
//    INITIAL_LOG_READY,      // Initial log generated, and waiting for reply.
// This state is entered only after an initial log has been composed, and
// prepared for transmission.  It is also the case that any previously unsent
// logs have been loaded into instance variables for possible transmission.
//
//    SEND_OLD_INITIAL_LOGS,  // Sending unsent logs from previous session.
// This state indicates that the initial log for this session has been
// successfully sent and it is now time to send any "initial logs" that were
// saved from previous sessions.  Most commonly, there are none, but all old
// logs that were "initial logs" must be sent before this state is exited.
//
//    SENDING_OLD_LOGS,       // Sending unsent logs from previous session.
// This state indicates that there are no more unsent initial logs, and now any
// ongoing logs from previous sessions should be transmitted.  All such logs
// will be transmitted before exiting this state, and proceeding with ongoing
// logs from the current session (see next state).
//
//    SENDING_CURRENT_LOGS,   // Sending standard current logs as they accrue.
// Current logs are being accumulated.  Typically every 5 minutes a log is
// closed and finalized for transmission, at the same time as a new log is
// started.
//
// The progression through the above states is simple, and sequential, in the
// most common use cases.  States proceed from INITIAL to SENDING_CURRENT_LOGS,
// and remain in the latter until shutdown.
//
// The one unusual case is when the user asks that we stop logging.  When that
// happens, any pending (transmission in progress) log is pushed into the list
// of old unsent logs (the appropriate list, depending on whether it is an
// initial log, or an ongoing log).  An addition, any log that is currently
// accumulating is also finalized, and pushed into the unsent log list.  With
// those pushed performed, we regress back to the SEND_OLD_INITIAL_LOGS state in
// case the user enables log recording again during this session.  This way
// anything we have "pushed back" will be sent automatically if/when we progress
// back to SENDING_CURRENT_LOG state.
//
// Also note that whenever the member variables containing unsent logs are
// modified (i.e., when we send an old log), we mirror the list of logs into
// the PrefServices.  This ensures that IF we crash, we won't start up and
// retransmit our old logs again.
//
// Due to race conditions, it is always possible that a log file could be sent
// twice.  For example, if a log file is sent, but not yet acknowledged by
// the external server, and the user shuts down, then a copy of the log may be
// saved for re-transmission.  These duplicates could be filtered out server
// side, but are not expected to be a significantly statistical problem.
//
//
//------------------------------------------------------------------------------

#include <windows.h>

#include "chrome/browser/metrics_service.h"

#include "base/histogram.h"
#include "base/path_service.h"
#include "base/string_util.h"
#include "base/task.h"
#include "chrome/app/google_update_settings.h"
#include "chrome/browser/bookmark_bar_model.h"
#include "chrome/browser/browser.h"
#include "chrome/browser/browser_list.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/load_notification_details.h"
#include "chrome/browser/memory_details.h"
#include "chrome/browser/plugin_process_info.h"
#include "chrome/browser/plugin_service.h"
#include "chrome/browser/profile.h"
#include "chrome/browser/render_process_host.h"
#include "chrome/browser/template_url.h"
#include "chrome/browser/template_url_model.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/pref_service.h"
#include "googleurl/src/gurl.h"
#include "net/base/load_flags.h"
#include "third_party/bzip2/bzlib.h"

// Check to see that we're being called on only one thread.
static bool IsSingleThreaded();

static const char kMetricsURL[] =
    "https://toolbarqueries.google.com/firefox/metrics/collect";

static const char kMetricsType[] = "application/vnd.mozilla.metrics.bz2";

// The delay, in seconds, after startup before sending the first log message.
static const int kInitialLogDelay = 60;  // one minute

// When we have logs from previous Chrome sessions to send, how long should we
// delay (in seconds) between each log transmission.
static const int kUnsentLogDelay = 15;  // 15 seconds

// Minimum time a log typically exists before sending, in seconds.
// This number is supplied by the server, but until we parse it out of a server
// response, we use this duration to specify how long we should wait before
// sending the next log.  If the channel is busy, such as when there is a
// failure during an attempt to transmit a previous log, then a log may wait
// (and continue to accrue now log entries) for a much greater period of time.
static const int kMinSecondsPerLog = 5 * 60;  // five minutes

// We accept suggestions from the log server for how long to wait between
// submitting logs.  We validate that this "suggestion" is at least the
// following:
static const int kMinSuggestedSecondsPerLog = 60;

// When we don't succeed at transmitting a log to a server, we progressively
// wait longer and longer before sending the next log.  This backoff process
// help reduce load on the server, and makes the amount of backoff vary between
// clients so that a collision (server overload?) on retransmit is less likely.
// The following is the constant we use to expand that inter-log duration.
static const double kBackoff = 1.1;
// We limit the maximum backoff to be no greater than some multiple of the
// default kMinSecondsPerLog.  The following is that maximum ratio.
static const int kMaxBackoff = 10;

// Interval, in seconds, between state saves.
static const int kSaveStateInterval = 5 * 60;  // five minutes

// The number of "initial" logs we're willing to save, and hope to send during
// a future Chrome session.  Initial logs contain crash stats, and are pretty
// small.
static const size_t kMaxInitialLogsPersisted = 20;

// The number of ongoing logs we're willing to save persistently, and hope to
// send during a this or future sessions.  Note that each log will be pretty
// large, as presumably the related "initial" log wasn't sent (probably nothing
// was, as the user was probably off-line).  As a result, the log probably kept
// accumulating while the "initial" log was stalled (pending_), and couldn't be
// sent.  As a result, we don't want to save too many of these mega-logs.
// A "standard shutdown" will create a small log, including just the data that
// was not yet been transmitted, and that is normal (to have exactly one
// ongoing_log_ at startup).
static const size_t kMaxOngoingLogsPersisted = 4;


// Handles asynchronous fetching of memory details.
// Will run the provided task after finished.
class MetricsMemoryDetails : public MemoryDetails {
 public:
  explicit MetricsMemoryDetails(Task* completion) : completion_(completion) {}

  virtual void OnDetailsAvailable() {
    MessageLoop::current()->PostTask(FROM_HERE, completion_);
  }

 private:
  Task* completion_;
  DISALLOW_EVIL_CONSTRUCTORS(MetricsMemoryDetails);
};

class MetricsService::GetPluginListTaskComplete : public Task {
  virtual void Run() {
    g_browser_process->metrics_service()->OnGetPluginListTaskComplete();
  }
};

class MetricsService::GetPluginListTask : public Task {
 public:
  explicit GetPluginListTask(MessageLoop* callback_loop)
      : callback_loop_(callback_loop) {}

  virtual void Run() {
    std::vector<WebPluginInfo> plugins;
    PluginService::GetInstance()->GetPlugins(false, &plugins);

    callback_loop_->PostTask(FROM_HERE, new GetPluginListTaskComplete());
  }

 private:
  MessageLoop* callback_loop_;
};

// static
void MetricsService::RegisterPrefs(PrefService* local_state) {
  DCHECK(IsSingleThreaded());
  local_state->RegisterStringPref(prefs::kMetricsClientID, L"");
  local_state->RegisterStringPref(prefs::kMetricsClientIDTimestamp, L"0");
  local_state->RegisterStringPref(prefs::kStabilityLaunchTimeSec, L"0");
  local_state->RegisterStringPref(prefs::kStabilityLastTimestampSec, L"0");
  local_state->RegisterStringPref(prefs::kStabilityUptimeSec, L"0");
  local_state->RegisterBooleanPref(prefs::kStabilityExitedCleanly, true);
  local_state->RegisterBooleanPref(prefs::kStabilitySessionEndCompleted, true);
  local_state->RegisterIntegerPref(prefs::kMetricsSessionID, -1);
  local_state->RegisterIntegerPref(prefs::kStabilityLaunchCount, 0);
  local_state->RegisterIntegerPref(prefs::kStabilityCrashCount, 0);
  local_state->RegisterIntegerPref(prefs::kStabilityIncompleteSessionEndCount,
                                   0);
  local_state->RegisterIntegerPref(prefs::kStabilityPageLoadCount, 0);
  local_state->RegisterIntegerPref(prefs::kSecurityRendererOnSboxDesktop, 0);
  local_state->RegisterIntegerPref(prefs::kSecurityRendererOnDefaultDesktop, 0);
  local_state->RegisterIntegerPref(prefs::kStabilityRendererCrashCount, 0);
  local_state->RegisterIntegerPref(prefs::kStabilityRendererHangCount, 0);
  local_state->RegisterDictionaryPref(prefs::kProfileMetrics);
  local_state->RegisterIntegerPref(prefs::kNumBookmarksOnBookmarkBar, 0);
  local_state->RegisterIntegerPref(prefs::kNumFoldersOnBookmarkBar, 0);
  local_state->RegisterIntegerPref(prefs::kNumBookmarksInOtherBookmarkFolder,
                                   0);
  local_state->RegisterIntegerPref(prefs::kNumFoldersInOtherBookmarkFolder, 0);
  local_state->RegisterIntegerPref(prefs::kNumKeywords, 0);
  local_state->RegisterListPref(prefs::kMetricsInitialLogs);
  local_state->RegisterListPref(prefs::kMetricsOngoingLogs);
}

MetricsService::MetricsService()
    : recording_(false),
      reporting_(true),
      pending_log_(NULL),
      pending_log_text_(""),
      current_fetch_(NULL),
      current_log_(NULL),
      state_(INITIALIZED),
      next_window_id_(0),
      log_sender_factory_(this),
      state_saver_factory_(this),
      logged_samples_(),
      interlog_duration_(TimeDelta::FromSeconds(kInitialLogDelay)),
      timer_pending_(false) {
  DCHECK(IsSingleThreaded());
  InitializeMetricsState();
}

MetricsService::~MetricsService() {
  SetRecording(false);
}

void MetricsService::SetRecording(bool enabled) {
  DCHECK(IsSingleThreaded());

  if (enabled == recording_)
    return;

  if (enabled) {
    StartRecording();
    ListenerRegistration(true);
  } else {
    // Turn off all observers.
    ListenerRegistration(false);
    PushPendingLogsToUnsentLists();
    DCHECK(!pending_log());
    if (state_ > INITIAL_LOG_READY && unsent_logs())
      state_ = SEND_OLD_INITIAL_LOGS;
  }
  recording_ = enabled;
}

bool MetricsService::IsRecording() const {
  DCHECK(IsSingleThreaded());
  return recording_;
}

bool MetricsService::EnableReporting(bool enable) {
  bool done = GoogleUpdateSettings::SetCollectStatsConsent(enable);
  if (!done) {
    bool update_pref = GoogleUpdateSettings::GetCollectStatsConsent();
    if (enable != update_pref) {
      DLOG(INFO) << "METRICS: Unable to set crash report status to " << enable;
      return false;
    }
  }
  if (reporting_ != enable) {
    reporting_ = enable;
    if (reporting_)
      StartLogTransmissionTimer();
  }
  return true;
}

void MetricsService::Observe(NotificationType type,
                             const NotificationSource& source,
                             const NotificationDetails& details) {
  DCHECK(current_log_);
  DCHECK(IsSingleThreaded());

  if (!CanLogNotification(type, source, details))
    return;

  switch (type) {
    case NOTIFY_USER_ACTION:
        current_log_->RecordUserAction(*Details<const wchar_t*>(details).ptr());
      break;

    case NOTIFY_BROWSER_OPENED:
    case NOTIFY_BROWSER_CLOSED:
      LogWindowChange(type, source, details);
      break;

    case NOTIFY_TAB_APPENDED:
    case NOTIFY_TAB_CLOSING:
      LogWindowChange(type, source, details);
      break;

    case NOTIFY_LOAD_STOP:
      LogLoadComplete(type, source, details);
      break;

    case NOTIFY_LOAD_START:
      LogLoadStarted();
      break;

    case NOTIFY_RENDERER_PROCESS_TERMINATED:
      if (!*Details<bool>(details).ptr())
        LogRendererCrash();
      break;

    case NOTIFY_RENDERER_PROCESS_HANG:
      LogRendererHang();
      break;

    case NOTIFY_RENDERER_PROCESS_IN_SBOX:
      LogRendererInSandbox(*Details<bool>(details).ptr());
      break;

    case NOTIFY_PLUGIN_PROCESS_HOST_CONNECTED:
    case NOTIFY_PLUGIN_PROCESS_CRASHED:
    case NOTIFY_PLUGIN_INSTANCE_CREATED:
      LogPluginChange(type, source, details);
      break;

    case TEMPLATE_URL_MODEL_LOADED:
      LogKeywords(Source<TemplateURLModel>(source).ptr());
      break;

    case NOTIFY_OMNIBOX_OPENED_URL:
      current_log_->RecordOmniboxOpenedURL(
          *Details<AutocompleteLog>(details).ptr());
      break;

    case NOTIFY_BOOKMARK_MODEL_LOADED:
      LogBookmarks(Source<Profile>(source)->GetBookmarkBarModel());
      break;

    default:
      NOTREACHED();
      break;
  }
  StartLogTransmissionTimer();
}

void MetricsService::RecordCleanShutdown() {
  RecordBooleanPrefValue(prefs::kStabilityExitedCleanly, true);
}

void MetricsService::RecordStartOfSessionEnd() {
  RecordBooleanPrefValue(prefs::kStabilitySessionEndCompleted, false);
}

void MetricsService::RecordCompletedSessionEnd() {
  RecordBooleanPrefValue(prefs::kStabilitySessionEndCompleted, true);
}

//------------------------------------------------------------------------------
// private methods
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Initialization methods

void MetricsService::InitializeMetricsState() {
  PrefService* pref = g_browser_process->local_state();
  DCHECK(pref);

  client_id_ = WideToUTF8(pref->GetString(prefs::kMetricsClientID));
  if (client_id_.empty()) {
    client_id_ = GenerateClientID();
    pref->SetString(prefs::kMetricsClientID, UTF8ToWide(client_id_));

    // Might as well make a note of how long this ID has existed
    pref->SetString(prefs::kMetricsClientIDTimestamp,
                    Int64ToWString(Time::Now().ToTimeT()));
  }

  // Update session ID
  session_id_ = pref->GetInteger(prefs::kMetricsSessionID);
  ++session_id_;
  pref->SetInteger(prefs::kMetricsSessionID, session_id_);

  bool done = EnableReporting(GoogleUpdateSettings::GetCollectStatsConsent());
  DCHECK(done);

  // Stability bookkeeping
  int launches = pref->GetInteger(prefs::kStabilityLaunchCount);
  pref->SetInteger(prefs::kStabilityLaunchCount, launches + 1);

  bool exited_cleanly = pref->GetBoolean(prefs::kStabilityExitedCleanly);
  if (!exited_cleanly) {
    int crashes = pref->GetInteger(prefs::kStabilityCrashCount);
    pref->SetInteger(prefs::kStabilityCrashCount, crashes + 1);
  }
  pref->SetBoolean(prefs::kStabilityExitedCleanly, false);

  bool shutdown_cleanly =
      pref->GetBoolean(prefs::kStabilitySessionEndCompleted);
  if (!shutdown_cleanly) {
    int incomplete_session_end_count = pref->GetInteger(
        prefs::kStabilityIncompleteSessionEndCount);
    pref->SetInteger(prefs::kStabilityIncompleteSessionEndCount,
                     incomplete_session_end_count + 1);
  }
  // This is marked false when we get a WM_ENDSESSION.
  pref->SetBoolean(prefs::kStabilitySessionEndCompleted, true);

  int64 last_start_time =
      StringToInt64(pref->GetString(prefs::kStabilityLaunchTimeSec));
  int64 last_end_time =
      StringToInt64(pref->GetString(prefs::kStabilityLastTimestampSec));
  int64 uptime =
      StringToInt64(pref->GetString(prefs::kStabilityUptimeSec));

  if (last_start_time && last_end_time) {
    // TODO(JAR): Exclude sleep time.  ... which must be gathered in UI loop.
    uptime += last_end_time - last_start_time;
    pref->SetString(prefs::kStabilityUptimeSec, Int64ToWString(uptime));
  }
  pref->SetString(prefs::kStabilityLaunchTimeSec,
                  Int64ToWString(Time::Now().ToTimeT()));

  // Save profile metrics.
  PrefService* prefs = g_browser_process->local_state();
  if (prefs) {
    // Remove the current dictionary and store it for use when sending data to
    // server. By removing the value we prune potentially dead profiles
    // (and keys). All valid values are added back once services startup.
    const DictionaryValue* profile_dictionary =
        prefs->GetDictionary(prefs::kProfileMetrics);
    if (profile_dictionary) {
      // Do a deep copy of profile_dictionary since ClearPref will delete it.
      profile_dictionary_.reset(static_cast<DictionaryValue*>(
          profile_dictionary->DeepCopy()));
      prefs->ClearPref(prefs::kProfileMetrics);
    }
  }

  // Kick off the process of saving the state (so the uptime numbers keep
  // getting updated) every n minutes.
  ScheduleNextStateSave();
}

void MetricsService::OnGetPluginListTaskComplete() {
  DCHECK(state_ == PLUGIN_LIST_REQUESTED);
  if (state_ == PLUGIN_LIST_REQUESTED)
    state_ = PLUGIN_LIST_ARRIVED;
}

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


//------------------------------------------------------------------------------
// State save methods

void MetricsService::ScheduleNextStateSave() {
  state_saver_factory_.RevokeAll();

  MessageLoop::current()->PostDelayedTask(FROM_HERE,
      state_saver_factory_.NewRunnableMethod(&MetricsService::SaveLocalState),
      kSaveStateInterval * 1000);
}

void MetricsService::SaveLocalState() {
  PrefService* pref = g_browser_process->local_state();
  if (!pref) {
    NOTREACHED();
    return;
  }

  RecordCurrentState(pref);
  pref->ScheduleSavePersistentPrefs(g_browser_process->file_thread());

  ScheduleNextStateSave();
}


//------------------------------------------------------------------------------
// Recording control methods

void MetricsService::StartRecording() {
  if (current_log_)
    return;

  current_log_ = new MetricsLog(client_id_, session_id_);
  if (state_ == INITIALIZED) {
    // We only need to schedule that run once.
    state_ = PLUGIN_LIST_REQUESTED;

    // Make sure the plugin list is loaded before the inital log is sent, so
    // that the main thread isn't blocked generating the list.
    g_browser_process->file_thread()->message_loop()->PostDelayedTask(FROM_HERE,
        new GetPluginListTask(MessageLoop::current()),
        kInitialLogDelay * 1000 / 2);
  }
}

void MetricsService::StopRecording(MetricsLog** log) {
  if (!current_log_)
    return;

  // Put incremental histogram data at the end of every log transmission.
  // Don't bother if we're going to discard current_log_.
  if (log)
    RecordCurrentHistograms();

  current_log_->CloseLog();
  if (log) {
    *log = current_log_;
  } else {
    delete current_log_;
  }
  current_log_ = NULL;
}

void MetricsService::ListenerRegistration(bool start_listening) {
  AddOrRemoveObserver(this, NOTIFY_BROWSER_OPENED, start_listening);
  AddOrRemoveObserver(this, NOTIFY_BROWSER_CLOSED, start_listening);
  AddOrRemoveObserver(this, NOTIFY_USER_ACTION, start_listening);
  AddOrRemoveObserver(this, NOTIFY_TAB_APPENDED, start_listening);
  AddOrRemoveObserver(this, NOTIFY_TAB_CLOSING, start_listening);
  AddOrRemoveObserver(this, NOTIFY_LOAD_START, start_listening);
  AddOrRemoveObserver(this, NOTIFY_LOAD_STOP, start_listening);
  AddOrRemoveObserver(this, NOTIFY_RENDERER_PROCESS_IN_SBOX, start_listening);
  AddOrRemoveObserver(this, NOTIFY_RENDERER_PROCESS_TERMINATED,
                      start_listening);
  AddOrRemoveObserver(this, NOTIFY_RENDERER_PROCESS_HANG, start_listening);
  AddOrRemoveObserver(this, NOTIFY_PLUGIN_PROCESS_HOST_CONNECTED,
                      start_listening);
  AddOrRemoveObserver(this, NOTIFY_PLUGIN_INSTANCE_CREATED, start_listening);
  AddOrRemoveObserver(this, NOTIFY_PLUGIN_PROCESS_CRASHED, start_listening);
  AddOrRemoveObserver(this, TEMPLATE_URL_MODEL_LOADED, start_listening);
  AddOrRemoveObserver(this, NOTIFY_OMNIBOX_OPENED_URL, start_listening);
  AddOrRemoveObserver(this, NOTIFY_BOOKMARK_MODEL_LOADED, start_listening);
}

// static
void MetricsService::AddOrRemoveObserver(NotificationObserver* observer,
                                NotificationType type,
                                bool is_add) {
  NotificationService* service = NotificationService::current();

  if (is_add) {
    service->AddObserver(observer, type, NotificationService::AllSources());
  } else {
    service->RemoveObserver(observer, type, NotificationService::AllSources());
  }
}

void MetricsService::PushPendingLogsToUnsentLists() {
  if (state_ < INITIAL_LOG_READY)
    return;  // We didn't and still don't have time to get DLL list etc.

  if (pending_log()) {
    PreparePendingLogText();
    if (state_ == INITIAL_LOG_READY) {
      // We may race here, and send second copy of initial log later.
      unsent_initial_logs_.push_back(pending_log_text_);
      state_ = SENDING_CURRENT_LOGS;
    } else {
      unsent_ongoing_logs_.push_back(pending_log_text_);
    }
    DiscardPendingLog();
  }
  DCHECK(!pending_log());
  StopRecording(&pending_log_);
  PreparePendingLogText();
  unsent_ongoing_logs_.push_back(pending_log_text_);
  DiscardPendingLog();
  StoreUnsentLogs();
}

//------------------------------------------------------------------------------
// Transmission of logs methods

void MetricsService::StartLogTransmissionTimer() {
  if (!current_log_)
    return;  // Recorder is shutdown.
  if (timer_pending_ || !reporting_)
    return;
  // If there is no work to do, don't set a timer yet.
  if (!current_log_->num_events() && !pending_log() && !unsent_logs())
    return;
  timer_pending_ = true;
  MessageLoop::current()->PostDelayedTask(FROM_HERE,
      log_sender_factory_.
          NewRunnableMethod(&MetricsService::CollectMemoryDetails),
      static_cast<int>(interlog_duration_.InMilliseconds()));
}

void MetricsService::TryToStartTransmission() {
  DCHECK(IsSingleThreaded());

  DCHECK(timer_pending_);  // ONLY call via timer.

  DCHECK(!current_fetch_.get());
  if (current_fetch_.get())
    return;  // Redundant defensive coding.

  timer_pending_ = false;

  if (!current_log_)
    return;  // Logging was disabled.
  if (!reporting_ )
    return;  // Don't do work if we're not going to send anything now.

  if (!pending_log())
    switch (state_) {
      case INITIALIZED:  // We must be further along by now.
        DCHECK(false);
        return;

      case PLUGIN_LIST_REQUESTED:
        StartLogTransmissionTimer();
        return;

      case PLUGIN_LIST_ARRIVED:
        // We need to wait for the initial log to be ready before sending
        // anything, because the server will tell us whether it wants to hear
        // from us.
        PrepareInitialLog();
        DCHECK(state_ == PLUGIN_LIST_ARRIVED);
        RecallUnsentLogs();
        state_ = INITIAL_LOG_READY;
        break;

      case SEND_OLD_INITIAL_LOGS:
        if (!unsent_initial_logs_.empty()) {
          pending_log_text_ = unsent_initial_logs_.back();
          break;
        }
        state_ = SENDING_OLD_LOGS;
        // Fall through.

      case SENDING_OLD_LOGS:
        if (!unsent_ongoing_logs_.empty()) {
          pending_log_text_ = unsent_ongoing_logs_.back();
          break;
        }
        state_ = SENDING_CURRENT_LOGS;
        // Fall through.

      case SENDING_CURRENT_LOGS:
        if (!current_log_->num_events())
          return;  // Nothing to send.
        StopRecording(&pending_log_);
        StartRecording();
        break;

      default:
        DCHECK(false);
        return;
  }
  DCHECK(pending_log());

  PreparePendingLogForTransmission();
  if (!current_fetch_.get())
    return;  // Compression failed, and log discarded :-/.

  DCHECK(!timer_pending_);
  timer_pending_ = true;  // The URL fetch is a pseudo timer.
  current_fetch_->Start();
}

void MetricsService::CollectMemoryDetails() {
  Task* task = log_sender_factory_.
      NewRunnableMethod(&MetricsService::TryToStartTransmission);
  MetricsMemoryDetails* details = new MetricsMemoryDetails(task);
  details->StartFetch();

  // Collect WebCore cache information to put into a histogram.
  for (RenderProcessHost::iterator it = RenderProcessHost::begin();
       it != RenderProcessHost::end(); ++it) {
    it->second->Send(new ViewMsg_GetCacheResourceStats());
  }
}

void MetricsService::PrepareInitialLog() {
  DCHECK(state_ == PLUGIN_LIST_ARRIVED);
  std::vector<WebPluginInfo> plugins;
  PluginService::GetInstance()->GetPlugins(false, &plugins);

  MetricsLog* log = new MetricsLog(client_id_, session_id_);
  log->RecordEnvironment(plugins, profile_dictionary_.get());

  // Histograms only get written to current_log_, so setup for the write.
  MetricsLog* save_log = current_log_;
  current_log_ = log;
  RecordCurrentHistograms();  // Into current_log_... which is really log.
  current_log_ = save_log;

  log->CloseLog();
  DCHECK(!pending_log());
  pending_log_ = log;
}

void MetricsService::RecallUnsentLogs() {
  DCHECK(unsent_initial_logs_.empty());
  DCHECK(unsent_ongoing_logs_.empty());

  PrefService* local_state = g_browser_process->local_state();
  DCHECK(local_state);

  ListValue* unsent_initial_logs = local_state->GetMutableList(
      prefs::kMetricsInitialLogs);
  for (ListValue::iterator it = unsent_initial_logs->begin();
      it != unsent_initial_logs->end(); ++it) {
    std::wstring wide_log;
    (*it)->GetAsString(&wide_log);
    unsent_initial_logs_.push_back(WideToUTF8(wide_log));
  }

  ListValue* unsent_ongoing_logs = local_state->GetMutableList(
      prefs::kMetricsOngoingLogs);
  for (ListValue::iterator it = unsent_ongoing_logs->begin();
      it != unsent_ongoing_logs->end(); ++it) {
    std::wstring wide_log;
    (*it)->GetAsString(&wide_log);
    unsent_ongoing_logs_.push_back(WideToUTF8(wide_log));
  }
}

void MetricsService::StoreUnsentLogs() {
  if (state_ < INITIAL_LOG_READY)
    return;  // We never Recalled the prior unsent logs.

  PrefService* local_state = g_browser_process->local_state();
  DCHECK(local_state);

  ListValue* unsent_initial_logs = local_state->GetMutableList(
      prefs::kMetricsInitialLogs);
  unsent_initial_logs->Clear();
  size_t start = 0;
  if (unsent_initial_logs_.size() > kMaxInitialLogsPersisted)
    start = unsent_initial_logs_.size() - kMaxInitialLogsPersisted;
  for (size_t i = start; i < unsent_initial_logs_.size(); ++i)
    unsent_initial_logs->Append(
        Value::CreateStringValue(UTF8ToWide(unsent_initial_logs_[i])));

  ListValue* unsent_ongoing_logs = local_state->GetMutableList(
      prefs::kMetricsOngoingLogs);
  unsent_ongoing_logs->Clear();
  start = 0;
  if (unsent_ongoing_logs_.size() > kMaxOngoingLogsPersisted)
    start = unsent_ongoing_logs_.size() - kMaxOngoingLogsPersisted;
  for (size_t i = start; i < unsent_ongoing_logs_.size(); ++i)
    unsent_ongoing_logs->Append(
        Value::CreateStringValue(UTF8ToWide(unsent_ongoing_logs_[i])));
}

void MetricsService::PreparePendingLogText() {
  DCHECK(pending_log());
  if (!pending_log_text_.empty())
    return;
  int original_size = pending_log_->GetEncodedLogSize();
  pending_log_->GetEncodedLog(WriteInto(&pending_log_text_, original_size),
                              original_size);
}

void MetricsService::PreparePendingLogForTransmission() {
  DCHECK(pending_log());
  DCHECK(!current_fetch_.get());
  PreparePendingLogText();
  DCHECK(!pending_log_text_.empty());

  // Allow security conscious users to see all metrics logs that we send.
  LOG(INFO) << "METRICS LOG: " << pending_log_text_;

  std::string compressed_log;
  bool result = Bzip2Compress(pending_log_text_, &compressed_log);

  if (!result) {
    NOTREACHED() << "Failed to compress log for transmission.";
    DiscardPendingLog();
    StartLogTransmissionTimer();  // Maybe we'll do better on next log :-/.
    return;
  }
  current_fetch_.reset(new URLFetcher(GURL(kMetricsURL), URLFetcher::POST,
                                      this));
  current_fetch_->set_request_context(Profile::GetDefaultRequestContext());
  current_fetch_->set_upload_data(kMetricsType, compressed_log);
  // This flag works around the cert mismatch on toolbarqueries.google.com.
  current_fetch_->set_load_flags(net::LOAD_IGNORE_CERT_COMMON_NAME_INVALID);
}

void MetricsService::DiscardPendingLog() {
  if (pending_log_) {  // Shutdown might have deleted it!
    delete pending_log_;
    pending_log_ = NULL;
  }
  pending_log_text_.clear();
}

// This implementation is based on the Firefox MetricsService implementation.
bool MetricsService::Bzip2Compress(const std::string& input,
                                   std::string* output) {
  bz_stream stream = {0};
  // As long as our input is smaller than the bzip2 block size, we should get
  // the best compression.  For example, if your input was 250k, using a block
  // size of 300k or 500k should result in the same compression ratio.  Since
  // our data should be under 100k, using the minimum block size of 100k should
  // allocate less temporary memory, but result in the same compression ratio.
  int result = BZ2_bzCompressInit(&stream,
                                  1,   // 100k (min) block size
                                  0,   // quiet
                                  0);  // default "work factor"
  if (result != BZ_OK) {  // out of memory?
    return false;
  }

  output->clear();

  stream.next_in = const_cast<char*>(input.data());
  stream.avail_in = static_cast<int>(input.size());
  // NOTE: we don't need a BZ_RUN phase since our input buffer contains
  //       the entire input
  do {
    output->resize(output->size() + 1024);
    stream.next_out = &((*output)[stream.total_out_lo32]);
    stream.avail_out = static_cast<int>(output->size()) - stream.total_out_lo32;
    result = BZ2_bzCompress(&stream, BZ_FINISH);
  } while (result == BZ_FINISH_OK);
  if (result != BZ_STREAM_END)  // unknown failure?
    return false;
  result = BZ2_bzCompressEnd(&stream);
  DCHECK(result == BZ_OK);

  output->resize(stream.total_out_lo32);

  return true;
}

static const char* StatusToString(const URLRequestStatus& status) {
  switch (status.status()) {
    case URLRequestStatus::SUCCESS:
      return "SUCCESS";

    case URLRequestStatus::IO_PENDING:
      return "IO_PENDING";

    case URLRequestStatus::HANDLED_EXTERNALLY:
      return "HANDLED_EXTERNALLY";

    case URLRequestStatus::CANCELED:
      return "CANCELED";

    case URLRequestStatus::FAILED:
      return "FAILED";

    default:
      NOTREACHED();
      return "Unknown";
  }
}

void MetricsService::OnURLFetchComplete(const URLFetcher* source,
                                        const GURL& url,
                                        const URLRequestStatus& status,
                                        int response_code,
                                        const ResponseCookies& cookies,
                                        const std::string& data) {
  DCHECK(timer_pending_);
  timer_pending_ = false;
  DCHECK(current_fetch_.get());
  current_fetch_.reset(NULL);  // We're not allowed to re-use it.

  // Confirm send so that we can move on.
  DLOG(INFO) << "METRICS RESPONSE CODE: " << response_code
      << " status=" << StatusToString(status);
  if (response_code == 200) {  // Success.
    switch (state_) {
      case INITIAL_LOG_READY:
        state_ = SEND_OLD_INITIAL_LOGS;
        break;

      case SEND_OLD_INITIAL_LOGS:
        DCHECK(!unsent_initial_logs_.empty());
        unsent_initial_logs_.pop_back();
        StoreUnsentLogs();
        break;

      case SENDING_OLD_LOGS:
        DCHECK(!unsent_ongoing_logs_.empty());
        unsent_ongoing_logs_.pop_back();
        StoreUnsentLogs();
        break;

      case SENDING_CURRENT_LOGS:
        break;

      default:
        DCHECK(false);
        break;
    }

    DLOG(INFO) << "METRICS RESPONSE DATA: " << data;
    DiscardPendingLog();
    if (unsent_logs()) {
      DCHECK(state_ < SENDING_CURRENT_LOGS);
      interlog_duration_ = TimeDelta::FromSeconds(kUnsentLogDelay);
    } else {
      GetSuggestedInterlogTime(data);
    }
  } else {
    DLOG(INFO) << "METRICS: transmission attempt returned a failure code.  "
        "Verify network connectivity";
#ifndef NDEBUG
    DLOG(INFO) << "Verify your metrics logs are formatted correctly."
        "  Verify server is active at "  << kMetricsURL;
#endif
    if (!pending_log()) {
      DLOG(INFO) << "METRICS: Recorder shutdown during log transmission.";
    } else {
      // Send progressively less frequently.
      DCHECK(kBackoff > 1.0);
      interlog_duration_ = TimeDelta::FromMicroseconds(
          static_cast<int64>(kBackoff * interlog_duration_.InMicroseconds()));

      if (kMaxBackoff * TimeDelta::FromSeconds(kMinSecondsPerLog) <
          interlog_duration_)
        interlog_duration_ = kMaxBackoff *
            TimeDelta::FromSeconds(kMinSecondsPerLog);

      DLOG(INFO) << "METRICS: transmission retry being scheduled in " <<
          interlog_duration_.InSeconds() << " seconds for " <<
          pending_log_text_;
    }
  }
  StartLogTransmissionTimer();
}

// TODO(JAR): Carfully parse XML, rather than hacking.
void MetricsService::GetSuggestedInterlogTime(const std::string& server_data) {
  int interlog_seconds = kMinSecondsPerLog;
  const char* prefix = "<upload interval=\"";
  size_t seconds_indent = server_data.find(prefix);
  if (std::string::npos != seconds_indent) {
    int seconds;
    int result = sscanf(server_data.c_str() + seconds_indent + strlen(prefix),
                        "%d", &seconds);
    if (1 == result && seconds > kMinSuggestedSecondsPerLog)
      interlog_seconds = seconds;
  }
  interlog_duration_ = TimeDelta::FromSeconds(interlog_seconds);
}


void MetricsService::LogWindowChange(NotificationType type,
                                     const NotificationSource& source,
                                     const NotificationDetails& details) {
  int window_id = -1;
  int parent_id = -1;
  uintptr_t window_key = source.map_key();
  MetricsLog::WindowEventType window_type;

  // Note: since we stop all logging when a single OTR session is active, it is
  // possible that we start getting notifications about a window that we don't
  // know about.
  if (window_map_.find(window_key) == window_map_.end()) {
    window_id = next_window_id_++;
    window_map_[window_key] = window_id;
  } else {
    window_id = window_map_[window_key];
  }

  DCHECK(window_id != -1);

  if (type == NOTIFY_TAB_APPENDED) {
    parent_id = window_map_[details.map_key()];
  }

  switch (type) {
    case NOTIFY_TAB_APPENDED:
    case NOTIFY_BROWSER_OPENED:
      window_type = MetricsLog::WINDOW_CREATE;
      break;

    case NOTIFY_TAB_CLOSING:
    case NOTIFY_BROWSER_CLOSED:
      window_map_.erase(window_map_.find(window_key));
      window_type = MetricsLog::WINDOW_DESTROY;
      break;

    default:
      NOTREACHED();
      break;
  }

  current_log_->RecordWindowEvent(window_type, window_id, parent_id);
}

void MetricsService::LogLoadComplete(NotificationType type,
                                     const NotificationSource& source,
                                     const NotificationDetails& details) {
  if (details == NotificationService::NoDetails())
    return;

  const Details<LoadNotificationDetails> load_details(details);

  int window_id =
      window_map_[reinterpret_cast<uintptr_t>(load_details->controller())];
  current_log_->RecordLoadEvent(window_id,
                                load_details->url(),
                                load_details->origin(),
                                load_details->session_index(),
                                load_details->load_time());
}

void MetricsService::LogLoadStarted() {
  PrefService* prefs = g_browser_process->local_state();
  DCHECK(prefs);
  int loads = prefs->GetInteger(prefs::kStabilityPageLoadCount);
  prefs->SetInteger(prefs::kStabilityPageLoadCount, loads + 1);
  // We need to save the prefs, as page load count is a critical stat, and
  // it might be lost due to a crash :-(.
}

void MetricsService::LogRendererInSandbox(bool on_sandbox_desktop) {
  PrefService* prefs = g_browser_process->local_state();
  DCHECK(prefs);
  if (on_sandbox_desktop) {
    int count = prefs->GetInteger(prefs::kSecurityRendererOnSboxDesktop);
    prefs->SetInteger(prefs::kSecurityRendererOnSboxDesktop, count + 1);
  } else {
    int count = prefs->GetInteger(prefs::kSecurityRendererOnDefaultDesktop);
    prefs->SetInteger(prefs::kSecurityRendererOnDefaultDesktop, count + 1);
  }
}

void MetricsService::LogRendererCrash() {
  PrefService* prefs = g_browser_process->local_state();
  DCHECK(prefs);
  int crashes = prefs->GetInteger(prefs::kStabilityRendererCrashCount);
  prefs->SetInteger(prefs::kStabilityRendererCrashCount, crashes + 1);
}

void MetricsService::LogRendererHang() {
  PrefService* prefs = g_browser_process->local_state();
  DCHECK(prefs);
  int hangs = prefs->GetInteger(prefs::kStabilityRendererHangCount);
  prefs->SetInteger(prefs::kStabilityRendererHangCount, hangs + 1);
}

void MetricsService::LogPluginChange(NotificationType type,
                                     const NotificationSource& source,
                                     const NotificationDetails& details) {
  std::wstring plugin = Details<PluginProcessInfo>(details)->dll_path();

  if (plugin_stats_buffer_.find(plugin) == plugin_stats_buffer_.end()) {
    plugin_stats_buffer_[plugin] = PluginStats();
  }

  PluginStats& stats = plugin_stats_buffer_[plugin];
  switch (type) {
    case NOTIFY_PLUGIN_PROCESS_HOST_CONNECTED:
      stats.process_launches++;
      break;

    case NOTIFY_PLUGIN_INSTANCE_CREATED:
      stats.instances++;
      break;

    case NOTIFY_PLUGIN_PROCESS_CRASHED:
      stats.process_crashes++;
      break;

    default:
      NOTREACHED() << "Unexpected notification type " << type;
      return;
  }
}

// Recursively counts the number of bookmarks and folders in node.
static void CountBookmarks(BookmarkBarNode* node,
                           int* bookmarks,
                           int* folders) {
  if (node->GetType() == history::StarredEntry::URL)
    (*bookmarks)++;
  else
    (*folders)++;
  for (int i = 0; i < node->GetChildCount(); ++i)
    CountBookmarks(node->GetChild(i), bookmarks, folders);
}

void MetricsService::LogBookmarks(BookmarkBarNode* node,
                                  const wchar_t* num_bookmarks_key,
                                  const wchar_t* num_folders_key) {
  DCHECK(node);
  int num_bookmarks = 0;
  int num_folders = 0;
  CountBookmarks(node, &num_bookmarks, &num_folders);
  num_folders--;  // Don't include the root folder in the count.

  PrefService* pref = g_browser_process->local_state();
  DCHECK(pref);
  pref->SetInteger(num_bookmarks_key, num_bookmarks);
  pref->SetInteger(num_folders_key, num_folders);
}

void MetricsService::LogBookmarks(BookmarkBarModel* model) {
  DCHECK(model);
  LogBookmarks(model->GetBookmarkBarNode(),
               prefs::kNumBookmarksOnBookmarkBar,
               prefs::kNumFoldersOnBookmarkBar);
  LogBookmarks(model->other_node(),
               prefs::kNumBookmarksInOtherBookmarkFolder,
               prefs::kNumFoldersInOtherBookmarkFolder);
  ScheduleNextStateSave();
}

void MetricsService::LogKeywords(const TemplateURLModel* url_model) {
  DCHECK(url_model);

  PrefService* pref = g_browser_process->local_state();
  DCHECK(pref);
  pref->SetInteger(prefs::kNumKeywords,
                   static_cast<int>(url_model->GetTemplateURLs().size()));
  ScheduleNextStateSave();
}

void MetricsService::RecordPluginChanges(PrefService* pref) {
  ListValue* plugins = pref->GetMutableList(prefs::kStabilityPluginStats);
  DCHECK(plugins);

  for (ListValue::iterator value_iter = plugins->begin();
       value_iter != plugins->end(); ++value_iter) {
    if (!(*value_iter)->IsType(Value::TYPE_DICTIONARY)) {
      NOTREACHED();
      continue;
    }

    DictionaryValue* plugin_dict = static_cast<DictionaryValue*>(*value_iter);
    std::wstring plugin_path;
    plugin_dict->GetString(prefs::kStabilityPluginPath, &plugin_path);
    if (plugin_path.empty()) {
      NOTREACHED();
      continue;
    }

    if (plugin_stats_buffer_.find(plugin_path) == plugin_stats_buffer_.end())
      continue;

    PluginStats stats = plugin_stats_buffer_[plugin_path];
    if (stats.process_launches) {
      int launches = 0;
      plugin_dict->GetInteger(prefs::kStabilityPluginLaunches, &launches);
      launches += stats.process_launches;
      plugin_dict->SetInteger(prefs::kStabilityPluginLaunches, launches);
    }
    if (stats.process_crashes) {
      int crashes = 0;
      plugin_dict->GetInteger(prefs::kStabilityPluginCrashes, &crashes);
      crashes += stats.process_crashes;
      plugin_dict->SetInteger(prefs::kStabilityPluginCrashes, crashes);
    }
    if (stats.instances) {
      int instances = 0;
      plugin_dict->GetInteger(prefs::kStabilityPluginInstances, &instances);
      instances += stats.instances;
      plugin_dict->SetInteger(prefs::kStabilityPluginInstances, instances);
    }

    plugin_stats_buffer_.erase(plugin_path);
  }

  // Now go through and add dictionaries for plugins that didn't already have
  // reports in Local State.
  for (std::map<std::wstring, PluginStats>::iterator cache_iter =
           plugin_stats_buffer_.begin();
       cache_iter != plugin_stats_buffer_.end(); ++cache_iter) {
    std::wstring plugin_path = cache_iter->first;
    PluginStats stats = cache_iter->second;
    DictionaryValue* plugin_dict = new DictionaryValue;

    plugin_dict->SetString(prefs::kStabilityPluginPath, plugin_path);
    plugin_dict->SetInteger(prefs::kStabilityPluginLaunches,
                            stats.process_launches);
    plugin_dict->SetInteger(prefs::kStabilityPluginCrashes,
                            stats.process_crashes);
    plugin_dict->SetInteger(prefs::kStabilityPluginInstances,
                            stats.instances);
    plugins->Append(plugin_dict);
  }
  plugin_stats_buffer_.clear();
}

bool MetricsService::CanLogNotification(NotificationType type,
                                        const NotificationSource& source,
                                        const NotificationDetails& details) {
  // We simply don't log anything to UMA if there is a single off the record
  // session visible. The problem is that we always notify using the orginal
  // profile in order to simplify notification processing.
  return !BrowserList::IsOffTheRecordSessionActive();
}

void MetricsService::RecordBooleanPrefValue(const wchar_t* path, bool value) {
  DCHECK(IsSingleThreaded());

  PrefService* pref = g_browser_process->local_state();
  DCHECK(pref);

  pref->SetBoolean(path, value);
  RecordCurrentState(pref);
}

void MetricsService::RecordCurrentState(PrefService* pref) {
  pref->SetString(prefs::kStabilityLastTimestampSec,
                  Int64ToWString(Time::Now().ToTimeT()));

  RecordPluginChanges(pref);
}

void MetricsService::RecordCurrentHistograms() {
  DCHECK(current_log_);

  StatisticsRecorder::Histograms histograms;
  StatisticsRecorder::GetHistograms(&histograms);
  for (StatisticsRecorder::Histograms::iterator it = histograms.begin();
       histograms.end() != it;
       it++) {
    if ((*it)->flags() & kUmaTargetedHistogramFlag)
      RecordHistogram(**it);
  }
}

void MetricsService::RecordHistogram(const Histogram& histogram) {
  // Get up-to-date snapshot of sample stats.
  Histogram::SampleSet snapshot;
  histogram.SnapshotSample(&snapshot);

  const std::string& histogram_name = histogram.histogram_name();

  // Find the already sent stats, or create an empty set.
  LoggedSampleMap::iterator it = logged_samples_.find(histogram_name);
  Histogram::SampleSet* already_logged;
  if (logged_samples_.end() == it) {
    // Add new entry
    already_logged = &logged_samples_[histogram.histogram_name()];
    already_logged->Resize(histogram);  // Complete initialization.
  } else {
    already_logged = &(it->second);
    // Deduct any stats we've already logged from our snapshot.
    snapshot.Subtract(*already_logged);
  }

  // snapshot now contains only a delta to what we've already_logged.

  if (snapshot.TotalCount() > 0) {
    current_log_->RecordHistogramDelta(histogram, snapshot);
    // Add new data into our running total.
    already_logged->Add(snapshot);
  }
}

void MetricsService::AddProfileMetric(Profile* profile,
                                      const std::wstring& key,
                                      int value) {
  // Restriction of types is needed for writing values. See
  // MetricsLog::WriteProfileMetrics.
  DCHECK(profile && !key.empty());
  PrefService* prefs = g_browser_process->local_state();
  DCHECK(prefs);

  // Key is stored in prefs, which interpret '.'s as paths. As such, key
  // shouldn't have any '.'s in it.
  DCHECK(key.find(L'.') == std::wstring::npos);
  // The id is most likely an email address. We shouldn't send it to the server.
  const std::wstring id_hash =
      UTF8ToWide(MetricsLog::CreateBase64Hash(WideToUTF8(profile->GetID())));
  DCHECK(id_hash.find('.') == std::string::npos);

  DictionaryValue* prof_prefs = prefs->GetMutableDictionary(
      prefs::kProfileMetrics);
  DCHECK(prof_prefs);
  const std::wstring pref_key = std::wstring(prefs::kProfilePrefix) + id_hash +
      L"." + key;
  prof_prefs->SetInteger(pref_key.c_str(), value);
}

static bool IsSingleThreaded() {
  static int thread_id = 0;
  if (!thread_id)
    thread_id = GetCurrentThreadId();
  return GetCurrentThreadId() == thread_id;
}
