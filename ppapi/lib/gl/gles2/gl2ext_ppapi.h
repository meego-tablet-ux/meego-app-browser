// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// OpenGL ES 2.0 extensions for PPAPI.

#ifndef PPAPI_LIB_GL_GLES2_GL2EXT_PPAPI_H_
#define PPAPI_LIB_GL_GLES2_GL2EXT_PPAPI_H_

#include <GLES2/gl2platform.h>

#include "ppapi/c/pp_module.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/ppb.h"
#include "ppapi/c/dev/ppb_opengles_dev.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// Initializes OpenGL ES 2.0 library.
// Must be called once before making any gl calls.
// GL_FALSE is returned on failure, GL_TRUE otherwise.
GL_APICALL int GL_APIENTRY glInitializePPAPI(
    PP_Module module,
    PPB_GetInterface get_browser_interface);

// Terminates OpenGL ES 2.0 library.
// GL_FALSE is returned on failure, GL_TRUE otherwise.
GL_APICALL int GL_APIENTRY glTerminatePPAPI();

// Sets context to be used for rendering in the current thread.
GL_APICALL void GL_APIENTRY glSetCurrentContextPPAPI(PP_Resource context);

// Gets context being used for rendering in the current thread.
// Returns NULL if a context has not been set yet.
GL_APICALL PP_Resource GL_APIENTRY glGetCurrentContextPPAPI();

// Returns OpenGL ES 2.0 interface.
GL_APICALL const struct PPB_OpenGLES_Dev* GL_APIENTRY glGetInterfacePPAPI();

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // PPAPI_LIB_GL_GLES2_GL2EXT_PPAPI_H_

