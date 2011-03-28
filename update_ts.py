#!/usr/bin/python
# -*- coding: utf-8 -*-

# This script generates TS file for meego-app-bowser project
# 1. For C++ files, it converts the modified strings in .grd to .ts file
# 2. For QML files, it updates the TS file by lupdate
# 3. Combine above two ts file to one
#
# It depends on chromium2pot.py, git, msgcat (gettext) and lupdate, lconvert (qt-devel-tools)

import os, glob, shutil, re, codecs
from xml.dom import minidom

base_commit = "3435f87ff009abf0f552da83900fafed4e4199b0"

grd_files = [#'chrome/app/generated_resources.grd',
             'chrome/app/chromium_strings.grd']

ts_dir = './ts'
po_dir = './po'

mtb_name = 'meego-app-browser'

mtb_po_dir = po_dir + '/' + mtb_name

grd_diff = './modified_strings.diff'

qml_dir = './chrome/browser/qt/*'

qml_ts = 'qml.ts'

def diffgrd():
    i = 0;
    for f in grd_files:
        redirect = ' >> '
        if f == grd_files[0]:
            redirect = ' > '
        cmd = 'git diff ' + base_commit + '..HEAD ' + f + redirect + grd_diff
        print cmd
        os.system(cmd)
        pass
    pass

def grd2po():
    for f in grd_files:
        cmd = 'python chromium2pot.py' + ' --grd-diff ' + grd_diff + ' --export-gettext ' + po_dir + ' ' + f
        print cmd
        os.system(cmd)
        pass
    pass

def merge_po_files():
    po_dirs = os.listdir(po_dir)
    pot_files = []
    po_files = {'': []}
    for d in po_dirs:
        os.chdir(po_dir + '/' + d)
        pot_files += [os.path.realpath(f) for f in glob.glob('*.pot')]
        local_po_files = glob.glob('*.po')
        for po in local_po_files:
            (name, ext) = os.path.splitext(po)
            if (po_files.has_key(name)):
                po_files[name].append(os.path.realpath(po))
            else:
                po_files[name] = [os.path.realpath(po)]
        os.chdir('../..')
        pass

    msgcat_cmd = 'msgcat --use-first ' + ' '.join(pot_files) + ' -o ' + mtb_po_dir + '/' + mtb_name + '.pot'
    print msgcat_cmd
    os.system(msgcat_cmd)

    for lang in po_files.keys():
        if (lang != ""):
            msgcat_cmd = 'msgcat --use-first ' + ' '.join(po_files[lang]) + ' -o ' + mtb_po_dir + '/' + mtb_name + '_' + lang + '.po'
            print msgcat_cmd
            os.system(msgcat_cmd)
            pass
        pass
    pass

def po2ts():
    mtb_po_files = []
    os.chdir(mtb_po_dir)
    mtb_po_files += [os.path.realpath(f) for f in glob.glob('*.pot')]
    mtb_po_files += [os.path.realpath(f) for f in glob.glob('*.po')]
    os.chdir('../..')
    for po in mtb_po_files:
        (name, ext) = os.path.splitext(os.path.split(po)[1])
        lconvert_cmd = 'lconvert' + ' -i ' + po + ' -o ' + ts_dir + '/' + name + '.ts'
        print lconvert_cmd
        os.system(lconvert_cmd)
    pass

def replace_ts_placeholder(ts):
    f = open(ts, 'r+')
    lines = []
    try:
        for line in f:
            i = 1
            while(re.search('%\{[^\}]*\}', line)):
                line = re.sub('%\{[^\}]*\}', '$%d' %i, line, 1)
                i = i + 1
                pass
            #print line
            lines += line
    finally:
        content = ''.join(lines)
        f.truncate(0)
        f.seek(0)
        f.write(content)
        print 'replace_placeholder ' + ts
        f.close()
    pass

def replace_placeholder():
    os.chdir(ts_dir)
    for ts in glob.glob('*.ts'):
        replace_ts_placeholder(ts)
        pass
    os.chdir('..')
    pass

def update_QML_ts():
    cmd = 'lupdate ' + qml_dir + ' -ts ' + ts_dir + '/' + qml_ts
    os.system(cmd)
    pass

def merge_ts():
    qml_dom = minidom.parse(ts_dir + '/' + qml_ts)
    chromium_dom = minidom.parse(ts_dir + '/' + mtb_name + '.ts')
    qml_context = qml_dom.getElementsByTagName('context')
    chromium_ts_node = chromium_dom.getElementsByTagName('TS')[0]
    for node in qml_context:
        chromium_ts_node.appendChild(node)
    ts_file = codecs.open(ts_dir + '/' + mtb_name + '.ts', 'wb', encoding='utf-8')
    chromium_dom.writexml(ts_file);
    ts_file.close();
    pass

if '__main__' == __name__:
    print 'generate grd diff...'
    try:
        os.remove(grd_diff)
    except:
        pass
    diffgrd()
    
    print 'parepare po dir...'
    try:
        shutil.rmtree(po_dir)
    except:
        pass
    try:
        os.mkdir(po_dir)
    except:
        pass
    grd2po()

    print 'parepare po dir for mtb...'
    try:
        shutil.rmtree(po_dir + '/' + mtb_name)
    except:
        pass
    try:
        os.mkdir(po_dir + '/' + mtb_name)
    except:
        pass
    merge_po_files()

    print 'prepare ts dir...'
    try:
        shutil.rmtree(ts_dir)
    except:
        pass
    try:
        os.mkdir(ts_dir)
    except:
        pass
    po2ts()
    replace_placeholder()

    print 'update QML ts...'
    update_QML_ts()
    
    print 'merge ts files...'
    merge_ts()
    print 'done!'
    pass
