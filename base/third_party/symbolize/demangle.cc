// Copyright (c) 2006, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Satoru Takabayashi

#include <stdio.h>  // for NULL
#include "demangle.h"

_START_GOOGLE_NAMESPACE_

typedef struct {
  const char *abbrev;
  const char *real_name;
} AbbrevPair;

// List of operators from Itanium C++ ABI.
static const AbbrevPair kOperatorList[] = {
  { "nw", "new" },
  { "na", "new[]" },
  { "dl", "delete" },
  { "da", "delete[]" },
  { "ps", "+" },
  { "ng", "-" },
  { "ad", "&" },
  { "de", "*" },
  { "co", "~" },
  { "pl", "+" },
  { "mi", "-" },
  { "ml", "*" },
  { "dv", "/" },
  { "rm", "%" },
  { "an", "&" },
  { "or", "|" },
  { "eo", "^" },
  { "aS", "=" },
  { "pL", "+=" },
  { "mI", "-=" },
  { "mL", "*=" },
  { "dV", "/=" },
  { "rM", "%=" },
  { "aN", "&=" },
  { "oR", "|=" },
  { "eO", "^=" },
  { "ls", "<<" },
  { "rs", ">>" },
  { "lS", "<<=" },
  { "rS", ">>=" },
  { "eq", "==" },
  { "ne", "!=" },
  { "lt", "<" },
  { "gt", ">" },
  { "le", "<=" },
  { "ge", ">=" },
  { "nt", "!" },
  { "aa", "&&" },
  { "oo", "||" },
  { "pp", "++" },
  { "mm", "--" },
  { "cm", "," },
  { "pm", "->*" },
  { "pt", "->" },
  { "cl", "()" },
  { "ix", "[]" },
  { "qu", "?" },
  { "st", "sizeof" },
  { "sz", "sizeof" },
  { NULL, NULL },
};

// List of builtin types from Itanium C++ ABI.
static const AbbrevPair kBuiltinTypeList[] = {
  { "v", "void" },
  { "w", "wchar_t" },
  { "b", "bool" },
  { "c", "char" },
  { "a", "signed char" },
  { "h", "unsigned char" },
  { "s", "short" },
  { "t", "unsigned short" },
  { "i", "int" },
  { "j", "unsigned int" },
  { "l", "long" },
  { "m", "unsigned long" },
  { "x", "long long" },
  { "y", "unsigned long long" },
  { "n", "__int128" },
  { "o", "unsigned __int128" },
  { "f", "float" },
  { "d", "double" },
  { "e", "long double" },
  { "g", "__float128" },
  { "z", "ellipsis" },
  { NULL, NULL }
};

// List of substitutions Itanium C++ ABI.
static const AbbrevPair kSubstitutionList[] = {
  { "St", "" },
  { "Sa", "allocator" },
  { "Sb", "basic_string" },
  // std::basic_string<char, std::char_traits<char>,std::allocator<char> >
  { "Ss", "string"},
  // std::basic_istream<char, std::char_traits<char> >
  { "Si", "istream" },
  // std::basic_ostream<char, std::char_traits<char> >
  { "So", "ostream" },
  // std::basic_iostream<char, std::char_traits<char> >
  { "Sd", "iostream" },
  { NULL, NULL }
};

// State needed for demangling.
typedef struct {
  const char *mangled_cur;  // Cursor of mangled name.
  const char *mangled_end;  // End of mangled name.
  char *out_cur;            // Cursor of output string.
  const char *out_begin;    // Beginning of output string.
  const char *out_end;      // End of output string.
  const char *prev_name;    // For constructors/destructors.
  int prev_name_length;     // For constructors/destructors.
  int nest_level;           // For nested names.
  int number;               // Remember the previous number.
  bool append;              // Append flag.
  bool overflowed;          // True if output gets overflowed.
} State;

// We don't use strlen() in libc since it's not guaranteed to be async
// signal safe.
static size_t StrLen(const char *str) {
  size_t len = 0;
  while (*str != '\0') {
    ++str;
    ++len;
  }
  return len;
}

// Returns true if "str" has "prefix" as a prefix.
static bool StrPrefix(const char *str, const char *prefix) {
  size_t i = 0;
  while (str[i] != '\0' && prefix[i] != '\0' &&
         str[i] == prefix[i]) {
    ++i;
  }
  return prefix[i] == '\0';  // Consumed everything in "prefix".
}

static void InitState(State *state, const char *mangled,
                      char *out, int out_size) {
  state->mangled_cur = mangled;
  state->mangled_end = mangled + StrLen(mangled);
  state->out_cur = out;
  state->out_begin = out;
  state->out_end = out + out_size;
  state->prev_name  = NULL;
  state->prev_name_length = -1;
  state->nest_level = -1;
  state->number = -1;
  state->append = true;
  state->overflowed = false;
}

// Calculates the remaining length of the mangled name.
static int RemainingLength(State *state) {
  return state->mangled_end - state->mangled_cur;
}

// Returns true and advances "mangled_cur" if we find "c" at
// "mangled_cur" position.
static bool ParseChar(State *state, const char c) {
  if (RemainingLength(state) >= 1 && *state->mangled_cur == c) {
    ++state->mangled_cur;
    return true;
  }
  return false;
}

// Returns true and advances "mangled_cur" if we find "two_chars" at
// "mangled_cur" position.
static bool ParseTwoChar(State *state, const char *two_chars) {
  if (RemainingLength(state) >= 2 &&
      state->mangled_cur[0] == two_chars[0] &&
      state->mangled_cur[1] == two_chars[1]) {
    state->mangled_cur += 2;
    return true;
  }
  return false;
}

// Returns true and advances "mangled_cur" if we find any character in
// "char_class" at "mangled_cur" position.
static bool ParseCharClass(State *state, const char *char_class) {
  if (state->mangled_cur == state->mangled_end) {
    return false;
  }
  const char *p = char_class;
  for (; *p != '\0'; ++p) {
    if (*state->mangled_cur == *p) {
      state->mangled_cur += 1;
      return true;
    }
  }
  return false;
}

// This function is used for handling an optional non-terminal.
static bool Optional(bool status) {
  return true;
}

// This function is used for handling <non-terminal>+ syntax.
typedef bool (*ParseFunc)(State *);
static bool OneOrMore(ParseFunc parse_func, State *state) {
  if (parse_func(state)) {
    while (parse_func(state)) {
    }
    return true;
  }
  return false;
}

// Append "str" at "out_cur".  If there is an overflow, "overflowed"
// is set to true for later use.  The output string is ensured to
// always terminate with '\0' as long as there is no overflow.
static void Append(State *state, const char * const str, const int length) {
  int i;
  for (i = 0; i < length; ++i) {
    if (state->out_cur + 1 < state->out_end) {  // +1 for '\0'
      *state->out_cur = str[i];
      ++state->out_cur;
    } else {
      state->overflowed = true;
      break;
    }
  }
  if (!state->overflowed) {
    *state->out_cur = '\0';  // Terminate it with '\0'
  }
}

// We don't use equivalents in libc to avoid locale issues.
static bool IsLower(char c) {
  return c >= 'a' && c <= 'z';
}

static bool IsAlpha(char c) {
  return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

// Append "str" with some tweaks, iff "append" state is true.
// Returns true so that it can be placed in "if" conditions.
static void MaybeAppendWithLength(State *state, const char * const str,
                                  const int length) {
  if (state->append && length > 0) {
    // Append a space if the output buffer ends with '<' and "str"
    // starts with '<' to avoid <<<.
    if (str[0] == '<' && state->out_begin < state->out_cur  &&
        state->out_cur[-1] == '<') {
      Append(state, " ", 1);
    }
    // Remember the last identifier name for ctors/dtors.
    if (IsAlpha(str[0]) || str[0] == '_') {
      state->prev_name = state->out_cur;
      state->prev_name_length = length;
    }
    Append(state, str, length);
  }
}

// A convenient wrapper arount MaybeAppendWithLength().
static bool MaybeAppend(State *state, const char * const str) {
  if (state->append) {
    int length = StrLen(str);
    MaybeAppendWithLength(state, str, length);
  }
  return true;
}

// This function is used for handling nested names.
static bool EnterNestedName(State *state) {
  state->nest_level = 0;
  return true;
}

// This function is used for handling nested names.
static bool LeaveNestedName(State *state, int prev_value) {
  state->nest_level = prev_value;
  return true;
}

// Disable the append mode not to print function parameters, etc.
static bool DisableAppend(State *state) {
  state->append = false;
  return true;
}

// Restore the append mode to the previous state.
static bool RestoreAppend(State *state, bool prev_value) {
  state->append = prev_value;
  return true;
}

// Increase the nest level for nested names.
static void MaybeIncreaseNestLevel(State *state) {
  if (state->nest_level > -1) {
    ++state->nest_level;
  }
}

// Appends :: for nested names if necessary.
static void MaybeAppendSeparator(State *state) {
  if (state->nest_level >= 1) {
    MaybeAppend(state, "::");
  }
}

// Cancel the last separator if necessary.
static void MaybeCancelLastSeparator(State *state) {
  if (state->nest_level >= 1 && state->append &&
      state->out_begin <= state->out_cur - 2) {
    state->out_cur -= 2;
    *state->out_cur = '\0';
  }
}

// Returns true if identifier pointed by "mangled_cur" is anonymous
// namespace.
static bool IdentifierIsAnonymousNamespace(State *state) {
  const char anon_prefix[] = "_GLOBAL__N_";
  return (state->number > sizeof(anon_prefix) - 1 &&  // Should be longer.
          StrPrefix(state->mangled_cur, anon_prefix));
}

// Forward declarations of our parsing functions.
static bool ParseMangledName(State *state);
static bool ParseEncoding(State *state);
static bool ParseName(State *state);
static bool ParseUnscopedName(State *state);
static bool ParseUnscopedTemplateName(State *state);
static bool ParseNestedName(State *state);
static bool ParsePrefix(State *state);
static bool ParseUnqualifiedName(State *state);
static bool ParseSourceName(State *state);
static bool ParseLocalSourceName(State *state);
static bool ParseNumber(State *state);
static bool ParseFloatNumber(State *state);
static bool ParseSeqId(State *state);
static bool ParseIdentifier(State *state);
static bool ParseOperatorName(State *state);
static bool ParseSpecialName(State *state);
static bool ParseCallOffset(State *state);
static bool ParseNVOffset(State *state);
static bool ParseVOffset(State *state);
static bool ParseCtorDtorName(State *state);
static bool ParseType(State *state);
static bool ParseCVQualifiers(State *state);
static bool ParseBuiltinType(State *state);
static bool ParseFunctionType(State *state);
static bool ParseBareFunctionType(State *state);
static bool ParseClassEnumType(State *state);
static bool ParseArrayType(State *state);
static bool ParsePointerToMemberType(State *state);
static bool ParseTemplateParam(State *state);
static bool ParseTemplateTemplateParam(State *state);
static bool ParseTemplateArgs(State *state);
static bool ParseTemplateArg(State *state);
static bool ParseExpression(State *state);
static bool ParseExprPrimary(State *state);
static bool ParseLocalName(State *state);
static bool ParseDiscriminator(State *state);
static bool ParseSubstitution(State *state);

// Implementation note: the following code is a straightforward
// translation of the Itanium C++ ABI defined in BNF with a couple of
// exceptions.
//
// - Support GNU extensions not defined in the Itanium C++ ABI
// - <prefix> and <template-prefix> are combined to avoid infinite loop
// - Reorder patterns to shorten the code
// - Reorder patterns to give greedier functions precedence
//   We'll mark "Less greedy than" for these cases in the code
//
// Each parsing function changes the state and returns true on
// success.  Otherwise, don't change the state and returns false.  To
// ensure that the state isn't changed in the latter case, we save the
// original state before we call more than one parsing functions
// consecutively with &&, and restore the state if unsuccessful.  See
// ParseEncoding() as an example of this convention.  We follow the
// convention throughout the code.
//
// Originally we tried to do demangling without following the full ABI
// syntax but it turned out we needed to follow the full syntax to
// parse complicated cases like nested template arguments.  Note that
// implementing a full-fledged demangler isn't trivial (libiberty's
// cp-demangle.c has +4300 lines).
//
// Note that (foo) in <(foo) ...> is a modifier to be ignored.
//
// Reference:
// - Itanium C++ ABI
//   <http://www.codesourcery.com/cxx-abi/abi.html#mangling>

// <mangled-name> ::= _Z <encoding>
static bool ParseMangledName(State *state) {
  if (ParseTwoChar(state, "_Z") && ParseEncoding(state)) {
    // Append trailing version suffix if any.
    // ex. _Z3foo@@GLIBCXX_3.4
    if (state->mangled_cur < state->mangled_end &&
        state->mangled_cur[0] == '@') {
      MaybeAppend(state, state->mangled_cur);
      state->mangled_cur = state->mangled_end;
    }
    return true;
  }
  return false;
}

// <encoding> ::= <(function) name> <bare-function-type>
//            ::= <(data) name>
//            ::= <special-name>
static bool ParseEncoding(State *state) {
  State copy = *state;
  if (ParseName(state) && ParseBareFunctionType(state)) {
    return true;
  }
  *state = copy;

  if (ParseName(state) || ParseSpecialName(state)) {
    return true;
  }
  return false;
}

// <name> ::= <nested-name>
//        ::= <unscoped-template-name> <template-args>
//        ::= <unscoped-name>
//        ::= <local-name>
static bool ParseName(State *state) {
  if (ParseNestedName(state) || ParseLocalName(state)) {
    return true;
  }

  State copy = *state;
  if (ParseUnscopedTemplateName(state) &&
      ParseTemplateArgs(state)) {
    return true;
  }
  *state = copy;

  // Less greedy than <unscoped-template-name> <template-args>.
  if (ParseUnscopedName(state)) {
    return true;
  }
  return false;
}

// <unscoped-name> ::= <unqualified-name>
//                 ::= St <unqualified-name>
static bool ParseUnscopedName(State *state) {
  if (ParseUnqualifiedName(state)) {
    return true;
  }

  State copy = *state;
  if (ParseTwoChar(state, "St") &&
      MaybeAppend(state, "std::") &&
      ParseUnqualifiedName(state)) {
    return true;
  }
  *state = copy;
  return false;
}

// <unscoped-template-name> ::= <unscoped-name>
//                          ::= <substitution>
static bool ParseUnscopedTemplateName(State *state) {
  return ParseUnscopedName(state) || ParseSubstitution(state);
}

// <nested-name> ::= N [<CV-qualifiers>] <prefix> <unqualified-name> E
//               ::= N [<CV-qualifiers>] <template-prefix> <template-args> E
static bool ParseNestedName(State *state) {
  State copy = *state;
  if (ParseChar(state, 'N') &&
      EnterNestedName(state) &&
      Optional(ParseCVQualifiers(state)) &&
      ParsePrefix(state) &&
      LeaveNestedName(state, copy.nest_level) &&
      ParseChar(state, 'E')) {
    return true;
  }
  *state = copy;
  return false;
}

// This part is tricky.  If we literally translate them to code, we'll
// end up infinite loop.  Hence we merge them to avoid the case.
//
// <prefix> ::= <prefix> <unqualified-name>
//          ::= <template-prefix> <template-args>
//          ::= <template-param>
//          ::= <substitution>
//          ::= # empty
// <template-prefix> ::= <prefix> <(template) unqualified-name>
//                   ::= <template-param>
//                   ::= <substitution>
static bool ParsePrefix(State *state) {
  bool has_something = false;
  while (true) {
    MaybeAppendSeparator(state);
    if (ParseTemplateParam(state) ||
        ParseSubstitution(state) ||
        ParseUnscopedName(state)) {
      has_something = true;
      MaybeIncreaseNestLevel(state);
      continue;
    }
    MaybeCancelLastSeparator(state);
    if (has_something && ParseTemplateArgs(state)) {
      return ParsePrefix(state);
    } else {
      break;
    }
  }
  return true;
}

// <unqualified-name> ::= <operator-name>
//                    ::= <ctor-dtor-name>
//                    ::= <source-name>
//                    ::= <local-source-name>
static bool ParseUnqualifiedName(State *state) {
  return (ParseOperatorName(state) ||
          ParseCtorDtorName(state) ||
          ParseSourceName(state) ||
          ParseLocalSourceName(state));
}

// <source-name> ::= <positive length number> <identifier>
static bool ParseSourceName(State *state) {
  State copy = *state;
  if (ParseNumber(state) && ParseIdentifier(state)) {
    return true;
  }
  *state = copy;
  return false;
}

// <local-source-name> ::= L <source-name> [<discriminator>]
//
// References:
//   http://gcc.gnu.org/bugzilla/show_bug.cgi?id=31775
//   http://gcc.gnu.org/viewcvs?view=rev&revision=124467
static bool ParseLocalSourceName(State *state) {
  State copy = *state;
  if (ParseChar(state, 'L') && ParseSourceName(state) &&
      Optional(ParseDiscriminator(state))) {
    return true;
  }
  *state = copy;
  return false;
}

// <number> ::= [n] <non-negative decimal integer>
static bool ParseNumber(State *state) {
  int sign = 1;
  if (ParseChar(state, 'n')) {
    sign = -1;
  }
  const char *p = state->mangled_cur;
  int number = 0;
  for (;p < state->mangled_end; ++p) {
    if ((*p >= '0' && *p <= '9')) {
      number = number * 10 + (*p - '0');
    } else {
      break;
    }
  }
  if (p != state->mangled_cur) {  // Conversion succeeded.
    state->mangled_cur = p;
    state->number = number * sign;
    return true;
  }
  return false;
}

// Floating-point literals are encoded using a fixed-length lowercase
// hexadecimal string.
static bool ParseFloatNumber(State *state) {
  const char *p = state->mangled_cur;
  int number = 0;
  for (;p < state->mangled_end; ++p) {
    if ((*p >= '0' && *p <= '9')) {
      number = number * 16 + (*p - '0');
    } else if (*p >= 'a' && *p <= 'f') {
      number = number * 16 + (*p - 'a' + 10);
    } else {
      break;
    }
  }
  if (p != state->mangled_cur) {  // Conversion succeeded.
    state->mangled_cur = p;
    state->number = number;
    return true;
  }
  return false;
}

// The <seq-id> is a sequence number in base 36,
// using digits and upper case letters
static bool ParseSeqId(State *state) {
  const char *p = state->mangled_cur;
  int number = 0;
  for (;p < state->mangled_end; ++p) {
    if ((*p >= '0' && *p <= '9')) {
      number = number * 36 + (*p - '0');
    } else if (*p >= 'A' && *p <= 'Z') {
      number = number * 36 + (*p - 'A' + 10);
    } else {
      break;
    }
  }
  if (p != state->mangled_cur) {  // Conversion succeeded.
    state->mangled_cur = p;
    state->number = number;
    return true;
  }
  return false;
}

// <identifier> ::= <unqualified source code identifier>
static bool ParseIdentifier(State *state) {
  if (state->number == -1 ||
      RemainingLength(state) < state->number) {
    return false;
  }
  if (IdentifierIsAnonymousNamespace(state)) {
    MaybeAppend(state, "(anonymous namespace)");
  } else {
    MaybeAppendWithLength(state, state->mangled_cur, state->number);
  }
  state->mangled_cur += state->number;
  state->number = -1;  // Reset the number.
  return true;
}

// <operator-name> ::= nw, and other two letters cases
//                 ::= cv <type>  # (cast)
//                 ::= v  <digit> <source-name> # vendor extended operator
static bool ParseOperatorName(State *state) {
  if (RemainingLength(state) < 2) {
    return false;
  }
  // First check with "cv" (cast) case.
  State copy = *state;
  if (ParseTwoChar(state, "cv") &&
      MaybeAppend(state, "operator ") &&
      EnterNestedName(state) &&
      ParseType(state) &&
      LeaveNestedName(state, copy.nest_level)) {
    return true;
  }
  *state = copy;

  // Then vendor extended operators.
  if (ParseChar(state, 'v') && ParseCharClass(state, "0123456789") &&
      ParseSourceName(state)) {
    return true;
  }
  *state = copy;

  // Other operator names should start with a lower alphabet followed
  // by a lower/upper alphabet.
  if (!(IsLower(state->mangled_cur[0]) &&
        IsAlpha(state->mangled_cur[1]))) {
    return false;
  }
  // We may want to perform a binary search if we really need speed.
  const AbbrevPair *p;
  for (p = kOperatorList; p->abbrev != NULL; ++p) {
    if (state->mangled_cur[0] == p->abbrev[0] &&
        state->mangled_cur[1] == p->abbrev[1]) {
      MaybeAppend(state, "operator");
      if (IsLower(*p->real_name)) {  // new, delete, etc.
        MaybeAppend(state, " ");
      }
      MaybeAppend(state, p->real_name);
      state->mangled_cur += 2;
      return true;
    }
  }
  return false;
}

// <special-name> ::= TV <type>
//                ::= TT <type>
//                ::= TI <type>
//                ::= TS <type>
//                ::= Tc <call-offset> <call-offset> <(base) encoding>
//                ::= GV <(object) name>
//                ::= T <call-offset> <(base) encoding>
// G++ extensions:
//                ::= TC <type> <(offset) number> _ <(base) type>
//                ::= TF <type>
//                ::= TJ <type>
//                ::= GR <name>
//                ::= GA <encoding>
//                ::= Th <call-offset> <(base) encoding>
//                ::= Tv <call-offset> <(base) encoding>
//
// Note: we don't care much about them since they don't appear in
// stack traces.  The are special data.
static bool ParseSpecialName(State *state) {
  State copy = *state;
  if (ParseChar(state, 'T') &&
      ParseCharClass(state, "VTIS") &&
      ParseType(state)) {
    return true;
  }
  *state = copy;

  if (ParseTwoChar(state, "Tc") && ParseCallOffset(state) &&
      ParseCallOffset(state) && ParseEncoding(state)) {
    return true;
  }
  *state = copy;

  if (ParseTwoChar(state, "GV") &&
      ParseName(state)) {
    return true;
  }
  *state = copy;

  if (ParseChar(state, 'T') && ParseCallOffset(state) &&
      ParseEncoding(state)) {
    return true;
  }
  *state = copy;

  // G++ extensions
  if (ParseTwoChar(state, "TC") && ParseType(state) &&
      ParseNumber(state) && ParseChar(state, '_') &&
      DisableAppend(state) &&
      ParseType(state)) {
    RestoreAppend(state, copy.append);
    return true;
  }
  *state = copy;

  if (ParseChar(state, 'T') && ParseCharClass(state, "FJ") &&
      ParseType(state)) {
    return true;
  }
  *state = copy;

  if (ParseTwoChar(state, "GR") && ParseName(state)) {
    return true;
  }
  *state = copy;

  if (ParseTwoChar(state, "GA") && ParseEncoding(state)) {
    return true;
  }
  *state = copy;

  if (ParseChar(state, 'T') && ParseCharClass(state, "hv") &&
      ParseCallOffset(state) && ParseEncoding(state)) {
    return true;
  }
  *state = copy;
  return false;
}

// <call-offset> ::= h <nv-offset> _
//               ::= v <v-offset> _
static bool ParseCallOffset(State *state) {
  State copy = *state;
  if (ParseChar(state, 'h') &&
      ParseNVOffset(state) && ParseChar(state, '_')) {
    return true;
  }
  *state = copy;

  if (ParseChar(state, 'v') &&
      ParseVOffset(state) && ParseChar(state, '_')) {
    return true;
  }
  *state = copy;

  return false;
}

// <nv-offset> ::= <(offset) number>
static bool ParseNVOffset(State *state) {
  return ParseNumber(state);
}

// <v-offset>  ::= <(offset) number> _ <(virtual offset) number>
static bool ParseVOffset(State *state) {
  State copy = *state;
  if (ParseNumber(state) && ParseChar(state, '_') &&
      ParseNumber(state)) {
    return true;
  }
  *state = copy;
  return false;
}

// <ctor-dtor-name> ::= C1 | C2 | C3
//                  ::= D0 | D1 | D2
static bool ParseCtorDtorName(State *state) {
  State copy = *state;
  if (ParseChar(state, 'C') &&
      ParseCharClass(state, "123")) {
    const char * const prev_name = state->prev_name;
    const int prev_name_length = state->prev_name_length;
    MaybeAppendWithLength(state, prev_name, prev_name_length);
    return true;
  }
  *state = copy;

  if (ParseChar(state, 'D') &&
      ParseCharClass(state, "012")) {
    const char * const prev_name = state->prev_name;
    const int prev_name_length = state->prev_name_length;
    MaybeAppend(state, "~");
    MaybeAppendWithLength(state, prev_name, prev_name_length);
    return true;
  }
  *state = copy;
  return false;
}

// <type> ::= <CV-qualifiers> <type>
//        ::= P <type>
//        ::= R <type>
//        ::= C <type>
//        ::= G <type>
//        ::= U <source-name> <type>
//        ::= <builtin-type>
//        ::= <function-type>
//        ::= <class-enum-type>
//        ::= <array-type>
//        ::= <pointer-to-member-type>
//        ::= <template-template-param> <template-args>
//        ::= <template-param>
//        ::= <substitution>
static bool ParseType(State *state) {
  // We should check CV-qualifers, and PRGC things first.
  State copy = *state;
  if (ParseCVQualifiers(state) && ParseType(state)) {
    return true;
  }
  *state = copy;

  if (ParseCharClass(state, "PRCG") && ParseType(state)) {
    return true;
  }
  *state = copy;

  if (ParseChar(state, 'U') && ParseSourceName(state) &&
      ParseType(state)) {
    return true;
  }
  *state = copy;

  if (ParseBuiltinType(state) ||
      ParseFunctionType(state) ||
      ParseClassEnumType(state) ||
      ParseArrayType(state) ||
      ParsePointerToMemberType(state) ||
      ParseSubstitution(state)) {
    return true;
  }

  if (ParseTemplateTemplateParam(state) &&
      ParseTemplateArgs(state)) {
    return true;
  }
  *state = copy;

  // Less greedy than <template-template-param> <template-args>.
  if (ParseTemplateParam(state)) {
    return true;
  }

  return false;
}

// <CV-qualifiers> ::= [r] [V] [K]
// We don't allow empty <CV-qualifiers> to avoid infinite loop in
// ParseType().
static bool ParseCVQualifiers(State *state) {
  int num_cv_qualifiers = 0;
  num_cv_qualifiers += ParseChar(state, 'r');
  num_cv_qualifiers += ParseChar(state, 'V');
  num_cv_qualifiers += ParseChar(state, 'K');
  return num_cv_qualifiers > 0;
}

// <builtin-type> ::= v, etc.
//                ::= u <source-name>
static bool ParseBuiltinType(State *state) {
  const AbbrevPair *p;
  for (p = kBuiltinTypeList; p->abbrev != NULL; ++p) {
    if (state->mangled_cur[0] == p->abbrev[0]) {
      MaybeAppend(state, p->real_name);
      ++state->mangled_cur;
      return true;
    }
  }

  State copy = *state;
  if (ParseChar(state, 'u') && ParseSourceName(state)) {
    return true;
  }
  *state = copy;
  return false;
}

// <function-type> ::= F [Y] <bare-function-type> E
static bool ParseFunctionType(State *state) {
  State copy = *state;
  if (ParseChar(state, 'F') && Optional(ParseChar(state, 'Y')) &&
      ParseBareFunctionType(state) && ParseChar(state, 'E')) {
    return true;
  }
  *state = copy;
  return false;
}

// <bare-function-type> ::= <(signature) type>+
static bool ParseBareFunctionType(State *state) {
  State copy = *state;
  DisableAppend(state);
  if (OneOrMore(ParseType, state)) {
    RestoreAppend(state, copy.append);
    MaybeAppend(state, "()");
    return true;
  }
  *state = copy;
  return false;
}

// <class-enum-type> ::= <name>
static bool ParseClassEnumType(State *state) {
  return ParseName(state);
}

// <array-type> ::= A <(positive dimension) number> _ <(element) type>
//              ::= A [<(dimension) expression>] _ <(element) type>
static bool ParseArrayType(State *state) {
  State copy = *state;
  if (ParseChar(state, 'A') && ParseNumber(state) &&
      ParseChar(state, '_') && ParseType(state)) {
    return true;
  }
  *state = copy;

  if (ParseChar(state, 'A') && Optional(ParseExpression(state)) &&
      ParseChar(state, '_') && ParseType(state)) {
    return true;
  }
  *state = copy;
  return false;
}

// <pointer-to-member-type> ::= M <(class) type> <(member) type>
static bool ParsePointerToMemberType(State *state) {
  State copy = *state;
  if (ParseChar(state, 'M') && ParseType(state) &&
      ParseType(state)) {
    return true;
  }
  *state = copy;
  return false;
}

// <template-param> ::= T_
//                  ::= T <parameter-2 non-negative number> _
static bool ParseTemplateParam(State *state) {
  if (ParseTwoChar(state, "T_")) {
    MaybeAppend(state, "?");  // We don't support template substitutions.
    return true;
  }

  State copy = *state;
  if (ParseChar(state, 'T') && ParseNumber(state) &&
      ParseChar(state, '_')) {
    MaybeAppend(state, "?");  // We don't support template substitutions.
    return true;
  }
  *state = copy;
  return false;
}


// <template-template-param> ::= <template-param>
//                           ::= <substitution>
static bool ParseTemplateTemplateParam(State *state) {
  return (ParseTemplateParam(state) ||
          ParseSubstitution(state));
}

// <template-args> ::= I <template-arg>+ E
static bool ParseTemplateArgs(State *state) {
  State copy = *state;
  DisableAppend(state);
  if (ParseChar(state, 'I') &&
      OneOrMore(ParseTemplateArg, state) &&
      ParseChar(state, 'E')) {
    RestoreAppend(state, copy.append);
    MaybeAppend(state, "<>");
    return true;
  }
  *state = copy;
  return false;
}

// <template-arg>  ::= <type>
//                 ::= <expr-primary>
//                 ::= X <expression> E
static bool ParseTemplateArg(State *state) {
  if (ParseType(state) ||
      ParseExprPrimary(state)) {
    return true;
  }

  State copy = *state;
  if (ParseChar(state, 'X') && ParseExpression(state) &&
      ParseChar(state, 'E')) {
    return true;
  }
  *state = copy;
  return false;
}

// <expression> ::= <template-param>
//              ::= <expr-primary>
//              ::= <unary operator-name> <expression>
//              ::= <binary operator-name> <expression> <expression>
//              ::= <trinary operator-name> <expression> <expression>
//                  <expression>
//              ::= st <type>
//              ::= sr <type> <unqualified-name> <template-args>
//              ::= sr <type> <unqualified-name>
static bool ParseExpression(State *state) {
  if (ParseTemplateParam(state) || ParseExprPrimary(state)) {
    return true;
  }

  State copy = *state;
  if (ParseOperatorName(state) &&
      ParseExpression(state) &&
      ParseExpression(state) &&
      ParseExpression(state)) {
    return true;
  }
  *state = copy;

  if (ParseOperatorName(state) &&
      ParseExpression(state) &&
      ParseExpression(state)) {
    return true;
  }
  *state = copy;

  if (ParseOperatorName(state) &&
      ParseExpression(state)) {
    return true;
  }
  *state = copy;

  if (ParseTwoChar(state, "st") && ParseType(state)) {
    return true;
  }
  *state = copy;

  if (ParseTwoChar(state, "sr") && ParseType(state) &&
      ParseUnqualifiedName(state) &&
      ParseTemplateArgs(state)) {
    return true;
  }
  *state = copy;

  if (ParseTwoChar(state, "sr") && ParseType(state) &&
      ParseUnqualifiedName(state)) {
    return true;
  }
  *state = copy;
  return false;
}

// <expr-primary> ::= L <type> <(value) number> E
//                ::= L <type> <(value) float> E
//                ::= L <mangled-name> E
//                // A bug in g++'s C++ ABI version 2 (-fabi-version=2).
//                ::= LZ <encoding> E
static bool ParseExprPrimary(State *state) {
  State copy = *state;
  if (ParseChar(state, 'L') && ParseType(state) &&
      ParseNumber(state) &&
      ParseChar(state, 'E')) {
    return true;
  }
  *state = copy;

  if (ParseChar(state, 'L') && ParseType(state) &&
      ParseFloatNumber(state) &&
      ParseChar(state, 'E')) {
    return true;
  }
  *state = copy;

  if (ParseChar(state, 'L') && ParseMangledName(state) &&
      ParseChar(state, 'E')) {
    return true;
  }
  *state = copy;

  if (ParseTwoChar(state, "LZ") && ParseEncoding(state) &&
      ParseChar(state, 'E')) {
    return true;
  }
  *state = copy;

  return false;
}

// <local-name> := Z <(function) encoding> E <(entity) name>
//                 [<discriminator>]
//              := Z <(function) encoding> E s [<discriminator>]
static bool ParseLocalName(State *state) {
  State copy = *state;
  if (ParseChar(state, 'Z') && ParseEncoding(state) &&
      ParseChar(state, 'E') && MaybeAppend(state, "::") &&
      ParseName(state) && Optional(ParseDiscriminator(state))) {
    return true;
  }
  *state = copy;

  if (ParseChar(state, 'Z') && ParseEncoding(state) &&
      ParseTwoChar(state, "Es") && Optional(ParseDiscriminator(state))) {
    return true;
  }
  *state = copy;
  return false;
}

// <discriminator> := _ <(non-negative) number>
static bool ParseDiscriminator(State *state) {
  State copy = *state;
  if (ParseChar(state, '_') && ParseNumber(state)) {
    return true;
  }
  *state = copy;
  return false;
}

// <substitution> ::= S_
//                ::= S <seq-id> _
//                ::= St, etc.
static bool ParseSubstitution(State *state) {
  if (ParseTwoChar(state, "S_")) {
    MaybeAppend(state, "?");  // We don't support substitutions.
    return true;
  }

  State copy = *state;
  if (ParseChar(state, 'S') && ParseSeqId(state) &&
      ParseChar(state, '_')) {
    MaybeAppend(state, "?");  // We don't support substitutions.
    return true;
  }
  *state = copy;

  // Expand abbreviations like "St" => "std".
  if (ParseChar(state, 'S')) {
    const AbbrevPair *p;
    for (p = kSubstitutionList; p->abbrev != NULL; ++p) {
      if (state->mangled_cur[0] == p->abbrev[1]) {
        MaybeAppend(state, "std");
        if (p->real_name[0] != '\0') {
          MaybeAppend(state, "::");
          MaybeAppend(state, p->real_name);
        }
        state->mangled_cur += 1;
        return true;
      }
    }
  }
  *state = copy;
  return false;
}

// The demangler entry point.
bool Demangle(const char *mangled, char *out, int out_size) {
  State state;
  InitState(&state, mangled, out, out_size);
  return (ParseMangledName(&state) &&
          state.overflowed == false &&
          RemainingLength(&state) == 0);
}

_END_GOOGLE_NAMESPACE_
