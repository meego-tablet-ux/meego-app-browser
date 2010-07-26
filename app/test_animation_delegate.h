// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APP_TEST_ANIMATION_DELEGATE_H_
#define APP_TEST_ANIMATION_DELEGATE_H_
#pragma once

#include "app/animation.h"
#include "base/message_loop.h"

// Trivial AnimationDelegate implementation. AnimationEnded/Canceled quit the
// message loop.
class TestAnimationDelegate : public AnimationDelegate {
 public:
  TestAnimationDelegate() : canceled_(false), finished_(false) {
  }

  virtual void AnimationEnded(const Animation* animation) {
    finished_ = true;
    MessageLoop::current()->Quit();
  }

  virtual void AnimationCanceled(const Animation* animation) {
    finished_ = true;
    canceled_ = true;
    MessageLoop::current()->Quit();
  }

  bool finished() const {
    return finished_;
  }

  bool canceled() const {
    return canceled_;
  }

 private:
  bool canceled_;
  bool finished_;

  DISALLOW_COPY_AND_ASSIGN(TestAnimationDelegate);
};

#endif  // APP_TEST_ANIMATION_DELEGATE_H_
