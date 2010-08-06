// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/capturer.h"

#include <algorithm>

namespace remoting {

Capturer::Capturer()
    : width_(0),
      height_(0),
      pixel_format_(PixelFormatInvalid),
      bytes_per_row_(0),
      current_buffer_(0) {
}

Capturer::~Capturer() {
}

void Capturer::ClearInvalidRects() {
  AutoLock auto_inval_rects_lock(inval_rects_lock_);
  inval_rects_.clear();
}

void Capturer::InvalidateRects(const InvalidRects& inval_rects) {
  InvalidRects temp_rects;
  std::set_union(inval_rects_.begin(), inval_rects_.end(),
                 inval_rects.begin(), inval_rects.end(),
                 std::inserter(temp_rects, temp_rects.begin()));
  {
    AutoLock auto_inval_rects_lock(inval_rects_lock_);
    inval_rects_.swap(temp_rects);
  }
}

void Capturer::InvalidateFullScreen() {
  AutoLock auto_inval_rects_lock(inval_rects_lock_);
  inval_rects_.clear();
  inval_rects_.insert(gfx::Rect(0, 0, width_, height_));
}

void Capturer::CaptureInvalidRects(CaptureCompletedCallback* callback) {
  // Calculate which rects need to be captured.
  CalculateInvalidRects();

  // Braced to scope the lock.
  InvalidRects local_rects;
  {
    AutoLock auto_inval_rects_lock(inval_rects_lock_);
    local_rects = inval_rects_;
    inval_rects_.clear();
  }

  CaptureRects(local_rects, callback);
}

void Capturer::FinishCapture(scoped_refptr<CaptureData> data,
                             CaptureCompletedCallback* callback) {
  // Select the next buffer to be the current buffer.
  current_buffer_ = (current_buffer_ + 1) % kNumBuffers;

  callback->Run(data);
  delete callback;
}

}  // namespace remoting
