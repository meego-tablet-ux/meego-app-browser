// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "app/linear_animation.h"
#include "app/test_animation_delegate.h"
#if defined(OS_WIN)
#include "base/win_util.h"
#endif
#include "testing/gtest/include/gtest/gtest.h"

class AnimationTest: public testing::Test {
 private:
  MessageLoopForUI message_loop_;
};

namespace {

///////////////////////////////////////////////////////////////////////////////
// RunAnimation

class RunAnimation : public LinearAnimation {
 public:
  RunAnimation(int frame_rate, AnimationDelegate* delegate)
      : LinearAnimation(frame_rate, delegate) {
  }

  virtual void AnimateToState(double state) {
    EXPECT_LE(0.0, state);
    EXPECT_GE(1.0, state);
  }
};

///////////////////////////////////////////////////////////////////////////////
// CancelAnimation

class CancelAnimation : public LinearAnimation {
 public:
  CancelAnimation(int duration, int frame_rate, AnimationDelegate* delegate)
      : LinearAnimation(duration, frame_rate, delegate) {
  }

  virtual void AnimateToState(double state) {
    if (state >= 0.5)
      Stop();
  }
};

///////////////////////////////////////////////////////////////////////////////
// EndAnimation

class EndAnimation : public LinearAnimation {
 public:
  EndAnimation(int duration, int frame_rate, AnimationDelegate* delegate)
      : LinearAnimation(duration, frame_rate, delegate) {
  }

  virtual void AnimateToState(double state) {
    if (state >= 0.5)
      End();
  }
};

///////////////////////////////////////////////////////////////////////////////
// DeletingAnimationDelegate

// AnimationDelegate implementation that deletes the animation in ended.
class DeletingAnimationDelegate : public AnimationDelegate {
 public:
  virtual void AnimationEnded(const Animation* animation) {
    delete animation;
    MessageLoop::current()->Quit();
  }
};

}  // namespace

///////////////////////////////////////////////////////////////////////////////
// LinearCase

TEST_F(AnimationTest, RunCase) {
  TestAnimationDelegate ad;
  RunAnimation a1(150, &ad);
  a1.SetDuration(2000);
  a1.Start();
  MessageLoop::current()->Run();

  EXPECT_TRUE(ad.finished());
  EXPECT_FALSE(ad.canceled());
}

TEST_F(AnimationTest, CancelCase) {
  TestAnimationDelegate ad;
  CancelAnimation a2(2000, 150, &ad);
  a2.Start();
  MessageLoop::current()->Run();

  EXPECT_TRUE(ad.finished());
  EXPECT_TRUE(ad.canceled());
}

// Lets an animation run, invoking End part way through and make sure we get the
// right delegate methods invoked.
TEST_F(AnimationTest, EndCase) {
  TestAnimationDelegate ad;
  EndAnimation a2(2000, 150, &ad);
  a2.Start();
  MessageLoop::current()->Run();

  EXPECT_TRUE(ad.finished());
  EXPECT_FALSE(ad.canceled());
}

// Runs an animation with a delegate that deletes the animation in end.
TEST_F(AnimationTest, DeleteFromEnd) {
  DeletingAnimationDelegate delegate;
  RunAnimation* animation = new RunAnimation(150, &delegate);
  animation->Start();
  MessageLoop::current()->Run();
  // delegate should have deleted animation.
}

TEST_F(AnimationTest, ShouldRenderRichAnimation) {
#if defined(OS_WIN)
  if (win_util::GetWinVersion() >= win_util::WINVERSION_VISTA) {
    BOOL result;
    ASSERT_NE(
        0, ::SystemParametersInfo(SPI_GETCLIENTAREAANIMATION, 0, &result, 0));
    // ShouldRenderRichAnimation() should check the SPI_GETCLIENTAREAANIMATION
    // value on Vista.
    EXPECT_EQ(!!result, Animation::ShouldRenderRichAnimation());
  } else {
    // On XP, the function should check the SM_REMOTESESSION value.
    EXPECT_EQ(!::GetSystemMetrics(SM_REMOTESESSION),
              Animation::ShouldRenderRichAnimation());
  }
#else
  EXPECT_TRUE(Animation::ShouldRenderRichAnimation());
#endif
}
