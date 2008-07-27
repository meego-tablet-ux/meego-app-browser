#!/usr/bin/python2.4
# Copyright 2008, Google Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#    * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#    * Neither the name of Google Inc. nor the names of its
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

'''Unit test that checks some of util functions.
'''

import os
import sys
if __name__ == '__main__':
  sys.path.append(os.path.join(os.path.dirname(sys.argv[0]), '..'))

import unittest

from grit import util


class UtilUnittest(unittest.TestCase):
  ''' Tests functions from util
  '''
  
  def testNewClassInstance(self):
    # Test short class name with no fully qualified package name
    # Should fail, it is not supported by the function now (as documented)
    cls = util.NewClassInstance('grit.util.TestClassToLoad',
                                TestBaseClassToLoad)
    self.failUnless(cls == None)
  
    # Test non existent class name
    cls = util.NewClassInstance('grit.util_unittest.NotExistingClass',
                                TestBaseClassToLoad) 
    self.failUnless(cls == None)

    # Test valid class name and valid base class
    cls = util.NewClassInstance('grit.util_unittest.TestClassToLoad',
                                TestBaseClassToLoad)
    self.failUnless(isinstance(cls, TestBaseClassToLoad))

    # Test valid class name with wrong hierarchy
    cls = util.NewClassInstance('grit.util_unittest.TestClassNoBase',
                                TestBaseClassToLoad)
    self.failUnless(cls == None)
  
  def testCanonicalLanguage(self):
    self.failUnless(util.CanonicalLanguage('en') == 'en')
    self.failUnless(util.CanonicalLanguage('pt_br') == 'pt-BR')
    self.failUnless(util.CanonicalLanguage('pt-br') == 'pt-BR')
    self.failUnless(util.CanonicalLanguage('pt-BR') == 'pt-BR')
    self.failUnless(util.CanonicalLanguage('pt/br') == 'pt-BR')
    self.failUnless(util.CanonicalLanguage('pt/BR') == 'pt-BR')
    self.failUnless(util.CanonicalLanguage('no_no_bokmal') == 'no-NO-BOKMAL')

  def testUnescapeHtml(self):
    self.failUnless(util.UnescapeHtml('&#1010;') == unichr(1010))
    self.failUnless(util.UnescapeHtml('&#xABcd;') == unichr(43981))

class TestBaseClassToLoad(object):
  pass

class TestClassToLoad(TestBaseClassToLoad):
  pass

class TestClassNoBase(object):
  pass


if __name__ == '__main__':
  unittest.main()
