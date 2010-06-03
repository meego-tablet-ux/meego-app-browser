/*
 * libjingle
 * Copyright 2004--2005, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products 
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <cstdlib>
#include <cstring>

#include "talk/base/time.h"

namespace talk_base {

#ifdef POSIX
#include <sys/time.h>
uint32 Time() {
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
#endif

#ifdef WIN32
#include <windows.h>
uint32 Time() {
  return GetTickCount();
}
#endif

uint32 StartTime() {
  // Close to program execution time
  static const uint32 g_start = Time();
  return g_start;
}

// Make sure someone calls it so that it gets initialized
static uint32 ignore = StartTime();

uint32 ElapsedTime() {
  return TimeDiff(Time(), StartTime());
}

bool TimeIsBetween(uint32 later, uint32 middle, uint32 earlier) {
  if (earlier <= later) {
    return ((earlier <= middle) && (middle <= later));
  } else {
    return !((later < middle) && (middle < earlier));
  }
}

int32 TimeDiff(uint32 later, uint32 earlier) {
  uint32 LAST = 0xFFFFFFFF;
  uint32 HALF = 0x80000000;
  if (TimeIsBetween(earlier + HALF, later, earlier)) {
    if (earlier <= later) {
      return static_cast<long>(later - earlier);
    } else {
      return static_cast<long>(later + (LAST - earlier) + 1);
    }
  } else {
    if (later <= earlier) {
      return -static_cast<long>(earlier - later);
    } else {
      return -static_cast<long>(earlier + (LAST - later) + 1);
    }
  }
}

} // namespace talk_base
