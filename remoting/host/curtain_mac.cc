// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/curtain.h"

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/logging.h"

namespace remoting {

namespace {

class CurtainMac : public Curtain {
 public:
  CurtainMac() {}
  virtual void EnableCurtainMode(bool enable) OVERRIDE;

 private:
  DISALLOW_COPY_AND_ASSIGN(CurtainMac);
};

void CurtainMac::EnableCurtainMode(bool enable) {
  NOTIMPLEMENTED();
}

}  // namespace

Curtain* Curtain::Create() {
  return new CurtainMac();
}

}  // namespace remoting
