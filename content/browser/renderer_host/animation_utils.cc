/*
 * Copyright (c) 2010, Intel Corporation. All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met:
 * 
 *     * Redistributions of source code must retain the above copyright 
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above 
 * copyright notice, this list of conditions and the following disclaimer 
 * in the documentation and/or other materials provided with the 
 * distribution.
 *     * Neither the name of Intel Corporation nor the names of its 
 * contributors may be used to endorse or promote products derived from 
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <QPoint>
#include <QEasingCurve>

#include "content/browser/renderer_host/animation_utils.h"

namespace {
const int ThresholdX = 5;
const int ThresholdY = 5;
const int HistoryWeight = 1;
const int Friction = 10;
const int MaxDuration = 5000;  // 5 secs
const int MinDuration = 1000;  // 1 sec

// for performance consideration, we try to combine several animation event into one
const int trigger_ratio = 3;
}


PanAnimation::PanAnimation(QObject *parent)
    : QVariantAnimation(parent)
{
  dx_ = 0;
  dy_ = 0;
  previous_dx_ = 0;
  previous_dy_ = 0;
  pending_dx_ = 0;
  pending_dy_ = 0;
  index_ = 0;

  duration_ = MinDuration;
  setEasingCurve(QEasingCurve::InQuad);
}

void PanAnimation::updateCurrentValue(const QVariant &value)
{
  QPointF dp = value.toPointF();

  pending_dx_ += dp.x();
  pending_dy_ += dp.y();

  if (++index_ < trigger_ratio)
    return;

  emit panTriggered(static_cast<int>(pending_dx_), static_cast<int>(pending_dy_));

  index_ = 0;
  pending_dx_ = 0;
  pending_dy_ = 0;
}

void PanAnimation::reset()
{
  dx_ = 0;
  dy_ = 0;
  previous_dx_ = 0;
  previous_dy_ = 0;
  pending_dx_ = 0;
  pending_dy_ = 0;
  index_ = 0;
}

void PanAnimation::feedMotion(int dx, int dy)
{
// Accumulate previous motion and lower the weight gradually
  dx_ = (previous_dx_ * HistoryWeight + dx) / (HistoryWeight + 1);
  dy_ = (previous_dy_ * HistoryWeight + dy) / (HistoryWeight + 1);
  previous_dx_ = dx_;
  previous_dy_ = dy_;
}

void PanAnimation::start()
{
  int vx, vy;

  vx = qAbs(dx_);
  vy = qAbs(dy_);

  duration_ = (vx + vy) * 1000 / Friction;
  duration_ = qBound(MinDuration, duration_, MaxDuration);

  QPointF speedxy(dx_, dy_);

  if ((vx > ThresholdX) || ( vy > ThresholdY)) {
    setStartValue(speedxy);
    setEndValue(QPointF(0, 0));
    setLoopCount(1);
    setDuration(duration_);
    QVariantAnimation::start();
  } else {
    // not meet the threshold, so we don't start an auto pan animation
  }

  reset();
}

#include "moc_animation_utils.cc"
