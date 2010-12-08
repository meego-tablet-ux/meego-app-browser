// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GFX_SCOPED_CG_CONTEXT_STATE_MAC_H_
#define GFX_SCOPED_CG_CONTEXT_STATE_MAC_H_

#import <QuartzCore/QuartzCore.h>

namespace gfx {

class ScopedCGContextSaveGState {
 public:
  explicit ScopedCGContextSaveGState(CGContextRef context) : context_(context) {
    CGContextSaveGState(context_);
  }

  ~ScopedCGContextSaveGState() {
    CGContextRestoreGState(context_);
  }

 private:
  CGContextRef context_;

  DISALLOW_COPY_AND_ASSIGN(ScopedCGContextSaveGState);
};

}  // namespace gfx

#endif  // GFX_SCOPED_CG_CONTEXT_STATE_MAC_H_
