#!/usr/bin/python
# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Update third_party/WebKit using git.

Under the assumption third_party/WebKit is a clone of git.webkit.org,
we can use git commands to make it match the version requested by DEPS.

See http://code.google.com/p/chromium/wiki/UsingWebKitGit for details on
how to use this.
"""

import os
import subprocess
import sys

# The name of the magic branch that lets us know that DEPS is managing
# the update cycle.
MAGIC_GCLIENT_BRANCH = 'refs/heads/gclient'

def RunGit(command):
  """Run a git subcommand, returning its output."""
  proc = subprocess.Popen(['git'] + command, stdout=subprocess.PIPE)
  return proc.communicate()[0].strip()

def GetWebKitRev():
  """Extract the 'webkit_revision' variable out of DEPS."""
  locals = {'Var': lambda _: ''}
  execfile('DEPS', {}, locals)
  return locals['vars']['webkit_revision']

def FindSVNRev(rev):
  """Map an SVN revision to a git hash.
  Like 'git svn find-rev' but without the git-svn bits."""
  # We find r123 by grepping for a line with "git-svn-id: blahblahblah@123".
  return RunGit(['rev-list', '-n', '1', '--grep=^git-svn-id: .*trunk@%s ' % rev,
                 'origin'])

def UpdateGClientBranch(webkit_rev):
  """Update the magic gclient branch to point at |webkit_rev|.

  Returns: true if the branch didn't need changes."""
  target = FindSVNRev(webkit_rev)
  if not target:
    print "r%s not available; fetching." % webkit_rev
    subprocess.check_call(['git', 'fetch'])
    target = FindSVNRev(webkit_rev)
  if not target:
    print "ERROR: Couldn't map r%s to a git revision." % webkit_rev
    sys.exit(1)

  current = RunGit(['show-ref', '--hash', MAGIC_GCLIENT_BRANCH])
  if current == target:
    return False  # No change necessary.

  subprocess.check_call(['git', 'update-ref', '-m', 'gclient sync',
                         MAGIC_GCLIENT_BRANCH, target])
  return True

def UpdateCurrentCheckoutIfAppropriate():
  """Reset the current gclient branch if that's what we have checked out."""
  branch = RunGit(['symbolic-ref', '-q', 'HEAD'])
  if branch != MAGIC_GCLIENT_BRANCH:
    print ("third_party/WebKit has some other branch ('%s') checked out." %
           branch)
    print "Run 'git checkout gclient' to put this under control of gclient."
    return

  if subprocess.call(['git', 'diff-index', '--exit-code', '--shortstat',
	              'HEAD']):
    print "Resetting tree state to new revision."
    subprocess.check_call(['git', 'reset', '--hard'])

def main():
  if not os.path.exists('third_party/WebKit/.git'):
    print "ERROR: third_party/WebKit appears to not be under git control."
    print "See http://code.google.com/p/chromium/wiki/UsingWebKitGit for"
    print "setup instructions."
    return

  webkit_rev = GetWebKitRev()
  print 'Desired revision: r%s.' % webkit_rev
  os.chdir('third_party/WebKit')
  changed = UpdateGClientBranch(webkit_rev)
  if changed:
    UpdateCurrentCheckoutIfAppropriate()
  else:
    print "Already on correct revision."

if __name__ == '__main__':
  main()
