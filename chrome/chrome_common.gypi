# Copyright (c) 2010 The Chromium Authors. All rights reserved.
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
              '<(DEPTH)/third_party/wtl/include',
            ],
          }],
        ],
        'sources': [
          # .cc, .h, and .mm files under chrome/common that are used on all
          # platforms, including both 32-bit and 64-bit Windows.
          # Test files are not included.
          'common/about_handler.cc',
          'common/about_handler.h',
          'common/app_mode_common_mac.h',
          'common/app_mode_common_mac.mm',
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
          'common/content_restriction.h',
          'common/content_settings.cc',
          'common/content_settings.h',
          'common/content_settings_helper.cc',
          'common/content_settings_helper.h',
          'common/content_settings_types.h',
          'common/database_messages.cc',
          'common/database_messages.h',
          'common/debug_flags.cc',
          'common/debug_flags.h',
          'common/devtools_messages.cc',
          'common/devtools_messages.h',
          'common/devtools_messages_internal.h',
          'common/dom_storage_messages.cc',
          'common/dom_storage_messages.h',
          'common/dx_diag_node.cc',
          'common/dx_diag_node.h',
          'common/file_system/webfilesystem_callback_dispatcher.cc',
          'common/file_system/webfilesystem_callback_dispatcher.h',
          'common/file_system/webfilesystem_impl.cc',
          'common/file_system/webfilesystem_impl.h',
          'common/file_system/webfilewriter_impl.cc',
          'common/file_system/webfilewriter_impl.h',
          'common/file_utilities_messages.cc',
          'common/file_utilities_messages.h',
          'common/font_descriptor_mac.h',
          'common/font_descriptor_mac.mm',
          'common/geoposition.cc',
          'common/geoposition.h',
          'common/gpu_create_command_buffer_config.cc',
          'common/gpu_create_command_buffer_config.h',
          'common/gpu_feature_flags.cc',
          'common/gpu_feature_flags.h',
          'common/gpu_info.h',
          'common/gpu_info.cc',
          'common/gpu_messages.cc',
          'common/gpu_messages.h',
          'common/gpu_messages_internal.h',
          'common/gpu_param_traits.h',
          'common/gpu_video_common.cc',
          'common/gpu_video_common.h',
          'common/indexed_db_key.cc',
          'common/indexed_db_key.h',
          'common/indexed_db_param_traits.cc',
          'common/indexed_db_param_traits.h',
          'common/logging_chrome.cc',
          'common/logging_chrome.h',
          'common/main_function_params.h',
          'common/message_router.cc',
          'common/message_router.h',
          'common/metrics_helpers.cc',
          'common/metrics_helpers.h',
          'common/mime_registry_messages.cc',
          'common/mime_registry_messages.h',
          'common/multi_process_lock.h',
          'common/multi_process_lock_linux.cc',
          'common/multi_process_lock_mac.cc',
          'common/multi_process_lock_win.cc',
          'common/nacl_cmd_line.cc',
          'common/nacl_cmd_line.h',
          'common/nacl_messages.cc',
          'common/nacl_messages.h',
          'common/nacl_messages_internal.h',
          'common/nacl_types.h',
          'common/notification_details.cc',
          'common/notification_details.h',
          'common/notification_observer.h',
          'common/notification_registrar.cc',
          'common/notification_registrar.h',
          'common/notification_service.cc',
          'common/notification_service.h',
          'common/notification_source.cc',
          'common/notification_source.h',
          'common/notification_type.h',
          'common/process_watcher_mac.cc',
          'common/process_watcher_posix.cc',
          'common/process_watcher_win.cc',
          'common/process_watcher.h',
          'common/property_bag.cc',
          'common/property_bag.h',
          'common/ref_counted_util.h',
          'common/resource_response.cc',
          'common/resource_response.h',
          'common/result_codes.h',
          'common/sandbox_init_wrapper.h',
          'common/sandbox_init_wrapper_linux.cc',
          'common/sandbox_init_wrapper_mac.cc',
          'common/sandbox_init_wrapper_win.cc',
          'common/sandbox_mac.h',
          'common/sandbox_mac.mm',
          'common/sandbox_policy.cc',
          'common/sandbox_policy.h',
          'common/serialized_script_value.cc',
          'common/serialized_script_value.h',
          'common/switch_utils.cc',
          'common/switch_utils.h',
          'common/time_format.cc',
          'common/time_format.h',
          'common/chrome_version_info.h',
          'common/chrome_version_info.cc',
          'common/webblobregistry_impl.cc',
          'common/webblobregistry_impl.h',
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
        'common_net',
        'default_plugin/default_plugin.gyp:default_plugin',
        'theme_resources',
        '../app/app.gyp:app_base',
        '../app/app.gyp:app_resources',
        '../base/base.gyp:base',
        '../base/base.gyp:base_i18n',
        '../build/temp_gyp/googleurl.gyp:googleurl',
        '../ipc/ipc.gyp:ipc',
        '../net/net.gyp:net',
        '../printing/printing.gyp:printing',
        '../skia/skia.gyp:skia',
        '../third_party/bzip2/bzip2.gyp:bzip2',
        '../third_party/icu/icu.gyp:icui18n',
        '../third_party/icu/icu.gyp:icuuc',
        '../third_party/libxml/libxml.gyp:libxml',
        '../third_party/sqlite/sqlite.gyp:sqlite',
        '../third_party/zlib/zlib.gyp:zlib',
        '../third_party/npapi/npapi.gyp:npapi',
        '../webkit/support/webkit_support.gyp:appcache',
        '../webkit/support/webkit_support.gyp:blob',
        '../webkit/support/webkit_support.gyp:glue',
      ],
      'sources': [
        # .cc, .h, and .mm files under chrome/common that are not required for
        # building 64-bit Windows targets. Test files are not included.
        'common/appcache/appcache_backend_proxy.cc',
        'common/appcache/appcache_backend_proxy.h',
        'common/appcache/appcache_dispatcher.cc',
        'common/appcache/appcache_dispatcher.h',
        'common/automation_constants.cc',
        'common/automation_constants.h',
        'common/automation_messages.cc',
        'common/automation_messages.h',
        'common/automation_messages_internal.h',
        'common/badge_util.cc',
        'common/badge_util.h',
        'common/child_process_host.cc',
        'common/child_process_host.h',
        'common/chrome_descriptors.h',
        'common/chrome_plugin_api.h',
        'common/chrome_plugin_lib.cc',
        'common/chrome_plugin_lib.h',
        'common/chrome_plugin_util.cc',
        'common/chrome_plugin_util.h',
        'common/common_glue.cc',
        'common/css_colors.h',
        'common/database_util.cc',
        'common/database_util.h',
        'common/db_message_filter.cc',
        'common/db_message_filter.h',
        'common/default_plugin.cc',
        'common/default_plugin.h',
        'common/deprecated/event_sys-inl.h',
        'common/deprecated/event_sys.h',
        'common/desktop_notifications/active_notification_tracker.cc',
        'common/desktop_notifications/active_notification_tracker.h',
        'common/dom_storage_common.h',
        'common/extensions/extension.cc',
        'common/extensions/extension.h',
        'common/extensions/extension_action.cc',
        'common/extensions/extension_action.h',
        'common/extensions/extension_constants.cc',
        'common/extensions/extension_constants.h',
        'common/extensions/extension_error_utils.cc',
        'common/extensions/extension_error_utils.h',
        'common/extensions/extension_extent.cc',
        'common/extensions/extension_extent.h',
        'common/extensions/extension_file_util.cc',
        'common/extensions/extension_file_util.h',
        'common/extensions/extension_icon_set.cc',
        'common/extensions/extension_icon_set.h',
        'common/extensions/extension_l10n_util.cc',
        'common/extensions/extension_l10n_util.h',
        'common/extensions/extension_localization_peer.cc',
        'common/extensions/extension_localization_peer.h',
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
        'common/file_system/file_system_dispatcher.cc',
        'common/file_system/file_system_dispatcher.h',
        'common/font_loader_mac.h',
        'common/font_loader_mac.mm',
        'common/gears_api.h',
        'common/gpu_plugin.cc',
        'common/gpu_plugin.h',
        'common/important_file_writer.cc',
        'common/important_file_writer.h',
        'common/json_pref_store.cc',
        'common/json_pref_store.h',
        'common/json_schema_validator.cc',
        'common/json_schema_validator.h',
        'common/jstemplate_builder.cc',
        'common/jstemplate_builder.h',
        'common/libxml_utils.cc',
        'common/libxml_utils.h',
        'common/mru_cache.h',
        'common/native_web_keyboard_event.h',
        'common/native_web_keyboard_event_linux.cc',
        'common/native_web_keyboard_event_mac.mm',
        'common/native_web_keyboard_event_win.cc',
        'common/native_window_notification_source.h',
        'common/navigation_gesture.h',
        'common/navigation_types.h',
        'common/page_transition_types.cc',
        'common/page_transition_types.h',
        'common/page_type.h',
        'common/page_zoom.h',
        'common/pepper_plugin_registry.cc',
        'common/pepper_plugin_registry.h',
        'common/plugin_carbon_interpose_constants_mac.cc',
        'common/plugin_carbon_interpose_constants_mac.h',
        'common/plugin_messages.cc',
        'common/plugin_messages.h',
        'common/plugin_messages_internal.h',
        'common/persistent_pref_store.h',
        'common/pref_store.h',
        'common/render_messages.cc',
        'common/render_messages.h',
        'common/render_messages_internal.h',
        'common/render_messages_params.cc',
        'common/render_messages_params.h',
        'common/renderer_preferences.cc',
        'common/renderer_preferences.h',
        'common/resource_dispatcher.cc',
        'common/resource_dispatcher.h',
        'common/security_filter_peer.cc',
        'common/security_filter_peer.h',
        'common/security_style.h',
        'common/service_messages.cc',
        'common/service_messages.h',
        'common/service_messages_internal.h',
        'common/service_process_util.cc',
        'common/service_process_util.h',
        'common/service_process_util_posix.cc',
        'common/service_process_util_win.cc',
        'common/socket_stream_dispatcher.cc',
        'common/socket_stream_dispatcher.h',
        'common/speech_input_result.h',
        'common/spellcheck_common.cc',
        'common/spellcheck_common.h',
        'common/sqlite_compiled_statement.cc',
        'common/sqlite_compiled_statement.h',
        'common/sqlite_utils.cc',
        'common/sqlite_utils.h',
        'common/thumbnail_score.cc',
        'common/thumbnail_score.h',
        'common/url_constants.cc',
        'common/url_constants.h',
        'common/utility_messages.cc',
        'common/utility_messages.h',
        'common/utility_messages_internal.h',
        'common/view_types.cc',
        'common/view_types.h',
        'common/visitedlink_common.cc',
        'common/visitedlink_common.h',
        'common/web_apps.cc',
        'common/web_apps.h',
        'common/web_database_observer_impl.cc',
        'common/web_database_observer_impl.h',
        'common/web_resource/web_resource_unpacker.cc',
        'common/web_resource/web_resource_unpacker.h',
        'common/webkit_param_traits.cc',
        'common/webkit_param_traits.h',
        'common/webmessageportchannel_impl.cc',
        'common/webmessageportchannel_impl.h',
        'common/window_container_type.cc',
        'common/window_container_type.h',
        'common/worker_messages.cc',
        'common/worker_messages.h',
        'common/worker_messages_internal.h',
        'common/worker_thread_ticker.cc',
        'common/worker_thread_ticker.h',
        'common/zip.cc',  # Requires zlib directly.
        'common/zip.h',
      ],
      'conditions': [
        ['OS=="linux" or OS=="freebsd" or OS=="openbsd"', {
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
        [ 'OS == "linux" or OS == "freebsd" or OS == "openbsd" or OS == "solaris"', {
          'include_dirs': [
            '<(SHARED_INTERMEDIATE_DIR)',
          ],
          # Because posix_version generates a header, we must set the
          # hard_dependency flag.
          'hard_dependency': 1,
          'actions': [
            {
              'action_name': 'posix_version',
              'variables': {
                'lastchange_path':
                  '<(SHARED_INTERMEDIATE_DIR)/build/LASTCHANGE',
                'version_py_path': 'tools/build/version.py',
                'version_path': 'VERSION',
                'template_input_path': 'common/chrome_version_info_posix.h.version',
              },
              'conditions': [
                [ 'branding == "Chrome"', {
                  'variables': {
                     'branding_path':
                       'app/theme/google_chrome/BRANDING',
                  },
                }, { # else branding!="Chrome"
                  'variables': {
                     'branding_path':
                       'app/theme/chromium/BRANDING',
                  },
                }],
              ],
              'inputs': [
                '<(template_input_path)',
                '<(version_path)',
                '<(branding_path)',
                '<(lastchange_path)',
              ],
              'outputs': [
                '<(SHARED_INTERMEDIATE_DIR)/chrome/common/chrome_version_info_posix.h',
              ],
              'action': [
                'python',
                '<(version_py_path)',
                '-f', '<(version_path)',
                '-f', '<(branding_path)',
                '-f', '<(lastchange_path)',
                '<(template_input_path)',
                '<@(_outputs)',
              ],
              'message': 'Generating version information',
            },
          ],
        }],
        ['OS=="linux" and selinux==1', {
          'dependencies': [
            '../build/linux/system.gyp:selinux',
          ],
        }],
        ['OS=="mac"', {
          'sources!': [
            'common/process_watcher_posix.cc',
          ],
          'link_settings': {
            'mac_bundle_resources': [
              'common/common.sb',
            ],
          },
        }],
        ['OS!="win"', {
          'sources!': [
            'common/sandbox_policy.cc',
          ],
        }],
        ['remoting==1', {
          'dependencies': [
            '../remoting/remoting.gyp:chromoting_plugin',
          ],
        }],
      ],
      'export_dependent_settings': [
        '../app/app.gyp:app_base',
      ],
    },
    {
      'target_name': 'common_net',
      'type': '<(library)',
      'sources': [
        'common/net/http_return.h',
        'common/net/net_resource_provider.cc',
        'common/net/net_resource_provider.h',
        'common/net/predictor_common.h',
        'common/net/raw_host_resolver_proc.cc',
        'common/net/raw_host_resolver_proc.h',
        'common/net/socket_stream.h',
        'common/net/url_fetcher.cc',
        'common/net/url_fetcher.h',
        'common/net/url_request_context_getter.cc',
        'common/net/url_request_context_getter.h',
        'common/net/url_request_intercept_job.cc',
        'common/net/url_request_intercept_job.h',
        'common/net/gaia/gaia_auth_consumer.h',
        'common/net/gaia/gaia_auth_fetcher.cc',
        'common/net/gaia/gaia_auth_fetcher.h',
        'common/net/gaia/gaia_authenticator.cc',
        'common/net/gaia/gaia_authenticator.h',
        'common/net/gaia/google_service_auth_error.h',
        'common/net/x509_certificate_model.cc',
        'common/net/x509_certificate_model_nss.cc',
        'common/net/x509_certificate_model_openssl.cc',
        'common/net/x509_certificate_model.h',
      ],
      'dependencies': [
        'chrome_resources',
        'chrome_strings',
        '../app/app.gyp:app_base',
        '../base/base.gyp:base',
        '../net/net.gyp:net_resources',
        '../net/net.gyp:net',
        '../third_party/icu/icu.gyp:icui18n',
        '../third_party/icu/icu.gyp:icuuc',
      ],
      'conditions': [
        [ 'OS == "linux" or OS == "freebsd" or OS == "openbsd"', {
            'conditions': [
              ['use_openssl==1', {
                 'dependencies': [
                   '../third_party/openssl/openssl.gyp:openssl',
                 ],
               },
               { # else !use_openssl
                'dependencies': [
                  '../build/linux/system.gyp:nss',
                ],
               },
              ],
            ],
          },
          {  # else: OS is not in the above list
            'sources!': [
              'common/net/x509_certificate_model_nss.cc',
              'common/net/x509_certificate_model_openssl.cc',
            ],
          },
        ],
        ['use_openssl==1', {
            'sources!': [
              'common/net/x509_certificate_model_nss.cc',
            ],
          },
          {  # else !use_openssl: remove the unneeded files
            'sources!': [
              'common/net/x509_certificate_model_openssl.cc',
            ],
          },
        ],
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
            '../third_party/libxml/libxml.gyp:libxml',
          ],
          'include_dirs': [
            '../third_party/npapi',
            '../third_party/icu/public/i18n',
            '../third_party/icu/public/common',
            # We usually get these skia directories by adding a dependency on
            # skia, bu we don't need it for NaCl's 64-bit Windows support. The
            # directories are required for resolving the includes in any case.
            '../third_party/skia/include/config',
            '../third_party/skia/include/core',
            '../skia/config',
            '../skia/config/win',
          ],
          'defines': [
            'EXCLUDE_SKIA_DEPENDENCIES',
            '<@(nacl_win64_defines)',
          ],
          'sources': [
            '../webkit/glue/webkit_glue_dummy.cc',
            'common/file_system/file_system_dispatcher_dummy.cc',
            'common/resource_dispatcher_dummy.cc',
            'common/socket_stream_dispatcher_dummy.cc',
            'common/url_constants.cc',
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
