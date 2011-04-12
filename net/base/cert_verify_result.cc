// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/cert_verify_result.h"

namespace net {

CertVerifyResult::CertVerifyResult() {
  Reset();
}

void CertVerifyResult::Reset() {
  cert_status = 0;
  has_md5 = false;
  has_md2 = false;
  has_md4 = false;
  has_md5_ca = false;
  has_md2_ca = false;
  is_issued_by_known_root = false;
  public_key_hashes.clear();
}

} // namespace net
