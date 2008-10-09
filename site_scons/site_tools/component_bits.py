#!/usr/bin/python2.4
# Copyright 2008, Google Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""Environment bit support for software construction toolkit.

This module is automatically included by the component_setup tool.
"""


import __builtin__
import SCons


_bit_descriptions = {}
_bits_with_options = set()
_bit_exclusive_groups = {}

#------------------------------------------------------------------------------


def DeclareBit(bit_name, desc, exclusive_groups=tuple()):
  """Declares and describes the bit.

  Args:
    bit_name: Name of the bit being described.
    desc: Description of bit.
    exclusive_groups: Bit groups which this bit belongs to.  At most one bit
        may be set in each exclusive group.

  Raises:
    ValueError: The bit has already been defined with a different description,
        or the description is empty.

  Adds a description for the bit in the global dictionary of bit names.  All
  bits should be described before being used in Bit()/AllBits()/AnyBits().
  """

  if not desc:
    raise ValueError('Must supply a description for bit "%s"' % bit_name)

  existing_desc = _bit_descriptions.get(bit_name)
  if existing_desc and desc != existing_desc:
    raise ValueError('Cannot describe bit "%s" as "%s" because it has already'
                     'been described as "%s".' %
                     (bit_name, desc, existing_desc))

  _bit_descriptions[bit_name] = desc

  # Add bit to its exclusive groups
  for g in exclusive_groups:
    if g not in _bit_exclusive_groups:
      _bit_exclusive_groups[g] = set()
    _bit_exclusive_groups[g].add(bit_name)

#------------------------------------------------------------------------------


def Bit(env, bit_name):
  """Checks if the environment has the bit.

  Args:
    env: Environment to check.
    bit_name: Name of the bit to check.

  Returns:
    True if the bit is present in the environment.
  """
  # TODO(rspangler): Add bit sanity checking (description exists, exclusive
  # groups not violated).
  return bit_name in env['_BITS']

#------------------------------------------------------------------------------


def AllBits(env, *args):
  """Checks if the environment has all the bits.

  Args:
    env: Environment to check.
    args: List of bit names to check.

  Returns:
    True if every bit listed is present in the environment.
  """
  # TODO(rspangler): Add bit sanity checking
  return set(args).issubset(env['_BITS'])

#------------------------------------------------------------------------------


def AnyBits(env, *args):
  """Checks if the environment has at least one of the bits.

  Args:
    env: Environment to check.
    args: List of bit names to check.

  Returns:
    True if at least one bit listed is present in the environment.
  """
  # TODO(rspangler): Add bit sanity checking
  return set(args).intersection(env['_BITS'])

#------------------------------------------------------------------------------


def SetBits(env, *args):
  """Sets the bits in the environment.

  Args:
    env: Environment to check.
    args: List of bit names to set.
  """
  # TODO(rspangler): Add bit sanity checking
  env['_BITS'] = env['_BITS'].union(args)

#------------------------------------------------------------------------------


def ClearBits(env, *args):
  """Sets the bits in the environment.

  Args:
    env: Environment to check.
    args: List of bit names to set.
  """
  # TODO(rspangler): Add bit sanity checking
  env['_BITS'] = env['_BITS'].difference(args)

#------------------------------------------------------------------------------


def SetBitFromOption(env, bit_name, default):
  """Sets the bit in the environment from a command line option.

  Args:
    env: Environment to check.
    bit_name: Name of the bit to set from a command line option.
    default: Default value for bit if command line option is not present.
  """
  # TODO(rspangler): Add bit sanity checking

  # Add the command line option, if not already present
  if bit_name not in _bits_with_options:
    _bits_with_options.add(bit_name)
    SCons.Script.AddOption('--' + bit_name,
                           dest=bit_name,
                           action='store_true',
                           help='set bit:' + _bit_descriptions[bit_name])
    SCons.Script.AddOption('--no-' + bit_name,
                           dest=bit_name,
                           action='store_false',
                           help='clear bit:' + _bit_descriptions[bit_name])

  bit_set = env.GetOption(bit_name)
  if bit_set is None:
    # Not specified on command line, so use default
    bit_set = default

  if bit_set:
    env['_BITS'].add(bit_name)
  elif bit_name in env['_BITS']:
    env['_BITS'].remove(bit_name)

#------------------------------------------------------------------------------


def generate(env):
  # NOTE: SCons requires the use of this name, which fails gpylint.
  """SCons entry point for this tool."""

  # Add methods to builtin
  # TODO(rspangler): These really belong in site_init.py - but if we do that,
  # what's the right way to access the bit global variables?
  __builtin__.DeclareBit = DeclareBit

  # Add methods to environment
  env.AddMethod(AllBits)
  env.AddMethod(AnyBits)
  env.AddMethod(Bit)
  env.AddMethod(ClearBits)
  env.AddMethod(SetBitFromOption)
  env.AddMethod(SetBits)

  env['_BITS'] = set()
