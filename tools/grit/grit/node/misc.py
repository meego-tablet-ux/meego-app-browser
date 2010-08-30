#!/usr/bin/python2.4
# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''Miscellaneous node types.
'''

import os.path
import sys

from grit.node import base
from grit.node import message

from grit import exception
from grit import constants
from grit import util

import grit.format.rc_header



class IfNode(base.Node):
  '''A node for conditional inclusion of resources.
  '''

  def _IsValidChild(self, child):
    from grit.node import empty
    assert self.parent, '<if> node should never be root.'
    if isinstance(self.parent, empty.IncludesNode):
      from grit.node import include
      return isinstance(child, include.IncludeNode)
    elif isinstance(self.parent, empty.MessagesNode):
      from grit.node import message
      return isinstance(child, message.MessageNode)
    elif isinstance(self.parent, empty.StructuresNode):
      from grit.node import structure
      return isinstance(child, structure.StructureNode)
    elif isinstance(self.parent, empty.OutputsNode):
      from grit.node import io
      return isinstance(child, io.OutputNode)
    else:
      return False

  def MandatoryAttributes(self):
    return ['expr']

  def IsConditionSatisfied(self):
    '''Returns true if and only if the Python expression stored in attribute
    'expr' evaluates to true.
    '''
    return self.EvaluateCondition(self.attrs['expr'])


class ReleaseNode(base.Node):
  '''The <release> element.'''

  def _IsValidChild(self, child):
    from grit.node import empty
    return isinstance(child, (empty.IncludesNode, empty.MessagesNode,
                              empty.StructuresNode, empty.IdentifiersNode))

  def _IsValidAttribute(self, name, value):
    return (
      (name == 'seq' and int(value) <= self.GetRoot().GetCurrentRelease()) or
      name == 'allow_pseudo'
    )

  def MandatoryAttributes(self):
    return ['seq']

  def DefaultAttributes(self):
    return { 'allow_pseudo' : 'true' }

  def GetReleaseNumber():
    '''Returns the sequence number of this release.'''
    return self.attribs['seq']

  def ItemFormatter(self, t):
    if t == 'data_package':
      from grit.format import data_pack
      return data_pack.DataPack()
    else:
      return super(type(self), self).ItemFormatter(t)

class GritNode(base.Node):
  '''The <grit> root element.'''

  def __init__(self):
    base.Node.__init__(self)
    self.output_language = ''
    self.defines = {}

  def _IsValidChild(self, child):
    from grit.node import empty
    return isinstance(child, (ReleaseNode, empty.TranslationsNode,
                              empty.OutputsNode))

  def _IsValidAttribute(self, name, value):
    if name not in ['base_dir', 'source_lang_id',
                    'latest_public_release', 'current_release',
                    'enc_check', 'tc_project']:
      return False
    if name in ['latest_public_release', 'current_release'] and value.strip(
      '0123456789') != '':
      return False
    return True

  def MandatoryAttributes(self):
    return ['latest_public_release', 'current_release']

  def DefaultAttributes(self):
    return {
      'base_dir' : '.',
      'source_lang_id' : 'en',
      'enc_check' : constants.ENCODING_CHECK,
      'tc_project' : 'NEED_TO_SET_tc_project_ATTRIBUTE',
    }

  def EndParsing(self):
    base.Node.EndParsing(self)
    if (int(self.attrs['latest_public_release'])
        > int(self.attrs['current_release'])):
      raise exception.Parsing('latest_public_release cannot have a greater '
                              'value than current_release')

    self.ValidateUniqueIds()

    # Add the encoding check if it's not present (should ensure that it's always
    # present in all .grd files generated by GRIT). If it's present, assert if
    # it's not correct.
    if 'enc_check' not in self.attrs or self.attrs['enc_check'] == '':
      self.attrs['enc_check'] = constants.ENCODING_CHECK
    else:
      assert self.attrs['enc_check'] == constants.ENCODING_CHECK, (
        'Are you sure your .grd file is in the correct encoding (UTF-8)?')

  def ValidateUniqueIds(self):
    '''Validate that 'name' attribute is unique in all nodes in this tree
    except for nodes that are children of <if> nodes.
    '''
    unique_names = {}
    duplicate_names = []
    for node in self:
      if isinstance(node, message.PhNode):
        continue  # PhNode objects have a 'name' attribute which is not an ID

      node_ids = node.GetTextualIds()
      if node_ids:
        for node_id in node_ids:
          if util.SYSTEM_IDENTIFIERS.match(node_id):
            continue  # predefined IDs are sometimes used more than once

          # Don't complain about duplicate IDs if they occur in a node that is
          # inside an <if> node.
          if (node_id in unique_names and node_id not in duplicate_names and
              (not node.parent or not isinstance(node.parent, IfNode))):
            duplicate_names.append(node_id)
          unique_names[node_id] = 1

    if len(duplicate_names):
      raise exception.DuplicateKey(', '.join(duplicate_names))


  def GetCurrentRelease(self):
    '''Returns the current release number.'''
    return int(self.attrs['current_release'])

  def GetLatestPublicRelease(self):
    '''Returns the latest public release number.'''
    return int(self.attrs['latest_public_release'])

  def GetSourceLanguage(self):
    '''Returns the language code of the source language.'''
    return self.attrs['source_lang_id']

  def GetTcProject(self):
    '''Returns the name of this project in the TranslationConsole, or
    'NEED_TO_SET_tc_project_ATTRIBUTE' if it is not defined.'''
    return self.attrs['tc_project']

  def SetOwnDir(self, dir):
    '''Informs the 'grit' element of the directory the file it is in resides.
    This allows it to calculate relative paths from the input file, which is
    what we desire (rather than from the current path).

    Args:
      dir: r'c:\bla'

    Return:
      None
    '''
    assert dir
    self.base_dir = os.path.normpath(os.path.join(dir, self.attrs['base_dir']))

  def GetBaseDir(self):
    '''Returns the base directory, relative to the working directory.  To get
    the base directory as set in the .grd file, use GetOriginalBaseDir()
    '''
    if hasattr(self, 'base_dir'):
      return self.base_dir
    else:
      return self.GetOriginalBaseDir()

  def GetOriginalBaseDir(self):
    '''Returns the base directory, as set in the .grd file.
    '''
    return self.attrs['base_dir']

  def _CollectOutputFiles(self, nodes, output_files):
    '''Recursively filters the list of nodes that may contain other lists
    in <if> nodes, and collects all the nodes that are not enclosed by
    unsatisfied <if> conditionals and not <if> nodes themselves.

    Args:
      nodes: The list of nodes to filter.
      output_files: The list of satisfying nodes.
    '''
    for node in nodes:
      if node.name == 'if':
        if node.IsConditionSatisfied():
          self._CollectOutputFiles(node.children, output_files)
      else:
        output_files.append(node)

  def GetOutputFiles(self):
    '''Returns the list of <output> nodes that are descendants of this node's
    <outputs> child and are not enclosed by unsatisfied <if> conditionals.
    '''
    for child in self.children:
      if child.name == 'outputs':
        output_files = []
        self._CollectOutputFiles(child.children, output_files)
        return output_files
    raise exception.MissingElement()

  def ItemFormatter(self, t):
    if t == 'rc_header':
      from grit.format import rc_header  # import here to avoid circular dep
      return rc_header.TopLevel()
    elif t in ['rc_all', 'rc_translateable', 'rc_nontranslateable']:
      from grit.format import rc  # avoid circular dep
      return rc.TopLevel()
    elif t == 'resource_map_header':
      from grit.format import resource_map
      return resource_map.HeaderTopLevel()
    elif t in ('resource_map_source', 'resource_file_map_source'):
      from grit.format import resource_map
      return resource_map.SourceTopLevel()
    elif t == 'js_map_format':
      from grit.format import js_map_format
      return js_map_format.TopLevel()
    elif t in ['adm', 'plist', 'plist_strings', 'admx', 'adml']:
      from grit.format.policy_templates import template_formatter
      return template_formatter.TemplateFormatter(t)
    else:
      return super(type(self), self).ItemFormatter(t)

  def SetOutputContext(self, output_language, defines):
    self.output_language = output_language
    self.defines = defines

  def SetDefines(self, defines):
    self.defines = defines

  def AssignFirstIds(self, filename_or_stream, first_id_filename):
    '''Assign first ids to each grouping node based on values from
    tools/grit/resource_ids.'''
    # If the input is a stream, then we're probably in a unit test and
    # should skip this step.
    if type(filename_or_stream) not in (str, unicode):
      return

    # By default, we use the the file resources_ids next to grit.py
    # to determine what ids to assign to resources.
    if not first_id_filename:
      first_id_filename = os.path.join(os.path.dirname(
          os.path.abspath(sys.argv[0])), 'resource_ids')

    first_ids = None
    from grit.node import empty
    for node in self.inorder():
      if isinstance(node, empty.GroupingNode):
        # The checkout base directory is 2 directories up from grit.py.
        src_root_dir = os.path.dirname(os.path.dirname(
            os.path.dirname(os.path.abspath(sys.argv[0]))))

        filename = os.path.abspath(filename_or_stream)[
            len(src_root_dir) + 1:]
        filename = filename.replace('\\', '/')
        if not first_ids:
          first_ids = eval(open(first_id_filename).read())

        if node.attrs['first_id'] != '':
          raise Exception("Don't set the first_id attribute, update "
              "%s instead." % first_id_filename)

        try:
          id_list = first_ids[filename][node.name]
        except KeyError, e:
          print '-' * 78
          print 'Resource id not set for %s (%s)!' % (filename, node.name)
          print ('Please update %s to include an entry for %s.  See the '
              'comments in resource_ids for information on why you need to '
              'update that file.' % (first_id_filename, filename))
          print '-' * 78
          raise e

        try:
          node.attrs['first_id'] = str(id_list.pop(0))
        except IndexError, e:
          raise Exception('Please update %s and add a first id for %s (%s).'
              % (first_id_filename, filename, node.name))


class IdentifierNode(base.Node):
  '''A node for specifying identifiers that should appear in the resource
  header file, and be unique amongst all other resource identifiers, but don't
  have any other attributes or reference any resources.
  '''

  def MandatoryAttributes(self):
    return ['name']

  def DefaultAttributes(self):
    return { 'comment' : '', 'id' : '' }

  def ItemFormatter(self, t):
    if t == 'rc_header':
      return grit.format.rc_header.Item()

  def GetId(self):
    '''Returns the id of this identifier if it has one, None otherwise
    '''
    if 'id' in self.attrs:
      return self.attrs['id']
    return None

  # static method
  def Construct(parent, name, id, comment):
    '''Creates a new node which is a child of 'parent', with attributes set
    by parameters of the same name.
    '''
    node = IdentifierNode()
    node.StartParsing('identifier', parent)
    node.HandleAttribute('name', name)
    node.HandleAttribute('id', id)
    node.HandleAttribute('comment', comment)
    node.EndParsing()
    return node
  Construct = staticmethod(Construct)
