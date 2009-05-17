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
    'sources/': [
      ['exclude', '/(cocoa|gtk|win)/'],
      ['exclude', '_(cocoa|gtk|linux|mac|posix|skia|win|x)\\.(cc|mm?)$'],
      ['exclude', '/(gtk|win|x11)_[^/]*\\.cc$'],
    ],
    'conditions': [
      ['OS=="linux"', {'sources/': [
        ['include', '/gtk/'],
        ['include', '_(gtk|linux|posix|skia|x)\\.cc$'],
        ['include', '/(gtk|x11)_[^/]*\\.cc$'],
      ]}],
      ['OS=="mac"', {'sources/': [
        ['include', '/cocoa/'],
        ['include', '_(cocoa|mac|posix)\\.(cc|mm?)$'],
      ]}, { # else: OS != "mac"
        'sources/': [
          ['exclude', '\\.mm?$'],
        ],
      }],
      ['OS=="win"', {'sources/': [
        ['include', '_(win)\\.cc$'],
        ['include', '/win/'],
        ['include', '/win_[^/]*\\.cc$'],
      ]}],
    ],
  },
  'targets': [
    {
      'target_name': 'app_base',
      'type': '<(library)',
      'msvs_guid': '4631946D-7D5F-44BD-A5A8-504C0A7033BE',
      'dependencies': [
        '../base/base.gyp:base',
        '../base/base.gyp:base_gfx',
        '../net/net.gyp:net',
        '../skia/skia.gyp:skia',
        '../third_party/icu38/icu38.gyp:icui18n',
        '../third_party/icu38/icu38.gyp:icuuc',
      ],
      'include_dirs': [
        '..',
        '../chrome/third_party/wtl/include',
      ],
      'sources': [
        # All .cc, .h, and .mm files under app/ except for tests.
        'animation.cc',
        'animation.h',
        'app_paths.h',
        'app_paths.cc',
        'app_switches.h',
        'app_switches.cc',
        'drag_drop_types.cc',
        'drag_drop_types.h',
        'gfx/canvas.cc',
        'gfx/canvas.h',
        'gfx/canvas_linux.cc',
        'gfx/canvas_win.cc',
        'gfx/font.h',
        'gfx/font_gtk.cc',
        'gfx/font_mac.mm',
        'gfx/font_skia.cc',
        'gfx/font_win.cc',
        'gfx/color_utils.cc',
        'gfx/color_utils.h',
        'gfx/favicon_size.h',
        'gfx/gtk_util.cc',
        'gfx/gtk_util.h',
        'gfx/icon_util.cc',
        'gfx/icon_util.h',
        'gfx/insets.h',
        'gfx/path_gtk.cc',
        'gfx/path_win.cc',
        'gfx/path.h',
        'gfx/text_elider.cc',
        'gfx/text_elider.h',
        'l10n_util.cc',
        'l10n_util.h',
        'l10n_util_posix.cc',
        'l10n_util_win.cc',
        'l10n_util_win.h',
        'message_box_flags.h',
        'os_exchange_data_win.cc',
        'os_exchange_data_gtk.cc',
        'os_exchange_data.h',
        'resource_bundle.cc',
        'resource_bundle.h',
        'resource_bundle_win.cc',
        'resource_bundle_linux.cc',
        'resource_bundle_mac.mm',
        'slide_animation.cc',
        'slide_animation.h',
        'theme_provider.h',
        'throb_animation.cc',
        'throb_animation.h',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '..',
        ],
      },
      'conditions': [
        ['OS=="linux"', {
          'dependencies': [
            # font_gtk.cc uses fontconfig.
            # TODO(evanm): I think this is wrong; it should just use GTK.
            '../build/linux/system.gyp:fontconfig',
            '../build/linux/system.gyp:gtk',
          ],
        }],
        ['OS=="win"', {
          # TODO: remove this when chrome_resources/chrome_strings are
          #       generated by GYP.
          # The legacy vcproj we rely on places the grit output in this
          # directory, so we need to explicitly add it to our include path.
          'include_dirs': [
            '<(PRODUCT_DIR)/grit_derived_sources',
          ],
          'sources': [
            'win_util.cc',
            'win_util.h',
          ],
        }],
        ['OS!="win"', {
          'sources!': [
            'drag_drop_types.cc',
            'drag_drop_types.h',
            'gfx/icon_util.cc',
            'gfx/icon_util.h',
            'os_exchange_data.cc',
          ],
          # TODO: Move these dependencies to platform-neutral once these
          # projects are generated by GYP.
          'dependencies': [
            '../chrome/chrome.gyp:chrome_resources',
            '../chrome/chrome.gyp:chrome_strings',
          ],
          'conditions': [
            ['toolkit_views==0', {
              # Note: because of gyp predence rules this has to be defined as
              # 'sources/' rather than 'sources!'.
              'sources/': [
                ['exclude', '^os_exchange_data_gtk.cc'],
                ['exclude', '^os_exchange_data.h'],
              ],
            }],
          ],
        }],
      ],
    },
  ],
}
