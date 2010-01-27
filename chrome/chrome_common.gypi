# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'target_defaults': {
    'variables': {
      'chrome_common_target': 0,
    },
    'target_conditions': [
      ['chrome_common_target==1', {
        'include_dirs': [
          '..',
        ],
        'conditions': [
          ['OS=="win"', {
            'include_dirs': [
              'third_party/wtl/include',
            ],
          }, { # else: OS != "win"
            'sources!': [
              'common/temp_scaffolding_stubs.h',
            ],
          }],
          ['OS=="win" or OS=="linux"', {
            'sources!': [
              'common/hi_res_timer_manager.cc',
              'common/hi_res_timer_manager.h',
              'common/temp_scaffolding_stubs.cc',
            ],
          }],
        ],
        'sources': [
          # .cc, .h, and .mm files under chrome/common that are used on all
          # platforms, including both 32-bit and 64-bit Windows.
          # Test files are not included.
          'common/accessibility_events.h',
          'common/accessibility_events.cc',
          'common/bindings_policy.h',
          'common/child_process.cc',
          'common/child_process.h',
          'common/child_process_info.cc',
          'common/child_process_info.h',
          'common/child_process_logging.h',
          'common/child_process_logging_linux.cc',
          'common/child_process_logging_mac.mm',
          'common/child_process_logging_win.cc',
          'common/child_thread.cc',
          'common/child_thread.h',
          'common/chrome_counters.cc',
          'common/chrome_counters.h',
          'common/common_param_traits.cc',
          'common/common_param_traits.h',
          'common/debug_flags.cc',
          'common/debug_flags.h',
          'common/devtools_messages.h',
          'common/devtools_messages_internal.h',
          'common/gpu_messages.h',
          'common/gpu_messages_internal.h',
          'common/logging_chrome.cc',
          'common/logging_chrome.h',
          'common/main_function_params.h',
          'common/message_router.cc',
          'common/message_router.h',
          'common/nacl_messages.h',
          'common/nacl_messages_internal.h',
          'common/notification_details.h',
          'common/notification_observer.h',
          'common/notification_registrar.cc',
          'common/notification_registrar.h',
          'common/notification_service.cc',
          'common/notification_service.h',
          'common/notification_source.h',
          'common/notification_type.h',
          'common/process_watcher_mac.cc',
          'common/process_watcher_posix.cc',
          'common/process_watcher_win.cc',
          'common/process_watcher.h',
          'common/property_bag.cc',
          'common/property_bag.h',
          'common/ref_counted_util.h',
          'common/result_codes.h',
          'common/sandbox_init_wrapper.h',
          'common/sandbox_init_wrapper_linux.cc',
          'common/sandbox_init_wrapper_mac.cc',
          'common/sandbox_init_wrapper_win.cc',
          'common/sandbox_mac.h',
          'common/sandbox_mac.mm',
          'common/sandbox_policy.cc',
          'common/sandbox_policy.h',
          'common/task_queue.cc',
          'common/task_queue.h',
          'common/time_format.cc',
          'common/time_format.h',
          'common/transport_dib.h',
          'common/win_safe_util.cc',
          'common/win_safe_util.h',
        ],
      }],
    ],
  },
  'targets': [
    {
      'target_name': 'common',
      'type': '<(library)',
      'msvs_guid': '899F1280-3441-4D1F-BA04-CCD6208D9146',
      'variables': {
        'chrome_common_target': 1,
      },
      # TODO(gregoryd): This could be shared with the 64-bit target, but
      # it does not work due to a gyp issue.
      'direct_dependent_settings': {
        'include_dirs': [
          '..',
        ],
      },
      'dependencies': [
        # TODO(gregoryd): chrome_resources and chrome_strings could be
        #  shared with the 64-bit target, but it does not work due to a gyp
        # issue.
        'chrome_resources',
        'chrome_strings',
        'common_constants',
        'theme_resources',
        '../app/app.gyp:app_base',
        '../app/app.gyp:app_resources',
        '../base/base.gyp:base',
        '../base/base.gyp:base_i18n',
        '../build/temp_gyp/googleurl.gyp:googleurl',
        '../ipc/ipc.gyp:ipc',
        '../net/net.gyp:net',
        '../net/net.gyp:net_resources',
        '../skia/skia.gyp:skia',
        '../third_party/icu/icu.gyp:icui18n',
        '../third_party/icu/icu.gyp:icuuc',
        '../third_party/libxml/libxml.gyp:libxml',
        '../third_party/sqlite/sqlite.gyp:sqlite',
        '../third_party/zlib/zlib.gyp:zlib',
        '../third_party/npapi/npapi.gyp:npapi',
        '../webkit/webkit.gyp:appcache',
        '../webkit/webkit.gyp:glue',
      ],
      'sources': [
        # .cc, .h, and .mm files under chrome/common that are not required for
        # building 64-bit Windows targets. Test files are not included.
        'common/desktop_notifications/active_notification_tracker.h',
        'common/desktop_notifications/active_notification_tracker.cc',
        'common/extensions/extension.cc',
        'common/extensions/extension.h',
        'common/extensions/extension_constants.cc',
        'common/extensions/extension_constants.h',
        'common/extensions/extension_error_reporter.cc',
        'common/extensions/extension_error_reporter.h',
        'common/extensions/extension_error_utils.cc',
        'common/extensions/extension_error_utils.h',
        'common/extensions/extension_action.cc',
        'common/extensions/extension_action.h',
        'common/extensions/extension_l10n_util.cc',
        'common/extensions/extension_l10n_util.h',
        'common/extensions/extension_message_bundle.cc',
        'common/extensions/extension_message_bundle.h',
        'common/extensions/extension_resource.cc',
        'common/extensions/extension_resource.h',
        'common/extensions/extension_unpacker.cc',
        'common/extensions/extension_unpacker.h',
        'common/extensions/update_manifest.cc',
        'common/extensions/update_manifest.h',
        'common/extensions/url_pattern.cc',
        'common/extensions/url_pattern.h',
        'common/extensions/user_script.cc',
        'common/extensions/user_script.h',
        'common/gfx/utils.h',
        'common/net/dns.h',
        'common/net/net_resource_provider.cc',
        'common/net/net_resource_provider.h',
        'common/net/socket_stream.h',
        'common/net/url_request_intercept_job.cc',
        'common/net/url_request_intercept_job.h',
        'common/web_resource/web_resource_unpacker.cc',
        'common/web_resource/web_resource_unpacker.h',
        'common/appcache/appcache_backend_proxy.cc',
        'common/appcache/appcache_backend_proxy.h',
        'common/appcache/appcache_dispatcher.cc',
        'common/appcache/appcache_dispatcher.h',
        'common/appcache/appcache_dispatcher_host.cc',
        'common/appcache/appcache_dispatcher_host.h',
        'common/appcache/appcache_frontend_proxy.cc',
        'common/appcache/appcache_frontend_proxy.h',
        'common/appcache/chrome_appcache_service.cc',
        'common/appcache/chrome_appcache_service.h',
        'common/automation_constants.cc',
        'common/automation_constants.h',
        'common/child_process_host.cc',
        'common/child_process_host.h',
        'common/chrome_descriptors.h',
        'common/chrome_plugin_api.h',
        'common/chrome_plugin_lib.cc',
        'common/chrome_plugin_lib.h',
        'common/chrome_plugin_util.cc',
        'common/chrome_plugin_util.h',
        'common/command_buffer_messages.h',
        'common/command_buffer_messages_internal.h',
        'common/common_glue.cc',
        'common/css_colors.h',
        'common/db_message_filter.cc',
        'common/db_message_filter.h',
        'common/dom_storage_common.h',
        'common/filter_policy.h',
        'common/gears_api.h',
        'common/gpu_plugin.cc',
        'common/gpu_plugin.h',
        'common/gtk_tree.cc',
        'common/gtk_tree.h',
        'common/gtk_util.cc',
        'common/gtk_util.h',
        'common/histogram_synchronizer.cc',
        'common/histogram_synchronizer.h',
        'common/important_file_writer.cc',
        'common/important_file_writer.h',
        'common/jstemplate_builder.cc',
        'common/jstemplate_builder.h',
        'common/libxml_utils.cc',
        'common/libxml_utils.h',
        'common/mru_cache.h',
        'common/navigation_gesture.h',
        'common/navigation_types.h',
        'common/native_web_keyboard_event.h',
        'common/native_web_keyboard_event_linux.cc',
        'common/native_web_keyboard_event_mac.mm',
        'common/native_web_keyboard_event_win.cc',
        'common/native_window_notification_source.h',
        'common/owned_widget_gtk.cc',
        'common/owned_widget_gtk.h',
        'common/page_transition_types.h',
        'common/page_zoom.h',
        'common/platform_util.h',
        'common/platform_util_linux.cc',
        'common/platform_util_mac.mm',
        'common/platform_util_win.cc',
        'common/plugin_carbon_interpose_constants_mac.h',
        'common/plugin_carbon_interpose_constants_mac.cc',
        'common/plugin_messages.h',
        'common/plugin_messages_internal.h',
        'common/pref_member.cc',
        'common/pref_member.h',
        'common/pref_service.cc',
        'common/pref_service.h',
        'common/render_messages.h',
        'common/render_messages_internal.h',
        'common/renderer_preferences.h',
        'common/resource_dispatcher.cc',
        'common/resource_dispatcher.h',
        'common/security_filter_peer.cc',
        'common/security_filter_peer.h',
        'common/socket_stream_dispatcher.cc',
        'common/socket_stream_dispatcher.h',
        'common/spellcheck_common.cc',
        'common/spellcheck_common.h',
        'common/sqlite_compiled_statement.cc',
        'common/sqlite_compiled_statement.h',
        'common/sqlite_utils.cc',
        'common/sqlite_utils.h',
        'common/temp_scaffolding_stubs.cc',
        'common/temp_scaffolding_stubs.h',
        'common/thumbnail_score.cc',
        'common/thumbnail_score.h',
        'common/transport_dib_linux.cc',
        'common/transport_dib_mac.cc',
        'common/transport_dib_win.cc',
        'common/url_constants.cc',
        'common/url_constants.h',
        'common/utility_messages.h',
        'common/utility_messages_internal.h',
        'common/view_types.cc',
        'common/view_types.h',
        'common/visitedlink_common.cc',
        'common/visitedlink_common.h',
        'common/webkit_param_traits.h',
        'common/webmessageportchannel_impl.cc',
        'common/webmessageportchannel_impl.h',
        'common/worker_messages.h',
        'common/worker_messages_internal.h',
        'common/worker_thread_ticker.cc',
        'common/worker_thread_ticker.h',
        'common/x11_util.cc',
        'common/x11_util.h',
        'common/x11_util_internal.h',
        'common/zip.cc',  # Requires zlib directly.
        'common/zip.h',
      ],
      'conditions': [
        ['OS=="linux"', {
          'dependencies': [
            '../build/linux/system.gyp:gtk',
          ],
          'export_dependent_settings': [
            '../third_party/sqlite/sqlite.gyp:sqlite',
          ],
          'link_settings': {
            'libraries': [
              '-lX11',
              '-lXrender',
              '-lXss',
              '-lXext',
            ],
          },
        },],
        ['OS=="linux" and selinux==1', {
          'dependencies': [
            '../build/linux/system.gyp:selinux',
          ],
        }],
        ['OS=="mac"', {
          'sources!': [
            'common/process_watcher_posix.cc',
          ],
        }],
        ['OS!="win"', {
          'sources!': [
            'common/sandbox_policy.cc',
          ],
        }],
      ],
      'export_dependent_settings': [
        '../app/app.gyp:app_base',
      ],
    },
  ],
  'conditions': [
    ['OS=="win"', {
      'targets': [
        {
          'target_name': 'common_nacl_win64',
          'type': '<(library)',
          'msvs_guid': '3AB5C5E9-470C-419B-A0AE-C7381FB632FA',
          'variables': {
            'chrome_common_target': 1,
          },
          'dependencies': [
            # TODO(gregoryd): chrome_resources and chrome_strings could be
            #  shared with the 32-bit target, but it does not work due to a gyp
            # issue.
            'chrome_resources',
            'chrome_strings',
            'common_constants_win64',
            '../app/app.gyp:app_base_nacl_win64',
            '../app/app.gyp:app_resources',
            '../base/base.gyp:base_nacl_win64',
            '../ipc/ipc.gyp:ipc_win64',
          ],
          'include_dirs': [
            '../third_party/npapi',
            '../third_party/icu/public/i18n',
            '../third_party/icu/public/common',
            # We usually get these skia directories by adding a dependency on
            # skia, bu we don't need it for NaCl's 64-bit Windows support. The
            # directories are required for resolving the includes in any case.
            '../third_party/skia/include/core',
            '../skia/config',
          ],
          'defines': [
            'EXCLUDE_SKIA_DEPENDENCIES',
            '<@(nacl_win64_defines)',
          ],
          'sources': [
            '../webkit/glue/webkit_glue_dummy.cc',
            'common/resource_dispatcher_dummy.cc',
            'common/socket_stream_dispatcher_dummy.cc',
          ],
          'export_dependent_settings': [
            '../app/app.gyp:app_base_nacl_win64',
          ],
          # TODO(gregoryd): This could be shared with the 32-bit target, but
          # it does not work due to a gyp issue.
          'direct_dependent_settings': {
            'include_dirs': [
              '..',
            ],
          },
          'configurations': {
            'Common_Base': {
              'msvs_target_platform': 'x64',
            },
          },
        },
      ],
    }],
  ],
}
