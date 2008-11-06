/*
 * Copyright (C) 2008 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef ThemeHelperWin_h
#define ThemeHelperWin_h

#include <windows.h>

#include "AffineTransform.h"
#include "ImageBuffer.h"
#include "IntRect.h"
#include "WTF/OwnPtr.h"

namespace WebCore {

class GraphicsContext;
class IntRect;

// Helps drawing theme elements like buttons and scroll bars. This will handle
// translations and scalings that Windows might not, by either making Windows
// draw the appropriate sized control, or by rendering it into an off-screen
// context and transforming it ourselves.
class ThemeHelperWin {
    enum Type {
        // Use the original canvas with no changes. This is the normal mode.
        ORIGINAL,

        // Use the original canvas but scale the rectangle of the control so
        // that it will be the correct size, undoing any scale already on the
        // canvas. This will have the effect of just drawing the control bigger
        // or smaller and not actually expanding or contracting the pixels in
        // it. This usually looks better.
        SCALE,

        // Make a copy of the control and then transform it ourselves after
        // Windows draws it. This allows us to get complex effects.
        COPY,
    };

public:
    // Prepares drawing a control with the given rect to the given context.
    ThemeHelperWin(GraphicsContext* context, const IntRect& rect);
    ~ThemeHelperWin();

    // Returns the context to draw the control into, which may be the original
    // or the copy, depending on the mode.
    GraphicsContext* context()
    {
        return m_newBuffer.get() ? m_newBuffer->context() : m_orgContext;
    }

    // Returns the rectangle in which to draw into the canvas() by Windows.
    const RECT& rect() { return m_rect; }

    RECT transformRect(const RECT& r) const;

private:
    Type m_type;

    // The original canvas to wrote to. Not owned by this class.
    GraphicsContext* m_orgContext;
    AffineTransform m_orgMatrix;
    IntRect m_orgRect;

    // When m_type == COPY, this will be a new surface owned by this class that
    // represents the copy.
    OwnPtr<ImageBuffer> m_newBuffer;

    // The control rectangle in the cooredinate space of canvas().
    RECT m_rect;
};

}  // namespace WebCore

#endif  // ThemeHelperWin_h
