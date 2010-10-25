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
  * \file ir_dead_functions.cpp
  *
  * Eliminates unused functions from the linked program.
  */

 #include "ir.h"
 #include "ir_visitor.h"
 #include "ir_expression_flattening.h"
 #include "glsl_types.h"
 #include "safe_strcmp.h"

 class signature_entry : public exec_node
 {
 public:
    signature_entry(ir_function_signature *sig)
    {
       this->signature = sig;
       this->used = false;
    }

    ir_function_signature *signature;
    bool used;
 };

 class ir_dead_functions_visitor : public ir_hierarchical_visitor {
 public:
    ir_dead_functions_visitor()
    {
       this->mem_ctx = talloc_new(NULL);
    }

    ~ir_dead_functions_visitor()
    {
       talloc_free(this->mem_ctx);
    }

    virtual ir_visitor_status visit_enter(ir_function_signature *);
    virtual ir_visitor_status visit_enter(ir_call *);

    signature_entry *get_signature_entry(ir_function_signature *var);

    bool (*predicate)(ir_instruction *ir);

    /* List of signature_entry */
    exec_list signature_list;
    void *mem_ctx;
 };


 signature_entry *
 ir_dead_functions_visitor::get_signature_entry(ir_function_signature *sig)
 {
    foreach_iter(exec_list_iterator, iter, this->signature_list) {
       signature_entry *entry = (signature_entry *)iter.get();
       if (entry->signature == sig)
	  return entry;
    }

    signature_entry *entry = new(mem_ctx) signature_entry(sig);
    this->signature_list.push_tail(entry);
    return entry;
 }


 ir_visitor_status
 ir_dead_functions_visitor::visit_enter(ir_function_signature *ir)
 {
    signature_entry *entry = this->get_signature_entry(ir);

    if (safe_strcmp(ir->function_name(), "main") == 0) {
       entry->used = true;
    }

    return visit_continue;
 }


 ir_visitor_status
 ir_dead_functions_visitor::visit_enter(ir_call *ir)
 {
    signature_entry *entry = this->get_signature_entry(ir->get_callee());

    entry->used = true;

   return visit_continue;
}

bool
do_dead_functions(exec_list *instructions)
{
   ir_dead_functions_visitor v;
   bool progress = false;

   visit_list_elements(&v, instructions);

   /* Now that we've figured out which function signatures are used, remove
    * the unused ones, and remove function definitions that have no more
    * signatures.
    */
    foreach_iter(exec_list_iterator, iter, v.signature_list) {
      signature_entry *entry = (signature_entry *)iter.get();

      if (!entry->used) {
	 entry->signature->remove();
	 progress = true;
      }
      delete(entry);
   }

   /* We don't just do this above when we nuked a signature because of
    * const pointers.
    */
   foreach_iter(exec_list_iterator, iter, *instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();
      ir_function *func = ir->as_function();

      if (func && func->signatures.is_empty()) {
	 /* At this point (post-linking), the symbol table is no
	  * longer in use, so not removing the function from the
	  * symbol table should be OK.
	  */
	 func->remove();
	 progress = true;
      }
   }

   return progress;
}
