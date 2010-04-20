/*
 * Copyright 2009 Nicolai Hähnle <nhaehnle@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#ifndef RADEON_CODE_H
#define RADEON_CODE_H

#include <stdint.h>

#define R300_PFS_MAX_ALU_INST     64
#define R300_PFS_MAX_TEX_INST     32
#define R300_PFS_MAX_TEX_INDIRECT 4
#define R300_PFS_NUM_TEMP_REGS    32
#define R300_PFS_NUM_CONST_REGS   32

#define R500_PFS_MAX_INST         512
#define R500_PFS_NUM_TEMP_REGS    128
#define R500_PFS_NUM_CONST_REGS   256


#define STATE_R300_WINDOW_DIMENSION (STATE_INTERNAL_DRIVER+0)

enum {
	/**
	 * External constants are constants whose meaning is unknown to this
	 * compiler. For example, a Mesa gl_program's constants are turned
	 * into external constants.
	 */
	RC_CONSTANT_EXTERNAL = 0,

	RC_CONSTANT_IMMEDIATE,

	/**
	 * Constant referring to state that is known by this compiler,
	 * see RC_STATE_xxx, i.e. *not* arbitrary Mesa (or other) state.
	 */
	RC_CONSTANT_STATE
};

enum {
	RC_STATE_SHADOW_AMBIENT = 0,

	RC_STATE_R300_WINDOW_DIMENSION,
	RC_STATE_R300_TEXRECT_FACTOR
};

struct rc_constant {
	unsigned Type:2; /**< RC_CONSTANT_xxx */
	unsigned Size:3;

	union {
		unsigned External;
		float Immediate[4];
		unsigned State[2];
	} u;
};

struct rc_constant_list {
	struct rc_constant * Constants;
	unsigned Count;

	unsigned _Reserved;
};

void rc_constants_init(struct rc_constant_list * c);
void rc_constants_copy(struct rc_constant_list * dst, struct rc_constant_list * src);
void rc_constants_destroy(struct rc_constant_list * c);
unsigned rc_constants_add(struct rc_constant_list * c, struct rc_constant * constant);
unsigned rc_constants_add_state(struct rc_constant_list * c, unsigned state1, unsigned state2);
unsigned rc_constants_add_immediate_vec4(struct rc_constant_list * c, const float * data);
unsigned rc_constants_add_immediate_scalar(struct rc_constant_list * c, float data, unsigned * swizzle);

/**
 * Compare functions.
 *
 * \note By design, RC_COMPARE_FUNC_xxx + GL_NEVER gives you
 * the correct GL compare function.
 */
typedef enum {
	RC_COMPARE_FUNC_NEVER = 0,
	RC_COMPARE_FUNC_LESS,
	RC_COMPARE_FUNC_EQUAL,
	RC_COMPARE_FUNC_LEQUAL,
	RC_COMPARE_FUNC_GREATER,
	RC_COMPARE_FUNC_NOTEQUAL,
	RC_COMPARE_FUNC_GEQUAL,
	RC_COMPARE_FUNC_ALWAYS
} rc_compare_func;

/**
 * Stores state that influences the compilation of a fragment program.
 */
struct r300_fragment_program_external_state {
	struct {
		/**
		 * If the sampler is used as a shadow sampler,
		 * this field is:
		 *  0 - GL_LUMINANCE
		 *  1 - GL_INTENSITY
		 *  2 - GL_ALPHA
		 * depending on the depth texture mode.
		 */
		unsigned depth_texture_mode : 2;

		/**
		 * If the sampler is used as a shadow sampler,
		 * this field specifies the compare function.
		 *
		 * Otherwise, this field is \ref RC_COMPARE_FUNC_NEVER (aka 0).
		 *
		 * Otherwise, this field is 0.
		 * \sa rc_compare_func
		 */
		unsigned texture_compare_func : 3;
	} unit[16];
};



struct r300_fragment_program_node {
	int tex_offset; /**< first tex instruction */
	int tex_end; /**< last tex instruction, relative to tex_offset */
	int alu_offset; /**< first ALU instruction */
	int alu_end; /**< last ALU instruction, relative to alu_offset */
	int flags;
};

/**
 * Stores an R300 fragment program in its compiled-to-hardware form.
 */
struct r300_fragment_program_code {
	struct {
		int length; /**< total # of texture instructions used */
		uint32_t inst[R300_PFS_MAX_TEX_INST];
	} tex;

	struct {
		int length; /**< total # of ALU instructions used */
		struct {
			uint32_t rgb_inst;
			uint32_t rgb_addr;
			uint32_t alpha_inst;
			uint32_t alpha_addr;
		} inst[R300_PFS_MAX_ALU_INST];
	} alu;

	uint32_t config; /* US_CONFIG */
	uint32_t pixsize; /* US_PIXSIZE */
	uint32_t code_offset; /* US_CODE_OFFSET */
	uint32_t code_addr[4]; /* US_CODE_ADDR */
};


struct r500_fragment_program_code {
	struct {
		uint32_t inst0;
		uint32_t inst1;
		uint32_t inst2;
		uint32_t inst3;
		uint32_t inst4;
		uint32_t inst5;
	} inst[R500_PFS_MAX_INST];

	int inst_end; /* Number of instructions - 1; also, last instruction to be executed */

	int max_temp_idx;

	uint32_t us_fc_ctrl;
};

struct rX00_fragment_program_code {
	union {
		struct r300_fragment_program_code r300;
		struct r500_fragment_program_code r500;
	} code;

	unsigned writes_depth:1;

	struct rc_constant_list constants;
};


#define VSF_MAX_FRAGMENT_LENGTH (255*4)
#define VSF_MAX_FRAGMENT_TEMPS (14)

#define VSF_MAX_INPUTS 32
#define VSF_MAX_OUTPUTS 32

struct r300_vertex_program_code {
	int length;
	union {
		uint32_t d[VSF_MAX_FRAGMENT_LENGTH];
		float f[VSF_MAX_FRAGMENT_LENGTH];
	} body;

	int pos_end;
	int num_temporaries;	/* Number of temp vars used by program */
	int inputs[VSF_MAX_INPUTS];
	int outputs[VSF_MAX_OUTPUTS];

	struct rc_constant_list constants;

	uint32_t InputsRead;
	uint32_t OutputsWritten;
};

void r300_vertex_program_dump(struct r300_vertex_program_code * vs);

#endif /* RADEON_CODE_H */

