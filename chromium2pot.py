#!/usr/bin/python
# -*- coding: utf-8 -*-

# (c) 2010-2011, Fabien Tassin <fta@ubuntu.com>

# Convert grd/xtb files into pot/po for integration into the Launchpad
# translation system

## grd files contain the strings for the 'pot' file(s).
## Keys are alphabetical (IDS_XXX).
# Sources:
# - $SRC/chrome/app/*.grd
# - $SRC/webkit/glue/*.grd

## xtb files are referenced to by the grd files. They contain the translated
## strings for the 'po' our files. Keys are numerical (64bit ids).
# Sources:
# - $SRC/chrome/app/resources/*.xtb
# - $SRC/webkit/glue/resources/*.xtb
# and for launchpad contributed strings that already landed:
# - $SRC/third_party/launchpad_translations/*.xtb

## the mapping between those keys is done using FingerPrint()
## [ taken from grit ] on a stripped version of the untranslated string

## grd files contain a lot of <if expr="..."> (python-like) conditions.
## Evaluate those expressions but only skip strings with a lang restriction.
## For all other conditions (os, defines), simply expose them so translators
## know when a given string is expected.

## TODO: handle <message translateable="false">

import os, sys, shutil, re, getopt, codecs, urllib
from xml.dom import minidom
from xml.sax.saxutils import unescape
from datetime import datetime
from difflib import unified_diff
import textwrap, filecmp, json

lang_mapping = {
  'no':    'nb', # 'no' is obsolete and the more specific 'nb' (Norwegian Bokmal)
                 # and 'nn' (Norwegian Nynorsk) are preferred.
  'pt-PT': 'pt'
}

id_set = set()

####
# vanilla from $SRC/tools/grit/grit/extern/FP.py (r10982)
# See svn log http://src.chromium.org/svn/trunk/src/tools/grit/grit/extern/FP.py

# Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

try:
  import hashlib
  _new_md5 = hashlib.md5
except ImportError:
  import md5
  _new_md5 = md5.new

def UnsignedFingerPrint(str, encoding='utf-8'):
  """Generate a 64-bit fingerprint by taking the first half of the md5
  of the string."""
  hex128 = _new_md5(str).hexdigest()
  int64 = long(hex128[:16], 16)
  return int64

def FingerPrint(str, encoding='utf-8'):
  fp = UnsignedFingerPrint(str, encoding=encoding)
  # interpret fingerprint as signed longs
  if fp & 0x8000000000000000L:
    fp = - ((~fp & 0xFFFFFFFFFFFFFFFFL) + 1)
  return fp
####

class EvalConditions:
  """ A class allowing an <if expr="xx"/> to be evaluated, based on an array of defines,
    a dict of local variables.
    As of Chromium 10:
    - the known defines are:
      [ 'chromeos', '_google_chrome', 'toolkit_views', 'touchui', 'use_titlecase' ]
      On Linux, only [ 'use_titlecase' ] is set.
    - the known variables are:
      'os'  ('linux2' on Linux)
      'lang'
    See http://src.chromium.org/svn/trunk/src/build/common.gypi
  """

  def eval(self, expression, defines = [ 'use_titlecase' ], vars = { 'os': "linux2" }):

    def pp_ifdef(match):
      return str(match.group(1) in defines)

    # evaluate all ifdefs
    expression = re.sub(r"pp_ifdef\('(.*?)'\)", pp_ifdef, expression)
    # evaluate the whole expression using the vars dict
    vars['__builtins__'] = { 'True': True, 'False': False } # prevent eval from using the real current globals
    return eval(expression, vars)

  def lang_eval(self, expression, lang):
    """ only evaluate the expression against the lang, ignore all defines and other variables.
    This is needed to ignore a string that has lang restrictions (numerals, plurals, ..) but
    still keep it even if it's OS or defined don't match the local platform.
    """
    conditions = [ x for x in re.split(r'\s+(and|or)\s+', expression) if x.find('lang') >= 0 ]
    if len(conditions) == 0:
      return True
    assert len(conditions) == 1, "Expression '%s' has multiple lang conditions" % expression
    vars = { 'lang': lang, '__builtins__': { 'True': True, 'False': False } }
    return eval(conditions[0], vars)

  def test(self):
    data = [
      { 'expr': "lang == 'ar'",
        'vars': { 'lang': 'ar' },
        'result': True
        },
      { 'expr': "lang == 'ar'",
        'vars': { 'lang': 'fr' },
        'result': False
        },
      { 'expr': "lang in ['ar', 'ro', 'lv']",
        'vars': { 'lang': 'ar' },
        'result': True
        },
      { 'expr': "lang in ['ar', 'ro', 'lv']",
        'vars': { 'lang': 'pt-BR' },
        'result': False
        },
      { 'expr': "lang not in ['ar', 'ro', 'lv']",
        'vars': { 'lang': 'ar' },
        'result': False
        },
      { 'expr': "lang not in ['ar', 'ro', 'lv']",
        'vars': { 'lang': 'no' },
        'result': True
        },
      { 'expr': "os != 'linux2' and os != 'darwin' and os.find('bsd') == -1",
        'vars': { 'lang': 'no', 'os': 'bsdos' },
        'result': False,
        'lresult': True  # no lang restriction in 'expr', so 'no' is ok
        },
      { 'expr': "os != 'linux2' and os != 'darwin' and os.find('bsd') > -1",
        'vars': { 'lang': 'no', 'os': 'bsdos' },
        'result': True,
        },
      { 'expr': "not pp_ifdef('chromeos')",
        'vars': { 'lang': 'no' },
        'defines': [],
        'result': True,
        },
      { 'expr': "not pp_ifdef('chromeos')",
        'vars': { 'lang': 'no' },
        'defines': [ 'chromeos' ],
        'result': False,
        'lresult': True  # no lang restriction in 'expr', so 'no' is ok
        },
      { 'expr': "pp_ifdef('_google_chrome') and (os == 'darwin')",
        'vars': { 'lang': 'no', 'os': 'linux2' },
        'defines': [ 'chromeos' ],
        'result': False,
        'lresult': True  # no lang restriction in 'expr', so 'no' is ok
        },
      { 'expr': "pp_ifdef('_google_chrome') and (os == 'darwin')",
        'vars': { 'lang': 'no', 'os': 'darwin' },
        'defines': [ '_google_chrome' ],
        'result': True
        },
      { 'expr': "not pp_ifdef('chromeos') and pp_ifdef('_google_chrome') and 'pt-PT' == lang",
        'vars': { 'lang': 'pt-PT', 'os': 'darwin' },
        'defines': [ '_google_chrome' ],
        'result': True
        },
      { 'expr': "not pp_ifdef('chromeos') and pp_ifdef('_google_chrome') and 'pt-PT' == lang",
        'vars': { 'lang': 'pt-PT', 'os': 'darwin' },
        'defines': [ ],
        'result': False,
        'lresult': True
        },
     ]
    i = -1
    for d in data:
      i += 1
      defines = d['defines'] if 'defines' in d else []
      vars = d['vars'] if 'vars' in d else {}
      lvars = vars.copy() # make a copy because eval modifies it
      res = self.eval(d['expr'], defines = defines, vars = lvars)
      assert res == d['result'], "FAILED %d: expr: \"%s\" returned %s with vars = %s and defines = %s" % \
          (i, d['expr'], repr(res), repr(vars), repr(defines))
    print "All %d tests passed for EvalConditions.eval()" % (i + 1)
    i = -1
    for d in data:
      i += 1
      assert 'lang' in vars, "All test must have a 'lang' in 'vars', test %d doesn't: %s" % (i, repr(d))
      res = self.lang_eval(d['expr'], lang = d['vars']['lang'])
      expected = d['lresult'] if 'lresult' in d else d['result']
      assert res == expected, "FAILED %d: expr: \"%s\" returned %s with lang = %s for the lang_eval test" % \
          (i, d['expr'], repr(res), d['vars']['lang'])
    print "All %d tests passed for EvalConditions.lang_eval()" % (i + 1)

class StringCvt:
  """ A class converting grit formatted strings to gettext back and forth.
    The idea is to always have:
    a/ grd2gettext(xtb2gettext(s)) == s
    b/ xtb2gettext(s) produces a string that the msgfmt checker likes and
    that makes sense to translators
    c/ grd2gettext(s) produces a string acceptable by upstream
  """

  def xtb2gettext(self, string):
    """ parse the xtb (xml encoded) string and convert it to a gettext string """

    def fold(string):
      return textwrap.wrap(string, break_long_words=False, width=76, drop_whitespace=False,
                           expand_tabs=False, replace_whitespace=False, break_on_hyphens=False)

    s = string.replace('\\n', '\\\\n')
    # escape all single '\' (not followed by 'n')
    s = re.sub(r'(?<!\\)(\\[^n\\\\])', r'\\\1', s)
    # remove all xml encodings
    s = self.unescape_xml(s)
    # replace '<ph name="FOO"/>' by '%{FOO}'
    s = re.sub(r'<ph name="(.*?)"/>', r'%{\1}', s)
    # fold
    # 1/ fold at \n
    # 2/ fold each part at ~76 char
    v = []
    ll = s.split('\n')
    sz = len(ll)
    if sz > 1:
      i = 0
      for l in ll:
        i += 1
        if i == sz:
          v.extend(fold(l))
        else:
          v.extend(fold(l + '\\n'))
    else:
      v.extend(fold(ll[0]))
    if len(v) > 1:
      v[:0] = [ '' ]
    s = '"' + '"\n"'.join(v)  + '"'
    return s

  def decode_xml_entities(self, string):
    def replace_xmlent(match):
      if match.group(1)[:1] == 'x':
        return unichr(int("0" + match.group(1), 16))
      else:
        return unichr(int(match.group(1)))

    return re.sub(r'&#(x\w+|\d+);', replace_xmlent, string)

  def unescape_xml(self, string):
    string = unescape(string).replace('&quot;', '\\"').replace('&apos;', "'")
    string = self.decode_xml_entities(string)
    return string

  def grd2gettext(self, string):
    """ parse the string returned from minidom and convert it to a gettext string.
    This is similar to str_cvt_xtb2gettext but minidom has its own magic for encoding 
    """
    return self.xtb2gettext(string)

  def gettext2xtb(self, string):
    """ parse the gettext string and convert it to an xtb (xml encoded) string. """
    u = []
    for s in string.split(u'\n'):
      # remove the enclosing double quotes
      u.append(s[1:][:-1])
    s = u"".join(u)

    # encode the xml special chars
    s = s.replace("&", "&amp;") # must be first!
    s = s.replace("<", "&lt;")
    s = s.replace(">", "&gt;")
    s = s.replace('\\"', "&quot;")
    # special case, html comments
    s = re.sub(r'&lt;!--(.*?)--&gt;', r'<!--\1-->', s, re.S)
    # replace non-ascii by &#xxx; codes
    # s = s.encode("ascii", "xmlcharrefreplace")
    # replace '%{FOO}' by '<ph name="FOO"/>'
    s = re.sub(r'%{(.*?)}', r'<ph name="\1"/>', s)
    # unquote \\n and \\\\n
    s = re.sub(r'(?<!\\)\\n', r'\n', s)
    # unquote all control chars
    s = re.sub(r'\\\\([^\\])', r'\\\1', s)

    # launchpad seems to always quote tabs 
    s = s.replace("\\t", "\t")
    return s

  def test(self):
    # unit tests
    data = [
      # tab
      { 'id': '0',
        'xtb': u'foo	bar',
        'po': u'"foo	bar"' },
      { 'id': '1',
        'xtb': u'foo\tbar',
        'po': u'"foo\tbar"' },
      # &amp;
      { 'id': '6779164083355903755',
        'xtb': u'Supprime&amp;r',
        'po': u'"Supprime&r"' },
      # &quot;
      { 'id': '4194570336751258953',
        'xtb': u'Activer la fonction &quot;taper pour cliquer&quot;',
        'po': u'"Activer la fonction \\"taper pour cliquer\\""' },
      # &lt; / &gt;
      { 'id': '7615851733760445951',
        'xtb': u'&lt;aucun cookie sélectionné&gt;',
        'po': u'"<aucun cookie sélectionné>"' },
      # <ph name="FOO"/>
      { 'id': '5070288309321689174',
        'xtb': u'<ph name="EXTENSION_NAME"/> :',
        'po': u'"%{EXTENSION_NAME} :"' },
      { 'id': '1467071896935429871',
        'xtb': u'Téléchargement de la mise à jour du système : <ph name="PERCENT"/>% terminé',
        'po': u'"Téléchargement de la mise à jour du système : %{PERCENT}% terminé"' },
      # line folding
      { 'id': '1526811905352917883',
        'xtb': u'Une nouvelle tentative de connexion avec SSL 3.0 a dû être effectuée. Cette opération indique généralement que le serveur utilise un logiciel très ancien et qu\'il est susceptible de présenter d\'autres problèmes de sécurité.',
        'po': u'""\n"Une nouvelle tentative de connexion avec SSL 3.0 a dû être effectuée. Cette "\n"opération indique généralement que le serveur utilise un logiciel très "\n"ancien et qu\'il est susceptible de présenter d\'autres problèmes de sécurité."' },
      { 'id': '7999229196265990314',
        'xtb': u'Les fichiers suivants ont été créés :\n\nExtension : <ph name="EXTENSION_FILE"/>\nFichier de clé : <ph name="KEY_FILE"/>\n\nConservez votre fichier de clé en lieu sûr. Vous en aurez besoin lors de la création de nouvelles versions de l\'extension.',
        'po': u'""\n"Les fichiers suivants ont été créés :\\n"\n"\\n"\n"Extension : %{EXTENSION_FILE}\\n"\n"Fichier de clé : %{KEY_FILE}\\n"\n"\\n"\n"Conservez votre fichier de clé en lieu sûr. Vous en aurez besoin lors de la "\n"création de nouvelles versions de l\'extension."' },
      # quoted LF
      { 'id': '4845656988780854088',
        'xtb': u'Synchroniser uniquement les paramètres et\\ndonnées qui ont changé depuis la dernière connexion\\n(requiert votre mot de passe précédent)',
        'po': u'""\n"Synchroniser uniquement les paramètres et\\\\ndonnées qui ont changé depuis la"\n" dernière connexion\\\\n(requiert votre mot de passe précédent)"' },
      { 'id': '1761265592227862828', # lang: 'el'
        'xtb': u'Συγχρονισμός όλων των ρυθμίσεων και των δεδομένων\\n (ενδέχεται να διαρκέσει ορισμένο χρονικό διάστημα)',
        'po': u'""\n"Συγχρονισμός όλων των ρυθμίσεων και των δεδομένων\\\\n (ενδέχεται να διαρκέσει"\n" ορισμένο χρονικό διάστημα)"' },
      { 'id': '1768211415369530011', # lang: 'de'
        'xtb': u'Folgende Anwendung wird gestartet, wenn Sie diese Anforderung akzeptieren:\\n\\n <ph name="APPLICATION"/>',
        'po': u'""\n"Folgende Anwendung wird gestartet, wenn Sie diese Anforderung "\n"akzeptieren:\\\\n\\\\n %{APPLICATION}"' },
      # weird controls
      { 'id': '5107325588313356747', # lang: 'es-419'
        'xtb': u'Para ocultar el acceso a este programa, debes desinstalarlo. Para ello, utiliza\\n<ph name="CONTROL_PANEL_APPLET_NAME"/> del Panel de control.\\n\¿Deseas iniciar <ph name="CONTROL_PANEL_APPLET_NAME"/>?',
        'po': u'""\n"Para ocultar el acceso a este programa, debes desinstalarlo. Para ello, "\n"utiliza\\\\n%{CONTROL_PANEL_APPLET_NAME} del Panel de control.\\\\n\\\\¿Deseas "\n"iniciar %{CONTROL_PANEL_APPLET_NAME}?"' }
    ]

    for string in data:
      s = u"<x>" + string['xtb'] + u"</x>"
      s = s.encode('ascii', 'xmlcharrefreplace')
      dom = minidom.parseString(s)
      s = dom.firstChild.toxml()[3:][:-4]
      e = self.grd2gettext(s)
      if e != string['po']:
        assert False, "grd2gettext() failed for id " + string['id'] + \
            ". \nExpected: " + repr(string['po']) + "\nGot:      " + repr(e)
      e = self.xtb2gettext(string['xtb'])
      if e != string['po']:
        assert False, "xtb2gettext() failed for id " + string['id'] + \
            ". \nExpected: " + repr(string['po']) + "\nGot:      " + repr(e)
      u = self.gettext2xtb(e)
      if u != string['xtb']:
        assert False, "gettext2xtb() failed for id " + string['id'] + \
            ". \nExpected: " + repr(string['xtb']) + "\nGot:      " + repr(u)
      print string['id'] + " ok"

    # more tests with only po to xtb to test some weird launchpad po exports
    data2 = [
      { 'id': '1768211415369530011', # lang: 'de'
        'po': u'""\n"Folgende Anwendung wird gestartet, wenn Sie diese Anforderung akzeptieren:\\\\"\n"n\\\\n %{APPLICATION}"',
        'xtb': u'Folgende Anwendung wird gestartet, wenn Sie diese Anforderung akzeptieren:\\n\\n <ph name="APPLICATION"/>' },
      ]
    for string in data2:
      u = self.gettext2xtb(string['po'])
      if u != string['xtb']:
        assert False, "gettext2xtb() failed for id " + string['id'] + \
            ". \nExpected: " + repr(string['xtb']) + "\nGot:      " + repr(u)
      print string['id'] + " ok"

######

class PotFile(dict):
  """
 Read and write gettext pot files
  """

  def __init__(self, filename, date = None, debug = False, branch_name = "default", branch_dir = os.getcwd()):
    self.debug = debug
    self.lang = None
    self.filename = filename
    self.tfile = filename + ".new"
    self.branch_dir = branch_dir
    self.branch_name = branch_name
    self.template_date = date
    self.translation_date = "YEAR-MO-DA HO:MI+ZONE"
    self.is_pot = True
    self.fd = None
    self.fd_mode = "rb"
    if self.template_date is None:
      self.template_date = datetime.utcnow().strftime("%Y-%m-%d %H:%M+0000")
    self.strings = []

  def add_string(self, id, comment, string, translation = "", origin = None):
    self.strings.append({ 'id': id, 'comment': comment, 'string': string,
                          'origin': origin, 'translation': translation })

  def replace_file_if_newer(self):
    filename = os.path.join(self.branch_dir, self.filename) if self.branch_dir is not None \
        else self.filename
    tfile = os.path.join(self.branch_dir, self.tfile) if self.branch_dir is not None \
        else self.tfile
    if os.path.isfile(filename) and filecmp.cmp(filename, tfile) == 1:
      os.unlink(tfile)
      return 0
    else:
      os.rename(tfile, filename)
      return 1

  def get_mtime(self, file):
    rfile = os.path.join(self.branch_dir, file)
    if self.debug:
      print "getmtime(%s) [%s]" % (file, os.path.abspath(rfile))
    return os.path.getmtime(rfile)

  def open(self, mode = "rb", filename = None):
    if filename is not None:
      self.filename = filename
      self.tfile = filename + ".new"
    rfile = os.path.join(self.branch_dir, self.filename)
    rtfile = os.path.join(self.branch_dir, self.tfile)
    if self.fd is not None:
      self.close()
    self.fd_mode = mode
    if mode.find("r") != -1:
      if self.debug:
        print "open %s [mode=%s] from branch '%s' [%s]" % (self.filename, mode, self.branch_name, os.path.abspath(rfile))
      self.fd = codecs.open(rfile, mode, encoding="utf-8")
    else:
      if self.debug:
        print "open %s [mode=%s] from branch '%s' [%s]" % (self.tfile, mode, self.branch_name, os.path.abspath(rtfile))
      self.fd = codecs.open(rtfile, mode, encoding="utf-8")

  def close(self):
    self.fd.close()
    self.fd = None
    if self.fd_mode.find("w") != -1:
      return self.replace_file_if_newer()

  def read_string(self):
    string = {}
    cur = None
    while 1:
      s = self.fd.readline()
      if len(s) == 0 or s == "\n":
        break # EOF or end of block
      if s.rfind('\n') == len(s) - 1:
        s = s[:-1]  # chomp
      if s.find("# ") == 0 or s == "#":  # translator-comment
        if 'comment' not in string:
          string['comment'] = ''
        string['comment'] += s[2:]
        continue
      if s.find("#:") == 0: # reference
        if 'reference' not in string:
          string['reference'] = ''
        string['reference'] += s[2:]
        if s[2:].find(" id: ") == 0:
          string['id'] = s[7:].split(' ')[0]
        continue
      if s.find("#.") == 0: # extracted-comments
        if 'extracted' not in string:
          string['extracted'] = ''
        string['extracted'] += s[2:]
        if s[2:].find(" - condition: ") == 0:
          if 'conditions' not in string:
            string['conditions'] = []
          string['conditions'].append(s[16:])
        continue
      if s.find("#~") == 0: # obsolete messages
        continue
      if s.find("#") == 0: # something else
        print "%s not expected. Skip" % repr(s)
        continue  # not supported/expected
      if s.find("msgid ") == 0:
        cur = "string"
        if cur not in string:
          string[cur] = u""
        else:
          string[cur] += "\n"
        string[cur] += s[6:]
        continue
      if s.find("msgstr ") == 0:
        cur = "translation"
        if cur not in string:
          string[cur] = u""
        else:
          string[cur] += "\n"
        string[cur] += s[7:]
        continue
      if s.find('"') == 0:
        if cur is None:
          print "'%s' not expected here. Skip" % s
          continue
        string[cur] += "\n" + s
        continue
      print "'%s' not expected here. Skip" % s
    return None if string == {} else string

  def write(self, string):
    self.fd.write(string)

  def write_header(self):
    lang_team = "LANGUAGE <LL@li.org>" if self.is_pot else "%s <%s@li.org>" % (self.lang, self.lang)
    lang_str = "template" if self.is_pot else "for lang '%s'" % self.lang
    date = "YEAR-MO-DA HO:MI+ZONE" if self.is_pot else \
        datetime.fromtimestamp(self.translation_date).strftime("%Y-%m-%d %H:%M+0000")
    #self.write("# Chromium Translations %s.\n" 
    #           "# Copyright (C) 2010-2011 Fabien Tassin\n"
    #           "# This file is distributed under the same license as the chromium-browser package.\n"
    #           "# Fabien Tassin <fta@ubuntu.com>, 2010-2011.\n"
    #           "#\n" % lang_str)
    # FIXME: collect contributors (can LP export them?)
    self.write('msgid ""\n'
               'msgstr ""\n'
               #'"Project-Id-Version: chromium-browser.head\\n"\n'
               #'"Report-Msgid-Bugs-To: https://bugs.launchpad.net/ubuntu/+source/chromium-browser/+filebug\\n"\n'
               #'"POT-Creation-Date: %s\\n"\n'
               #'"PO-Revision-Date: %s\\n"\n'
               #'"Last-Translator: FULL NAME <EMAIL@ADDRESS>\\n"\n'
               #'"Language-Team: %s\\n"\n'
               '"MIME-Version: 1.0\\n"\n'
               '"Content-Type: text/plain; charset=UTF-8\\n"\n'
               #'"Content-Transfer-Encoding: 8bit\\n"\n\n' % \
               #  (datetime.fromtimestamp(self.template_date).strftime("%Y-%m-%d %H:%M+0000"),
               #   date, lang_team))
               )
 
  def write_footer(self):
    pass

  def write_all_strings(self):
    for string in self.strings:
      self.write(u"#. %s\n" % u"\n#. ".join(string['comment'].split("\n")))
      self.write(u"#: id: %s (used in the following branches: %s)\n" % \
                   (string['id'], ", ".join(string['origin'])))
      self.write(u'msgid %s\n' % StringCvt().xtb2gettext(string['string']))
      self.write(u'msgstr %s\n\n' % StringCvt().xtb2gettext(string['translation']))

  def export_file(self, directory = None, filename = None):
    self.open(mode = "wb", filename = filename)
    self.write_header()
    self.write_all_strings()
    self.write_footer()
    return self.close()

  def import_file(self):
    self.mtime = self.get_mtime(self.filename)
    self.open()
    while 1:
      string = self.read_string()
      if string is None:
        break
      self.strings.append(string)
    self.close()

  def pack_comment(self, data):
    comment = ""
    for ent in sorted(data, lambda x,y: cmp(x['code'], y['code'])):
      comment += "%s\n- description: %s\n" % (ent['code'], ent['desc'])
      if ent['test'] is not None:
        comment += "- condition: %s\n" % ent['test']
    comment = comment[:-1] # strip trailing \n
    return comment

  def get_origins(self, data):
    o = []
    for ent in sorted(data, lambda x,y: cmp(x['code'], y['code'])):
      for origin in ent['origin']:
        if origin not in o:
          o.append(origin)
    return o

  def import_grd(self, grd):
    imported = 0
    for id in sorted(grd.supported_ids.keys()):
      if 'ids' not in grd.supported_ids[id]:
        continue
      comment = self.pack_comment(grd.supported_ids[id]['ids'])
      string = grd.supported_ids[id]['ids'][0]['val']
      origin = self.get_origins(grd.supported_ids[id]['ids'])
      self.strings.append({ 'id': id, 'comment': comment, 'string': string,
                            'origin': origin, 'translation': '' })
      imported += 1
    if self.debug:
      print "imported %d strings from the grd template" % imported

class PoFile(PotFile):
  """
 Read and write gettext po files
  """

  def __init__(self, lang, filename, template, date = None, debug = None,
               branch_name = "default", branch_dir = os.getcwd()):
    super(PoFile, self).__init__(filename, date = template.template_date, debug = debug,
                                 branch_name = branch_name, branch_dir = branch_dir)
    self.template = template
    self.lang = lang
    self.translation_date = date
    self.is_pot = False

  def import_xtb(self, xtb):
    # only import strings present in the current template
    imported = 0
    for id in sorted(xtb.template.supported_ids.keys()):
      if 'ids' not in xtb.template.supported_ids[id]:
        continue
      translation = xtb.strings[id] if id in xtb.strings else ""
      comment = self.template.pack_comment(xtb.template.supported_ids[id]['ids'])
      string = xtb.template.supported_ids[id]['ids'][0]['val']
      origin = self.get_origins(xtb.template.supported_ids[id]['ids'])
      self.add_string(id, comment, string, translation, origin)
      imported += 1
    if self.debug:
      print "imported %d translations for lang %s from xtb into po %s" % (imported, self.lang, self.filename)

class GrdFile(PotFile):
  """
 Read a Grit GRD file (write is not supported)
  """
  def __init__(self, filename, date = None, lang_mapping = None, debug = None,
               branch_name = "default", branch_dir = os.getcwd()):
    super(GrdFile, self).__init__(filename, date = date, debug = debug,
                                  branch_name = branch_name, branch_dir = branch_dir)
    self.lang_mapping = lang_mapping
    self.mapped_langs = {}
    self.supported_langs = {}
    self.supported_ids = {}
    self.supported_ids_counts = {}
    self.translated_strings = {}
    self.stats = {} # per lang
    self.debug = debug
    self._PH_REGEXP = re.compile('(<ph name=")([^"]*)("/>)')

  def open(self):
    pass

  def close(self):
    pass

  def write_header(self):
    raise Exception("Not implemented!")

  def write_footer(self):
    raise Exception("Not implemented!")

  def write_all_strings(self):
    raise Exception("Not implemented!")

  def export_file(self, directory = None, filename = None, global_langs = None, langs = None):
    fdi = codecs.open(self.filename, 'rb', encoding="utf-8")
    fdo = codecs.open(filename, 'wb', encoding="utf-8")
    # can't use minidom here as the file is manually generated and the
    # output will create big diffs. parse the source file line by line
    # and insert our xtb in the <translations> section. Also insert new
    # langs in the <outputs> section (with type="data_package" or type="js_map_format").
    # Let everything else untouched
    tr_found = False
    tr_saved = []
    tr_has_ifs = False
    pak_found = False
    pak_saved = []
    # langs, sorted by their xtb names
    our_langs = map(lambda x: x[0],
                    sorted(map(lambda x: (x, self.mapped_langs[x]['xtb_file']),
                               self.mapped_langs),
                           key = lambda x: x[1])) # d'oh!
    if langs is None:
      langs = our_langs[:]
    for line in fdi.readlines():
      if re.match(r'.*?<output filename=".*?" type="(data_package|js_map_format)"', line):
        pak_found = True
        pak_saved.append(line)
        continue
      if line.find('</outputs>') > 0:
        pak_found = False
        ours = global_langs[:]
        chunks = {}
        c = None
        pak_if = None
        pak_is_in_if = False
        for l in pak_saved:
          if l.find("<!-- ") > 0:
            c = l
            continue
          if l.find("<if ") > -1:
            c = l if c is None else c + l
            tr_has_ifs = True
            pak_is_in_if = True
            continue
          if l.find("</if>") > -1:
            c = l if c is None else c + l
            pak_is_in_if = False
            continue
          m = re.match(r'.*?<output filename="(.*?)_([^_\.]+)\.(pak|js)" type="(data_package|js_map_format)" lang="(.*?)" />', l)
          if m is not None:
            x = { 'name': m.group(1), 'ext': m.group(3), 'lang': m.group(5), 'file_lang': m.group(2),
                  'type': m.group(4), 'in_if': pak_is_in_if, 'line': l }
            if c is not None:
              x['comment'] = c
              c = None
            k = m.group(2) if m.group(2) != 'nb' else 'no'
            chunks[k] = x
          else:
            if c is None:
              c = l
            else:
              c += l
        is_in_if = False
        for lang in sorted(chunks.keys()):
          tlang = lang if lang != 'no' else 'nb'
          while len(ours) > 0 and ((ours[0] == 'nb' and 'no' < tlang) or (ours[0] != 'nb' and ours[0] < tlang)):
            if ours[0] in chunks:
              ours = ours[1:]
              continue
            if tr_has_ifs and is_in_if is False:
              fdo.write('    <if expr="pp_ifdef(\'use_third_party_translations\')">\n')
            f = "%s_%s.%s" % (chunks[lang]['name'], ours[0], chunks[lang]['ext'])
            fdo.write('    %s<output filename="%s" type="%s" lang="%s" />\n' % \
                          ('  ' if tr_has_ifs else '', f, chunks[lang]['type'], ours[0]))
            is_in_if = True
            if tr_has_ifs and chunks[lang]['in_if'] is False:
              fdo.write('    </if>\n')
              is_in_if = False
            ours = ours[1:]
          if 'comment' in chunks[lang]:
            for s in chunks[lang]['comment'].split('\n')[:-1]:
              if chunks[lang]['in_if'] is True and is_in_if and s.find('<if ') > -1:
                continue
              if s.find('<!-- No translations available. -->') > -1:
                continue
              fdo.write(s + '\n')
          fdo.write(chunks[lang]['line'])
          ours = ours[1:]
          is_in_if = chunks[lang]['in_if']
        if len(chunks.keys()) > 0:
          while len(ours) > 0:
            f = "%s_%s.%s" % (chunks[lang]['name'], ours[0], chunks[lang]['ext'])
            if tr_has_ifs and is_in_if is False:
              fdo.write('    <if expr="pp_ifdef(\'use_third_party_translations\')">\n')
            fdo.write('    %s<output filename="%s" type="data_package" lang="%s" />\n' % \
                          ('  ' if tr_has_ifs else '', f, ours[0]))
            is_in_if = True
            ours = ours[1:]
          if tr_has_ifs and is_in_if:
            fdo.write('    </if>\n')
            is_in_if = False
        if c is not None:
          for s in c.split('\n')[:-1]:
            if s.find('<!-- No translations available. -->') > -1:
              continue
            if s.find('</if>') > -1:
              continue
            fdo.write(s + '\n')
      if line.find('<translations>') > 0:
        fdo.write(line)
        tr_found = True
        continue
      if line.find('</translations>') > 0:
        tr_found = False
        ours = our_langs[:]
        chunks = {}
        obsolete = []
        c = None
        tr_if = None
        tr_is_in_if = False
        for l in tr_saved:
          if l.find("<!-- ") > 0:
            c = l if c is None else c + l
            continue
          if l.find("<if ") > -1:
            c = l if c is None else c + l
            tr_has_ifs = True
            tr_is_in_if = True
            continue
          if l.find("</if>") > -1:
            c = l if c is None else c + l
            tr_is_in_if = False
            continue
          m = re.match(r'.*?<file path=".*_([^_]+)\.xtb" lang="(.*?)"', l)
          if m is not None:
            x = { 'lang': m.group(1), 'line': l, 'in_if': tr_is_in_if }
            if c is not None:
              x['comment'] = c
              c = None
            chunks[m.group(1)] = x
            if m.group(1) not in langs and m.group(1) not in map(lambda t: self.mapped_langs[t]['xtb_file'], langs):
              obsolete.append(m.group(1))
          else:
            if c is None:
              c = l
            else:
              c += l
        is_in_if = False
        # Do we want <if/> in the <translations/> block? (they are only mandatory in the <outputs/> block)
        want_ifs_in_translations = False
        for lang in sorted(chunks.keys()):
          while len(ours) > 0 and self.mapped_langs[ours[0]]['xtb_file'] < lang:
            if ours[0] not in self.supported_langs:
              if self.debug:
                print "Skipped export of lang '%s' (most probably a 'po' file without any translated strings)" % ours[0]
              ours = ours[1:]
              continue
            if ours[0] in obsolete:
              if self.debug:
                print "Skipped export of lang '%s' (now obsolete)" % ours[0]
              ours = ours[1:]
              continue
            f = os.path.relpath(self.supported_langs[ours[0]], os.path.dirname(self.filename))
            if want_ifs_in_translations and tr_has_ifs and is_in_if is False:
              fdo.write('    <if expr="pp_ifdef(\'use_third_party_translations\')">\n')
              is_in_if = True
            fdo.write('    %s<file path="%s" lang="%s" />\n' % ('  ' if want_ifs_in_translations and tr_has_ifs else '', f, ours[0]))
            if tr_has_ifs and chunks[lang]['in_if'] is False:
              if want_ifs_in_translations:
                fdo.write('    </if>\n')
              is_in_if = False
            ours = ours[1:]
          if 'comment' in chunks[lang]:
            for s in chunks[lang]['comment'].split('\n')[:-1]:
              if chunks[lang]['in_if'] is True and is_in_if and s.find('<if ') > -1:
                continue
              if s.find('<!-- No translations available. -->') > -1:
                continue
              fdo.write(s + '\n')
          if lang not in obsolete:
            fdo.write(chunks[lang]['line'])
          ours = ours[1:]
          is_in_if = chunks[lang]['in_if']
        while len(ours) > 0:
          if ours[0] in self.supported_langs:
            f = os.path.relpath(self.supported_langs[ours[0]], os.path.dirname(self.filename))
            if want_ifs_in_translations and tr_has_ifs and is_in_if is False:
              fdo.write('    <if expr="pp_ifdef(\'use_third_party_translations\')">\n')
              is_in_if = True
            fdo.write('    %s<file path="%s" lang="%s" />\n' % ('  ' if want_ifs_in_translations and tr_has_ifs else '', f, ours[0]))
          elif self.debug:
            print "Skipped lang %s with no translated strings" % ours[0]
          ours = ours[1:]
        if is_in_if:
          fdo.write('    </if>\n')
          is_in_if = False
        if c is not None:
          for s in c.split('\n')[:-1]:
            if s.find('<!-- No translations available. -->') > -1:
              continue
            if s.find('</if>') > -1:
              continue
            fdo.write(s + '\n')
      if tr_found:
        tr_saved.append(line)
        continue
      if pak_found:
        pak_saved.append(line)
        continue
      fdo.write(line)
    fdi.close()
    fdo.close()

  def uc(self, match):
    return match.group(2).upper()

  def uc_name(self, match):
    return match.group(1) + match.group(2).upper() + match.group(3)

  def is_string_valid_for_lang(self, id, lang):
    ok = False
    for string in self.supported_ids[id]['ids']:
      if string['test'] is not None:
        ok |= EvalConditions().lang_eval(string['test'], lang)
        if ok:
          break
      else:
        ok = True
        break
    return ok

  def get_supported_strings_count(self, lang):
    # need to ignore strings for which this lang is not wanted in the <if> conditions
    if lang in self.supported_ids_counts:
      return self.supported_ids_counts[lang]['count'], self.supported_ids_counts[lang]['skipped']
    count = 0
    skipped = 0
    for id in self.supported_ids:
      ok = self.is_string_valid_for_lang(id, lang)
      if ok:
        count += 1
      else:
        skipped += 1
    assert count + skipped == len(self.supported_ids.keys())
    self.supported_ids_counts[lang] = { 'count': count, 'skipped': skipped }
    return count, skipped

  def get_supported_langs(self):
    return sorted(self.supported_langs.keys())

  def get_supported_lang_filenames(self):
    """ return the list of (xtb) filenames sorted by langs (so it's
    possible to zip() it) """
    return map(lambda l: self.supported_langs[l], sorted(self.supported_langs.keys()))

  def update_stats(self, lang, translated_upstream = 0, obsolete = 0,
                   new = 0, updated = 0, skipped_lang = 0, mandatory_linux = 0):
    if lang not in self.stats:
      self.stats[lang] = { 'translated_upstream': 0, 'skipped_lang': 0,
                           'obsolete': 0, 'new': 0, 'updated': 0,
                           'mandatory_linux': 0 }
    self.stats[lang]['translated_upstream'] += translated_upstream - updated
    self.stats[lang]['obsolete'] += obsolete
    self.stats[lang]['new']      += new
    self.stats[lang]['updated']  += updated
    self.stats[lang]['skipped_lang'] += skipped_lang
    self.stats[lang]['mandatory_linux'] += mandatory_linux

  def merge_template(self, template, newer_preferred = True):
    """ merge strings from 'template' into self (the master template).
    If the string differs, prefer the new one when newer_preferred is set """
    for id in template.supported_ids:
      if id not in self.supported_ids:
        if self.debug:
          print "merged code %s (id %s) from branch '%s' from %s" % \
              (template.supported_ids[id]['ids'][0]['code'], id,
               template.supported_ids[id]['ids'][0]['origin'][0], template.filename)
        self.supported_ids[id] = template.supported_ids[id]
      else:
        for ent in template.supported_ids[id]['ids']:
          found = False
          for ent2 in self.supported_ids[id]['ids']:
            if ent2['code'] != ent['code']:
              continue
            found = True
            ent2['origin'].append(ent['origin'][0])
            if ent['test'] != ent2['test'] or \
                  ent['desc'] != ent2['desc']:
              if newer_preferred:
                ent2['test'] = ent['test'] 
                ent2['desc'] = ent['desc'] 
          if not found:
            if self.debug:
              print "adding new ids code '%s' from branch '%s' for string id %s" % \
                  (ent['code'], template.supported_ids[id]['ids'][0]['origin'][0], id)
            self.supported_ids[id]['ids'].append(ent)

  def add_translation(self, lang, id, translation):
    if id not in self.supported_ids:
      if self.debug:
        print "*warn* obsolete string id %s for lang %s" % (id, lang)
      return
    self.supported_ids[id]['lang'][lang] = translation

  def merge_translations(self, lang, xtb, master_xtb = None, newer_preferred = True):
    if lang not in self.supported_langs:
      self.supported_langs[lang] = xtb.filename
    for id in xtb.strings:
      if id not in self.supported_ids:
        # d'oh!! obsolete translation?
        self.update_stats(lang, obsolete = 1)
        continue
      if not self.is_string_valid_for_lang(id, lang):
        # string not wanted for that lang, skipped
        continue
      if 'lang' not in self.supported_ids[id]:
        self.supported_ids[id]['lang'] = {}
      if lang in self.supported_ids[id]['lang']:
        # already have a translation for this string
        if newer_preferred and xtb.strings[id] != self.supported_ids[id]['lang'][lang]:
          self.supported_ids[id]['lang'][lang] = xtb.strings[id]
      else:
        self.update_stats(lang, translated_upstream = 1)
        self.supported_ids[id]['lang'][lang] = xtb.strings[id]
        if master_xtb is not None:
          master_xtb.strings[id] = xtb.strings[id]

  def read_string(self, node, test = None):
    desc = node.getAttribute('desc')
    name = node.getAttribute('name')

    # only convert modified strings
    if not (name in id_set):
      return

    if not node.firstChild:
      # no string? weird. Skip. (e.g. IDS_LOAD_STATE_IDLE)
      return

    # Get a/ the full string from the grd, b/ its transformation
    # into the smaller version found in xtb files (val) and c/ another into
    # something suitable for the 64bit key generator (kval)

    orig_val = "".join([ n.toxml() for n in node.childNodes ])

    # encode the value to create the 64bit ID needed for the xtb mapping.
    #
    # grd: 'f&amp;oo &quot;<ph name="IDS_xX">$1<ex>blabla</ex></ph>&quot; bar'
    # xtb: 'f&amp;oo &quot;<ph name="IDS_XX"/>&quot; bar'
    # but the string used to create the 64bit id is only 'f&oo "IDS_XX" bar'.
    # Also, the final value must be positive, while FingerPrint() returns
    # a signed long. Of course, none of this is documented...
  
    # grd->xtb
    for x in node.getElementsByTagName('ph'):
      while x.hasChildNodes():
        x.removeChild(x.childNodes[0])
    val = "".join([ n.toxml() for n in node.childNodes ]).strip()
    # xtb->id
    kval = StringCvt().decode_xml_entities(unescape(self._PH_REGEXP.sub(self.uc, val))).encode('utf-8')
    kval = kval.replace('&quot;', '"')  # not replaced by unescape()

    val = self._PH_REGEXP.sub(self.uc_name, val)
    val = val.encode("ascii", "xmlcharrefreplace").strip().encode('utf-8')

    # finally, create the 64bit ID
    id = str(FingerPrint(kval) & 0x7fffffffffffffffL)

    if id not in self.supported_ids:
      self.supported_ids[id] = { 'ids': [] }
    self.supported_ids[id]['ids'].append({ 'code': name, 'desc': desc,
                                           'val': val, 'test': test,
                                           'origin': [ self.branch_name ] })

  def read_strings(self, node, test = None):
    for n in node.childNodes:
      if n.nodeName == '#text' or n.nodeName == '#comment':
        # comments, skip
        continue
      if n.nodeName == 'message':
        self.read_string(n, test)
        continue
      if n.nodeName == 'if':
        expr = n.getAttribute('expr')
        if expr is not None and test is not None:
          assert "nested <if> not supported"
        self.read_strings(n, expr)
        continue
      print "unknown tag (<%s> type %s): ''%s''" % \
          (n.nodeName, n.nodeType, n.toxml())

  def import_json_file(self, filename):
    # unlike its name seems to indicate, this file is definitely not a json file.
    # It's a python object, dumped in a file. It means it's far easier to parse
    # because there's no extra unescaping to do on all the strings. It also
    # means we can't use the json module
    rfile = os.path.join(self.branch_dir, filename)
    if self.debug:
      print "parse_json('%s') [%s]" % (filename, rfile)
    fd = open(rfile, "rb")
    data = fd.read()
    fd.close()
    vars = { '__builtins__': { 'True': True, 'False': False } } # prevent eval from using the real current globals
    data = eval(data, vars)
    # Check if this is a format we support
    if 'policy_definitions' in data and len(data['policy_definitions']) > 0 and \
          'caption' not in data['policy_definitions'][0]:
      # most probably Chromium v9. It used 'annotations' instead of 'caption'
      # Not worth supporting that, all the strings we need in v9 are already in
      # the grd file. Skip this json file
      if self.debug:
        print "Found older unsupported json format. Skipped"
      return
    if 'messages' in data:
      for msg in data['messages']:
        self.read_policy('IDS_POLICY_' + msg.upper(),
                         data['messages'][msg]['desc'],
                         data['messages'][msg]['text'])
    if 'policy_definitions' in data:
      for policy in data['policy_definitions']:
        name = 'IDS_POLICY_' + policy['name'].upper()
        if policy['type'] in [ 'main', 'int', 'string', 'list' ]:
          # caption
          self.read_policy(name + '_CAPTION',
                           "Caption of the '%s' policy." % policy['name'],
                           policy['caption'])
          # label (optional)
          if 'label' in policy:
            self.read_policy(name + '_LABEL',
                             "Label of the '%s' policy." % policy['name'],
                             policy['label'])
          # desc
          self.read_policy(name + '_DESC',
                           "Description of the '%s' policy." % policy['name'],
                           policy['desc'])
          continue
        if policy['type'] == 'group':
          # group caption
          self.read_policy(name + '_CAPTION',
                           "Caption of the group of '%s' related policies." % name,
                           policy['caption'])
          # group label (optional)
          if 'label' in policy:
            self.read_policy(name + '_LABEL',
                             "Label of the group of '%s' related policies." % name,
                             policy['label'])
          # group desc
          self.read_policy(name + '_DESC',
                           "Description of the group of '%s' related policies." % name,
                           policy['desc'])
          for spolicy in policy['policies']:
            sname = 'IDS_POLICY_' + spolicy['name'].upper()
            # desc
            self.read_policy(sname + '_DESC',
                             "Description of the '%s' policy." % spolicy['name'],
                             spolicy['desc'])
            # label (optional)
            if 'label' in spolicy:
              self.read_policy(sname + '_LABEL',
                               "Label of the '%s' policy." % spolicy['name'],
                               spolicy['label'])
            # caption
            self.read_policy(sname + '_CAPTION',
                             "Caption of the '%s' policy." % spolicy['name'],
                             spolicy['caption'])
            if spolicy['type'] in [ 'int-enum', 'string-enum' ]:
              # only caption
              for item in spolicy['items']:
                self.read_policy('IDS_POLICY_ENUM_' + item['name'].upper() + '_CAPTION',
                                 "Label in a '%s' dropdown menu for selecting a '%s' of '%s'" % \
                                   (policy['name'], spolicy['name'], item['name']),
                                 item['caption'])
          continue
        assert False, "Policy type '%s' not supported" % policy['type']

  def read_policy(self, name, desc, text):
    xml = '<x><message name="%s" desc="%s">\n%s\n</message></x>' % (name, desc, text)
    dom = minidom.parseString(xml)
    self.read_strings(dom.getElementsByTagName('x')[0])

  def _add_xtb(self, node):
    if node.nodeName != 'file':
      return
    path = node.getAttribute('path')
    m = re.match(r'.*_([^_]+)\.xtb', path)
    flang = m.group(1)
    lang = node.getAttribute('lang')
    tlang = lang
    if self.lang_mapping is not None and lang in self.lang_mapping:
      if self.debug:
        print "# mapping lang '%s' to '%s'" % (lang, self.lang_mapping[lang])
      tlang = self.lang_mapping[lang]
    tlang = tlang.replace('-', '_')
    self.supported_langs[lang] = os.path.normpath(os.path.join(os.path.dirname(self.filename), path))
    self.translated_strings[lang] = {}
    self.mapped_langs[lang] = { 'xtb_file': flang, 'gettext': tlang }

  def import_file(self):
    filename = os.path.join(self.branch_dir, self.filename) if self.branch_dir is not None \
        else self.filename
    self.supported_langs = {}
    self.mtime = self.get_mtime(self.filename)
    if self.debug:
      print "minidom.parse(%s)" % filename
    dom = minidom.parse(filename)
    grit = dom.getElementsByTagName('grit')[0]
    for node in grit.childNodes:
      if node.nodeName == '#text' or node.nodeName == '#comment':
        # comments, skip
        continue
      if node.nodeName == 'outputs':
        # skip, nothing for us here
        continue
      if node.nodeName == 'translations':
        # collect the supported langs by scanning the list of xtb files
        for n in node.childNodes:
          if n.nodeName == 'if':
            for nn in n.childNodes:
              self._add_xtb(nn)
            continue
          self._add_xtb(n)
        continue
      if node.nodeName == 'release':
        for n in node.childNodes:
          if n.nodeName == '#text' or n.nodeName == '#comment':
            # comments, skip
            continue
          if n.nodeName == 'includes':
            # skip, nothing for us here
            continue
          if n.nodeName == 'structures':
            for sn in n.childNodes:
              if sn.nodeName != 'structure':
                continue
              type = sn.getAttribute('type')
              if type == 'dialog':
                # nothing for us here
                continue
              name = sn.getAttribute('name')
              file = sn.getAttribute('file')
              if type == 'policy_template_metafile':
                # included file containing the strings that are usually in the <messages> tree.
                fname = os.path.normpath(os.path.join(os.path.dirname(self.filename), file))
                self.import_json_file(fname)
                continue
              else:
                if self.debug:
                  print "unknown <structure> type found ('%s') in %s" % (type, self.filename)
            continue
          if n.nodeName == 'messages':
            self.read_strings(n)
            continue
          print "unknown tag (<%s> type %s): ''%s''" % (n.nodeName, n.nodeType, n.toxml())
        continue
      print "unknown tag (<%s> type %s): ''%s''" % (node.nodeName, node.nodeType, node.toxml())

class XtbFile(PoFile):
  """
 Read and write a Grit XTB file
  """

  def __init__(self, lang, filename, grd, date = None, debug = None,
               branch_name = "default", branch_dir = os.getcwd()):
    super(XtbFile, self).__init__(lang, filename, grd, date = date, debug = debug,
                                  branch_name = branch_name, branch_dir = branch_dir)
    self.template = grd
    self.strings = {}
    self.strings_updated = 0
    self.strings_new = 0
    self.strings_order = [] # needed to recreate xtb files in a similar order :(

  def add_translation(self, id, string):
    assert id in self.template.supported_ids, "'%s' is not in supported_ids (file=%s)" % (id, self.filename)
    if string[-1:] == '\n' and self.template.supported_ids[id]['ids'][0]['val'][-1:] != '\n':
      # prevent the `msgid' and `msgstr' entries do not both end with '\n' error
      if self.debug:
        print "Found unwanted \\n at the end of translation id " + id + " lang " + self.lang + ". Dropped"
      string = string[:-1]
    if string[0] == '\n' and self.template.supported_ids[id]['ids'][0]['val'][0] != '\n':
      # prevent the `msgid' and `msgstr' entries do not both begin with '\n' error
      if self.debug:
        print "Found unwanted \\n at the begin of translation id " + id + " lang " + self.lang + ". Dropped"
      string = string[1:]
    self.strings[id] = string
    self.strings_order.append(id)

  def write_header(self):
    self.write('<?xml version="1.0" ?>\n')
    self.write('<!DOCTYPE translationbundle>\n')
    self.write('<translationbundle lang="%s">\n' % \
                 self.template.mapped_langs[self.lang]['xtb_file'])

  def write_footer(self):
    self.write('</translationbundle>')

  def write_all_strings(self):
    for id in self.strings_order:
      if id in self.strings:
        self.write('<translation id="%s">%s</translation>\n' % \
                   (id, self.strings[id]))
    for id in sorted(self.strings.keys()):
      if id in self.strings_order:
        continue
      self.write('<translation id="%s">%s</translation>\n' % \
                   (id, self.strings[id]))

  def import_po(self, po):
    for string in po.strings:
      if string['string'] == '':
        continue
      self.add_string(string['id'], string['extracted'],
                      string['string'], string['translation'])

  def import_file(self):
    self.open()
    file = self.fd.read() # *sigh*
    self.close()
    imported = 0
    for m in re.finditer('<translation id="(.*?)">(.*?)</translation>',
                         file, re.S):
      if m.group(1) not in self.template.supported_ids:
        if self.debug:
          print "found a translation for obsolete string id %s in upstream xtb %s" % (m.group(1), self.filename)
        continue
      self.add_translation(m.group(1), m.group(2))
      imported += 1
    for m in re.finditer('<translationbundle lang="(.*?)">', file):
      lang = m.group(1)
      if self.lang in self.template.mapped_langs:
        assert self.template.mapped_langs[self.lang]['xtb_file'] == lang, \
            "bad lang mapping for '%s' while importing %s" % (lang, self.filename)
      else:
        tlang = lang
        if self.template.lang_mapping is not None and lang in self.template.lang_mapping:
          if self.debug:
            print "# mapping lang '%s' to '%s'" % (lang, self.template.lang_mapping[lang])
          tlang = self.template.lang_mapping[lang]
        tlang = tlang.replace('-', '_')
        self.template.mapped_langs[lang] = { 'xtb_file': lang, 'gettext': tlang }
    if self.debug:
      print "imported %d strings from the xtb file into lang '%s'" % (imported, self.lang)
    self.mtime = self.get_mtime(self.filename)

###

class Converter(dict):
  """
  Given a grd template and its xtb translations,
  a/ exports gettext pot template and po translations,
     possibly by merging grd/xtb files from multiple branches
  or 
  b/ imports and merges some gettext po translations,
  and exports xtb translations
  """

  def __init__(self, template_filename, lang_mapping = None, date = None, debug = False,
               html_output = False, branches = None):
    self.debug = debug
    self.translations = {}
    self.errors = 0
    self.template_changes = 0
    self.translations_changes = 0
    self.lang_mapping = lang_mapping
    self.file_mapping = {}
    self.html_output = html_output
    self.stats = {}
    self.branches = branches if branches is not None else [ { 'branch': 'default', 'dir': os.getcwd(), 'grd': template_filename } ]

    # read a grd template from a file
    self.template = GrdFile(self.branches[0]['grd'], date, lang_mapping = self.lang_mapping, debug = self.debug,
                            branch_name = self.branches[0]['branch'], branch_dir = self.branches[0]['dir'])
    self.file_mapping['grd'] = { 'src': self.branches[0]['grd'],
                                 'branches': { self.branches[0]['branch']: self.branches[0]['dir'] } }
    self.template.import_file()
    self.template_pot = None
    for lang, file in zip(self.template.get_supported_langs(),
                          self.template.get_supported_lang_filenames()):
      # also read all the xtb files referenced by this grd template
      rfile = os.path.join(self.branches[0]['dir'] , file)
      xtb = XtbFile(lang, file, self.template, date = self.template.get_mtime(file), debug = self.debug,
                    branch_name = self.branches[0]['branch'], branch_dir = self.branches[0]['dir'])
      self.file_mapping['lang_' + lang] = { 'src': file,
                                            'branches': { self.branches[0]['branch']: self.branches[0]['dir'] } }
      self.stats[lang] = { 'strings': self.template.get_supported_strings_count(lang),
                           'translated_upstream': 0,
                           'changed_in_gettext': 0,
                           'rejected': 0
                          } 
      xtb.import_file()
      self.template.merge_translations(lang, xtb)
      self.translations[lang] = xtb
    # read other grd templates
    if len(self.branches) > 1:
      for branch in self.branches[1:]:
        if self.debug:
          print "merging %s from branch '%s' from %s" % (branch['grd'], branch['branch'], branch['dir'])
        template = GrdFile(branch['grd'], date, lang_mapping = self.lang_mapping, debug = self.debug,
                           branch_name = branch['branch'], branch_dir = branch['dir'])
        self.file_mapping['grd']['branches'][branch['branch']] = branch['dir']
        template.import_file()
        self.template.merge_template(template, newer_preferred = False)
        for lang, file in zip(template.get_supported_langs(),
                              template.get_supported_lang_filenames()):
          xtb = XtbFile(lang, file, self.template, date = template.get_mtime(file), debug = self.debug,
                        branch_name = branch['branch'], branch_dir = branch['dir'])
          if 'lang_' + lang not in self.file_mapping:
            self.file_mapping['lang_' + lang] = { 'src': file, 'branches': {} }
          self.file_mapping['lang_' + lang]['branches'][branch['branch']] = branch['dir']
          # TODO: stats
          xtb.import_file()
          if lang not in self.translations:
            if self.debug:
              print "Add lang '%s' as master xtb for alt branch '%s'" % (lang, branch['branch'])
            self.translations[lang] = xtb
          self.template.merge_translations(lang, xtb, master_xtb = self.translations[lang],
                                           newer_preferred = False)

  def export_gettext_files(self, directory):
    name = os.path.splitext(os.path.basename(self.template.filename))[0]
    if directory is not None:
      directory = os.path.join(directory, name)
      if not os.path.isdir(directory):
        os.makedirs(directory, 0755)
      filename = os.path.join(directory, name + ".pot")
    else:
      filename = os.path.splitext(self.template.filename)[0] + ".pot"
    # create a pot template and merge the grd strings into it
    self.template_pot = PotFile(filename, date = self.template.mtime, debug = self.debug)
    self.template_pot.import_grd(self.template)
    # write it to a file
    self.template_changes += self.template_pot.export_file(directory = directory)

    # do the same for all langs (xtb -> po)
    for lang in self.translations:
      gtlang = self.template.mapped_langs[lang]['gettext']
      file = os.path.join(os.path.dirname(filename), gtlang + ".po")
      po = PoFile(gtlang, file, self.template_pot,
                  date = self.translations[lang].translation_date, debug = self.debug)
      po.import_xtb(self.translations[lang])
      self.translations_changes += po.export_file(directory)

  def export_grit_xtb_file(self, lang, directory):
    name = os.path.splitext(os.path.basename(self.template.filename))[0]
    file = os.path.join(directory, os.path.basename(self.template.supported_langs[lang]))
    if len(self.translations[lang].strings.keys()) > 0:
      if 'lang_' + lang in self.file_mapping:
        self.file_mapping['lang_' + lang]['dst'] = file
      else:
        self.file_mapping['lang_' + lang] = { 'src': None, 'dst': file }
      self.translations[lang].export_file(filename = file)

  def export_grit_files(self, directory, langs):
    grd_dst = os.path.join(directory, os.path.basename(self.template.filename))
    if len(self.translations.keys()) == 0:
      if self.debug:
        print "no translation at all, nothing to export here (template: %s)" % self.template.filename
      return
    if not os.path.isdir(directory):
      os.makedirs(directory, 0755)
    # 'langs' may contain langs for which this template no longer have translations for.
    # They need to be dropped from the grd file
    self.template.export_file(filename = grd_dst, global_langs = langs, langs = self.translations.keys())
    self.file_mapping['grd']['dst'] = grd_dst
    self.file_mapping['grd']['dir'] = directory[:-len(os.path.dirname(self.template.filename)) - 1]
    for lang in self.translations:
      prefix = os.path.relpath(os.path.dirname(self.template.supported_langs[lang]), os.path.dirname(self.template.filename))
      fdirectory = os.path.normpath(os.path.join(directory, prefix))
      if not os.path.isdir(fdirectory):
        os.makedirs(fdirectory, 0755)
      self.export_grit_xtb_file(lang, fdirectory)

  def get_supported_strings_count(self):
    return len(self.template.supported_ids.keys())

  def compare_translations(self, old, new, id, lang):
    # strip leading and trailing whitespaces from the upstream strings
    # (this should be done upstream)
    old = old.strip()
    if old != new:
      s = self.template.supported_ids[id]['ids'][0]['val'] if 'ids' in self.template.supported_ids[id] else "<none?>"
      if self.debug:
        print "Found a different translation for id %s in lang '%s':\n     string: \"%s\"\n   " \
            "upstream: \"%s\"\n  launchpad: \"%s\"\n" %  (id, lang, s, old, new)
    return old == new

  def import_gettext_po_file(self, lang, filename):
    """ import a single lang file into the current translations set,
    matching the current template. Could be useful to merge the upstream
    and launchpad translations, or to merge strings from another project
    (like webkit) """
    po = PoFile(self.template.mapped_langs[lang]['gettext'], filename, self.template,
                date = self.template.get_mtime(filename), debug = self.debug)
    po.import_file()
    # no need to continue if there are no translation in this po
    translated_count = 0
    for s in po.strings:
      if s['string'] != '""' and s['translation'] != '""':
        translated_count += 1
    if translated_count == 0:
      if self.debug:
        print "No translation found for lang %s in %s" % (lang, filename)
      return
    if lang not in self.translations:
      # assuming the filename should be <template_dirname>/../../third_party/launchpad_translations/<template_name>_<lang>.xtb
      tname = os.path.splitext(os.path.basename(self.template.filename))[0]
      f = os.path.normpath(os.path.join(os.path.dirname(self.template.filename), '../../third_party/launchpad_translations',
                       tname + '_' + self.template.mapped_langs[lang]['xtb_file'] + '.xtb'))
      self.translations[lang] = XtbFile(lang, f, self.template, date = po.mtime, debug = self.debug)
      self.template.supported_langs[lang] = f # *sigh*
      
    lp669831_skipped = 0
    for string in po.strings:
      if 'id' not in string:
        continue # PO header
      id = string['id']
      if id in self.template.supported_ids:
        if 'conditions' in string:
          # test the lang against all those conditions. If at least one passes, we need
          # the string
          found = False
          for c in string['conditions']:
            found |= EvalConditions().lang_eval(c, lang)
          if found is False:
            self.template.update_stats(lang, skipped_lang = 1)
            if self.debug:
              print "Skipped string (lang condition) for %s/%s: %s" % \
                  (os.path.splitext(os.path.basename(self.template.filename))[0],
                   lang, repr(string))
            continue
        # workaround bug https://bugs.launchpad.net/rosetta/+bug/669831
        ustring = StringCvt().gettext2xtb(string['string'])
        gt_translation = string['translation'][1:-1].replace('"\n"', '')
        string['translation'] = StringCvt().gettext2xtb(string['translation'])
        grit_str = StringCvt().decode_xml_entities(self.template.supported_ids[id]['ids'][0]['val'])
        if False and 'ids' in self.template.supported_ids[id] and \
              ustring != grit_str:
          # the string for this id is no longer the same, skip it
          lp669831_skipped += 1
          if self.debug:
            print "lp669831_skipped:\n      lp: '%s'\n     grd: '%s'" % (ustring, grit_str)
          continue
        # check for xml errors when '<' or '>' are in the string
        if string['translation'].find('<') >= 0 or \
              string['translation'].find('>') >= 0:
          try:
            # try to parse it with minidom (it's slow!!), and skip if it fails
            s = u"<x>" + string['translation'] + u"</x>"
            dom = minidom.parseString(s.encode('utf-8'))
          except Exception as inst:
            print "Parse error in '%s/%s' for id %s. Skipped.\n%s\n%s" % \
                (os.path.splitext(os.path.basename(self.template.filename))[0], lang, id,
                 repr(string['translation']), inst)
            continue
        # if the upstream string is not empty, but the contributed string is, keep
        # the upstream string untouched
        if string['translation'] == '':
          continue
        # check if we have the same variables in both the upstream string and its
        # translation. Otherwise, complain and reject the translation
        if 'ids' in self.template.supported_ids[id]:
          uvars = sorted([e for e in re.split('(<ph name=".*?"/>)', self.template.supported_ids[id]['ids'][0]['val']) \
                            if re.match('^<ph name=".*?"/>$', e)])
          tvars = sorted([e for e in re.split('(<ph name=".*?"/>)', string['translation'])\
                            if re.match('^<ph name=".*?"/>$', e)])
          lostvars = list(set(uvars).difference(set(tvars)))
          createdvars = list(set(tvars).difference(set(uvars)))
          if len(lostvars) or len(createdvars):
            template = os.path.splitext(os.path.basename(self.template.filename))[0].replace('_', '-')
            self.errors += 1
            if self.html_output:
              print "<div class='error'>[<a id='pherr-%s-%d' href='javascript:toggle(\"pherr-%s-%d\");'>+</a>] " \
                  "<b>ERROR</b>: Found mismatching placeholder variables in string id %s of <b>%s</b> lang <b>%s</b>" % \
                  (template, self.errors, template, self.errors, id, template, lang)
            else:
              print "ERROR: Found mismatching placeholder variables in string id %s of %s/%s:" % \
                  (id, template, lang)
            url = 'https://translations.launchpad.net/chromium-browser/translations/+pots/%s/%s/+translate?batch=10&show=all&search=%s' % \
                (template, self.template.mapped_langs[lang]['gettext'], urllib.quote(gt_translation.encode('utf-8')))
            if self.html_output:
              print "<div id='pherr-%s-%d-t' style='display: none'>\n" \
                  "<fieldset><legend>Details</legend><p><ul>" % (template, self.errors)
              print "<li> <a href='%s'>this string in Launchpad</a>\n" % url
              if len(lostvars):
                print " <li> expected but not found: <code>%s</code>" % " ".join([ re.sub(r'<ph name="(.*?)"/>', r'%{\1}', s) for s in lostvars ])
              if len(createdvars):
                print " <li> found but not expected: <code>%s</code>" % " ".join([ re.sub(r'<ph name="(.*?)"/>', r'%{\1}', s) for s in createdvars ])
              print "</ul><table border='1'>" \
                  "<tr><th rowspan='2'>GetText</th><th>template</th><td><code>%s</code></td></tr>\n" \
                  "<tr><th>translation</th><td><code>%s</code></td></tr>\n" \
                  "<tr><th rowspan='2'>Grit</th><th>template</th><td><code>%s</code></td></tr>\n" \
                  "<tr><th>translation</th><td><code>%s</code></td></tr>\n" \
                  "</table><p>   => <b>translation skipped</b>\n" % \
                  (string['string'][1:-1].replace('"\n"', '').replace('<', '&lt;').replace('>', '&gt;'),
                   gt_translation.replace('<', '&lt;').replace('>', '&gt;'),
                   self.template.supported_ids[id]['ids'][0]['val'].replace('<', '&lt;').replace('>', '&gt;'),
                   string['translation'].replace('<', '&lt;').replace('>', '&gt;'))
              print "</fieldset></div></div>"
            else:
              if len(lostvars):
                print " - expected but not found: " + " ".join(lostvars)
              if len(createdvars):
                print " - found but not expected: " + " ".join(createdvars)
              print "      string: '%s'\n translation: '%s'\n     gettext: '%s'\n         url: %s\n   => translation skipped\n" % \
                  (self.template.supported_ids[id]['ids'][0]['val'], string['translation'], gt_translation, url)
            continue
        # check if the translated string is the same
        if 'lang' in self.template.supported_ids[id] and \
              lang in self.template.supported_ids[id]['lang']:
          # compare
          if self.compare_translations(self.template.supported_ids[id]['lang'][lang],
                                       string['translation'], id, lang):
            continue # it's the same
          if id in self.translations[lang].strings:
            # already added from a previously merged gettext po file
            if self.debug:
              print "already added from a previously merged gettext po file for" + \
                  " template %s %s id %s in lang %s: %s" % \
                  (self.template.branch_name, self.template.filename,
                   id, lang, repr(string['translation']))
            # compare
            if self.compare_translations(self.translations[lang].strings[id],
                                         string['translation'], id, lang):
              continue # it's the same
            # update it..
          if self.debug:
            print "updated string for template %s %s id %s in lang %s: %s" % \
                (self.template.branch_name, self.template.filename, id, lang,
                 repr(string['translation']))
          self.template.update_stats(lang, updated = 1)
          self.translations[lang].strings[id] = string['translation']
          self.translations[lang].strings_updated += 1
        elif id in self.translations[lang].strings:
          # already added from a previously merged gettext po file
          if self.debug:
            print "already added from a previously merged gettext po file for" + \
                "template %s %s id %s in lang %s: %s" % \
                (self.template.branch_name, self.template.filename,
                 id, lang, repr(string['translation']))
          # compare
          if self.compare_translations(self.translations[lang].strings[id],
                                       string['translation'], id, lang):
            continue # it's the same
          # update it..
          self.translations[lang].strings[id] = string['translation']
        else:
          # add
          if self.debug:
            print "add new string for template %s %s id %s in lang %s: %s" % \
                (self.template.branch_name, self.template.filename,
                 id, lang, repr(string['translation']))
          self.template.update_stats(lang, new = 1)
          self.translations[lang].strings[id] = string['translation']
          self.translations[lang].strings_new += 1
    if self.debug and lp669831_skipped > 0:
      print "lp669831: skipped %s bogus/obsolete strings from %s" % \
          (lp669831_skipped, filename[filename[:filename.rfind('/')].rfind('/') + 1:])

  def import_gettext_po_files(self, directory):
    template_name = os.path.splitext(os.path.basename(self.template.filename))[0]
    directory = os.path.join(directory, template_name)
    if not os.path.isdir(directory):
      if self.debug:
         print "WARN: Launchpad didn't export anything for template '%s' [%s]" % (template_name, directory)
      return
    for file in os.listdir(directory):
      base, ext = os.path.splitext(file)
      if ext != ".po":
        continue
      # 'base' is a gettext lang, map it
      lang = None
      for l in self.template.mapped_langs:
        if base == self.template.mapped_langs[l]['gettext']:
          lang = l
          break
      if lang is None: # most probably a new lang, map back
        lang = base.replace('_', '-')
        for l in self.lang_mapping:
          if lang == self.lang_mapping[l]:
            lang = l
            break
        self.template.mapped_langs[lang] = { 'xtb_file': lang, 'gettext': base }
      self.import_gettext_po_file(lang, os.path.join(directory, file))
    # remove from the supported langs list all langs with no translated strings
    # (to catch either empty 'po' files exported by Launchpad, or 'po' files
    # containing only obsolete or too new strings for this branch)
    dropped = []
    for lang in self.translations:
      if len(self.translations[lang].strings.keys()) == 0:
        if self.debug:
          print "no translation found for template '%s' and lang '%s'. lang removed from the supported lang list" % \
              (os.path.splitext(os.path.basename(self.template.filename))[0], lang)
        del(self.template.supported_langs[lang])
        dropped.append(lang)
    for lang in dropped:
      del(self.translations[lang])

  def copy_grit_files(self, directory):
    dst = os.path.join(directory, os.path.dirname(self.template.filename))
    if not os.path.isdir(dst):
      os.makedirs(dst, 0755)
    shutil.copy2(self.template.filename, dst)
    for lang in self.template.supported_langs:
      dst = os.path.join(directory, os.path.dirname(self.translations[lang].filename))
      if not os.path.isdir(dst):
        os.makedirs(dst, 0755)
      shutil.copy2(self.translations[lang].filename, dst)

  def create_patches(self, directory):
    if not os.path.isdir(directory):
      os.makedirs(directory, 0755)
    template_name = os.path.splitext(os.path.basename(self.template.filename))[0]
    patch = codecs.open(os.path.join(directory, "translations-" + template_name + ".patch"),
                        "wb", encoding="utf-8")
    for e in sorted(self.file_mapping.keys()):
      if 'dst' not in self.file_mapping[e]:
        self.file_mapping[e]['dst'] = None
      if self.file_mapping[e]['src'] is not None and \
            self.file_mapping[e]['dst'] is not None and \
            filecmp.cmp(self.file_mapping[e]['src'], self.file_mapping[e]['dst']) == True:
        continue # files are the same

      if self.file_mapping[e]['src'] is not None:
        fromfile  = "old/" + self.file_mapping[e]['src']
        tofile    = "new/" + self.file_mapping[e]['src']
        fromdate  = datetime.fromtimestamp(self.template.get_mtime(
            self.file_mapping[e]['src'])).strftime("%Y-%m-%d %H:%M:%S.%f000 +0000")
        fromlines = codecs.open(self.file_mapping[e]['src'], 'rb', encoding="utf-8").readlines()
      else:
        fromfile  = "old/" + self.file_mapping[e]['dst'][len(self.file_mapping['grd']['dir']) + 1:]
        tofile    = "new/" + self.file_mapping[e]['dst'][len(self.file_mapping['grd']['dir']) + 1:]
        fromdate  = datetime.fromtimestamp(0).strftime("%Y-%m-%d %H:%M:%S.%f000 +0000")
        fromlines = ""
      if self.file_mapping[e]['dst'] is not None:
        todate  = datetime.fromtimestamp(self.template.get_mtime(
            self.file_mapping[e]['dst'])).strftime("%Y-%m-%d %H:%M:%S.%f000 +0000")
        tolines = codecs.open(self.file_mapping[e]['dst'], 'rb', encoding="utf-8").readlines()
      else:
        todate  = datetime.fromtimestamp(0).strftime("%Y-%m-%d %H:%M:%S.%f000 +0000")
        tolines = ""
      diff = unified_diff(fromlines, tolines, fromfile, tofile,
                          fromdate, todate, n=3)
      patch.write("diff -Nur %s %s\n" % (fromfile, tofile))
      s = ''.join(diff)
      # fix the diff so that older patch (<< 2.6) don't fail on new files
      s = re.sub(r'@@ -1,0 ', '@@ -0,0 ', s)
      # ..and make sure patch is able to detect a patch removing files
      s = re.sub(r'(@@ \S+) \+1,0 @@', '\\1 +0,0 @@', s)
      patch.writelines(s)
      if s[-1:] != '\n':
        patch.write("\n\\ No newline at end of file\n")
    patch.close()

  def update_supported_langs_in_grd(self, grd_in, grd_out, langs):
    fdi = codecs.open(grd_in, 'rb', encoding="utf-8")
    fdo = codecs.open(grd_out, 'wb', encoding="utf-8")
    # can't use minidom here as the file is manually generated and the
    # output will create big diffs. parse the source file line by line
    # and insert new langs in the <outputs> section (with type="data_package"
    # or type="js_map_format"). Let everything else untouched
    # FIXME: this is mostly a copy of GrdFile::export_file()
    pak_found = False
    pak_saved = []
    has_ifs = False
    for line in fdi.readlines():
      if re.match(r'.*?<output filename=".*?" type="(data_package|js_map_format)"', line):
        pak_found = True
        pak_saved.append(line)
        continue
      if line.find('</outputs>') > 0:
        pak_found = False
        ours = langs[:]
        chunks = {}
        c = None
        pak_if = None
        pak_is_in_if = False
        for l in pak_saved:
          if l.find("<!-- ") > 0:
            c = l
            continue
          if l.find("<if ") > -1:
            c = l if c is None else c + l
            has_ifs = True
            pak_is_in_if = True
            continue
          if l.find("</if>") > -1:
            c = l if c is None else c + l
            pak_is_in_if = False
            continue
          m = re.match(r'.*?<output filename="(.*?)_([^_\.]+)\.(pak|js)" type="(data_package|js_map_format)" lang="(.*?)" />', l)
          if m is not None:
            x = { 'name': m.group(1), 'ext': m.group(3), 'lang': m.group(5), 'file_lang': m.group(2),
                  'type': m.group(4), 'in_if': pak_is_in_if, 'line': l }
            if c is not None:
              x['comment'] = c
              c = None
            k = m.group(2) if m.group(2) != 'nb' else 'no'
            chunks[k] = x
          else:
            if c is None:
              c = l
            else:
              c += l
        is_in_if = False
        for lang in sorted(chunks.keys()):
          tlang = lang if lang != 'no' else 'nb'
          while len(ours) > 0 and ((ours[0] == 'nb' and 'no' < tlang) or (ours[0] != 'nb' and ours[0] < tlang)):
            if has_ifs and is_in_if is False:
              fdo.write('    <if expr="pp_ifdef(\'use_third_party_translations\')">\n')
            f = "%s_%s.%s" % (chunks[lang]['name'], ours[0], chunks[lang]['ext'])
            fdo.write('    %s<output filename="%s" type="%s" lang="%s" />\n' % \
                        ('  ' if has_ifs else '', f, chunks[lang]['type'], ours[0]))
            is_in_if = True
            if has_ifs and chunks[lang]['in_if'] is False:
              fdo.write('    </if>\n')
              is_in_if = False
            ours = ours[1:]
          if 'comment' in chunks[lang]:
            for s in chunks[lang]['comment'].split('\n')[:-1]:
              if chunks[lang]['in_if'] is True and is_in_if and s.find('<if ') > -1:
                continue
              if s.find('<!-- No translations available. -->') > -1:
                continue
              fdo.write(s + '\n')
          fdo.write(chunks[lang]['line'])
          ours = ours[1:]
          is_in_if = chunks[lang]['in_if']
        if len(chunks.keys()) > 0:
          while len(ours) > 0:
            f = "%s_%s.%s" % (chunks[lang]['name'], ours[0], chunks[lang]['ext'])
            if has_ifs and is_in_if is False:
              fdo.write('    <if expr="pp_ifdef(\'use_third_party_translations\')">\n')
            fdo.write('    %s<output filename="%s" type="data_package" lang="%s" />\n' % \
                        ('  ' if has_ifs else '', f, ours[0]))
            is_in_if = True
            ours = ours[1:]
          if has_ifs and is_in_if:
            fdo.write('    </if>\n')
            is_in_if = False
        if c is not None:
          for s in c.split('\n')[:-1]:
            if s.find('<!-- No translations available. -->') > -1:
              continue
            if s.find('</if>') > -1:
              continue
            fdo.write(s + '\n')
      if pak_found:
        pak_saved.append(line)
        continue
      fdo.write(line)
    fdi.close()
    fdo.close()

  def create_build_gyp_patch(self, directory, build_gyp_file, other_grd_files, nlangs,
                             whitelisted_new_langs = None):
    # read the list of langs supported upstream
    fd = open(build_gyp_file, "r")
    data = fd.read()
    fd.close()
    r = data[data.find("'locales':"):]
    olangs = sorted(re.findall("'(.*?)'", r[r.find('['):r.find(']')]))
    # check for an optional use_third_party_translations list of locales
    tpt = data.find('use_third_party_translations==1')
    if tpt > 0:
      tpt += data[tpt:].find("'locales':")
      r = data[tpt:]
      tptlangs = sorted(re.findall("'(.*?)'", r[r.find('['):r.find(']')]))
      if nlangs == sorted(tptlangs + olangs):
        return tptlangs
    else:  
      if nlangs == olangs:
        return []
    # check if we need to only activate some whitelisted new langs
    xlangs = None
    nnlangs = [ x for x in nlangs if x not in olangs ]
    if whitelisted_new_langs is not None:
      if tpt > 0:
        nlangs = [ x for x in nlangs if x not in olangs and x in whitelisted_new_langs ]
      else:
        xlangs = [ x for x in nlangs if x not in olangs and x not in whitelisted_new_langs ]
        nlangs = [ x for x in nlangs if x in olangs or x in whitelisted_new_langs ]
    elif tpt > 0:
      nlangs = [ x for x in nlangs if x not in olangs ]

    # we need a patch
    if tpt > 0:
      pos = tpt + data[tpt:].find('[')
      end = data[:pos + 1]
      ndata = end[:]
    else:
      pos = data.find("'locales':")
      begin = data[pos:]
      end = data[:pos + begin.find('\n')]
      ndata = end[:]
    end = data[pos + data[pos:].find(']'):]

    # list of langs, by chunks of 10
    if len(nlangs) > 10:
      chunks = map(lambda i: nlangs[i:i + 10], xrange(0, len(nlangs), 10))
      ndata += '\n'
      for chunk in chunks:
        ndata += "      %s'%s',\n" % ('    ' if tpt > 0 else '', "', '".join(chunk))
      ndata += '    %s' % '    ' if tpt > 0 else ''
    else:
      ndata += "'%s'" % "', '".join(nlangs)

    ndata += end

    # write the patch
    fromfile  = "old/" + build_gyp_file
    tofile    = "new/" + build_gyp_file
    fromdate  = datetime.fromtimestamp(self.template.get_mtime(build_gyp_file)).strftime("%Y-%m-%d %H:%M:%S.%f000 +0000")
    fromlines = [ x for x in re.split('(.*\n?)', data) if x != '' ]
    todate    = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f000 +0000")
    tolines   = [ x for x in re.split('(.*\n?)', ndata) if x != '' ]
    patch = codecs.open(os.path.join(directory, "build.patch"), "wb", encoding="utf-8")
    diff = unified_diff(fromlines, tolines, fromfile, tofile, fromdate, todate, n=3)
    patch.write("diff -Nur %s %s\n" % (fromfile, tofile))
    patch.writelines(''.join(diff))

    for grd in other_grd_files:
      grd_out = os.path.join(directory, os.path.basename(grd))
      self.update_supported_langs_in_grd(grd, grd_out, langs)
      if filecmp.cmp(grd, grd_out) == True:
        os.unlink(grd_out)
        continue # files are the same
      # add it to the patch
      fromfile  = "old/" + grd
      tofile    = "new/" + grd
      fromdate  = datetime.fromtimestamp(self.template.get_mtime(grd)).strftime("%Y-%m-%d %H:%M:%S.%f000 +0000")
      fromlines = codecs.open(grd, 'rb', encoding="utf-8").readlines()
      todate    = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f000 +0000")
      tolines   = codecs.open(grd_out, 'rb', encoding="utf-8").readlines()
      diff = unified_diff(fromlines, tolines, fromfile, tofile, fromdate, todate, n=3)
      patch.write("diff -Nur %s %s\n" % (fromfile, tofile))
      patch.writelines(''.join(diff))
      os.unlink(grd_out)
    patch.close()
    return nnlangs

def process_grd_diff(grd_diff):
  print 'process_grd_diff ' + str(grd_diff)
  lines = codecs.open(grd_diff, 'rb', encoding="utf-8").readlines()
  state = 'end'
  message = ''
  for line in lines:
    if re.search('\<message ', line) and state == 'end':
      state = 'begin'
      message = line
      continue
    if re.search('^\-', line) and state == 'begin':
      state = 'begin-'
      continue
    if re.search('^\+', line) and state == 'begin-':
      state = 'begin-+'
      continue
    if re.search('\<\/message\>', line) and state == 'begin-+':
      result = re.findall('name\=\".*\" desc=', message)
      #print message[result.start():result.end()]
      id = result[0][6:-7]
      print id
      id_set.add(id)
      state = 'end'
      continue
    state = 'end'
    pass
  #print id_set
  pass

def usage():
  print """
Usage: %s [options] [grd_file [more_grd_files]]

  Convert Chromium translation files (grd/xtb) into gettext files (pot/po) and back

  options could be:
    -d | --debug      debug mode
    -v | --verbose    verbose mode
    -h | --help       this help screen

    --export-gettext dir
                      export pot/po gettext files to dir

    --import-gettext dir[,dir2][...]
                      import gettext pot/po files from those directories.
                      Directories must be ordered from the oldest to
                      the freshest. Only strings different from the grit
                      (upstream) translations are considered.

    --import-grit-branch name:dir:grd1[,grd2,...]]
                      import the Grit files for this branch from this
                      directory. --import-grit-branch could be used several
                      times, and then, branches must be specified from the
                      freshest (trunk) to the more stable ones.
                      The default value is trunk:<cwd>
                      Note: must not be used along with --export-grit

    --export-grit dir
                      export grd/xtb grit files to dir

    --copy-grit dir   copy the src grit files containing strings to dir
                      (useful to create diffs after --export-grit)

    --whitelisted-new-langs lang1[,lang2][..]
                      comma separated list of new langs that have to be enabled
                      (assuming they have some strings translated). The default
                      is to enable all new langs, but for stable builds, a good
                      enough coverage is preferred

    --create-patches dir
                      create unified patches per template in dir
                      (only useful after --export-grit)

    --build-gyp-file file
                      location of the build/common.gypi file, used only
                      with --create-patches to add all new langs
                      for which we merged translated strings

    --other-grd-files file1[,file2][..]
                      comma separated list of grd files to also patch
                      to add new langs for (see --build-gyp-file)

    --html-output     produce nice some HTML as output (on stdout)

    --json-branches-info file
                      location of a json file containing the url, revision
                      and last date of both the upstream branch and
                      launchpad export. optionally used in the html output

    --landable-templates template1[,template2][...]
                      comma separated list of templates that are landable upstream
                      for all langs

    --unlandable-templates template1[,template2][...]
                      comma separated list of templates that are not landable upstream,
                      even for new langs

    --test-strcvt     run the grit2gettext2grit checker
    --test-conditions run the conditions evaluation checker

""" % sys.argv[0].rpartition('/')[2]

if '__main__' == __name__:
  sys.stdout = codecs.getwriter('utf8')(sys.stdout)
  try:
    opts, args = getopt.getopt(sys.argv[1:], "dhv",
                               [ "test-strcvt", "test-conditions", "debug", "verbose", "help", "copy-grit=",
                                 "import-grit-branch=", "export-gettext=", "import-gettext=", "export-grit=",
                                 "create-patches=", "build-gyp-file=", "other-grd-files=",
                                 "landable-templates=", "unlandable-templates=",
                                 "whitelisted-new-langs=", "html-output", "json-branches-info=", "grd-diff="])
  except getopt.GetoptError, err:
    print str(err)
    usage()
    sys.exit(2)

  verbose = False
  debug = False
  html_output = False
  outdir = None
  indir = None
  export_gettext = None
  import_gettext = None
  export_grit    = None
  copy_grit      = None
  create_patches = None
  build_gyp_file = None
  json_info      = None
  other_grd_files = []
  whitelisted_new_langs = None
  grd_diff = None
  landable_templates = []
  unlandable_templates = []
  branches       = None
  for o, a in opts:
    if o in ("-v", "--verbose"):
      verbose = True
    elif o in ("-h", "--help"):
      usage()
      sys.exit()
    elif o in ("-d", "--debug"):
      debug = True
    elif o == "--import-grit-branch":
      if branches is None:
        branches = {}
      branch, dir, grds = a.split(':')
      for grd in grds.split(','):
        name = os.path.basename(grd)
        if name not in branches:
          branches[name] = []
        branches[name].append({ 'branch': branch, 'dir': dir, 'grd': grd })
    elif o == "--export-gettext":
      export_gettext = a
    elif o == "--import-gettext":
      import_gettext = a.split(",")
    elif o == "--export-grit":
      export_grit = a
    elif o == "--copy-grit":
      copy_grit = a
    elif o == "--whitelisted-new-langs":
      whitelisted_new_langs = a.split(",")
    elif o == "--create-patches":
      create_patches = a
    elif o == "--build-gyp-file":
      build_gyp_file = a
    elif o == "--other-grd-files":
      other_grd_files = a.split(',')
    elif o == "--html-output":
      html_output = True
    elif o == "--json-branches-info":
      json_info = a
    elif o == "--landable-templates":
      landable_templates = a.split(",")
    elif o == "--unlandable-templates":
      unlandable_templates = a.split(",")
    elif o == "--test-strcvt":
      StringCvt().test()
      sys.exit()
    elif o == "--test-conditions":
      EvalConditions().test()
      sys.exit()
    elif o == "--grd-diff":
      grd_diff = a
    else:
      assert False, "unhandled option"
      
  if branches is None and len(args) != 0:
    branches = {}
    for arg in args:
      branches[os.path.basename(arg)] = [ { 'branch': 'default', 'dir': os.getcwd(), 'grd': arg } ]
  if branches is None:
    print "Please specify at least one grd file or use --import-grit-branch"
    usage()
    sys.exit(2)

  if html_output:
    print """\
<html>
<head><meta charset="UTF-8">
</head><body>
<style type="text/css">
body {
  font-family: UbuntuBeta,Ubuntu,"Bitstream Vera Sans","DejaVu Sans",Tahoma,sans-serif;
}
div#legend {
  float: left;
}
fieldset {
  border-width: 1px;
  border-color: #f0f0f0;
}
div#legend fieldset, div#branches fieldset {
  border-width: 0px;
}
legend {
  font-size: 80%;
}
div#branches {
  float: left;
  padding-left: 40px;
}
div#branches td {
  padding-right: 5px;
}
div#stats {
  padding-top: 5px;
  clear: both;
}
a {
  text-decoration: none;
}
a.l:link, a.l:visited {
  color: black;
}
.error {
  font-size: 90%;
}
div.error a {
  font-family: monospace;
  font-size: 120%;
}
table {
  border-collapse: collapse;
  border-spacing: 1px;
  font-size: 0.9em;
}
th {
  font-weight: bold;
  color: #666;
  padding-right: 5px;
}
th, td {
  border: 1px #d2d2d2;
  border-style: solid;
  padding-left: 4px;
  padding-top: 0px;
  padding-bottom: 0px;
}
td.d {
  font-size: 90%;
  text-align: right;
}
td.n {
  background: #FFA;
}
.lang {
  font-weight: bold;
  padding-left: 0.5em;
  padding-right: 0.5em;
  white-space: nowrap;
}
.progress_bar {
  width: 100px; overflow: hidden; position: relative; padding: 0px;
}
.pb_label {
  text-align: center; width: 100%;
  position: absolute; z-index: 1001; left: 4px; top: -2px; color: white; font-size: 0.7em;
}
.pb_label2 {
  text-align: center; width: 100%;
  position: absolute; z-index: 1000; left: 5px; top: -1px; color: black; font-size: 0.7em;
}
.green_gradient {
  height: 1em; position: relative; float: left;
  background: #00ff00;
  background: -moz-linear-gradient(top, #00ff00, #007700);
  background: -webkit-gradient(linear, left top, left bottom, from(#00ff00), to(#007700));
  filter: progid:DXImageTransform.Microsoft.Gradient(StartColorStr='#00ff00', EndColorStr='#007700', GradientType=0);
}
.red_gradient {
  height: 1em; position: relative; float: left;
  background: #ff8888;
  background: -moz-linear-gradient(top, #ff8888, #771111);
  background: -webkit-gradient(linear, left top, left bottom, from(#ff8888), to(#771111));
  filter: progid:DXImageTransform.Microsoft.Gradient(StartColorStr='#ff8888', EndColorStr='#771111', GradientType=0);
}
.blue_gradient {
  height: 1em; position: relative; float: left;
  background: #62b0dd;
  background: -moz-linear-gradient(top, #62b0dd, #1f3d4a);
  background: -webkit-gradient(linear, left top, left bottom, from(#62b0dd), to(#1f3d4a));
  filter: progid:DXImageTransform.Microsoft.Gradient(StartColorStr='#62b0dd', EndColorStr='#1f3d4a', GradientType=0);
}
.purple_gradient {
  height: 1em; position: relative; float: left;
  background: #b8a4ba;
  background: -moz-linear-gradient(top, #b8a4ba, #5c3765);
  background: -webkit-gradient(linear, left top, left bottom, from(#b8a4ba), to(#5c3765));
  filter: progid:DXImageTransform.Microsoft.Gradient(StartColorStr='#b8a4ba', EndColorStr='#5c3765', GradientType=0);
}
</style>
<script type="text/javascript" language="javascript">
function progress_bar(where, red, green, purple, blue) {
  var total = green + red + blue + purple;
  if (total == 0)
    total = 1;
  var d = document.getElementById(where);
  var v = 100 * (1 - (red / total));
  if (total != 1) {
    d.innerHTML += '<div class="pb_label">' + v.toFixed(1) + "%</div>";
    d.innerHTML += '<div class="pb_label2">' + v.toFixed(1) + "%</div>";
  }
  else
    d.style.width = "25px";
  var pgreen  = parseInt(100 * green / total);
  var pblue   = parseInt(100 * blue / total);
  var ppurple = parseInt(100 * purple / total);
  var pred    = parseInt(100 * red / total);
  if (pgreen + pblue + ppurple + pred != 100) {
    if (red > 0)
      pred = 100 - pgreen - pblue - ppurple;
    else if (purple > 0)
      ppurple = 100 - pgreen - pblue;
    else if (blue > 0)
      pblue = 100 - pgreen;
    else
      pgreen = 100;
  }
  if (green > 0)
    d.innerHTML += '<div class="green_gradient" style="width:' + pgreen + '%;"></div>';
  if (blue > 0)
    d.innerHTML += '<div class="blue_gradient" style="width:' + pblue + '%;"></div>';
  if (purple > 0)
    d.innerHTML += '<div class="purple_gradient" style="width:' + ppurple + '%;"></div>';
  if (red > 0)
    d.innerHTML += '<div class="red_gradient" style="width:' + pred + '%;"></div>';
  return true;
}

function toggle(e) {
  var elt = document.getElementById(e + "-t");
  var text = document.getElementById(e);
  if (elt.style.display == "block") {
    elt.style.display = "none";
    text.innerHTML = "+";
  }
  else {
    elt.style.display = "block";
    text.innerHTML = "-";
  }
}

function time_delta(date, e) {
  var now = new Date();
  var d = new Date(date);
  var delta = (now - d) / 1000;
  var elt = document.getElementById(e);
  if (delta >= 3600) {
    var h = parseInt(delta / 3600);
    var m = parseInt((delta - h * 3600) / 60);
    elt.innerHTML = '(' + h + 'h ' + m + 'min ago)';
    return;
  }
  if (delta >= 60) {
    var m = parseInt(delta / 60);
    elt.innerHTML = '(' + m + 'min ago)';
    return;
  }
  elt.innerHTML = '(seconds ago)';
}

</script>
"""

  if grd_diff:
    process_grd_diff(grd_diff)

  prefix = os.path.commonprefix([ branches[x][0]['grd'] for x in branches.keys() ])
  changes = 0
  langs = []
  mapped_langs = {}
  cvts = {}
  for grd in branches.keys():
    cvts[grd] = Converter(branches[grd][0]['grd'], lang_mapping = lang_mapping, debug = debug, html_output = html_output,
                          branches = branches[grd])

    if cvts[grd].get_supported_strings_count() == 0:
      if debug:
        print "no string found in %s" % grd
      if export_grit is not None and copy_grit is None:
        directory =  os.path.join(export_grit, os.path.dirname(branches[grd][0]['grd'])[len(prefix):])
        if not os.path.isdir(directory):
          os.makedirs(directory, 0755)
        shutil.copy2(branches[grd][0]['grd'], directory)
      continue

    if copy_grit is not None:
      cvts[grd].copy_grit_files(copy_grit)

    if import_gettext is not None:
      for directory in import_gettext:
        cvts[grd].import_gettext_po_files(directory)
        langs.extend(cvts[grd].translations.keys())

    if export_gettext is not None:
      cvts[grd].export_gettext_files(export_gettext)
      changes += cvts[grd].template_changes + cvts[grd].translations_changes

  # as we need to add all supported langs to the <outputs> section of all grd files,
  # we have to wait for all the 'po' files to be imported and merged before we export
  # the grit files and create the patches.

  # supported langs
  langs.append('en-US') # special case, it's not translated, but needs to be here
  for lang in [ 'no' ]: # workaround for cases like the infamous no->nb mapping
    while lang in langs:
      langs.remove(lang)
      langs.append(lang_mapping[lang])
  r = {}
  langs = sorted([ r.setdefault(e, e) for e in langs if e not in r ])

  for grd in branches.keys():
    if export_grit is not None:
      cvts[grd].export_grit_files(os.path.join(export_grit, os.path.dirname(branches[grd][0]['grd'])[len(prefix):]), langs)
      for lang in cvts[grd].template.mapped_langs:
        mapped_langs[lang] = cvts[grd].template.mapped_langs[lang]['gettext']
      if create_patches is not None:
        cvts[grd].create_patches(create_patches)

  # patch the build/common.gypi file if we have to
  nlangs = None
  if create_patches is not None and build_gyp_file is not None:
    nlangs = cvts[branches.keys()[0]].create_build_gyp_patch(create_patches, build_gyp_file, other_grd_files, langs,
                                                  whitelisted_new_langs)

  if create_patches is None:
    # no need to display the stats
    exit(1 if changes > 0 else 0)

  # display some stats
  html_js = ""
  if html_output:
    print """
<p>
<div>
<div id="legend">
<fieldset><legend>Legend</legend>
<table border="0">
<tr><td><div id='green_l' class='progress_bar'></td><td>translated upstream</td></tr>
<tr><td><div id='blue_l' class='progress_bar'></td><td>translations updated in Launchpad</td></tr>
<tr><td><div id='purple_l' class='progress_bar'></td><td>translated in Launchpad</td></tr>
<tr><td><div id='red_l' class='progress_bar'></td><td>untranslated</td></tr>
</table>
</fieldset>
</div>
"""
    html_js += "progress_bar('%s', %d, %d, %d, %d);\n" % ('green_l', 0, 1, 0, 0)
    html_js += "progress_bar('%s', %d, %d, %d, %d);\n" % ('blue_l', 0, 0, 0, 1)
    html_js += "progress_bar('%s', %d, %d, %d, %d);\n" % ('purple_l', 0, 0, 1, 0)
    html_js += "progress_bar('%s', %d, %d, %d, %d);\n" % ('red_l', 1, 0, 0, 0)
    if json_info:
      now = datetime.utcfromtimestamp(os.path.getmtime(json_info)).strftime("%a %b %e %H:%M:%S UTC %Y")
      binfo = json.loads(open(json_info, "r").read())
      print """
<div id="branches">
<fieldset><legend>Last update info</legend>
<table border="0">
<tr><th>Branch</th><th>Revision</th><th>Date</th></tr>
<tr><td><a href="%s">Upstream</a></td><td>r%s</td><td>%s <em id='em-u'></em> </td></tr>
<tr><td><a href="%s">Launchpad export</a></td><td>r%s</td><td>%s <em id='em-lp'></em> </td></tr>
<tr><td>This page</a></td><td>-</td><td>%s <em id='em-now'></em> </td></tr>
</table>
</fieldset>
</div>
""" % (binfo['upstream']['url'], binfo['upstream']['revision'], binfo['upstream']['date'],
       binfo['launchpad-export']['url'], binfo['launchpad-export']['revision'],
       binfo['launchpad-export']['date'], now)
      html_js += "time_delta('%s', '%s');\n" % (binfo['upstream']['date'], 'em-u')
      html_js += "time_delta('%s', '%s');\n" % (binfo['launchpad-export']['date'], 'em-lp')
      html_js += "time_delta('%s', '%s');\n" % (now, 'em-now')
    print """
<div id="stats">
<table border="0">
<tr><th rowspan="2">Rank</th><th rowspan="2">Lang</th><th colspan='5'>TOTAL</th><th colspan='5'>"""
    print ("</th><th colspan='5'>".join([ "%s (<a href='http://git.chromium.org/gitweb/?p=chromium.git;a=history;f=%s;hb=HEAD'>+</a>)" \
                                            % (os.path.splitext(grd)[0], branches[grd][0]['grd']) \
                                            for grd in sorted(branches.keys()) ])) + "</th></tr><tr>"
    j = 0
    for grd in [ 'TOTAL' ] + sorted(branches.keys()):
      print """
<th>Status</th>
<th><div id='%s_t%d' class='progress_bar'></th>
<th><div id='%s_t%d' class='progress_bar'></th>
<th><div id='%s_t%d' class='progress_bar'></th>
<th><div id='%s_t%d' class='progress_bar'></th>""" % ('red', j, 'green', j, 'purple', j, 'blue', j)
      html_js += "progress_bar('%s_t%d', %d, %d, %d, %d);\n" % ('green', j, 0, 1, 0, 0)
      html_js += "progress_bar('%s_t%d', %d, %d, %d, %d);\n" % ('blue', j, 0, 0, 0, 1)
      html_js += "progress_bar('%s_t%d', %d, %d, %d, %d);\n" % ('purple', j, 0, 0, 1, 0)
      html_js += "progress_bar('%s_t%d', %d, %d, %d, %d);\n" % ('red', j, 1, 0, 0, 0)
      j += 1
    print "</tr>"
  else:
    print """\
               +----------------------- % translated
               |     +----------------- untranslated
               |     |    +------------ translated upstream
               |     |    |    +------- translated in Launchpad
               |     |    |    |    +-- translations updated in Launchpad
               |     |    |    |    |
               V     V    V    V    V"""
    print "-- lang --  " + \
      '     '.join([ (" %s " % os.path.splitext(grd)[0]).center(25, "-") \
                       for grd in [ 'TOTAL' ] + sorted(branches.keys()) ])
  totals = {}
  for lang in langs:
    klang = lang
    if lang == 'nb':
      klang = 'no'
    totals[klang] = { 'total': 0, 'missing': 0, 'translated_upstream': 0, 'new': 0, 'updated': 0, 'lskipped': 0 }
    for grd in branches.keys():
      tot, lskipped = cvts[grd].template.get_supported_strings_count(klang)
      totals[klang]['lskipped'] += lskipped
      totals[klang]['total'] += tot
      totals[klang]['missing'] += tot
      if klang in cvts[grd].template.stats:
        totals[klang]['missing'] -= cvts[grd].template.stats[klang]['translated_upstream'] + \
            cvts[grd].template.stats[klang]['new'] + cvts[grd].template.stats[klang]['updated']
        totals[klang]['translated_upstream'] += cvts[grd].template.stats[klang]['translated_upstream']
        totals[klang]['new'] += cvts[grd].template.stats[klang]['new']
        totals[klang]['updated'] += cvts[grd].template.stats[klang]['updated']

  rank = 0
  p_rank = 0
  p_score = -1
  t_landable = 0
  for lang in sorted(totals, lambda x, y: cmp("%05d %05d %s" % (totals[x]['missing'], totals[x]['total'] - totals[x]['updated'] - totals[x]['new'], x),
                                              "%05d %05d %s" % (totals[y]['missing'], totals[y]['total'] - totals[y]['updated'] - totals[y]['new'], y))):
    if lang == 'en-US':
      continue
    rank += 1
    if p_score != totals[lang]['missing']:
      p_score = totals[lang]['missing']
      p_rank = rank
    rlang = lang
    if lang in lang_mapping:
      rlang = lang_mapping[lang]
    if html_output:
      s = "<tr><td>%s</td><td class='lang'><a class='l' href='%s'>%s</a></td>" % \
          ("#%d" % p_rank, 'https://translations.launchpad.net/chromium-browser/translations/+lang/' + \
           mapped_langs[lang], rlang)
      s += "<td><div id='%s' class='progress_bar'></div></td>" % rlang
      s += "<td class='d'>%d</td><td class='d'>%d</td><td class='d'>%d</td><td class='d'>%d</td>" % \
          (totals[lang]['missing'], totals[lang]['translated_upstream'],
           totals[lang]['new'], totals[lang]['updated'])
      html_js += "progress_bar('%s', %d, %d, %d, %d);\n" % \
          (rlang, totals[lang]['missing'], totals[lang]['translated_upstream'],
           totals[lang]['new'], totals[lang]['updated'])
    else:
      s = "%-3s  %-6s " % ("#%d" % p_rank, rlang)
      s += "%3d%%  %4d %4d %4d %4d" % \
          (100.0 * float(totals[lang]['total'] - totals[lang]['missing']) / float(totals[lang]['total']),
           totals[lang]['missing'], totals[lang]['translated_upstream'],
           totals[lang]['new'], totals[lang]['updated'])
    j = 0
    for grd in sorted(branches.keys()):
      j += 1
      tplt = os.path.splitext(grd)[0].replace('_', '-')
      total, lskipped = cvts[grd].template.get_supported_strings_count(lang)
      if lang in cvts[grd].template.stats:
        missing = total - cvts[grd].template.stats[lang]['translated_upstream'] - \
            cvts[grd].template.stats[lang]['new'] - cvts[grd].template.stats[lang]['updated']
        if html_output:
          if len(unlandable_templates) == 0 and len(landable_templates) == 0:
            landable = False
          else:
            landable = (nlangs is not None and lang in nlangs and tplt not in unlandable_templates) or \
              (nlangs is not None and lang not in nlangs and tplt in landable_templates)
          if landable:
            t_landable += cvts[grd].template.stats[lang]['new'] + cvts[grd].template.stats[lang]['updated']
          s += "<td><div id='%s_%d' class='progress_bar'></div></td>" % (rlang, j)
          s += "<td class='d'>%d</td><td class='d'>%d</td><td class='d%s'>%d</td><td class='d%s'>%d</td>" % \
              (missing,
               cvts[grd].template.stats[lang]['translated_upstream'],
               " n" if landable and cvts[grd].template.stats[lang]['new'] > 0 else "",
               cvts[grd].template.stats[lang]['new'],
               " n" if landable and cvts[grd].template.stats[lang]['updated'] > 0 else "",
               cvts[grd].template.stats[lang]['updated'])
          html_js += "progress_bar('%s_%d', %d, %d, %d, %d);\n" % \
              (rlang, j, missing,
               cvts[grd].template.stats[lang]['translated_upstream'],
               cvts[grd].template.stats[lang]['new'],
               cvts[grd].template.stats[lang]['updated'])
        else:
          if float(total) > 0:
            pct = 100.0 * float(total - missing) / float(total)
          else:
            pct = 0
          s += "     %3d%%  %4d %4d %4d %4d" % \
              (pct, missing,
               cvts[grd].template.stats[lang]['translated_upstream'],
               cvts[grd].template.stats[lang]['new'],
               cvts[grd].template.stats[lang]['updated'])
      else:
        if html_output:
          s += "<td><div id='%s_%d' class='progress_bar'></div></td>" % (rlang, j)
          s += "<td class='d'>%d</td><td class='d'>%d</td><td class='d'>%d</td><td class='d'>%d</td>" % \
              (total, 0, 0, 0)
          html_js += "progress_bar('%s_%d', %d, %d, %d, %d);\n" % \
              (rlang, j, total, 0, 0, 0)
        else:
          s += "     %3d%%  %4d %4d %4d %4d" % (0, total, 0, 0, 0)
    if html_output:
      s += "</tr>"
    print s
  if html_output:
    landable_sum = ""
    if t_landable > 0:
       landable_sum = """<p>
<div name='landable'>
<table border="0"><tr><td class="d n">%d strings are landable upstream</td></tr></table></div>
""" % t_landable
    print """\
</table>
%s</div>
</div>
<script type="text/javascript" language="javascript">
%s
</script>
</body>
</html>""" % (landable_sum, html_js)
  exit(1 if changes > 0 else 0)

