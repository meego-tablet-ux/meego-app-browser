#!/usr/bin/python

# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Usage: make_ib_classes.py output.mm input.xib [...]
#
# Generates an Objective-C++ file at output.mm referencing each class described
# in input.xib.
#
# This script is useful when building an application containing .nib or .xib
# files that reference Objective-C classes that may not be referenced by other
# code in the application.  The intended use case is when the .nib and .xib
# files refer to classes that are built into a static library that gets linked
# into the main executable.  If nothing in the main executable references those
# classes, the linker will not include them in its output (without -all_load or
# -ObjC).  Using this script, references to such classes are created, such that
# if output.mm is compiled into the application itself, it will provide the
# class references and cause the linker to bring the required code into the
# executable.
#
# If your application is structured in the above way, and you're plagued with
# messages like:
#   app[12345:101] Unknown class `MyApp' in nib file, using `NSObject' instead.
# then this script may be right for you.


import errno
import os
import os.path
import re
import subprocess
import sys


# Patterns used by ListIBClasses

# A pattern that matches the line preceding a class name.
_class_re = re.compile('<key>class</key>$')

# A pattern that matches the line with a class name; match group 1 should be
# the class name.
_class_name_re = re.compile('<string>(.*)</string>$')

# A pattern that matches class names to exclude from the output.  This includes
# various Cocoa classes.
_forbidden_class_re = re.compile('^(NS|IB|FirstResponder$|WebView$)')

def ListIBClasses(ib_path, class_names=None):
  """Returns a list of class names referenced by ib_path.

  ib_path is a path to an Interface Builder document.  It may be a .nib or a
  .xib.

  This function calls "ibtool --classes" to get the list of class names.
  ibtool's output is in XML plist format.  Rather than doing proper structured
  plist scanning, this function relies on the fact that plists are serialized
  to XML in a consistent way, and simply takes the string value names of any
  dictionary key named "class" as class names.

  class_names may be specified as an existing list to use.  This is helpful
  when this function will be called several times for multiple nib/xib files.
  """
  if class_names == None:
    class_names = []

  # When running within an Xcode build, use the tools from that Xcode
  # installation.
  developer_tools_dir = os.getenv('DEVELOPER_BIN_DIR', '/usr/bin')
  ibtool_path = os.path.join(developer_tools_dir, 'ibtool')
  ibtool_command = [ibtool_path, '--classes', ib_path]

  ibtool = subprocess.Popen(ibtool_command, stdout=subprocess.PIPE)

  ibtool_output = ibtool.communicate()[0]

  ibtool_rv = ibtool.returncode
  assert ibtool_rv == 0

  # Loop through the output, looking for "class" keys.  The string value on
  # any line following a "class" key is taken as a class name.
  is_class_name = False
  for line in ibtool_output.splitlines():
    if is_class_name:
      class_name = _class_name_re.search(line).group(1)
      is_class_name = False
      if not class_name in class_names and \
         not _forbidden_class_re.search(class_name):
        class_names.append(class_name)
    elif _class_re.search(line):
      is_class_name = True

  return class_names


def main(args):
  assert len(args) > 2
  (script_path, output_path) = args[0:2]
  assert output_path.endswith('.mm')
  input_paths = args[2:]

  try:
    os.unlink(output_path)
  except OSError, e:
    if e.errno != errno.ENOENT:
      raise

  class_names = []

  # Get the class names for all desired files.
  for input_path in input_paths:
    ListIBClasses(input_path, class_names)

  class_names.sort()

  # Write the requested output file.  Each class is referenced simply by
  # calling its +class function.  In order to do this, each class needs a
  # bogus @interface to tell the compiler that it's an NSObject subclass.
  # #import NSObject.h to get the definition of NSObject without bringing in
  # other headers that might provide real declarations.

  output_file = open(output_path, 'w')
  print >>output_file, \
"""// This file was generated by %s.  Do not edit.

#import <Foundation/NSObject.h>
""" % os.path.basename(script_path)

  for class_name in class_names:
    print >>output_file, '@interface %s : NSObject\n@end' % class_name

  print >>output_file, '\nnamespace {\n\nvoid IBClasses() {'

  for class_name in class_names:
    print >>output_file, '  [%s class];' % class_name

  print >>output_file, '}\n\n}  // namespace'

  output_file.close()

  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv))
