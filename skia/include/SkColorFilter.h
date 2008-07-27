/* include/graphics/SkColorFilter.h
**
** Copyright 2006, Google Inc.
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

#ifndef SkColorFilter_DEFINED
#define SkColorFilter_DEFINED

#include "SkColor.h"
#include "SkFlattenable.h"
#include "SkPorterDuff.h"

class SkColorFilter : public SkFlattenable {
public:
    /** Called with a scanline of colors, as if there was a shader installed.
        The implementation writes out its filtered version into result[].
        Note: shader and result may be the same buffer.
        @param src      array of colors, possibly generated by a shader
        @param count    the number of entries in the src[] and result[] arrays
        @param result   written by the filter
    */
    virtual void filterSpan(const SkPMColor src[], int count,
                            SkPMColor result[]) = 0;
    /** Called with a scanline of colors, as if there was a shader installed.
        The implementation writes out its filtered version into result[].
        Note: shader and result may be the same buffer.
        @param src      array of colors, possibly generated by a shader
        @param count    the number of entries in the src[] and result[] arrays
        @param result   written by the filter
    */
    virtual void filterSpan16(const uint16_t shader[], int count,
                              uint16_t result[]);

    enum Flags {
        /** If set the filter methods will not change the alpha channel of the
            colors.
        */
        kAlphaUnchanged_Flag = 0x01,
        /** If set, this subclass implements filterSpan16(). If this flag is
            set, then kAlphaUnchanged_Flag must also be set.
        */
        kHasFilter16_Flag    = 0x02
    };

    /** Returns the flags for this filter. Override in subclasses to return
        custom flags.
    */
    virtual uint32_t getFlags() { return 0; }

    /** Create a colorfilter that uses the specified color and porter-duff mode.
        If porterDuffMode is DST, this function will return NULL (since that
        mode will have no effect on the result).
        @param srcColor The source color used with the specified mode
        @param mode     The porter-duff mode that is applied to each color in
                        the colorfilter's filterSpan[16,32] methods
        @return colorfilter object that applies the src color and porter-duff
                mode, or NULL if the mode will have no effect.
    */
    static SkColorFilter* CreatePorterDuffFilter(SkColor srcColor,
                                                 SkPorterDuff::Mode mode);

    /** Create a colorfilter that calls through to the specified procs to
        filter the colors. The SkXfermodeProc parameter must be non-null, but
        the SkXfermodeProc16 is optional, and may be null.
    */
    static SkColorFilter* CreatXfermodeProcFilter(SkColor srcColor,
                                              SkXfermodeProc proc,
                                              SkXfermodeProc16 proc16 = NULL);

    /** Create a colorfilter that multiplies the RGB channels by one color, and
        then adds a second color, pinning the result for each component to
        [0..255]. The alpha components of the mul and add arguments
        are ignored.
    */
    static SkColorFilter* CreateLightingFilter(SkColor mul, SkColor add);
    
protected:
    SkColorFilter() {}
    SkColorFilter(SkFlattenableReadBuffer& rb) : INHERITED(rb) {}
    
private:
    typedef SkFlattenable INHERITED;
};

#include "SkShader.h"

class SkFilterShader : public SkShader {
public:
    SkFilterShader(SkShader* shader, SkColorFilter* filter);
    virtual ~SkFilterShader();

    // override
    virtual uint32_t getFlags();
    virtual bool setContext(const SkBitmap& device, const SkPaint& paint,
                            const SkMatrix& matrix);
    virtual void shadeSpan(int x, int y, SkPMColor result[], int count);
    virtual void shadeSpan16(int x, int y, uint16_t result[], int count);
    virtual void beginSession();
    virtual void endSession();
    
protected:
    SkFilterShader(SkFlattenableReadBuffer& );
    virtual void flatten(SkFlattenableWriteBuffer& );
    virtual Factory getFactory() { return CreateProc; }
private:
    static SkFlattenable* CreateProc(SkFlattenableReadBuffer& buffer) { 
        return SkNEW_ARGS(SkFilterShader, (buffer)); }
    SkShader*       fShader;
    SkColorFilter*  fFilter;
    
    typedef SkShader INHERITED;
};

#endif
