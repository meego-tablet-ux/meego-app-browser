/**************************************************************************
 *
 * Copyright © 2010 Jakob Bornecrantz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#define USE_TRACE 0
#define WIDTH 300
#define HEIGHT 300
#define NEAR 30
#define FAR 1000
#define FLIP 0

/* pipe_*_state structs */
#include "pipe/p_state.h"
/* pipe_context */
#include "pipe/p_context.h"
/* pipe_screen */
#include "pipe/p_screen.h"
/* PIPE_* */
#include "pipe/p_defines.h"
/* TGSI_SEMANTIC_{POSITION|GENERIC} */
#include "pipe/p_shader_tokens.h"
/* pipe_buffer_* helpers */
#include "util/u_inlines.h"

/* constant state object helper */
#include "cso_cache/cso_context.h"

/* u_sampler_view_default_template */
#include "util/u_sampler.h"
/* debug_dump_surface_bmp */
#include "util/u_debug.h"
/* util_draw_vertex_buffer helper */
#include "util/u_draw_quad.h"
/* FREE & CALLOC_STRUCT */
#include "util/u_memory.h"
/* util_make_[fragment|vertex]_passthrough_shader */
#include "util/u_simple_shaders.h"

/* sw_screen_create: to get a software pipe driver */
#include "target-helpers/inline_sw_helper.h"
/* debug_screen_wrap: to wrap with debug pipe drivers */
#include "target-helpers/inline_debug_helper.h"
/* null software winsys */
#include "sw/null/null_sw_winsys.h"

struct program
{
	struct pipe_screen *screen;
	struct pipe_context *pipe;
	struct cso_context *cso;

	struct pipe_blend_state blend;
	struct pipe_depth_stencil_alpha_state depthstencil;
	struct pipe_rasterizer_state rasterizer;
	struct pipe_sampler_state sampler;
	struct pipe_viewport_state viewport;
	struct pipe_framebuffer_state framebuffer;
	struct pipe_vertex_element velem[2];

	void *vs;
	void *fs;

	float clear_color[4];

	struct pipe_resource *vbuf;
	struct pipe_resource *target;
	struct pipe_resource *tex;
	struct pipe_sampler_view *view;
};

static void init_prog(struct program *p)
{
	/* create the software rasterizer */
	p->screen = sw_screen_create(null_sw_create());
	/* wrap the screen with any debugger */
	p->screen = debug_screen_wrap(p->screen);

	/* create the pipe driver context and cso context */
	p->pipe = p->screen->context_create(p->screen, NULL);
	p->cso = cso_create_context(p->pipe);

	/* set clear color */
	p->clear_color[0] = 0.3;
	p->clear_color[1] = 0.1;
	p->clear_color[2] = 0.3;
	p->clear_color[3] = 1.0;

	/* vertex buffer */
	{
		float vertices[4][2][4] = {
			{
				{ 0.9f, 0.9f, 0.0f, 1.0f },
				{ 1.0f, 1.0f, 0.0f, 1.0f }
			},
			{
				{ -0.9f, 0.9f, 0.0f, 1.0f },
				{  0.0f, 1.0f, 0.0f, 1.0f }
			},
			{
				{ -0.9f, -0.9f, 0.0f, 1.0f },
				{  0.0f,  0.0f, 1.0f, 1.0f }
			},
			{
				{ 0.9f, -0.9f, 0.0f, 1.0f },
				{ 1.0f,  0.0f, 1.0f, 1.0f }
			}
		};

		p->vbuf = pipe_buffer_create(p->screen, PIPE_BIND_VERTEX_BUFFER, sizeof(vertices));
		pipe_buffer_write(p->pipe, p->vbuf, 0, sizeof(vertices), vertices);
	}

	/* render target texture */
	{
		struct pipe_resource tmplt;
		memset(&tmplt, 0, sizeof(tmplt));
		tmplt.target = PIPE_TEXTURE_2D;
		tmplt.format = PIPE_FORMAT_B8G8R8A8_UNORM; /* All drivers support this */
		tmplt.width0 = WIDTH;
		tmplt.height0 = HEIGHT;
		tmplt.depth0 = 1;
		tmplt.last_level = 0;
		tmplt.bind = PIPE_BIND_RENDER_TARGET;

		p->target = p->screen->resource_create(p->screen, &tmplt);
	}

	/* sampler texture */
	{
		uint32_t *ptr;
		struct pipe_transfer *t;
		struct pipe_resource t_tmplt;
		struct pipe_sampler_view v_tmplt;
		struct pipe_subresource sub;
		struct pipe_box box;

		memset(&t_tmplt, 0, sizeof(t_tmplt));
		t_tmplt.target = PIPE_TEXTURE_2D;
		t_tmplt.format = PIPE_FORMAT_B8G8R8A8_UNORM; /* All drivers support this */
		t_tmplt.width0 = 2;
		t_tmplt.height0 = 2;
		t_tmplt.depth0 = 1;
		t_tmplt.last_level = 0;
		t_tmplt.bind = PIPE_BIND_RENDER_TARGET;

		p->tex = p->screen->resource_create(p->screen, &t_tmplt);

		memset(&sub, 0, sizeof(sub));
		memset(&box, 0, sizeof(box));
		box.width = 2;
		box.height = 2;

		t = p->pipe->get_transfer(p->pipe, p->tex, sub, PIPE_TRANSFER_WRITE, &box);

		ptr = p->pipe->transfer_map(p->pipe, t);
		ptr[0] = 0xffff0000;
		ptr[1] = 0xff0000ff;
		ptr[2] = 0xff00ff00;
		ptr[3] = 0xffffff00;
		p->pipe->transfer_unmap(p->pipe, t);

		p->pipe->transfer_destroy(p->pipe, t);

		u_sampler_view_default_template(&v_tmplt, p->tex, p->tex->format);

		p->view = p->pipe->create_sampler_view(p->pipe, p->tex, &v_tmplt);
	}

	/* disabled blending/masking */
	memset(&p->blend, 0, sizeof(p->blend));
	p->blend.rt[0].colormask = PIPE_MASK_RGBA;

	/* no-op depth/stencil/alpha */
	memset(&p->depthstencil, 0, sizeof(p->depthstencil));

	/* rasterizer */
	memset(&p->rasterizer, 0, sizeof(p->rasterizer));
	p->rasterizer.cull_face = PIPE_FACE_NONE;
	p->rasterizer.gl_rasterization_rules = 1;

	/* sampler */
	memset(&p->sampler, 0, sizeof(p->sampler));
	p->sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
	p->sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
	p->sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
	p->sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
	p->sampler.min_img_filter = PIPE_TEX_MIPFILTER_LINEAR;
	p->sampler.mag_img_filter = PIPE_TEX_MIPFILTER_LINEAR;
	p->sampler.normalized_coords = 1;

	/* drawing destination */
	memset(&p->framebuffer, 0, sizeof(p->framebuffer));
	p->framebuffer.width = WIDTH;
	p->framebuffer.height = HEIGHT;
	p->framebuffer.nr_cbufs = 1;
	p->framebuffer.cbufs[0] = p->screen->get_tex_surface(p->screen, p->target, 0, 0, 0, PIPE_BIND_RENDER_TARGET);

	/* viewport, depth isn't really needed */
	{
		float x = 0;
		float y = 0;
		float z = FAR;
		float half_width = (float)WIDTH / 2.0f;
		float half_height = (float)HEIGHT / 2.0f;
		float half_depth = ((float)FAR - (float)NEAR) / 2.0f;
		float scale, bias;

		if (FLIP) {
			scale = -1.0f;
			bias = (float)HEIGHT;
		} else {
			scale = 1.0f;
			bias = 0.0f;
		}

		p->viewport.scale[0] = half_width;
		p->viewport.scale[1] = half_height * scale;
		p->viewport.scale[2] = half_depth;
		p->viewport.scale[3] = 1.0f;

		p->viewport.translate[0] = half_width + x;
		p->viewport.translate[1] = (half_height + y) * scale + bias;
		p->viewport.translate[2] = half_depth + z;
		p->viewport.translate[3] = 0.0f;
	}

	/* vertex elements state */
	memset(p->velem, 0, sizeof(p->velem));
	p->velem[0].src_offset = 0 * 4 * sizeof(float); /* offset 0, first element */
	p->velem[0].instance_divisor = 0;
	p->velem[0].vertex_buffer_index = 0;
	p->velem[0].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;

	p->velem[1].src_offset = 1 * 4 * sizeof(float); /* offset 16, second element */
	p->velem[1].instance_divisor = 0;
	p->velem[1].vertex_buffer_index = 0;
	p->velem[1].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;

	/* vertex shader */
	{
		const uint semantic_names[] = { TGSI_SEMANTIC_POSITION,
		                                TGSI_SEMANTIC_GENERIC };
		const uint semantic_indexes[] = { 0, 0 };
		p->vs = util_make_vertex_passthrough_shader(p->pipe, 2, semantic_names, semantic_indexes);
	}

	/* fragment shader */
	p->fs = util_make_fragment_tex_shader(p->pipe, TGSI_TEXTURE_2D, TGSI_INTERPOLATE_LINEAR);
}

static void close_prog(struct program *p)
{
	/* unset bound textures as well */
	cso_set_fragment_sampler_views(p->cso, 0, NULL);

	/* unset all state */
	cso_release_all(p->cso);

	p->pipe->delete_vs_state(p->pipe, p->vs);
	p->pipe->delete_fs_state(p->pipe, p->fs);

	pipe_surface_reference(&p->framebuffer.cbufs[0], NULL);
	pipe_sampler_view_reference(&p->view, NULL);
	pipe_resource_reference(&p->target, NULL);
	pipe_resource_reference(&p->tex, NULL);
	pipe_resource_reference(&p->vbuf, NULL);

	cso_destroy_context(p->cso);
	p->pipe->destroy(p->pipe);
	p->screen->destroy(p->screen);

	FREE(p);
}

static void draw(struct program *p)
{
	/* set the render target */
	cso_set_framebuffer(p->cso, &p->framebuffer);

	/* clear the render target */
	p->pipe->clear(p->pipe, PIPE_CLEAR_COLOR, p->clear_color, 0, 0);

	/* set misc state we care about */
	cso_set_blend(p->cso, &p->blend);
	cso_set_depth_stencil_alpha(p->cso, &p->depthstencil);
	cso_set_rasterizer(p->cso, &p->rasterizer);
	cso_set_viewport(p->cso, &p->viewport);

	/* sampler */
	cso_single_sampler(p->cso, 0, &p->sampler);
	cso_single_sampler_done(p->cso);

	/* texture sampler view */
	cso_set_fragment_sampler_views(p->cso, 1, &p->view);

	/* shaders */
	cso_set_fragment_shader_handle(p->cso, p->fs);
	cso_set_vertex_shader_handle(p->cso, p->vs);

	/* vertex element data */
	cso_set_vertex_elements(p->cso, 2, p->velem);

	util_draw_vertex_buffer(p->pipe,
	                        p->vbuf, 0,
	                        PIPE_PRIM_QUADS,
	                        4,  /* verts */
	                        2); /* attribs/vert */

	p->pipe->flush(p->pipe, PIPE_FLUSH_RENDER_CACHE, NULL);

	debug_dump_surface_bmp(p->pipe, "result.bmp", p->framebuffer.cbufs[0]);
}

int main(int argc, char** argv)
{
	struct program *p = CALLOC_STRUCT(program);

	init_prog(p);
	draw(p);
	close_prog(p);

	return 0;
}
