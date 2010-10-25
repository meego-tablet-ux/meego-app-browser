/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
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
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "r600_asm.h"
#include "r600_context.h"
#include "util/u_memory.h"
#include "r700_sq.h"
#include <stdio.h>


int r700_bc_alu_build(struct r600_bc *bc, struct r600_bc_alu *alu, unsigned id)
{
	unsigned i;

	bc->bytecode[id++] = S_SQ_ALU_WORD0_SRC0_SEL(alu->src[0].sel) |
		S_SQ_ALU_WORD0_SRC0_REL(alu->src[0].rel) |
		S_SQ_ALU_WORD0_SRC0_CHAN(alu->src[0].chan) |
		S_SQ_ALU_WORD0_SRC0_NEG(alu->src[0].neg) |
		S_SQ_ALU_WORD0_SRC1_SEL(alu->src[1].sel) |
		S_SQ_ALU_WORD0_SRC0_REL(alu->src[1].rel) |
		S_SQ_ALU_WORD0_SRC1_CHAN(alu->src[1].chan) |
		S_SQ_ALU_WORD0_SRC1_NEG(alu->src[1].neg) |
		S_SQ_ALU_WORD0_LAST(alu->last);

	/* don't replace gpr by pv or ps for destination register */
	if (alu->is_op3) {
		bc->bytecode[id++] = S_SQ_ALU_WORD1_DST_GPR(alu->dst.sel) |
					S_SQ_ALU_WORD1_DST_CHAN(alu->dst.chan) |
			                S_SQ_ALU_WORD1_DST_REL(alu->dst.rel) |
			                S_SQ_ALU_WORD1_CLAMP(alu->dst.clamp) |
					S_SQ_ALU_WORD1_OP3_SRC2_SEL(alu->src[2].sel) |
					S_SQ_ALU_WORD1_OP3_SRC2_REL(alu->src[2].rel) |
					S_SQ_ALU_WORD1_OP3_SRC2_CHAN(alu->src[2].chan) |
					S_SQ_ALU_WORD1_OP3_SRC2_NEG(alu->src[2].neg) |
					S_SQ_ALU_WORD1_OP3_ALU_INST(alu->inst) |
					S_SQ_ALU_WORD1_BANK_SWIZZLE(alu->bank_swizzle);
	} else {
		bc->bytecode[id++] = S_SQ_ALU_WORD1_DST_GPR(alu->dst.sel) |
					S_SQ_ALU_WORD1_DST_CHAN(alu->dst.chan) |
			                S_SQ_ALU_WORD1_DST_REL(alu->dst.rel) |
			                S_SQ_ALU_WORD1_CLAMP(alu->dst.clamp) |
					S_SQ_ALU_WORD1_OP2_SRC0_ABS(alu->src[0].abs) |
					S_SQ_ALU_WORD1_OP2_SRC1_ABS(alu->src[1].abs) |
					S_SQ_ALU_WORD1_OP2_WRITE_MASK(alu->dst.write) |
					S_SQ_ALU_WORD1_OP2_ALU_INST(alu->inst) |
					S_SQ_ALU_WORD1_BANK_SWIZZLE(alu->bank_swizzle) |
			                S_SQ_ALU_WORD1_OP2_UPDATE_EXECUTE_MASK(alu->predicate) |
		 	                S_SQ_ALU_WORD1_OP2_UPDATE_PRED(alu->predicate);
	}
	if (alu->last) {
		if (alu->nliteral && !alu->literal_added) {
			R600_ERR("Bug in ALU processing for instruction 0x%08x, literal not added correctly\n", alu->inst);
		}
		for (i = 0; i < alu->nliteral; i++) {
			bc->bytecode[id++] = alu->value[i];
		}
	}
	return 0;
}
