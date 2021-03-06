#!/usr/bin/python
#
# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

""" Lexer for PPAPI IDL """

#
# IDL Lexer
#
# The lexer is uses the PLY lex library to build a tokenizer which understands
# WebIDL tokens.
#
# WebIDL, and WebIDL regular expressions can be found at:
#   http://dev.w3.org/2006/webapi/WebIDL/
# PLY can be found at:
#   http://www.dabeaz.com/ply/

import getopt
import os.path
import re
import sys

LEXER_OPTIONS = {
  'output': False,
  'test_expect' : False,
  'test_same' : False,
  'verbose': False
}


#
# Try to load the ply module, if not, then assume it is in the third_party
# directory, relative to ppapi
#
try:
  from ply import lex
except:
  module_path, module_name = os.path.split(__file__)
  third_party = os.path.join(module_path, '..', '..', 'third_party')
  sys.path.append(third_party)
  from ply import lex



#
# IDL Lexer
#
class IDLLexer(object):
  # 'tokens' is a value required by lex which specifies the complete list
  # of valid token types.
  tokens = [
    # Symbol and keywords types
      'COMMENT',
      'DESCRIBE',
      'ENUM',
      'SYMBOL',
      'INTERFACE',
      'STRUCT',
      'TYPEDEF',

    # Data types
      'FLOAT',
      'OCT',
      'INT',
      'HEX',
      'STRING',

    # Operators
      'LSHIFT'
  ]

  # 'keywords' is a map of string to token type.  All SYMBOL tokens are
  # matched against keywords, to determine if the token is actually a keyword.
  keywords = {
    'describe' : 'DESCRIBE',
    'enum'  : 'ENUM',
    'interface' : 'INTERFACE',
    'readonly' : 'READONLY',
    'struct' : 'STRUCT',
    'typedef' : 'TYPEDEF',
  }

  # 'literals' is a value expected by lex which specifies a list of valid
  # literal tokens, meaning the token type and token value are identical.
  literals = '"*.(){}[],;:=+-'

  # Token definitions
  #
  # Lex assumes any value or function in the form of 't_<TYPE>' represents a
  # regular expression where a match will emit a token of type <TYPE>.  In the
  # case of a function, the function is called when a match is made. These
  # definitions come from WebIDL.

  # 't_ignore' is a special match of items to ignore
  t_ignore = ' \t'

  # Constant values
  t_FLOAT = r'-?(\d+\.\d*|\d*\.\d+)([Ee][+-]?\d+)?|-?\d+[Ee][+-]?\d+'
  t_INT = r'-?[0-9]+'
  t_OCT = r'-?0[0-7]+'
  t_HEX = r'-?0[Xx][0-9A-Fa-f]+'
  t_LSHIFT = r'<<'

  # A line ending '\n', we use this to increment the line number
  def t_LINE_END(self, t):
    r'\n+'
    self.AddLines(len(t.value))

  # We do not process escapes in the IDL strings.  Strings are exclusively
  # used for attributes, and not used as typical 'C' constants.
  def t_STRING(self, t):
    r'"[^"]*"'
    t.value = t.value[1:-1]
    self.AddLines(t.value.count('\n'))
    return t

  # A C or C++ style comment:  /* xxx */ or //
  def t_COMMENT(self, t):
    r'(/\*(.|\n)*?\*/)|(//.*)'
    self.AddLines(t.value.count('\n'))

    # C++ comments should keep the newline
    if t.value[:2] == '//': t.value += '\n'
    return t

  # A symbol or keyword.
  def t_KEYWORD_SYMBOL(self, t):
    r'[A-Za-z][A-Za-z_0-9]*'

    #All non-keywords are assumed to be symbols
    t.type = self.keywords.get(t.value, 'SYMBOL')
    return t

  def t_ANY_error(self, t):
    line = self.lexobj.lineno
    pos = self.lexobj.lexpos - self.index[line]
    file = self.lexobj.filename
    out = self.ErrorMessage(file, line, pos, "Unrecognized input")
    sys.stderr.write(out + '\n')

  def AddLines(self, count):
    # Set the lexer position for the beginning of the next line.  In the case
    # of multiple lines, tokens can not exist on any of the lines except the
    # last one, so the recorded value for previous lines are unused.  We still
    # fill the array however, to make sure the line count is correct.
    self.lexobj.lineno += count
    for i in range(count):
      self.index.append(self.lexobj.lexpos)

  def FileLineMsg(self, file, line, msg):
    if file:  return "%s(%d) : %s" % (file, line + 1, msg)
    return "<BuiltIn> : %s" % msg

  def SourceLine(self, file, line, pos):
    caret = '\t^'.expandtabs(pos)
    return "%s\n%s" % (self.lines[line], caret)

  def ErrorMessage(self, file, line, pos, msg):
    return "\n%s\n%s" % (
        self.FileLineMsg(file, line, msg),
        self.SourceLine(file, line, pos))

  def SetData(self, filename, data):
    self.lexobj.filename = filename
    self.lexobj.lineno = 0
    self.lines = data.split('\n')
    self.index = [0]
    self.lexobj.input(data)

  def __init__(self, options = {}):
    self.lexobj = lex.lex(object=self, lextab=None, optimize=0)
    for k in options:
      LEXER_OPTIONS[k] = True



#
# FilesToTokens
#
# From a set of source file names, generate a list of tokens.
#
def FilesToTokens(filenames, verbose=False):
  lexer = IDLLexer()
  outlist = []
  for filename in filenames:
    data = open(filename).read()
    lexer.SetData(filename, data)
    if verbose: sys.stdout.write('  Loaded %s...\n' % filename)
    while 1:
      t = lexer.lexobj.token()
      if t is None: break
      outlist.append(t)
  return outlist

#
# TextToTokens
#
# From a block of text, generate a list of tokens
#
def TextToTokens(source):
  lexer = IDLLexer()
  outlist = []
  lexer.SetData('AUTO', source)
  while 1:
    t = lexer.lexobj.token()
    if t is None: break
    outlist.append(t.value)
  return outlist


#
# TestSame
#
# From a set of token values, generate a new source text by joining with a
# single space.  The new source is then tokenized and compared against the
# old set.
#
def TestSame(values):
  global LEXER_OPTIONS

  src1 = ' '.join(values)
  src2 = ' '.join(TextToTokens(src1))

  if LEXER_OPTIONS['output']:
    sys.stdout.write('Generating original.txt and tokenized.txt\n')
    open('original.txt', 'w').write(src1)
    open('tokenized.txt', 'w').write(src2)

  if src1 == src2:
    sys.stdout.write('Same: Pass\n')
    return 0

  sys.stdout.write('Same: Failed\n')
  return -1


#
# TestExpect
#
# From a set of tokens pairs, verify the type field of the second matches
# the value of the first, so that:
# INT 123 FLOAT 1.1
# will generate a passing test, where the first token is the SYMBOL INT,
# and the second token is the INT 123, third token is the SYMBOL FLOAT and
# the fourth is the FLOAT 1.1, etc...
def TestExpect(tokens):
  count = len(tokens)
  index = 0
  errors = 0
  while index < count:
    type = tokens[index].value
    token = tokens[index + 1]
    index += 2

    if type != token.type:
      sys.stderr.write('Mismatch:  Expected %s, but got %s = %s.\n' %
                       (type, token.type, token.value))
      errors += 1

  if not errors:
    sys.stdout.write('Expect: Pass\n')
    return 0

  sys.stdout.write('Expect: Failed\n')
  return -1




def Main(args):
  global LEXER_OPTIONS

  try:
    long_opts = ['output', 'verbose', 'test_expect', 'test_same']
    usage = 'Usage: idl_lexer.py %s [<src.idl> ...]' % ' '.join(
       ['--%s' % opt for opt in long_opts])

    opts, filenames = getopt.getopt(args, '', long_opts)
  except getopt.error, e:
    sys.stderr.write('Illegal option: %s\n%s\n' % (str(e), usage))
    return 1

  output = False
  test_same = False
  test_expect = False
  verbose = False

  for opt, val in opts:
    LEXER_OPTIONS[opt[2:]] = True

  try:
    tokens = FilesToTokens(filenames, verbose)
    values = [tok.value for tok in tokens]
    if LEXER_OPTIONS['output']: sys.stdout.write(' <> '.join(values) + '\n')
    if LEXER_OPTIONS['test_same']:
      if TestSame(values):
        return -1

    if LEXER_OPTIONS['test_expect']:
      if TestExpect(tokens):
        return -1
    return 0

  except lex.LexError as le:
    sys.stderr.write('%s\n' % str(le))
  return -1


if __name__ == '__main__':
  sys.exit(Main(sys.argv[1:]))

