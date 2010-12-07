// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/ssl_cipher_suite_names.h"

#include <stdlib.h>

#include "base/logging.h"
#include "net/base/ssl_connection_status_flags.h"

// Rather than storing the names of all the ciphersuites we eliminate the
// redundancy and break each cipher suite into a key exchange method, cipher
// and mac. For all the ciphersuites in the IANA registry, we extract each of
// those components from the name, number them and pack the result into a
// 16-bit number thus:
//   (MSB to LSB)
//   <4 bits> unused
//   <5 bits> key exchange
//   <4 bits> cipher
//   <3 bits> mac

// The following tables were generated by ssl_cipher_suite_names_generate.go,
// found in the same directory as this file.

struct CipherSuite {
  uint16 cipher_suite, encoded;
};

static const struct CipherSuite kCipherSuites[] = {
  {0x0, 0x0},  // TLS_NULL_WITH_NULL_NULL
  {0x1, 0x81},  // TLS_RSA_WITH_NULL_MD5
  {0x2, 0x82},  // TLS_RSA_WITH_NULL_SHA
  {0x3, 0x109},  // TLS_RSA_EXPORT_WITH_RC4_40_MD5
  {0x4, 0x91},  // TLS_RSA_WITH_RC4_128_MD5
  {0x5, 0x92},  // TLS_RSA_WITH_RC4_128_SHA
  {0x6, 0x119},  // TLS_RSA_EXPORT_WITH_RC2_CBC_40_MD5
  {0x7, 0xa2},  // TLS_RSA_WITH_IDEA_CBC_SHA
  {0x8, 0x12a},  // TLS_RSA_EXPORT_WITH_DES40_CBC_SHA
  {0x9, 0xb2},  // TLS_RSA_WITH_DES_CBC_SHA
  {0xa, 0xba},  // TLS_RSA_WITH_3DES_EDE_CBC_SHA
  {0xb, 0x1aa},  // TLS_DH_DSS_EXPORT_WITH_DES40_CBC_SHA
  {0xc, 0x232},  // TLS_DH_DSS_WITH_DES_CBC_SHA
  {0xd, 0x23a},  // TLS_DH_DSS_WITH_3DES_EDE_CBC_SHA
  {0xe, 0x2aa},  // TLS_DH_RSA_EXPORT_WITH_DES40_CBC_SHA
  {0xf, 0x332},  // TLS_DH_RSA_WITH_DES_CBC_SHA
  {0x10, 0x33a},  // TLS_DH_RSA_WITH_3DES_EDE_CBC_SHA
  {0x11, 0x3aa},  // TLS_DHE_DSS_EXPORT_WITH_DES40_CBC_SHA
  {0x12, 0x432},  // TLS_DHE_DSS_WITH_DES_CBC_SHA
  {0x13, 0x43a},  // TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA
  {0x14, 0x4aa},  // TLS_DHE_RSA_EXPORT_WITH_DES40_CBC_SHA
  {0x15, 0x532},  // TLS_DHE_RSA_WITH_DES_CBC_SHA
  {0x16, 0x53a},  // TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA
  {0x17, 0x589},  // TLS_DH_anon_EXPORT_WITH_RC4_40_MD5
  {0x18, 0x611},  // TLS_DH_anon_WITH_RC4_128_MD5
  {0x19, 0x5aa},  // TLS_DH_anon_EXPORT_WITH_DES40_CBC_SHA
  {0x1a, 0x632},  // TLS_DH_anon_WITH_DES_CBC_SHA
  {0x1b, 0x63a},  // TLS_DH_anon_WITH_3DES_EDE_CBC_SHA
  {0x1e, 0x6b2},  // TLS_KRB5_WITH_DES_CBC_SHA
  {0x1f, 0x6ba},  // TLS_KRB5_WITH_3DES_EDE_CBC_SHA
  {0x20, 0x692},  // TLS_KRB5_WITH_RC4_128_SHA
  {0x21, 0x6a2},  // TLS_KRB5_WITH_IDEA_CBC_SHA
  {0x22, 0x6b1},  // TLS_KRB5_WITH_DES_CBC_MD5
  {0x23, 0x6b9},  // TLS_KRB5_WITH_3DES_EDE_CBC_MD5
  {0x24, 0x691},  // TLS_KRB5_WITH_RC4_128_MD5
  {0x25, 0x6a1},  // TLS_KRB5_WITH_IDEA_CBC_MD5
  {0x26, 0x742},  // TLS_KRB5_EXPORT_WITH_DES_CBC_40_SHA
  {0x27, 0x71a},  // TLS_KRB5_EXPORT_WITH_RC2_CBC_40_SHA
  {0x28, 0x70a},  // TLS_KRB5_EXPORT_WITH_RC4_40_SHA
  {0x29, 0x741},  // TLS_KRB5_EXPORT_WITH_DES_CBC_40_MD5
  {0x2a, 0x719},  // TLS_KRB5_EXPORT_WITH_RC2_CBC_40_MD5
  {0x2b, 0x709},  // TLS_KRB5_EXPORT_WITH_RC4_40_MD5
  {0x2c, 0x782},  // TLS_PSK_WITH_NULL_SHA
  {0x2d, 0x802},  // TLS_DHE_PSK_WITH_NULL_SHA
  {0x2e, 0x882},  // TLS_RSA_PSK_WITH_NULL_SHA
  {0x2f, 0xca},  // TLS_RSA_WITH_AES_128_CBC_SHA
  {0x30, 0x24a},  // TLS_DH_DSS_WITH_AES_128_CBC_SHA
  {0x31, 0x34a},  // TLS_DH_RSA_WITH_AES_128_CBC_SHA
  {0x32, 0x44a},  // TLS_DHE_DSS_WITH_AES_128_CBC_SHA
  {0x33, 0x54a},  // TLS_DHE_RSA_WITH_AES_128_CBC_SHA
  {0x34, 0x64a},  // TLS_DH_anon_WITH_AES_128_CBC_SHA
  {0x35, 0xd2},  // TLS_RSA_WITH_AES_256_CBC_SHA
  {0x36, 0x252},  // TLS_DH_DSS_WITH_AES_256_CBC_SHA
  {0x37, 0x352},  // TLS_DH_RSA_WITH_AES_256_CBC_SHA
  {0x38, 0x452},  // TLS_DHE_DSS_WITH_AES_256_CBC_SHA
  {0x39, 0x552},  // TLS_DHE_RSA_WITH_AES_256_CBC_SHA
  {0x3a, 0x652},  // TLS_DH_anon_WITH_AES_256_CBC_SHA
  {0x3b, 0x83},  // TLS_RSA_WITH_NULL_SHA256
  {0x3c, 0xcb},  // TLS_RSA_WITH_AES_128_CBC_SHA256
  {0x3d, 0xd3},  // TLS_RSA_WITH_AES_256_CBC_SHA256
  {0x3e, 0x24b},  // TLS_DH_DSS_WITH_AES_128_CBC_SHA256
  {0x3f, 0x34b},  // TLS_DH_RSA_WITH_AES_128_CBC_SHA256
  {0x40, 0x44b},  // TLS_DHE_DSS_WITH_AES_128_CBC_SHA256
  {0x41, 0xda},  // TLS_RSA_WITH_CAMELLIA_128_CBC_SHA
  {0x42, 0x25a},  // TLS_DH_DSS_WITH_CAMELLIA_128_CBC_SHA
  {0x43, 0x35a},  // TLS_DH_RSA_WITH_CAMELLIA_128_CBC_SHA
  {0x44, 0x45a},  // TLS_DHE_DSS_WITH_CAMELLIA_128_CBC_SHA
  {0x45, 0x55a},  // TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA
  {0x46, 0x65a},  // TLS_DH_anon_WITH_CAMELLIA_128_CBC_SHA
  {0x67, 0x54b},  // TLS_DHE_RSA_WITH_AES_128_CBC_SHA256
  {0x68, 0x253},  // TLS_DH_DSS_WITH_AES_256_CBC_SHA256
  {0x69, 0x353},  // TLS_DH_RSA_WITH_AES_256_CBC_SHA256
  {0x6a, 0x453},  // TLS_DHE_DSS_WITH_AES_256_CBC_SHA256
  {0x6b, 0x553},  // TLS_DHE_RSA_WITH_AES_256_CBC_SHA256
  {0x6c, 0x64b},  // TLS_DH_anon_WITH_AES_128_CBC_SHA256
  {0x6d, 0x653},  // TLS_DH_anon_WITH_AES_256_CBC_SHA256
  {0x84, 0xe2},  // TLS_RSA_WITH_CAMELLIA_256_CBC_SHA
  {0x85, 0x262},  // TLS_DH_DSS_WITH_CAMELLIA_256_CBC_SHA
  {0x86, 0x362},  // TLS_DH_RSA_WITH_CAMELLIA_256_CBC_SHA
  {0x87, 0x462},  // TLS_DHE_DSS_WITH_CAMELLIA_256_CBC_SHA
  {0x88, 0x562},  // TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA
  {0x89, 0x662},  // TLS_DH_anon_WITH_CAMELLIA_256_CBC_SHA
  {0x8a, 0x792},  // TLS_PSK_WITH_RC4_128_SHA
  {0x8b, 0x7ba},  // TLS_PSK_WITH_3DES_EDE_CBC_SHA
  {0x8c, 0x7ca},  // TLS_PSK_WITH_AES_128_CBC_SHA
  {0x8d, 0x7d2},  // TLS_PSK_WITH_AES_256_CBC_SHA
  {0x8e, 0x812},  // TLS_DHE_PSK_WITH_RC4_128_SHA
  {0x8f, 0x83a},  // TLS_DHE_PSK_WITH_3DES_EDE_CBC_SHA
  {0x90, 0x84a},  // TLS_DHE_PSK_WITH_AES_128_CBC_SHA
  {0x91, 0x852},  // TLS_DHE_PSK_WITH_AES_256_CBC_SHA
  {0x92, 0x892},  // TLS_RSA_PSK_WITH_RC4_128_SHA
  {0x93, 0x8ba},  // TLS_RSA_PSK_WITH_3DES_EDE_CBC_SHA
  {0x94, 0x8ca},  // TLS_RSA_PSK_WITH_AES_128_CBC_SHA
  {0x95, 0x8d2},  // TLS_RSA_PSK_WITH_AES_256_CBC_SHA
  {0x96, 0xea},  // TLS_RSA_WITH_SEED_CBC_SHA
  {0x97, 0x26a},  // TLS_DH_DSS_WITH_SEED_CBC_SHA
  {0x98, 0x36a},  // TLS_DH_RSA_WITH_SEED_CBC_SHA
  {0x99, 0x46a},  // TLS_DHE_DSS_WITH_SEED_CBC_SHA
  {0x9a, 0x56a},  // TLS_DHE_RSA_WITH_SEED_CBC_SHA
  {0x9b, 0x66a},  // TLS_DH_anon_WITH_SEED_CBC_SHA
  {0x9c, 0xf3},  // TLS_RSA_WITH_AES_128_GCM_SHA256
  {0x9d, 0xfc},  // TLS_RSA_WITH_AES_256_GCM_SHA384
  {0x9e, 0x573},  // TLS_DHE_RSA_WITH_AES_128_GCM_SHA256
  {0x9f, 0x57c},  // TLS_DHE_RSA_WITH_AES_256_GCM_SHA384
  {0xa0, 0x373},  // TLS_DH_RSA_WITH_AES_128_GCM_SHA256
  {0xa1, 0x37c},  // TLS_DH_RSA_WITH_AES_256_GCM_SHA384
  {0xa2, 0x473},  // TLS_DHE_DSS_WITH_AES_128_GCM_SHA256
  {0xa3, 0x47c},  // TLS_DHE_DSS_WITH_AES_256_GCM_SHA384
  {0xa4, 0x273},  // TLS_DH_DSS_WITH_AES_128_GCM_SHA256
  {0xa5, 0x27c},  // TLS_DH_DSS_WITH_AES_256_GCM_SHA384
  {0xa6, 0x673},  // TLS_DH_anon_WITH_AES_128_GCM_SHA256
  {0xa7, 0x67c},  // TLS_DH_anon_WITH_AES_256_GCM_SHA384
  {0xa8, 0x7f3},  // TLS_PSK_WITH_AES_128_GCM_SHA256
  {0xa9, 0x7fc},  // TLS_PSK_WITH_AES_256_GCM_SHA384
  {0xaa, 0x873},  // TLS_DHE_PSK_WITH_AES_128_GCM_SHA256
  {0xab, 0x87c},  // TLS_DHE_PSK_WITH_AES_256_GCM_SHA384
  {0xac, 0x8f3},  // TLS_RSA_PSK_WITH_AES_128_GCM_SHA256
  {0xad, 0x8fc},  // TLS_RSA_PSK_WITH_AES_256_GCM_SHA384
  {0xae, 0x7cb},  // TLS_PSK_WITH_AES_128_CBC_SHA256
  {0xaf, 0x7d4},  // TLS_PSK_WITH_AES_256_CBC_SHA384
  {0xb0, 0x783},  // TLS_PSK_WITH_NULL_SHA256
  {0xb1, 0x784},  // TLS_PSK_WITH_NULL_SHA384
  {0xb2, 0x84b},  // TLS_DHE_PSK_WITH_AES_128_CBC_SHA256
  {0xb3, 0x854},  // TLS_DHE_PSK_WITH_AES_256_CBC_SHA384
  {0xb4, 0x803},  // TLS_DHE_PSK_WITH_NULL_SHA256
  {0xb5, 0x804},  // TLS_DHE_PSK_WITH_NULL_SHA384
  {0xb6, 0x8cb},  // TLS_RSA_PSK_WITH_AES_128_CBC_SHA256
  {0xb7, 0x8d4},  // TLS_RSA_PSK_WITH_AES_256_CBC_SHA384
  {0xb8, 0x883},  // TLS_RSA_PSK_WITH_NULL_SHA256
  {0xb9, 0x884},  // TLS_RSA_PSK_WITH_NULL_SHA384
  {0xba, 0xdb},  // TLS_RSA_WITH_CAMELLIA_128_CBC_SHA256
  {0xbb, 0x25b},  // TLS_DH_DSS_WITH_CAMELLIA_128_CBC_SHA256
  {0xbc, 0x35b},  // TLS_DH_RSA_WITH_CAMELLIA_128_CBC_SHA256
  {0xbd, 0x45b},  // TLS_DHE_DSS_WITH_CAMELLIA_128_CBC_SHA256
  {0xbe, 0x55b},  // TLS_DHE_RSA_WITH_CAMELLIA_128_CBC_SHA256
  {0xbf, 0x65b},  // TLS_DH_anon_WITH_CAMELLIA_128_CBC_SHA256
  {0xc0, 0xe3},  // TLS_RSA_WITH_CAMELLIA_256_CBC_SHA256
  {0xc1, 0x263},  // TLS_DH_DSS_WITH_CAMELLIA_256_CBC_SHA256
  {0xc2, 0x363},  // TLS_DH_RSA_WITH_CAMELLIA_256_CBC_SHA256
  {0xc3, 0x463},  // TLS_DHE_DSS_WITH_CAMELLIA_256_CBC_SHA256
  {0xc4, 0x563},  // TLS_DHE_RSA_WITH_CAMELLIA_256_CBC_SHA256
  {0xc5, 0x663},  // TLS_DH_anon_WITH_CAMELLIA_256_CBC_SHA256
  {0xc001, 0x902},  // TLS_ECDH_ECDSA_WITH_NULL_SHA
  {0xc002, 0x912},  // TLS_ECDH_ECDSA_WITH_RC4_128_SHA
  {0xc003, 0x93a},  // TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA
  {0xc004, 0x94a},  // TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA
  {0xc005, 0x952},  // TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA
  {0xc006, 0x982},  // TLS_ECDHE_ECDSA_WITH_NULL_SHA
  {0xc007, 0x992},  // TLS_ECDHE_ECDSA_WITH_RC4_128_SHA
  {0xc008, 0x9ba},  // TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA
  {0xc009, 0x9ca},  // TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA
  {0xc00a, 0x9d2},  // TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA
  {0xc00b, 0xa02},  // TLS_ECDH_RSA_WITH_NULL_SHA
  {0xc00c, 0xa12},  // TLS_ECDH_RSA_WITH_RC4_128_SHA
  {0xc00d, 0xa3a},  // TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA
  {0xc00e, 0xa4a},  // TLS_ECDH_RSA_WITH_AES_128_CBC_SHA
  {0xc00f, 0xa52},  // TLS_ECDH_RSA_WITH_AES_256_CBC_SHA
  {0xc010, 0xa82},  // TLS_ECDHE_RSA_WITH_NULL_SHA
  {0xc011, 0xa92},  // TLS_ECDHE_RSA_WITH_RC4_128_SHA
  {0xc012, 0xaba},  // TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA
  {0xc013, 0xaca},  // TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA
  {0xc014, 0xad2},  // TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA
  {0xc015, 0xb02},  // TLS_ECDH_anon_WITH_NULL_SHA
  {0xc016, 0xb12},  // TLS_ECDH_anon_WITH_RC4_128_SHA
  {0xc017, 0xb3a},  // TLS_ECDH_anon_WITH_3DES_EDE_CBC_SHA
  {0xc018, 0xb4a},  // TLS_ECDH_anon_WITH_AES_128_CBC_SHA
  {0xc019, 0xb52},  // TLS_ECDH_anon_WITH_AES_256_CBC_SHA
  {0xc01a, 0xbba},  // TLS_SRP_SHA_WITH_3DES_EDE_CBC_SHA
  {0xc01b, 0xc3a},  // TLS_SRP_SHA_RSA_WITH_3DES_EDE_CBC_SHA
  {0xc01c, 0xcba},  // TLS_SRP_SHA_DSS_WITH_3DES_EDE_CBC_SHA
  {0xc01d, 0xbca},  // TLS_SRP_SHA_WITH_AES_128_CBC_SHA
  {0xc01e, 0xc4a},  // TLS_SRP_SHA_RSA_WITH_AES_128_CBC_SHA
  {0xc01f, 0xcca},  // TLS_SRP_SHA_DSS_WITH_AES_128_CBC_SHA
  {0xc020, 0xbd2},  // TLS_SRP_SHA_WITH_AES_256_CBC_SHA
  {0xc021, 0xc52},  // TLS_SRP_SHA_RSA_WITH_AES_256_CBC_SHA
  {0xc022, 0xcd2},  // TLS_SRP_SHA_DSS_WITH_AES_256_CBC_SHA
  {0xc023, 0x9cb},  // TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256
  {0xc024, 0x9d4},  // TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384
  {0xc025, 0x94b},  // TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256
  {0xc026, 0x954},  // TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384
  {0xc027, 0xacb},  // TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256
  {0xc028, 0xad4},  // TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384
  {0xc029, 0xa4b},  // TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256
  {0xc02a, 0xa54},  // TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384
  {0xc02b, 0x9f3},  // TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256
  {0xc02c, 0x9fc},  // TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384
  {0xc02d, 0x973},  // TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256
  {0xc02e, 0x97c},  // TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384
  {0xc02f, 0xaf3},  // TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
  {0xc030, 0xafc},  // TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
  {0xc031, 0xa73},  // TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256
  {0xc032, 0xa7c},  // TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384
  {0xc033, 0xd12},  // TLS_ECDHE_PSK_WITH_RC4_128_SHA
  {0xc034, 0xd3a},  // TLS_ECDHE_PSK_WITH_3DES_EDE_CBC_SHA
  {0xc035, 0xd4a},  // TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA
  {0xc036, 0xd52},  // TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA
  {0xc037, 0xd4b},  // TLS_ECDHE_PSK_WITH_AES_128_CBC_SHA256
  {0xc038, 0xd54},  // TLS_ECDHE_PSK_WITH_AES_256_CBC_SHA384
  {0xc039, 0xd02},  // TLS_ECDHE_PSK_WITH_NULL_SHA
  {0xc03a, 0xd03},  // TLS_ECDHE_PSK_WITH_NULL_SHA256
  {0xc03b, 0xd04},  // TLS_ECDHE_PSK_WITH_NULL_SHA384
};

static const struct {
  char name[15];
} kKeyExchangeNames[27] = {
  {"NULL"},  // 0
  {"RSA"},  // 1
  {"RSA_EXPORT"},  // 2
  {"DH_DSS_EXPORT"},  // 3
  {"DH_DSS"},  // 4
  {"DH_RSA_EXPORT"},  // 5
  {"DH_RSA"},  // 6
  {"DHE_DSS_EXPORT"},  // 7
  {"DHE_DSS"},  // 8
  {"DHE_RSA_EXPORT"},  // 9
  {"DHE_RSA"},  // 10
  {"DH_anon_EXPORT"},  // 11
  {"DH_anon"},  // 12
  {"KRB5"},  // 13
  {"KRB5_EXPORT"},  // 14
  {"PSK"},  // 15
  {"DHE_PSK"},  // 16
  {"RSA_PSK"},  // 17
  {"ECDH_ECDSA"},  // 18
  {"ECDHE_ECDSA"},  // 19
  {"ECDH_RSA"},  // 20
  {"ECDHE_RSA"},  // 21
  {"ECDH_anon"},  // 22
  {"SRP_SHA"},  // 23
  {"SRP_SHA_RSA"},  // 24
  {"SRP_SHA_DSS"},  // 25
  {"ECDHE_PSK"},  // 26
};

static const struct {
  char name[17];
} kCipherNames[16] = {
  {"NULL"},  // 0
  {"RC4_40"},  // 1
  {"RC4_128"},  // 2
  {"RC2_CBC_40"},  // 3
  {"IDEA_CBC"},  // 4
  {"DES40_CBC"},  // 5
  {"DES_CBC"},  // 6
  {"3DES_EDE_CBC"},  // 7
  {"DES_CBC_40"},  // 8
  {"AES_128_CBC"},  // 9
  {"AES_256_CBC"},  // 10
  {"CAMELLIA_128_CBC"},  // 11
  {"CAMELLIA_256_CBC"},  // 12
  {"SEED_CBC"},  // 13
  {"AES_128_GCM"},  // 14
  {"AES_256_GCM"},  // 15
};

static const struct {
  char name[7];
} kMacNames[5] = {
  {"NULL"},  // 0
  {"MD5"},  // 1
  {"SHA1"},  // 2
  {"SHA256"},  // 3
  {"SHA384"},  // 4
};


namespace net {

static int CipherSuiteCmp(const void* ia, const void* ib) {
  const CipherSuite* a = static_cast<const CipherSuite*>(ia);
  const CipherSuite* b = static_cast<const CipherSuite*>(ib);

  if (a->cipher_suite < b->cipher_suite) {
    return -1;
  } else if (a->cipher_suite == b->cipher_suite) {
    return 0;
  } else {
    return 1;
  }
}

void SSLCipherSuiteToStrings(const char** key_exchange_str,
                             const char** cipher_str,
                             const char** mac_str, uint16 cipher_suite) {
  *key_exchange_str = *cipher_str = *mac_str = "???";

  struct CipherSuite desired = {0};
  desired.cipher_suite = cipher_suite;

  void* r = bsearch(&desired, kCipherSuites,
                    arraysize(kCipherSuites), sizeof(kCipherSuites[0]),
                    CipherSuiteCmp);

  if (!r)
    return;

  const CipherSuite* cs = static_cast<CipherSuite*>(r);

  const int key_exchange = cs->encoded >> 7;
  const int cipher = (cs->encoded >> 3) & 0xf;
  const int mac = cs->encoded & 0x7;

  *key_exchange_str = kKeyExchangeNames[key_exchange].name;
  *cipher_str = kCipherNames[cipher].name;
  *mac_str = kMacNames[mac].name;
}

void SSLCompressionToString(const char** name, uint8 compresssion) {
  if (compresssion == 0) {
    *name = "NONE";
  } else if (compresssion == 1) {
    *name = "DEFLATE";
  } else if (compresssion == 64) {
    *name = "LZS";
  } else {
    *name = "???";
  }
}

void SSLVersionToString(const char** name, int ssl_version) {
  switch (ssl_version) {
    case SSL_CONNECTION_VERSION_SSL2:
      *name = "SSL 2.0";
      break;
    case SSL_CONNECTION_VERSION_SSL3:
      *name = "SSL 3.0";
      break;
    case SSL_CONNECTION_VERSION_TLS1:
      *name = "TLS 1.0";
      break;
    case SSL_CONNECTION_VERSION_TLS1_1:
      *name = "TLS 1.1";
      break;
    case SSL_CONNECTION_VERSION_TLS1_2:
      *name = "TLS 1.2";
      break;
    default:
      NOTREACHED() << ssl_version;
      *name = "???";
      break;
  }
}

}  // namespace net
