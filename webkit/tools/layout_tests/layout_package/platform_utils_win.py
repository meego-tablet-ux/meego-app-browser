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

"""Platform specific utility methods.  This file contains methods that are
specific to running the layout tests on windows.

This file constitutes a complete wrapper for google.platform_utils_win,
implementing or mapping all needed functions from there.  Layout-test scripts
should be able to import only this file (via platform_utils.py), with no need
to fall back to the base functions.
"""

import os
import re
import subprocess

import google.httpd_utils
import google.path_utils
import google.platform_utils_win

import layout_package.path_utils


class PlatformUtility(google.platform_utils_win.PlatformUtility):
  """Overrides base PlatformUtility methods as needed for layout tests."""

  LAYOUTTEST_HTTP_DIR = "LayoutTests/http/tests/"
  PENDING_HTTP_DIR    = "pending/http/tests/"

  def FilenameToUri(self, full_path):
    relative_path = layout_package.path_utils.RelativeTestFilename(full_path)
    port = None
    use_ssl = False

    # LayoutTests/http/tests/ run off port 8000 and ssl/ off 8443
    if relative_path.startswith(self.LAYOUTTEST_HTTP_DIR):
      relative_path = relative_path[len(self.LAYOUTTEST_HTTP_DIR):]
      port = 8000
    # pending/http/tests/ run off port 9000 and ssl/ off 9443
    elif relative_path.startswith(self.PENDING_HTTP_DIR):
      relative_path = relative_path[len(self.PENDING_HTTP_DIR):]
      port = 9000
    # chrome/http/tests run off of port 8081 with the full path
    elif relative_path.find("/http/") >= 0:
      print relative_path
      port = 8081

    # We want to run off of the http server
    if port:
      if relative_path.startswith("ssl/"):
        port += 443
        use_ssl = True
      return google.platform_utils_win.PlatformUtility.FilenameToUri(self,
                                                       relative_path,
                                                       use_http=True,
                                                       use_ssl=use_ssl,
                                                       port=port)

    # Run off file://
    return google.platform_utils_win.PlatformUtility.FilenameToUri(
        self, full_path)

  def KillAllTestShells(self):
    """Kills all instances of the test_shell binary currently running."""
    subprocess.Popen(('taskkill.exe', '/f', '/im',
                      layout_package.path_utils.TestShellBinary()),
                     stdout=subprocess.PIPE,
                     stderr=subprocess.PIPE).wait()

  def _GetVirtualHostConfig(self, document_root, port, ssl=False):
    """Returns a <VirtualHost> directive block for an httpd.conf file.  It will
    listen to 127.0.0.1 on each of the given port.
    """
    cygwin_document_root = google.platform_utils_win.GetCygwinPath(
                           document_root)

    return '\n'.join(('<VirtualHost 127.0.0.1:%s>' % port,
                      'DocumentRoot %s' % cygwin_document_root,
                      ssl and 'SSLEngine On' or '',
                      '</VirtualHost>', ''))

  def GetStartHttpdCommand(self, output_dir, apache2=False):
    """Prepares the config file and output directory to start an httpd server.
    Returns a list of strings containing the server's command line+args.

    Creates the test output directory and generates an httpd.conf (or 
    httpd2.conf for Apache 2 if apache2 is True) file in it that contains 
    the necessary <VirtualHost> directives for running all the http tests. 

    WebKit http tests expect the DocumentRoot to be in LayoutTests/http/tests/,
    but that prevents us from running http tests in chrome/ or pending/.  So we
    run two virtual hosts, one on ports 8000 and 8080 for WebKit, and one on
    port 8081 with a much broader DocumentRoot for everything else.  (Note that
    WebKit http tests that have been modified and are temporarily in pending/
    will still fail, if they expect the DocumentRoot to be located as described
    above.)

    Args:
      output_dir: the path to the test output directory.  It will be created.
      apache2: boolean if true will cause this function to return start
               command for Apache 2.x instead of Apache 1.3.x
    """
    layout_dir = google.platform_utils_win.GetCygwinPath(
        layout_package.path_utils.LayoutDataDir())
    main_document_root = os.path.join(layout_dir, "LayoutTests",
                                      "http", "tests")
    pending_document_root = os.path.join(layout_dir, "pending",
                                         "http", "tests")
    chrome_document_root = layout_dir
    apache_config_dir = google.httpd_utils.ApacheConfigDir(self._base_dir)
    mime_types_path = os.path.join(apache_config_dir, "mime.types")
    
    conf_file_name = "httpd.conf"
    if apache2:
      conf_file_name = "httpd2.conf"
    # Make the test output directory and place the generated httpd.conf in it.
    orig_httpd_conf_path = os.path.join(apache_config_dir, conf_file_name)

    httpd_conf_path = os.path.join(output_dir, conf_file_name)
    google.path_utils.MaybeMakeDirectory(output_dir)
    httpd_conf = open(orig_httpd_conf_path).read()
    httpd_conf = (httpd_conf +
        self._GetVirtualHostConfig(main_document_root, 8000) +
        self._GetVirtualHostConfig(main_document_root, 8080) +
        self._GetVirtualHostConfig(pending_document_root, 9000) +
        self._GetVirtualHostConfig(pending_document_root, 9080) +
        self._GetVirtualHostConfig(chrome_document_root, 8081))
    if apache2:
      httpd_conf += self._GetVirtualHostConfig(main_document_root, 8443,
                                               ssl=True)
      httpd_conf += self._GetVirtualHostConfig(pending_document_root, 9443,
                                               ssl=True)
    f = open(httpd_conf_path, 'wb')
    f.write(httpd_conf)
    f.close()

    return google.platform_utils_win.PlatformUtility.GetStartHttpdCommand(
                                                     self,
                                                     output_dir,
                                                     httpd_conf_path,
                                                     mime_types_path,
                                                     apache2=apache2)
