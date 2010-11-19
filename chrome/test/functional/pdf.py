#!/usr/bin/python
# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import glob

import pyauto_functional  # Must be imported before pyauto
import pyauto


class PDFTest(pyauto.PyUITest):
  """PDF related tests

  This test runs only on Google Chrome build, not on Chromium.
  """

  def _PerformPDFAction(self, action, tab_index=0, windex=0):
    """Perform an action on a PDF tab.

    Args:
      action: one of "fitToHeight", "fitToWidth", "ZoomIn", "ZoomOut"
      tab_index: tab index  Defaults to 0
      windex: window index.  Defaults to 0
    """
    assert action in ('fitToHeight', 'fitToWidth', 'ZoomIn', 'ZoomOut')
    js = 'document.getElementsByName("plugin")[0].%s()' % action
    # Add an empty string so that there's something to return back
    # (or else it hangs)
    return self.GetDOMValue('%s + ""' % js, 0, tab_index)


  def testPDFRunner(self):
    """Navigate to pdf files and verify that browser doesn't crash"""
    # bail out if not a branded build
    properties = self.GetBrowserInfo()['properties']
    if properties['branding'] != 'Google Chrome':
      return
    pdf_files_path = os.path.join(self.DataDir(), 'plugin', 'pdf')
    pdf_files = glob.glob(os.path.join(pdf_files_path, '*.pdf'))
    for pdf_file in pdf_files:
      # Some pdfs cause known crashes. Exclude them. crbug.com/63549
      if os.path.basename(pdf_file) in ('nullip.pdf', 'sample.pdf'):
        continue
      url = self.GetFileURLForPath(os.path.join(pdf_files_path, pdf_file))
      self.AppendTab(pyauto.GURL(url))
    for tab_index in range(1, len(pdf_files) + 1):
      self.ActivateTab(tab_index)
      self._PerformPDFAction('fitToHeight', tab_index=tab_index)
      self._PerformPDFAction('fitToWidth', tab_index=tab_index)
    # Assert that there is at least 1 browser window.
    self.assertTrue(self.GetBrowserWindowCount(),
                    'Browser crashed, no window is open')


if __name__ == '__main__':
  pyauto_functional.Main()
