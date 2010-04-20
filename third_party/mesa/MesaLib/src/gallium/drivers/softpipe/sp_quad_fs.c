/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2008 VMware, Inc.  All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/* Vertices are just an array of floats, with all the attributes
 * packed.  We currently assume a layout like:
 *
 * attr[0][0..3] - window position
 * attr[1..n][0..3] - remaining attributes.
 *
 * Attributes are assumed to be 4 floats wide but are packed so that
 * all the enabled attributes run contiguously.
 */

#include "util/u_math.h"
#include "util/u_memory.h"
#include "pipe/p_defines.h"
#include "pipe/p_shader_tokens.h"

#include "sp_context.h"
#include "sp_state.h"
#include "sp_quad.h"
#include "sp_quad_pipe.h"
#include "sp_texture.h"
#include "sp_tex_sample.h"


struct quad_shade_stage
{
   struct quad_stage stage;  /**< base class */
   struct tgsi_exec_machine *machine;
   struct tgsi_exec_vector *inputs, *outputs;
};


/** cast wrapper */
static INLINE struct quad_shade_stage *
quad_shade_stage(struct quad_stage *qs)
{
   return (struct quad_shade_stage *) qs;
}


/**
 * Execute fragment shader for the four fragments in the quad.
 */
static INLINE boolean
shade_quad(struct quad_stage *qs, struct quad_header *quad)
{
   struct quad_shade_stage *qss = quad_shade_stage( qs );
   struct softpipe_context *softpipe = qs->softpipe;
   struct tgsi_exec_machine *machine = qss->machine;

   /* run shader */
   return softpipe->fs->run( softpipe->fs, machine, quad );
}



static void
coverage_quad(struct quad_stage *qs, struct quad_header *quad)
{
   struct softpipe_context *softpipe = qs->softpipe;
   uint cbuf;

   /* loop over colorbuffer outputs */
   for (cbuf = 0; cbuf < softpipe->framebuffer.nr_cbufs; cbuf++) {
      float (*quadColor)[4] = quad->output.color[cbuf];
      unsigned j;
      for (j = 0; j < QUAD_SIZE; j++) {
         assert(quad->input.coverage[j] >= 0.0);
         assert(quad->input.coverage[j] <= 1.0);
         quadColor[3][j] *= quad->input.coverage[j];
      }
   }
}



static void
shade_quads(struct quad_stage *qs, 
                 struct quad_header *quads[],
                 unsigned nr)
{
   struct quad_shade_stage *qss = quad_shade_stage( qs );
   struct softpipe_context *softpipe = qs->softpipe;
   struct tgsi_exec_machine *machine = qss->machine;

   unsigned i, pass = 0;
   
   machine->Consts = softpipe->mapped_constants[PIPE_SHADER_FRAGMENT];
   machine->InterpCoefs = quads[0]->coef;

   for (i = 0; i < nr; i++) {
      if (!shade_quad(qs, quads[i]))
         continue;

      if (/*do_coverage*/ 0)
         coverage_quad( qs, quads[i] );

      quads[pass++] = quads[i];
   }
   
   if (pass)
      qs->next->run(qs->next, quads, pass);
}
   




/**
 * Per-primitive (or per-begin?) setup
 */
static void
shade_begin(struct quad_stage *qs)
{
   struct quad_shade_stage *qss = quad_shade_stage(qs);
   struct softpipe_context *softpipe = qs->softpipe;

   softpipe->fs->prepare( softpipe->fs, 
			  qss->machine,
			  (struct tgsi_sampler **)
                             softpipe->tgsi.frag_samplers_list );

   qs->next->begin(qs->next);
}


static void
shade_destroy(struct quad_stage *qs)
{
   struct quad_shade_stage *qss = (struct quad_shade_stage *) qs;

   tgsi_exec_machine_destroy(qss->machine);

   FREE( qs );
}


struct quad_stage *
sp_quad_shade_stage( struct softpipe_context *softpipe )
{
   struct quad_shade_stage *qss = CALLOC_STRUCT(quad_shade_stage);
   if (!qss)
      goto fail;

   qss->stage.softpipe = softpipe;
   qss->stage.begin = shade_begin;
   qss->stage.run = shade_quads;
   qss->stage.destroy = shade_destroy;

   qss->machine = tgsi_exec_machine_create();
   if (!qss->machine)
      goto fail;

   return &qss->stage;

fail:
   if (qss && qss->machine)
      tgsi_exec_machine_destroy(qss->machine);

   FREE(qss);
   return NULL;
}
