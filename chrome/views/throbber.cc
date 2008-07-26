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

#include "chrome/views/throbber.h"

#include "base/message_loop.h"
#include "base/timer.h"
#include "chrome/app/theme/theme_resources.h"
#include "chrome/common/gfx/chrome_canvas.h"
#include "chrome/common/logging_chrome.h"
#include "chrome/common/resource_bundle.h"
#include "skia/include/SkBitmap.h"

namespace ChromeViews {

Throbber::Throbber(int frame_time_ms,
                   bool paint_while_stopped)
    : paint_while_stopped_(paint_while_stopped),
      running_(false),
      last_frame_drawn_(-1),
      frame_time_ms_(frame_time_ms),
      frames_(NULL),
      last_time_recorded_(0) {
  ResourceBundle &rb = ResourceBundle::GetSharedInstance();
  frames_ = rb.GetBitmapNamed(IDR_THROBBER);
  DCHECK(frames_->width() > 0 && frames_->height() > 0);
  DCHECK(frames_->width() % frames_->height() == 0);
  frame_count_ = frames_->width() / frames_->height();
}

Throbber::~Throbber() {
  Stop();
}

void Throbber::Start() {
  if (running_)
    return;

  start_time_ = GetTickCount();
  last_time_recorded_ = start_time_;

  timer_ = MessageLoop::current()->timer_manager()->StartTimer(
      frame_time_ms_ - 10, this, true);

  running_ = true;

  SchedulePaint();  // paint right away
}

void Throbber::Stop() {
  if (!running_)
    return;

  MessageLoop::current()->timer_manager()->StopTimer(timer_);
  timer_ = NULL;

  running_ = false;
  SchedulePaint();  // Important if we're not painting while stopped
}

void Throbber::Run() {
  DCHECK(running_);

  SchedulePaint();
}

void Throbber::GetPreferredSize(CSize *out) {
  DCHECK(out);

  out->SetSize(frames_->height(), frames_->height());
}

void Throbber::Paint(ChromeCanvas* canvas) {
  if (!running_ && !paint_while_stopped_)
    return;

  DWORD current_time = GetTickCount();
  int current_frame = 0;

  // deal with timer wraparound
  if (current_time < last_time_recorded_) {
    start_time_ = current_time;
    current_frame = (last_frame_drawn_ + 1) % frame_count_;
  } else {
    current_frame =
        ((current_time - start_time_) / frame_time_ms_) % frame_count_;
  }

  last_time_recorded_ = current_time;
  last_frame_drawn_ = current_frame;

  int image_size = frames_->height();
  int image_offset = current_frame * image_size;
  canvas->DrawBitmapInt(*frames_,
                        image_offset, 0, image_size, image_size,
                        0, 0, image_size, image_size,
                        false);
}



// Smoothed throbber ---------------------------------------------------------


// Delay after work starts before starting throbber, in milliseconds.
static const int kStartDelay = 200;

// Delay after work stops before stopping, in milliseconds.
static const int kStopDelay = 50;


SmoothedThrobber::SmoothedThrobber(int frame_time_ms)
    : Throbber(frame_time_ms, /* paint_while_stopped= */ false),
      start_delay_factory_(this),
      end_delay_factory_(this) {
}

void SmoothedThrobber::Start() {
  end_delay_factory_.RevokeAll();

  if (!running_ && start_delay_factory_.empty()) {
    MessageLoop::current()->PostDelayedTask(FROM_HERE,
        start_delay_factory_.NewRunnableMethod(
            &SmoothedThrobber::StartDelayOver),
        kStartDelay);
  }
}

void SmoothedThrobber::StartDelayOver() {
  Throbber::Start();
}

void SmoothedThrobber::Stop() {
  TimerManager* timer_manager = MessageLoop::current()->timer_manager();

  if (!running_)
    start_delay_factory_.RevokeAll();

  end_delay_factory_.RevokeAll();
  MessageLoop::current()->PostDelayedTask(FROM_HERE,
      end_delay_factory_.NewRunnableMethod(&SmoothedThrobber::StopDelayOver),
      kStopDelay);
}

void SmoothedThrobber::StopDelayOver() {
  Throbber::Stop();
}

// Checkmark throbber ---------------------------------------------------------

CheckmarkThrobber::CheckmarkThrobber()
    : checked_(false),
      Throbber(kFrameTimeMs, false) {
  InitClass();
}

void CheckmarkThrobber::SetChecked(bool checked) {
  bool changed = checked != checked_;
  if (changed) {
    checked_ = checked;
    SchedulePaint();
  }
}

void CheckmarkThrobber::Paint(ChromeCanvas* canvas) {
  if (running_) {
    // Let the throbber throb...
    Throbber::Paint(canvas);
    return;
  }
  // Otherwise we paint our tick mark or nothing depending on our state.
  if (checked_) {
    int checkmark_x = (GetWidth() - checkmark_->width()) / 2;
    int checkmark_y = (GetHeight() - checkmark_->height()) / 2;
    canvas->DrawBitmapInt(*checkmark_, checkmark_x, checkmark_y);
  }
}

// static
void CheckmarkThrobber::InitClass() {
  static bool initialized = false;
  if (!initialized) {
    ResourceBundle& rb = ResourceBundle::GetSharedInstance();
    checkmark_ = rb.GetBitmapNamed(IDR_INPUT_GOOD);
    initialized = true;
  }
}

// static
SkBitmap* CheckmarkThrobber::checkmark_ = NULL;

}  // namespace ChromeViews
