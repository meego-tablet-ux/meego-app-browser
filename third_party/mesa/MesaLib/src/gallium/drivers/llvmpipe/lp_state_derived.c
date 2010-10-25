/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
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

#include "util/u_math.h"
#include "util/u_memory.h"
#include "pipe/p_shader_tokens.h"
#include "draw/draw_context.h"
#include "draw/draw_vertex.h"
#include "draw/draw_private.h"
#include "lp_context.h"
#include "lp_screen.h"
#include "lp_setup.h"
#include "lp_state.h"



/**
 * The vertex info describes how to convert the post-transformed vertices
 * (simple float[][4]) used by the 'draw' module into vertices for
 * rasterization.
 *
 * This function validates the vertex layout.
 */
static void
compute_vertex_info(struct llvmpipe_context *llvmpipe)
{
   const struct lp_fragment_shader *lpfs = llvmpipe->fs;
   struct vertex_info *vinfo = &llvmpipe->vertex_info;
   struct lp_shader_input *inputs = llvmpipe->inputs;
   unsigned vs_index;
   uint i;

   /*
    * Match FS inputs against VS outputs, emitting the necessary attributes.
    */

   vinfo->num_attribs = 0;

   vs_index = draw_find_shader_output(llvmpipe->draw,
                                       TGSI_SEMANTIC_POSITION,
                                       0);

   draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_PERSPECTIVE, vs_index);

   for (i = 0; i < lpfs->info.num_inputs; i++) {
      /*
       * Search for each input in current vs output:
       */

      vs_index = draw_find_shader_output(llvmpipe->draw,
                                         lpfs->info.input_semantic_name[i],
                                         lpfs->info.input_semantic_index[i]);
      if (vs_index < 0) {
         /*
          * This can happen with sprite coordinates - the vertex
          * shader doesn't need to provide an output as we generate
          * them internally.  However, lets keep pretending that there
          * is something there to not confuse other code.
          */
         vs_index = 0;
      }

      /* This can be pre-computed, except for flatshade:
       */
      inputs[i].usage_mask = lpfs->info.input_usage_mask[i];

      switch (lpfs->info.input_interpolate[i]) {
      case TGSI_INTERPOLATE_CONSTANT:
         inputs[i].interp = LP_INTERP_CONSTANT;
         break;
      case TGSI_INTERPOLATE_LINEAR:
         inputs[i].interp = LP_INTERP_LINEAR;
         break;
      case TGSI_INTERPOLATE_PERSPECTIVE:
         inputs[i].interp = LP_INTERP_PERSPECTIVE;
         break;
      default:
         assert(0);
         break;
      }

      switch (lpfs->info.input_semantic_name[i]) {
      case TGSI_SEMANTIC_FACE:
         inputs[i].interp = LP_INTERP_FACING;
         break;
      case TGSI_SEMANTIC_POSITION:
         /* Position was already emitted above
          */
         inputs[i].interp = LP_INTERP_POSITION;
         inputs[i].src_index = 0;
         continue;
      case TGSI_SEMANTIC_COLOR:
         /* Colors are linearly inputs[i].interpolated in the fragment shader
          * even when flatshading is active.  This just tells the
          * setup module to use coefficients with ddx==0 and
          * ddy==0.
          */
         if (llvmpipe->rasterizer->flatshade)
            inputs[i].interp = LP_INTERP_CONSTANT;
         break;

      default:
         break;
      }

      /*
       * Emit the requested fs attribute for all but position.
       */

      inputs[i].src_index = vinfo->num_attribs;
      draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_PERSPECTIVE, vs_index);
   }

   /* Figure out if we need pointsize as well.
    */
   vs_index = draw_find_shader_output(llvmpipe->draw,
                                      TGSI_SEMANTIC_PSIZE, 0);

   if (vs_index > 0) {
      llvmpipe->psize_slot = vinfo->num_attribs;
      draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_CONSTANT, vs_index);
   }

   llvmpipe->num_inputs = lpfs->info.num_inputs;

   draw_compute_vertex_size(vinfo);

   lp_setup_set_vertex_info(llvmpipe->setup, vinfo);

   lp_setup_set_fs_inputs(llvmpipe->setup,
                          inputs,
                          lpfs->info.num_inputs);
}


/**
 * Handle state changes.
 * Called just prior to drawing anything (pipe::draw_arrays(), etc).
 *
 * Hopefully this will remain quite simple, otherwise need to pull in
 * something like the state tracker mechanism.
 */
void llvmpipe_update_derived( struct llvmpipe_context *llvmpipe )
{
   struct llvmpipe_screen *lp_screen = llvmpipe_screen(llvmpipe->pipe.screen);

   /* Check for updated textures.
    */
   if (llvmpipe->tex_timestamp != lp_screen->timestamp) {
      llvmpipe->tex_timestamp = lp_screen->timestamp;
      llvmpipe->dirty |= LP_NEW_SAMPLER_VIEW;
   }
      
   if (llvmpipe->dirty & (LP_NEW_RASTERIZER |
                          LP_NEW_FS |
                          LP_NEW_VS))
      compute_vertex_info( llvmpipe );

   if (llvmpipe->dirty & (LP_NEW_FS |
                          LP_NEW_BLEND |
                          LP_NEW_SCISSOR |
                          LP_NEW_DEPTH_STENCIL_ALPHA |
                          LP_NEW_RASTERIZER |
                          LP_NEW_SAMPLER |
                          LP_NEW_SAMPLER_VIEW |
                          LP_NEW_QUERY))
      llvmpipe_update_fs( llvmpipe );

   if (llvmpipe->dirty & LP_NEW_BLEND_COLOR)
      lp_setup_set_blend_color(llvmpipe->setup,
                               &llvmpipe->blend_color);

   if (llvmpipe->dirty & LP_NEW_SCISSOR)
      lp_setup_set_scissor(llvmpipe->setup, &llvmpipe->scissor);

   if (llvmpipe->dirty & LP_NEW_DEPTH_STENCIL_ALPHA) {
      lp_setup_set_alpha_ref_value(llvmpipe->setup, 
                                   llvmpipe->depth_stencil->alpha.ref_value);
      lp_setup_set_stencil_ref_values(llvmpipe->setup,
                                      llvmpipe->stencil_ref.ref_value);
   }

   if (llvmpipe->dirty & LP_NEW_CONSTANTS)
      lp_setup_set_fs_constants(llvmpipe->setup, 
                                llvmpipe->constants[PIPE_SHADER_FRAGMENT][0]);

   if (llvmpipe->dirty & LP_NEW_SAMPLER_VIEW)
      lp_setup_set_fragment_sampler_views(llvmpipe->setup,
                                          llvmpipe->num_fragment_sampler_views,
                                          llvmpipe->fragment_sampler_views);

   llvmpipe->dirty = 0;
}

