// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_NET_NOTIFIER_COMMUNICATOR_AUTO_RECONNECT_H_
#define CHROME_COMMON_NET_NOTIFIER_COMMUNICATOR_AUTO_RECONNECT_H_

#include <string>

#include "chrome/common/net/notifier/base/time.h"
#include "chrome/common/net/notifier/communicator/login.h"
#include "talk/base/sigslot.h"

namespace talk_base {
class Task;
}

namespace notifier {

class Timer;

class AutoReconnect : public sigslot::has_slots<> {
 public:
  explicit AutoReconnect(talk_base::Task* parent);
  void StartReconnectTimer();
  void StopReconnectTimer();
  void OnClientStateChange(Login::ConnectionState state);

  void NetworkStateChanged(bool is_alive);

  // Callback when power is suspended.
  void OnPowerSuspend(bool suspended);

  void set_idle(bool idle) {
    is_idle_ = idle;
  }

  // Returns true if the auto-retry is to be done (pending a countdown).
  bool is_retrying() const {
    return reconnect_timer_ != NULL;
  }

  int seconds_until() const;
  sigslot::signal0<> SignalTimerStartStop;
  sigslot::signal0<> SignalStartConnection;

 private:
  void StartReconnectTimerWithInterval(time64 interval_ns);
  void DoReconnect();
  void ResetState();
  void SetupReconnectInterval();
  void StopDelayedResetTimer();

  time64 reconnect_interval_ns_;
  Timer* reconnect_timer_;
  Timer* delayed_reset_timer_;
  talk_base::Task* parent_;

  bool is_idle_;
  DISALLOW_COPY_AND_ASSIGN(AutoReconnect);
};

// Wait 2 seconds until after we actually connect to reset reconnect related
// items.
//
// The reason for this delay is to avoid the situation in which buzz is trying
// to block the client due to abuse and the client responses by going into
// rapid reconnect mode, which makes the problem more severe.
extern const int kResetReconnectInfoDelaySec;

}  // namespace notifier

#endif  // CHROME_COMMON_NET_NOTIFIER_COMMUNICATOR_AUTO_RECONNECT_H_
