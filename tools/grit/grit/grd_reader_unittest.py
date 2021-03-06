#!/usr/bin/python2.4
# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''Unit tests for grd_reader package'''

import os
import sys
if __name__ == '__main__':
  sys.path.append(os.path.join(os.path.dirname(sys.argv[0]), '..'))

import unittest
import StringIO

from grit import grd_reader
from grit import constants
from grit import util
from grit.node import empty


class GrdReaderUnittest(unittest.TestCase):
  def testParsingAndXmlOutput(self):
    input = u'''<?xml version="1.0" encoding="UTF-8"?>
<grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
  <release seq="3">
    <includes>
      <include name="ID_LOGO" file="images/logo.gif" type="gif" />
    </includes>
    <messages>
      <if expr="True">
        <message name="IDS_GREETING" desc="Printed to greet the currently logged in user">
          Hello <ph name="USERNAME">%s<ex>Joi</ex></ph>, how are you doing today?
        </message>
      </if>
    </messages>
    <structures>
      <structure name="IDD_NARROW_DIALOG" file="rc_files/dialogs.rc" type="dialog">
        <skeleton variant_of_revision="3" expr="lang == 'fr-FR'" file="bla.rc" />
      </structure>
      <structure name="VS_VERSION_INFO" file="rc_files/version.rc" type="version" />
    </structures>
  </release>
  <translations>
    <file lang="nl" path="nl_translations.xtb" />
  </translations>
  <outputs>
    <output type="rc_header" filename="resource.h" />
    <output lang="en-US" type="rc_all" filename="resource.rc" />
  </outputs>
</grit>'''
    pseudo_file = StringIO.StringIO(input)
    tree = grd_reader.Parse(pseudo_file, '.')
    output = unicode(tree)
    # All but first two lines are the same (sans enc_check)
    self.failUnless('\n'.join(input.split('\n')[2:]) ==
                    '\n'.join(output.split('\n')[2:]))
    self.failUnless(tree.GetNodeById('IDS_GREETING'))


  def testStopAfter(self):
    input = u'''<?xml version="1.0" encoding="UTF-8"?>
<grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
  <outputs>
    <output filename="resource.h" type="rc_header" />
    <output filename="resource.rc" lang="en-US" type="rc_all" />
  </outputs>
  <release seq="3">
    <includes>
      <include type="gif" name="ID_LOGO" file="images/logo.gif"/>
    </includes>
  </release>
</grit>'''
    pseudo_file = util.WrapInputStream(StringIO.StringIO(input))
    tree = grd_reader.Parse(pseudo_file, '.', stop_after='outputs')
    # only an <outputs> child
    self.failUnless(len(tree.children) == 1)
    self.failUnless(tree.children[0].name == 'outputs')

  def testLongLinesWithComments(self):
    input = u'''<?xml version="1.0" encoding="UTF-8"?>
<grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
  <release seq="3">
    <messages>
      <message name="IDS_GREETING" desc="Printed to greet the currently logged in user">
        This is a very long line with no linebreaks yes yes it stretches on <!--
        -->and on <!--
        -->and on!
      </message>
    </messages>
  </release>
</grit>'''
    pseudo_file = StringIO.StringIO(input)
    tree = grd_reader.Parse(pseudo_file, '.')

    greeting = tree.GetNodeById('IDS_GREETING')
    self.failUnless(greeting.GetCliques()[0].GetMessage().GetRealContent() ==
                    'This is a very long line with no linebreaks yes yes it '
                    'stretches on and on and on!')

  def testAssignFirstIds(self):
    input = u'''<?xml version="1.0" encoding="UTF-8"?>
<grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
  <release seq="3">
    <messages>
      <message name="IDS_TEST" desc="test">
        test
      </message>
    </messages>
  </release>
</grit>'''
    pseudo_file = StringIO.StringIO(input)
    root = grd_reader.Parse(pseudo_file, '.')
    grit_root_dir = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                                 '..')
    root.AssignFirstIds(
        os.path.join(grit_root_dir, "../../chrome/app/generated_resources.grd"),
        None, {})
    messages_node = root.children[0].children[0]
    self.failUnless(isinstance(messages_node, empty.MessagesNode))
    self.failUnless(messages_node.attrs["first_id"] !=
        empty.MessagesNode().DefaultAttributes()["first_id"])

  def testAssignFirstIdsMultipleMessages(self):
    """If there are multiple messages sections, the resource_ids file
    needs to list multiple first_id values."""
    input = u'''<?xml version="1.0" encoding="UTF-8"?>
<grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
  <release seq="3">
    <messages>
      <message name="IDS_TEST" desc="test">
        test
      </message>
    </messages>
    <messages>
      <message name="IDS_TEST2" desc="test">
        test2
      </message>
    </messages>
  </release>
</grit>'''
    pseudo_file = StringIO.StringIO(input)
    root = grd_reader.Parse(pseudo_file, '.')
    grit_root_dir = os.path.join(os.path.abspath(os.path.dirname(__file__)),
                                 '..')
    root.AssignFirstIds(
        os.path.join(grit_root_dir, "../../test.grd"),
        os.path.join(grit_root_dir, "grit/test/data/resource_ids"),
        {})
    messages_node = root.children[0].children[0]
    self.assertTrue(isinstance(messages_node, empty.MessagesNode))
    self.assertEqual('100', messages_node.attrs["first_id"])
    messages_node = root.children[0].children[1]
    self.assertTrue(isinstance(messages_node, empty.MessagesNode))
    self.assertEqual('10000', messages_node.attrs["first_id"])

  def testUseNameForIdAndPpIfdef(self):
    input = u'''<?xml version="1.0" encoding="UTF-8"?>
<grit latest_public_release="2" source_lang_id="en-US" current_release="3" base_dir=".">
  <release seq="3">
    <messages>
      <if expr="pp_ifdef('hello')">
        <message name="IDS_HELLO" use_name_for_id="true">
          Hello!
        </message>
      </if>
    </messages>
  </release>
</grit>'''
    pseudo_file = StringIO.StringIO(input)
    root = grd_reader.Parse(pseudo_file, '.', defines={'hello': '1'})

    # Check if the ID is set to the name. In the past, there was a bug
    # that caused the ID to be a generated number.
    hello = root.GetNodeById('IDS_HELLO')
    self.failUnless(hello.GetCliques()[0].GetId() == 'IDS_HELLO')

if __name__ == '__main__':
  unittest.main()
