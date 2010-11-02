# Copyright (c) 2010 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'includes': [
    '../../build/common.gypi',
    '../../ceee/common.gypi',
    'xpi_file_list.gypi',
  ],
  'targets': [
    {
      'target_name': 'xpi',
      'type': 'none',
      'msvs_guid': 'C697F5CD-04EB-4260-95D4-E051258809B2',
      'dependencies': [
        'create_xpi',
        '../ie/plugin/scripting/scripting.gyp:javascript_bindings',
      ],
      'sources': [
        'zipfiles.py',
        '<(SHARED_INTERMEDIATE_DIR)/ceee_ff.xpi',
        '<(SHARED_INTERMEDIATE_DIR)/event_bindings.js',
        '<(SHARED_INTERMEDIATE_DIR)/renderer_extension_bindings.js',
      ],
      'actions': [
        {
          'action_name': 'append_xpi',
          'msvs_cygwin_shell': 0,
          'inputs': [
            '<@(_sources)',
          ],
          'outputs': [
            '$(OutDir)/ceee_ff.xpi',
          ],
          'action': [
            '<@(python)',
            '$(ProjectDir)\zipfiles.py',
            '-i',
            '<(SHARED_INTERMEDIATE_DIR)/ceee_ff.xpi',
            '-o',
            '<(_outputs)',
            '-p',
            'content/us',
            '<(SHARED_INTERMEDIATE_DIR)/event_bindings.js',
            '<(SHARED_INTERMEDIATE_DIR)/renderer_extension_bindings.js',
          ],
        },
      ],
    },
    {
      'target_name': 'create_xpi',
      'type': 'none',
      # TODO(rogerta@chromium.org): This needs to be conditional on
      # configuration.  We should only include _xpi_test_files for
      # debug builds, or when we want to test the add-on.
      'sources': [
        'zipfiles.py',
        '<@(_xpi_files)',
        '<@(_xpi_test_files)',
      ],
      'actions': [
        {
          'action_name': 'create_xpi',
          'msvs_cygwin_shell': 0,
          'inputs': [
            '<@(_sources)',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/ceee_ff.xpi',
          ],
          'action': [
            # Because gyp automatically wraps all strings below in quotes,
            # we need to keep command line arguments separate so that they
            # don't get merged together in one quoted string.  Below, we want
            # -o and >(_outputs) to be seen as two command line arguments, not
            # one of the form "-o >(_outputs)".
            '<@(python)',
            '$(ProjectDir)\zipfiles.py',
            '-o',
            '<(_outputs)',
            '<@(_xpi_files)',
            '<@(_xpi_test_files)',
          ],
        },
      ],
    },
  ],
}
