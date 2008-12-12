/* libs/graphics/animator/SkDrawPaint.cpp
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

#include "SkDrawPaint.h"
#include "SkAnimateMaker.h"
#include "SkDrawColor.h"
#include "SkDrawShader.h"
#include "SkMaskFilter.h"
#include "SkPaintParts.h"
#include "SkPathEffect.h"

enum SkPaint_Functions {
    SK_FUNCTION(measureText)
};

enum SkPaint_Properties {
    SK_PROPERTY(ascent),
    SK_PROPERTY(descent)
};

// !!! in the future, this could be compiled by build-condensed-info into an array of parameters
// with a lookup table to find the first parameter -- for now, it is iteratively searched through
const SkFunctionParamType SkDrawPaint::fFunctionParameters[] = {
    (SkFunctionParamType) SkType_String,
    (SkFunctionParamType) 0 // terminator for parameter list (there may be multiple parameter lists)
};


#if SK_USE_CONDENSED_INFO == 0

const SkMemberInfo SkDrawPaint::fInfo[] = {
    SK_MEMBER(antiAlias, Boolean),
    SK_MEMBER_PROPERTY(ascent, Float),
    SK_MEMBER(color, Color),
    SK_MEMBER_PROPERTY(descent, Float),
    SK_MEMBER(fakeBold, Boolean),
    SK_MEMBER(filterBitmap, Boolean),
    SK_MEMBER(linearText, Boolean),
    SK_MEMBER(maskFilter, MaskFilter),
    SK_MEMBER_FUNCTION(measureText, Float),
    SK_MEMBER(pathEffect, PathEffect),
    SK_MEMBER(shader, Shader),
    SK_MEMBER(strikeThru, Boolean),
    SK_MEMBER(stroke, Boolean),
    SK_MEMBER(strokeCap, Cap),
    SK_MEMBER(strokeJoin, Join),
    SK_MEMBER(strokeMiter, Float),
    SK_MEMBER(strokeWidth, Float),
    SK_MEMBER(style, Style),
    SK_MEMBER(textAlign, Align),
    SK_MEMBER(textScaleX, Float),
    SK_MEMBER(textSize, Float),
    SK_MEMBER(textSkewX, Float),
    SK_MEMBER(typeface, Typeface),
    SK_MEMBER(underline, Boolean),
    SK_MEMBER(xfermode, Xfermode)
};

#endif

DEFINE_GET_MEMBER(SkDrawPaint);

SkDrawPaint::SkDrawPaint() : antiAlias(-1), color(NULL), fakeBold(-1), filterBitmap(-1),
    linearText(-1), maskFilter((SkDrawMaskFilter*) -1), pathEffect((SkDrawPathEffect*) -1),
    shader((SkDrawShader*) -1), strikeThru(-1), stroke(-1),
    strokeCap((SkPaint::Cap) -1), strokeJoin((SkPaint::Join) -1), strokeMiter(SK_ScalarNaN), 
    strokeWidth(SK_ScalarNaN), style((SkPaint::Style) -1),
    textAlign((SkPaint::Align) -1), textScaleX(SK_ScalarNaN), textSize(SK_ScalarNaN), 
    textSkewX(SK_ScalarNaN), typeface((SkDrawTypeface*) -1),
    underline(-1), xfermode((SkPorterDuff::Mode) -1), fOwnsColor(false), fOwnsMaskFilter(false), 
    fOwnsPathEffect(false), fOwnsShader(false), fOwnsTypeface(false) {
}

SkDrawPaint::~SkDrawPaint() {
    if (fOwnsColor)
        delete color;
    if (fOwnsMaskFilter)
        delete maskFilter;
    if (fOwnsPathEffect)
        delete pathEffect;
    if (fOwnsShader)
        delete shader;
    if (fOwnsTypeface)
        delete typeface;
}

bool SkDrawPaint::add(SkAnimateMaker& maker, SkDisplayable* child) {
    SkASSERT(child && child->isPaintPart());
    SkPaintPart* part = (SkPaintPart*) child;
    if (part->add())
        maker.setErrorCode(SkDisplayXMLParserError::kErrorAddingToPaint); 
    return true;
}

SkDisplayable* SkDrawPaint::deepCopy(SkAnimateMaker* maker) {
    SkDrawColor* tempColor = color;
    color = NULL;
    SkDrawPaint* copy = (SkDrawPaint*) INHERITED::deepCopy(maker);
    color = tempColor;
    tempColor = (SkDrawColor*) color->deepCopy(maker);
    tempColor->setParent(copy);
    tempColor->add();
    copy->fOwnsColor = true;
    return copy;
}

bool SkDrawPaint::draw(SkAnimateMaker& maker) {
    SkPaint* paint = maker.fPaint;
    setupPaint(paint);
    return false;
}

#ifdef SK_DUMP_ENABLED
void SkDrawPaint::dump(SkAnimateMaker* maker) {
    dumpBase(maker);
    dumpAttrs(maker);
    bool closedYet = false;
    SkDisplayList::fIndent +=4;
    //should i say if (maskFilter && ...?
    if (maskFilter != (SkDrawMaskFilter*)-1) {
        SkDebugf(">\n");
        maskFilter->dump(maker);
        closedYet = true;
    }
    if (pathEffect != (SkDrawPathEffect*) -1) {
        if (closedYet == false) {
            SkDebugf(">\n");
            closedYet = true;
        }
        pathEffect->dump(maker);
    }
    if (fOwnsTypeface) {
        if (closedYet == false) {
            SkDebugf(">\n");
            closedYet = true;
        }
        typeface->dump(maker);
    }
    SkDisplayList::fIndent -= 4;
    dumpChildren(maker, closedYet);
}
#endif
    
void SkDrawPaint::executeFunction(SkDisplayable* target, int index, 
        SkTDArray<SkScriptValue>& parameters, SkDisplayTypes type,
        SkScriptValue* scriptValue) {
        if (scriptValue == NULL)
            return;
    SkASSERT(target == this);
    switch (index) {
        case SK_FUNCTION(measureText): {
            SkASSERT(parameters.count() == 1);
            SkASSERT(type == SkType_Float);
            SkPaint paint;
            setupPaint(&paint);
            scriptValue->fType = SkType_Float;
            SkASSERT(parameters[0].fType == SkType_String);
            scriptValue->fOperand.fScalar = paint.measureText(parameters[0].fOperand.fString->c_str(), 
                parameters[0].fOperand.fString->size()); 
//          SkDebugf("measureText: %s = %g\n", parameters[0].fOperand.fString->c_str(), 
//              scriptValue->fOperand.fScalar / 65536.0f);
            } break;
        default:
            SkASSERT(0);
    }
}

const SkFunctionParamType* SkDrawPaint::getFunctionsParameters() {
    return fFunctionParameters;
}

bool SkDrawPaint::getProperty(int index, SkScriptValue* value) const {
    SkPaint::FontMetrics    metrics;
    SkPaint paint;
    setupPaint(&paint);
    paint.getFontMetrics(&metrics);
    switch (index) {
        case SK_PROPERTY(ascent):
            value->fOperand.fScalar = metrics.fAscent;
            break;
        case SK_PROPERTY(descent):
            value->fOperand.fScalar = metrics.fDescent;
            break;
        // should consider returning fLeading as well (or roll it into ascent/descent somehow 
        default:
            SkASSERT(0);
            return false;
    }
    value->fType = SkType_Float;
    return true;
}

bool SkDrawPaint::resolveIDs(SkAnimateMaker& maker, SkDisplayable* origDisp, SkApply* ) {
    SkASSERT(origDisp->isPaint());
    SkDrawPaint* original = (SkDrawPaint*) origDisp;
    if (fOwnsColor && maker.resolveID(color, original->color) == false)
        return true;
    if (fOwnsMaskFilter && maker.resolveID(maskFilter, original->maskFilter) == false)
        return true;
    if (fOwnsPathEffect && maker.resolveID(pathEffect, original->pathEffect) == false)
        return true;
    if (fOwnsShader && maker.resolveID(shader, original->shader) == false)
        return true;
    if (fOwnsTypeface && maker.resolveID(typeface, original->typeface) == false)
        return true;
    return false; // succeeded 
}

void SkDrawPaint::setupPaint(SkPaint* paint) const {
    if (antiAlias != -1)
        paint->setAntiAlias(SkToBool(antiAlias));
    if (color != NULL)
        paint->setColor(color->getColor());
    if (fakeBold != -1)
        paint->setFakeBoldText(SkToBool(fakeBold));
    if (filterBitmap != -1)
        paint->setFilterBitmap(SkToBool(filterBitmap));
    //  stroke is legacy; style setting if present overrides stroke
    if (stroke != -1)
        paint->setStyle(SkToBool(stroke) ? SkPaint::kStroke_Style : SkPaint::kFill_Style);
    if (style != (SkPaint::Style) -1)
        paint->setStyle((SkPaint::Style) style);
    if (linearText != -1)
        paint->setLinearText(SkToBool(linearText));
    if (maskFilter == NULL)
        paint->setMaskFilter(NULL);
    else if (maskFilter != (SkDrawMaskFilter*) -1)
        paint->setMaskFilter(maskFilter->getMaskFilter())->safeUnref();
    if (pathEffect == NULL)
        paint->setPathEffect(NULL);
    else if (pathEffect != (SkDrawPathEffect*) -1)
        paint->setPathEffect(pathEffect->getPathEffect())->safeUnref();
    if (shader == NULL)
        paint->setShader(NULL);
    else if (shader != (SkDrawShader*) -1)
        paint->setShader(shader->getShader())->safeUnref();
    if (strikeThru != -1)
        paint->setStrikeThruText(SkToBool(strikeThru));
    if (strokeCap != (SkPaint::Cap) -1)
        paint->setStrokeCap((SkPaint::Cap) strokeCap);
    if (strokeJoin != (SkPaint::Join) -1)
        paint->setStrokeJoin((SkPaint::Join) strokeJoin);
    if (SkScalarIsNaN(strokeMiter) == false)
        paint->setStrokeMiter(strokeMiter);
    if (SkScalarIsNaN(strokeWidth) == false)
        paint->setStrokeWidth(strokeWidth);
    if (textAlign != (SkPaint::Align) -1)
        paint->setTextAlign((SkPaint::Align) textAlign);
    if (SkScalarIsNaN(textScaleX) == false)
        paint->setTextScaleX(textScaleX);
    if (SkScalarIsNaN(textSize) == false)
        paint->setTextSize(textSize);
    if (SkScalarIsNaN(textSkewX) == false)
        paint->setTextSkewX(textSkewX);
    if (typeface == NULL)
        paint->setTypeface(NULL);
    else if (typeface != (SkDrawTypeface*) -1)
        paint->setTypeface(typeface->getTypeface())->safeUnref();
    if (underline != -1)
        paint->setUnderlineText(SkToBool(underline));
    if (xfermode != (SkPorterDuff::Mode) -1) 
        paint->setPorterDuffXfermode((SkPorterDuff::Mode) xfermode);
}
