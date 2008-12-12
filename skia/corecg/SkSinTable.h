/* libs/corecg/SkSinTable.h
**
** Copyright 2006, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

#ifndef SkSinTable_DEFINED
#define SkSinTable_DEFINED

#include "SkTypes.h"

/* Fixed point values (low 16 bits) of sin(radians) for
    radians in [0...PI/2)
*/
static const uint16_t gSkSinTable[256] = {
    0x0000,
    0x0192,
    0x0324,
    0x04B6,
    0x0648,
    0x07DA,
    0x096C,
    0x0AFE,
    0x0C8F,
    0x0E21,
    0x0FB2,
    0x1144,
    0x12D5,
    0x1466,
    0x15F6,
    0x1787,
    0x1917,
    0x1AA7,
    0x1C37,
    0x1DC7,
    0x1F56,
    0x20E5,
    0x2273,
    0x2402,
    0x2590,
    0x271D,
    0x28AA,
    0x2A37,
    0x2BC4,
    0x2D50,
    0x2EDB,
    0x3066,
    0x31F1,
    0x337B,
    0x3505,
    0x368E,
    0x3817,
    0x399F,
    0x3B26,
    0x3CAD,
    0x3E33,
    0x3FB9,
    0x413E,
    0x42C3,
    0x4447,
    0x45CA,
    0x474D,
    0x48CE,
    0x4A50,
    0x4BD0,
    0x4D50,
    0x4ECF,
    0x504D,
    0x51CA,
    0x5347,
    0x54C3,
    0x563E,
    0x57B8,
    0x5931,
    0x5AAA,
    0x5C22,
    0x5D98,
    0x5F0E,
    0x6083,
    0x61F7,
    0x636A,
    0x64DC,
    0x664D,
    0x67BD,
    0x692D,
    0x6A9B,
    0x6C08,
    0x6D74,
    0x6EDF,
    0x7049,
    0x71B1,
    0x7319,
    0x7480,
    0x75E5,
    0x774A,
    0x78AD,
    0x7A0F,
    0x7B70,
    0x7CD0,
    0x7E2E,
    0x7F8B,
    0x80E7,
    0x8242,
    0x839C,
    0x84F4,
    0x864B,
    0x87A1,
    0x88F5,
    0x8A48,
    0x8B9A,
    0x8CEA,
    0x8E39,
    0x8F87,
    0x90D3,
    0x921E,
    0x9368,
    0x94B0,
    0x95F6,
    0x973C,
    0x987F,
    0x99C2,
    0x9B02,
    0x9C42,
    0x9D7F,
    0x9EBC,
    0x9FF6,
    0xA12F,
    0xA267,
    0xA39D,
    0xA4D2,
    0xA605,
    0xA736,
    0xA866,
    0xA994,
    0xAAC0,
    0xABEB,
    0xAD14,
    0xAE3B,
    0xAF61,
    0xB085,
    0xB1A8,
    0xB2C8,
    0xB3E7,
    0xB504,
    0xB620,
    0xB73A,
    0xB852,
    0xB968,
    0xBA7C,
    0xBB8F,
    0xBCA0,
    0xBDAE,
    0xBEBC,
    0xBFC7,
    0xC0D0,
    0xC1D8,
    0xC2DE,
    0xC3E2,
    0xC4E3,
    0xC5E4,
    0xC6E2,
    0xC7DE,
    0xC8D8,
    0xC9D1,
    0xCAC7,
    0xCBBB,
    0xCCAE,
    0xCD9F,
    0xCE8D,
    0xCF7A,
    0xD064,
    0xD14D,
    0xD233,
    0xD318,
    0xD3FA,
    0xD4DB,
    0xD5B9,
    0xD695,
    0xD770,
    0xD848,
    0xD91E,
    0xD9F2,
    0xDAC4,
    0xDB94,
    0xDC61,
    0xDD2D,
    0xDDF6,
    0xDEBE,
    0xDF83,
    0xE046,
    0xE106,
    0xE1C5,
    0xE282,
    0xE33C,
    0xE3F4,
    0xE4AA,
    0xE55E,
    0xE60F,
    0xE6BE,
    0xE76B,
    0xE816,
    0xE8BF,
    0xE965,
    0xEA09,
    0xEAAB,
    0xEB4B,
    0xEBE8,
    0xEC83,
    0xED1C,
    0xEDB2,
    0xEE46,
    0xEED8,
    0xEF68,
    0xEFF5,
    0xF080,
    0xF109,
    0xF18F,
    0xF213,
    0xF294,
    0xF314,
    0xF391,
    0xF40B,
    0xF484,
    0xF4FA,
    0xF56D,
    0xF5DE,
    0xF64D,
    0xF6BA,
    0xF724,
    0xF78B,
    0xF7F1,
    0xF853,
    0xF8B4,
    0xF912,
    0xF96E,
    0xF9C7,
    0xFA1E,
    0xFA73,
    0xFAC5,
    0xFB14,
    0xFB61,
    0xFBAC,
    0xFBF5,
    0xFC3B,
    0xFC7E,
    0xFCBF,
    0xFCFE,
    0xFD3A,
    0xFD74,
    0xFDAB,
    0xFDE0,
    0xFE13,
    0xFE43,
    0xFE70,
    0xFE9B,
    0xFEC4,
    0xFEEA,
    0xFF0E,
    0xFF2F,
    0xFF4E,
    0xFF6A,
    0xFF84,
    0xFF9C,
    0xFFB1,
    0xFFC3,
    0xFFD3,
    0xFFE1,
    0xFFEC,
    0xFFF4,
    0xFFFB,
    0xFFFE
};

#endif
