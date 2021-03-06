# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'conditions': [
      [ 'OS=="linux" or OS=="freebsd" or OS=="openbsd"', {
        # Link to system .so since we already use it due to GTK.
        'use_system_libjpeg%': 1,
      }, {  # OS!="linux" and OS!="freebsd" and OS!="openbsd"
        'use_system_libjpeg%': 0,
      }],
    ],
  },
  'conditions': [
    ['use_system_libjpeg==0', {
      'targets': [
        {
          'target_name': 'libjpeg',
          'type': '<(library)',
          'msvs_guid': '238CE175-76CE-4A25-A676-69D115885601',
          'sources': [
            'jcapimin.c',
            'jcapistd.c',
            'jccoefct.c',
            'jccolor.c',
            'jcdctmgr.c',
            'jchuff.c',
            'jchuff.h',
            'jcinit.c',
            'jcmainct.c',
            'jcmarker.c',
            'jcmaster.c',
            'jcomapi.c',
            'jconfig.h',
            'jcparam.c',
            'jcphuff.c',
            'jcprepct.c',
            'jcsample.c',
            'jdapimin.c',
            'jdapistd.c',
            'jdatadst.c',
            'jdatasrc.c',
            'jdcoefct.c',
            'jdcolor.c',
            'jdct.h',
            'jddctmgr.c',
            'jdhuff.c',
            'jdhuff.h',
            'jdinput.c',
            'jdmainct.c',
            'jdmarker.c',
            'jdmaster.c',
            'jdmerge.c',
            'jdphuff.c',
            'jdpostct.c',
            'jdsample.c',
            'jerror.c',
            'jerror.h',
            'jfdctflt.c',
            'jfdctfst.c',
            'jfdctint.c',
            'jidctflt.c',
            'jidctfst.c',
            'jidctint.c',
            'jinclude.h',
            'jmemmgr.c',
            'jmemnobs.c',
            'jmemsys.h',
            'jmorecfg.h',
            'jpegint.h',
            'jpeglib.h',
            'jquant1.c',
            'jquant2.c',
            'jutils.c',
            'jversion.h',
          ],
          'direct_dependent_settings': {
            'include_dirs': [
              '.',
            ],
          },
          'conditions': [
            ['OS!="win"', {'product_name': 'jpeg'}],
          ],
        },
      ],
    }, {
      'targets': [
        {
          'target_name': 'libjpeg',
          'type': 'settings',
          'direct_dependent_settings': {
            'defines': [
              'USE_SYSTEM_LIBJPEG',
            ],
          },
          'link_settings': {
            'libraries': [
              '-ljpeg',
            ],
          },
        }
      ],
    }],
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
