/*
 * Copyright © 2010 Intel Corporation
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file ir_tree_grafting.cpp
 *
 * Takes assignments to variables that are dereferenced only once and
 * pastes the RHS expression into where the variable is dereferenced.
 *
 * In the process of various operations like function inlining and
 * tertiary op handling, we'll end up with our expression trees having
 * been chopped up into a series of assignments of short expressions
 * to temps.  Other passes like ir_algebraic.cpp would prefer to see
 * the deepest expression trees they can to try to optimize them.
 *
 * This is a lot like copy propagaton.  In comparison, copy
 * propagation only acts on plain copies, not arbitrary expressions on
 * the RHS.  Generally, we wouldn't want to go pasting some
 * complicated expression everywhere it got used, though, so we don't
 * handle expressions in that pass.
 *
 * The hard part is making sure we don't move an expression across
 * some other assignments that would change the value of the
 * expression.  So we split this into two passes: First, find the
 * variables in our scope which are written to once and read once, and
 * then go through basic blocks seeing if we find an opportunity to
 * move those expressions safely.
 */

#include "ir.h"
#include "ir_visitor.h"
#include "ir_variable_refcount.h"
#include "ir_basic_block.h"
#include "ir_optimization.h"
#include "glsl_types.h"

static bool debug = false;

class ir_tree_grafting_visitor : public ir_hierarchical_visitor {
public:
   ir_tree_grafting_visitor(ir_assignment *graft_assign,
			    ir_variable *graft_var)
   {
      this->progress = false;
      this->graft_assign = graft_assign;
      this->graft_var = graft_var;
   }

   virtual ir_visitor_status visit_leave(class ir_assignment *);
   virtual ir_visitor_status visit_enter(class ir_call *);
   virtual ir_visitor_status visit_enter(class ir_expression *);
   virtual ir_visitor_status visit_enter(class ir_function *);
   virtual ir_visitor_status visit_enter(class ir_function_signature *);
   virtual ir_visitor_status visit_enter(class ir_if *);
   virtual ir_visitor_status visit_enter(class ir_loop *);
   virtual ir_visitor_status visit_enter(class ir_swizzle *);
   virtual ir_visitor_status visit_enter(class ir_texture *);

   bool do_graft(ir_rvalue **rvalue);

   bool progress;
   ir_variable *graft_var;
   ir_assignment *graft_assign;
};

struct find_deref_info {
   ir_variable *var;
   bool found;
};

void
dereferences_variable_callback(ir_instruction *ir, void *data)
{
   struct find_deref_info *info = (struct find_deref_info *)data;
   ir_dereference_variable *deref = ir->as_dereference_variable();

   if (deref && deref->var == info->var)
      info->found = true;
}

static bool
dereferences_variable(ir_instruction *ir, ir_variable *var)
{
   struct find_deref_info info;

   info.var = var;
   info.found = false;

   visit_tree(ir, dereferences_variable_callback, &info);

   return info.found;
}

bool
ir_tree_grafting_visitor::do_graft(ir_rvalue **rvalue)
{
   if (!*rvalue)
      return false;

   ir_dereference_variable *deref = (*rvalue)->as_dereference_variable();

   if (!deref || deref->var != this->graft_var)
      return false;

   if (debug) {
      printf("GRAFTING:\n");
      this->graft_assign->print();
      printf("\n");
      printf("TO:\n");
      (*rvalue)->print();
      printf("\n");
   }

   this->graft_assign->remove();
   *rvalue = this->graft_assign->rhs;

   this->progress = true;
   return true;
}

ir_visitor_status
ir_tree_grafting_visitor::visit_enter(ir_loop *ir)
{
   (void)ir;
   /* Do not traverse into the body of the loop since that is a
    * different basic block.
    */
   return visit_stop;
}

ir_visitor_status
ir_tree_grafting_visitor::visit_leave(ir_assignment *ir)
{
   if (do_graft(&ir->rhs) ||
       do_graft(&ir->condition))
      return visit_stop;

   /* If this assignment updates a variable used in the assignment
    * we're trying to graft, then we're done.
    */
   if (dereferences_variable(this->graft_assign->rhs,
			     ir->lhs->variable_referenced())) {
      if (debug) {
	 printf("graft killed by: ");
	 ir->print();
	 printf("\n");
      }
      return visit_stop;
   }

   return visit_continue;
}

ir_visitor_status
ir_tree_grafting_visitor::visit_enter(ir_function *ir)
{
   (void) ir;
   return visit_continue_with_parent;
}

ir_visitor_status
ir_tree_grafting_visitor::visit_enter(ir_function_signature *ir)
{
   (void)ir;
   return visit_continue_with_parent;
}

ir_visitor_status
ir_tree_grafting_visitor::visit_enter(ir_call *ir)
{
   exec_list_iterator sig_iter = ir->get_callee()->parameters.iterator();
   /* Reminder: iterating ir_call iterates its parameters. */
   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_variable *sig_param = (ir_variable *)sig_iter.get();
      ir_rvalue *ir = (ir_rvalue *)iter.get();
      ir_rvalue *new_ir = ir;

      if (sig_param->mode != ir_var_in)
	 continue;

      if (do_graft(&new_ir)) {
	 ir->replace_with(new_ir);
	 return visit_stop;
      }
      sig_iter.next();
   }

   return visit_continue;
}

ir_visitor_status
ir_tree_grafting_visitor::visit_enter(ir_expression *ir)
{
   for (unsigned int i = 0; i < ir->get_num_operands(); i++) {
      if (do_graft(&ir->operands[i]))
	 return visit_stop;
   }

   return visit_continue;
}

ir_visitor_status
ir_tree_grafting_visitor::visit_enter(ir_if *ir)
{
   if (do_graft(&ir->condition))
      return visit_stop;

   /* Do not traverse into the body of the if-statement since that is a
    * different basic block.
    */
   return visit_continue_with_parent;
}

ir_visitor_status
ir_tree_grafting_visitor::visit_enter(ir_swizzle *ir)
{
   if (do_graft(&ir->val))
      return visit_stop;

   return visit_continue;
}

ir_visitor_status
ir_tree_grafting_visitor::visit_enter(ir_texture *ir)
{
   if (do_graft(&ir->coordinate) ||
       do_graft(&ir->projector) ||
       do_graft(&ir->shadow_comparitor))
	 return visit_stop;

   switch (ir->op) {
   case ir_tex:
      break;
   case ir_txb:
      if (do_graft(&ir->lod_info.bias))
	 return visit_stop;
      break;
   case ir_txf:
   case ir_txl:
      if (do_graft(&ir->lod_info.lod))
	 return visit_stop;
      break;
   case ir_txd:
      if (do_graft(&ir->lod_info.grad.dPdx) ||
	  do_graft(&ir->lod_info.grad.dPdy))
	 return visit_stop;
      break;
   }

   return visit_continue;
}

struct tree_grafting_info {
   ir_variable_refcount_visitor *refs;
   bool progress;
};

static bool
try_tree_grafting(ir_assignment *start,
		  ir_variable *lhs_var,
		  ir_instruction *bb_last)
{
   ir_tree_grafting_visitor v(start, lhs_var);

   if (debug) {
      printf("trying to graft: ");
      lhs_var->print();
      printf("\n");
   }

   for (ir_instruction *ir = (ir_instruction *)start->next;
	ir != bb_last->next;
	ir = (ir_instruction *)ir->next) {

      if (debug) {
	 printf("- ");
	 ir->print();
	 printf("\n");
      }

      ir_visitor_status s = ir->accept(&v);
      if (s == visit_stop)
	 return v.progress;
   }

   return false;
}

static void
tree_grafting_basic_block(ir_instruction *bb_first,
			  ir_instruction *bb_last,
			  void *data)
{
   struct tree_grafting_info *info = (struct tree_grafting_info *)data;
   ir_instruction *ir, *next;

   for (ir = bb_first, next = (ir_instruction *)ir->next;
	ir != bb_last->next;
	ir = next, next = (ir_instruction *)ir->next) {
      ir_assignment *assign = ir->as_assignment();

      if (!assign)
	 continue;

      ir_variable *lhs_var = assign->whole_variable_written();
      if (!lhs_var)
	 continue;

      if (lhs_var->mode == ir_var_out ||
	  lhs_var->mode == ir_var_inout)
	 continue;

      variable_entry *entry = info->refs->get_variable_entry(lhs_var);

      if (!entry->declaration ||
	  entry->assigned_count != 1 ||
	  entry->referenced_count != 2)
	 continue;

      assert(assign == entry->assign);

      /* Found a possibly graftable assignment.  Now, walk through the
       * rest of the BB seeing if the deref is here, and if nothing interfered with
       * pasting its expression's values in between.
       */
      info->progress |= try_tree_grafting(assign, lhs_var, bb_last);
   }
}

/**
 * Does a copy propagation pass on the code present in the instruction stream.
 */
bool
do_tree_grafting(exec_list *instructions)
{
   ir_variable_refcount_visitor refs;
   struct tree_grafting_info info;

   info.progress = false;
   info.refs = &refs;

   visit_list_elements(info.refs, instructions);

   call_for_basic_blocks(instructions, tree_grafting_basic_block, &info);

   return info.progress;
}
