// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/dnsrr_resolver.h"

#include "base/callback.h"
#include "base/condition_variable.h"
#include "base/lock.h"
#include "net/base/dns_util.h"
#include "net/base/net_errors.h"
#include "net/base/test_completion_callback.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

class DnsRRResolverTest : public testing::Test {
};

#if defined(OS_LINUX)

class Rendezvous : public CallbackRunner<Tuple1<int> > {
 public:
  Rendezvous()
      : have_result_(false),
        cv_(&lock_) {
  }

  int WaitForResult() {
    lock_.Acquire();
    while (!have_result_)
      cv_.Wait();
    lock_.Release();
    return result_;
  }

  virtual void RunWithParams(const Tuple1<int>& params) {
    lock_.Acquire();
    result_ = params.a;
    have_result_ = true;
    lock_.Release();
    cv_.Broadcast();
  }

 private:
  bool have_result_;
  int result_;
  Lock lock_;
  ConditionVariable cv_;
};

// This test is disabled because it depends on the external network to pass.
// However, it may be useful when chaging the code.
TEST_F(DnsRRResolverTest, DISABLED_NetworkResolve) {
  RRResponse response;
  Rendezvous callback;
  ASSERT_TRUE(DnsRRResolver::Resolve(
        "agl._pka.imperialviolet.org", kDNS_TXT, 0, &callback, &response));
  ASSERT_EQ(OK, callback.WaitForResult());
  ASSERT_EQ(1u, response.rrdatas.size());
  ASSERT_EQ(1u, response.signatures.size());
  ASSERT_STREQ("]v=pka1;fpr=2AF0032B48E856CE06157A1AD43C670DE04AAA74;"
               "uri=http://www.imperialviolet.org/key.asc",
               response.rrdatas[0].c_str());
}

// This is a DNS packet resulting from querying a recursive resolver for a TXT
// record for agl._pka.imperialviolet.org. You should be able to get a
// replacement from a packet capture should it ever be needed.
static const uint8 kExamplePacket[] = {
  0xce, 0xfe, 0x81, 0x80, 0x00, 0x01, 0x00, 0x02, 0x00, 0x06, 0x00, 0x01, 0x03,
  0x61, 0x67, 0x6c, 0x04, 0x5f, 0x70, 0x6b, 0x61, 0x0e, 0x69, 0x6d, 0x70, 0x65,
  0x72, 0x69, 0x61, 0x6c, 0x76, 0x69, 0x6f, 0x6c, 0x65, 0x74, 0x03, 0x6f, 0x72,
  0x67, 0x00, 0x00, 0x10, 0x00, 0x01, 0xc0, 0x0c, 0x00, 0x10, 0x00, 0x01, 0x00,
  0x00, 0x01, 0x2c, 0x00, 0x5e, 0x5d, 0x76, 0x3d, 0x70, 0x6b, 0x61, 0x31, 0x3b,
  0x66, 0x70, 0x72, 0x3d, 0x32, 0x41, 0x46, 0x30, 0x30, 0x33, 0x32, 0x42, 0x34,
  0x38, 0x45, 0x38, 0x35, 0x36, 0x43, 0x45, 0x30, 0x36, 0x31, 0x35, 0x37, 0x41,
  0x31, 0x41, 0x44, 0x34, 0x33, 0x43, 0x36, 0x37, 0x30, 0x44, 0x45, 0x30, 0x34,
  0x41, 0x41, 0x41, 0x37, 0x34, 0x3b, 0x75, 0x72, 0x69, 0x3d, 0x68, 0x74, 0x74,
  0x70, 0x3a, 0x2f, 0x2f, 0x77, 0x77, 0x77, 0x2e, 0x69, 0x6d, 0x70, 0x65, 0x72,
  0x69, 0x61, 0x6c, 0x76, 0x69, 0x6f, 0x6c, 0x65, 0x74, 0x2e, 0x6f, 0x72, 0x67,
  0x2f, 0x6b, 0x65, 0x79, 0x2e, 0x61, 0x73, 0x63, 0xc0, 0x0c, 0x00, 0x2e, 0x00,
  0x01, 0x00, 0x00, 0x01, 0x2c, 0x00, 0xc6, 0x00, 0x10, 0x05, 0x04, 0x00, 0x01,
  0x51, 0x80, 0x4c, 0x74, 0x2f, 0x1a, 0x4c, 0x4c, 0x9c, 0xeb, 0x45, 0xc9, 0x0e,
  0x69, 0x6d, 0x70, 0x65, 0x72, 0x69, 0x61, 0x6c, 0x76, 0x69, 0x6f, 0x6c, 0x65,
  0x74, 0x03, 0x6f, 0x72, 0x67, 0x00, 0x3b, 0x6d, 0x3d, 0xbb, 0xae, 0x1b, 0x07,
  0x8d, 0xa9, 0xb0, 0xa7, 0xa5, 0x7a, 0x84, 0x24, 0x34, 0x29, 0x43, 0x36, 0x3f,
  0x5a, 0x48, 0x3b, 0x79, 0xa3, 0x16, 0xa4, 0x28, 0x5b, 0xd7, 0x03, 0xc6, 0x93,
  0xba, 0x4e, 0x93, 0x4d, 0x18, 0x5c, 0x98, 0xc2, 0x0d, 0x57, 0xd2, 0x6b, 0x9a,
  0x72, 0xbd, 0xe5, 0x8d, 0x10, 0x7b, 0x03, 0xe7, 0x19, 0x1e, 0x51, 0xe5, 0x7e,
  0x49, 0x6b, 0xa3, 0xa8, 0xf1, 0xd3, 0x1b, 0xff, 0x40, 0x26, 0x82, 0x65, 0xd0,
  0x74, 0x8e, 0xcf, 0xc9, 0x71, 0xea, 0x91, 0x57, 0x7e, 0x50, 0x61, 0x4d, 0x4b,
  0x77, 0x05, 0x6a, 0xd8, 0x3f, 0x12, 0x87, 0x50, 0xc2, 0x35, 0x13, 0xab, 0x01,
  0x78, 0xd2, 0x3a, 0x55, 0xa2, 0x89, 0xc8, 0x87, 0xe2, 0x7b, 0xec, 0x51, 0x7c,
  0xc0, 0x24, 0xb5, 0xa3, 0x33, 0x78, 0x98, 0x28, 0x8e, 0x9b, 0x6b, 0x88, 0x13,
  0x25, 0xfa, 0x1d, 0xdc, 0xf1, 0xf0, 0xa6, 0x8d, 0x2a, 0xbb, 0xbc, 0xb0, 0xc7,
  0x97, 0x98, 0x8e, 0xef, 0xd9, 0x12, 0x24, 0xee, 0x38, 0x50, 0xdb, 0xd3, 0x59,
  0xcc, 0x30, 0x54, 0x4c, 0x38, 0x94, 0x24, 0xbc, 0x75, 0xa5, 0xc0, 0xc4, 0x00,
  0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3a, 0x00, 0x15, 0x02, 0x62, 0x30, 0x03,
  0x6f, 0x72, 0x67, 0x0b, 0x61, 0x66, 0x69, 0x6c, 0x69, 0x61, 0x73, 0x2d, 0x6e,
  0x73, 0x74, 0xc0, 0xc4, 0xc0, 0xc4, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x3a, 0x00, 0x19, 0x02, 0x63, 0x30, 0x03, 0x6f, 0x72, 0x67, 0x0b, 0x61, 0x66,
  0x69, 0x6c, 0x69, 0x61, 0x73, 0x2d, 0x6e, 0x73, 0x74, 0x04, 0x69, 0x6e, 0x66,
  0x6f, 0x00, 0xc0, 0xc4, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3a, 0x00,
  0x05, 0x02, 0x61, 0x30, 0xc1, 0x99, 0xc0, 0xc4, 0x00, 0x02, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x3a, 0x00, 0x05, 0x02, 0x62, 0x32, 0xc1, 0x78, 0xc0, 0xc4, 0x00,
  0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3a, 0x00, 0x05, 0x02, 0x64, 0x30, 0xc1,
  0x78, 0xc0, 0xc4, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3a, 0x00, 0x05,
  0x02, 0x61, 0x32, 0xc1, 0x99, 0x00, 0x00, 0x29, 0x10, 0x00, 0x00, 0x00, 0x80,
  0x00, 0x00, 0x00,
};

TEST_F(DnsRRResolverTest, ParseExample) {
  RRResponse response;
  ASSERT_TRUE(response.ParseFromResponse(kExamplePacket,
              sizeof(kExamplePacket), kDNS_TXT));
  ASSERT_EQ(1u, response.rrdatas.size());
  ASSERT_EQ(1u, response.signatures.size());
  ASSERT_STREQ("agl._pka.imperialviolet.org", response.name.c_str());
  ASSERT_STREQ("]v=pka1;fpr=2AF0032B48E856CE06157A1AD43C670DE04AAA74;"
               "uri=http://www.imperialviolet.org/key.asc",
               response.rrdatas[0].c_str());
  ASSERT_FALSE(response.dnssec);
}

TEST_F(DnsRRResolverTest, FuzzTruncation) {
  RRResponse response;

  for (unsigned len = sizeof(kExamplePacket); len <= sizeof(kExamplePacket);
       len--) {
    response.ParseFromResponse(kExamplePacket, len, kDNS_TXT);
  }
}

TEST_F(DnsRRResolverTest, FuzzCorruption) {
  RRResponse response;
  uint8 copy[sizeof(kExamplePacket)];


  for (unsigned bit_to_corrupt = 0; bit_to_corrupt < sizeof(kExamplePacket) * 8;
       bit_to_corrupt++) {
    unsigned byte = bit_to_corrupt >> 3;
    unsigned bit = bit_to_corrupt & 7;

    memcpy(copy, kExamplePacket, sizeof(copy));
    copy[byte] ^= (1 << bit);

    response.ParseFromResponse(copy, sizeof(copy), kDNS_TXT);
  }
}

#endif  // OS_LINUX

}  // namespace net
