#!/bin/env python
# Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""Module to setup and generate code coverage data
"""


import logging
import optparse
import os
import shutil
import subprocess
import sys
import tempfile

import google.logging_utils
import google.process_utils as proc


class Coverage(object):
  """Class to set up and generate code coverage.

  This class contains methods that are useful to set up the environment for 
  code coverage.

  Attributes:
    instrumented: A boolean indicating if all the binaries have been
                  instrumented.
  """

  def __init__(self, 
               revision,
               vsts_path = None,
               lcov_converter_path = None):
    google.logging_utils.config_root()
    self.revision = revision
    self.instrumented = False
    self.vsts_path = vsts_path
    self.lcov_converter_path = lcov_converter_path
    self._dir = None
  
  
  def _IsWindows(self):
    """Checks if the current platform is Windows.
    """
    return sys.platform[:3] == 'win'
  
  
  def SetUp(self, binaries):
    """Set up the platform specific environment and instrument the binaries for
    coverage.

    This method sets up the environment, instruments all the compiled binaries 
    and sets up the code coverage counters.

    Args:
      binaries: List of binaries that need to be instrumented.

    Returns:
      Path of the file containing code coverage data on successful
      instrumentation.
      None on error.
    """
    if self.instrumented:
      logging.error('Binaries already instrumented')
      return None
    coverage_file = None
    if self._IsWindows():
      # Stop all previous instance of VSPerfMon counters
      counters_command = ('%s -shutdown' % 
                          (os.path.join(self.vsts_path, 'vsperfcmd.exe')))
      (retcode, output) = proc.RunCommandFull(counters_command,
                                              collect_output=True)
      # TODO(niranjan): Add a check that to verify that the binaries were built
      # using the /PROFILE linker flag.
      if self.vsts_path == None or self.lcov_converter_path == None:
        return None
      # Remove trailing slashes
      self.vsts_path = self.vsts_path.rstrip('\\') 
      instrument_command = '%s /COVERAGE ' % (os.path.join(self.vsts_path,
                                                           'vsinstr.exe'))
      for binary in binaries:
        logging.info('binary = %s' % (binary))
        logging.info('instrument_command = %s' % (instrument_command))
        # Instrument each binary in the list
        (retcode, output) = proc.RunCommandFull(instrument_command + binary,
                                                collect_output=True)
        # Check if the file has been instrumented correctly.
        if output.pop().rfind('Successfully instrumented') == -1:
          logging.error('Error instrumenting %s' % (binary))
          return None

      # Generate the file name for the coverage results
      self._dir = tempfile.mkdtemp()
      coverage_file = os.path.join(self._dir, 'chrome_win32_%s.coverage' % 
                                              (self.revision))
      logging.info('.coverage file: %s' % (coverage_file))

      # After all the binaries have been instrumented, we start the counters.
      counters_command = ('%s -start:coverage -output:%s' %
                          (os.path.join(self.vsts_path, 'vsperfcmd.exe'),
                          coverage_file))
      # Here we use subprocess.call() instead of the RunCommandFull because the
      # VSPerfCmd spawns another process before terminating and this confuses
      # the subprocess.Popen() used by RunCommandFull.
      retcode = subprocess.call(counters_command)
      # TODO(niranjan): Check whether the counters have been started
      # successfully.
      
      # We are now ready to run tests and measure code coverage.
      self.instrumented = True
    else:
      return None
    return coverage_file
      

  def TearDown(self):
    """Tear down method.

    This method shuts down the counters, and cleans up all the intermediate
    artifacts. 
    """
    if self.instrumented == False:
      return
    
    if self._IsWindows():
      # Stop counters
      counters_command = ('%s -shutdown' % 
                         (os.path.join(self.vsts_path, 'vsperfcmd.exe')))
      (retcode, output) = proc.RunCommandFull(counters_command,
                                              collect_output=True)
      logging.info('Counters shut down: %s' % (output))
      # TODO(niranjan): Revert the instrumented binaries to their original
      # versions.
    else:
      return
    # Delete all the temp files and folders
    if self._dir != None:
      shutil.rmtree(self._dir, ignore_errors=True)
      logging.info('Cleaned up temporary files and folders')
    # Reset the instrumented flag.
    self.instrumented = False
    

  def Upload(self, coverage_file, upload_path, sym_path=None, src_root=None):
    """Upload the results to the dashboard.

    This method uploads the coverage data to a dashboard where it will be
    processed. On Windows, this method will first convert the .coverage file to
    the lcov format. This method needs to be called before the TearDown method.

    Args:
      coverage_file: The coverage data file to upload.
      upload_path: Destination where the coverage data will be processed.
      sym_path: Symbol path for the build (Win32 only)
      src_root: Root folder of the source tree (Win32 only)
    
    Returns:
      True on success.
      False on failure.
    """
    if self._IsWindows():
      # Stop counters
      counters_command = ('%s -shutdown' % 
                         (os.path.join(self.vsts_path, 'vsperfcmd.exe')))
      (retcode, output) = proc.RunCommandFull(counters_command,
                                              collect_output=True)
      logging.info('Counters shut down: %s' % (output))
      # Convert the .coverage file to lcov format
      if self.lcov_converter_path == False:
        logging.error('Lcov converter tool not found')
        return False
      self.lcov_converter_path = self.lcov_converter_path.rstrip('\\')
      convert_command = ('%s -sym_path=%s -src_root=%s ' % 
                         (os.path.join(self.lcov_converter_path, 
                                      'coverage_analyzer.exe'),
                         sym_path,
                         src_root))
      logging.info('Conversion to lcov complete')
      (retcode, output) = proc.RunCommandFull(convert_command + coverage_file,
                                              collect_output=True)
      shutil.copy(coverage_file, coverage_file.replace('.coverage', ''))
    # TODO(niranjan): Upload this somewhere!

