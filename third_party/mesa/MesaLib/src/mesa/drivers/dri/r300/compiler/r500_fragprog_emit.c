/*
 * Copyright (C) 2005 Ben Skeggs.
 *
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 * Adaptation and modification for ATI/AMD Radeon R500 GPU chipsets.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/**
 * \file
 *
 * \author Ben Skeggs <darktama@iinet.net.au>
 *
 * \author Jerome Glisse <j.glisse@gmail.com>
 *
 * \author Corbin Simpson <MostAwesomeDude@gmail.com>
 *
 */

#include "r500_fragprog.h"

#include "../r300_reg.h"

#include "radeon_program_pair.h"


#define PROG_CODE \
	struct r500_fragment_program_code *code = &c->code->code.r500

#define error(fmt, args...) do {			\
		rc_error(&c->Base, "%s::%s(): " fmt "\n",	\
			__FILE__, __FUNCTION__, ##args);	\
	} while(0)


struct branch_info {
	int If;
	int Else;
	int Endif;
};

struct emit_state {
	struct radeon_compiler * C;
	struct r500_fragment_program_code * Code;

	struct branch_info * Branches;
	unsigned int CurrentBranchDepth;
	unsigned int BranchesReserved;

	unsigned int MaxBranchDepth;
};

static unsigned int translate_rgb_op(struct r300_fragment_program_compiler *c, rc_opcode opcode)
{
	switch(opcode) {
	case RC_OPCODE_CMP: return R500_ALU_RGBA_OP_CMP;
	case RC_OPCODE_DDX: return R500_ALU_RGBA_OP_MDH;
	case RC_OPCODE_DDY: return R500_ALU_RGBA_OP_MDV;
	case RC_OPCODE_DP3: return R500_ALU_RGBA_OP_DP3;
	case RC_OPCODE_DP4: return R500_ALU_RGBA_OP_DP4;
	case RC_OPCODE_FRC: return R500_ALU_RGBA_OP_FRC;
	default:
		error("translate_rgb_op(%d): unknown opcode\n", opcode);
		/* fall through */
	case RC_OPCODE_NOP:
		/* fall through */
	case RC_OPCODE_MAD: return R500_ALU_RGBA_OP_MAD;
	case RC_OPCODE_MAX: return R500_ALU_RGBA_OP_MAX;
	case RC_OPCODE_MIN: return R500_ALU_RGBA_OP_MIN;
	case RC_OPCODE_REPL_ALPHA: return R500_ALU_RGBA_OP_SOP;
	}
}

static unsigned int translate_alpha_op(struct r300_fragment_program_compiler *c, rc_opcode opcode)
{
	switch(opcode) {
	case RC_OPCODE_CMP: return R500_ALPHA_OP_CMP;
	case RC_OPCODE_COS: return R500_ALPHA_OP_COS;
	case RC_OPCODE_DDX: return R500_ALPHA_OP_MDH;
	case RC_OPCODE_DDY: return R500_ALPHA_OP_MDV;
	case RC_OPCODE_DP3: return R500_ALPHA_OP_DP;
	case RC_OPCODE_DP4: return R500_ALPHA_OP_DP;
	case RC_OPCODE_EX2: return R500_ALPHA_OP_EX2;
	case RC_OPCODE_FRC: return R500_ALPHA_OP_FRC;
	case RC_OPCODE_LG2: return R500_ALPHA_OP_LN2;
	default:
		error("translate_alpha_op(%d): unknown opcode\n", opcode);
		/* fall through */
	case RC_OPCODE_NOP:
		/* fall through */
	case RC_OPCODE_MAD: return R500_ALPHA_OP_MAD;
	case RC_OPCODE_MAX: return R500_ALPHA_OP_MAX;
	case RC_OPCODE_MIN: return R500_ALPHA_OP_MIN;
	case RC_OPCODE_RCP: return R500_ALPHA_OP_RCP;
	case RC_OPCODE_RSQ: return R500_ALPHA_OP_RSQ;
	case RC_OPCODE_SIN: return R500_ALPHA_OP_SIN;
	}
}

static unsigned int fix_hw_swizzle(unsigned int swz)
{
	if (swz == 5) swz = 6;
	if (swz == RC_SWIZZLE_UNUSED) swz = 4;
	return swz;
}

static unsigned int translate_arg_rgb(struct rc_pair_instruction *inst, int arg)
{
	unsigned int t = inst->RGB.Arg[arg].Source;
	int comp;
	t |= inst->RGB.Arg[arg].Negate << 11;
	t |= inst->RGB.Arg[arg].Abs << 12;

	for(comp = 0; comp < 3; ++comp)
		t |= fix_hw_swizzle(GET_SWZ(inst->RGB.Arg[arg].Swizzle, comp)) << (3*comp + 2);

	return t;
}

static unsigned int translate_arg_alpha(struct rc_pair_instruction *inst, int i)
{
	unsigned int t = inst->Alpha.Arg[i].Source;
	t |= fix_hw_swizzle(inst->Alpha.Arg[i].Swizzle) << 2;
	t |= inst->Alpha.Arg[i].Negate << 5;
	t |= inst->Alpha.Arg[i].Abs << 6;
	return t;
}

static uint32_t translate_alu_result_op(struct r300_fragment_program_compiler * c, rc_compare_func func)
{
	switch(func) {
	case RC_COMPARE_FUNC_EQUAL: return R500_INST_ALU_RESULT_OP_EQ;
	case RC_COMPARE_FUNC_LESS: return R500_INST_ALU_RESULT_OP_LT;
	case RC_COMPARE_FUNC_GEQUAL: return R500_INST_ALU_RESULT_OP_GE;
	case RC_COMPARE_FUNC_NOTEQUAL: return R500_INST_ALU_RESULT_OP_NE;
	default:
		rc_error(&c->Base, "%s: unsupported compare func %i\n", __FUNCTION__, func);
		return 0;
	}
}

static void use_temporary(struct r500_fragment_program_code* code, unsigned int index)
{
	if (index > code->max_temp_idx)
		code->max_temp_idx = index;
}

static unsigned int use_source(struct r500_fragment_program_code* code, struct radeon_pair_instruction_source src)
{
	if (src.File == RC_FILE_CONSTANT) {
		return src.Index | 0x100;
	} else if (src.File == RC_FILE_TEMPORARY) {
		use_temporary(code, src.Index);
		return src.Index;
	}

	return 0;
}


/**
 * Emit a paired ALU instruction.
 */
static void emit_paired(struct r300_fragment_program_compiler *c, struct rc_pair_instruction *inst)
{
	PROG_CODE;

	if (code->inst_end >= 511) {
		error("emit_alu: Too many instructions");
		return;
	}

	int ip = ++code->inst_end;

	code->inst[ip].inst5 = translate_rgb_op(c, inst->RGB.Opcode);
	code->inst[ip].inst4 = translate_alpha_op(c, inst->Alpha.Opcode);

	if (inst->RGB.OutputWriteMask || inst->Alpha.OutputWriteMask || inst->Alpha.DepthWriteMask) {
		code->inst[ip].inst0 = R500_INST_TYPE_OUT;
		if (inst->WriteALUResult) {
			error("%s: cannot write output and ALU result at the same time");
			return;
		}
	} else {
		code->inst[ip].inst0 = R500_INST_TYPE_ALU;
	}
	code->inst[ip].inst0 |= R500_INST_TEX_SEM_WAIT;

	code->inst[ip].inst0 |= (inst->RGB.WriteMask << 11) | (inst->Alpha.WriteMask << 14);
	code->inst[ip].inst0 |= (inst->RGB.OutputWriteMask << 15) | (inst->Alpha.OutputWriteMask << 18);
	if (inst->Alpha.DepthWriteMask) {
		code->inst[ip].inst4 |= R500_ALPHA_W_OMASK;
		c->code->writes_depth = 1;
	}

	code->inst[ip].inst4 |= R500_ALPHA_ADDRD(inst->Alpha.DestIndex);
	code->inst[ip].inst5 |= R500_ALU_RGBA_ADDRD(inst->RGB.DestIndex);
	use_temporary(code, inst->Alpha.DestIndex);
	use_temporary(code, inst->RGB.DestIndex);

	if (inst->RGB.Saturate)
		code->inst[ip].inst0 |= R500_INST_RGB_CLAMP;
	if (inst->Alpha.Saturate)
		code->inst[ip].inst0 |= R500_INST_ALPHA_CLAMP;

	code->inst[ip].inst1 |= R500_RGB_ADDR0(use_source(code, inst->RGB.Src[0]));
	code->inst[ip].inst1 |= R500_RGB_ADDR1(use_source(code, inst->RGB.Src[1]));
	code->inst[ip].inst1 |= R500_RGB_ADDR2(use_source(code, inst->RGB.Src[2]));

	code->inst[ip].inst2 |= R500_ALPHA_ADDR0(use_source(code, inst->Alpha.Src[0]));
	code->inst[ip].inst2 |= R500_ALPHA_ADDR1(use_source(code, inst->Alpha.Src[1]));
	code->inst[ip].inst2 |= R500_ALPHA_ADDR2(use_source(code, inst->Alpha.Src[2]));

	code->inst[ip].inst3 |= translate_arg_rgb(inst, 0) << R500_ALU_RGB_SEL_A_SHIFT;
	code->inst[ip].inst3 |= translate_arg_rgb(inst, 1) << R500_ALU_RGB_SEL_B_SHIFT;
	code->inst[ip].inst5 |= translate_arg_rgb(inst, 2) << R500_ALU_RGBA_SEL_C_SHIFT;

	code->inst[ip].inst4 |= translate_arg_alpha(inst, 0) << R500_ALPHA_SEL_A_SHIFT;
	code->inst[ip].inst4 |= translate_arg_alpha(inst, 1) << R500_ALPHA_SEL_B_SHIFT;
	code->inst[ip].inst5 |= translate_arg_alpha(inst, 2) << R500_ALU_RGBA_ALPHA_SEL_C_SHIFT;

	if (inst->WriteALUResult) {
		code->inst[ip].inst3 |= R500_ALU_RGB_WMASK;

		if (inst->WriteALUResult == RC_ALURESULT_X)
			code->inst[ip].inst0 |= R500_INST_ALU_RESULT_SEL_RED;
		else
			code->inst[ip].inst0 |= R500_INST_ALU_RESULT_SEL_ALPHA;

		code->inst[ip].inst0 |= translate_alu_result_op(c, inst->ALUResultCompare);
	}
}

static unsigned int translate_strq_swizzle(unsigned int swizzle)
{
	unsigned int swiz = 0;
	int i;
	for (i = 0; i < 4; i++)
		swiz |= (GET_SWZ(swizzle, i) & 0x3) << i*2;
	return swiz;
}

/**
 * Emit a single TEX instruction
 */
static int emit_tex(struct r300_fragment_program_compiler *c, struct rc_sub_instruction *inst)
{
	PROG_CODE;

	if (code->inst_end >= 511) {
		error("emit_tex: Too many instructions");
		return 0;
	}

	int ip = ++code->inst_end;

	code->inst[ip].inst0 = R500_INST_TYPE_TEX
		| (inst->DstReg.WriteMask << 11)
		| R500_INST_TEX_SEM_WAIT;
	code->inst[ip].inst1 = R500_TEX_ID(inst->TexSrcUnit)
		| R500_TEX_SEM_ACQUIRE | R500_TEX_IGNORE_UNCOVERED;

	if (inst->TexSrcTarget == RC_TEXTURE_RECT)
		code->inst[ip].inst1 |= R500_TEX_UNSCALED;

	switch (inst->Opcode) {
	case RC_OPCODE_KIL:
		code->inst[ip].inst1 |= R500_TEX_INST_TEXKILL;
		break;
	case RC_OPCODE_TEX:
		code->inst[ip].inst1 |= R500_TEX_INST_LD;
		break;
	case RC_OPCODE_TXB:
		code->inst[ip].inst1 |= R500_TEX_INST_LODBIAS;
		break;
	case RC_OPCODE_TXP:
		code->inst[ip].inst1 |= R500_TEX_INST_PROJ;
		break;
	default:
		error("emit_tex can't handle opcode %x\n", inst->Opcode);
	}

	use_temporary(code, inst->SrcReg[0].Index);
	if (inst->Opcode != RC_OPCODE_KIL)
		use_temporary(code, inst->DstReg.Index);

	code->inst[ip].inst2 = R500_TEX_SRC_ADDR(inst->SrcReg[0].Index)
		| (translate_strq_swizzle(inst->SrcReg[0].Swizzle) << 8)
		| R500_TEX_DST_ADDR(inst->DstReg.Index)
		| R500_TEX_DST_R_SWIZ_R | R500_TEX_DST_G_SWIZ_G
		| R500_TEX_DST_B_SWIZ_B | R500_TEX_DST_A_SWIZ_A;

	return 1;
}

static void grow_branches(struct emit_state * s)
{
	unsigned int newreserved = s->BranchesReserved * 2;
	struct branch_info * newbranches;

	if (!newreserved)
		newreserved = 4;

	newbranches = memory_pool_malloc(&s->C->Pool, newreserved*sizeof(struct branch_info));
	memcpy(newbranches, s->Branches, s->CurrentBranchDepth*sizeof(struct branch_info));

	s->Branches = newbranches;
	s->BranchesReserved = newreserved;
}

static void emit_flowcontrol(struct emit_state * s, struct rc_instruction * inst)
{
	if (s->Code->inst_end >= 511) {
		rc_error(s->C, "emit_tex: Too many instructions");
		return;
	}

	unsigned int newip = ++s->Code->inst_end;

	s->Code->inst[newip].inst0 = R500_INST_TYPE_FC | R500_INST_ALU_WAIT;

	if (inst->U.I.Opcode == RC_OPCODE_IF) {
		if (s->CurrentBranchDepth >= 32) {
			rc_error(s->C, "Branch depth exceeds hardware limit");
			return;
		}

		if (s->CurrentBranchDepth >= s->BranchesReserved)
			grow_branches(s);

		struct branch_info * branch = &s->Branches[s->CurrentBranchDepth++];
		branch->If = newip;
		branch->Else = -1;
		branch->Endif = -1;

		if (s->CurrentBranchDepth > s->MaxBranchDepth)
			s->MaxBranchDepth = s->CurrentBranchDepth;

		/* actual instruction is filled in at ENDIF time */
	} else if (inst->U.I.Opcode == RC_OPCODE_ELSE) {
		if (!s->CurrentBranchDepth) {
			rc_error(s->C, "%s: got ELSE outside a branch", __FUNCTION__);
			return;
		}

		struct branch_info * branch = &s->Branches[s->CurrentBranchDepth - 1];
		branch->Else = newip;

		/* actual instruction is filled in at ENDIF time */
	} else if (inst->U.I.Opcode == RC_OPCODE_ENDIF) {
		if (!s->CurrentBranchDepth) {
			rc_error(s->C, "%s: got ELSE outside a branch", __FUNCTION__);
			return;
		}

		struct branch_info * branch = &s->Branches[s->CurrentBranchDepth - 1];
		branch->Endif = newip;

		s->Code->inst[branch->If].inst2 = R500_FC_OP_JUMP
			| R500_FC_A_OP_NONE /* no address stack */
			| R500_FC_JUMP_FUNC(0x0f) /* jump if ALU result is false */
			| R500_FC_B_OP0_INCR /* increment branch counter if stay */
		;

		if (branch->Else >= 0) {
			/* increment branch counter also if jump */
			s->Code->inst[branch->If].inst2 |= R500_FC_B_OP1_INCR;
			s->Code->inst[branch->If].inst3 = R500_FC_JUMP_ADDR(branch->Else + 1);

			s->Code->inst[branch->Else].inst2 = R500_FC_OP_JUMP
				| R500_FC_A_OP_NONE /* no address stack */
				| R500_FC_B_ELSE /* all active pixels want to jump */
				| R500_FC_B_OP0_NONE /* no counter op if stay */
				| R500_FC_B_OP1_DECR /* decrement branch counter if jump */
				| R500_FC_B_POP_CNT(1)
			;
			s->Code->inst[branch->Else].inst3 = R500_FC_JUMP_ADDR(branch->Endif + 1);
		} else {
			/* don't touch branch counter on jump */
			s->Code->inst[branch->If].inst2 |= R500_FC_B_OP1_NONE;
			s->Code->inst[branch->If].inst3 = R500_FC_JUMP_ADDR(branch->Endif + 1);
		}

		s->Code->inst[branch->Endif].inst2 = R500_FC_OP_JUMP
			| R500_FC_A_OP_NONE /* no address stack */
			| R500_FC_JUMP_ANY /* docs says set this, but I don't understand why */
			| R500_FC_B_OP0_DECR /* decrement branch counter if stay */
			| R500_FC_B_OP1_NONE /* no branch counter if stay */
			| R500_FC_B_POP_CNT(1)
		;
		s->Code->inst[branch->Endif].inst3 = R500_FC_JUMP_ADDR(branch->Endif + 1);

		s->CurrentBranchDepth--;
	} else {
		rc_error(s->C, "%s: unknown opcode %i\n", __FUNCTION__, inst->U.I.Opcode);
	}
}

void r500BuildFragmentProgramHwCode(struct r300_fragment_program_compiler *compiler)
{
	struct emit_state s;
	struct r500_fragment_program_code *code = &compiler->code->code.r500;

	memset(&s, 0, sizeof(s));
	s.C = &compiler->Base;
	s.Code = code;

	memset(code, 0, sizeof(*code));
	code->max_temp_idx = 1;
	code->inst_end = -1;

	for(struct rc_instruction * inst = compiler->Base.Program.Instructions.Next;
	    inst != &compiler->Base.Program.Instructions && !compiler->Base.Error;
	    inst = inst->Next) {
		if (inst->Type == RC_INSTRUCTION_NORMAL) {
			const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->U.I.Opcode);

			if (opcode->IsFlowControl) {
				emit_flowcontrol(&s, inst);
			} else if (inst->U.I.Opcode == RC_OPCODE_BEGIN_TEX) {
				continue;
			} else {
				emit_tex(compiler, &inst->U.I);
			}
		} else {
			emit_paired(compiler, &inst->U.P);
		}
	}

	if (code->max_temp_idx >= 128)
		rc_error(&compiler->Base, "Too many hardware temporaries used");

	if (compiler->Base.Error)
		return;

	if ((code->inst[code->inst_end].inst0 & R500_INST_TYPE_MASK) != R500_INST_TYPE_OUT) {
		/* This may happen when dead-code elimination is disabled or
		 * when most of the fragment program logic is leading to a KIL */
		if (code->inst_end >= 511) {
			rc_error(&compiler->Base, "Introducing fake OUT: Too many instructions");
			return;
		}

		int ip = ++code->inst_end;
		code->inst[ip].inst0 = R500_INST_TYPE_OUT | R500_INST_TEX_SEM_WAIT;
	}

	if (s.MaxBranchDepth >= 4) {
		if (code->max_temp_idx < 1)
			code->max_temp_idx = 1;

		code->us_fc_ctrl |= R500_FC_FULL_FC_EN;
	}
}
