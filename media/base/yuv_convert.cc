// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This webpage shows layout of YV12 and other YUV formats
// http://www.fourcc.org/yuv.php
// The actual conversion is best described here
// http://en.wikipedia.org/wiki/YUV
// excerpt from wiki:
//   These formulae are based on the NTSC standard;
//     Y' = 0.299 x R + 0.587 x G + 0.114 x B
//     U = -0.147 x R - 0.289 x G + 0.436 x B
//     V =  0.615 x R - 0.515 x G - 0.100 x B
//   On older, non-SIMD architectures, floating point arithmetic is much
//    slower than using fixed-point arithmetic, so an alternative formulation
//    is:
//      C = Y' - 16
//      D = U - 128
//      E = V - 128
//   Using the previous coefficients and noting that clip() denotes clipping a
//    value to the range of 0 to 255, the following formulae provide the
//    conversion from Y'UV to RGB (NTSC version):
//      R = clip((298 x C + 409 x E + 128) >> 8)
//      G = clip((298 x C - 100 x D - 208 x E + 128) >> 8)
//      B = clip((298 x C + 516 x D + 128) >> 8)
//
// An article on optimizing YUV conversion using tables instead of multiplies
//   http://lestourtereaux.free.fr/papers/data/yuvrgb.pdf
//
// Implimentation notes
//   This version uses MMX for Visual C and GCC, which should cover all
//   current platforms.  C++ is included for reference and future platforms.
//
//   ARGB pixel format is assumed, which on little endian is stored as BGRA.
//   The alpha is filled in, allowing the application to use RGBA or RGB32.
//   The row based conversion allows for a future YV16 version, and simplifies
//   the platform specific portion of the code.
//
//  The Visual C assembler is considered the source.
//  The GCC asm was created by compiling with Visual C and disassembling
//  with GNU objdump.
//    cl /c /Ox yuv_convert.cc
//    objdump -d yuv_convert.o
//  The code almost copy/pasted in, except the table lookups, which produced
//    movq   0x800(,%eax,8),%mm0
//  and needed to be changed to cdecl style table names
//   "movq   _coefficients_RGB_U(,%eax,8),%mm0\n"
//  extern "C" was used to avoid name mangling.
//
//  Once compiled with both MinGW GCC and Visual C on PC, performance should
//    be identical.  A small difference will occur in the C++ calling code,
//    depending on the frame size.
//  To confirm the same code is being generated
//    g++ -O3 -c yuv_convert.cc
//    dumpbin -disasm yuv_convert.o >gcc.txt
//    cl /Ox /c yuv_convert.cc
//    dumpbin -disasm yuv_convert.obj >vc.txt
//    and compare the files.
//
//  The GCC function label is inside the assembler to avoid a stack frame
//    push ebp, that may vary depending on compile options.

#include "media/base/yuv_convert.h"

#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef _DEBUG
#include "base/logging.h"
#else
#define DCHECK(a)
#endif

// MMX for Windows, Linux and Mac.
// C++ code provided as a fall back.
// Compile with /DUSE_MMX=0
#ifndef USE_MMX
#define USE_MMX 1
#endif

namespace media {

#if USE_MMX

#define RGBY(i) { \
  static_cast<int16>(1.164 * 64 * (i - 16) + 0.5), \
  static_cast<int16>(1.164 * 64 * (i - 16) + 0.5), \
  static_cast<int16>(1.164 * 64 * (i - 16) + 0.5), \
  0 \
}

#define RGBU(i) { \
  static_cast<int16>(2.018 * 64 * (i - 128) + 0.5), \
  static_cast<int16>(-0.391 * 64 * (i - 128) + 0.5), \
  0, \
  static_cast<int16>(256 * 64 - 1) \
}

#define RGBV(i) { \
  0, \
  static_cast<int16>(-0.813 * 64 * (i - 128) + 0.5), \
  static_cast<int16>(1.596 * 64 * (i - 128) + 0.5), \
  0 \
}

#if defined(_MSC_VER)
#define MMX_ALIGNED(var) __declspec(align(16)) var
#else
#define MMX_ALIGNED(var) var __attribute__((aligned(16)))
#endif

extern "C" {
MMX_ALIGNED(int16 coefficients_RGB_Y[256][4]) = {
  RGBY(0x00), RGBY(0x01), RGBY(0x02), RGBY(0x03),
  RGBY(0x04), RGBY(0x05), RGBY(0x06), RGBY(0x07),
  RGBY(0x08), RGBY(0x09), RGBY(0x0A), RGBY(0x0B),
  RGBY(0x0C), RGBY(0x0D), RGBY(0x0E), RGBY(0x0F),
  RGBY(0x10), RGBY(0x11), RGBY(0x12), RGBY(0x13),
  RGBY(0x14), RGBY(0x15), RGBY(0x16), RGBY(0x17),
  RGBY(0x18), RGBY(0x19), RGBY(0x1A), RGBY(0x1B),
  RGBY(0x1C), RGBY(0x1D), RGBY(0x1E), RGBY(0x1F),
  RGBY(0x20), RGBY(0x21), RGBY(0x22), RGBY(0x23),
  RGBY(0x24), RGBY(0x25), RGBY(0x26), RGBY(0x27),
  RGBY(0x28), RGBY(0x29), RGBY(0x2A), RGBY(0x2B),
  RGBY(0x2C), RGBY(0x2D), RGBY(0x2E), RGBY(0x2F),
  RGBY(0x30), RGBY(0x31), RGBY(0x32), RGBY(0x33),
  RGBY(0x34), RGBY(0x35), RGBY(0x36), RGBY(0x37),
  RGBY(0x38), RGBY(0x39), RGBY(0x3A), RGBY(0x3B),
  RGBY(0x3C), RGBY(0x3D), RGBY(0x3E), RGBY(0x3F),
  RGBY(0x40), RGBY(0x41), RGBY(0x42), RGBY(0x43),
  RGBY(0x44), RGBY(0x45), RGBY(0x46), RGBY(0x47),
  RGBY(0x48), RGBY(0x49), RGBY(0x4A), RGBY(0x4B),
  RGBY(0x4C), RGBY(0x4D), RGBY(0x4E), RGBY(0x4F),
  RGBY(0x50), RGBY(0x51), RGBY(0x52), RGBY(0x53),
  RGBY(0x54), RGBY(0x55), RGBY(0x56), RGBY(0x57),
  RGBY(0x58), RGBY(0x59), RGBY(0x5A), RGBY(0x5B),
  RGBY(0x5C), RGBY(0x5D), RGBY(0x5E), RGBY(0x5F),
  RGBY(0x60), RGBY(0x61), RGBY(0x62), RGBY(0x63),
  RGBY(0x64), RGBY(0x65), RGBY(0x66), RGBY(0x67),
  RGBY(0x68), RGBY(0x69), RGBY(0x6A), RGBY(0x6B),
  RGBY(0x6C), RGBY(0x6D), RGBY(0x6E), RGBY(0x6F),
  RGBY(0x70), RGBY(0x71), RGBY(0x72), RGBY(0x73),
  RGBY(0x74), RGBY(0x75), RGBY(0x76), RGBY(0x77),
  RGBY(0x78), RGBY(0x79), RGBY(0x7A), RGBY(0x7B),
  RGBY(0x7C), RGBY(0x7D), RGBY(0x7E), RGBY(0x7F),
  RGBY(0x80), RGBY(0x81), RGBY(0x82), RGBY(0x83),
  RGBY(0x84), RGBY(0x85), RGBY(0x86), RGBY(0x87),
  RGBY(0x88), RGBY(0x89), RGBY(0x8A), RGBY(0x8B),
  RGBY(0x8C), RGBY(0x8D), RGBY(0x8E), RGBY(0x8F),
  RGBY(0x90), RGBY(0x91), RGBY(0x92), RGBY(0x93),
  RGBY(0x94), RGBY(0x95), RGBY(0x96), RGBY(0x97),
  RGBY(0x98), RGBY(0x99), RGBY(0x9A), RGBY(0x9B),
  RGBY(0x9C), RGBY(0x9D), RGBY(0x9E), RGBY(0x9F),
  RGBY(0xA0), RGBY(0xA1), RGBY(0xA2), RGBY(0xA3),
  RGBY(0xA4), RGBY(0xA5), RGBY(0xA6), RGBY(0xA7),
  RGBY(0xA8), RGBY(0xA9), RGBY(0xAA), RGBY(0xAB),
  RGBY(0xAC), RGBY(0xAD), RGBY(0xAE), RGBY(0xAF),
  RGBY(0xB0), RGBY(0xB1), RGBY(0xB2), RGBY(0xB3),
  RGBY(0xB4), RGBY(0xB5), RGBY(0xB6), RGBY(0xB7),
  RGBY(0xB8), RGBY(0xB9), RGBY(0xBA), RGBY(0xBB),
  RGBY(0xBC), RGBY(0xBD), RGBY(0xBE), RGBY(0xBF),
  RGBY(0xC0), RGBY(0xC1), RGBY(0xC2), RGBY(0xC3),
  RGBY(0xC4), RGBY(0xC5), RGBY(0xC6), RGBY(0xC7),
  RGBY(0xC8), RGBY(0xC9), RGBY(0xCA), RGBY(0xCB),
  RGBY(0xCC), RGBY(0xCD), RGBY(0xCE), RGBY(0xCF),
  RGBY(0xD0), RGBY(0xD1), RGBY(0xD2), RGBY(0xD3),
  RGBY(0xD4), RGBY(0xD5), RGBY(0xD6), RGBY(0xD7),
  RGBY(0xD8), RGBY(0xD9), RGBY(0xDA), RGBY(0xDB),
  RGBY(0xDC), RGBY(0xDD), RGBY(0xDE), RGBY(0xDF),
  RGBY(0xE0), RGBY(0xE1), RGBY(0xE2), RGBY(0xE3),
  RGBY(0xE4), RGBY(0xE5), RGBY(0xE6), RGBY(0xE7),
  RGBY(0xE8), RGBY(0xE9), RGBY(0xEA), RGBY(0xEB),
  RGBY(0xEC), RGBY(0xED), RGBY(0xEE), RGBY(0xEF),
  RGBY(0xF0), RGBY(0xF1), RGBY(0xF2), RGBY(0xF3),
  RGBY(0xF4), RGBY(0xF5), RGBY(0xF6), RGBY(0xF7),
  RGBY(0xF8), RGBY(0xF9), RGBY(0xFA), RGBY(0xFB),
  RGBY(0xFC), RGBY(0xFD), RGBY(0xFE), RGBY(0xFF),
};

MMX_ALIGNED(int16 coefficients_RGB_U[256][4]) = {
  RGBU(0x00), RGBU(0x01), RGBU(0x02), RGBU(0x03),
  RGBU(0x04), RGBU(0x05), RGBU(0x06), RGBU(0x07),
  RGBU(0x08), RGBU(0x09), RGBU(0x0A), RGBU(0x0B),
  RGBU(0x0C), RGBU(0x0D), RGBU(0x0E), RGBU(0x0F),
  RGBU(0x10), RGBU(0x11), RGBU(0x12), RGBU(0x13),
  RGBU(0x14), RGBU(0x15), RGBU(0x16), RGBU(0x17),
  RGBU(0x18), RGBU(0x19), RGBU(0x1A), RGBU(0x1B),
  RGBU(0x1C), RGBU(0x1D), RGBU(0x1E), RGBU(0x1F),
  RGBU(0x20), RGBU(0x21), RGBU(0x22), RGBU(0x23),
  RGBU(0x24), RGBU(0x25), RGBU(0x26), RGBU(0x27),
  RGBU(0x28), RGBU(0x29), RGBU(0x2A), RGBU(0x2B),
  RGBU(0x2C), RGBU(0x2D), RGBU(0x2E), RGBU(0x2F),
  RGBU(0x30), RGBU(0x31), RGBU(0x32), RGBU(0x33),
  RGBU(0x34), RGBU(0x35), RGBU(0x36), RGBU(0x37),
  RGBU(0x38), RGBU(0x39), RGBU(0x3A), RGBU(0x3B),
  RGBU(0x3C), RGBU(0x3D), RGBU(0x3E), RGBU(0x3F),
  RGBU(0x40), RGBU(0x41), RGBU(0x42), RGBU(0x43),
  RGBU(0x44), RGBU(0x45), RGBU(0x46), RGBU(0x47),
  RGBU(0x48), RGBU(0x49), RGBU(0x4A), RGBU(0x4B),
  RGBU(0x4C), RGBU(0x4D), RGBU(0x4E), RGBU(0x4F),
  RGBU(0x50), RGBU(0x51), RGBU(0x52), RGBU(0x53),
  RGBU(0x54), RGBU(0x55), RGBU(0x56), RGBU(0x57),
  RGBU(0x58), RGBU(0x59), RGBU(0x5A), RGBU(0x5B),
  RGBU(0x5C), RGBU(0x5D), RGBU(0x5E), RGBU(0x5F),
  RGBU(0x60), RGBU(0x61), RGBU(0x62), RGBU(0x63),
  RGBU(0x64), RGBU(0x65), RGBU(0x66), RGBU(0x67),
  RGBU(0x68), RGBU(0x69), RGBU(0x6A), RGBU(0x6B),
  RGBU(0x6C), RGBU(0x6D), RGBU(0x6E), RGBU(0x6F),
  RGBU(0x70), RGBU(0x71), RGBU(0x72), RGBU(0x73),
  RGBU(0x74), RGBU(0x75), RGBU(0x76), RGBU(0x77),
  RGBU(0x78), RGBU(0x79), RGBU(0x7A), RGBU(0x7B),
  RGBU(0x7C), RGBU(0x7D), RGBU(0x7E), RGBU(0x7F),
  RGBU(0x80), RGBU(0x81), RGBU(0x82), RGBU(0x83),
  RGBU(0x84), RGBU(0x85), RGBU(0x86), RGBU(0x87),
  RGBU(0x88), RGBU(0x89), RGBU(0x8A), RGBU(0x8B),
  RGBU(0x8C), RGBU(0x8D), RGBU(0x8E), RGBU(0x8F),
  RGBU(0x90), RGBU(0x91), RGBU(0x92), RGBU(0x93),
  RGBU(0x94), RGBU(0x95), RGBU(0x96), RGBU(0x97),
  RGBU(0x98), RGBU(0x99), RGBU(0x9A), RGBU(0x9B),
  RGBU(0x9C), RGBU(0x9D), RGBU(0x9E), RGBU(0x9F),
  RGBU(0xA0), RGBU(0xA1), RGBU(0xA2), RGBU(0xA3),
  RGBU(0xA4), RGBU(0xA5), RGBU(0xA6), RGBU(0xA7),
  RGBU(0xA8), RGBU(0xA9), RGBU(0xAA), RGBU(0xAB),
  RGBU(0xAC), RGBU(0xAD), RGBU(0xAE), RGBU(0xAF),
  RGBU(0xB0), RGBU(0xB1), RGBU(0xB2), RGBU(0xB3),
  RGBU(0xB4), RGBU(0xB5), RGBU(0xB6), RGBU(0xB7),
  RGBU(0xB8), RGBU(0xB9), RGBU(0xBA), RGBU(0xBB),
  RGBU(0xBC), RGBU(0xBD), RGBU(0xBE), RGBU(0xBF),
  RGBU(0xC0), RGBU(0xC1), RGBU(0xC2), RGBU(0xC3),
  RGBU(0xC4), RGBU(0xC5), RGBU(0xC6), RGBU(0xC7),
  RGBU(0xC8), RGBU(0xC9), RGBU(0xCA), RGBU(0xCB),
  RGBU(0xCC), RGBU(0xCD), RGBU(0xCE), RGBU(0xCF),
  RGBU(0xD0), RGBU(0xD1), RGBU(0xD2), RGBU(0xD3),
  RGBU(0xD4), RGBU(0xD5), RGBU(0xD6), RGBU(0xD7),
  RGBU(0xD8), RGBU(0xD9), RGBU(0xDA), RGBU(0xDB),
  RGBU(0xDC), RGBU(0xDD), RGBU(0xDE), RGBU(0xDF),
  RGBU(0xE0), RGBU(0xE1), RGBU(0xE2), RGBU(0xE3),
  RGBU(0xE4), RGBU(0xE5), RGBU(0xE6), RGBU(0xE7),
  RGBU(0xE8), RGBU(0xE9), RGBU(0xEA), RGBU(0xEB),
  RGBU(0xEC), RGBU(0xED), RGBU(0xEE), RGBU(0xEF),
  RGBU(0xF0), RGBU(0xF1), RGBU(0xF2), RGBU(0xF3),
  RGBU(0xF4), RGBU(0xF5), RGBU(0xF6), RGBU(0xF7),
  RGBU(0xF8), RGBU(0xF9), RGBU(0xFA), RGBU(0xFB),
  RGBU(0xFC), RGBU(0xFD), RGBU(0xFE), RGBU(0xFF),
};

MMX_ALIGNED(int16 coefficients_RGB_V[256][4]) = {
  RGBV(0x00), RGBV(0x01), RGBV(0x02), RGBV(0x03),
  RGBV(0x04), RGBV(0x05), RGBV(0x06), RGBV(0x07),
  RGBV(0x08), RGBV(0x09), RGBV(0x0A), RGBV(0x0B),
  RGBV(0x0C), RGBV(0x0D), RGBV(0x0E), RGBV(0x0F),
  RGBV(0x10), RGBV(0x11), RGBV(0x12), RGBV(0x13),
  RGBV(0x14), RGBV(0x15), RGBV(0x16), RGBV(0x17),
  RGBV(0x18), RGBV(0x19), RGBV(0x1A), RGBV(0x1B),
  RGBV(0x1C), RGBV(0x1D), RGBV(0x1E), RGBV(0x1F),
  RGBV(0x20), RGBV(0x21), RGBV(0x22), RGBV(0x23),
  RGBV(0x24), RGBV(0x25), RGBV(0x26), RGBV(0x27),
  RGBV(0x28), RGBV(0x29), RGBV(0x2A), RGBV(0x2B),
  RGBV(0x2C), RGBV(0x2D), RGBV(0x2E), RGBV(0x2F),
  RGBV(0x30), RGBV(0x31), RGBV(0x32), RGBV(0x33),
  RGBV(0x34), RGBV(0x35), RGBV(0x36), RGBV(0x37),
  RGBV(0x38), RGBV(0x39), RGBV(0x3A), RGBV(0x3B),
  RGBV(0x3C), RGBV(0x3D), RGBV(0x3E), RGBV(0x3F),
  RGBV(0x40), RGBV(0x41), RGBV(0x42), RGBV(0x43),
  RGBV(0x44), RGBV(0x45), RGBV(0x46), RGBV(0x47),
  RGBV(0x48), RGBV(0x49), RGBV(0x4A), RGBV(0x4B),
  RGBV(0x4C), RGBV(0x4D), RGBV(0x4E), RGBV(0x4F),
  RGBV(0x50), RGBV(0x51), RGBV(0x52), RGBV(0x53),
  RGBV(0x54), RGBV(0x55), RGBV(0x56), RGBV(0x57),
  RGBV(0x58), RGBV(0x59), RGBV(0x5A), RGBV(0x5B),
  RGBV(0x5C), RGBV(0x5D), RGBV(0x5E), RGBV(0x5F),
  RGBV(0x60), RGBV(0x61), RGBV(0x62), RGBV(0x63),
  RGBV(0x64), RGBV(0x65), RGBV(0x66), RGBV(0x67),
  RGBV(0x68), RGBV(0x69), RGBV(0x6A), RGBV(0x6B),
  RGBV(0x6C), RGBV(0x6D), RGBV(0x6E), RGBV(0x6F),
  RGBV(0x70), RGBV(0x71), RGBV(0x72), RGBV(0x73),
  RGBV(0x74), RGBV(0x75), RGBV(0x76), RGBV(0x77),
  RGBV(0x78), RGBV(0x79), RGBV(0x7A), RGBV(0x7B),
  RGBV(0x7C), RGBV(0x7D), RGBV(0x7E), RGBV(0x7F),
  RGBV(0x80), RGBV(0x81), RGBV(0x82), RGBV(0x83),
  RGBV(0x84), RGBV(0x85), RGBV(0x86), RGBV(0x87),
  RGBV(0x88), RGBV(0x89), RGBV(0x8A), RGBV(0x8B),
  RGBV(0x8C), RGBV(0x8D), RGBV(0x8E), RGBV(0x8F),
  RGBV(0x90), RGBV(0x91), RGBV(0x92), RGBV(0x93),
  RGBV(0x94), RGBV(0x95), RGBV(0x96), RGBV(0x97),
  RGBV(0x98), RGBV(0x99), RGBV(0x9A), RGBV(0x9B),
  RGBV(0x9C), RGBV(0x9D), RGBV(0x9E), RGBV(0x9F),
  RGBV(0xA0), RGBV(0xA1), RGBV(0xA2), RGBV(0xA3),
  RGBV(0xA4), RGBV(0xA5), RGBV(0xA6), RGBV(0xA7),
  RGBV(0xA8), RGBV(0xA9), RGBV(0xAA), RGBV(0xAB),
  RGBV(0xAC), RGBV(0xAD), RGBV(0xAE), RGBV(0xAF),
  RGBV(0xB0), RGBV(0xB1), RGBV(0xB2), RGBV(0xB3),
  RGBV(0xB4), RGBV(0xB5), RGBV(0xB6), RGBV(0xB7),
  RGBV(0xB8), RGBV(0xB9), RGBV(0xBA), RGBV(0xBB),
  RGBV(0xBC), RGBV(0xBD), RGBV(0xBE), RGBV(0xBF),
  RGBV(0xC0), RGBV(0xC1), RGBV(0xC2), RGBV(0xC3),
  RGBV(0xC4), RGBV(0xC5), RGBV(0xC6), RGBV(0xC7),
  RGBV(0xC8), RGBV(0xC9), RGBV(0xCA), RGBV(0xCB),
  RGBV(0xCC), RGBV(0xCD), RGBV(0xCE), RGBV(0xCF),
  RGBV(0xD0), RGBV(0xD1), RGBV(0xD2), RGBV(0xD3),
  RGBV(0xD4), RGBV(0xD5), RGBV(0xD6), RGBV(0xD7),
  RGBV(0xD8), RGBV(0xD9), RGBV(0xDA), RGBV(0xDB),
  RGBV(0xDC), RGBV(0xDD), RGBV(0xDE), RGBV(0xDF),
  RGBV(0xE0), RGBV(0xE1), RGBV(0xE2), RGBV(0xE3),
  RGBV(0xE4), RGBV(0xE5), RGBV(0xE6), RGBV(0xE7),
  RGBV(0xE8), RGBV(0xE9), RGBV(0xEA), RGBV(0xEB),
  RGBV(0xEC), RGBV(0xED), RGBV(0xEE), RGBV(0xEF),
  RGBV(0xF0), RGBV(0xF1), RGBV(0xF2), RGBV(0xF3),
  RGBV(0xF4), RGBV(0xF5), RGBV(0xF6), RGBV(0xF7),
  RGBV(0xF8), RGBV(0xF9), RGBV(0xFA), RGBV(0xFB),
  RGBV(0xFC), RGBV(0xFD), RGBV(0xFE), RGBV(0xFF),
};

#undef RGBY
#undef RGBU
#undef RGBV
#undef MMX_ALIGNED

#if defined(_MSC_VER)

// Warning C4799: function has no EMMS instruction.
#pragma warning(disable: 4799)

__declspec(naked)
void ConvertYV12ToRGB32Row(const uint8* y_buf,
                           const uint8* u_buf,
                           const uint8* v_buf,
                           uint8* rgb_buf,
                           size_t width) {
  __asm {
    pushad
    mov       edx, [esp + 32 + 4]   // Y
    mov       edi, [esp + 32 + 8]   // U
    mov       esi, [esp + 32 + 12]  // V
    mov       ebp, [esp + 32 + 16]  // rgb
    mov       ecx, [esp + 32 + 20]  // width
    shr       ecx, 1

 wloop :
    movzx     eax, byte ptr [edi]
    add       edi, 1
    movzx     ebx, byte ptr [esi]
    add       esi, 1
    movq      mm0, [coefficients_RGB_U + 8 * eax]
    movzx     eax, byte ptr [edx]
    paddsw    mm0, [coefficients_RGB_V + 8 * ebx]
    movzx     ebx, byte ptr [edx + 1]
    movq      mm1, [coefficients_RGB_Y + 8 * eax]
    add       edx, 2
    movq      mm2, [coefficients_RGB_Y + 8 * ebx]
    paddsw    mm1, mm0
    paddsw    mm2, mm0
    psraw     mm1, 6
    psraw     mm2, 6
    packuswb  mm1, mm2
    movntq    [ebp], mm1  // NOLINT
    add       ebp, 8
    sub       ecx, 1
    jnz       wloop

    popad
    ret
  }
}


#elif defined(__linux)

void ConvertYV12ToRGB32Row(const uint8* y_buf,
                           const uint8* u_buf,
                           const uint8* v_buf,
                           uint8* rgb_buf,
                           size_t width) __attribute__((noinline));

  asm(
"ConvertYV12ToRGB32Row:\n"
  "pusha\n"
  "mov    0x24(%esp),%edx\n"
  "mov    0x28(%esp),%edi\n"
  "mov    0x2c(%esp),%esi\n"
  "mov    0x30(%esp),%ebp\n"
  "mov    0x34(%esp),%ecx\n"
  "shr    %ecx\n"
"1:\n"
  "movzx  byte ptr (%edi),%eax\n"
  "add    $0x1,%edi\n"
  "movzx  byte ptr (%esi),%ebx\n"
  "add    $0x1,%esi\n"
  "movq   coefficients_RGB_U(,%eax,8),%mm0\n"
  "movzx  byte ptr (%edx),%eax\n"
  "paddsw coefficients_RGB_V(,%ebx,8),%mm0\n"
  "movzx  byte ptr 0x1(%edx),%ebx\n"
  "movq   coefficients_RGB_Y(,%eax,8),%mm1\n"
  "add    $0x2,%edx\n"
  "movq   coefficients_RGB_Y(,%ebx,8),%mm2\n"
  "paddsw %mm0,%mm1\n"
  "paddsw %mm0,%mm2\n"
  "psraw  $0x6,%mm1\n"
  "psraw  $0x6,%mm2\n"
  "packuswb %mm2,%mm1\n"
  "movntq %mm1,0x0(%ebp)\n"
  "add    $0x8,%ebp\n"
  "sub    $0x1,%ecx\n"
  "jne    1b\n"
  "popa\n"
  "ret\n"
);

#else


void ConvertYV12ToRGB32Row(const uint8* y_buf,
                           const uint8* u_buf,
                           const uint8* v_buf,
                           uint8* rgb_buf,
                           size_t width) __attribute__((noinline));

  asm(
"_ConvertYV12ToRGB32Row:\n"
  "pusha\n"
  "mov    0x24(%esp),%edx\n"
  "mov    0x28(%esp),%edi\n"
  "mov    0x2c(%esp),%esi\n"
  "mov    0x30(%esp),%ebp\n"
  "mov    0x34(%esp),%ecx\n"
  "shr    %ecx\n"
  "xor    %eax,%eax\n"
  "xor    %ebx,%ebx\n"
"1:\n"
  "movzx  byte ptr (%edi),%eax\n"
  "add    $0x1,%edi\n"
  "movzx  byte ptr (%esi),%ebx\n"
  "add    $0x1,%esi\n"
  "movq   _coefficients_RGB_U(,%eax,8),%mm0\n"
  "movzx  byte ptr (%edx),%eax\n"
  "paddsw _coefficients_RGB_V(,%ebx,8),%mm0\n"
  "movzx  byte ptr 0x1(%edx),%ebx\n"
  "movq   _coefficients_RGB_Y(,%eax,8),%mm1\n"
  "add    $0x2,%edx\n"
  "movq   _coefficients_RGB_Y(,%ebx,8),%mm2\n"
  "paddsw %mm0,%mm1\n"
  "paddsw %mm0,%mm2\n"
  "psraw  $0x6,%mm1\n"
  "psraw  $0x6,%mm2\n"
  "packuswb %mm2,%mm1\n"
  "movntq %mm1,0x0(%ebp)\n"
  "add    $0x8,%ebp\n"
  "sub    $0x1,%ecx\n"
  "jne    1b\n"
  "popa\n"
  "ret\n"
);

#endif  // MSC_VER
}  // extern "C"

#else  // USE_MMX

void ConvertYV12ToRGB32Row(const uint8* y_buf,
                           const uint8* u_buf,
                           const uint8* v_buf,
                           uint8* rgb_buf,
                           size_t width);
#endif

// Convert a frame of YUV to 32 bit ARGB.
void ConvertYV12ToRGB32(const uint8* y_buf,
                        const uint8* u_buf,
                        const uint8* v_buf,
                        uint8* rgb_buf,
                        size_t width,
                        size_t height,
                        int y_pitch,
                        int uv_pitch,
                        int rgb_pitch) {
  // Image must be multiple of 2 in width.
  DCHECK((width & 1) == 0);
  // Check alignment. Use memalign to allocate the buffer if you hit this
  // check:
  DCHECK((reinterpret_cast<uintptr_t>(rgb_buf) & 7) == 0);
#ifdef _OPENMP
#pragma omp parallel for
#endif
  for (int y = 0; y < static_cast<int>(height); ++y) {
    uint8* d1 = rgb_buf + y * rgb_pitch;
    const uint8* y_ptr = y_buf + y * y_pitch;
    const uint8* u_ptr = u_buf + y/2 * uv_pitch;
    const uint8* v_ptr = v_buf + y/2 * uv_pitch;

    ConvertYV12ToRGB32Row(y_ptr,
                          u_ptr,
                          v_ptr,
                          d1,
                          width);
  }
#if USE_MMX
#if defined(_MSC_VER)
  __asm emms;
#else
  asm("emms");
#endif
#endif
}

//------------------------------------------------------------------------------
// This is pure C code

#if !USE_MMX

// Reference version of YUV converter.
static const int kClipTableSize = 256;
static const int kClipOverflow = 128;

static uint8 g_rgb_clip_table[kClipOverflow
                            + kClipTableSize
                            + kClipOverflow] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // 128 underflow values
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // clamped to 0.
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,  // Unclipped values.
  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
  0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
  0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
  0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
  0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
  0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
  0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
  0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
  0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
  0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
  0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
  0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
  0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
  0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
  0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
  0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
  0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
  0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
  0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7,
  0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
  0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
  0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
  0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
  0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // 128 overflow values
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // clamped to 255.
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

// Clip an rgb channel value to 0..255 range.
// Source is signed fixed point 8.8.
// Table allows for values to underflow or overflow by 128.
// Therefore source range is -128 to 384.
// Output clamps to unsigned 0 to 255.
static inline uint32 clip(int32 value) {
  DCHECK((((value) >> 8) + kClipOverflow) >= 0);
  DCHECK((((value) >> 8) + kClipOverflow)
         < kClipOverflow + kClipTableSize + kClipOverflow);
  return static_cast<uint32>(g_rgb_clip_table[((value) >> 8) + kClipOverflow]);
}

void ConvertYV12ToRGB32Row(const uint8* y_buf,
                           const uint8* u_buf,
                           const uint8* v_buf,
                           uint8* rgb_buf,
                           size_t width) {
  for (int32 x = 0; x < static_cast<int32>(width); x += 2) {
    uint8 u = u_buf[x >> 1];
    uint8 v = v_buf[x >> 1];
    int32 D = static_cast<int32>(u) - 128;
    int32 E = static_cast<int32>(v) - 128;

    int32 Cb =   (516 * D + 128);
    int32 Cg = (- 100 * D - 208 * E + 128);
    int32 Cr =             (409 * E + 128);

    uint8 y0 = y_buf[x];
    int32 C298a = ((static_cast<int32>(y0) - 16) * 298 + 128);
    *reinterpret_cast<uint32*>(rgb_buf) = clip(C298a + Cb)
                                       | (clip(C298a + Cg) << 8)
                                       | (clip(C298a + Cr) << 16)
                                       | 0xff000000;

    uint8 y1 = y_buf[x + 1];
    int32 C298b = ((static_cast<int32>(y1) - 16) * 298 + 128);
    *reinterpret_cast<uint32*>(rgb_buf + 4) = clip(C298b + Cb)
                                           | (clip(C298b + Cg) << 8)
                                           | (clip(C298b + Cr) << 16)
                                           | 0xff000000;

    rgb_buf += 8;  // Advance 2 pixels.
  }
}
#endif


}  // namespace media

