#!/usr/bin/env python
# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generate and process code coverage on POSIX systems.

Written for and tested on Mac and Linux.  To use this script to
generate coverage numbers, please run from within a gyp-generated
project.

All platforms, to set up coverage:
  cd ...../chromium ; src/tools/gyp/gyp_dogfood -Dcoverage=1 src/build/all.gyp

Run coverage on...
Mac:
  ( cd src/chrome ; xcodebuild -configuration Debug -target coverage )
Linux:
  ( cd src/chrome ; hammer coverage )
  # In particular, don't try and run 'coverage' from src/build


--directory=DIR: specify directory that contains gcda files, and where
  a "coverage" directory will be created containing the output html.
  Example name:   ..../chromium/src/xcodebuild/Debug

--all_unittests: is present, run all files named *_unittests that we
  can find.

Strings after all options are considered tests to run.  Test names
have all text before a ':' stripped to help with gyp compatibility.
For example, ../base/base.gyp:base_unittests is interpreted as a test
named "base_unittests".
"""

import glob
import logging
import optparse
import os
import shutil
import subprocess
import sys

class Coverage(object):
  """Doitall class for code coverage."""

  def __init__(self, directory):
    super(Coverage, self).__init__()
    self.directory = directory
    self.directory_parent = os.path.dirname(self.directory)
    self.output_directory = os.path.join(self.directory, 'coverage')
    if not os.path.exists(self.output_directory):
      os.mkdir(self.output_directory)
    self.lcov_directory = os.path.join(sys.path[0],
                                       '../../third_party/lcov/bin')
    self.lcov = os.path.join(self.lcov_directory, 'lcov')
    self.mcov = os.path.join(self.lcov_directory, 'mcov')
    self.genhtml = os.path.join(self.lcov_directory, 'genhtml')
    self.coverage_info_file = os.path.join(self.directory, 'coverage.info')
    self.ConfirmPlatformAndPaths()
    self.tests = []

  def FindTests(self, options, args):
    """Find unit tests to run; set self.tests to this list.

    Obtain instructions from the command line seen in the provided
    parsed options and post-option args.
    """
    # Small tests: can be run in the "chromium" directory.
    # If asked, run all we can find.
    if options.all_unittests:
      self.tests += glob.glob(os.path.join(self.directory, '*_unittests'))

    # If told explicit tests, run those (after stripping the name as
    # appropriate)
    for testname in args:
      if ':' in testname:
        self.tests += [os.path.join(self.directory, testname.split(':')[1])]
      else:
        self.tests += [os.path.join(self.directory, testname)]

    # Needs to be run in the "chrome" directory?
    # ut = os.path.join(self.directory, 'unit_tests')
    # if os.path.exists(ut):
    #  self.tests.append(ut)
    # Medium tests?
    # Not sure all of these work yet (e.g. page_cycler_tests)
    # self.tests += glob.glob(os.path.join(self.directory, '*_tests'))

  def ConfirmPlatformAndPaths(self):
    """Confirm OS and paths (e.g. lcov)."""
    if not self.IsPosix():
      logging.fatal('Not posix.')
    programs = [self.lcov, self.genhtml]
    if self.IsMac():
      programs.append(self.mcov)
    for program in programs:
      if not os.path.exists(program):
        logging.fatal('lcov program missing: ' + program)

  def IsPosix(self):
    """Return True if we are POSIX."""
    return self.IsMac() or self.IsLinux()

  def IsMac(self):
    return sys.platform == 'darwin'

  def IsLinux(self):
    return sys.platform == 'linux2'

  def ClearData(self):
    """Clear old gcda files"""
    subprocess.call([self.lcov,
                     '--directory', self.directory_parent,
                     '--zerocounters'])
    shutil.rmtree(os.path.join(self.directory, 'coverage'))

  def RunTests(self):
    """Run all unit tests."""
    for fulltest in self.tests:
      if not os.path.exists(fulltest):
        logging.fatal(fulltest + ' does not exist')
      # TODO(jrg): add timeout?
      # TODO(jrg): check return value and choke if it failed?
      # TODO(jrg): add --gtest_print_time like as run from XCode?
      print 'Running test: ' + fulltest
      # subprocess.call([fulltest, '--gtest_filter=TupleTest*'])  # quick check
      subprocess.call([fulltest])

  def GenerateOutput(self):
    """Convert profile data to html."""
    if self.IsLinux():
      command = [self.lcov,
                 '--directory', self.directory,
                 '--capture',
                 '--output-file',
                 self.coverage_info_file]
    else:
      command = [self.mcov,
                 '--directory', self.directory_parent,
                 '--output', self.coverage_info_file]
    print 'Assembly command: ' + ' '.join(command)
    subprocess.call(command)

    command = [self.genhtml,
               self.coverage_info_file,
               '--output-directory',
               self.output_directory]
    print 'html generation command: ' + ' '.join(command)
    subprocess.call(command)

def main():
  parser = optparse.OptionParser()
  parser.add_option('-d',
                    '--directory',
                    dest='directory',
                    default=None,
                    help='Directory of unit test files')
  parser.add_option('-a',
                    '--all_unittests',
                    dest='all_unittests',
                    default=False,
                    help='Run all tests we can find (*_unittests)')
  (options, args) = parser.parse_args()
  if not options.directory:
    parser.error('Directory not specified')

  coverage = Coverage(options.directory)
  coverage.ClearData()
  coverage.FindTests(options, args)
  coverage.RunTests()
  coverage.GenerateOutput()
  return 0


if __name__ == '__main__':
  sys.exit(main())
