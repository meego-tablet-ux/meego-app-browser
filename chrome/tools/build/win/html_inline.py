#!/usr/bin/python
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

"""Flattens a HTML file by inlining its external resources.

This is a small script that takes a HTML file, looks for src attributes
and inlines the specified file, producing one HTML file with no external
dependencies.

This does not inline CSS styles, nor does it inline anything referenced
from an inlined file.
"""

import re
import sys
import base64
import mimetypes
from os import path

def ReadFile(input_filename):
  """Helper function that returns input_filename as a string.
  
  Args:
    input_filename: name of file to be read
  
  Returns:
    string
  """
  f = open(input_filename, 'rb')
  file_contents = f.read()
  f.close()
  return file_contents

def SrcInline(src_match, base_path):
  """regex replace function.

  Takes a regex match for src="filename", attempts to read the file 
  at 'filename' and returns the src attribute with the file inlined
  as a data URI

  Args:
    src_match: regex match object with 'filename' named capturing group
    base_path: path that to look for files in

  Returns:
    string
  """
  filename = src_match.group('filename')

  if filename.find(':') != -1:
    # filename is probably a URL, which we don't want to bother inlining
    return src_match.group(0)

  filepath = path.join(base_path, filename)    
  mimetype = mimetypes.guess_type(filename)[0] or 'text/plain'
  inline_data = base64.standard_b64encode(ReadFile(filepath))

  prefix = src_match.string[src_match.start():src_match.start('filename')-1]
  return "%s\"data:%s;base64,%s\"" % (prefix, mimetype, inline_data)
  
def InlineFile(input_filename, output_filename):
  """Inlines the resources in a specified file.
  
  Reads input_filename, finds all the src attributes and attempts to
  inline the files they are referring to, then writes the result
  to output_filename.
  
  Args:
    input_filename: name of file to read in
    output_filename: name of file to be written to
  """
  print "inlining %s to %s" % (input_filename, output_filename)
  input_filepath = path.dirname(input_filename)  
 
  def SrcReplace(src_match):
    """Helper function to provide SrcInline with the base file path"""
    return SrcInline(src_match, input_filepath)
 
  # TODO(glen): Make this regex not match src="" text that is not inside a tag
  flat_text = re.sub('src="(?P<filename>[^"\']*)"',
                     SrcReplace,
                     ReadFile(input_filename))

  # TODO(glen): Make this regex not match url('') that is not inside a style
  flat_text = re.sub('background:[ ]*url\(\'(?P<filename>[^"\']*)\'',
                    SrcReplace,
                    flat_text)

  out_file = open(output_filename, 'wb')
  out_file.writelines(flat_text)
  out_file.close()

def main():
  if len(sys.argv) <= 2:
    print "Flattens a HTML file by inlining its external resources.\n"
    print "html_inline.py inputfile outputfile"
  else:
    InlineFile(sys.argv[1], sys.argv[2])

if __name__ == '__main__':
  main()
