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

#include "glsl_types.h"
#include "loop_analysis.h"
#include "ir_hierarchical_visitor.h"

class loop_unroll_visitor : public ir_hierarchical_visitor {
public:
   loop_unroll_visitor(loop_state *state, unsigned max_iterations)
   {
      this->state = state;
      this->progress = false;
      this->max_iterations = max_iterations;
   }

   virtual ir_visitor_status visit_leave(ir_loop *ir);

   loop_state *state;

   bool progress;
   unsigned max_iterations;
};


ir_visitor_status
loop_unroll_visitor::visit_leave(ir_loop *ir)
{
   loop_variable_state *const ls = this->state->get(ir);
   int iterations;

   /* If we've entered a loop that hasn't been analyzed, something really,
    * really bad has happened.
    */
   if (ls == NULL) {
      assert(ls != NULL);
      return visit_continue;
   }

   iterations = ls->max_iterations;

   /* Don't try to unroll loops where the number of iterations is not known
    * at compile-time.
    */
   if (iterations < 0)
      return visit_continue;

   /* Don't try to unroll loops that have zillions of iterations either.
    */
   if (iterations > max_iterations)
      return visit_continue;

   if (ls->num_loop_jumps > 1)
      return visit_continue;
   else if (ls->num_loop_jumps) {
      /* recognize loops in the form produced by ir_lower_jumps */
      ir_instruction *last_ir =
	 ((ir_instruction*)ir->body_instructions.get_tail());

      assert(last_ir != NULL);

      ir_if *last_if = last_ir->as_if();
      if (last_if) {
	 bool continue_from_then_branch;

	 /* Determine which if-statement branch, if any, ends with a break.
	  * The branch that did *not* have the break will get a temporary
	  * continue inserted in each iteration of the loop unroll.
	  *
	  * Note that since ls->num_loop_jumps is <= 1, it is impossible for
	  * both branches to end with a break.
	  */
	 ir_instruction *last =
	    (ir_instruction *) last_if->then_instructions.get_tail();

	 if (last && last->ir_type == ir_type_loop_jump
	     && ((ir_loop_jump*) last)->is_break()) {
	    continue_from_then_branch = false;
	 } else {
	    last = (ir_instruction *) last_if->then_instructions.get_tail();

	    if (last && last->ir_type == ir_type_loop_jump
		&& ((ir_loop_jump*) last)->is_break())
	       continue_from_then_branch = true;
	    else
	       /* Bail out if neither if-statement branch ends with a break.
		*/
	       return visit_continue;
	 }

	 /* Remove the break from the if-statement.
	  */
	 last->remove();

         void *const mem_ctx = talloc_parent(ir);
         ir_instruction *ir_to_replace = ir;

         for (int i = 0; i < iterations; i++) {
            exec_list copy_list;

            copy_list.make_empty();
            clone_ir_list(mem_ctx, &copy_list, &ir->body_instructions);

            last_if = ((ir_instruction*)copy_list.get_tail())->as_if();
            assert(last_if);

            ir_to_replace->insert_before(&copy_list);
            ir_to_replace->remove();

            /* placeholder that will be removed in the next iteration */
            ir_to_replace =
	       new(mem_ctx) ir_loop_jump(ir_loop_jump::jump_continue);

            exec_list *const list = (continue_from_then_branch)
	       ? &last_if->then_instructions : &last_if->else_instructions;

            list->push_tail(ir_to_replace);
         }

         ir_to_replace->remove();

         this->progress = true;
         return visit_continue;
      } else if (last_ir->ir_type == ir_type_loop_jump
		 && ((ir_loop_jump *)last_ir)->is_break()) {
	 /* If the only loop-jump is a break at the end of the loop, the loop
	  * will execute exactly once.  Remove the break, set the iteration
	  * count, and fall through to the normal unroller.
	  */
         last_ir->remove();
	 iterations = 1;

	 this->progress = true;
      } else
         return visit_continue;
   }

   void *const mem_ctx = talloc_parent(ir);

   for (int i = 0; i < iterations; i++) {
      exec_list copy_list;

      copy_list.make_empty();
      clone_ir_list(mem_ctx, &copy_list, &ir->body_instructions);

      ir->insert_before(&copy_list);
   }

   /* The loop has been replaced by the unrolled copies.  Remove the original
    * loop from the IR sequence.
    */
   ir->remove();

   this->progress = true;
   return visit_continue;
}


bool
unroll_loops(exec_list *instructions, loop_state *ls, unsigned max_iterations)
{
   loop_unroll_visitor v(ls, max_iterations);

   v.run(instructions);

   return v.progress;
}
