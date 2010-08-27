// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/dnssec_chain_verifier.h"
#include "net/base/dnssec_keyset.h"
#include "net/base/dns_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class SignatureVerifierDNSSECTest : public testing::Test {
};

class DNSSECChainVerifierTest : public testing::Test {
};

}  // anonymous namespace

// This is a DNSKEY record. You can get one with `dig dnskey .` (where '.' can
// be any signed zone).
static const unsigned char kExampleKey[] = {
 0x01, 0x01, 0x03, 0x08, 0x03, 0x01, 0x00, 0x01, 0xa8, 0x00, 0x20, 0xa9, 0x55,
 0x66, 0xba, 0x42, 0xe8, 0x86, 0xbb, 0x80, 0x4c, 0xda, 0x84, 0xe4, 0x7e, 0xf5,
 0x6d, 0xbd, 0x7a, 0xec, 0x61, 0x26, 0x15, 0x55, 0x2c, 0xec, 0x90, 0x6d, 0x21,
 0x16, 0xd0, 0xef, 0x20, 0x70, 0x28, 0xc5, 0x15, 0x54, 0x14, 0x4d, 0xfe, 0xaf,
 0xe7, 0xc7, 0xcb, 0x8f, 0x00, 0x5d, 0xd1, 0x82, 0x34, 0x13, 0x3a, 0xc0, 0x71,
 0x0a, 0x81, 0x18, 0x2c, 0xe1, 0xfd, 0x14, 0xad, 0x22, 0x83, 0xbc, 0x83, 0x43,
 0x5f, 0x9d, 0xf2, 0xf6, 0x31, 0x32, 0x51, 0x93, 0x1a, 0x17, 0x6d, 0xf0, 0xda,
 0x51, 0xe5, 0x4f, 0x42, 0xe6, 0x04, 0x86, 0x0d, 0xfb, 0x35, 0x95, 0x80, 0x25,
 0x0f, 0x55, 0x9c, 0xc5, 0x43, 0xc4, 0xff, 0xd5, 0x1c, 0xbe, 0x3d, 0xe8, 0xcf,
 0xd0, 0x67, 0x19, 0x23, 0x7f, 0x9f, 0xc4, 0x7e, 0xe7, 0x29, 0xda, 0x06, 0x83,
 0x5f, 0xa4, 0x52, 0xe8, 0x25, 0xe9, 0xa1, 0x8e, 0xbc, 0x2e, 0xcb, 0xcf, 0x56,
 0x34, 0x74, 0x65, 0x2c, 0x33, 0xcf, 0x56, 0xa9, 0x03, 0x3b, 0xcd, 0xf5, 0xd9,
 0x73, 0x12, 0x17, 0x97, 0xec, 0x80, 0x89, 0x04, 0x1b, 0x6e, 0x03, 0xa1, 0xb7,
 0x2d, 0x0a, 0x73, 0x5b, 0x98, 0x4e, 0x03, 0x68, 0x73, 0x09, 0x33, 0x23, 0x24,
 0xf2, 0x7c, 0x2d, 0xba, 0x85, 0xe9, 0xdb, 0x15, 0xe8, 0x3a, 0x01, 0x43, 0x38,
 0x2e, 0x97, 0x4b, 0x06, 0x21, 0xc1, 0x8e, 0x62, 0x5e, 0xce, 0xc9, 0x07, 0x57,
 0x7d, 0x9e, 0x7b, 0xad, 0xe9, 0x52, 0x41, 0xa8, 0x1e, 0xbb, 0xe8, 0xa9, 0x01,
 0xd4, 0xd3, 0x27, 0x6e, 0x40, 0xb1, 0x14, 0xc0, 0xa2, 0xe6, 0xfc, 0x38, 0xd1,
 0x9c, 0x2e, 0x6a, 0xab, 0x02, 0x64, 0x4b, 0x28, 0x13, 0xf5, 0x75, 0xfc, 0x21,
 0x60, 0x1e, 0x0d, 0xee, 0x49, 0xcd, 0x9e, 0xe9, 0x6a, 0x43, 0x10, 0x3e, 0x52,
 0x4d, 0x62, 0x87, 0x3d,
};

TEST(SignatureVerifierDNSSECTest, KeyID) {
  uint16 keyid = net::DNSSECKeySet::DNSKEYToKeyID(
      base::StringPiece(reinterpret_cast<const char*>(kExampleKey),
                        sizeof(kExampleKey)));
  ASSERT_EQ(19036u, keyid);
}

TEST(SignatureVerifierDNSSECTest, ImportKey) {
  net::DNSSECKeySet keyset;

  ASSERT_TRUE(keyset.AddKey(
     base::StringPiece(reinterpret_cast<const char*>(kExampleKey),
                       sizeof(kExampleKey))));
}

// This is root's DNSSKEY signature
const unsigned char kSignatureData[] = {
 0x08, 0x00, 0x00, 0x01, 0x51, 0x80, 0x4c, 0x59, 0xfe, 0xff, 0x4c, 0x46, 0x38,
 0x80, 0x4a, 0x5c, 0x2e, 0x63, 0xb7, 0x71, 0xf6, 0xb0, 0x32, 0xb7, 0x71, 0xb5,
 0xaa, 0xca, 0xd4, 0x4c, 0xeb, 0xf1, 0x7f, 0xa3, 0x28, 0x00, 0x43, 0x85, 0x76,
 0x9f, 0x46, 0x3d, 0x85, 0x39, 0x8d, 0x66, 0x7a, 0xeb, 0x4c, 0x6e, 0x60, 0x0a,
 0xe6, 0x49, 0x20, 0x47, 0xf9, 0x13, 0x81, 0x3e, 0xc8, 0xf5, 0x09, 0x92, 0x75,
 0xab, 0x87, 0xc6, 0x70, 0x35, 0xdd, 0xd9, 0x9e, 0xb6, 0xd4, 0x09, 0x66, 0xcc,
 0x0d, 0x71, 0x71, 0x3d, 0x4c, 0x96, 0xbc, 0x7d, 0x2c, 0x05, 0x0a, 0x9c, 0xc3,
 0xcd, 0x5d, 0xfa, 0xab, 0xb5, 0x17, 0x55, 0xca, 0x86, 0x31, 0x4b, 0x7b, 0x93,
 0x4a, 0x4b, 0x91, 0xd9, 0xea, 0x71, 0xf8, 0x3f, 0xb3, 0x4f, 0xb4, 0x94, 0xcd,
 0x6f, 0xe4, 0x83, 0xf6, 0xd4, 0xcb, 0xb8, 0x3c, 0x7d, 0xf6, 0x73, 0xf7, 0xf2,
 0x6f, 0xa5, 0x92, 0x25, 0xdc, 0xe5, 0xdd, 0x83, 0x55, 0xef, 0xde, 0x20, 0x00,
 0x64, 0x9e, 0x25, 0x76, 0x70, 0x08, 0x14, 0x29, 0xec, 0x66, 0xa3, 0xfd, 0x48,
 0x3b, 0x67, 0x21, 0x6e, 0x3d, 0x1e, 0x26, 0xb4, 0x74, 0x07, 0x1f, 0x1f, 0x4d,
 0xdf, 0x74, 0xae, 0x04, 0x70, 0xf0, 0x08, 0x3f, 0xe2, 0x6a, 0x39, 0x51, 0x79,
 0x25, 0xd9, 0xc2, 0xf9, 0xa4, 0xb6, 0x38, 0x4a, 0x5f, 0x80, 0x12, 0x4d, 0x98,
 0x7a, 0x3b, 0x8d, 0xb8, 0x3d, 0x51, 0x6b, 0x7c, 0x27, 0xc9, 0xc0, 0xcc, 0x26,
 0x73, 0xef, 0x43, 0x8a, 0x6c, 0x42, 0xa5, 0x2d, 0x11, 0x77, 0x9f, 0xe4, 0xa4,
 0x50, 0xb3, 0x29, 0xe4, 0x5c, 0x04, 0xc7, 0x38, 0xbb, 0xfa, 0x27, 0xfa, 0x02,
 0x76, 0x07, 0x5b, 0x88, 0x39, 0xd8, 0x60, 0x81, 0x9f, 0x36, 0xfc, 0x9c, 0x17,
 0x83, 0x0a, 0x54, 0x59, 0x86, 0x6b, 0xd6, 0x54, 0x5c, 0x9a, 0xba, 0x10, 0xe6,
 0x2e, 0x12, 0x78, 0x85, 0x1c, 0xed, 0x26, 0x79, 0xd4, 0xfc, 0x83, 0x51,
};

// The is root's 1024-bit key.
static const unsigned char kRRDATA1[] = {
  1, 0, 3, 8, 3, 1, 0, 1, 189, 96, 112, 56, 65, 148, 127, 253, 50, 88, 20, 197,
  45, 34, 147, 103, 112, 99, 242, 98, 4, 138, 85, 248, 72, 74, 101, 94, 203,
  113, 204, 77, 115, 164, 37, 143, 142, 187, 66, 49, 208, 220, 88, 38, 218, 45,
  139, 19, 220, 58, 46, 163, 197, 13, 41, 224, 230, 165, 106, 212, 230, 5, 201,
  48, 109, 220, 52, 41, 166, 160, 231, 0, 250, 226, 79, 5, 185, 168, 132, 13,
  12, 209, 111, 223, 140, 168, 235, 123, 0, 116, 23, 148, 224, 111, 109, 191,
  183, 115, 79, 155, 15, 200, 8, 38, 86, 30, 71, 12, 39, 190, 233, 115, 54,
  248, 135, 165, 215, 233, 20, 40, 39, 165, 240, 135, 50, 215, 216, 81,
};

// The is root's 2048-bit key.
static const unsigned char kRRDATA2[] = {
  1, 1, 3, 8, 3, 1, 0, 1, 168, 0, 32, 169, 85, 102, 186, 66, 232, 134, 187,
  128, 76, 218, 132, 228, 126, 245, 109, 189, 122, 236, 97, 38, 21, 85, 44,
  236, 144, 109, 33, 22, 208, 239, 32, 112, 40, 197, 21, 84, 20, 77, 254, 175,
  231, 199, 203, 143, 0, 93, 209, 130, 52, 19, 58, 192, 113, 10, 129, 24, 44,
  225, 253, 20, 173, 34, 131, 188, 131, 67, 95, 157, 242, 246, 49, 50, 81, 147,
  26, 23, 109, 240, 218, 81, 229, 79, 66, 230, 4, 134, 13, 251, 53, 149, 128,
  37, 15, 85, 156, 197, 67, 196, 255, 213, 28, 190, 61, 232, 207, 208, 103, 25,
  35, 127, 159, 196, 126, 231, 41, 218, 6, 131, 95, 164, 82, 232, 37, 233, 161,
  142, 188, 46, 203, 207, 86, 52, 116, 101, 44, 51, 207, 86, 169, 3, 59, 205,
  245, 217, 115, 18, 23, 151, 236, 128, 137, 4, 27, 110, 3, 161, 183, 45, 10,
  115, 91, 152, 78, 3, 104, 115, 9, 51, 35, 36, 242, 124, 45, 186, 133, 233,
  219, 21, 232, 58, 1, 67, 56, 46, 151, 75, 6, 33, 193, 142, 98, 94, 206, 201,
  7, 87, 125, 158, 123, 173, 233, 82, 65, 168, 30, 187, 232, 169, 1, 212, 211,
  39, 110, 64, 177, 20, 192, 162, 230, 252, 56, 209, 156, 46, 106, 171, 2, 100,
  75, 40, 19, 245, 117, 252, 33, 96, 30, 13, 238, 73, 205, 158, 233, 106, 67,
  16, 62, 82, 77, 98, 135, 61,
};

TEST(SignatureVerifierDNSSECTest, VerifySignature) {
  net::DNSSECKeySet keyset;

  ASSERT_TRUE(keyset.AddKey(
     base::StringPiece(reinterpret_cast<const char*>(kExampleKey),
                       sizeof(kExampleKey))));
  keyset.IgnoreTimestamps();

  static const uint16 kDNSKEY = 48;  // RRTYPE for DNSKEY
  static const char kRootLabel[] = "";
  base::StringPiece root(kRootLabel, 1);
  base::StringPiece signature(reinterpret_cast<const char*>(kSignatureData),
                              sizeof(kSignatureData));
  std::vector<base::StringPiece> rrdatas;
  rrdatas.push_back(base::StringPiece(reinterpret_cast<const char*>(kRRDATA1),
                                      sizeof(kRRDATA1)));
  rrdatas.push_back(base::StringPiece(reinterpret_cast<const char*>(kRRDATA2),
                                      sizeof(kRRDATA2)));
  ASSERT_TRUE(keyset.CheckSignature(root, root, signature, kDNSKEY, rrdatas));
}

static std::string FromDNSName(const char* name) {
  std::string result;
  bool ok = net::DNSDomainFromDot(name, &result);
  EXPECT_TRUE(ok);
  if (!ok)
    result = "";
  return result;
}

TEST(DNSSECChainVerifierTest, MatchingLabels) {
  ASSERT_EQ(1u, net::DNSSECChainVerifier::MatchingLabels(
        FromDNSName(""), FromDNSName("")));
  ASSERT_EQ(2u, net::DNSSECChainVerifier::MatchingLabels(
        FromDNSName("org"), FromDNSName("org")));
  ASSERT_EQ(3u, net::DNSSECChainVerifier::MatchingLabels(
        FromDNSName("foo.org"), FromDNSName("foo.org")));
  ASSERT_EQ(3u, net::DNSSECChainVerifier::MatchingLabels(
        FromDNSName("bar.foo.org"), FromDNSName("foo.org")));
  ASSERT_EQ(3u, net::DNSSECChainVerifier::MatchingLabels(
        FromDNSName("foo.org"), FromDNSName("bar.foo.org")));
  ASSERT_EQ(1u, net::DNSSECChainVerifier::MatchingLabels(
        FromDNSName("foo.org"), FromDNSName("foo.com")));
  ASSERT_EQ(2u, net::DNSSECChainVerifier::MatchingLabels(
        FromDNSName("foo.org"), FromDNSName("bar.org")));
}

static const unsigned char kChain[] = {
  0x4a, 0x5c, 0x01, 0x01, 0x10, 0x08, 0x00, 0x00, 0x01, 0x51, 0x80, 0x4c, 0x59,
  0xfe, 0xff, 0x4c, 0x46, 0x38, 0x80, 0x4a, 0x5c, 0x2e, 0x63, 0xb7, 0x71, 0xf6,
  0xb0, 0x32, 0xb7, 0x71, 0xb5, 0xaa, 0xca, 0xd4, 0x4c, 0xeb, 0xf1, 0x7f, 0xa3,
  0x28, 0x00, 0x43, 0x85, 0x76, 0x9f, 0x46, 0x3d, 0x85, 0x39, 0x8d, 0x66, 0x7a,
  0xeb, 0x4c, 0x6e, 0x60, 0x0a, 0xe6, 0x49, 0x20, 0x47, 0xf9, 0x13, 0x81, 0x3e,
  0xc8, 0xf5, 0x09, 0x92, 0x75, 0xab, 0x87, 0xc6, 0x70, 0x35, 0xdd, 0xd9, 0x9e,
  0xb6, 0xd4, 0x09, 0x66, 0xcc, 0x0d, 0x71, 0x71, 0x3d, 0x4c, 0x96, 0xbc, 0x7d,
  0x2c, 0x05, 0x0a, 0x9c, 0xc3, 0xcd, 0x5d, 0xfa, 0xab, 0xb5, 0x17, 0x55, 0xca,
  0x86, 0x31, 0x4b, 0x7b, 0x93, 0x4a, 0x4b, 0x91, 0xd9, 0xea, 0x71, 0xf8, 0x3f,
  0xb3, 0x4f, 0xb4, 0x94, 0xcd, 0x6f, 0xe4, 0x83, 0xf6, 0xd4, 0xcb, 0xb8, 0x3c,
  0x7d, 0xf6, 0x73, 0xf7, 0xf2, 0x6f, 0xa5, 0x92, 0x25, 0xdc, 0xe5, 0xdd, 0x83,
  0x55, 0xef, 0xde, 0x20, 0x00, 0x64, 0x9e, 0x25, 0x76, 0x70, 0x08, 0x14, 0x29,
  0xec, 0x66, 0xa3, 0xfd, 0x48, 0x3b, 0x67, 0x21, 0x6e, 0x3d, 0x1e, 0x26, 0xb4,
  0x74, 0x07, 0x1f, 0x1f, 0x4d, 0xdf, 0x74, 0xae, 0x04, 0x70, 0xf0, 0x08, 0x3f,
  0xe2, 0x6a, 0x39, 0x51, 0x79, 0x25, 0xd9, 0xc2, 0xf9, 0xa4, 0xb6, 0x38, 0x4a,
  0x5f, 0x80, 0x12, 0x4d, 0x98, 0x7a, 0x3b, 0x8d, 0xb8, 0x3d, 0x51, 0x6b, 0x7c,
  0x27, 0xc9, 0xc0, 0xcc, 0x26, 0x73, 0xef, 0x43, 0x8a, 0x6c, 0x42, 0xa5, 0x2d,
  0x11, 0x77, 0x9f, 0xe4, 0xa4, 0x50, 0xb3, 0x29, 0xe4, 0x5c, 0x04, 0xc7, 0x38,
  0xbb, 0xfa, 0x27, 0xfa, 0x02, 0x76, 0x07, 0x5b, 0x88, 0x39, 0xd8, 0x60, 0x81,
  0x9f, 0x36, 0xfc, 0x9c, 0x17, 0x83, 0x0a, 0x54, 0x59, 0x86, 0x6b, 0xd6, 0x54,
  0x5c, 0x9a, 0xba, 0x10, 0xe6, 0x2e, 0x12, 0x78, 0x85, 0x1c, 0xed, 0x26, 0x79,
  0xd4, 0xfc, 0x83, 0x51, 0x02, 0x00, 0x88, 0x01, 0x00, 0x03, 0x08, 0x03, 0x01,
  0x00, 0x01, 0xbd, 0x60, 0x70, 0x38, 0x41, 0x94, 0x7f, 0xfd, 0x32, 0x58, 0x14,
  0xc5, 0x2d, 0x22, 0x93, 0x67, 0x70, 0x63, 0xf2, 0x62, 0x04, 0x8a, 0x55, 0xf8,
  0x48, 0x4a, 0x65, 0x5e, 0xcb, 0x71, 0xcc, 0x4d, 0x73, 0xa4, 0x25, 0x8f, 0x8e,
  0xbb, 0x42, 0x31, 0xd0, 0xdc, 0x58, 0x26, 0xda, 0x2d, 0x8b, 0x13, 0xdc, 0x3a,
  0x2e, 0xa3, 0xc5, 0x0d, 0x29, 0xe0, 0xe6, 0xa5, 0x6a, 0xd4, 0xe6, 0x05, 0xc9,
  0x30, 0x6d, 0xdc, 0x34, 0x29, 0xa6, 0xa0, 0xe7, 0x00, 0xfa, 0xe2, 0x4f, 0x05,
  0xb9, 0xa8, 0x84, 0x0d, 0x0c, 0xd1, 0x6f, 0xdf, 0x8c, 0xa8, 0xeb, 0x7b, 0x00,
  0x74, 0x17, 0x94, 0xe0, 0x6f, 0x6d, 0xbf, 0xb7, 0x73, 0x4f, 0x9b, 0x0f, 0xc8,
  0x08, 0x26, 0x56, 0x1e, 0x47, 0x0c, 0x27, 0xbe, 0xe9, 0x73, 0x36, 0xf8, 0x87,
  0xa5, 0xd7, 0xe9, 0x14, 0x28, 0x27, 0xa5, 0xf0, 0x87, 0x32, 0xd7, 0xd8, 0x51,
  0x01, 0x08, 0x01, 0x01, 0x03, 0x08, 0x03, 0x01, 0x00, 0x01, 0xa8, 0x00, 0x20,
  0xa9, 0x55, 0x66, 0xba, 0x42, 0xe8, 0x86, 0xbb, 0x80, 0x4c, 0xda, 0x84, 0xe4,
  0x7e, 0xf5, 0x6d, 0xbd, 0x7a, 0xec, 0x61, 0x26, 0x15, 0x55, 0x2c, 0xec, 0x90,
  0x6d, 0x21, 0x16, 0xd0, 0xef, 0x20, 0x70, 0x28, 0xc5, 0x15, 0x54, 0x14, 0x4d,
  0xfe, 0xaf, 0xe7, 0xc7, 0xcb, 0x8f, 0x00, 0x5d, 0xd1, 0x82, 0x34, 0x13, 0x3a,
  0xc0, 0x71, 0x0a, 0x81, 0x18, 0x2c, 0xe1, 0xfd, 0x14, 0xad, 0x22, 0x83, 0xbc,
  0x83, 0x43, 0x5f, 0x9d, 0xf2, 0xf6, 0x31, 0x32, 0x51, 0x93, 0x1a, 0x17, 0x6d,
  0xf0, 0xda, 0x51, 0xe5, 0x4f, 0x42, 0xe6, 0x04, 0x86, 0x0d, 0xfb, 0x35, 0x95,
  0x80, 0x25, 0x0f, 0x55, 0x9c, 0xc5, 0x43, 0xc4, 0xff, 0xd5, 0x1c, 0xbe, 0x3d,
  0xe8, 0xcf, 0xd0, 0x67, 0x19, 0x23, 0x7f, 0x9f, 0xc4, 0x7e, 0xe7, 0x29, 0xda,
  0x06, 0x83, 0x5f, 0xa4, 0x52, 0xe8, 0x25, 0xe9, 0xa1, 0x8e, 0xbc, 0x2e, 0xcb,
  0xcf, 0x56, 0x34, 0x74, 0x65, 0x2c, 0x33, 0xcf, 0x56, 0xa9, 0x03, 0x3b, 0xcd,
  0xf5, 0xd9, 0x73, 0x12, 0x17, 0x97, 0xec, 0x80, 0x89, 0x04, 0x1b, 0x6e, 0x03,
  0xa1, 0xb7, 0x2d, 0x0a, 0x73, 0x5b, 0x98, 0x4e, 0x03, 0x68, 0x73, 0x09, 0x33,
  0x23, 0x24, 0xf2, 0x7c, 0x2d, 0xba, 0x85, 0xe9, 0xdb, 0x15, 0xe8, 0x3a, 0x01,
  0x43, 0x38, 0x2e, 0x97, 0x4b, 0x06, 0x21, 0xc1, 0x8e, 0x62, 0x5e, 0xce, 0xc9,
  0x07, 0x57, 0x7d, 0x9e, 0x7b, 0xad, 0xe9, 0x52, 0x41, 0xa8, 0x1e, 0xbb, 0xe8,
  0xa9, 0x01, 0xd4, 0xd3, 0x27, 0x6e, 0x40, 0xb1, 0x14, 0xc0, 0xa2, 0xe6, 0xfc,
  0x38, 0xd1, 0x9c, 0x2e, 0x6a, 0xab, 0x02, 0x64, 0x4b, 0x28, 0x13, 0xf5, 0x75,
  0xfc, 0x21, 0x60, 0x1e, 0x0d, 0xee, 0x49, 0xcd, 0x9e, 0xe9, 0x6a, 0x43, 0x10,
  0x3e, 0x52, 0x4d, 0x62, 0x87, 0x3d, 0x03, 0x6f, 0x72, 0x67, 0x00, 0x00, 0x2b,
  0x00, 0x90, 0x08, 0x01, 0x00, 0x02, 0xa3, 0x00, 0x4c, 0x54, 0xb9, 0x00, 0x4c,
  0x4b, 0x70, 0x70, 0xa1, 0x20, 0x5c, 0x2f, 0xd0, 0x29, 0x2c, 0x13, 0xf8, 0xbc,
  0xbb, 0xfe, 0xd2, 0xb2, 0xf2, 0x6a, 0xfa, 0xe0, 0x4a, 0x2e, 0x80, 0xb5, 0x3c,
  0xb9, 0xa1, 0x6f, 0x14, 0x57, 0x9e, 0x80, 0x3f, 0xb8, 0x5d, 0xbc, 0xac, 0x81,
  0x1f, 0x7a, 0xf9, 0x18, 0xda, 0x31, 0x3f, 0x97, 0x68, 0x2b, 0x37, 0x79, 0xb2,
  0xb8, 0xd7, 0x1a, 0x40, 0x35, 0xb1, 0x3b, 0x64, 0x22, 0x0d, 0xb6, 0xb9, 0x17,
  0x64, 0xfe, 0xed, 0x23, 0xaf, 0x17, 0xb2, 0x7f, 0x49, 0xa9, 0x2b, 0xa0, 0x06,
  0xfb, 0xe9, 0x4a, 0x17, 0x88, 0x91, 0x55, 0xe5, 0xb3, 0x5f, 0xee, 0x73, 0xd7,
  0x00, 0x58, 0x21, 0x29, 0x39, 0x7a, 0xe2, 0xc5, 0x03, 0x8f, 0xf2, 0x11, 0xed,
  0xd6, 0xb0, 0xdb, 0x5e, 0x7f, 0x37, 0x46, 0x80, 0xf9, 0x67, 0xfc, 0xdd, 0xbf,
  0xb5, 0x46, 0x4d, 0xe5, 0xcf, 0x3f, 0xf9, 0xb2, 0x08, 0x52, 0x36, 0xf5, 0x7f,
  0x8c, 0x19, 0x88, 0x02, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x02, 0x01, 0x10,
  0x07, 0x01, 0x00, 0x00, 0x03, 0x84, 0x4c, 0x54, 0x44, 0xe0, 0x4c, 0x41, 0xc1,
  0xd0, 0x53, 0x76, 0x53, 0x1b, 0xac, 0x55, 0xb7, 0x5b, 0xc8, 0xca, 0xe7, 0x05,
  0xe2, 0x19, 0x50, 0x5b, 0xf4, 0x05, 0xc6, 0x11, 0x41, 0xb7, 0xde, 0x29, 0xd8,
  0x71, 0x9b, 0x1d, 0x44, 0x8b, 0x1f, 0xf9, 0x9d, 0x4e, 0x94, 0xc9, 0xe9, 0xfe,
  0xc2, 0x7c, 0x73, 0xd1, 0xc7, 0x9b, 0xbe, 0x15, 0x8f, 0xc4, 0xb6, 0xe7, 0x91,
  0xe4, 0x67, 0xc3, 0xab, 0x2f, 0x89, 0x60, 0xf0, 0x29, 0xfb, 0xb1, 0x0a, 0x05,
  0xc6, 0xf2, 0xf6, 0xb3, 0xf2, 0x4d, 0x68, 0xb5, 0xde, 0x66, 0x45, 0x39, 0xe8,
  0x9b, 0xfa, 0x0f, 0x7d, 0x9d, 0xbc, 0xd4, 0xc5, 0xb7, 0x14, 0x9d, 0xa0, 0x21,
  0x33, 0x04, 0x2a, 0x97, 0x40, 0x98, 0x40, 0x16, 0xfe, 0xe0, 0xcf, 0xdd, 0x22,
  0x94, 0xbe, 0xbc, 0x4c, 0x09, 0x5c, 0xef, 0x0e, 0x99, 0xac, 0xa0, 0xb6, 0xa5,
  0xde, 0xe8, 0xf1, 0x1f, 0xff, 0x84, 0x3a, 0x19, 0x7e, 0x46, 0x26, 0x6f, 0xdc,
  0xfd, 0x15, 0x9d, 0xfd, 0x6e, 0x58, 0xf2, 0x94, 0x02, 0x25, 0x9a, 0x37, 0x99,
  0x9f, 0x27, 0xdb, 0xbe, 0xcd, 0x3a, 0x2b, 0xa8, 0xa3, 0xd6, 0x96, 0x4e, 0xdb,
  0xe5, 0x87, 0x55, 0xd8, 0xd2, 0x6e, 0xa7, 0x09, 0x7d, 0xe4, 0xfc, 0x20, 0x6c,
  0x4d, 0x99, 0xb3, 0x48, 0xf9, 0x63, 0xee, 0x4e, 0xa5, 0x24, 0xcc, 0xb1, 0x99,
  0xf8, 0x0e, 0x1d, 0x6e, 0x1a, 0x88, 0x0b, 0x98, 0xa5, 0x21, 0xdc, 0x43, 0x75,
  0xb5, 0x20, 0x5a, 0xcd, 0xac, 0xb8, 0x08, 0x60, 0x2c, 0xb0, 0x2f, 0x6f, 0xac,
  0x7b, 0xda, 0x16, 0x24, 0xc7, 0xc6, 0x22, 0xf6, 0xc8, 0x47, 0x8a, 0x93, 0x93,
  0x1e, 0x1d, 0xb5, 0xe2, 0x1e, 0xe5, 0xc6, 0x8e, 0xa9, 0xe4, 0xd3, 0x35, 0x97,
  0x31, 0x64, 0xce, 0xd8, 0xa1, 0x16, 0x1d, 0x67, 0x0e, 0x2c, 0x07, 0x2e, 0x67,
  0xef, 0xcc, 0x80, 0x59, 0x35, 0xdd, 0xa0, 0xa8, 0x60, 0x99, 0x4e, 0x8e, 0x04,
  0x00, 0x88, 0x01, 0x00, 0x03, 0x07, 0x03, 0x01, 0x00, 0x01, 0x9c, 0xf3, 0x56,
  0x70, 0x44, 0x1d, 0x37, 0x51, 0xf9, 0x88, 0xa4, 0xf7, 0xf9, 0x1f, 0x60, 0x1b,
  0x14, 0x03, 0xa8, 0xcd, 0xc1, 0xaf, 0x44, 0x6e, 0x5c, 0xdb, 0x83, 0x64, 0x48,
  0x9a, 0x7f, 0xdc, 0x6b, 0x64, 0x46, 0x72, 0xb5, 0xd5, 0x7f, 0xd2, 0xe9, 0x95,
  0x2f, 0x14, 0xf9, 0xff, 0x68, 0xbf, 0x35, 0xb7, 0xcc, 0x13, 0x95, 0xab, 0x07,
  0x4d, 0x5d, 0x47, 0xdb, 0xaa, 0xc0, 0xfc, 0xf9, 0xdc, 0xd4, 0x97, 0x51, 0x64,
  0xbb, 0x8a, 0x66, 0x71, 0xa8, 0x1c, 0xb8, 0x7b, 0xa4, 0xf6, 0xb9, 0x54, 0x98,
  0xf6, 0xa7, 0x41, 0xa8, 0xae, 0x8e, 0x17, 0x2b, 0x7e, 0xdd, 0x60, 0x8b, 0x84,
  0x21, 0xe4, 0x68, 0xdd, 0xfc, 0x34, 0x5b, 0x8f, 0x53, 0x14, 0x93, 0xa3, 0xb1,
  0xd6, 0xcb, 0x9f, 0x26, 0xac, 0xfd, 0x9e, 0xf3, 0x44, 0x8a, 0x35, 0xf6, 0x8b,
  0x20, 0xf8, 0x7a, 0xf7, 0xa2, 0x2c, 0xf3, 0xfd, 0x00, 0x88, 0x01, 0x00, 0x03,
  0x07, 0x03, 0x01, 0x00, 0x01, 0xc9, 0x06, 0x00, 0x53, 0x45, 0x4a, 0x0b, 0xb8,
  0xe2, 0xb0, 0x4e, 0x29, 0xc8, 0x19, 0xb4, 0xa3, 0x63, 0x27, 0xe2, 0xcd, 0xc7,
  0xc7, 0x6d, 0x60, 0x31, 0xeb, 0xc0, 0x82, 0x5f, 0x44, 0x14, 0x96, 0x60, 0x4e,
  0xc8, 0x62, 0xf4, 0xcc, 0xb9, 0x99, 0x7a, 0x19, 0xf0, 0xaf, 0x34, 0xd9, 0x63,
  0xca, 0x40, 0xe3, 0x7b, 0xb6, 0xbc, 0xfa, 0x40, 0x08, 0x1d, 0xe7, 0xc3, 0xa4,
  0xd2, 0x73, 0x3a, 0x32, 0xf2, 0xa4, 0x4c, 0x3c, 0x4f, 0xd6, 0x52, 0x52, 0xc8,
  0x6d, 0xa5, 0xf6, 0xd9, 0x4d, 0x0c, 0xfd, 0xb4, 0x93, 0x8b, 0x61, 0x72, 0xdb,
  0x6e, 0x5f, 0x2d, 0xd9, 0x2d, 0xab, 0x18, 0x2f, 0x87, 0x2d, 0xbf, 0x8d, 0x42,
  0x37, 0x93, 0x41, 0x18, 0xf6, 0x93, 0x97, 0xda, 0x27, 0x31, 0xdc, 0xda, 0xec,
  0x21, 0x16, 0x61, 0xe1, 0xe0, 0x7a, 0x53, 0x26, 0x82, 0xc7, 0x62, 0x99, 0x18,
  0x81, 0x6a, 0x65, 0x01, 0x08, 0x01, 0x01, 0x03, 0x07, 0x03, 0x01, 0x00, 0x01,
  0x8a, 0x58, 0x7e, 0x3d, 0xda, 0x69, 0x1c, 0xf3, 0x93, 0x15, 0x90, 0xa8, 0xc7,
  0x65, 0xed, 0x81, 0x31, 0x63, 0xcd, 0x4d, 0x75, 0x84, 0xaf, 0xfa, 0xa6, 0xb2,
  0xb9, 0x90, 0xe8, 0x76, 0x76, 0x7d, 0x20, 0xc8, 0x74, 0x6f, 0x03, 0x1c, 0x61,
  0xa5, 0x54, 0x77, 0x33, 0x40, 0x6d, 0x57, 0x89, 0xf2, 0x07, 0x7a, 0x8e, 0xad,
  0x6c, 0x47, 0x75, 0x6f, 0x3f, 0xf4, 0x91, 0xdf, 0xa9, 0xa6, 0x1a, 0xcb, 0x1b,
  0x57, 0x85, 0x1d, 0x97, 0x93, 0x91, 0x0e, 0xda, 0xa2, 0x64, 0xfd, 0x93, 0x0c,
  0xd0, 0xc7, 0xc4, 0x49, 0xca, 0x29, 0x35, 0xfe, 0x8d, 0x67, 0xf2, 0xb5, 0x97,
  0x93, 0xed, 0xdd, 0xc0, 0x6d, 0x2c, 0xc1, 0x28, 0x2d, 0x2f, 0xee, 0xe6, 0x6b,
  0x33, 0xa3, 0x36, 0x7a, 0x82, 0x67, 0x97, 0xa8, 0x9d, 0xeb, 0xaa, 0xc4, 0x52,
  0x64, 0x02, 0xda, 0x9f, 0x43, 0xae, 0xb0, 0xe0, 0xf4, 0x5e, 0xad, 0x5c, 0x2f,
  0x42, 0x0f, 0xfc, 0xc2, 0xef, 0xfc, 0xbe, 0x04, 0xd3, 0x69, 0x88, 0xe7, 0x67,
  0x33, 0x90, 0xd7, 0x93, 0xb1, 0xe1, 0x66, 0x6e, 0xeb, 0x6b, 0xd1, 0x3b, 0x96,
  0xd2, 0xf5, 0xde, 0x1d, 0xa6, 0xc7, 0xb9, 0x04, 0x81, 0x4f, 0x1e, 0xea, 0x7a,
  0x49, 0x94, 0x2a, 0x17, 0x8e, 0xb6, 0x88, 0x06, 0x05, 0x03, 0xb6, 0x16, 0x2c,
  0xe3, 0xc5, 0xbf, 0xb1, 0xd4, 0xc3, 0x2e, 0xee, 0xcd, 0xe7, 0xda, 0xe3, 0x08,
  0x6f, 0x9b, 0xa6, 0x29, 0x7e, 0x73, 0xca, 0x19, 0xf5, 0xfe, 0xcd, 0x47, 0x7a,
  0xa6, 0x49, 0x3a, 0x53, 0x3f, 0x59, 0xbc, 0xe9, 0x1a, 0x94, 0x42, 0x75, 0x44,
  0xae, 0x27, 0xeb, 0x1f, 0xc2, 0xa3, 0x0e, 0xa2, 0xfe, 0xdf, 0x0c, 0xd4, 0x74,
  0x5e, 0x40, 0x0a, 0x46, 0x30, 0xb7, 0x55, 0xe1, 0x3d, 0x53, 0xd4, 0xfb, 0x04,
  0x88, 0x97, 0x36, 0xda, 0x01, 0x03, 0x78, 0xf4, 0xf5, 0x01, 0x08, 0x01, 0x01,
  0x03, 0x07, 0x03, 0x01, 0x00, 0x01, 0x94, 0xe3, 0x6c, 0x83, 0xb9, 0x90, 0x8a,
  0x71, 0x59, 0x4b, 0x72, 0x5d, 0xcf, 0x1a, 0xbe, 0xc2, 0xb2, 0x1c, 0x82, 0x19,
  0xf8, 0xb8, 0xc2, 0xd8, 0x3b, 0xfc, 0x9d, 0xa3, 0xbe, 0x4f, 0x3e, 0x97, 0xd9,
  0xfa, 0xb3, 0x0c, 0x2d, 0x5b, 0x76, 0xae, 0xc7, 0x95, 0x9c, 0x2d, 0x91, 0xaa,
  0x93, 0x90, 0xc5, 0x55, 0x27, 0xef, 0x20, 0x13, 0xd1, 0x48, 0xad, 0xe1, 0x89,
  0xe1, 0xcf, 0x06, 0xd4, 0x67, 0x5b, 0x8d, 0x08, 0x1b, 0x3f, 0x53, 0xb2, 0x60,
  0x81, 0xbb, 0x38, 0x74, 0xdc, 0xe2, 0x1b, 0xf9, 0x4f, 0x63, 0x65, 0xc9, 0x6a,
  0xfa, 0x93, 0xa4, 0x05, 0xcf, 0xdf, 0x10, 0xe3, 0x3c, 0x05, 0x20, 0x64, 0xc5,
  0x56, 0xfc, 0x01, 0x86, 0x6a, 0xcc, 0x0d, 0x8b, 0x0e, 0x4e, 0xd5, 0xda, 0x90,
  0xae, 0x90, 0xc0, 0x7a, 0x2f, 0x03, 0x5f, 0xbc, 0xdc, 0x1b, 0x14, 0x00, 0x2c,
  0x65, 0x89, 0xda, 0x70, 0x07, 0x48, 0x50, 0x69, 0xc6, 0xc3, 0xeb, 0x1f, 0x88,
  0xd9, 0x10, 0x83, 0xcd, 0x8b, 0x93, 0xce, 0x3e, 0xb8, 0x26, 0xfd, 0x3f, 0xf5,
  0x7b, 0x17, 0xe8, 0x06, 0x15, 0xdd, 0xe6, 0xdc, 0x82, 0x7e, 0x21, 0x2f, 0x58,
  0xc8, 0x47, 0x67, 0x89, 0x63, 0x25, 0xe5, 0xac, 0x0a, 0x16, 0xc5, 0xdc, 0xf1,
  0x71, 0x6f, 0xff, 0xe7, 0x27, 0x8b, 0xe5, 0x15, 0x56, 0xba, 0x14, 0x71, 0x7a,
  0x39, 0xa7, 0x49, 0x59, 0xc2, 0xbb, 0x19, 0x1f, 0x4b, 0x80, 0x10, 0xe3, 0xce,
  0x4a, 0x1f, 0x6b, 0x69, 0x75, 0xb5, 0x9c, 0x0a, 0x8a, 0x4b, 0x25, 0x9b, 0x3a,
  0xb7, 0x0f, 0x2a, 0xde, 0x35, 0x9c, 0xa5, 0x31, 0xb3, 0x76, 0x1f, 0xef, 0xdf,
  0x17, 0x58, 0x7c, 0xda, 0x50, 0x35, 0xc3, 0xc8, 0x98, 0x59, 0x71, 0x02, 0xe9,
  0xf7, 0x06, 0xd3, 0x91, 0x3c, 0x0d, 0xab, 0xf2, 0xd8, 0xba, 0x30, 0xda, 0x09,
  0x10, 0x75, 0x0a, 0x64, 0x6e, 0x73, 0x73, 0x65, 0x63, 0x2d, 0x65, 0x78, 0x70,
  0x03, 0x6f, 0x72, 0x67, 0x00, 0x00, 0x2b, 0x00, 0x90, 0x07, 0x02, 0x00, 0x01,
  0x51, 0x80, 0x4c, 0x54, 0x44, 0xe0, 0x4c, 0x41, 0xc1, 0xd0, 0xf2, 0x12, 0x0d,
  0x95, 0xb4, 0x6f, 0x7c, 0xfc, 0xde, 0x67, 0xa0, 0x1d, 0x84, 0x49, 0x3e, 0xa8,
  0xe1, 0x07, 0xea, 0xde, 0x75, 0x14, 0x98, 0xa9, 0xcc, 0x09, 0x20, 0x66, 0x59,
  0xd2, 0x40, 0xfd, 0xc6, 0xcf, 0x54, 0x22, 0x4a, 0x13, 0xf9, 0xc2, 0x1e, 0xe3,
  0x48, 0xf1, 0xd0, 0x6d, 0x18, 0x27, 0xe5, 0x8e, 0x92, 0xc2, 0x5f, 0xcc, 0xb8,
  0xd6, 0x1a, 0xbb, 0xfa, 0xe7, 0xac, 0xa8, 0x23, 0x4b, 0x89, 0xef, 0xb0, 0xc7,
  0x9a, 0xaa, 0x00, 0x99, 0x82, 0x3d, 0xa8, 0x25, 0x6b, 0xb3, 0x59, 0xd5, 0x7a,
  0x90, 0xba, 0xc4, 0xa4, 0x79, 0x8c, 0xa4, 0x39, 0xc2, 0xe9, 0x54, 0xc0, 0x4c,
  0x0a, 0x0e, 0x06, 0xcc, 0x25, 0xac, 0xb6, 0x75, 0x45, 0x8c, 0xd9, 0x99, 0x4f,
  0x31, 0xff, 0x3d, 0x40, 0x3b, 0x1c, 0x40, 0x94, 0xed, 0x9d, 0x7f, 0x60, 0x08,
  0x23, 0x63, 0x18, 0x5c, 0x50, 0x6f, 0x26, 0x7b, 0x39, 0x8b, 0x01, 0x02, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xa8, 0x01, 0x00, 0x03, 0x05, 0x03, 0x01,
  0x00, 0x01, 0xd0, 0x0a, 0x64, 0x8e, 0xb5, 0x90, 0x0b, 0x75, 0xc2, 0xeb, 0x52,
  0x5f, 0xa4, 0xcb, 0xeb, 0x1d, 0x77, 0x8d, 0x84, 0x2c, 0xef, 0xb6, 0x88, 0xd4,
  0x7c, 0x50, 0x4c, 0x52, 0x7b, 0x9d, 0x37, 0x31, 0x36, 0xb2, 0x5b, 0x6c, 0x47,
  0xb4, 0x21, 0x80, 0x61, 0x46, 0xfa, 0x5b, 0x44, 0x50, 0x91, 0x9d, 0xf8, 0xc1,
  0x78, 0x00, 0x78, 0x29, 0xfe, 0xe2, 0x08, 0x65, 0xf8, 0xc9, 0xdf, 0x69, 0x0b,
  0x59, 0x6b, 0xf4, 0x93, 0x9f, 0x8b, 0x25, 0x0b, 0x6b, 0x93, 0x12, 0x06, 0x57,
  0xc8, 0x04, 0x9d, 0x3f, 0x33, 0xbd, 0x2b, 0x35, 0x54, 0xb9, 0x98, 0x75, 0xe4,
  0xb0, 0x49, 0x3c, 0x29, 0xc1, 0xfb, 0x74, 0xbc, 0x91, 0x82, 0x9e, 0xc5, 0x61,
  0xa6, 0x16, 0xe1, 0xf0, 0x8f, 0xe9, 0xe1, 0x23, 0x55, 0x5e, 0xfb, 0xf1, 0xcc,
  0x8a, 0x75, 0x5d, 0xf2, 0x86, 0x89, 0x0a, 0x29, 0xcb, 0xca, 0x33, 0xde, 0x9d,
  0x74, 0x43, 0x0d, 0xde, 0x67, 0xde, 0x71, 0x0f, 0x7f, 0xa3, 0xbb, 0x22, 0x82,
  0x81, 0x66, 0xd1, 0xbd, 0x49, 0x0d, 0x89, 0x45, 0xd7, 0x18, 0xcf, 0x98, 0x2c,
  0x19, 0xaf, 0x63, 0x16, 0x20, 0x91, 0x0a, 0x64, 0x6e, 0x73, 0x73, 0x65, 0x63,
  0x2d, 0x65, 0x78, 0x70, 0x03, 0x6f, 0x72, 0x67, 0x00, 0x00, 0x25, 0x00, 0xb0,
  0x05, 0x02, 0x00, 0x00, 0x00, 0x3c, 0x4c, 0x71, 0x48, 0x0a, 0x4c, 0x49, 0xb6,
  0xcb, 0x83, 0x7f, 0x3d, 0xfa, 0xd9, 0x59, 0x7a, 0xc5, 0xa2, 0xd3, 0x0e, 0x3e,
  0x48, 0xa8, 0x9b, 0xc5, 0xef, 0x43, 0xe3, 0x3e, 0x3a, 0x1a, 0xb2, 0x39, 0x00,
  0x01, 0x73, 0x78, 0x80, 0x26, 0xac, 0xb0, 0x01, 0xe9, 0xc9, 0x7a, 0x0f, 0x5e,
  0xa7, 0xa2, 0x7f, 0xec, 0xa8, 0xf4, 0x42, 0x58, 0xdc, 0xba, 0xc1, 0x9e, 0xc8,
  0xa8, 0xc7, 0x93, 0x9f, 0x13, 0xc5, 0xbe, 0x99, 0xa9, 0xcc, 0x75, 0xd6, 0x7b,
  0xb4, 0xd4, 0x9b, 0x0e, 0x61, 0x96, 0x6d, 0x3f, 0xbf, 0x47, 0x42, 0xae, 0x8c,
  0xd7, 0xc1, 0x09, 0xb3, 0x84, 0x50, 0xac, 0xae, 0xbd, 0xc4, 0x54, 0x77, 0xe0,
  0xfe, 0xa8, 0xc3, 0xd4, 0x68, 0x76, 0x07, 0xdb, 0x14, 0x51, 0x2e, 0x43, 0x15,
  0x2b, 0x67, 0x4b, 0x76, 0x18, 0xbd, 0x04, 0xb3, 0xb7, 0xe5, 0xb7, 0xaf, 0xaa,
  0xb1, 0x4c, 0xa6, 0xd2, 0xbe, 0x2d, 0xdb, 0xa5, 0x2c, 0x1d, 0xe7, 0x67, 0x79,
  0x0b, 0xc4, 0x2e, 0x26, 0x0f, 0x9d, 0x98, 0x1f, 0x21, 0x1f, 0x9e, 0xfe, 0x3f,
  0x67, 0xf3, 0x67, 0x73, 0xe5, 0x88, 0xdf, 0xa8, 0x9f, 0x8c, 0x74, 0x97, 0xc2,
  0x44, 0x29, 0x28, 0x15, 0x0f, 0x1f, 0x83, 0x01, 0x00, 0x19, 0x00, 0x09, 0x00,
  0x00, 0x00, 0x90, 0x38, 0x61, 0x31, 0x19, 0x52, 0x21, 0x14, 0x83, 0x08, 0x44,
  0xeb, 0xdd, 0xe5, 0xc7, 0x1d, 0x16, 0xa2, 0x45, 0x1c,
};

TEST(DNSSECChainVerifierTest, TestChain) {
  base::StringPiece chain(reinterpret_cast<const char*>(kChain),
                          sizeof(kChain));
  net::DNSSECChainVerifier verifier(FromDNSName("dnssec-exp.org"), chain);
  verifier.IgnoreTimestamps();
  ASSERT_EQ(net::DNSSECChainVerifier::OK, verifier.Verify());
  ASSERT_EQ(net::kDNS_CERT, verifier.rrtype());
}

TEST(DNSSECChainVerifierTest, OffCourse) {
  base::StringPiece chain(reinterpret_cast<const char*>(kChain),
                          sizeof(kChain));
  net::DNSSECChainVerifier verifier(FromDNSName("foo.org"), chain);
  verifier.IgnoreTimestamps();
  ASSERT_EQ(net::DNSSECChainVerifier::OFF_COURSE, verifier.Verify());
}

TEST(DNSSECChainVerifierTest, BadTarget) {
  base::StringPiece chain(reinterpret_cast<const char*>(kChain),
                          sizeof(kChain));
  net::DNSSECChainVerifier verifier(FromDNSName("www.dnssec-exp.org"), chain);
  verifier.IgnoreTimestamps();
  ASSERT_EQ(net::DNSSECChainVerifier::BAD_TARGET, verifier.Verify());
}

// This is too slow to run all the time.
TEST(DNSSECChainVerifierTest, DISABLED_Fuzz) {
  uint8 copy[sizeof(kChain)];
  base::StringPiece chain(reinterpret_cast<const char*>(copy), sizeof(copy));
  static const unsigned bit_length = sizeof(kChain) * 8;

  for (unsigned bit_to_flip = 0; bit_to_flip < bit_length; bit_to_flip++) {
    memcpy(copy, kChain, sizeof(copy));

    unsigned byte = bit_to_flip >> 3;
    unsigned bit = bit_to_flip & 7;
    copy[byte] ^= (1 << bit);

    net::DNSSECChainVerifier verifier(FromDNSName("dnssec-exp.org"), chain);
    verifier.IgnoreTimestamps();
    ASSERT_NE(net::DNSSECChainVerifier::OK, verifier.Verify());
  }
}
