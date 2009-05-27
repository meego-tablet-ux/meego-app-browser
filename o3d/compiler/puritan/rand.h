/*
 * Copyright 2009, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef PURITAN_RAND_H
#define PURITAN_RAND_H

// Use a strong RNG if available.
// TODO: Investigate using openssl on all platforms.
#ifdef _WIN32
#include <windows.h>
#include <wincrypt.h>
#else
typedef void* HCRYPTPROV;
#endif


namespace Salem 
{
namespace Puritan
{

class Rand 
{
public:
    unsigned y;
    unsigned rnd_uint(void) ;
    Rand(unsigned seed);
    double rnd_flt(void);
    int range(int lo, int hi);
    unsigned urange(unsigned lo, unsigned hi);
    size_t srange(size_t lo, size_t hi);
    const char *from_list (const char **);

private:
    bool InitializeProvider();

    HCRYPTPROV crypt_provider_;
    static const size_t kCacheSize = 0x1000;
    unsigned int cached_numbers_[kCacheSize];
    long available_;
};
}
}
#endif
