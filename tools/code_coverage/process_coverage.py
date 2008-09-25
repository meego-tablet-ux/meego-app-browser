#!/usr/bin/python2.4
#
# Copyright 2008, Google Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#        * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#        * Redistributions in binary form must reproduce the above
#     copyright notice, this list of conditions and the following disclaimer
#     in the documentation and/or other materials provided with the
#     distribution.
#        * Neither the name of Google Inc. nor the names of its
#     contributors may be used to endorse or promote products derived from
#     this software without specific prior written permission.
#
#     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#     "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#     A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#     OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#     SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#     LIMITED TO, PROCUREMENT  OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#     DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#     THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#     (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#     OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


"""Script to clean the lcov files and convert it to HTML

TODO(niranjan): Add usage information here
"""


import optparse
import os
import shutil
import sys
import tempfile


# These are source files that were generated during compile time. We want to
# remove references to these files from the lcov file otherwise genhtml will
# throw an error.
win32_srcs_exclude = ['parse.y',
                      'xpathgrammar.cpp',
                      'cssgrammar.cpp']


def CleanPathNames(dir):
  """Clean the pathnames of the HTML generated by genhtml.

  This method is required only for code coverage on Win32. Due to a known issue
  with reading from CIFS shares mounted on Linux, genhtml appends a ^M to every
  file name it reads from the Windows share, causing corrupt filenames in
  genhtml's output folder.

  Args:
    dir: Output folder of the genhtml output.

  Returns:
    None
  """
  # Stip off the ^M characters that get appended to the file name
  for file in os.walk(dir):
    file_clean = file.replace('\r', '')
    if file_clean != file:
      os.rename(file, file_clean)


def GenerateHtml(lcov_path, dash_root):
  """Runs genhtml to convert lcov data to human readable HTML.

  This script expects the LCOV file name to be in the format:
  chrome_<platform>_<revision#>.lcov.
  This method parses the file name and then sets up the correct folder
  hierarchy for the coverage data and then runs genhtml to get the actual HTML
  formatted coverage data.

  Args:
    lcov_path: Path of the lcov data file.
    dash_root: Root location of the dashboard.

  Returns:
    Code coverage percentage on sucess.
    None on failure.
  """
  # Parse the LCOV file name.
  filename = os.path.basename(lcov_path).split('.')[0]
  buffer = filename.split('_')
  dash_root = dash_root.rstrip('/') # Remove trailing '/'

  # Set up correct folder heirarchy in the dashboard root
  # TODO(niranjan): Check the formatting using a regexp
  if len(buffer) >= 3: # Check if filename has right formatting
    platform = buffer[len(buffer) - 2]
    revision = buffer[len(buffer) - 1]
    if os.path.exists(os.path.join(dash_root, platform)) == False:
      os.mkdir(os.path.join(dash_root, platform))
    output_dir = os.join.path(dash_root, platform, revision)
    os.mkdir(output_dir)
  else:
    # TODO(niranjan): Add failure logging here.
    return None # File not formatted correctly
  
  # Run genhtml
  os.system('/usr/bin/genhtml -o %s %s' % (output_dir, lcov_path))
  # TODO(niranjan): Check the exit status of the genhtml command.
  # TODO(niranjan): Parse the stdout and return coverage percentage.
  CleanPathNames(output_dir)
  return 'dummy' # TODO(niranjan): Return actual percentage.


def CleanWin32Lcov(lcov_path, src_root):
  """Cleanup the lcov data generated on Windows.

  This method fixes up the paths inside the lcov file from the Win32 specific
  paths to the actual paths of the mounted CIFS share. The lcov files generated
  on Windows have the following format:

  SF:c:\chrome_src\src\skia\sgl\skscan_antihair.cpp
  DA:97,0
  DA:106,0
  DA:107,0
  DA:109,0
  ...
  end_of_record

  This method changes the source-file (SF) lines to a format compatible with
  genhtml on Linux by fixing paths. This method also removes references to
  certain dynamically generated files to be excluded from the code ceverage.

  Args:
    lcov_path: Path of the Win32 lcov file to be cleaned.
    src_root: Location of the source and symbols dir.
  Returns:
    None
  """
  strip_flag = False
  lcov = open(lcov_path, 'r')
  (tmpfile, tmpfile_name) = tempfile.mkstemp()
  src_root = src_root.rstrip('/')       # Remove trailing '/'
  for line in lcov:
    if line.startswith('SF'):
      # We want to exclude certain auto-generated files otherwise genhtml will
      # fail to convert lcov to HTML.
      for exp in win32_srcs_exclude:
        if line.rfind(exp) != -1:
          strip_flag = True # Indicates that we want to remove this section

      # Now we normalize the paths
      # e.g. Change SF:c:\foo\src\... to SF:/chrome_src/...
      parse_buffer = line.split(':')
      buffer = '%s:%s%s' % (parse_buffer[0],
                            src_root,
                            parse_buffer[2])
      buffer = buffer.replace('\\', '/')
      line = buffer

    # Write to the temp file if the section to write is valid
    if strip_flag == False:
      tmpfile.write('%s' % (line))

    # Reset the strip flag
    if line.endswith('end_of_record'):
      strip_flag = False

  # Close the files and replace the lcov file by the 'clean' tmpfile
  tmpfile.close()
  lcov.close()
  shutil.move(tmpfile_name, lcov_path)


def main():
  if sys.platform[:5] != 'linux': # Run this only on Linux
    print 'This script is supported only on Linux'
    os.exit(1)

  # Command line parsing
  parser = optparse.OptionParser()
  parser.add_option('-p',
                    '--platform',
                    dest='platform',
                    default=None,
                    help='Platform that the locv file was generated on. Must be
                    one of {win32, linux2, macosx}')
  parser.add_option('-s',
                    '--source',
                    dest='src_dir',
                    default=None,
                    help='Path to the source code and symbols')
  parser.add_option('-d', 
                    '--dash_root',
                    dest='dash_root',
                    default=None,
                    help='Root directory for the dashboard')
  parser.add_option('-l',
                    '--lcov',
                    dest='lcov_path',
                    default=None,
                    help='Location of the LCOV file to process')
  (options, args) = parser.parse_args()
  
  if options.platform == None:
    parser.error('Platform not specified')
  if options.lcov_path == None:
    parser.error('lcov file path not specified')
  if options.src_dir == None:
    parser.error('Source directory not specified')
  if options.dash_root == None:
    parser.error('Dashboard root not specified')
  if options.platform == 'win32':
    CleanWin32Lcov(options.lcov_path, options.src_dir)
    percent = GenerateHtml(options.lcov_path, options.dash_root)
    if percent == None:
      # TODO(niranjan): Add logging.
      print 'Failed to generate code coverage'
      os.exit(1)
    else:
      # TODO(niranjan): Do something with the code coverage numbers
      pass
  else:
    print 'Unsupported platform'
    os.exit(1)


if __name__ == '__main__':
  main()

