
// NO INCLUDE GUARDS, THE HEADER IS INTENDED FOR MULTIPLE INCLUSION

// Copyright Aleksey Gurtovoy 2000-2006
//
// Distributed under the Boost Software License, Version 1.0. 
// (See accompanying file LICENSE_1_0.txt or copy at 
// http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/mpl for documentation.

// $Source$
// $Date: 2006-11-08 19:46:57 -0500 (Wed, 08 Nov 2006) $
// $Revision: 35931 $

#include <boost/mpl/aux_/config/compiler.hpp>
#include <boost/mpl/aux_/config/preprocessor.hpp>
#include <boost/mpl/aux_/config/workaround.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>

#if !defined(BOOST_NEEDS_TOKEN_PASTING_OP_FOR_TOKENS_JUXTAPOSING)
#   define AUX778076_PREPROCESSED_HEADER \
    BOOST_MPL_CFG_COMPILER_DIR/BOOST_MPL_PREPROCESSED_HEADER \
/**/
#else
#   define AUX778076_PREPROCESSED_HEADER \
    BOOST_PP_CAT(BOOST_MPL_CFG_COMPILER_DIR,/)##BOOST_MPL_PREPROCESSED_HEADER \
/**/
#endif

#if BOOST_WORKAROUND(__IBMCPP__, BOOST_TESTED_AT(700))
#   define AUX778076_INCLUDE_STRING BOOST_PP_STRINGIZE(boost/mpl/aux_/preprocessed/AUX778076_PREPROCESSED_HEADER)
#   include AUX778076_INCLUDE_STRING
#   undef AUX778076_INCLUDE_STRING
#else
#   include BOOST_PP_STRINGIZE(boost/mpl/aux_/preprocessed/AUX778076_PREPROCESSED_HEADER)
#endif

#   undef AUX778076_PREPROCESSED_HEADER

#undef BOOST_MPL_PREPROCESSED_HEADER
