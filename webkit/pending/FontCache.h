/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FontCache_h
#define FontCache_h

#include <unicode/uscript.h>
#include <wtf/unicode/Unicode.h>

#if PLATFORM(WIN)
#include <objidl.h>
#include <mlang.h>
#endif

namespace WebCore
{

class AtomicString;
class Font;
class FontPlatformData;
class FontData;
class FontDescription;
class FontSelector;
class SimpleFontData;

class FontCache {
public:
    static const FontData* getFontData(const Font&, int& familyIndex, FontSelector*);
    
    // This method is implemented by the platform.
    static const SimpleFontData* getFontDataForCharacters(const Font&, const UChar* characters, int length);
    
    // Also implemented by the platform.
    static void platformInit();

#if PLATFORM(WIN)
    static IMLangFontLink2* getFontLinkInterface();
#endif

    static bool fontExists(const FontDescription&, const AtomicString& family);

    static FontPlatformData* getCachedFontPlatformData(const FontDescription&, const AtomicString& family, bool checkingAlternateName = false);
    static SimpleFontData* getCachedFontData(const FontPlatformData*);
    static FontPlatformData* getLastResortFallbackFont(const FontDescription&);

    // TODO(jungshik): Is this the best place to put this function? It may
    // or may not be. Font.h is another place we can cosider.
    // Return a font family for |script| and |FontDescription.genericFamily()|.
    // It will return an empty atom if we can't find a font matching 
    // script and genericFamily in FontDescription. A caller should check
    // the emptyness before using it.
    static AtomicString getGenericFontForScript(UScriptCode script, const FontDescription&);
    
private:
    // These methods are implemented by each platform.
    static FontPlatformData* getSimilarFontPlatformData(const Font&);
    static FontPlatformData* createFontPlatformData(const FontDescription&, const AtomicString& family);
    static const AtomicString& alternateFamilyName(const AtomicString& family);

    friend class SimpleFontData;
    friend class FontFallbackList;
};

}

#endif
