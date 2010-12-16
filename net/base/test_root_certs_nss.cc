// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/test_root_certs.h"

#include <cert.h>

#include "base/logging.h"
#include "base/nss_util.h"
#include "base/stl_util-inl.h"
#include "net/base/x509_certificate.h"

namespace net {

// TrustEntry is used to store the original CERTCertificate and CERTCertTrust
// for a certificate whose trust status has been changed by the
// TestRootCerts.
class TestRootCerts::TrustEntry {
 public:
  // Creates a new TrustEntry by incrementing the reference to |certificate|
  // and copying |trust|.
  TrustEntry(CERTCertificate* certificate, CERTCertTrust trust);
  ~TrustEntry();

  CERTCertificate* certificate() const { return certificate_; }
  CERTCertTrust trust() const { return trust_; }

 private:
  // The temporary root certificate.
  CERTCertificate* certificate_;

  // The original trust settings, before |certificate_| was manipulated to
  // be a temporarily trusted root.
  CERTCertTrust trust_;

  DISALLOW_COPY_AND_ASSIGN(TrustEntry);
};

TestRootCerts::TrustEntry::TrustEntry(CERTCertificate* certificate,
                                      CERTCertTrust trust)
    : certificate_(CERT_DupCertificate(certificate)),
      trust_(trust) {
}

TestRootCerts::TrustEntry::~TrustEntry() {
  CERT_DestroyCertificate(certificate_);
}

bool TestRootCerts::Add(X509Certificate* certificate) {
  // Preserve the original trust bits so that they can be restored when
  // the certificate is removed.
  CERTCertTrust original_trust;
  SECStatus rv = CERT_GetCertTrust(certificate->os_cert_handle(),
                                   &original_trust);
  if (rv != SECSuccess) {
    // CERT_GetCertTrust will fail if the certificate does not have any
    // particular trust settings associated with it, and attempts to use
    // |original_trust| later to restore the original trust settings will not
    // cause the trust settings to be revoked. If the certificate has no
    // particular trust settings associated with it, mark the certificate as
    // a valid CA certificate with no specific trust.
    rv = CERT_DecodeTrustString(&original_trust, "c,c,c");
  }

  // Change the trust bits to unconditionally trust this certificate.
  CERTCertTrust new_trust;
  rv = CERT_DecodeTrustString(&new_trust, "TCu,Cu,Tu");
  if (rv != SECSuccess) {
    LOG(ERROR) << "Cannot decode certificate trust string.";
    return false;
  }

  rv = CERT_ChangeCertTrust(CERT_GetDefaultCertDB(),
                            certificate->os_cert_handle(),
                            &new_trust);
  if (rv != SECSuccess) {
    LOG(ERROR) << "Cannot change certificate trust.";
    return false;
  }

  trust_cache_.push_back(new TrustEntry(certificate->os_cert_handle(),
                                        original_trust));
  return true;
}

void TestRootCerts::Clear() {
  // Restore the certificate trusts to what they were originally, before
  // Add() was called. Work from the rear first, since if a certificate was
  // added twice, the second entry's original trust status will be that of
  // the first entry, while the first entry contains the desired resultant
  // status.
  for (std::list<TrustEntry*>::reverse_iterator it = trust_cache_.rbegin();
       it != trust_cache_.rend(); ++it) {
    CERTCertTrust original_trust = (*it)->trust();
    SECStatus rv = CERT_ChangeCertTrust(CERT_GetDefaultCertDB(),
                                        (*it)->certificate(),
                                        &original_trust);
    // DCHECK(), rather than LOG(), as a failure to restore the original
    // trust can cause flake or hard-to-trace errors in any unit tests that
    // occur after Clear() has been called.
    DCHECK_EQ(SECSuccess, rv) << "Cannot restore certificate trust.";
  }
  STLDeleteElements(&trust_cache_);
}

bool TestRootCerts::IsEmpty() const {
  return trust_cache_.empty();
}

TestRootCerts::~TestRootCerts() {
  Clear();
}

void TestRootCerts::Init() {
  base::EnsureNSSInit();
}

}  // namespace net
