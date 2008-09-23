// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/port.h"
#include "net/base/x509_certificate.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class X509CertificateTest : public testing::Test {
};

// Google's cert (for our test data). This was obtained with:
// 
// $ openssl s_client -connect www.google.com:443 -showcerts
// $ openssl x509 -inform PEM -outform DER > /tmp/google.der
// $ xxd -i /tmp/google.der

unsigned char google_der[] = {
  0x30, 0x82, 0x03, 0x21, 0x30, 0x82, 0x02, 0x8a, 0xa0, 0x03, 0x02, 0x01,
  0x02, 0x02, 0x10, 0x3c, 0x8d, 0x3a, 0x64, 0xee, 0x18, 0xdd, 0x1b, 0x73,
  0x0b, 0xa1, 0x92, 0xee, 0xf8, 0x98, 0x1b, 0x30, 0x0d, 0x06, 0x09, 0x2a,
  0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x05, 0x05, 0x00, 0x30, 0x4c,
  0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x5a,
  0x41, 0x31, 0x25, 0x30, 0x23, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x1c,
  0x54, 0x68, 0x61, 0x77, 0x74, 0x65, 0x20, 0x43, 0x6f, 0x6e, 0x73, 0x75,
  0x6c, 0x74, 0x69, 0x6e, 0x67, 0x20, 0x28, 0x50, 0x74, 0x79, 0x29, 0x20,
  0x4c, 0x74, 0x64, 0x2e, 0x31, 0x16, 0x30, 0x14, 0x06, 0x03, 0x55, 0x04,
  0x03, 0x13, 0x0d, 0x54, 0x68, 0x61, 0x77, 0x74, 0x65, 0x20, 0x53, 0x47,
  0x43, 0x20, 0x43, 0x41, 0x30, 0x1e, 0x17, 0x0d, 0x30, 0x38, 0x30, 0x35,
  0x30, 0x32, 0x31, 0x37, 0x30, 0x32, 0x35, 0x35, 0x5a, 0x17, 0x0d, 0x30,
  0x39, 0x30, 0x35, 0x30, 0x32, 0x31, 0x37, 0x30, 0x32, 0x35, 0x35, 0x5a,
  0x30, 0x68, 0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13,
  0x02, 0x55, 0x53, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x08,
  0x13, 0x0a, 0x43, 0x61, 0x6c, 0x69, 0x66, 0x6f, 0x72, 0x6e, 0x69, 0x61,
  0x31, 0x16, 0x30, 0x14, 0x06, 0x03, 0x55, 0x04, 0x07, 0x13, 0x0d, 0x4d,
  0x6f, 0x75, 0x6e, 0x74, 0x61, 0x69, 0x6e, 0x20, 0x56, 0x69, 0x65, 0x77,
  0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13, 0x0a, 0x47,
  0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x20, 0x49, 0x6e, 0x63, 0x31, 0x17, 0x30,
  0x15, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0e, 0x77, 0x77, 0x77, 0x2e,
  0x67, 0x6f, 0x6f, 0x67, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d, 0x30, 0x81,
  0x9f, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01,
  0x01, 0x01, 0x05, 0x00, 0x03, 0x81, 0x8d, 0x00, 0x30, 0x81, 0x89, 0x02,
  0x81, 0x81, 0x00, 0x9b, 0x19, 0xed, 0x5d, 0xa5, 0x56, 0xaf, 0x49, 0x66,
  0xdb, 0x79, 0xfd, 0xc2, 0x1c, 0x78, 0x4e, 0x4f, 0x11, 0xa5, 0x8a, 0xac,
  0xe2, 0x94, 0xee, 0xe3, 0xe2, 0x4b, 0xc0, 0x03, 0x25, 0xa7, 0x99, 0xcc,
  0x65, 0xe1, 0xec, 0x94, 0xae, 0xae, 0xf0, 0xa7, 0x99, 0xbc, 0x10, 0xd7,
  0xed, 0x87, 0x30, 0x47, 0xcd, 0x50, 0xf9, 0xaf, 0xd3, 0xd3, 0xf4, 0x0b,
  0x8d, 0x47, 0x8a, 0x2e, 0xe2, 0xce, 0x53, 0x9b, 0x91, 0x99, 0x7f, 0x1e,
  0x5c, 0xf9, 0x1b, 0xd6, 0xe9, 0x93, 0x67, 0xe3, 0x4a, 0xf8, 0xcf, 0xc4,
  0x8c, 0x0c, 0x68, 0xd1, 0x97, 0x54, 0x47, 0x0e, 0x0a, 0x24, 0x30, 0xa7,
  0x82, 0x94, 0xae, 0xde, 0xae, 0x3f, 0xbf, 0xba, 0x14, 0xc6, 0xf8, 0xb2,
  0x90, 0x8e, 0x36, 0xad, 0xe1, 0xd0, 0xbe, 0x16, 0x9a, 0xb3, 0x5e, 0x72,
  0x38, 0x49, 0xda, 0x74, 0xa1, 0x3f, 0xff, 0xd2, 0x87, 0x81, 0xed, 0x02,
  0x03, 0x01, 0x00, 0x01, 0xa3, 0x81, 0xe7, 0x30, 0x81, 0xe4, 0x30, 0x28,
  0x06, 0x03, 0x55, 0x1d, 0x25, 0x04, 0x21, 0x30, 0x1f, 0x06, 0x08, 0x2b,
  0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x01, 0x06, 0x08, 0x2b, 0x06, 0x01,
  0x05, 0x05, 0x07, 0x03, 0x02, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x86,
  0xf8, 0x42, 0x04, 0x01, 0x30, 0x36, 0x06, 0x03, 0x55, 0x1d, 0x1f, 0x04,
  0x2f, 0x30, 0x2d, 0x30, 0x2b, 0xa0, 0x29, 0xa0, 0x27, 0x86, 0x25, 0x68,
  0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x63, 0x72, 0x6c, 0x2e, 0x74, 0x68,
  0x61, 0x77, 0x74, 0x65, 0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x54, 0x68, 0x61,
  0x77, 0x74, 0x65, 0x53, 0x47, 0x43, 0x43, 0x41, 0x2e, 0x63, 0x72, 0x6c,
  0x30, 0x72, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x01, 0x01,
  0x04, 0x66, 0x30, 0x64, 0x30, 0x22, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05,
  0x05, 0x07, 0x30, 0x01, 0x86, 0x16, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f,
  0x2f, 0x6f, 0x63, 0x73, 0x70, 0x2e, 0x74, 0x68, 0x61, 0x77, 0x74, 0x65,
  0x2e, 0x63, 0x6f, 0x6d, 0x30, 0x3e, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05,
  0x05, 0x07, 0x30, 0x02, 0x86, 0x32, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f,
  0x2f, 0x77, 0x77, 0x77, 0x2e, 0x74, 0x68, 0x61, 0x77, 0x74, 0x65, 0x2e,
  0x63, 0x6f, 0x6d, 0x2f, 0x72, 0x65, 0x70, 0x6f, 0x73, 0x69, 0x74, 0x6f,
  0x72, 0x79, 0x2f, 0x54, 0x68, 0x61, 0x77, 0x74, 0x65, 0x5f, 0x53, 0x47,
  0x43, 0x5f, 0x43, 0x41, 0x2e, 0x63, 0x72, 0x74, 0x30, 0x0c, 0x06, 0x03,
  0x55, 0x1d, 0x13, 0x01, 0x01, 0xff, 0x04, 0x02, 0x30, 0x00, 0x30, 0x0d,
  0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x05, 0x05,
  0x00, 0x03, 0x81, 0x81, 0x00, 0x31, 0x0a, 0x6c, 0xa2, 0x9e, 0xe9, 0x54,
  0x19, 0x16, 0x68, 0x99, 0x91, 0xd6, 0x43, 0xcb, 0x6b, 0xb4, 0xcc, 0x6c,
  0xcc, 0xb0, 0xfb, 0xf1, 0xee, 0x81, 0xbf, 0x00, 0x2b, 0x6f, 0x50, 0x12,
  0xc6, 0xaf, 0x02, 0x2a, 0x36, 0xc1, 0x28, 0xde, 0xc5, 0x4c, 0x56, 0x20,
  0x6d, 0xf5, 0x3d, 0x42, 0xb9, 0x18, 0x81, 0x20, 0xb2, 0xdd, 0x57, 0x5d,
  0xeb, 0xbe, 0x32, 0x84, 0x50, 0x45, 0x51, 0x6e, 0xcd, 0xe4, 0x2e, 0x2a,
  0x38, 0x88, 0x9f, 0x52, 0xed, 0x28, 0xff, 0xfc, 0x8d, 0x57, 0xb5, 0xad,
  0x64, 0xae, 0x4d, 0x0e, 0x0e, 0xd9, 0x3d, 0xac, 0xb8, 0xfe, 0x66, 0x4c,
  0x15, 0x8f, 0x44, 0x52, 0xfa, 0x7c, 0x3c, 0x04, 0xed, 0x7f, 0x37, 0x61,
  0x04, 0xfe, 0xd5, 0xe9, 0xb9, 0xb0, 0x9e, 0xfe, 0xa5, 0x11, 0x69, 0xc9,
  0x63, 0xd6, 0x46, 0x81, 0x6f, 0x00, 0xd8, 0x72, 0x2f, 0x82, 0x37, 0x44,
  0xc1
};

unsigned char google_fingerprint[] = {
  0x8a, 0xaa, 0x9a, 0x71, 0xf0, 0x5c, 0xe7, 0x25, 0x8a, 0x35, 0x0a, 0x32,
  0xb1, 0x91, 0x69, 0x44, 0x9b, 0x36, 0x93, 0xa8
};

// Webkit.org's cert (for our test data).

unsigned char webkit_der[] = {
  0x30, 0x82, 0x05, 0x0d, 0x30, 0x82, 0x03, 0xf5, 0xa0, 0x03, 0x02, 0x01,
  0x02, 0x02, 0x03, 0x43, 0xdd, 0x63, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86,
  0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x05, 0x05, 0x00, 0x30, 0x81, 0xca,
  0x31, 0x0b, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x55,
  0x53, 0x31, 0x10, 0x30, 0x0e, 0x06, 0x03, 0x55, 0x04, 0x08, 0x13, 0x07,
  0x41, 0x72, 0x69, 0x7a, 0x6f, 0x6e, 0x61, 0x31, 0x13, 0x30, 0x11, 0x06,
  0x03, 0x55, 0x04, 0x07, 0x13, 0x0a, 0x53, 0x63, 0x6f, 0x74, 0x74, 0x73,
  0x64, 0x61, 0x6c, 0x65, 0x31, 0x1a, 0x30, 0x18, 0x06, 0x03, 0x55, 0x04,
  0x0a, 0x13, 0x11, 0x47, 0x6f, 0x44, 0x61, 0x64, 0x64, 0x79, 0x2e, 0x63,
  0x6f, 0x6d, 0x2c, 0x20, 0x49, 0x6e, 0x63, 0x2e, 0x31, 0x33, 0x30, 0x31,
  0x06, 0x03, 0x55, 0x04, 0x0b, 0x13, 0x2a, 0x68, 0x74, 0x74, 0x70, 0x3a,
  0x2f, 0x2f, 0x63, 0x65, 0x72, 0x74, 0x69, 0x66, 0x69, 0x63, 0x61, 0x74,
  0x65, 0x73, 0x2e, 0x67, 0x6f, 0x64, 0x61, 0x64, 0x64, 0x79, 0x2e, 0x63,
  0x6f, 0x6d, 0x2f, 0x72, 0x65, 0x70, 0x6f, 0x73, 0x69, 0x74, 0x6f, 0x72,
  0x79, 0x31, 0x30, 0x30, 0x2e, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x27,
  0x47, 0x6f, 0x20, 0x44, 0x61, 0x64, 0x64, 0x79, 0x20, 0x53, 0x65, 0x63,
  0x75, 0x72, 0x65, 0x20, 0x43, 0x65, 0x72, 0x74, 0x69, 0x66, 0x69, 0x63,
  0x61, 0x74, 0x69, 0x6f, 0x6e, 0x20, 0x41, 0x75, 0x74, 0x68, 0x6f, 0x72,
  0x69, 0x74, 0x79, 0x31, 0x11, 0x30, 0x0f, 0x06, 0x03, 0x55, 0x04, 0x05,
  0x13, 0x08, 0x30, 0x37, 0x39, 0x36, 0x39, 0x32, 0x38, 0x37, 0x30, 0x1e,
  0x17, 0x0d, 0x30, 0x38, 0x30, 0x33, 0x31, 0x38, 0x32, 0x33, 0x33, 0x35,
  0x31, 0x39, 0x5a, 0x17, 0x0d, 0x31, 0x31, 0x30, 0x33, 0x31, 0x38, 0x32,
  0x33, 0x33, 0x35, 0x31, 0x39, 0x5a, 0x30, 0x79, 0x31, 0x0b, 0x30, 0x09,
  0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31, 0x13, 0x30,
  0x11, 0x06, 0x03, 0x55, 0x04, 0x08, 0x13, 0x0a, 0x43, 0x61, 0x6c, 0x69,
  0x66, 0x6f, 0x72, 0x6e, 0x69, 0x61, 0x31, 0x12, 0x30, 0x10, 0x06, 0x03,
  0x55, 0x04, 0x07, 0x13, 0x09, 0x43, 0x75, 0x70, 0x65, 0x72, 0x74, 0x69,
  0x6e, 0x6f, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x0a, 0x13,
  0x0a, 0x41, 0x70, 0x70, 0x6c, 0x65, 0x20, 0x49, 0x6e, 0x63, 0x2e, 0x31,
  0x15, 0x30, 0x13, 0x06, 0x03, 0x55, 0x04, 0x0b, 0x13, 0x0c, 0x4d, 0x61,
  0x63, 0x20, 0x4f, 0x53, 0x20, 0x46, 0x6f, 0x72, 0x67, 0x65, 0x31, 0x15,
  0x30, 0x13, 0x06, 0x03, 0x55, 0x04, 0x03, 0x13, 0x0c, 0x2a, 0x2e, 0x77,
  0x65, 0x62, 0x6b, 0x69, 0x74, 0x2e, 0x6f, 0x72, 0x67, 0x30, 0x81, 0x9f,
  0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01,
  0x01, 0x05, 0x00, 0x03, 0x81, 0x8d, 0x00, 0x30, 0x81, 0x89, 0x02, 0x81,
  0x81, 0x00, 0xa7, 0x62, 0x79, 0x41, 0xda, 0x28, 0xf2, 0xc0, 0x4f, 0xe0,
  0x25, 0xaa, 0xa1, 0x2e, 0x3b, 0x30, 0x94, 0xb5, 0xc9, 0x26, 0x3a, 0x1b,
  0xe2, 0xd0, 0xcc, 0xa2, 0x95, 0xe2, 0x91, 0xc0, 0xf0, 0x40, 0x9e, 0x27,
  0x6e, 0xbd, 0x6e, 0xde, 0x7c, 0xb6, 0x30, 0x5c, 0xb8, 0x9b, 0x01, 0x2f,
  0x92, 0x04, 0xa1, 0xef, 0x4a, 0xb1, 0x6c, 0xb1, 0x7e, 0x8e, 0xcd, 0xa6,
  0xf4, 0x40, 0x73, 0x1f, 0x2c, 0x96, 0xad, 0xff, 0x2a, 0x6d, 0x0e, 0xba,
  0x52, 0x84, 0x83, 0xb0, 0x39, 0xee, 0xc9, 0x39, 0xdc, 0x1e, 0x34, 0xd0,
  0xd8, 0x5d, 0x7a, 0x09, 0xac, 0xa9, 0xee, 0xca, 0x65, 0xf6, 0x85, 0x3a,
  0x6b, 0xee, 0xe4, 0x5c, 0x5e, 0xf8, 0xda, 0xd1, 0xce, 0x88, 0x47, 0xcd,
  0x06, 0x21, 0xe0, 0xb9, 0x4b, 0xe4, 0x07, 0xcb, 0x57, 0xdc, 0xca, 0x99,
  0x54, 0xf7, 0x0e, 0xd5, 0x17, 0x95, 0x05, 0x2e, 0xe9, 0xb1, 0x02, 0x03,
  0x01, 0x00, 0x01, 0xa3, 0x82, 0x01, 0xce, 0x30, 0x82, 0x01, 0xca, 0x30,
  0x09, 0x06, 0x03, 0x55, 0x1d, 0x13, 0x04, 0x02, 0x30, 0x00, 0x30, 0x0b,
  0x06, 0x03, 0x55, 0x1d, 0x0f, 0x04, 0x04, 0x03, 0x02, 0x05, 0xa0, 0x30,
  0x1d, 0x06, 0x03, 0x55, 0x1d, 0x25, 0x04, 0x16, 0x30, 0x14, 0x06, 0x08,
  0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x01, 0x06, 0x08, 0x2b, 0x06,
  0x01, 0x05, 0x05, 0x07, 0x03, 0x02, 0x30, 0x57, 0x06, 0x03, 0x55, 0x1d,
  0x1f, 0x04, 0x50, 0x30, 0x4e, 0x30, 0x4c, 0xa0, 0x4a, 0xa0, 0x48, 0x86,
  0x46, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x63, 0x65, 0x72, 0x74,
  0x69, 0x66, 0x69, 0x63, 0x61, 0x74, 0x65, 0x73, 0x2e, 0x67, 0x6f, 0x64,
  0x61, 0x64, 0x64, 0x79, 0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x72, 0x65, 0x70,
  0x6f, 0x73, 0x69, 0x74, 0x6f, 0x72, 0x79, 0x2f, 0x67, 0x6f, 0x64, 0x61,
  0x64, 0x64, 0x79, 0x65, 0x78, 0x74, 0x65, 0x6e, 0x64, 0x65, 0x64, 0x69,
  0x73, 0x73, 0x75, 0x69, 0x6e, 0x67, 0x33, 0x2e, 0x63, 0x72, 0x6c, 0x30,
  0x52, 0x06, 0x03, 0x55, 0x1d, 0x20, 0x04, 0x4b, 0x30, 0x49, 0x30, 0x47,
  0x06, 0x0b, 0x60, 0x86, 0x48, 0x01, 0x86, 0xfd, 0x6d, 0x01, 0x07, 0x17,
  0x02, 0x30, 0x38, 0x30, 0x36, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05,
  0x07, 0x02, 0x01, 0x16, 0x2a, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f,
  0x63, 0x65, 0x72, 0x74, 0x69, 0x66, 0x69, 0x63, 0x61, 0x74, 0x65, 0x73,
  0x2e, 0x67, 0x6f, 0x64, 0x61, 0x64, 0x64, 0x79, 0x2e, 0x63, 0x6f, 0x6d,
  0x2f, 0x72, 0x65, 0x70, 0x6f, 0x73, 0x69, 0x74, 0x6f, 0x72, 0x79, 0x30,
  0x7f, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x01, 0x01, 0x04,
  0x73, 0x30, 0x71, 0x30, 0x23, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05,
  0x07, 0x30, 0x01, 0x86, 0x17, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f,
  0x6f, 0x63, 0x73, 0x70, 0x2e, 0x67, 0x6f, 0x64, 0x61, 0x64, 0x64, 0x79,
  0x2e, 0x63, 0x6f, 0x6d, 0x30, 0x4a, 0x06, 0x08, 0x2b, 0x06, 0x01, 0x05,
  0x05, 0x07, 0x30, 0x02, 0x86, 0x3e, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f,
  0x2f, 0x63, 0x65, 0x72, 0x74, 0x69, 0x66, 0x69, 0x63, 0x61, 0x74, 0x65,
  0x73, 0x2e, 0x67, 0x6f, 0x64, 0x61, 0x64, 0x64, 0x79, 0x2e, 0x63, 0x6f,
  0x6d, 0x2f, 0x72, 0x65, 0x70, 0x6f, 0x73, 0x69, 0x74, 0x6f, 0x72, 0x79,
  0x2f, 0x67, 0x64, 0x5f, 0x69, 0x6e, 0x74, 0x65, 0x72, 0x6d, 0x65, 0x64,
  0x69, 0x61, 0x74, 0x65, 0x2e, 0x63, 0x72, 0x74, 0x30, 0x1d, 0x06, 0x03,
  0x55, 0x1d, 0x0e, 0x04, 0x16, 0x04, 0x14, 0x48, 0xdf, 0x60, 0x32, 0xcc,
  0x89, 0x01, 0xb6, 0xdc, 0x2f, 0xe3, 0x73, 0xb5, 0x9c, 0x16, 0x58, 0x32,
  0x68, 0xa9, 0xc3, 0x30, 0x1f, 0x06, 0x03, 0x55, 0x1d, 0x23, 0x04, 0x18,
  0x30, 0x16, 0x80, 0x14, 0xfd, 0xac, 0x61, 0x32, 0x93, 0x6c, 0x45, 0xd6,
  0xe2, 0xee, 0x85, 0x5f, 0x9a, 0xba, 0xe7, 0x76, 0x99, 0x68, 0xcc, 0xe7,
  0x30, 0x23, 0x06, 0x03, 0x55, 0x1d, 0x11, 0x04, 0x1c, 0x30, 0x1a, 0x82,
  0x0c, 0x2a, 0x2e, 0x77, 0x65, 0x62, 0x6b, 0x69, 0x74, 0x2e, 0x6f, 0x72,
  0x67, 0x82, 0x0a, 0x77, 0x65, 0x62, 0x6b, 0x69, 0x74, 0x2e, 0x6f, 0x72,
  0x67, 0x30, 0x0d, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01,
  0x01, 0x05, 0x05, 0x00, 0x03, 0x82, 0x01, 0x01, 0x00, 0x1e, 0x6a, 0xe7,
  0xe0, 0x4f, 0xe7, 0x4d, 0xd0, 0x69, 0x7c, 0xf8, 0x8f, 0x99, 0xb4, 0x18,
  0x95, 0x36, 0x24, 0x0f, 0x0e, 0xa3, 0xea, 0x34, 0x37, 0xf4, 0x7d, 0xd5,
  0x92, 0x35, 0x53, 0x72, 0x76, 0x3f, 0x69, 0xf0, 0x82, 0x56, 0xe3, 0x94,
  0x7a, 0x1d, 0x1a, 0x81, 0xaf, 0x9f, 0xc7, 0x43, 0x01, 0x64, 0xd3, 0x7c,
  0x0d, 0xc8, 0x11, 0x4e, 0x4a, 0xe6, 0x1a, 0xc3, 0x01, 0x74, 0xe8, 0x35,
  0x87, 0x5c, 0x61, 0xaa, 0x8a, 0x46, 0x06, 0xbe, 0x98, 0x95, 0x24, 0x9e,
  0x01, 0xe3, 0xe6, 0xa0, 0x98, 0xee, 0x36, 0x44, 0x56, 0x8d, 0x23, 0x9c,
  0x65, 0xea, 0x55, 0x6a, 0xdf, 0x66, 0xee, 0x45, 0xe8, 0xa0, 0xe9, 0x7d,
  0x9a, 0xba, 0x94, 0xc5, 0xc8, 0xc4, 0x4b, 0x98, 0xff, 0x9a, 0x01, 0x31,
  0x6d, 0xf9, 0x2b, 0x58, 0xe7, 0xe7, 0x2a, 0xc5, 0x4d, 0xbb, 0xbb, 0xcd,
  0x0d, 0x70, 0xe1, 0xad, 0x03, 0xf5, 0xfe, 0xf4, 0x84, 0x71, 0x08, 0xd2,
  0xbc, 0x04, 0x7b, 0x26, 0x1c, 0xa8, 0x0f, 0x9c, 0xd8, 0x12, 0x6a, 0x6f,
  0x2b, 0x67, 0xa1, 0x03, 0x80, 0x9a, 0x11, 0x0b, 0xe9, 0xe0, 0xb5, 0xb3,
  0xb8, 0x19, 0x4e, 0x0c, 0xa4, 0xd9, 0x2b, 0x3b, 0xc2, 0xca, 0x20, 0xd3,
  0x0c, 0xa4, 0xff, 0x93, 0x13, 0x1f, 0xfc, 0xba, 0x94, 0x93, 0x8c, 0x64,
  0x15, 0x2e, 0x28, 0xa9, 0x55, 0x8c, 0x2c, 0x48, 0xd3, 0xd3, 0xc1, 0x50,
  0x69, 0x19, 0xe8, 0x34, 0xd3, 0xf1, 0x04, 0x9f, 0x0a, 0x7a, 0x21, 0x87,
  0xbf, 0xb9, 0x59, 0x37, 0x2e, 0xf4, 0x71, 0xa5, 0x3e, 0xbe, 0xcd, 0x70,
  0x83, 0x18, 0xf8, 0x8a, 0x72, 0x85, 0x45, 0x1f, 0x08, 0x01, 0x6f, 0x37,
  0xf5, 0x2b, 0x7b, 0xea, 0xb9, 0x8b, 0xa3, 0xcc, 0xfd, 0x35, 0x52, 0xdd,
  0x66, 0xde, 0x4f, 0x30, 0xc5, 0x73, 0x81, 0xb6, 0xe8, 0x3c, 0xd8, 0x48,
  0x8a
};

unsigned char webkit_fingerprint[] = {
  0xa1, 0x4a, 0x94, 0x46, 0x22, 0x8e, 0x70, 0x66, 0x2b, 0x94, 0xf9, 0xf8,
  0x57, 0x83, 0x2d, 0xa2, 0xff, 0xbc, 0x84, 0xc2
};

}  // namespace

using net::X509Certificate;

TEST(X509CertificateTest, GoogleCertParsing) {
  scoped_refptr<X509Certificate> google_cert = X509Certificate::CreateFromBytes(
      reinterpret_cast<const char*>(google_der), sizeof(google_der));
  
  ASSERT_NE(static_cast<X509Certificate *>(NULL), google_cert);

  const X509Certificate::Principal& subject = google_cert->subject();
  EXPECT_EQ("www.google.com", subject.common_name);
  EXPECT_EQ("Mountain View", subject.locality_name);
  EXPECT_EQ("California", subject.state_or_province_name);
  EXPECT_EQ("US", subject.country_name);
  EXPECT_EQ(0U, subject.street_addresses.size());
  EXPECT_EQ(1U, subject.organization_names.size());
  EXPECT_EQ("Google Inc", subject.organization_names[0]);
  EXPECT_EQ(0U, subject.organization_unit_names.size());
  EXPECT_EQ(0U, subject.domain_components.size());
  
  const X509Certificate::Principal& issuer = google_cert->issuer();
  EXPECT_EQ("Thawte SGC CA", issuer.common_name);
  EXPECT_EQ("", issuer.locality_name);
  EXPECT_EQ("", issuer.state_or_province_name);
  EXPECT_EQ("ZA", issuer.country_name);
  EXPECT_EQ(0U, issuer.street_addresses.size());
  EXPECT_EQ(1U, issuer.organization_names.size());
  EXPECT_EQ("Thawte Consulting (Pty) Ltd.", issuer.organization_names[0]);
  EXPECT_EQ(0U, issuer.organization_unit_names.size());
  EXPECT_EQ(0U, issuer.domain_components.size());
  
  // Use DoubleT because its epoch is the same on all platforms
  const Time& valid_start = google_cert->valid_start();
  EXPECT_EQ(1209747775, valid_start.ToDoubleT());
  
  const Time& valid_expiry = google_cert->valid_expiry();
  EXPECT_EQ(1241283775, valid_expiry.ToDoubleT());
  
  const X509Certificate::Fingerprint& fingerprint = google_cert->fingerprint();
  for (size_t i = 0; i < 20; ++i)
    EXPECT_EQ(google_fingerprint[i], fingerprint.data[i]);

  std::vector<std::string> dns_names;
  google_cert->GetDNSNames(&dns_names);
  EXPECT_EQ(1U, dns_names.size());
  EXPECT_EQ("www.google.com", dns_names[0]);
}

TEST(X509CertificateTest, WebkitCertParsing) {
  scoped_refptr<X509Certificate> webkit_cert = X509Certificate::CreateFromBytes(
      reinterpret_cast<const char*>(webkit_der), sizeof(webkit_der));
  
  ASSERT_NE(static_cast<X509Certificate *>(NULL), webkit_cert);

  const X509Certificate::Principal& subject = webkit_cert->subject();
  EXPECT_EQ("Cupertino", subject.locality_name);
  EXPECT_EQ("California", subject.state_or_province_name);
  EXPECT_EQ("US", subject.country_name);
  EXPECT_EQ(0U, subject.street_addresses.size());
  EXPECT_EQ(1U, subject.organization_names.size());
  EXPECT_EQ("Apple Inc.", subject.organization_names[0]);
  EXPECT_EQ(1U, subject.organization_unit_names.size());
  EXPECT_EQ("Mac OS Forge", subject.organization_unit_names[0]);
  EXPECT_EQ(0U, subject.domain_components.size());
  
  const X509Certificate::Principal& issuer = webkit_cert->issuer();
  EXPECT_EQ("Go Daddy Secure Certification Authority", issuer.common_name);
  EXPECT_EQ("Scottsdale", issuer.locality_name);
  EXPECT_EQ("Arizona", issuer.state_or_province_name);
  EXPECT_EQ("US", issuer.country_name);
  EXPECT_EQ(0U, issuer.street_addresses.size());
  EXPECT_EQ(1U, issuer.organization_names.size());
  EXPECT_EQ("GoDaddy.com, Inc.", issuer.organization_names[0]);
  EXPECT_EQ(1U, issuer.organization_unit_names.size());
  EXPECT_EQ("http://certificates.godaddy.com/repository",
      issuer.organization_unit_names[0]);
  EXPECT_EQ(0U, issuer.domain_components.size());
  
  // Use DoubleT because its epoch is the same on all platforms
  const Time& valid_start = webkit_cert->valid_start();
  EXPECT_EQ(1205883319, valid_start.ToDoubleT());
  
  const Time& valid_expiry = webkit_cert->valid_expiry();
  EXPECT_EQ(1300491319, valid_expiry.ToDoubleT());
  
  const X509Certificate::Fingerprint& fingerprint = webkit_cert->fingerprint();
  for (size_t i = 0; i < 20; ++i)
    EXPECT_EQ(webkit_fingerprint[i], fingerprint.data[i]);

  std::vector<std::string> dns_names;
  webkit_cert->GetDNSNames(&dns_names);
  EXPECT_EQ(2U, dns_names.size());
  EXPECT_EQ("*.webkit.org", dns_names[0]);
  EXPECT_EQ("webkit.org", dns_names[1]);
}

// TODO(avi): Tests needed
// - Test EV
