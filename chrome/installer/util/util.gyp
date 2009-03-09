{
  'includes': [
    '../../../build/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'installer_util',
      'type': 'static_library',
      'dependencies': [
        'prebuild/util_prebuild.gyp:installer_util_prebuild',
        '../../chrome.gyp:common',
        '../../chrome.gyp:resources',
        '../../../net/net.gyp:net_resources',
        '../../../media/media.gyp:media',
        '../../../skia/skia.gyp:skia',
        '../../../third_party/icu38/icu38.gyp:icui18n',
        '../../../third_party/icu38/icu38.gyp:icuuc',
        '../../../third_party/libxml/libxml.gyp:libxml',
        '../../../third_party/npapi/npapi.gyp:npapi',
        '../../third_party/hunspell/hunspell.gyp:hunspell',
      ],
      'include_dirs': [
        '../../..',
        # TODO(bradnelson): this should probably come from a using using_lzma
        # file but I'll put it here for now.
        '../../../third_party/lzma_sdk',
      ],
      'defines': [
        # TODO(bradnelson): this should probably come from a using using_lzma
        # file but I'll put it here for now.
        '_LZMA_IN_CB',
      ],
      'sources': [
        'browser_distribution.cc',
        'browser_distribution.h',
        'compat_checks.cc',
        'compat_checks.h',
        'copy_tree_work_item.cc',
        'copy_tree_work_item.h',
        'create_dir_work_item.cc',
        'create_dir_work_item.h',
        'create_reg_key_work_item.cc',
        'create_reg_key_work_item.h',
        'delete_reg_value_work_item.cc',
        'delete_reg_value_work_item.h',
        'delete_tree_work_item.cc',
        'delete_tree_work_item.h',
        'google_chrome_distribution.cc',
        'google_chrome_distribution.h',
        'google_update_constants.cc',
        'google_update_constants.h',
        'google_update_settings.cc',
        'google_update_settings.h',
        'helper.cc',
        'helper.h',
        'html_dialog.h',
        'html_dialog_impl.cc',
        'install_util.cc',
        'install_util.h',
        'l10n_string_util.cc',
        'l10n_string_util.h',
        'logging_installer.cc',
        'logging_installer.h',
        'lzma_util.cc',
        'lzma_util.h',
        'master_preferences.cc',
        'master_preferences.h',
        'move_tree_work_item.cc',
        'move_tree_work_item.h',
        'set_reg_value_work_item.cc',
        'set_reg_value_work_item.h',
        'shell_util.cc',
        'shell_util.h',
        'util_constants.cc',
        'util_constants.h',
        'version.cc',
        'version.h',
        'work_item.cc',
        'work_item.h',
        'work_item_list.cc',
        'work_item_list.h',
      ],
    },
  ],
}
