/* Copyright (c) 2010 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * This file has compile assertions for the sizes of types that are dependent
 * on the architecture for which they are compiled (i.e., 32-bit vs 64-bit).
 */

#ifndef PPAPI_TESTS_ARCH_DEPENDENT_SIZES_32_H_
#define PPAPI_TESTS_ARCH_DEPENDENT_SIZES_32_H_

#include "ppapi/tests/test_struct_sizes.c"

PP_COMPILE_ASSERT_SIZE_IN_BYTES(GLintptr, 4);
PP_COMPILE_ASSERT_SIZE_IN_BYTES(GLsizeiptr, 4);
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_ClassDestructor, 4);
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_ClassFunction, 4);
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_CompletionCallback_Func, 4);
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_URLLoaderTrusted_StatusCallback, 4);
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_VideoConfig_Dev, 4);
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_VideoDecodeEventHandler_Func_Dev, 4);
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_VideoDecodeInputCallback_Func_Dev, 4);
PP_COMPILE_ASSERT_SIZE_IN_BYTES(PP_VideoDecodeOutputCallback_Func_Dev, 4);
PP_COMPILE_ASSERT_STRUCT_SIZE_IN_BYTES(PP_ClassProperty, 20);
PP_COMPILE_ASSERT_STRUCT_SIZE_IN_BYTES(PP_CompletionCallback, 8);
PP_COMPILE_ASSERT_STRUCT_SIZE_IN_BYTES(PP_FileChooserOptions_Dev, 8);
PP_COMPILE_ASSERT_STRUCT_SIZE_IN_BYTES(PP_VideoDecoderConfig_Dev, 20);
PP_COMPILE_ASSERT_STRUCT_SIZE_IN_BYTES(PP_VideoFrameBuffer_Dev, 112);
PP_COMPILE_ASSERT_STRUCT_SIZE_IN_BYTES(PP_VideoUncompressedDataBuffer_Dev, 136);

#endif  /* PPAPI_TESTS_ARCH_DEPENDENT_SIZES_32_H_ */
