/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef _VIAINIT_H
#define _VIAINIT_H

#include <sys/time.h>
#include "dri_util.h"
#include "via_dri.h"
#include "xmlconfig.h"

typedef struct {
    viaRegion regs;
    viaRegion agp;
    int deviceID;
    int width;
    int height;
    int mem;

    int cpp;                    
    int bitsPerPixel;
    int bytesPerPixel;
    int fbFormat;
    int fbOffset;
    int fbSize;
    
    int fbStride;

    int backOffset;
    int depthOffset;

    int backPitch;
    int backPitchBits;

    int textureOffset;
    int textureSize;
    int logTextureGranularity;
    
    drmAddress reg;
    drmAddress agpLinearStart;
    GLuint agpBase;

    __DRIscreen *driScrnPriv;
    drmBufMapPtr bufs;
    unsigned int sareaPrivOffset;
    /*=* John Sheng [2003.12.9] Tuxracer & VQ *=*/
    int VQEnable;
    int irqEnabled;

    /* Configuration cache with default values for all contexts */
    driOptionCache optionCache;

    const __DRIextension *extensions[5];
} viaScreenPrivate;


extern GLboolean
viaCreateContext(gl_api api,
		 const __GLcontextModes *mesaVis,
                 __DRIcontext *driContextPriv,
                 void *sharedContextPrivate);

extern void
viaDestroyContext(__DRIcontext *driContextPriv);

extern GLboolean
viaUnbindContext(__DRIcontext *driContextPriv);

extern GLboolean
viaMakeCurrent(__DRIcontext *driContextPriv,
               __DRIdrawable *driDrawPriv,
               __DRIdrawable *driReadPriv);

extern void
viaSwapBuffers(__DRIdrawable *drawablePrivate);

#endif
