/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 2005-2007  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef SLANG_BUILTIN_H
#define SLANG_BUILTIN_H

#include "shader/prog_parameter.h"
#include "slang_utility.h"
#include "slang_ir.h"


extern GLint
_slang_alloc_statevar(slang_ir_node *n,
                      struct gl_program_parameter_list *paramList,
                      GLboolean *direct);


extern GLint
_slang_input_index(const char *name, GLenum target, GLuint *swizzleOut);

extern GLint
_slang_output_index(const char *name, GLenum target);


extern const char *
_slang_vert_attrib_name(GLuint attrib);

extern GLenum
_slang_vert_attrib_type(GLuint attrib);


#endif /* SLANG_BUILTIN_H */
