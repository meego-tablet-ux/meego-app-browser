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

#ifndef NET_BASE_X509_CERTIFICATE_H_
#define NET_BASE_X509_CERTIFICATE_H_

#include <windows.h>
#include <wincrypt.h>

#include <set>
#include <string>
#include <vector>

#include "base/ref_counted.h"
#include "base/time.h"

class Pickle;

namespace net {

// X509Certificate represents an X.509 certificate used by SSL.
class X509Certificate : public base::RefCountedThreadSafe<X509Certificate> {
 public:
  // SHA-1 fingerprint (160 bits) of a certificate.
  struct Fingerprint {
    unsigned char data[20];
  };

  class FingerprintLessThan
      : public std::binary_function<Fingerprint, Fingerprint, bool> {
   public:
    bool operator() (const Fingerprint& lhs, const Fingerprint& rhs) const;
  };

  // Predicate functor used in maps when X509Certificate is used as the key.
  class LessThan
      : public std::binary_function<X509Certificate*, X509Certificate*, bool> {
   public:
    bool operator() (X509Certificate* lhs,  X509Certificate* rhs) const;
  };

  typedef PCCERT_CONTEXT OSCertHandle;

  // Principal represent an X.509 principal.
  struct Principal {
    Principal() { }
    explicit Principal(std::string name) : common_name(name) { }

    // The different attributes for a principal.  They may be "".
    // Note that some of them can have several values.

    std::string common_name;
    std::string locality_name;
    std::string state_or_province_name;
    std::string country_name;

    std::vector<std::string> street_addresses;
    std::vector<std::string> organization_names;
    std::vector<std::string> organization_unit_names;
    std::vector<std::string> domain_components;
  };

  // This class is useful for maintaining policies about which certificates are
  // permitted or forbidden for a particular purpose.
  class Policy {
   public:
    // The judgments this policy can reach.
    enum Judgment {
      // We don't have policy information for this certificate.
      UNKNOWN,

      // This certificate is allowed.
      ALLOWED,

      // This certificate is denied.
      DENIED,
    };

    // Returns the judgment this policy makes about this certificate.
    Judgment Check(X509Certificate* cert) const;

    // Causes the policy to allow this certificate.
    void Allow(X509Certificate* cert);

    // Causes the policy to deny this certificate.
    void Deny(X509Certificate* cert);

   private:
    // The set of fingerprints of allowed certificates.
    std::set<Fingerprint, FingerprintLessThan> allowed_;

    // The set of fingerprints of denied certificates.
    std::set<Fingerprint, FingerprintLessThan> denied_;
  };

  // Create an X509Certificate from a handle to the certificate object
  // in the underlying crypto library.
  static X509Certificate* CreateFromHandle(OSCertHandle cert_handle);

  // Create an X509Certificate from the representation stored in the given
  // pickle.  The data for this object is found relative to the given
  // pickle_iter, which should be passed to the pickle's various Read* methods.
  static X509Certificate* CreateFromPickle(const Pickle& pickle,
                                           void** pickle_iter);

  // Creates a X509Certificate from the ground up.  Used by tests that simulate
  // SSL connections.
  X509Certificate(std::string subject, std::string issuer,
                  Time start_date, Time expiration_date);

  // Appends a representation of this object to the given pickle.
  void Persist(Pickle* pickle);

  // The subject of the certificate.  For HTTPS server certificates, this
  // represents the web server.  The common name of the subject should match
  // the host name of the web server.
  const Principal& subject() const { return subject_; }

  // The issuer of the certificate.
  const Principal& issuer() const { return issuer_; }

  // Time period during which the certificate is valid.  More precisely, this
  // certificate is invalid before the |valid_start| date and invalid after
  // the |valid_expiry| date.
  // If we were unable to parse either date from the certificate (or if the cert
  // lacks either date), the date will be null (i.e., is_null() will be true).
  const Time& valid_start() const { return valid_start_; }
  const Time& valid_expiry() const { return valid_expiry_; }

  // The fingerprint of this certificate.
  const Fingerprint& fingerprint() const { return fingerprint_; }

  // Gets the DNS names in the certificate.  Pursuant to RFC 2818, Section 3.1
  // Server Identity, if the certificate has a subjectAltName extension of
  // type dNSName, this method gets the DNS names in that extension.
  // Otherwise, it gets the common name in the subject field.
  void GetDNSNames(std::vector<std::string>* dns_names) const;

  // Convenience method that returns whether this certificate has expired as of
  // now.
  bool HasExpired() const;

  // Returns true if the certificate is an extended-validation (EV)
  // certificate.
  bool IsEV(int cert_status) const;

  OSCertHandle os_cert_handle() const { return cert_handle_; }

 private:
  // A cache of X509Certificate objects.
  class Cache;

  // Construct an X509Certificate from a handle to the certificate object
  // in the underlying crypto library.
  explicit X509Certificate(OSCertHandle cert_handle);

  friend RefCountedThreadSafe<X509Certificate>;
  ~X509Certificate();

  // Common object initialization code.  Called by the constructors only.
  void Initialize();

  // Helper function to parse a principal from a WinInet description of that
  // principal.
  static void ParsePrincipal(const std::string& description,
                             Principal* principal);

  // The subject of the certificate.
  Principal subject_;

  // The issuer of the certificate.
  Principal issuer_;

  // This certificate is not valid before |valid_start_|
  Time valid_start_;

  // This certificate is not valid after |valid_expiry_|
  Time valid_expiry_;

  // The fingerprint of this certificate.
  Fingerprint fingerprint_;

  // A handle to the certificate object in the underlying crypto library.
  OSCertHandle cert_handle_;

  DISALLOW_EVIL_CONSTRUCTORS(X509Certificate);
};

}  // namespace net

#endif  // NET_BASE_X509_CERTIFICATE_H_
