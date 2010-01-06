# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'es_util',
      'type': 'static_library',
      'dependencies': [
        '../../gpu/gpu.gyp:gles2_c_lib',
      ],
      'include_dirs': [
        'Common/Include',
      ],
      'all_dependent_settings': {
        'include_dirs': [
          'Common/Include',
        ],
      },
      'sources': [
        'Common/Include/esUtil.h',
        'Common/Include/esUtil_win.h',
        'Common/Source/esShader.c',
        'Common/Source/esShapes.c',
        'Common/Source/esTransform.c',
        'Common/Source/esUtil.c',
        'Common/Source/Win32/esUtil_TGA.c',
      ],
    },
    {
      'target_name': 'hello_triangle',
      'type': 'static_library',
      'dependencies': [
        'es_util',
      ],
      'sources': [
        'Chapter_2/Hello_Triangle/Hello_Triangle.c',
        'Chapter_2/Hello_Triangle/Hello_Triangle.h',
      ],
    },
  ]
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
