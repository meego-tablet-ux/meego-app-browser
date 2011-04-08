# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [
    {
      'target_name': 'content_common',
      'type': '<(library)',
      'dependencies': [
        '../ipc/ipc.gyp:ipc',
        '../skia/skia.gyp:skia',
        '../third_party/icu/icu.gyp:icuuc',
        '../third_party/npapi/npapi.gyp:npapi',
        '../third_party/WebKit/Source/WebKit/chromium/WebKit.gyp:webkit',
        '../ui/gfx/gl/gl.gyp:gl',
        '../webkit/support/webkit_support.gyp:appcache',
        '../webkit/support/webkit_support.gyp:blob',
        '../webkit/support/webkit_support.gyp:database',
        '../webkit/support/webkit_support.gyp:fileapi',
      ],
      'include_dirs': [
        '..',
      ],
      'sources': [
        'common/appcache/appcache_backend_proxy.cc',
        'common/appcache/appcache_backend_proxy.h',
        'common/appcache/appcache_dispatcher.cc',
        'common/appcache/appcache_dispatcher.h',
        'common/appcache_messages.h',
        'common/audio_messages.h',
        'common/audio_stream_state.h',
        'common/child_process.cc',
        'common/child_process.h',
        'common/child_process_host.cc',
        'common/child_process_host.h',
        'common/child_process_info.cc',
        'common/child_process_info.h',
        'common/child_process_messages.h',
        'common/child_thread.cc',
        'common/child_thread.h',
        'common/child_trace_message_filter.cc',
        'common/child_trace_message_filter.h',
        'common/chrome_application_mac.h',
        'common/chrome_application_mac.mm',
        'common/chrome_descriptors.h',
        'common/clipboard_messages.h',
        'common/common_param_traits.cc',
        'common/common_param_traits.h',
        'common/content_message_generator.cc',
        'common/content_message_generator.h',
        'common/content_client.cc',
        'common/content_client.h',
        'common/content_constants.cc',
        'common/content_constants.h',
        'common/content_paths.cc',
        'common/content_paths.h',
        'common/content_switches.cc',
        'common/content_switches.h',
        'common/css_colors.h',
        'common/database_messages.h',
        'common/database_util.cc',
        'common/database_util.h',
        'common/db_message_filter.cc',
        'common/db_message_filter.h',
        'common/debug_flags.cc',
        'common/debug_flags.h',
        'common/desktop_notification_messages.h',
        'common/device_orientation_messages.h',
        'common/dom_storage_common.h',
        'common/dom_storage_messages.h',
        'common/drag_messages.h',
        'common/dx_diag_node.cc',
        'common/dx_diag_node.h',
        'common/edit_command.h',
        'common/file_path_watcher/file_path_watcher.cc',
        'common/file_path_watcher/file_path_watcher.h',
        'common/file_path_watcher/file_path_watcher_inotify.cc',
        'common/file_path_watcher/file_path_watcher_mac.cc',
        'common/file_path_watcher/file_path_watcher_win.cc',
        'common/file_system/file_system_dispatcher.cc',
        'common/file_system/file_system_dispatcher.h',
        'common/file_system/webfilesystem_callback_dispatcher.cc',
        'common/file_system/webfilesystem_callback_dispatcher.h',
        'common/file_system/webfilesystem_impl.cc',
        'common/file_system/webfilesystem_impl.h',
        'common/file_system/webfilewriter_impl.cc',
        'common/file_system/webfilewriter_impl.h',
        'common/file_system_messages.h',
        'common/file_utilities_messages.h',
        'common/font_config_ipc_linux.cc',
        'common/font_config_ipc_linux.h',
        'common/font_descriptor_mac.h',
        'common/font_descriptor_mac.mm',
        'common/font_loader_mac.h',
        'common/font_loader_mac.mm',
        'common/gpu_feature_flags.cc',
        'common/gpu_feature_flags.h',
        'common/geolocation_messages.h',
        'common/geoposition.cc',
        'common/geoposition.h',
        'common/gpu/content_gpu_client.h',
        'common/gpu/gpu_channel.cc',
        'common/gpu/gpu_channel.h',
        'common/gpu/gpu_channel_manager.cc',
        'common/gpu/gpu_channel_manager.h',
        'common/gpu/gpu_command_buffer_stub.cc',
        'common/gpu/gpu_command_buffer_stub.h',
        'common/gpu/gpu_config.h',
        'common/gpu/gpu_info.cc',
        'common/gpu/gpu_info.h',
        'common/gpu/gpu_video_decoder.cc',
        'common/gpu/gpu_video_decoder.h',
        'common/gpu/gpu_video_service.cc',
        'common/gpu/gpu_video_service.h',
        'common/gpu/media/gpu_video_device.h',
        'common/gpu/media/fake_gl_video_decode_engine.cc',
        'common/gpu/media/fake_gl_video_decode_engine.h',
        'common/gpu/media/fake_gl_video_device.cc',
        'common/gpu/media/fake_gl_video_device.h',
        'common/gpu_process_launch_causes.h',
        'common/gpu_messages.h',
        'common/hi_res_timer_manager_posix.cc',
        'common/hi_res_timer_manager_win.cc',
        'common/hi_res_timer_manager.h',
        'common/indexed_db_key.cc',
        'common/indexed_db_key.h',
        'common/indexed_db_messages.h',
        'common/indexed_db_param_traits.cc',
        'common/indexed_db_param_traits.h',
        'common/main_function_params.h',
        'common/message_router.cc',
        'common/message_router.h',
        'common/mime_registry_messages.h',
        'common/mru_cache.h',
        'common/native_web_keyboard_event.h',
        'common/native_web_keyboard_event_linux.cc',
        'common/native_web_keyboard_event_mac.mm',
        'common/native_web_keyboard_event_win.cc',
        'common/navigation_gesture.h',
        'common/navigation_types.h',
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
        'common/p2p_messages.h',
        'common/p2p_sockets.h',
        'common/page_transition_types.cc',
        'common/page_transition_types.h',
        'common/page_type.h',
        'common/page_zoom.h',
        'common/pepper_file_messages.cc',
        'common/pepper_file_messages.h',
        'common/pepper_messages.cc',
        'common/pepper_messages.h',
        'common/plugin_carbon_interpose_constants_mac.cc',
        'common/plugin_carbon_interpose_constants_mac.h',
        'common/plugin_messages.h',
        'common/process_watcher.h',
        'common/process_watcher_mac.cc',
        'common/process_watcher_posix.cc',
        'common/process_watcher_win.cc',
        'common/property_bag.cc',
        'common/property_bag.h',
        'common/renderer_preferences.cc',
        'common/renderer_preferences.h',
        'common/resource_dispatcher.cc',
        'common/resource_dispatcher.h',
        'common/resource_messages.h',
        'common/resource_response.cc',
        'common/resource_response.h',
        'common/sandbox_init_wrapper.h',
        'common/sandbox_init_wrapper_linux.cc',
        'common/sandbox_init_wrapper_mac.cc',
        'common/sandbox_init_wrapper_win.cc',
        'common/sandbox_mac.h',
        'common/sandbox_mac.mm',
        'common/sandbox_methods_linux.h',
        'common/section_util_win.cc',
        'common/section_util_win.h',
        'common/serialized_script_value.cc',
        'common/serialized_script_value.h',
        'common/set_process_title.cc',
        'common/set_process_title.h',
        'common/set_process_title_linux.cc',
        'common/set_process_title_linux.h',
        'common/socket_stream.h',
        'common/socket_stream_dispatcher.cc',
        'common/socket_stream_dispatcher.h',
        'common/socket_stream_messages.h',
        'common/speech_input_messages.h',
        'common/speech_input_result.h',
        'common/unix_domain_socket_posix.cc',
        'common/unix_domain_socket_posix.h',
        'common/view_messages.h',
        'common/web_database_observer_impl.cc',
        'common/web_database_observer_impl.h',
        'common/webblobregistry_impl.cc',
        'common/webblobregistry_impl.h',
        'common/webblob_messages.h',
        'common/webmessageportchannel_impl.cc',
        'common/webmessageportchannel_impl.h',
        'common/window_container_type.cc',
        'common/window_container_type.h',
        'common/worker_messages.h',
      ],
      'conditions': [
        ['OS=="win"', {
          'msvs_guid': '062E9260-304A-4657-A74C-0D3AA1A0A0A4',
          'sources': [
            'common/gpu/media/mft_angle_video_device.cc',
            'common/gpu/media/mft_angle_video_device.h',
          ],
          'include_dirs': [
            '<(DEPTH)/third_party/angle/include',
            '<(DEPTH)/third_party/angle/src',
            '<(DEPTH)/third_party/wtl/include',
            '$(DXSDK_DIR)/include',
          ],
          'dependencies': [
            '../third_party/angle/src/build_angle.gyp:libEGL',
            '../third_party/angle/src/build_angle.gyp:libGLESv2',
          ],
        }],
        ['OS!="linux"', {
          'sources!': [
            'common/file_path_watcher/file_path_watcher_inotify.cc',
          ],
        }],
        ['OS=="freebsd" or OS=="openbsd"', {
          'sources': [
            'common/file_path_watcher/file_path_watcher_stub.cc',
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
        ['OS=="linux" or OS=="freebsd" or OS=="openbsd"', {
          'dependencies': [
            '../build/linux/system.gyp:gtk',
          ],
        }],
        ['OS=="linux" and target_arch!="arm"', {
          'sources': [
            'common/gpu/x_util.cc',
            'common/gpu/x_util.h',
          ],
        }],
        ['toolkit_views==1', {
          'sources': [
            'common/native_web_keyboard_event_views.cc',
          ],
        }],
        ['enable_gpu==1', {
          'dependencies': [
            '../gpu/gpu.gyp:command_buffer_service',
          ],
        }],
      ],
    },
  ],
}
