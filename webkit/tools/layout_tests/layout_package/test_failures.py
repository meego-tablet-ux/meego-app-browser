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

"""Classes for failures that occur during tests."""

import path_utils

class FailureSort(object):
  """A repository for failure sort orders and tool to facilitate sorting."""

  # Each failure class should have an entry in this dictionary. Sort order 1
  # will be sorted first in the list. Failures with the same numeric sort
  # order will be sorted alphabetically by Message().
  SORT_ORDERS = {
    'FailureTextMismatch': 1,
    'FailureSimplifiedTextMismatch': 2,
    'FailureImageHashMismatch': 3,
    'FailureTimeout': 4,
    'FailureCrash': 5,
    'FailureMissingImageHash': 6,
    'FailureMissingImage': 7,
    'FailureMissingResult': 8,
  }

  @staticmethod
  def SortOrder(failure_type):
    """Returns a tuple of the class's numeric sort order and its message."""
    order = FailureSort.SORT_ORDERS.get(failure_type.__name__, -1)
    return (order, failure_type.Message())


class TestFailure(object):
  """Abstract base class that defines the failure interface."""
  @staticmethod
  def Message():
    """Returns a string describing the failure in more detail."""
    raise NotImplemented

  def ResultHtmlOutput(self, filename):
    """Returns an HTML string to be included on the results.html page."""
    raise NotImplemented

  def ShouldKillTestShell(self):
    """Returns True if we should kill the test shell before the next test."""
    return False


class FailureWithType(TestFailure):
  """Base class that produces standard HTML output based on the test type.

  Subclasses may commonly choose to override the ResultHtmlOutput, but still
  use the standard OutputLinks.
  """
  def __init__(self, test_type):
    TestFailure.__init__(self)
    self._test_type = test_type

  # Filename suffixes used by ResultHtmlOutput.
  OUT_FILENAMES = []

  def OutputLinks(self, filename, out_names):
    """Returns a string holding all applicable output file links.

    Args:
      filename: the test filename, used to construct the result file names
      out_names: list of filename suffixes for the files. If three or more
          suffixes are in the list, they should be [actual, expected, diff].
          Two suffixes should be [actual, expected], and a single item is the
          [actual] filename suffix.  If out_names is empty, returns the empty
          string.
    """
    links = ['']
    uris = [self._test_type.RelativeOutputFilename(filename, fn)
            for fn in out_names]
    if len(uris) > 1:
      links.append("<a href='%s'>expected</a>" % uris[1])
    if len(uris) > 0:
      links.append("<a href='%s'>actual</a>" % uris[0])
    if len(uris) > 2:
      links.append("<a href='%s'>diff</a>" % uris[2])
    return ' '.join(links)

  def ResultHtmlOutput(self, filename):
    return self.Message() + self.OutputLinks(filename, self.OUT_FILENAMES)


class FailureTimeout(TestFailure):
  """Test timed out.  We also want to restart the test shell if this
  happens."""
  @staticmethod
  def Message():
    return "Test timed out"

  def ResultHtmlOutput(self, filename):
    return "<strong>%s</strong>" % self.Message()

  def ShouldKillTestShell(self):
    return True


class FailureCrash(TestFailure):
  """Test shell crashed."""
  @staticmethod
  def Message():
    return "Test shell crashed"

  def ResultHtmlOutput(self, filename):
    # TODO(tc): create a link to the minidump file
    return "<strong>%s</strong>" % self.Message()

  def ShouldKillTestShell(self):
    return True


class FailureMissingResult(FailureWithType):
  """Expected result was missing."""
  OUT_FILENAMES = ["-actual-win.txt"]

  @staticmethod
  def Message():
    return "No expected results found"

  def ResultHtmlOutput(self, filename):
    return ("<strong>%s</strong>" % self.Message() +
            self.OutputLinks(filename, self.OUT_FILENAMES))


class FailureTextMismatch(FailureWithType):
  """Text diff output failed."""
  # Filename suffixes used by ResultHtmlOutput.
  OUT_FILENAMES = ["-actual-win.txt", "-expected.txt", "-diff-win.txt"]

  @staticmethod
  def Message():
    return "Text diff mismatch"


class FailureSimplifiedTextMismatch(FailureTextMismatch):
  """Simplified text diff output failed.

  The results.html output format is basically the same as regular diff
  failures (links to expected, actual and diff text files) so we share code
  with the FailureTextMismatch class.
  """

  OUT_FILENAMES = ["-simp-actual-win.txt", "-simp-expected.txt",
                   "-simp-diff-win.txt"]

  @staticmethod
  def Message():
    return "Simplified text diff mismatch"


class FailureMissingImageHash(FailureWithType):
  """Actual result hash was missing."""
  # Chrome doesn't know to display a .checksum file as text, so don't bother
  # putting in a link to the actual result.
  OUT_FILENAMES = []

  @staticmethod
  def Message():
    return "No expected image hash found"

  def ResultHtmlOutput(self, filename):
    return "<strong>%s</strong>" % self.Message()


class FailureMissingImage(FailureWithType):
  """Actual result image was missing."""
  OUT_FILENAMES = ["-actual-win.png"]

  @staticmethod
  def Message():
    return "No expected image found"

  def ResultHtmlOutput(self, filename):
    return ("<strong>%s</strong>" % self.Message() +
            self.OutputLinks(filename, self.OUT_FILENAMES))


class FailureImageHashMismatch(FailureWithType):
  """Image hashes didn't match."""
  OUT_FILENAMES = ["-actual-win.png", "-expected.png"]

  @staticmethod
  def Message():
    # We call this a simple image mismatch to avoid confusion, since we link
    # to the PNGs rather than the checksums.
    return "Image mismatch"
