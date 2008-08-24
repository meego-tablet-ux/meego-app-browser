#!/usr/bin/python2.4
# Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

'''The 'grit resize' tool.
'''

import getopt
import os
import types

from grit.tool import interface
from grit.tool import build
from grit import grd_reader
from grit import pseudo
from grit import util

from grit.node import include
from grit.node import structure
from grit.node import message

from grit.format import rc_header


# Template for the .vcproj file, with a couple of [[REPLACEABLE]] parts.
PROJECT_TEMPLATE = '''\
<?xml version="1.0" encoding="Windows-1252"?>
<VisualStudioProject
	ProjectType="Visual C++"
	Version="7.10"
	Name="[[DIALOG_NAME]]"
	ProjectGUID="[[PROJECT_GUID]]"
	Keyword="Win32Proj">
	<Platforms>
		<Platform
			Name="Win32"/>
	</Platforms>
	<Configurations>
		<Configuration
			Name="Debug|Win32"
			OutputDirectory="Debug"
			IntermediateDirectory="Debug"
			ConfigurationType="1"
			CharacterSet="2">
		</Configuration>
	</Configurations>
	<References>
	</References>
	<Files>
		<Filter
			Name="Resource Files"
			Filter="rc;ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe;resx"
			UniqueIdentifier="{67DA6AB6-F800-4c08-8B7A-83BB121AAD01}">
			<File
				RelativePath=".\[[DIALOG_NAME]].rc">
			</File>
		</Filter>
	</Files>
	<Globals>
	</Globals>
</VisualStudioProject>'''


# Template for the .rc file with a couple of [[REPLACEABLE]] parts.
# TODO(joi) Improve this (and the resource.h template) to allow saving and then
# reopening of the RC file in Visual Studio.  Currently you can only open it
# once and change it, then after you close it you won't be able to reopen it.
RC_TEMPLATE = '''\
// Copyright (c) Google Inc. 2005
// All rights reserved.
// This file is automatically generated by GRIT and intended for editing
// the layout of the dialogs contained in it.  Do not edit anything but the
// dialogs.  Any changes made to translateable portions of the dialogs will
// be ignored by GRIT.

#include "resource.h"
#include <winres.h>
#include <winresrc.h>

LANGUAGE LANG_NEUTRAL, SUBLANG_NEUTRAL

#pragma code_page([[CODEPAGE_NUM]])

[[INCLUDES]]

[[DIALOGS]]
'''


# Template for the resource.h file with a couple of [[REPLACEABLE]] parts.
HEADER_TEMPLATE = '''\
// Copyright (c) Google Inc. 2005
// All rights reserved.
// This file is automatically generated by GRIT.  Do not edit.

#pragma once

// Edit commands
#define ID_EDIT_CLEAR                   0xE120
#define ID_EDIT_CLEAR_ALL               0xE121
#define ID_EDIT_COPY                    0xE122
#define ID_EDIT_CUT                     0xE123
#define ID_EDIT_FIND                    0xE124
#define ID_EDIT_PASTE                   0xE125
#define ID_EDIT_PASTE_LINK              0xE126
#define ID_EDIT_PASTE_SPECIAL           0xE127
#define ID_EDIT_REPEAT                  0xE128
#define ID_EDIT_REPLACE                 0xE129
#define ID_EDIT_SELECT_ALL              0xE12A
#define ID_EDIT_UNDO                    0xE12B
#define ID_EDIT_REDO                    0xE12C


[[DEFINES]]
'''


class ResizeDialog(interface.Tool):
  '''Generates an RC file, header and Visual Studio project that you can use
with Visual Studio's GUI resource editor to modify the layout of dialogs for
the language of your choice.  You then use the RC file, after you resize the
dialog, for the language or languages of your choice, using the <skeleton> child
of the <structure> node for the dialog.  The translateable bits of the dialog
will be ignored when you use the <skeleton> node (GRIT will instead use the
translateable bits from the original dialog) but the layout changes you make
will be used.  Note that your layout changes must preserve the order of the
translateable elements in the RC file.

Usage: grit resize [-f BASEFOLDER] [-l LANG] [-e RCENCODING] DIALOGID*

Arguments:
  DIALOGID        The 'name' attribute of a dialog to output for resizing.  Zero
                  or more of these parameters can be used.  If none are
                  specified, all dialogs from the input .grd file are output.

Options:

  -f BASEFOLDER   The project will be created in a subfolder of BASEFOLDER.
                  The name of the subfolder will be the first DIALOGID you
                  specify.  Defaults to '.'

  -l LANG         Specifies that the RC file should contain a dialog translated
                  into the language LANG.  The default is a cp1252-representable
                  pseudotranslation, because Visual Studio's GUI RC editor only
                  supports single-byte encodings.

  -c CODEPAGE     Code page number to indicate to the RC compiler the encoding
                  of the RC file, default is something reasonable for the
                  language you selected (but this does not work for every single
                  language).  See details on codepages below.  NOTE that you do
                  not need to specify the codepage unless the tool complains
                  that it's not sure which codepage to use.  See the following
                  page for codepage numbers supported by Windows:
                  http://www.microsoft.com/globaldev/reference/wincp.mspx

  -D NAME[=VAL]   Specify a C-preprocessor-like define NAME with optional
                  value VAL (defaults to 1) which will be used to control
                  conditional inclusion of resources.


IMPORTANT NOTE:  For now, the tool outputs a UTF-8 encoded file for any language
that can not be represented in cp1252 (i.e. anything other than Western
European languages).  You will need to open this file in a text editor and
save it using the codepage indicated in the #pragma code_page(XXXX) command
near the top of the file, before you open it in Visual Studio.

'''

  # TODO(joi) It would be cool to have this tool note the Perforce revision
  # of the original RC file somewhere, such that the <skeleton> node could warn
  # if the original RC file gets updated without the skeleton file being updated.
  
  # TODO(joi) Would be cool to have option to add the files to Perforce

  def __init__(self):
    self.lang = pseudo.PSEUDO_LANG
    self.defines = {}
    self.base_folder = '.'
    self.codepage_number = 1252
    self.codepage_number_specified_explicitly = False
  
  def SetLanguage(self, lang):
    '''Sets the language code to output things in.
    '''
    self.lang = lang
    if not self.codepage_number_specified_explicitly:
      self.codepage_number = util.LanguageToCodepage(lang)

  def GetEncoding(self):
    if self.codepage_number == 1200:
      return 'utf_16'
    if self.codepage_number == 65001:
      return 'utf_8'
    return 'cp%d' % self.codepage_number

  def ShortDescription(self):
    return 'Generate a file where you can resize a given dialog.'
  
  def Run(self, opts, args):
    self.SetOptions(opts)
    
    own_opts, args = getopt.getopt(args, 'l:f:c:D:')
    for key, val in own_opts:
      if key == '-l':
        self.SetLanguage(val)
      if key == '-f':
        self.base_folder = val
      if key == '-c':
        self.codepage_number = int(val)
        self.codepage_number_specified_explicitly = True
      if key == '-D':
        name, val = build.ParseDefine(val)
        self.defines[name] = val
    
    res_tree = grd_reader.Parse(opts.input, debug=opts.extra_verbose)
    res_tree.OnlyTheseTranslations([self.lang])
    res_tree.RunGatherers(True)
    
    # Dialog IDs are either explicitly listed, or we output all dialogs from the
    # .grd file
    dialog_ids = args
    if not len(dialog_ids):
      for node in res_tree:
        if node.name == 'structure' and node.attrs['type'] == 'dialog':
          dialog_ids.append(node.attrs['name'])
    
    self.Process(res_tree, dialog_ids)
    
  def Process(self, grd, dialog_ids):
    '''Outputs an RC file and header file for the dialog 'dialog_id' stored in
    resource tree 'grd', to self.base_folder, as discussed in this class's
    documentation.
    
    Arguments:
      grd: grd = grd_reader.Parse(...); grd.RunGatherers()
      dialog_ids: ['IDD_MYDIALOG', 'IDD_OTHERDIALOG']
    '''
    grd.SetOutputContext(self.lang, self.defines)
    
    project_name = dialog_ids[0]
    
    dir_path = os.path.join(self.base_folder, project_name)
    if not os.path.isdir(dir_path):
      os.mkdir(dir_path)
    
    # If this fails then we're not on Windows (or you don't have the required
    # win32all Python libraries installed), so what are you doing mucking
    # about with RC files anyway? :)
    import pythoncom
    
    # Create the .vcproj file
    project_text = PROJECT_TEMPLATE.replace(
      '[[PROJECT_GUID]]', str(pythoncom.CreateGuid())
      ).replace('[[DIALOG_NAME]]', project_name)
    fname = os.path.join(dir_path, '%s.vcproj' % project_name)
    self.WriteFile(fname, project_text)
    print "Wrote %s" % fname
    
    # Create the .rc file
    # Output all <include> nodes since the dialogs might depend on them (e.g.
    # for icons and bitmaps).
    include_items = []
    for node in grd:
      if isinstance(node, include.IncludeNode):
        formatter = node.ItemFormatter('rc_all')
        if formatter:
          include_items.append(formatter.Format(node, self.lang))
    rc_text = RC_TEMPLATE.replace('[[CODEPAGE_NUM]]',
                                  str(self.codepage_number))
    rc_text = rc_text.replace('[[INCLUDES]]', ''.join(include_items))
    
    # Then output the dialogs we have been asked to output.
    dialogs = []
    for dialog_id in dialog_ids:
      node = grd.GetNodeById(dialog_id)
      # TODO(joi) Add exception handling for better error reporting
      formatter = node.ItemFormatter('rc_all')
      dialogs.append(formatter.Format(node, self.lang))
    rc_text = rc_text.replace('[[DIALOGS]]', ''.join(dialogs))
    
    fname = os.path.join(dir_path, '%s.rc' % project_name)
    self.WriteFile(fname, rc_text, self.GetEncoding())
    print "Wrote %s" % fname
    
    # Create the resource.h file
    header_defines = []
    for node in grd:
      formatter = node.ItemFormatter('rc_header')
      if formatter and not isinstance(formatter, rc_header.TopLevel):
        header_defines.append(formatter.Format(node, self.lang))
    header_text = HEADER_TEMPLATE.replace('[[DEFINES]]', ''.join(header_defines))
    fname = os.path.join(dir_path, 'resource.h')
    self.WriteFile(fname, header_text)
    print "Wrote %s" % fname
    
  def WriteFile(self, filename, contents, encoding='cp1252'):
    f = util.WrapOutputStream(file(filename, 'wb'), encoding)
    f.write(contents)
    f.close()
