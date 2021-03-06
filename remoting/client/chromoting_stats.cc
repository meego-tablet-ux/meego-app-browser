// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/chromoting_stats.h"

namespace {

// The default window of bandwidth in seconds. Bandwidth is reported as
// number of numbers received in this time frame.
static const int kBandwidthWindow = 3;

// We take the last 10 latency numbers and report the average.
static const int kLatencyWindow = 10;

}  // namespace

namespace remoting {

ChromotingStats::ChromotingStats()
    : video_bandwidth_(base::TimeDelta::FromSeconds(kBandwidthWindow)),
      video_capture_ms_(kLatencyWindow),
      video_encode_ms_(kLatencyWindow),
      video_decode_ms_(kLatencyWindow),
      video_paint_ms_(kLatencyWindow) {
}

ChromotingStats::~ChromotingStats() {
}

}  // namespace remoting
