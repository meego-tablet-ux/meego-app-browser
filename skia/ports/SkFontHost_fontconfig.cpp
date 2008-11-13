/* libs/graphics/ports/SkFontHost_fontconfig.cpp
**
** Copyright 2008, Google Inc.
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

// -----------------------------------------------------------------------------
// This file provides implementations of the font resolution members of
// SkFontHost by using the fontconfig[1] library. Fontconfig is usually found
// on Linux systems and handles configuration, parsing and caching issues
// involved with enumerating and matching fonts.
//
// [1] http://fontconfig.org
// -----------------------------------------------------------------------------

#include <map>
#include <string>

#include <fontconfig/fontconfig.h>

#include "SkDescriptor.h"
#include "SkFontHost.h"
#include "SkMMapStream.h"
#include "SkPaint.h"
#include "SkStream.h"
#include "SkString.h"
#include "SkThread.h"
#include "SkTSearch.h"

// This is an extern from SkFontHost_FreeType
SkTypeface::Style find_name_and_style(SkStream* stream, SkString* name);

// -----------------------------------------------------------------------------
// The rest of Skia requires that fonts be identified by a unique unsigned id
// and that we be able to load them given the id. What we actually get from
// fontconfig is the filename of the font so we keep a locked map from
// filenames to fileid numbers and back.
//
// Note that there's also a unique id in the SkTypeface. This is unique over
// both filename and style. Thus we encode that id as (fileid << 8) | style.
// Although truetype fonts can support multiple faces in a single file, at the
// moment Skia doesn't.
// -----------------------------------------------------------------------------
static SkMutex global_fc_map_lock;
static std::map<std::string, unsigned> global_fc_map;
static std::map<unsigned, std::string> global_fc_map_inverted;
static unsigned global_fc_map_next_id = 0;

// This is the maximum size of the font cache.
static const unsigned kFontCacheMemoryBudget = 2 * 1024 * 1024;  // 2MB

static unsigned UniqueIdToFileId(unsigned uniqueid)
{
    return uniqueid >> 8;
}

static SkTypeface::Style UniqueIdToStyle(unsigned uniqueid)
{
    return static_cast<SkTypeface::Style>(uniqueid & 0xff);
}

static unsigned FileIdAndStyleToUniqueId(unsigned fileid,
                                         SkTypeface::Style style)
{
    SkASSERT(style & 0xff == style);
    return (fileid << 8) | static_cast<int>(style);
}

class FontConfigTypeface : public SkTypeface {
public:
    FontConfigTypeface(Style style, uint32_t id)
        : SkTypeface(style, id)
    { }
};

// -----------------------------------------------------------------------------
// Find a matching font where @type (one of FC_*) is equal to @value. For a
// list of types, see http://fontconfig.org/fontconfig-devel/x19.html#AEN27.
// The variable arguments are a list of triples, just like the first three
// arguments, and must be NULL terminated.
//
// For example, FontMatchString(FC_FILE, FcTypeString,
//                              "/usr/share/fonts/myfont.ttf", NULL);
// -----------------------------------------------------------------------------
static FcPattern* FontMatch(const char* type, FcType vtype, const void* value,
                            ...)
{
    va_list ap;
    va_start(ap, value);

    FcPattern* pattern = FcPatternCreate();

    for (;;) {
        FcValue fcvalue;
        fcvalue.type = vtype;
        switch (vtype) {
            case FcTypeString:
                fcvalue.u.s = (FcChar8*) value;
                break;
            case FcTypeInteger:
                fcvalue.u.i = (int) value;
                break;
            default:
                SkASSERT(!"FontMatch unhandled type");
        }
        FcPatternAdd(pattern, type, fcvalue, 0);

        type = va_arg(ap, const char *);
        if (!type)
            break;
        // FcType is promoted to int when passed through ...
        vtype = static_cast<FcType>(va_arg(ap, int));
        value = va_arg(ap, const void *);
    };
    va_end(ap);

    FcConfigSubstitute(0, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    FcResult result;
    FcPattern* match = FcFontMatch(0, pattern, &result);
    FcPatternDestroy(pattern);

    return match;
}

// -----------------------------------------------------------------------------
// Check to see if the filename has already been assigned a fileid and, if so,
// use it. Otherwise, assign one. Return the resulting fileid.
// -----------------------------------------------------------------------------
static unsigned FileIdFromFilename(const char* filename)
{
    SkAutoMutexAcquire ac(global_fc_map_lock);

    std::map<std::string, unsigned>::const_iterator i =
        global_fc_map.find(filename);
    if (i == global_fc_map.end()) {
        const unsigned fileid = global_fc_map_next_id++;
        global_fc_map[filename] = fileid;
        global_fc_map_inverted[fileid] = filename;
        return fileid;
    } else {
        return i->second;
    }
}

SkTypeface* SkFontHost::FindTypeface(const SkTypeface* familyFace,
                                     const char familyName[],
                                     SkTypeface::Style style)
{
    const char* resolved_family_name = NULL;
    FcPattern* face_match = NULL;

    if (familyFace) {
        // Here we use the inverted global id map to find the filename from the
        // SkTypeface object. Given the filename we can ask fontconfig for the
        // familyname of the font.
        SkAutoMutexAcquire ac(global_fc_map_lock);

        const unsigned fileid = UniqueIdToFileId(familyFace->uniqueID());
        std::map<unsigned, std::string>::const_iterator i =
            global_fc_map_inverted.find(fileid);
        if (i == global_fc_map_inverted.end())
            return NULL;

        FcInit();
        face_match = FontMatch(FC_FILE, FcTypeString, i->second.c_str(), NULL);

        if (!face_match)
            return NULL;
        FcChar8* family;
        if (FcPatternGetString(face_match, FC_FAMILY, 0, &family)) {
            FcPatternDestroy(face_match);
            return NULL;
        }
        // At this point, @family is pointing into the @face_match object so we
        // cannot release it yet.

        resolved_family_name = reinterpret_cast<char*>(family);
    } else if (familyName) {
        resolved_family_name = familyName;
    } else {
        return NULL;
    }

    {
        SkAutoMutexAcquire ac(global_fc_map_lock);
        FcInit();
    }

    // At this point, we have a resolved_family_name from somewhere
    SkASSERT(resolved_family_name);

    const int bold = style & SkTypeface::kBold ?
                     FC_WEIGHT_BOLD : FC_WEIGHT_NORMAL;
    const int italic = style & SkTypeface::kItalic ?
                       FC_SLANT_ITALIC : FC_SLANT_ROMAN;
    FcPattern* match = FontMatch(FC_FULLNAME, FcTypeString, resolved_family_name,
                                 FC_WEIGHT, FcTypeInteger, bold,
                                 FC_SLANT, FcTypeInteger, italic,
                                 NULL);
    if (face_match)
        FcPatternDestroy(face_match);

    if (!match)
        return NULL;

    FcChar8* filename;
    if (FcPatternGetString(match, FC_FILE, 0, &filename) != FcResultMatch) {
        FcPatternDestroy(match);
        return NULL;
    }
    // Now @filename is pointing into @match

    const unsigned fileid = FileIdFromFilename(reinterpret_cast<char*>(filename));
    const unsigned id = FileIdAndStyleToUniqueId(fileid, style);
    SkTypeface* typeface = SkNEW_ARGS(FontConfigTypeface, (style, id));
    FcPatternDestroy(match);
    return typeface;
}

SkTypeface* SkFontHost::ResolveTypeface(uint32_t id)
{
    SkAutoMutexAcquire ac(global_fc_map_lock);
    const SkTypeface::Style style = UniqueIdToStyle(id);
    const unsigned fileid = UniqueIdToFileId(id);

    std::map<unsigned, std::string>::const_iterator i =
        global_fc_map_inverted.find(fileid);
    if (i == global_fc_map_inverted.end())
        return NULL;

    return SkNEW_ARGS(FontConfigTypeface, (style, id));
}

SkStream* SkFontHost::OpenStream(uint32_t id)
{
    SkAutoMutexAcquire ac(global_fc_map_lock);
    const unsigned fileid = UniqueIdToFileId(id);

    std::map<unsigned, std::string>::const_iterator i =
        global_fc_map_inverted.find(fileid);
    if (i == global_fc_map_inverted.end())
        return NULL;

    return SkNEW_ARGS(SkFILEStream, (i->second.c_str()));
}

void SkFontHost::CloseStream(uint32_t fontID, SkStream* stream)
{
}

SkTypeface* SkFontHost::CreateTypeface(SkStream* stream)
{
    SkASSERT(!"SkFontHost::CreateTypeface unimplemented");
    return NULL;
}

SkTypeface* SkFontHost::Deserialize(SkStream* stream) {
    SkASSERT(!"SkFontHost::Deserialize unimplemented");
    return NULL;
}

void SkFontHost::Serialize(const SkTypeface*, SkWStream*) {
    SkASSERT(!"SkFontHost::Serialize unimplemented");
}

SkScalerContext* SkFontHost::CreateFallbackScalerContext
        (const SkScalerContext::Rec& rec) {
    FcPattern* pattern = FcPatternCreate();
    FcConfigSubstitute(0, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    FcResult result;
    FcPattern* match = FcFontMatch(0, pattern, &result);
    FcPatternDestroy(pattern);

    // This will fail when we have no fonts on the system.
    SkASSERT(match);

    FcChar8* filename;
    if (FcPatternGetString(match, FC_FILE, 0, &filename) != FcResultMatch) {
        FcPatternDestroy(match);
        return NULL;
    }
    // Now @filename is pointing into @match

    const unsigned id = FileIdFromFilename(reinterpret_cast<char*>(filename));
    FcPatternDestroy(match);

    SkAutoDescriptor    ad(sizeof(rec) + SkDescriptor::ComputeOverhead(1));
    SkDescriptor*       desc = ad.getDesc();
    
    desc->init();
    SkScalerContext::Rec* newRec =
        (SkScalerContext::Rec*)desc->addEntry(kRec_SkDescriptorTag,
                                              sizeof(rec), &rec);
    newRec->fFontID = id;
    desc->computeChecksum();
    
    return SkFontHost::CreateScalerContext(desc);
}

///////////////////////////////////////////////////////////////////////////////

size_t SkFontHost::ShouldPurgeFontCache(size_t sizeAllocatedSoFar)
{
    if (sizeAllocatedSoFar > kFontCacheMemoryBudget)
        return sizeAllocatedSoFar - kFontCacheMemoryBudget;
    else
        return 0;   // nothing to do
}
