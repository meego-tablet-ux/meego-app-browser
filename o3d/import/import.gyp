# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'includes': [
    '../build/common.gypi',
  ],
  'target_defaults': {
    'include_dirs': [
      '..',
      '../..',
      '../../<(cgdir)/include',
      '../../<(gtestdir)',
    ],
  },
  'targets': [
    {
      'target_name': 'o3dImport',
      'type': 'static_library',
      'dependencies': [
        'archive.gyp:o3dArchive',
        '../../<(antlrdir)/antlr.gyp:antlr3c',
        '../../<(fcolladadir)/fcollada.gyp:fcollada',
        '../../<(jpegdir)/libjpeg.gyp:libjpeg',
        '../../<(pngdir)/libpng.gyp:libpng',
        '../../<(zlibdir)/zlib.gyp:zlib',
        '../compiler/technique/technique.gyp:technique',
      ],
      'sources': [
        'cross/collada_conditioner.cc',
        'cross/collada_conditioner.h',
        'cross/collada.cc',
        'cross/collada.h',
        'cross/collada_zip_archive.cc',
        'cross/collada_zip_archive.h',
        'cross/destination_buffer.cc',
        'cross/destination_buffer.h',
        'cross/file_output_stream_processor.cc',
        'cross/file_output_stream_processor.h',
        'cross/precompile.h',
        'cross/tar_generator.cc',
        'cross/tar_generator.h',
        'cross/targz_generator.cc',
        'cross/targz_generator.h',
        'cross/zip_archive.cc',
        'cross/zip_archive.h',
      ],
      'conditions' : [
        ['renderer == "d3d9" and OS == "win"',
          {
            'include_dirs': [
              '$(DXSDK_DIR)/Include',
            ],
          }
        ],
        ['OS == "win"',
          {
            'sources': [
              'win/collada_conditioner_win.cc',
            ],
          },
        ],
        ['OS == "mac"',
          {
            'sources': [
              'mac/collada_conditioner_mac.mm',
            ],
            'link_settings': {
              'libraries': [
                '$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
              ],
            },
          },
        ],
        ['OS == "linux"',
          {
            'sources': [
              'linux/collada_conditioner_linux.cc',
            ],
          },
        ],
      ],
    },
    {
      'target_name': 'o3dImportTest',
      'type': 'none',
      'direct_dependent_settings': {
        'sources': [
          'cross/tar_generator_test.cc',
          'cross/targz_generator_test.cc',
        ],
      },
    },
  ],
}
