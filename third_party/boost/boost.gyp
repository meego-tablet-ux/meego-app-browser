# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'includes': [
    '../../build/common.gypi',
  ],
  'targets': [
    # This only has a target for windows.
  ],
  'conditions': [
    ['OS=="win"', {
      'targets': [
        {
          # Only VS2005 needs boost tuple.  VS2008 SP1, and gcc on mac + linux
          # include tr1.  However, since we cannot differentiate between VS2008
          # and VS2005 in gyp, we do our best by only enabling this target for
          # OS=="win".
          'target_name': 'boost_tuple',
          'type': 'none',
          'msvs_guid': 'FCC71A12-BC0A-49C6-9ED7-80331F8B4071',
          'sources': [
            "boost_1_36_0/boost/config.hpp",
            "boost_1_36_0/boost/config/compiler/visualc.hpp",
            "boost_1_36_0/boost/config/no_tr1/utility.hpp",
            "boost_1_36_0/boost/config/platform/win32.hpp",
            "boost_1_36_0/boost/config/select_compiler_config.hpp",
            "boost_1_36_0/boost/config/select_platform_config.hpp",
            "boost_1_36_0/boost/config/select_stdlib_config.hpp",
            "boost_1_36_0/boost/config/stdlib/dinkumware.hpp",
            "boost_1_36_0/boost/config/suffix.hpp",
            "boost_1_36_0/boost/config/user.hpp",
            "boost_1_36_0/boost/detail/workaround.hpp",
            "boost_1_36_0/boost/fusion/adapted/std_pair.hpp",
            "boost_1_36_0/boost/fusion/adapted/struct.hpp",
            "boost_1_36_0/boost/fusion/adapted/struct/adapt_assoc_struct.hpp",
            "boost_1_36_0/boost/fusion/adapted/struct/adapt_struct.hpp",
            "boost_1_36_0/boost/fusion/adapted/struct/detail/at_impl.hpp",
            "boost_1_36_0/boost/fusion/adapted/struct/detail/at_key_impl.hpp",
            "boost_1_36_0/boost/fusion/adapted/struct/detail/begin_impl.hpp",
            "boost_1_36_0/boost/fusion/adapted/struct/detail/category_of_impl.hpp",
            "boost_1_36_0/boost/fusion/adapted/struct/detail/end_impl.hpp",
            "boost_1_36_0/boost/fusion/adapted/struct/detail/has_key_impl.hpp",
            "boost_1_36_0/boost/fusion/adapted/struct/detail/is_sequence_impl.hpp",
            "boost_1_36_0/boost/fusion/adapted/struct/detail/is_view_impl.hpp",
            "boost_1_36_0/boost/fusion/adapted/struct/detail/size_impl.hpp",
            "boost_1_36_0/boost/fusion/adapted/struct/detail/value_at_impl.hpp",
            "boost_1_36_0/boost/fusion/adapted/struct/detail/value_at_key_impl.hpp",
            "boost_1_36_0/boost/fusion/adapted/struct/extension.hpp",
            "boost_1_36_0/boost/fusion/adapted/struct/struct_iterator.hpp",
            "boost_1_36_0/boost/fusion/container/generation/ignore.hpp",
            "boost_1_36_0/boost/fusion/container/vector/detail/advance_impl.hpp",
            "boost_1_36_0/boost/fusion/container/vector/detail/at_impl.hpp",
            "boost_1_36_0/boost/fusion/container/vector/detail/begin_impl.hpp",
            "boost_1_36_0/boost/fusion/container/vector/detail/deref_impl.hpp",
            "boost_1_36_0/boost/fusion/container/vector/detail/distance_impl.hpp",
            "boost_1_36_0/boost/fusion/container/vector/detail/end_impl.hpp",
            "boost_1_36_0/boost/fusion/container/vector/detail/equal_to_impl.hpp",
            "boost_1_36_0/boost/fusion/container/vector/detail/next_impl.hpp",
            "boost_1_36_0/boost/fusion/container/vector/detail/prior_impl.hpp",
            "boost_1_36_0/boost/fusion/container/vector/detail/value_at_impl.hpp",
            "boost_1_36_0/boost/fusion/container/vector/detail/value_of_impl.hpp",
            "boost_1_36_0/boost/fusion/container/vector/detail/vector_forward_ctor.hpp",
            "boost_1_36_0/boost/fusion/container/vector/detail/vector_n.hpp",
            "boost_1_36_0/boost/fusion/container/vector/detail/vector_n_chooser.hpp",
            "boost_1_36_0/boost/fusion/container/vector/limits.hpp",
            "boost_1_36_0/boost/fusion/container/vector/vector.hpp",
            "boost_1_36_0/boost/fusion/container/vector/vector10.hpp",
            "boost_1_36_0/boost/fusion/container/vector/vector_fwd.hpp",
            "boost_1_36_0/boost/fusion/container/vector/vector_iterator.hpp",
            "boost_1_36_0/boost/fusion/include/std_pair.hpp",
            "boost_1_36_0/boost/fusion/include/tuple.hpp",
            "boost_1_36_0/boost/fusion/iterator/deref.hpp",
            "boost_1_36_0/boost/fusion/iterator/detail/advance.hpp",
            "boost_1_36_0/boost/fusion/iterator/equal_to.hpp",
            "boost_1_36_0/boost/fusion/iterator/iterator_facade.hpp",
            "boost_1_36_0/boost/fusion/iterator/next.hpp",
            "boost_1_36_0/boost/fusion/iterator/prior.hpp",
            "boost_1_36_0/boost/fusion/sequence/comparison.hpp",
            "boost_1_36_0/boost/fusion/sequence/comparison/detail/enable_comparison.hpp",
            "boost_1_36_0/boost/fusion/sequence/comparison/detail/equal_to.hpp",
            "boost_1_36_0/boost/fusion/sequence/comparison/detail/less.hpp",
            "boost_1_36_0/boost/fusion/sequence/comparison/equal_to.hpp",
            "boost_1_36_0/boost/fusion/sequence/comparison/greater.hpp",
            "boost_1_36_0/boost/fusion/sequence/comparison/greater_equal.hpp",
            "boost_1_36_0/boost/fusion/sequence/comparison/less.hpp",
            "boost_1_36_0/boost/fusion/sequence/comparison/less_equal.hpp",
            "boost_1_36_0/boost/fusion/sequence/comparison/not_equal_to.hpp",
            "boost_1_36_0/boost/fusion/sequence/intrinsic/at.hpp",
            "boost_1_36_0/boost/fusion/sequence/intrinsic/begin.hpp",
            "boost_1_36_0/boost/fusion/sequence/intrinsic/end.hpp",
            "boost_1_36_0/boost/fusion/sequence/intrinsic/size.hpp",
            "boost_1_36_0/boost/fusion/sequence/intrinsic/value_at.hpp",
            "boost_1_36_0/boost/fusion/sequence/io.hpp",
            "boost_1_36_0/boost/fusion/sequence/io/detail/in.hpp",
            "boost_1_36_0/boost/fusion/sequence/io/detail/manip.hpp",
            "boost_1_36_0/boost/fusion/sequence/io/detail/out.hpp",
            "boost_1_36_0/boost/fusion/sequence/io/in.hpp",
            "boost_1_36_0/boost/fusion/sequence/io/out.hpp",
            "boost_1_36_0/boost/fusion/support/category_of.hpp",
            "boost_1_36_0/boost/fusion/support/detail/access.hpp",
            "boost_1_36_0/boost/fusion/support/detail/as_fusion_element.hpp",
            "boost_1_36_0/boost/fusion/support/detail/category_of.hpp",
            "boost_1_36_0/boost/fusion/support/detail/is_mpl_sequence.hpp",
            "boost_1_36_0/boost/fusion/support/is_iterator.hpp",
            "boost_1_36_0/boost/fusion/support/is_sequence.hpp",
            "boost_1_36_0/boost/fusion/support/iterator_base.hpp",
            "boost_1_36_0/boost/fusion/support/sequence_base.hpp",
            "boost_1_36_0/boost/fusion/support/tag_of.hpp",
            "boost_1_36_0/boost/fusion/support/tag_of_fwd.hpp",
            "boost_1_36_0/boost/fusion/tuple.hpp",
            "boost_1_36_0/boost/fusion/tuple/detail/tuple_forward_ctor.hpp",
            "boost_1_36_0/boost/fusion/tuple/make_tuple.hpp",
            "boost_1_36_0/boost/fusion/tuple/tuple.hpp",
            "boost_1_36_0/boost/fusion/tuple/tuple_fwd.hpp",
            "boost_1_36_0/boost/fusion/tuple/tuple_tie.hpp",
            "boost_1_36_0/boost/mpl/advance.hpp",
            "boost_1_36_0/boost/mpl/advance_fwd.hpp",
            "boost_1_36_0/boost/mpl/always.hpp",
            "boost_1_36_0/boost/mpl/and.hpp",
            "boost_1_36_0/boost/mpl/apply.hpp",
            "boost_1_36_0/boost/mpl/apply_fwd.hpp",
            "boost_1_36_0/boost/mpl/apply_wrap.hpp",
            "boost_1_36_0/boost/mpl/arg.hpp",
            "boost_1_36_0/boost/mpl/arg_fwd.hpp",
            "boost_1_36_0/boost/mpl/assert.hpp",
            "boost_1_36_0/boost/mpl/at.hpp",
            "boost_1_36_0/boost/mpl/at_fwd.hpp",
            "boost_1_36_0/boost/mpl/aux_/adl_barrier.hpp",
            "boost_1_36_0/boost/mpl/aux_/advance_backward.hpp",
            "boost_1_36_0/boost/mpl/aux_/advance_forward.hpp",
            "boost_1_36_0/boost/mpl/aux_/arg_typedef.hpp",
            "boost_1_36_0/boost/mpl/aux_/arithmetic_op.hpp",
            "boost_1_36_0/boost/mpl/aux_/arity.hpp",
            "boost_1_36_0/boost/mpl/aux_/arity_spec.hpp",
            "boost_1_36_0/boost/mpl/aux_/at_impl.hpp",
            "boost_1_36_0/boost/mpl/aux_/begin_end_impl.hpp",
            "boost_1_36_0/boost/mpl/aux_/common_name_wknd.hpp",
            "boost_1_36_0/boost/mpl/aux_/comparison_op.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/adl.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/arrays.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/bind.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/compiler.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/ctps.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/dtp.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/eti.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/forwarding.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/gcc.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/has_apply.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/has_xxx.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/integral.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/intel.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/lambda.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/msvc.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/msvc_typename.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/nttp.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/overload_resolution.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/pp_counter.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/preprocessor.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/static_constant.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/ttp.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/typeof.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/use_preprocessed.hpp",
            "boost_1_36_0/boost/mpl/aux_/config/workaround.hpp",
            "boost_1_36_0/boost/mpl/aux_/find_if_pred.hpp",
            "boost_1_36_0/boost/mpl/aux_/full_lambda.hpp",
            "boost_1_36_0/boost/mpl/aux_/has_apply.hpp",
            "boost_1_36_0/boost/mpl/aux_/has_begin.hpp",
            "boost_1_36_0/boost/mpl/aux_/has_size.hpp",
            "boost_1_36_0/boost/mpl/aux_/has_tag.hpp",
            "boost_1_36_0/boost/mpl/aux_/has_type.hpp",
            "boost_1_36_0/boost/mpl/aux_/include_preprocessed.hpp",
            "boost_1_36_0/boost/mpl/aux_/integral_wrapper.hpp",
            "boost_1_36_0/boost/mpl/aux_/is_msvc_eti_arg.hpp",
            "boost_1_36_0/boost/mpl/aux_/iter_apply.hpp",
            "boost_1_36_0/boost/mpl/aux_/iter_fold_if_impl.hpp",
            "boost_1_36_0/boost/mpl/aux_/iter_fold_impl.hpp",
            "boost_1_36_0/boost/mpl/aux_/lambda_arity_param.hpp",
            "boost_1_36_0/boost/mpl/aux_/lambda_spec.hpp",
            "boost_1_36_0/boost/mpl/aux_/lambda_support.hpp",
            "boost_1_36_0/boost/mpl/aux_/largest_int.hpp",
            "boost_1_36_0/boost/mpl/aux_/msvc_eti_base.hpp",
            "boost_1_36_0/boost/mpl/aux_/msvc_never_true.hpp",
            "boost_1_36_0/boost/mpl/aux_/msvc_type.hpp",
            "boost_1_36_0/boost/mpl/aux_/na.hpp",
            "boost_1_36_0/boost/mpl/aux_/na_assert.hpp",
            "boost_1_36_0/boost/mpl/aux_/na_fwd.hpp",
            "boost_1_36_0/boost/mpl/aux_/na_spec.hpp",
            "boost_1_36_0/boost/mpl/aux_/nested_type_wknd.hpp",
            "boost_1_36_0/boost/mpl/aux_/nttp_decl.hpp",
            "boost_1_36_0/boost/mpl/aux_/numeric_cast_utils.hpp",
            "boost_1_36_0/boost/mpl/aux_/numeric_op.hpp",
            "boost_1_36_0/boost/mpl/aux_/o1_size_impl.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessed/plain/advance_backward.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessed/plain/advance_forward.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessed/plain/and.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessed/plain/apply.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessed/plain/apply_fwd.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessed/plain/apply_wrap.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessed/plain/arg.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessed/plain/bind.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessed/plain/bind_fwd.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessed/plain/equal_to.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessed/plain/full_lambda.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessed/plain/iter_fold_if_impl.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessed/plain/iter_fold_impl.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessed/plain/less.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessed/plain/minus.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessed/plain/or.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessed/plain/placeholders.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessed/plain/plus.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessed/plain/quote.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessed/plain/template_arity.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessor/def_params_tail.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessor/enum.hpp",
            "boost_1_36_0/boost/mpl/aux_/preprocessor/params.hpp",
            "boost_1_36_0/boost/mpl/aux_/static_cast.hpp",
            "boost_1_36_0/boost/mpl/aux_/template_arity.hpp",
            "boost_1_36_0/boost/mpl/aux_/template_arity_fwd.hpp",
            "boost_1_36_0/boost/mpl/aux_/traits_lambda_spec.hpp",
            "boost_1_36_0/boost/mpl/aux_/type_wrapper.hpp",
            "boost_1_36_0/boost/mpl/aux_/value_wknd.hpp",
            "boost_1_36_0/boost/mpl/aux_/yes_no.hpp",
            "boost_1_36_0/boost/mpl/back_fwd.hpp",
            "boost_1_36_0/boost/mpl/begin_end.hpp",
            "boost_1_36_0/boost/mpl/begin_end_fwd.hpp",
            "boost_1_36_0/boost/mpl/bind.hpp",
            "boost_1_36_0/boost/mpl/bind_fwd.hpp",
            "boost_1_36_0/boost/mpl/bool.hpp",
            "boost_1_36_0/boost/mpl/bool_fwd.hpp",
            "boost_1_36_0/boost/mpl/clear_fwd.hpp",
            "boost_1_36_0/boost/mpl/deref.hpp",
            "boost_1_36_0/boost/mpl/distance.hpp",
            "boost_1_36_0/boost/mpl/distance_fwd.hpp",
            "boost_1_36_0/boost/mpl/empty_fwd.hpp",
            "boost_1_36_0/boost/mpl/equal_to.hpp",
            "boost_1_36_0/boost/mpl/eval_if.hpp",
            "boost_1_36_0/boost/mpl/find.hpp",
            "boost_1_36_0/boost/mpl/find_if.hpp",
            "boost_1_36_0/boost/mpl/front_fwd.hpp",
            "boost_1_36_0/boost/mpl/has_xxx.hpp",
            "boost_1_36_0/boost/mpl/identity.hpp",
            "boost_1_36_0/boost/mpl/if.hpp",
            "boost_1_36_0/boost/mpl/int.hpp",
            "boost_1_36_0/boost/mpl/int_fwd.hpp",
            "boost_1_36_0/boost/mpl/integral_c.hpp",
            "boost_1_36_0/boost/mpl/integral_c_fwd.hpp",
            "boost_1_36_0/boost/mpl/integral_c_tag.hpp",
            "boost_1_36_0/boost/mpl/is_sequence.hpp",
            "boost_1_36_0/boost/mpl/iter_fold.hpp",
            "boost_1_36_0/boost/mpl/iter_fold_if.hpp",
            "boost_1_36_0/boost/mpl/iterator_range.hpp",
            "boost_1_36_0/boost/mpl/iterator_tags.hpp",
            "boost_1_36_0/boost/mpl/lambda.hpp",
            "boost_1_36_0/boost/mpl/lambda_fwd.hpp",
            "boost_1_36_0/boost/mpl/less.hpp",
            "boost_1_36_0/boost/mpl/limits/arity.hpp",
            "boost_1_36_0/boost/mpl/logical.hpp",
            "boost_1_36_0/boost/mpl/long.hpp",
            "boost_1_36_0/boost/mpl/long_fwd.hpp",
            "boost_1_36_0/boost/mpl/minus.hpp",
            "boost_1_36_0/boost/mpl/negate.hpp",
            "boost_1_36_0/boost/mpl/next.hpp",
            "boost_1_36_0/boost/mpl/next_prior.hpp",
            "boost_1_36_0/boost/mpl/not.hpp",
            "boost_1_36_0/boost/mpl/numeric_cast.hpp",
            "boost_1_36_0/boost/mpl/o1_size.hpp",
            "boost_1_36_0/boost/mpl/o1_size_fwd.hpp",
            "boost_1_36_0/boost/mpl/or.hpp",
            "boost_1_36_0/boost/mpl/pair.hpp",
            "boost_1_36_0/boost/mpl/placeholders.hpp",
            "boost_1_36_0/boost/mpl/plus.hpp",
            "boost_1_36_0/boost/mpl/pop_back_fwd.hpp",
            "boost_1_36_0/boost/mpl/pop_front_fwd.hpp",
            "boost_1_36_0/boost/mpl/prior.hpp",
            "boost_1_36_0/boost/mpl/protect.hpp",
            "boost_1_36_0/boost/mpl/push_back_fwd.hpp",
            "boost_1_36_0/boost/mpl/push_front_fwd.hpp",
            "boost_1_36_0/boost/mpl/quote.hpp",
            "boost_1_36_0/boost/mpl/same_as.hpp",
            "boost_1_36_0/boost/mpl/sequence_tag.hpp",
            "boost_1_36_0/boost/mpl/sequence_tag_fwd.hpp",
            "boost_1_36_0/boost/mpl/size_fwd.hpp",
            "boost_1_36_0/boost/mpl/tag.hpp",
            "boost_1_36_0/boost/mpl/vector/aux_/at.hpp",
            "boost_1_36_0/boost/mpl/vector/aux_/back.hpp",
            "boost_1_36_0/boost/mpl/vector/aux_/begin_end.hpp",
            "boost_1_36_0/boost/mpl/vector/aux_/clear.hpp",
            "boost_1_36_0/boost/mpl/vector/aux_/empty.hpp",
            "boost_1_36_0/boost/mpl/vector/aux_/front.hpp",
            "boost_1_36_0/boost/mpl/vector/aux_/include_preprocessed.hpp",
            "boost_1_36_0/boost/mpl/vector/aux_/item.hpp",
            "boost_1_36_0/boost/mpl/vector/aux_/iterator.hpp",
            "boost_1_36_0/boost/mpl/vector/aux_/o1_size.hpp",
            "boost_1_36_0/boost/mpl/vector/aux_/pop_back.hpp",
            "boost_1_36_0/boost/mpl/vector/aux_/pop_front.hpp",
            "boost_1_36_0/boost/mpl/vector/aux_/preprocessed/plain/vector10.hpp",
            "boost_1_36_0/boost/mpl/vector/aux_/push_back.hpp",
            "boost_1_36_0/boost/mpl/vector/aux_/push_front.hpp",
            "boost_1_36_0/boost/mpl/vector/aux_/size.hpp",
            "boost_1_36_0/boost/mpl/vector/aux_/tag.hpp",
            "boost_1_36_0/boost/mpl/vector/aux_/vector0.hpp",
            "boost_1_36_0/boost/mpl/vector/vector0.hpp",
            "boost_1_36_0/boost/mpl/vector/vector10.hpp",
            "boost_1_36_0/boost/mpl/void.hpp",
            "boost_1_36_0/boost/mpl/void_fwd.hpp",
            "boost_1_36_0/boost/preprocessor/arithmetic/add.hpp",
            "boost_1_36_0/boost/preprocessor/arithmetic/dec.hpp",
            "boost_1_36_0/boost/preprocessor/arithmetic/inc.hpp",
            "boost_1_36_0/boost/preprocessor/arithmetic/sub.hpp",
            "boost_1_36_0/boost/preprocessor/array/data.hpp",
            "boost_1_36_0/boost/preprocessor/array/elem.hpp",
            "boost_1_36_0/boost/preprocessor/array/size.hpp",
            "boost_1_36_0/boost/preprocessor/cat.hpp",
            "boost_1_36_0/boost/preprocessor/comma_if.hpp",
            "boost_1_36_0/boost/preprocessor/config/config.hpp",
            "boost_1_36_0/boost/preprocessor/control/detail/msvc/while.hpp",
            "boost_1_36_0/boost/preprocessor/control/expr_iif.hpp",
            "boost_1_36_0/boost/preprocessor/control/if.hpp",
            "boost_1_36_0/boost/preprocessor/control/iif.hpp",
            "boost_1_36_0/boost/preprocessor/control/while.hpp",
            "boost_1_36_0/boost/preprocessor/debug/error.hpp",
            "boost_1_36_0/boost/preprocessor/dec.hpp",
            "boost_1_36_0/boost/preprocessor/detail/auto_rec.hpp",
            "boost_1_36_0/boost/preprocessor/detail/check.hpp",
            "boost_1_36_0/boost/preprocessor/detail/is_binary.hpp",
            "boost_1_36_0/boost/preprocessor/empty.hpp",
            "boost_1_36_0/boost/preprocessor/facilities/empty.hpp",
            "boost_1_36_0/boost/preprocessor/facilities/identity.hpp",
            "boost_1_36_0/boost/preprocessor/facilities/intercept.hpp",
            "boost_1_36_0/boost/preprocessor/identity.hpp",
            "boost_1_36_0/boost/preprocessor/inc.hpp",
            "boost_1_36_0/boost/preprocessor/iterate.hpp",
            "boost_1_36_0/boost/preprocessor/iteration/detail/bounds/lower1.hpp",
            "boost_1_36_0/boost/preprocessor/iteration/detail/bounds/upper1.hpp",
            "boost_1_36_0/boost/preprocessor/iteration/detail/iter/forward1.hpp",
            "boost_1_36_0/boost/preprocessor/iteration/iterate.hpp",
            "boost_1_36_0/boost/preprocessor/list/adt.hpp",
            "boost_1_36_0/boost/preprocessor/list/detail/fold_left.hpp",
            "boost_1_36_0/boost/preprocessor/list/detail/fold_right.hpp",
            "boost_1_36_0/boost/preprocessor/list/fold_left.hpp",
            "boost_1_36_0/boost/preprocessor/list/fold_right.hpp",
            "boost_1_36_0/boost/preprocessor/list/reverse.hpp",
            "boost_1_36_0/boost/preprocessor/logical/and.hpp",
            "boost_1_36_0/boost/preprocessor/logical/bitand.hpp",
            "boost_1_36_0/boost/preprocessor/logical/bool.hpp",
            "boost_1_36_0/boost/preprocessor/logical/compl.hpp",
            "boost_1_36_0/boost/preprocessor/punctuation/comma.hpp",
            "boost_1_36_0/boost/preprocessor/punctuation/comma_if.hpp",
            "boost_1_36_0/boost/preprocessor/repeat.hpp",
            "boost_1_36_0/boost/preprocessor/repetition/detail/msvc/for.hpp",
            "boost_1_36_0/boost/preprocessor/repetition/enum.hpp",
            "boost_1_36_0/boost/preprocessor/repetition/enum_binary_params.hpp",
            "boost_1_36_0/boost/preprocessor/repetition/enum_params.hpp",
            "boost_1_36_0/boost/preprocessor/repetition/enum_params_with_a_default.hpp",
            "boost_1_36_0/boost/preprocessor/repetition/enum_shifted.hpp",
            "boost_1_36_0/boost/preprocessor/repetition/for.hpp",
            "boost_1_36_0/boost/preprocessor/repetition/repeat.hpp",
            "boost_1_36_0/boost/preprocessor/repetition/repeat_from_to.hpp",
            "boost_1_36_0/boost/preprocessor/seq/elem.hpp",
            "boost_1_36_0/boost/preprocessor/seq/for_each_i.hpp",
            "boost_1_36_0/boost/preprocessor/seq/seq.hpp",
            "boost_1_36_0/boost/preprocessor/seq/size.hpp",
            "boost_1_36_0/boost/preprocessor/slot/detail/def.hpp",
            "boost_1_36_0/boost/preprocessor/slot/detail/shared.hpp",
            "boost_1_36_0/boost/preprocessor/slot/slot.hpp",
            "boost_1_36_0/boost/preprocessor/stringize.hpp",
            "boost_1_36_0/boost/preprocessor/tuple/eat.hpp",
            "boost_1_36_0/boost/preprocessor/tuple/elem.hpp",
            "boost_1_36_0/boost/preprocessor/tuple/rem.hpp",
            "boost_1_36_0/boost/ref.hpp",
            "boost_1_36_0/boost/static_assert.hpp",
            "boost_1_36_0/boost/tr1/detail/config.hpp",
            "boost_1_36_0/boost/tr1/detail/config_all.hpp",
            "boost_1_36_0/boost/tr1/tr1/tuple",
            "boost_1_36_0/boost/tr1/tuple.hpp",
            "boost_1_36_0/boost/type_traits/add_const.hpp",
            "boost_1_36_0/boost/type_traits/add_reference.hpp",
            "boost_1_36_0/boost/type_traits/broken_compiler_spec.hpp",
            "boost_1_36_0/boost/type_traits/config.hpp",
            "boost_1_36_0/boost/type_traits/detail/bool_trait_def.hpp",
            "boost_1_36_0/boost/type_traits/detail/bool_trait_undef.hpp",
            "boost_1_36_0/boost/type_traits/detail/cv_traits_impl.hpp",
            "boost_1_36_0/boost/type_traits/detail/ice_and.hpp",
            "boost_1_36_0/boost/type_traits/detail/ice_eq.hpp",
            "boost_1_36_0/boost/type_traits/detail/ice_not.hpp",
            "boost_1_36_0/boost/type_traits/detail/ice_or.hpp",
            "boost_1_36_0/boost/type_traits/detail/template_arity_spec.hpp",
            "boost_1_36_0/boost/type_traits/detail/type_trait_def.hpp",
            "boost_1_36_0/boost/type_traits/detail/type_trait_undef.hpp",
            "boost_1_36_0/boost/type_traits/detail/yes_no_type.hpp",
            "boost_1_36_0/boost/type_traits/ice.hpp",
            "boost_1_36_0/boost/type_traits/integral_constant.hpp",
            "boost_1_36_0/boost/type_traits/intrinsics.hpp",
            "boost_1_36_0/boost/type_traits/is_abstract.hpp",
            "boost_1_36_0/boost/type_traits/is_arithmetic.hpp",
            "boost_1_36_0/boost/type_traits/is_array.hpp",
            "boost_1_36_0/boost/type_traits/is_base_and_derived.hpp",
            "boost_1_36_0/boost/type_traits/is_base_of.hpp",
            "boost_1_36_0/boost/type_traits/is_const.hpp",
            "boost_1_36_0/boost/type_traits/is_convertible.hpp",
            "boost_1_36_0/boost/type_traits/is_float.hpp",
            "boost_1_36_0/boost/type_traits/is_integral.hpp",
            "boost_1_36_0/boost/type_traits/is_reference.hpp",
            "boost_1_36_0/boost/type_traits/is_same.hpp",
            "boost_1_36_0/boost/type_traits/is_void.hpp",
            "boost_1_36_0/boost/type_traits/is_volatile.hpp",
            "boost_1_36_0/boost/type_traits/remove_const.hpp",
            "boost_1_36_0/boost/type_traits/remove_cv.hpp",
            "boost_1_36_0/boost/utility/addressof.hpp",
            "boost_1_36_0/boost/utility/enable_if.hpp",
          ],
          'direct_dependent_settings': {
            'include_dirs': [
# TODO(ajwong): Enable with the gmock checkin.
#              'boost_1_36_0/boost/tr1/tr1',
#              'boost_1_36_0',
            ],
          },
        }
      ],
    }],
  ],
}
