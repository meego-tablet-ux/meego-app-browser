/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/**
 * Polygon stipple stage:  implement polygon stipple with texture map and
 * fragment program.  The fragment program samples the texture and does
 * a fragment kill for the stipple-failing fragments.
 *
 * Authors:  Brian Paul
 */


#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_shader_tokens.h"

#include "util/u_math.h"
#include "util/u_memory.h"

#include "tgsi/tgsi_transform.h"
#include "tgsi/tgsi_dump.h"

#include "draw_context.h"
#include "draw_pipe.h"



/**
 * Subclass of pipe_shader_state to carry extra fragment shader info.
 */
struct pstip_fragment_shader
{
   struct pipe_shader_state state;
   void *driver_fs;
   void *pstip_fs;
   uint sampler_unit;
};


/**
 * Subclass of draw_stage
 */
struct pstip_stage
{
   struct draw_stage stage;

   void *sampler_cso;
   struct pipe_texture *texture;
   uint num_samplers;
   uint num_textures;

   /*
    * Currently bound state
    */
   struct pstip_fragment_shader *fs;
   struct {
      void *samplers[PIPE_MAX_SAMPLERS];
      struct pipe_texture *textures[PIPE_MAX_SAMPLERS];
      const struct pipe_poly_stipple *stipple;
   } state;

   /*
    * Driver interface/override functions
    */
   void * (*driver_create_fs_state)(struct pipe_context *,
                                    const struct pipe_shader_state *);
   void (*driver_bind_fs_state)(struct pipe_context *, void *);
   void (*driver_delete_fs_state)(struct pipe_context *, void *);

   void (*driver_bind_sampler_states)(struct pipe_context *, unsigned, void **);

   void (*driver_set_sampler_textures)(struct pipe_context *, unsigned,
                                       struct pipe_texture **);

   void (*driver_set_polygon_stipple)(struct pipe_context *,
                                      const struct pipe_poly_stipple *);

   struct pipe_context *pipe;
};



/**
 * Subclass of tgsi_transform_context, used for transforming the
 * user's fragment shader to add the special AA instructions.
 */
struct pstip_transform_context {
   struct tgsi_transform_context base;
   uint tempsUsed;  /**< bitmask */
   int wincoordInput;
   int maxInput;
   uint samplersUsed;  /**< bitfield of samplers used */
   int freeSampler;  /** an available sampler for the pstipple */
   int texTemp;  /**< temp registers */
   int numImmed;
   boolean firstInstruction;
};


/**
 * TGSI declaration transform callback.
 * Look for a free sampler, a free input attrib, and two free temp regs.
 */
static void
pstip_transform_decl(struct tgsi_transform_context *ctx,
                     struct tgsi_full_declaration *decl)
{
   struct pstip_transform_context *pctx = (struct pstip_transform_context *) ctx;

   if (decl->Declaration.File == TGSI_FILE_SAMPLER) {
      uint i;
      for (i = decl->DeclarationRange.First;
           i <= decl->DeclarationRange.Last; i++) {
         pctx->samplersUsed |= 1 << i;
      }
   }
   else if (decl->Declaration.File == TGSI_FILE_INPUT) {
      pctx->maxInput = MAX2(pctx->maxInput, (int) decl->DeclarationRange.Last);
      if (decl->Semantic.SemanticName == TGSI_SEMANTIC_POSITION)
         pctx->wincoordInput = (int) decl->DeclarationRange.First;
   }
   else if (decl->Declaration.File == TGSI_FILE_TEMPORARY) {
      uint i;
      for (i = decl->DeclarationRange.First;
           i <= decl->DeclarationRange.Last; i++) {
         pctx->tempsUsed |= (1 << i);
      }
   }

   ctx->emit_declaration(ctx, decl);
}


static void
pstip_transform_immed(struct tgsi_transform_context *ctx,
                      struct tgsi_full_immediate *immed)
{
   struct pstip_transform_context *pctx = (struct pstip_transform_context *) ctx;
   pctx->numImmed++;
}


/**
 * Find the lowest zero bit in the given word, or -1 if bitfield is all ones.
 */
static int
free_bit(uint bitfield)
{
   int i;
   for (i = 0; i < 32; i++) {
      if ((bitfield & (1 << i)) == 0)
         return i;
   }
   return -1;
}


/**
 * TGSI instruction transform callback.
 * Replace writes to result.color w/ a temp reg.
 * Upon END instruction, insert texture sampling code for antialiasing.
 */
static void
pstip_transform_inst(struct tgsi_transform_context *ctx,
                     struct tgsi_full_instruction *inst)
{
   struct pstip_transform_context *pctx = (struct pstip_transform_context *) ctx;

   if (pctx->firstInstruction) {
      /* emit our new declarations before the first instruction */

      struct tgsi_full_declaration decl;
      struct tgsi_full_instruction newInst;
      uint i;
      int wincoordInput;

      /* find free sampler */
      pctx->freeSampler = free_bit(pctx->samplersUsed);
      if (pctx->freeSampler >= PIPE_MAX_SAMPLERS)
         pctx->freeSampler = PIPE_MAX_SAMPLERS - 1;

      if (pctx->wincoordInput < 0)
         wincoordInput = pctx->maxInput + 1;
      else
         wincoordInput = pctx->wincoordInput;

      /* find one free temp reg */
      for (i = 0; i < 32; i++) {
         if ((pctx->tempsUsed & (1 << i)) == 0) {
            /* found a free temp */
            if (pctx->texTemp < 0)
               pctx->texTemp  = i;
            else
               break;
         }
      }
      assert(pctx->texTemp >= 0);

      if (pctx->wincoordInput < 0) {
         /* declare new position input reg */
         decl = tgsi_default_full_declaration();
         decl.Declaration.File = TGSI_FILE_INPUT;
         decl.Declaration.Interpolate = TGSI_INTERPOLATE_LINEAR; /* XXX? */
         decl.Declaration.Semantic = 1;
         decl.Semantic.SemanticName = TGSI_SEMANTIC_POSITION;
         decl.Semantic.SemanticIndex = 0;
         decl.DeclarationRange.First = 
            decl.DeclarationRange.Last = wincoordInput;
         ctx->emit_declaration(ctx, &decl);
      }

      /* declare new sampler */
      decl = tgsi_default_full_declaration();
      decl.Declaration.File = TGSI_FILE_SAMPLER;
      decl.DeclarationRange.First = 
      decl.DeclarationRange.Last = pctx->freeSampler;
      ctx->emit_declaration(ctx, &decl);

      /* declare new temp regs */
      decl = tgsi_default_full_declaration();
      decl.Declaration.File = TGSI_FILE_TEMPORARY;
      decl.DeclarationRange.First = 
      decl.DeclarationRange.Last = pctx->texTemp;
      ctx->emit_declaration(ctx, &decl);

      /* emit immediate = {1/32, 1/32, 1, 1}
       * The index/position of this immediate will be pctx->numImmed
       */
      {
         static const float value[4] = { 1.0/32, 1.0/32, 1.0, 1.0 };
         struct tgsi_full_immediate immed;
         uint size = 4;
         immed = tgsi_default_full_immediate();
         immed.Immediate.NrTokens = 1 + size; /* one for the token itself */
         immed.u[0].Float = value[0];
         immed.u[1].Float = value[1];
         immed.u[2].Float = value[2];
         immed.u[3].Float = value[3];
         ctx->emit_immediate(ctx, &immed);
      }

      pctx->firstInstruction = FALSE;


      /* 
       * Insert new MUL/TEX/KILP instructions at start of program
       * Take gl_FragCoord, divide by 32 (stipple size), sample the
       * texture and kill fragment if needed.
       *
       * We'd like to use non-normalized texcoords to index into a RECT
       * texture, but we can only use GL_REPEAT wrap mode with normalized
       * texcoords.  Darn.
       */

      /* MUL texTemp, INPUT[wincoord], 1/32; */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_MUL;
      newInst.Instruction.NumDstRegs = 1;
      newInst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullDstRegisters[0].DstRegister.Index = pctx->texTemp;
      newInst.Instruction.NumSrcRegs = 2;
      newInst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_INPUT;
      newInst.FullSrcRegisters[0].SrcRegister.Index = wincoordInput;
      newInst.FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_IMMEDIATE;
      newInst.FullSrcRegisters[1].SrcRegister.Index = pctx->numImmed;
      ctx->emit_instruction(ctx, &newInst);

      /* TEX texTemp, texTemp, sampler; */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_TEX;
      newInst.Instruction.NumDstRegs = 1;
      newInst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullDstRegisters[0].DstRegister.Index = pctx->texTemp;
      newInst.Instruction.NumSrcRegs = 2;
      newInst.InstructionExtTexture.Texture = TGSI_TEXTURE_2D;
      newInst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullSrcRegisters[0].SrcRegister.Index = pctx->texTemp;
      newInst.FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_SAMPLER;
      newInst.FullSrcRegisters[1].SrcRegister.Index = pctx->freeSampler;
      ctx->emit_instruction(ctx, &newInst);

      /* KIL -texTemp;   # if -texTemp < 0, KILL fragment */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_KIL;
      newInst.Instruction.NumDstRegs = 0;
      newInst.Instruction.NumSrcRegs = 1;
      newInst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullSrcRegisters[0].SrcRegister.Index = pctx->texTemp;
      newInst.FullSrcRegisters[0].SrcRegister.Negate = 1;
      ctx->emit_instruction(ctx, &newInst);
   }

   /* emit this instruction */
   ctx->emit_instruction(ctx, inst);
}


/**
 * Generate the frag shader we'll use for doing polygon stipple.
 * This will be the user's shader prefixed with a TEX and KIL instruction.
 */
static boolean
generate_pstip_fs(struct pstip_stage *pstip)
{
   const struct pipe_shader_state *orig_fs = &pstip->fs->state;
   /*struct draw_context *draw = pstip->stage.draw;*/
   struct pipe_shader_state pstip_fs;
   struct pstip_transform_context transform;

#define MAX 1000

   pstip_fs = *orig_fs; /* copy to init */
   pstip_fs.tokens = MALLOC(sizeof(struct tgsi_token) * MAX);
   if (pstip_fs.tokens == NULL)
      return FALSE;

   memset(&transform, 0, sizeof(transform));
   transform.wincoordInput = -1;
   transform.maxInput = -1;
   transform.texTemp = -1;
   transform.firstInstruction = TRUE;
   transform.base.transform_instruction = pstip_transform_inst;
   transform.base.transform_declaration = pstip_transform_decl;
   transform.base.transform_immediate = pstip_transform_immed;

   tgsi_transform_shader(orig_fs->tokens,
                         (struct tgsi_token *) pstip_fs.tokens,
                         MAX, &transform.base);

#if 0 /* DEBUG */
   tgsi_dump(orig_fs->tokens, 0);
   tgsi_dump(pstip_fs.tokens, 0);
#endif

   pstip->fs->sampler_unit = transform.freeSampler;
   assert(pstip->fs->sampler_unit < PIPE_MAX_SAMPLERS);

   pstip->fs->pstip_fs = pstip->driver_create_fs_state(pstip->pipe, &pstip_fs);

   FREE((void *)pstip_fs.tokens);
   return TRUE;
}


/**
 * Load texture image with current stipple pattern.
 */
static void
pstip_update_texture(struct pstip_stage *pstip)
{
   static const uint bit31 = 1 << 31;
   struct pipe_context *pipe = pstip->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_transfer *transfer;
   const uint *stipple = pstip->state.stipple->stipple;
   uint i, j;
   ubyte *data;

   /* XXX: want to avoid flushing just because we use stipple: 
    */
   pipe->flush( pipe, PIPE_FLUSH_TEXTURE_CACHE, NULL );

   transfer = screen->get_tex_transfer(screen, pstip->texture, 0, 0, 0,
                                       PIPE_TRANSFER_WRITE, 0, 0, 32, 32);
   data = screen->transfer_map(screen, transfer);

   /*
    * Load alpha texture.
    * Note: 0 means keep the fragment, 255 means kill it.
    * We'll negate the texel value and use KILP which kills if value
    * is negative.
    */
   for (i = 0; i < 32; i++) {
      for (j = 0; j < 32; j++) {
         if (stipple[i] & (bit31 >> j)) {
            /* fragment "on" */
            data[i * transfer->stride + j] = 0;
         }
         else {
            /* fragment "off" */
            data[i * transfer->stride + j] = 255;
         }
      }
   }

   /* unmap */
   screen->transfer_unmap(screen, transfer);
   screen->tex_transfer_destroy(transfer);
}


/**
 * Create the texture map we'll use for stippling.
 */
static boolean
pstip_create_texture(struct pstip_stage *pstip)
{
   struct pipe_context *pipe = pstip->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_texture texTemp;

   memset(&texTemp, 0, sizeof(texTemp));
   texTemp.target = PIPE_TEXTURE_2D;
   texTemp.format = PIPE_FORMAT_A8_UNORM; /* XXX verify supported by driver! */
   texTemp.last_level = 0;
   texTemp.width[0] = 32;
   texTemp.height[0] = 32;
   texTemp.depth[0] = 1;
   pf_get_block(texTemp.format, &texTemp.block);

   pstip->texture = screen->texture_create(screen, &texTemp);
   if (pstip->texture == NULL)
      return FALSE;

   return TRUE;
}


/**
 * Create the sampler CSO that'll be used for stippling.
 */
static boolean
pstip_create_sampler(struct pstip_stage *pstip)
{
   struct pipe_sampler_state sampler;
   struct pipe_context *pipe = pstip->pipe;

   memset(&sampler, 0, sizeof(sampler));
   sampler.wrap_s = PIPE_TEX_WRAP_REPEAT;
   sampler.wrap_t = PIPE_TEX_WRAP_REPEAT;
   sampler.wrap_r = PIPE_TEX_WRAP_REPEAT;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler.normalized_coords = 1;
   sampler.min_lod = 0.0f;
   sampler.max_lod = 0.0f;

   pstip->sampler_cso = pipe->create_sampler_state(pipe, &sampler);
   if (pstip->sampler_cso == NULL)
      return FALSE;
   
   return TRUE;
}


/**
 * When we're about to draw our first stipple polygon in a batch, this function
 * is called to tell the driver to bind our modified fragment shader.
 */
static boolean
bind_pstip_fragment_shader(struct pstip_stage *pstip)
{
   struct draw_context *draw = pstip->stage.draw;
   if (!pstip->fs->pstip_fs &&
       !generate_pstip_fs(pstip))
      return FALSE;

   draw->suspend_flushing = TRUE;
   pstip->driver_bind_fs_state(pstip->pipe, pstip->fs->pstip_fs);
   draw->suspend_flushing = FALSE;
   return TRUE;
}


static INLINE struct pstip_stage *
pstip_stage( struct draw_stage *stage )
{
   return (struct pstip_stage *) stage;
}


static void
pstip_first_tri(struct draw_stage *stage, struct prim_header *header)
{
   struct pstip_stage *pstip = pstip_stage(stage);
   struct pipe_context *pipe = pstip->pipe;
   struct draw_context *draw = stage->draw;
   uint num_samplers;

   assert(stage->draw->rasterizer->poly_stipple_enable);

   /* bind our fragprog */
   if (!bind_pstip_fragment_shader(pstip)) {
      stage->tri = draw_pipe_passthrough_tri;
      stage->tri(stage, header);
      return;
   }
      

   /* how many samplers? */
   /* we'll use sampler/texture[pstip->sampler_unit] for the stipple */
   num_samplers = MAX2(pstip->num_textures, pstip->num_samplers);
   num_samplers = MAX2(num_samplers, pstip->fs->sampler_unit + 1);

   /* plug in our sampler, texture */
   pstip->state.samplers[pstip->fs->sampler_unit] = pstip->sampler_cso;
   pipe_texture_reference(&pstip->state.textures[pstip->fs->sampler_unit],
                          pstip->texture);

   assert(num_samplers <= PIPE_MAX_SAMPLERS);

   draw->suspend_flushing = TRUE;
   pstip->driver_bind_sampler_states(pipe, num_samplers, pstip->state.samplers);
   pstip->driver_set_sampler_textures(pipe, num_samplers, pstip->state.textures);
   draw->suspend_flushing = FALSE;

   /* now really draw first triangle */
   stage->tri = draw_pipe_passthrough_tri;
   stage->tri(stage, header);
}


static void
pstip_flush(struct draw_stage *stage, unsigned flags)
{
   struct draw_context *draw = stage->draw;
   struct pstip_stage *pstip = pstip_stage(stage);
   struct pipe_context *pipe = pstip->pipe;

   stage->tri = pstip_first_tri;
   stage->next->flush( stage->next, flags );

   /* restore original frag shader, texture, sampler state */
   draw->suspend_flushing = TRUE;
   pstip->driver_bind_fs_state(pipe, pstip->fs->driver_fs);
   pstip->driver_bind_sampler_states(pipe, pstip->num_samplers,
                                     pstip->state.samplers);
   pstip->driver_set_sampler_textures(pipe, pstip->num_textures,
                                      pstip->state.textures);
   draw->suspend_flushing = FALSE;
}


static void
pstip_reset_stipple_counter(struct draw_stage *stage)
{
   stage->next->reset_stipple_counter( stage->next );
}


static void
pstip_destroy(struct draw_stage *stage)
{
   struct pstip_stage *pstip = pstip_stage(stage);
   uint i;

   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      pipe_texture_reference(&pstip->state.textures[i], NULL);
   }

   pstip->pipe->delete_sampler_state(pstip->pipe, pstip->sampler_cso);

   pipe_texture_reference(&pstip->texture, NULL);

   draw_free_temp_verts( stage );
   FREE( stage );
}


static struct pstip_stage *
draw_pstip_stage(struct draw_context *draw)
{
   struct pstip_stage *pstip = CALLOC_STRUCT(pstip_stage);

   draw_alloc_temp_verts( &pstip->stage, 8 );

   pstip->stage.draw = draw;
   pstip->stage.name = "pstip";
   pstip->stage.next = NULL;
   pstip->stage.point = draw_pipe_passthrough_point;
   pstip->stage.line = draw_pipe_passthrough_line;
   pstip->stage.tri = pstip_first_tri;
   pstip->stage.flush = pstip_flush;
   pstip->stage.reset_stipple_counter = pstip_reset_stipple_counter;
   pstip->stage.destroy = pstip_destroy;

   return pstip;
}


static struct pstip_stage *
pstip_stage_from_pipe(struct pipe_context *pipe)
{
   struct draw_context *draw = (struct draw_context *) pipe->draw;
   return pstip_stage(draw->pipeline.pstipple);
}


/**
 * This function overrides the driver's create_fs_state() function and
 * will typically be called by the state tracker.
 */
static void *
pstip_create_fs_state(struct pipe_context *pipe,
                       const struct pipe_shader_state *fs)
{
   struct pstip_stage *pstip = pstip_stage_from_pipe(pipe);
   struct pstip_fragment_shader *aafs = CALLOC_STRUCT(pstip_fragment_shader);

   if (aafs) {
      aafs->state = *fs;

      /* pass-through */
      aafs->driver_fs = pstip->driver_create_fs_state(pstip->pipe, fs);
   }

   return aafs;
}


static void
pstip_bind_fs_state(struct pipe_context *pipe, void *fs)
{
   struct pstip_stage *pstip = pstip_stage_from_pipe(pipe);
   struct pstip_fragment_shader *aafs = (struct pstip_fragment_shader *) fs;
   /* save current */
   pstip->fs = aafs;
   /* pass-through */
   pstip->driver_bind_fs_state(pstip->pipe,
                               (aafs ? aafs->driver_fs : NULL));
}


static void
pstip_delete_fs_state(struct pipe_context *pipe, void *fs)
{
   struct pstip_stage *pstip = pstip_stage_from_pipe(pipe);
   struct pstip_fragment_shader *aafs = (struct pstip_fragment_shader *) fs;
   /* pass-through */
   pstip->driver_delete_fs_state(pstip->pipe, aafs->driver_fs);

   if (aafs->pstip_fs)
      pstip->driver_delete_fs_state(pstip->pipe, aafs->pstip_fs);

   FREE(aafs);
}


static void
pstip_bind_sampler_states(struct pipe_context *pipe,
                          unsigned num, void **sampler)
{
   struct pstip_stage *pstip = pstip_stage_from_pipe(pipe);
   uint i;

   /* save current */
   memcpy(pstip->state.samplers, sampler, num * sizeof(void *));
   for (i = num; i < PIPE_MAX_SAMPLERS; i++) {
      pstip->state.samplers[i] = NULL;
   }

   pstip->num_samplers = num;
   /* pass-through */
   pstip->driver_bind_sampler_states(pstip->pipe, num, sampler);
}


static void
pstip_set_sampler_textures(struct pipe_context *pipe,
                           unsigned num, struct pipe_texture **texture)
{
   struct pstip_stage *pstip = pstip_stage_from_pipe(pipe);
   uint i;

   /* save current */
   for (i = 0; i < num; i++) {
      pipe_texture_reference(&pstip->state.textures[i], texture[i]);
   }
   for (; i < PIPE_MAX_SAMPLERS; i++) {
      pipe_texture_reference(&pstip->state.textures[i], NULL);
   }

   pstip->num_textures = num;

   /* pass-through */
   pstip->driver_set_sampler_textures(pstip->pipe, num, texture);
}


static void
pstip_set_polygon_stipple(struct pipe_context *pipe,
                          const struct pipe_poly_stipple *stipple)
{
   struct pstip_stage *pstip = pstip_stage_from_pipe(pipe);

   /* save current */
   pstip->state.stipple = stipple;

   /* pass-through */
   pstip->driver_set_polygon_stipple(pstip->pipe, stipple);

   pstip_update_texture(pstip);
}


/**
 * Called by drivers that want to install this polygon stipple stage
 * into the draw module's pipeline.  This will not be used if the
 * hardware has native support for polygon stipple.
 */
boolean
draw_install_pstipple_stage(struct draw_context *draw,
                            struct pipe_context *pipe)
{
   struct pstip_stage *pstip;

   pipe->draw = (void *) draw;

   /*
    * Create / install pgon stipple drawing / prim stage
    */
   pstip = draw_pstip_stage( draw );
   if (pstip == NULL)
      goto fail;

   draw->pipeline.pstipple = &pstip->stage;

   pstip->pipe = pipe;

   /* create special texture, sampler state */
   if (!pstip_create_texture(pstip))
      goto fail;

   if (!pstip_create_sampler(pstip))
      goto fail;

   /* save original driver functions */
   pstip->driver_create_fs_state = pipe->create_fs_state;
   pstip->driver_bind_fs_state = pipe->bind_fs_state;
   pstip->driver_delete_fs_state = pipe->delete_fs_state;

   pstip->driver_bind_sampler_states = pipe->bind_sampler_states;
   pstip->driver_set_sampler_textures = pipe->set_sampler_textures;
   pstip->driver_set_polygon_stipple = pipe->set_polygon_stipple;

   /* override the driver's functions */
   pipe->create_fs_state = pstip_create_fs_state;
   pipe->bind_fs_state = pstip_bind_fs_state;
   pipe->delete_fs_state = pstip_delete_fs_state;

   pipe->bind_sampler_states = pstip_bind_sampler_states;
   pipe->set_sampler_textures = pstip_set_sampler_textures;
   pipe->set_polygon_stipple = pstip_set_polygon_stipple;

   return TRUE;

 fail:
   if (pstip)
      pstip->stage.destroy( &pstip->stage );

   return FALSE;
}
