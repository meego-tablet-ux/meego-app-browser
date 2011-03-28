# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'conditions': [
    ['OS=="win" or OS=="mac" or OS=="linux"', {
      'targets': [
        {
          # policy_templates has different inputs and outputs, so it can't use
          # the rules of chrome_strings
          'target_name': 'policy_templates',
          'type': 'none',
          'variables': {
            'grit_grd_file': 'policy_templates.grd',
            'grit_info_cmd': ['python', '<(DEPTH)/tools/grit/grit_info.py',
                              '<@(grit_defines)'],
          },
          'includes': [ '../../../build/grit_target.gypi' ],
          'actions': [
            {
              'action_name': 'policy_templates',
              'includes': [ '../../../build/grit_action.gypi' ],
            },
          ],
          'conditions': [
            ['OS=="win"', {
              'variables': {
                'version_path': '<(grit_out_dir)/app/policy/VERSION',
                'template_files': [
                  '<!@(<(grit_info_cmd) --outputs \'<(grit_out_dir)\' <(grit_grd_file))'
                ],
              },
              'actions': [
                {
                  'action_name': 'add_version',
                  'inputs': ['../../VERSION'],
                  'outputs': ['<(version_path)'],
                  'action': ['cp', '<@(_inputs)', '<@(_outputs)'],
                },
                {
                  # Add all the templates generated at the previous step into
                  # a zip archive.
                  'action_name': 'pack_templates',
                  'variables': {
                    'zip_script':
                        'tools/build/win/make_zip_with_relative_entries.py'
                  },
                  'inputs': [
                    '<(version_path)',
                    '<@(template_files)',
                    '<(zip_script)'
                  ],
                  'outputs': [
                    '<(PRODUCT_DIR)/policy_templates.zip'
                  ],
                  'action': [
                    'python',
                    '<(zip_script)',
                    '<@(_outputs)',
                    '<(grit_out_dir)/app/policy',
                    '<@(template_files)',
                    '<(version_path)'
                  ],
                  'message': 'Packing generated templates into <(_outputs)',
                }
              ]
            }],
          ],  # conditions
        },
      ],  # 'targets'
    }],  # OS=="win" or OS=="mac" or OS=="linux"
    ['OS=="mac"', {
      'targets': [
        {
          # This is the bundle of the manifest file of Chrome.
          # It contains the manifest file and its string tables.
          'target_name': 'chrome_manifest_bundle',
          'type': 'loadable_module',
          'mac_bundle': 1,
          'product_extension': 'manifest',
          'product_name': '<(mac_bundle_id)',
          'variables': {
            # This avoids stripping debugging symbols from the target, which
            # would fail because there is no binary code here.
            'mac_strip': 0,
          },
          'dependencies': [
             # Provides app-Manifest.plist and its string tables:
            'policy_templates',
          ],
          'actions': [
            {
              'action_name': 'Copy MCX manifest file to manifest bundle',
              'inputs': [
                '<(grit_out_dir)/app/policy/mac/app-Manifest.plist',
              ],
              'outputs': [
                '<(INTERMEDIATE_DIR)/app_manifest/<(mac_bundle_id).manifest',
              ],
              'action': [
                'cp',
                '<@(_inputs)',
                '<@(_outputs)',
              ],
              'message':
                'Copying the MCX policy manifest file to the manifest bundle',
              'process_outputs_as_mac_bundle_resources': 1,
            },
            {
              'action_name':
                'Copy Localizable.strings files to manifest bundle',
              'variables': {
                'input_path': '<(grit_out_dir)/app/policy/mac/strings',
                # Directory to collect the Localizable.strings files before
                # they are copied to the bundle.
                'output_path': '<(INTERMEDIATE_DIR)/app_manifest',
                # TODO(gfeher): replace this with <(locales) when we have real
                # translations
                'available_locales': 'en',
              },
              'inputs': [
                # TODO: remove this helper when we have loops in GYP
                '>!@(<(apply_locales_cmd) -d \'<(input_path)/ZZLOCALE.lproj/Localizable.strings\' <(available_locales))',
              ],
              'outputs': [
                # TODO: remove this helper when we have loops in GYP
                '>!@(<(apply_locales_cmd) -d \'<(output_path)/ZZLOCALE.lproj/Localizable.strings\' <(available_locales))',
              ],
              'action': [
                'cp', '-R',
                '<(input_path)/',
                '<(output_path)',
              ],
              'message':
                'Copy the Localizable.strings files to the manifest bundle',
              'process_outputs_as_mac_bundle_resources': 1,
            },
          ],
        },
      ]
    }]
  ],  # 'conditions'
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
