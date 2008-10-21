# Copyright (c) 2001-2004 Twisted Matrix Laboratories.
# See LICENSE for details.

#
from __future__ import nested_scopes

import os, tempfile
from twisted.web import domhelpers, microdom
import latex, tree, lint, default

class MathLatexSpitter(latex.LatexSpitter):

    start_html = '\\documentclass{amsart}\n'

    def visitNode_div_latexmacros(self, node):
        self.writer(domhelpers.getNodeText(node))

    def visitNode_span_latexformula(self, node):
        self.writer('\[')
        self.writer(domhelpers.getNodeText(node))
        self.writer('\]')

def formulaeToImages(document, dir):
    # gather all macros
    macros = ''
    for node in domhelpers.findElementsWithAttribute(document, 'class',
                                                     'latexmacros'):
        macros += domhelpers.getNodeText(node)
        node.parentNode.removeChild(node)
    i = 0
    for node in domhelpers.findElementsWithAttribute(document, 'class',
                                                    'latexformula'):
        latexText='''\\documentclass[12pt]{amsart}%s
                     \\begin{document}\[%s\]
                     \\end{document}''' % (macros, domhelpers.getNodeText(node))
        file = tempfile.mktemp()
        open(file+'.tex', 'w').write(latexText)
        os.system('latex %s.tex' % file)
        os.system('dvips %s.dvi -o %s.ps' % (os.path.basename(file), file))
        baseimgname = 'latexformula%d.png' % i
        imgname = os.path.join(dir, baseimgname)
        i += 1
        os.system('pstoimg -type png -crop a -trans -interlace -out '
                  '%s %s.ps' % (imgname, file))
        newNode = microdom.parseString('<span><br /><img src="%s" /><br /></span>' %
                                       baseimgname)
        node.parentNode.replaceChild(newNode, node)


def doFile(fn, docsdir, ext, url, templ, linkrel='', d=None):
    d = d or {}
    doc = tree.parseFileAndReport(fn)
    formulaeToImages(doc, os.path.dirname(fn))
    cn = templ.cloneNode(1)
    tree.munge(doc, cn, linkrel, docsdir, fn, ext, url, d)
    cn.writexml(open(os.path.splitext(fn)[0]+ext, 'wb'))


class ProcessingFunctionFactory(default.ProcessingFunctionFactory):

    latexSpitters = {None: MathLatexSpitter}

    def getDoFile(self):
        return doFile

    def getLintChecker(self):
        checker = lint.getDefaultChecker()
        checker.allowedClasses = checker.allowedClasses.copy()
        oldDiv = checker.allowedClasses['div']
        oldSpan = checker.allowedClasses['span']
        checker.allowedClasses['div'] = lambda x:oldDiv(x) or x=='latexmacros'
        checker.allowedClasses['span'] = (lambda x:oldSpan(x) or
                                                     x=='latexformula')
        return checker

factory = ProcessingFunctionFactory()
