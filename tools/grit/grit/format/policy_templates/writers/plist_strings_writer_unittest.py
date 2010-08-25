#!/usr/bin/python2.4
# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''Unit tests for grit.format.policy_templates.writers.plist_strings_writer'''


import os
import sys
if __name__ == '__main__':
  sys.path.append(os.path.join(os.path.dirname(sys.argv[0]), '../../../..'))

import tempfile
import unittest
import StringIO

from grit.format.policy_templates.writers import writer_unittest_common
from grit import grd_reader
from grit import util
from grit.tool import build


class PListStringsWriterUnittest(writer_unittest_common.WriterUnittestCommon):
  '''Unit tests for PListWriter.'''

  def testEmpty(self):
    # Test PListWriter in case of empty polices.
    grd = self.prepareTest('''
      {
        'policy_groups': [],
        'placeholders': [],
      }''', '''
    <grit base_dir="." latest_public_release="0" current_release="1" source_lang_id="en">
      <release seq="1">
        <structures>
          <structure name="IDD_POLICY_SOURCE_FILE" file="%s" type="policy_template_metafile" />
        </structures>
        <messages>
          <message name="IDS_POLICY_MAC_CHROME_PREFERENCES">$1 preferen"ces</message>
        </messages>
      </release>
      </grit>
      ''' )

    self.CompareResult(
        grd,
        'fr',
        {'_chromium': '1', 'mac_bundle_id': 'com.example.Test'},
        'plist_strings',
        'en',
        '''Chromium.pfm_title = "Chromium";
Chromium.pfm_description = "Chromium preferen\\"ces";
        ''')

  def testMainPolicy(self):
    # Tests a policy group with a single policy of type 'main'.
    grd = self.prepareTest('''
      {
        'policy_groups': [
          {
            'name': 'MainGroup',
            'policies': [{
              'name': 'MainPolicy',
              'type': 'main',
            }],
          },
        ],
        'placeholders': [],
      }''', '''
    <grit base_dir="." latest_public_release="0" current_release="1" source_lang_id="en">
      <release seq="1">
        <structures>
          <structure name="IDD_POLICY_SOURCE_FILE" file="%s" type="policy_template_metafile" />
        </structures>
        <messages>
          <message name="IDS_POLICY_GROUP_MAINGROUP_CAPTION">Caption of main.</message>
          <message name="IDS_POLICY_GROUP_MAINGROUP_DESC">Title of main.</message>
          <message name="IDS_POLICY_MAC_CHROME_PREFERENCES">Preferences of $1</message>
        </messages>
      </release>
      </grit>
      ''' )
    self.CompareResult(
        grd,
        'fr',
        {'_google_chrome' : '1', 'mac_bundle_id': 'com.example.Test'},
        'plist_strings',
        'en',
        '''Google Chrome.pfm_title = "Google Chrome";
Google Chrome.pfm_description = "Preferences of Google Chrome";
MainPolicy.pfm_title = "Caption of main.";
MainPolicy.pfm_description = "Title of main.";
''')

  def testStringPolicy(self):
    # Tests a policy group with a single policy of type 'string'. Also test
    # inheriting group description to policy description.
    grd = self.prepareTest('''
      {
        'policy_groups': [
          {
            'name': 'StringGroup',
            'policies': [{
              'name': 'StringPolicy',
              'type': 'string',
            }],
          },
        ],
        'placeholders': [],
      }''', '''
    <grit base_dir="." latest_public_release="0" current_release="1" source_lang_id="en">
      <release seq="1">
        <structures>
          <structure name="IDD_POLICY_SOURCE_FILE" file="%s" type="policy_template_metafile" />
        </structures>
        <messages>
          <message name="IDS_POLICY_GROUP_STRINGGROUP_CAPTION">Caption of group.</message>
          <message name="IDS_POLICY_GROUP_STRINGGROUP_DESC">Description of group.
With a newline.</message>
          <message name="IDS_POLICY_STRINGPOLICY_CAPTION">Caption of policy.</message>
          <message name="IDS_POLICY_MAC_CHROME_PREFERENCES">Preferences Of $1</message>
        </messages>
      </release>
      </grit>
      ''' )
    self.CompareResult(
        grd,
        'fr',
        {'_chromium' : '1', 'mac_bundle_id': 'com.example.Test'},
        'plist_strings',
        'en',
        '''Chromium.pfm_title = "Chromium";
Chromium.pfm_description = "Preferences Of Chromium";
StringPolicy.pfm_title = "Caption of policy.";
StringPolicy.pfm_description = "Description of group.\\nWith a newline.";
        ''')

  def testEnumPolicy(self):
    # Tests a policy group with a single policy of type 'enum'.
    grd = self.prepareTest('''
      {
        'policy_groups': [
          {
            'name': 'EnumGroup',
            'policies': [{
              'name': 'EnumPolicy',
              'type': 'enum',
              'items': [
                {'name': 'ProxyServerDisabled', 'value': '0'},
                {'name': 'ProxyServerAutoDetect', 'value': '1'},
              ]
            }],
          },
        ],
        'placeholders': [],
      }''', '''
    <grit base_dir="." latest_public_release="0" current_release="1" source_lang_id="en">
      <release seq="1">
        <structures>
          <structure name="IDD_POLICY_SOURCE_FILE" file="%s" type="policy_template_metafile" />
        </structures>
        <messages>
          <message name="IDS_POLICY_GROUP_ENUMGROUP_CAPTION">Caption of group.</message>
          <message name="IDS_POLICY_GROUP_ENUMGROUP_DESC">Description of group.</message>
          <message name="IDS_POLICY_ENUMPOLICY_CAPTION">Caption of policy.</message>
          <message name="IDS_POLICY_ENUMPOLICY_DESC">Description of policy.</message>
          <message name="IDS_POLICY_ENUM_PROXYSERVERDISABLED_CAPTION">Option1</message>
          <message name="IDS_POLICY_ENUM_PROXYSERVERAUTODETECT_CAPTION">Option2</message>
          <message name="IDS_POLICY_MAC_CHROME_PREFERENCES">$1 preferences</message>
        </messages>
      </release>
      </grit>
      ''' )
    self.CompareResult(
        grd,
        'fr',
        {'_google_chrome': '1', 'mac_bundle_id': 'com.example.Test2'},
        'plist_strings',
        'en',
        '''Google Chrome.pfm_title = "Google Chrome";
Google Chrome.pfm_description = "Google Chrome preferences";
EnumPolicy.pfm_title = "Caption of policy.";
EnumPolicy.pfm_description = "0 - Option1\\n1 - Option2\\nDescription of policy.";
        ''')


if __name__ == '__main__':
  unittest.main()

