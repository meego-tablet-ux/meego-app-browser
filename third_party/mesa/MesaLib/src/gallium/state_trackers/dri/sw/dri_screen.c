/**************************************************************************
 *
 * Copyright 2009, VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/
/*
 * Author: Keith Whitwell <keithw@vmware.com>
 * Author: Jakob Bornecrantz <wallbraker@gmail.com>
 */

#include "utils.h"
#include "xmlpool.h"

#include "dri_screen.h"
#include "dri_context.h"

#include "util/u_inlines.h"
#include "pipe/p_screen.h"
#include "pipe/p_format.h"
#include "state_tracker/st_gl_api.h" /* for st_gl_api_create */

#include "util/u_debug.h"

PUBLIC const char __driConfigOptions[] =
   DRI_CONF_BEGIN DRI_CONF_SECTION_PERFORMANCE
   DRI_CONF_FTHROTTLE_MODE(DRI_CONF_FTHROTTLE_IRQS)
   DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_DEF_INTERVAL_0)
   DRI_CONF_SECTION_END DRI_CONF_SECTION_QUALITY
/* DRI_CONF_FORCE_S3TC_ENABLE(false) */
   DRI_CONF_ALLOW_LARGE_TEXTURES(1)
   DRI_CONF_SECTION_END DRI_CONF_END;

static const uint __driNConfigOptions = 3;

static const __DRIconfig **
dri_fill_in_modes(struct dri_screen *screen,
		  unsigned pixel_bits)
{
   __DRIconfig **configs = NULL;
   __DRIconfig **configs_r5g6b5 = NULL;
   __DRIconfig **configs_a8r8g8b8 = NULL;
   __DRIconfig **configs_x8r8g8b8 = NULL;
   uint8_t depth_bits_array[5];
   uint8_t stencil_bits_array[5];
   uint8_t msaa_samples_array[5];
   unsigned depth_buffer_factor;
   unsigned back_buffer_factor;
   unsigned msaa_samples_factor;
   unsigned i;
   struct pipe_screen *p_screen = screen->base.screen;
   boolean pf_r5g6b5, pf_a8r8g8b8, pf_x8r8g8b8;
   boolean pf_z16, pf_x8z24, pf_z24x8, pf_s8z24, pf_z24s8, pf_z32;

   static const GLenum back_buffer_modes[] = {
      GLX_NONE, GLX_SWAP_UNDEFINED_OML, GLX_SWAP_COPY_OML
   };

   depth_bits_array[0] = 0;
   stencil_bits_array[0] = 0;
   depth_buffer_factor = 1;

   pf_x8z24 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_Z24X8_UNORM,
					    PIPE_TEXTURE_2D, 0,
					    PIPE_BIND_DEPTH_STENCIL, 0);
   pf_z24x8 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_X8Z24_UNORM,
					    PIPE_TEXTURE_2D, 0,
					    PIPE_BIND_DEPTH_STENCIL, 0);
   pf_s8z24 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_Z24_UNORM_S8_USCALED,
					    PIPE_TEXTURE_2D, 0,
					    PIPE_BIND_DEPTH_STENCIL, 0);
   pf_z24s8 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_S8_USCALED_Z24_UNORM,
					    PIPE_TEXTURE_2D, 0,
					    PIPE_BIND_DEPTH_STENCIL, 0);
   pf_a8r8g8b8 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_B8G8R8A8_UNORM,
					       PIPE_TEXTURE_2D, 0,
					       PIPE_BIND_RENDER_TARGET, 0);
   pf_x8r8g8b8 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_B8G8R8X8_UNORM,
					       PIPE_TEXTURE_2D, 0,
					       PIPE_BIND_RENDER_TARGET, 0);
   pf_r5g6b5 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_B5G6R5_UNORM,
					     PIPE_TEXTURE_2D, 0,
					     PIPE_BIND_RENDER_TARGET, 0);

   /* We can only get a 16 or 32 bit depth buffer with getBuffersWithFormat */
   if (dri_with_format(screen->sPriv)) {
      pf_z16 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_Z16_UNORM,
                                             PIPE_TEXTURE_2D, 0,
                                             PIPE_BIND_DEPTH_STENCIL, 0);
      pf_z32 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_Z32_UNORM,
                                             PIPE_TEXTURE_2D, 0,
                                             PIPE_BIND_DEPTH_STENCIL, 0);
   } else {
      pf_z16 = FALSE;
      pf_z32 = FALSE;
   }

   if (pf_z16) {
      depth_bits_array[depth_buffer_factor] = 16;
      stencil_bits_array[depth_buffer_factor++] = 0;
   }
   if (pf_x8z24 || pf_z24x8) {
      depth_bits_array[depth_buffer_factor] = 24;
      stencil_bits_array[depth_buffer_factor++] = 0;
      screen->d_depth_bits_last = pf_x8z24;
   }
   if (pf_s8z24 || pf_z24s8) {
      depth_bits_array[depth_buffer_factor] = 24;
      stencil_bits_array[depth_buffer_factor++] = 8;
      screen->sd_depth_bits_last = pf_s8z24;
   }
   if (pf_z32) {
      depth_bits_array[depth_buffer_factor] = 32;
      stencil_bits_array[depth_buffer_factor++] = 0;
   }

   msaa_samples_array[0] = 0;
   back_buffer_factor = 3;

   /* also test color for msaa 2/4/6/8 - just assume it'll work for all depth buffers */
   if (pf_r5g6b5) {
      msaa_samples_factor = 1;
      for (i = 1; i < 5; i++) {
         if (p_screen->is_format_supported(p_screen, PIPE_FORMAT_B5G6R5_UNORM,
						   PIPE_TEXTURE_2D, i*2,
						   PIPE_BIND_RENDER_TARGET, 0)) {
            msaa_samples_array[msaa_samples_factor] = i * 2;
            msaa_samples_factor++;
         }
      }

      configs_r5g6b5 = driCreateConfigs(GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
                                        depth_bits_array, stencil_bits_array,
                                        depth_buffer_factor, back_buffer_modes,
                                        back_buffer_factor,
                                        msaa_samples_array, msaa_samples_factor,
                                        GL_TRUE);
   }

   if (pf_a8r8g8b8) {
      msaa_samples_factor = 1;
      for (i = 1; i < 5; i++) {
         if (p_screen->is_format_supported(p_screen, PIPE_FORMAT_B8G8R8A8_UNORM,
						   PIPE_TEXTURE_2D, i*2,
						   PIPE_BIND_RENDER_TARGET, 0)) {
            msaa_samples_array[msaa_samples_factor] = i * 2;
            msaa_samples_factor++;
         }
      }

      configs_a8r8g8b8 = driCreateConfigs(GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                                          depth_bits_array,
                                          stencil_bits_array,
                                          depth_buffer_factor,
                                          back_buffer_modes,
                                          back_buffer_factor,
                                          msaa_samples_array,
                                          msaa_samples_factor,
                                          GL_TRUE);
   }

   if (pf_x8r8g8b8) {
      msaa_samples_factor = 1;
      for (i = 1; i < 5; i++) {
         if (p_screen->is_format_supported(p_screen, PIPE_FORMAT_B8G8R8X8_UNORM,
						   PIPE_TEXTURE_2D, i*2,
						   PIPE_BIND_RENDER_TARGET, 0)) {
            msaa_samples_array[msaa_samples_factor] = i * 2;
            msaa_samples_factor++;
         }
      }

      configs_x8r8g8b8 = driCreateConfigs(GL_BGR, GL_UNSIGNED_INT_8_8_8_8_REV,
                                          depth_bits_array,
                                          stencil_bits_array,
                                          depth_buffer_factor,
                                          back_buffer_modes,
                                          back_buffer_factor,
                                          msaa_samples_array,
                                          msaa_samples_factor,
                                          GL_TRUE);
   }

   if (pixel_bits == 16) {
      configs = configs_r5g6b5;
      if (configs_a8r8g8b8)
         configs = configs ? driConcatConfigs(configs, configs_a8r8g8b8) : configs_a8r8g8b8;
      if (configs_x8r8g8b8)
	 configs = configs ? driConcatConfigs(configs, configs_x8r8g8b8) : configs_x8r8g8b8;
   } else {
      configs = configs_a8r8g8b8;
      if (configs_x8r8g8b8)
	 configs = configs ? driConcatConfigs(configs, configs_x8r8g8b8) : configs_x8r8g8b8;
      if (configs_r5g6b5)
         configs = configs ? driConcatConfigs(configs, configs_r5g6b5) : configs_r5g6b5;
   }

   if (configs == NULL) {
      debug_printf("%s: driCreateConfigs failed\n", __FUNCTION__);
      return NULL;
   }

   return (const __DRIconfig **)configs;
}

/**
 * Roughly the converse of dri_fill_in_modes.
 */
void
dri_fill_st_visual(struct st_visual *stvis, struct dri_screen *screen,
                   const __GLcontextModes *mode)
{
   memset(stvis, 0, sizeof(*stvis));

   stvis->samples = mode->samples;
   stvis->render_buffer = ST_ATTACHMENT_INVALID;

   if (mode->redBits == 8) {
      if (mode->alphaBits == 8)
         stvis->color_format = PIPE_FORMAT_B8G8R8A8_UNORM;
      else
         stvis->color_format = PIPE_FORMAT_B8G8R8X8_UNORM;
   } else {
      stvis->color_format = PIPE_FORMAT_B5G6R5_UNORM;
   }

   switch (mode->depthBits) {
   default:
   case 0:
      stvis->depth_stencil_format = PIPE_FORMAT_NONE;
      break;
   case 16:
      stvis->depth_stencil_format = PIPE_FORMAT_Z16_UNORM;
      break;
   case 24:
      if (mode->stencilBits == 0) {
	 stvis->depth_stencil_format = (screen->d_depth_bits_last) ?
                                          PIPE_FORMAT_Z24X8_UNORM:
                                          PIPE_FORMAT_X8Z24_UNORM;
      } else {
	 stvis->depth_stencil_format = (screen->sd_depth_bits_last) ?
                                          PIPE_FORMAT_Z24_UNORM_S8_USCALED:
                                          PIPE_FORMAT_S8_USCALED_Z24_UNORM;
      }
      break;
   case 32:
      stvis->depth_stencil_format = PIPE_FORMAT_Z32_UNORM;
      break;
   }

   stvis->accum_format = (mode->haveAccumBuffer) ?
      PIPE_FORMAT_R16G16B16A16_SNORM : PIPE_FORMAT_NONE;

   stvis->buffer_mask |= ST_ATTACHMENT_FRONT_LEFT_MASK;
   if (mode->doubleBufferMode)
      stvis->buffer_mask |= ST_ATTACHMENT_BACK_LEFT_MASK;
   if (mode->stereoMode) {
      stvis->buffer_mask |= ST_ATTACHMENT_FRONT_RIGHT_MASK;
      if (mode->doubleBufferMode)
         stvis->buffer_mask |= ST_ATTACHMENT_BACK_RIGHT_MASK;
   }

   if (mode->haveDepthBuffer || mode->haveStencilBuffer)
      stvis->buffer_mask |= ST_ATTACHMENT_DEPTH_STENCIL_MASK;
   /* let the state tracker allocate the accum buffer */
}

static boolean
dri_get_egl_image(struct st_manager *smapi,
                  void *egl_image,
                  struct st_egl_image *stimg)
{
   struct dri_screen *screen = (struct dri_screen *)smapi;
   __DRIimage *img = NULL;

   if (screen->lookup_egl_image) {
      img = screen->lookup_egl_image(screen, egl_image);
   }

   if (!img)
      return FALSE;

   stimg->texture = NULL;
   pipe_resource_reference(&stimg->texture, img->texture);
   stimg->face = img->face;
   stimg->level = img->level;
   stimg->zslice = img->zslice;

   return TRUE;
}

static int
dri_get_param(struct st_manager *smapi,
              enum st_manager_param param)
{
   struct dri_screen *screen = (struct dri_screen *)smapi;

   switch(param) {
   case ST_MANAGER_BROKEN_INVALIDATE:
      return screen->broken_invalidate;
   default:
      return 0;
   }
}

static void
dri_destroy_option_cache(struct dri_screen * screen)
{
   int i;

   if (screen->optionCache.info) {
      for (i = 0; i < (1 << screen->optionCache.tableSize); ++i) {
         FREE(screen->optionCache.info[i].name);
         FREE(screen->optionCache.info[i].ranges);
      }
      FREE(screen->optionCache.info);
   }

   FREE(screen->optionCache.values);
}

void
dri_destroy_screen_helper(struct dri_screen * screen)
{
   if (screen->st_api && screen->st_api->destroy)
      screen->st_api->destroy(screen->st_api);

   if (screen->base.screen)
      screen->base.screen->destroy(screen->base.screen);

   dri_destroy_option_cache(screen);
}

void
dri_destroy_screen(__DRIscreen * sPriv)
{
   struct dri_screen *screen = dri_screen(sPriv);

   dri_destroy_screen_helper(screen);

   FREE(screen);
   sPriv->private = NULL;
   sPriv->extensions = NULL;
}

const __DRIconfig **
dri_init_screen_helper(struct dri_screen *screen,
                       struct pipe_screen *pscreen,
                       unsigned pixel_bits)
{
   screen->base.screen = pscreen;
   if (!screen->base.screen) {
      debug_printf("%s: failed to create pipe_screen\n", __FUNCTION__);
      return NULL;
   }

   screen->base.get_egl_image = dri_get_egl_image;
   screen->base.get_param = dri_get_param;

   screen->st_api = st_gl_api_create();
   if (!screen->st_api)
      return NULL;

   if(pscreen->get_param(pscreen, PIPE_CAP_NPOT_TEXTURES))
      screen->target = PIPE_TEXTURE_2D;
   else
      screen->target = PIPE_TEXTURE_RECT;

   driParseOptionInfo(&screen->optionCache,
                      __driConfigOptions, __driNConfigOptions);

   return dri_fill_in_modes(screen, pixel_bits);
}

/* vim: set sw=3 ts=8 sts=3 expandtab: */
